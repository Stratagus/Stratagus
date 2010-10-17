//     ____                _       __               
//    / __ )____  _____   | |     / /___ ___________
//   / __  / __ \/ ___/   | | /| / / __ `/ ___/ ___/
//  / /_/ / /_/ (__  )    | |/ |/ / /_/ / /  (__  ) 
// /_____/\____/____/     |__/|__/\__,_/_/  /____/  
//                                              
//       A futuristic real-time strategy game.
//          This file is part of Bos Wars.
//
/**@name sdl.cpp - SDL video support. */
//
//      (c) Copyright 1999-2010 by Lutz Sammer, Jimmy Salmon, Nehal Mistry
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

//@{

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#ifdef DEBUG
#include <signal.h>
#endif

#include <map>
#include <string>

#include <stdlib.h>
#include <string.h>

#include <limits.h>
#ifndef _MSC_VER
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef USE_WIN32
#include "net_lowlevel.h"
#endif

#include "video.h"
#include "font.h"
#include "interface.h"
#include "network.h"
#include "ui.h"
#include "sound_server.h"
#include "sound.h"
#include "interface.h"
#include "minimap.h"
#include "widgets.h"
#include "editor.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

SDL_Surface *TheScreen; /// Internal screen

static SDL_Rect Rects[100];
static int NumRects;
GLint GLMaxTextureSize;             /// Max texture size supported on the video card
GLint GLMaxTextureSizeOverride;     /// User-specified limit for ::GLMaxTextureSize
bool GLTextureCompressionSupported; /// Is OpenGL texture compression supported
bool UseGLTextureCompression;       /// Use OpenGL texture compression

static std::map<int, std::string> Key2Str;
static std::map<std::string, int> Str2Key;

static int FrameTicks;     /// Frame length in ms
static int FrameRemainder; /// Frame remainder 0.1 ms
static int FrameFraction;  /// Frame fractional term

const EventCallback *Callbacks;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

// ARB_texture_compression
PFNGLCOMPRESSEDTEXIMAGE3DARBPROC    glCompressedTexImage3DARB;
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC    glCompressedTexImage2DARB;
PFNGLCOMPRESSEDTEXIMAGE1DARBPROC    glCompressedTexImage1DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glCompressedTexSubImage3DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glCompressedTexSubImage1DARB;
PFNGLGETCOMPRESSEDTEXIMAGEARBPROC   glGetCompressedTexImageARB;

/*----------------------------------------------------------------------------
--  Sync
----------------------------------------------------------------------------*/

/**
**  Initialise video sync.
**  Calculate the length of video frame and any simulation skips.
**
**  @see VideoSyncSpeed @see SkipFrames @see FrameTicks @see FrameRemainder
*/
void SetVideoSync(void)
{
	int ms;

	if (VideoSyncSpeed) {
		ms = (1000 * 1000 / CYCLES_PER_SECOND) / VideoSyncSpeed;
	} else {
		ms = INT_MAX;
	}
	SkipFrames = ms / 400;
	while (SkipFrames && ms / SkipFrames < 200) {
		--SkipFrames;
	}
	ms /= SkipFrames + 1;

	FrameTicks = ms / 10;
	FrameRemainder = ms % 10;
	DebugPrint("frames %d - %d.%dms\n" _C_ SkipFrames _C_ ms / 10 _C_ ms % 10);
}

/*----------------------------------------------------------------------------
--  Video
----------------------------------------------------------------------------*/

/**
**  Check if an extension is supported
*/
static bool IsExtensionSupported(const char *extension)
{
	const GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *ptr, *terminator;
	int len;

	// Extension names should not have spaces.
	ptr = (GLubyte *)strchr(extension, ' ');
	if (ptr || *extension == '\0') {
		return false;
	}

	extensions = glGetString(GL_EXTENSIONS);
	len = strlen(extension);
	start = extensions;
	while (true) {
		ptr = (GLubyte *)strstr((const char *)start, extension);
		if (!ptr) {
			break;
		}

		terminator = ptr + len;
		if (ptr == start || *(ptr - 1) == ' ') {
			if (*terminator == ' ' || *terminator == '\0') {
				return true;
			}
		}
		start = terminator;
	}
	return false;
}

/**
**  Initialize OpenGL extensions
*/
static void InitOpenGLExtensions()
{
	// ARB_texture_compression
	if (IsExtensionSupported("GL_ARB_texture_compression"))
	{
		glCompressedTexImage3DARB =
			(PFNGLCOMPRESSEDTEXIMAGE3DARBPROC)SDL_GL_GetProcAddress("glCompressedTexImage3DARB");
		glCompressedTexImage2DARB =
			(PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)SDL_GL_GetProcAddress("glCompressedTexImage2DARB");
		glCompressedTexImage1DARB =
			(PFNGLCOMPRESSEDTEXIMAGE1DARBPROC)SDL_GL_GetProcAddress("glCompressedTexImage1DARB");
		glCompressedTexSubImage3DARB =
			(PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC)SDL_GL_GetProcAddress("glCompressedTexSubImage3DARB");
		glCompressedTexSubImage2DARB =
			(PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)SDL_GL_GetProcAddress("glCompressedTexSubImage2DARB");
		glCompressedTexSubImage1DARB =
			(PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC)SDL_GL_GetProcAddress("glCompressedTexSubImage1DARB");
		glGetCompressedTexImageARB =
			(PFNGLGETCOMPRESSEDTEXIMAGEARBPROC)SDL_GL_GetProcAddress("glGetCompressedTexImageARB");

		if (glCompressedTexImage3DARB && glCompressedTexImage2DARB &&
			glCompressedTexImage1DARB && glCompressedTexSubImage3DARB &&
			glCompressedTexSubImage2DARB && glCompressedTexSubImage1DARB &&
			glGetCompressedTexImageARB)
		{
			GLTextureCompressionSupported = true;
		}
		else
		{
			GLTextureCompressionSupported = false;
		}
	}
}

/**
**  Initialize OpenGL
*/
static void InitOpenGL(void)
{
	InitOpenGLExtensions();

	glViewport(0, 0, (GLsizei)Video.Width, (GLsizei)Video.Height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, Video.Width, Video.Height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375, 0.375, 0.);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glShadeModel(GL_FLAT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &GLMaxTextureSize);
	if (GLMaxTextureSize == 0) {
		// FIXME: try to use GL_PROXY_TEXTURE_2D to get a valid size
#if 0
		glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, size, size, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glGetTexLevelParameterfv(GL_PROXY_TEXTURE_2D, 0,
			GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
#endif
		fprintf(stderr, "GL_MAX_TEXTURE_SIZE is 0, using 256 by default\n");
		GLMaxTextureSize = 256;
	}
	if (GLMaxTextureSize > GLMaxTextureSizeOverride
	    && GLMaxTextureSizeOverride > 0) {
		GLMaxTextureSize = GLMaxTextureSizeOverride;
	}
}

void ReloadOpenGL()
{
	InitOpenGL();
	ReloadGraphics();
	ReloadFonts();
	UI.Minimap.Reload();
}

#if defined(DEBUG) && !defined(USE_WIN32)
static void CleanExit(int signum)
{
	// Clean SDL
	SDL_Quit();
	// Reestablish normal behaviour for next abort call
	signal(SIGABRT, SIG_DFL);
	// Generates a core dump
	abort();
}
#endif

/**
**  Initialize SDLKey to string map
*/
static void InitKey2Str()
{
	Str2Key[_("esc")] = SDLK_ESCAPE;

	if (!Key2Str.empty()) {
		return;
	}

	int i;
	char str[20];

	Key2Str[SDLK_BACKSPACE] = "backspace";
	Key2Str[SDLK_TAB] = "tab";
	Key2Str[SDLK_CLEAR] = "clear";
	Key2Str[SDLK_RETURN] = "return";
	Key2Str[SDLK_PAUSE] = "pause";
	Key2Str[SDLK_ESCAPE] = "escape";
	Key2Str[SDLK_SPACE] = " ";
	Key2Str[SDLK_EXCLAIM] = "!";
	Key2Str[SDLK_QUOTEDBL] = "\"";
	Key2Str[SDLK_HASH] = "#";
	Key2Str[SDLK_DOLLAR] = "$";
	Key2Str[SDLK_AMPERSAND] = "&";
	Key2Str[SDLK_QUOTE] = "'";
	Key2Str[SDLK_LEFTPAREN] = "(";
	Key2Str[SDLK_RIGHTPAREN] = ")";
	Key2Str[SDLK_ASTERISK] = "*";
	Key2Str[SDLK_PLUS] = "+";
	Key2Str[SDLK_COMMA] = ",";
	Key2Str[SDLK_MINUS] = "-";
	Key2Str[SDLK_PERIOD] = ".";
	Key2Str[SDLK_SLASH] = "/";

	str[1] = '\0';
	for (i = SDLK_0; i <= SDLK_9; ++i) {
		str[0] = i;
		Key2Str[i] = str;
	}

	Key2Str[SDLK_COLON] = ":";
	Key2Str[SDLK_SEMICOLON] = ";";
	Key2Str[SDLK_LESS] = "<";
	Key2Str[SDLK_EQUALS] = "=";
	Key2Str[SDLK_GREATER] = ">";
	Key2Str[SDLK_QUESTION] = "?";
	Key2Str[SDLK_AT] = "@";
	Key2Str[SDLK_LEFTBRACKET] = "[";
	Key2Str[SDLK_BACKSLASH] = "\\";
	Key2Str[SDLK_RIGHTBRACKET] = "]";
	Key2Str[SDLK_BACKQUOTE] = "`";

	str[1] = '\0';
	for (i = SDLK_a; i <= SDLK_z; ++i) {
		str[0] = i;
		Key2Str[i] = str;
	}

	Key2Str[SDLK_DELETE] = "delete";

	for (i = SDLK_KP0; i <= SDLK_KP9; ++i) {
		sprintf_s(str, sizeof(str), "kp_%d", i - SDLK_KP0);
		Key2Str[i] = str;
	}

	Key2Str[SDLK_KP_PERIOD] = "kp_period";
	Key2Str[SDLK_KP_DIVIDE] = "kp_divide";
	Key2Str[SDLK_KP_MULTIPLY] = "kp_multiply";
	Key2Str[SDLK_KP_MINUS] = "kp_minus";
	Key2Str[SDLK_KP_PLUS] = "kp_plus";
	Key2Str[SDLK_KP_ENTER] = "kp_enter";
	Key2Str[SDLK_KP_EQUALS] = "kp_equals";
	Key2Str[SDLK_UP] = "up";
	Key2Str[SDLK_DOWN] = "down";
	Key2Str[SDLK_RIGHT] = "right";
	Key2Str[SDLK_LEFT] = "left";
	Key2Str[SDLK_INSERT] = "insert";
	Key2Str[SDLK_HOME] = "home";
	Key2Str[SDLK_END] = "end";
	Key2Str[SDLK_PAGEUP] = "pageup";
	Key2Str[SDLK_PAGEDOWN] = "pagedown";

	for (i = SDLK_F1; i <= SDLK_F15; ++i) {
		sprintf_s(str, sizeof(str), "f%d", i - SDLK_F1 + 1);
		Key2Str[i] = str;
		sprintf_s(str, sizeof(str), "F%d", i - SDLK_F1 + 1);
		Str2Key[str] = i;
	}

	Key2Str[SDLK_HELP] = "help";
	Key2Str[SDLK_PRINT] = "print";
	Key2Str[SDLK_SYSREQ] = "sysreq";
	Key2Str[SDLK_BREAK] = "break";
	Key2Str[SDLK_MENU] = "menu";
	Key2Str[SDLK_POWER] = "power";
	Key2Str[SDLK_EURO] = "euro";
	Key2Str[SDLK_UNDO] = "undo";
}

/**
**  Initialize the video part for SDL.
*/
void InitVideoSdl(void)
{
	Uint32 flags;

	if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
#ifdef HAVE_X
		// Fix tablet input in full-screen mode.
		//
		// By default, SDL 1.2 on X11 uses mouse relative mode
		// if the mouse cursor is hidden and input is grabbed.
		// In this mode, SDL attempts to report the direction
		// in which the mouse is moving.  SDL computes this
		// from the mouse-cursor coordinates it gets from X.
		// If the mouse cursor of X were to hit an edge of the
		// screen, SDL would not know whether the user keeps
		// moving the mouse in that direction.  To prevent
		// that, SDL warps the mouse cursor of X back to the
		// center of the screen whenever it gets too far.
		// SDL then compensates for this warping in its
		// relative-motion computations, and uses the relative
		// coordinates to maintain its own idea of the mouse
		// cursor location, separately from what X thinks.
		//
		// All of the above works great with mice but fails
		// horribly with tablet or touch-screen devices, where
		// the coordinates reported by X depend only on the
		// position of the stylus and not at all on where SDL
		// previously warped the pointer.  Because Bos Wars
		// doesn't actually care about relative mouse motion,
		// let's just disable that whole feature in SDL.  The
		// easiest way to do that is by setting an environment
		// variable.  Do this before SDL_Init in case that
		// caches the value.
		//
		// SDL 1.2 documentation warns that environment
		// variables are mostly for debugging and might not be
		// supported in future SDL releases.  In that event,
		// Bos Wars can instead use the same trick as Barrage
		// does: tell SDL to show the mouse cursor, but make
		// all of its pixels transparent, so it's actually
		// invisible.  However, SDL 1.3 is apparently going to
		// support tablet devices natively, so this might not
		// be needed then.
		static char mouseRelative0[] = "SDL_MOUSE_RELATIVE=0";
		SDL_putenv(mouseRelative0); // doesn't want const
#endif

		if (SDL_Init(
#ifdef DEBUG
				SDL_INIT_NOPARACHUTE |
#endif
				SDL_INIT_AUDIO | SDL_INIT_VIDEO |
				SDL_INIT_TIMER) < 0 ) {
			fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
			exit(1);
		}

		// Clean up on exit

		atexit(SDL_Quit);

		// If debug is enabled, Stratagus disable SDL Parachute.
		// So we need gracefully handle segfaults and aborts.
#if defined(DEBUG) && !defined(USE_WIN32)
		signal(SIGSEGV, CleanExit);
		signal(SIGABRT, CleanExit);
#endif
		// Set WindowManager Title
		SDL_WM_SetCaption("Bos Wars", "Bos Wars");
	}

	// Initialize the display

	flags = 0;
	// Sam said: better for windows.
	/* SDL_HWSURFACE|SDL_HWPALETTE | */
	if (Video.FullScreen) {
		flags |= SDL_FULLSCREEN;
	}
	if (UseOpenGL) {
		flags |= SDL_OPENGL;
	}

	if (!Video.Width || !Video.Height) {
		Video.Width = 640;
		Video.Height = 480;
	}

	TheScreen = SDL_SetVideoMode(Video.Width, Video.Height, Video.Depth, flags);
	if (TheScreen && (TheScreen->format->BitsPerPixel != 16 &&
			TheScreen->format->BitsPerPixel != 32)) {
		// Only support 16 and 32 bpp, default to 16
		TheScreen = SDL_SetVideoMode(Video.Width, Video.Height, 16, flags);
	}
	if (TheScreen == NULL) {
		fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n",
			Video.Width, Video.Height, Video.Depth, SDL_GetError());
		exit(1);
	}

	Video.FullScreen = (TheScreen->flags & SDL_FULLSCREEN) ? 1 : 0;
	Video.Depth = TheScreen->format->BitsPerPixel;

	// Turn cursor off, we use our own.
	SDL_ShowCursor(0);

	// Make default character translation easier
	SDL_EnableUNICODE(1);

	if (UseOpenGL) {
		InitOpenGL();
	}

	InitKey2Str();

	ColorBlack = Video.MapRGB(TheScreen->format, 0, 0, 0);
	ColorDarkGreen = Video.MapRGB(TheScreen->format, 48, 100, 4);
	ColorDarkBlue = Video.MapRGB(TheScreen->format, 0, 0, 96);
	ColorBlue = Video.MapRGB(TheScreen->format, 0, 0, 252);
	ColorCyan = Video.MapRGB(TheScreen->format, 0, 160, 208);
	ColorOrange = Video.MapRGB(TheScreen->format, 248, 140, 20);
	ColorWhite = Video.MapRGB(TheScreen->format, 252, 248, 240);
	ColorGray = Video.MapRGB(TheScreen->format, 128, 128, 128);
	ColorRed = Video.MapRGB(TheScreen->format, 252, 0, 0);
	ColorGreen = Video.MapRGB(TheScreen->format, 0, 252, 0);
	ColorYellow = Video.MapRGB(TheScreen->format, 252, 252, 0);

	UI.MouseWarpX = UI.MouseWarpY = -1;
}

/**
**  Check if a resolution is valid
**
**  @param w  Width
**  @param h  Height
*/
int VideoValidResolution(int w, int h)
{
	return SDL_VideoModeOK(w, h, TheScreen->format->BitsPerPixel, TheScreen->flags);
}

/**
**  Invalidate some area
**
**  @param x  screen pixel X position.
**  @param y  screen pixel Y position.
**  @param w  width of rectangle in pixels.
**  @param h  height of rectangle in pixels.
*/
void InvalidateArea(int x, int y, int w, int h)
{
	if (!UseOpenGL) {
		Assert(NumRects != sizeof(Rects) / sizeof(*Rects));
		Assert(x >= 0 && y >= 0 && x + w <= Video.Width && y + h <= Video.Height);
		Rects[NumRects].x = x;
		Rects[NumRects].y = y;
		Rects[NumRects].w = w;
		Rects[NumRects].h = h;
		++NumRects;
	}
}

/**
**  Invalidate whole window
*/
void Invalidate(void)
{
	if (!UseOpenGL) {
		Rects[0].x = 0;
		Rects[0].y = 0;
		Rects[0].w = Video.Width;
		Rects[0].h = Video.Height;
		NumRects = 1;
	}
}


#if defined(DEBUG) && defined(DUMP_SDL_EVENTS)

static void DumpSdlActiveEvent(const SDL_ActiveEvent &active, const char *type)
{
	printf("SDL_ActiveEvent {"
	       " type=%s, gain=%d, state=%d }\n",
	       type,
	       static_cast<int>(active.gain),
	       static_cast<int>(active.state));
}

static void DumpSdlKeyboardEvent(const SDL_KeyboardEvent &key, const char *type)
{
	printf("SDL_KeyboardEvent {"
	       " type=%s, which=%d, state=%d, keysym={"
	       " scancode=%d, sym=%d, mod=%d, unicode=%d }}\n",
	       type,
	       static_cast<int>(key.which),
	       static_cast<int>(key.state),
	       static_cast<int>(key.keysym.scancode),
	       static_cast<int>(key.keysym.sym),
	       static_cast<int>(key.keysym.mod),
	       static_cast<int>(key.keysym.unicode));
}

static void DumpSdlMouseMotionEvent(const SDL_MouseMotionEvent &motion, const char *type)
{
	printf("SDL_MouseMotionEvent {"
	       " type=%s, which=%d, state=%d,"
	       " x=%d, y=%d, xrel=%d, yrel=%d }\n",
	       type,
	       static_cast<int>(motion.which),
	       static_cast<int>(motion.state),
	       static_cast<int>(motion.x),
	       static_cast<int>(motion.y),
	       static_cast<int>(motion.xrel),
	       static_cast<int>(motion.yrel));
}

static void DumpSdlMouseButtonEvent(const SDL_MouseButtonEvent &button, const char *type)
{
	printf("SDL_MouseButtonEvent {"
	       " type=%s, which=%d, button=%d, state=%d, x=%d, y=%d }\n",
	       type,
	       static_cast<int>(button.which),
	       static_cast<int>(button.button),
	       static_cast<int>(button.state),
	       static_cast<int>(button.x),
	       static_cast<int>(button.y));
}

static void DumpSdlJoyAxisEvent(const SDL_JoyAxisEvent &jaxis, const char *type)
{
	printf("SDL_JoyAxisEvent {"
	       " type=%s, which=%d, axis=%d, value=%d }\n",
	       type,
	       static_cast<int>(jaxis.which),
	       static_cast<int>(jaxis.axis),
	       static_cast<int>(jaxis.value));
}

static void DumpSdlJoyBallEvent(const SDL_JoyBallEvent &jball, const char *type)
{
	printf("SDL_JoyBallEvent {"
	       " type=%s, which=%d, ball=%d, xrel=%d, yrel=%d }\n",
	       type,
	       static_cast<int>(jball.which),
	       static_cast<int>(jball.ball),
	       static_cast<int>(jball.xrel),
	       static_cast<int>(jball.yrel));
}

static void DumpSdlJoyHatEvent(const SDL_JoyHatEvent &jhat, const char *type)
{
	printf("SDL_JoyHatEvent {"
	       " type=%s, which=%d, button=%d, state=%d }\n",
	       type,
	       static_cast<int>(jhat.which),
	       static_cast<int>(jhat.hat),
	       static_cast<int>(jhat.value));
}

static void DumpSdlJoyButtonEvent(const SDL_JoyButtonEvent &jbutton, const char *type)
{
	printf("SDL_JoyButtonEvent {"
	       " type=%s, which=%d, button=%d, state=%d }\n",
	       type,
	       static_cast<int>(jbutton.which),
	       static_cast<int>(jbutton.button),
	       static_cast<int>(jbutton.state));
}

static void DumpSdlResizeEvent(const SDL_ResizeEvent &resize, const char *type)
{
	printf("SDL_ResizeEvent {"
	       " type=%s, w=%d, h=%d }\n",
	       type,
	       static_cast<int>(resize.w),
	       static_cast<int>(resize.h));
}

static void DumpSdlExposeEvent(const SDL_ExposeEvent &, const char *type)
{
	printf("SDL_ExposeEvent {"
	       " type=%s }\n",
	       type);
}

static void DumpSdlQuitEvent(const SDL_QuitEvent &, const char *type)
{
	printf("SDL_QuitEvent {"
	       " type=%s }\n",
	       type);
}

static void DumpSdlEvent(const SDL_Event *event)
{
	switch (event->type) {
	case SDL_ACTIVEEVENT:
		DumpSdlActiveEvent(event->active, "SDL_ACTIVEEVENT");
		break;

	case SDL_KEYDOWN:
		DumpSdlKeyboardEvent(event->key, "SDL_KEYDOWN");
		break;

	case SDL_KEYUP:
		DumpSdlKeyboardEvent(event->key, "SDL_KEYUP");
		break;

	case SDL_MOUSEMOTION:
		DumpSdlMouseMotionEvent(event->motion, "SDL_MOUSEMOTION");
		break;

	case SDL_MOUSEBUTTONDOWN:
		DumpSdlMouseButtonEvent(event->button, "SDL_MOUSEBUTTONDOWN");
		break;

	case SDL_MOUSEBUTTONUP:
		DumpSdlMouseButtonEvent(event->button, "SDL_MOUSEBUTTONUP");
		break;

	case SDL_JOYAXISMOTION:
		DumpSdlJoyAxisEvent(event->jaxis, "SDL_JOYAXISMOTION");
		break;

	case SDL_JOYBALLMOTION:
		DumpSdlJoyBallEvent(event->jball, "SDL_JOYBALLMOTION");
		break;

	case SDL_JOYHATMOTION:
		DumpSdlJoyHatEvent(event->jhat, "SDL_JOYHATMOTION");
		break;

	case SDL_JOYBUTTONDOWN:
		DumpSdlJoyButtonEvent(event->jbutton, "SDL_JOYBUTTONDOWN");
		break;

	case SDL_JOYBUTTONUP:
		DumpSdlJoyButtonEvent(event->jbutton, "SDL_JOYBUTTONUP");
		break;

	case SDL_VIDEORESIZE:
		DumpSdlResizeEvent(event->resize, "SDL_VIDEORESIZE");
		break;
		
	case SDL_VIDEOEXPOSE:
		DumpSdlExposeEvent(event->expose, "SDL_VIDEOEXPOSE");
		break;
		
	case SDL_QUIT:
		DumpSdlQuitEvent(event->quit, "SDL_QUIT");
		break;

	default:
		printf("SDL_Event { type=%d, ... }\n",
		       static_cast<int>(event->type));
		break;
		
	}
	fflush(stdout);
}

#endif // DUMP_SDL_EVENTS

/**
**  Handle interactive input event.
**
**  @param callbacks  Callback structure for events.
**  @param event      SDL event structure pointer.
*/
static void SdlDoEvent(const EventCallback *callbacks, const SDL_Event *event)
{
#ifdef DUMP_SDL_EVENTS
	DumpSdlEvent(event);
#endif

	switch (event->type) {
		case SDL_MOUSEBUTTONDOWN:
			InputMouseButtonPress(callbacks, SDL_GetTicks(),
				event->button.button);
			break;

		case SDL_MOUSEBUTTONUP:
			InputMouseButtonRelease(callbacks, SDL_GetTicks(),
				event->button.button);
			break;

			// FIXME: check if this is only useful for the cursor
			// FIXME: if this is the case we don't need this.
		case SDL_MOUSEMOTION:
			InputMouseMove(callbacks, SDL_GetTicks(),
				event->motion.x, event->motion.y);
			// FIXME: Same bug fix from X11
			if ((UI.MouseWarpX != -1 || UI.MouseWarpY != -1) &&
					(event->motion.x != UI.MouseWarpX ||
						event->motion.y != UI.MouseWarpY)) {
				int xw = UI.MouseWarpX;
				int yw = UI.MouseWarpY;
				UI.MouseWarpX = -1;
				UI.MouseWarpY = -1;
				SDL_WarpMouse(xw, yw);
			}
			break;

		case SDL_ACTIVEEVENT:
			if (event->active.state & SDL_APPMOUSEFOCUS) {
				static bool InMainWindow = true;

				if (InMainWindow && !event->active.gain) {
					InputMouseExit(callbacks, SDL_GetTicks());
				}
				InMainWindow = (event->active.gain != 0);
			}
			if (event->active.state & SDL_APPACTIVE) {
				static bool IsVisible = true;
				static bool DoTogglePause = false;

				if (IsVisible && !event->active.gain) {
					IsVisible = false;
					if (!GamePaused) {
						DoTogglePause = true;
						UiTogglePause();
					}
				} else if (!IsVisible && event->active.gain) {
					IsVisible = true;
					if (GamePaused && DoTogglePause) {
						DoTogglePause = false;
						UiTogglePause();
					}
					if (UseOpenGL) {
						Video.ResizeScreen(Video.Width, Video.Height);
					}
				}
			}
			break;

		case SDL_KEYDOWN:
			InputKeyButtonPress(callbacks, SDL_GetTicks(),
				event->key.keysym.sym, event->key.keysym.unicode);
			break;

		case SDL_KEYUP:
			InputKeyButtonRelease(callbacks, SDL_GetTicks(),
				event->key.keysym.sym, event->key.keysym.unicode);
			break;

		case SDL_QUIT:
			Exit(0);
			break;
	}

	if (callbacks == GetCallbacks()) {
		handleInput(event);
	}
}

/**
**  Set the current callbacks
*/
void SetCallbacks(const EventCallback *callbacks)
{
	Callbacks = callbacks;
}

/**
**  Get the current callbacks
*/
const EventCallback *GetCallbacks()
{
	return Callbacks;
}

/**
**  Wait for interactive input event for one frame.
**
**  Handles system events, joystick, keyboard, mouse.
**  Handles the network messages.
**  Handles the sound queue.
**
**  All events available are fetched. Sound and network only if available.
**  Returns if the time for one frame is over.
*/
void WaitEventsOneFrame()
{
	struct timeval tv;
	fd_set rfds;
	fd_set wfds;
	Socket maxfd;
	int i;
	int s;
	SDL_Event event[1];
	Uint32 ticks;
	int interrupts;

	++FrameCounter;

	ticks = SDL_GetTicks();
	if (ticks > NextFrameTicks) { // We are too slow :(
		++SlowFrameCounter;
	}

	InputMouseTimeout(GetCallbacks(), ticks);
	InputKeyTimeout(GetCallbacks(), ticks);
	CursorAnimate(ticks);

	interrupts = 0;

	for (;;) {
		//
		// Time of frame over? This makes the CPU happy. :(
		//
		ticks = SDL_GetTicks();
		if (!interrupts && ticks < NextFrameTicks) {
			SDL_Delay(NextFrameTicks - ticks);
			ticks = SDL_GetTicks();
		}
		while (ticks >= NextFrameTicks) {
			++interrupts;
			FrameFraction += FrameRemainder;
			if (FrameFraction > 10) {
				FrameFraction -= 10;
				++NextFrameTicks;
			}
			NextFrameTicks += FrameTicks;
		}

		//
		// Prepare select
		//
		maxfd = 0;
		tv.tv_sec = tv.tv_usec = 0;
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		//
		// Network
		//
		if (IsNetworkGame()) {
			if (NetworkFildes > maxfd) {
				maxfd = NetworkFildes;
			}
			FD_SET(NetworkFildes, &rfds);
		}

#if 0
		s = select(maxfd + 1, &rfds, &wfds, NULL,
			(i = SDL_PollEvent(event)) ? &tv : NULL);
#else
		// QUICK HACK to fix the event/timer problem
		// The timer code didn't interrupt the select call.
		// Perhaps I could send a signal to the process
		// Not very nice, but this is the problem if you use other libraries
		// The event handling of SDL is wrong designed = polling only.
		// There is hope on SDL 1.3 which will have this fixed.

		s = select(maxfd + 1, &rfds, &wfds, NULL, &tv);
		i = SDL_PollEvent(event);
#endif

		if (i) { // Handle SDL event
			SdlDoEvent(GetCallbacks(), event);
		}

		if (s > 0) {
			//
			// Network
			//
			if (IsNetworkGame() && FD_ISSET(NetworkFildes, &rfds) ) {
				GetCallbacks()->NetworkEvent();
			}
		}

		//
		// No more input and time for frame over: return
		//
		if (!i && s <= 0 && interrupts) {
			break;
		}
	}
	handleInput(NULL);

	if (!SkipGameCycle--) {
		SkipGameCycle = SkipFrames;
	}

	if (!UseOpenGL && (GameRunning || Editor.Running || PatchEditorRunning)) {
		Video.ClearScreen();
	}
}

/**
**  Realize video memory.
*/
void RealizeVideoMemory(void)
{
	if (UseOpenGL) {
		SDL_GL_SwapBuffers();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	} else {
		if (NumRects) {
			SDL_UpdateRects(TheScreen, NumRects, Rects);
			NumRects = 0;
		}
	}
	HideCursor();
}

/**
**  Lock the screen for write access.
*/
void SdlLockScreen(void)
{
	if (!UseOpenGL) {
		if (SDL_MUSTLOCK(TheScreen)) {
			SDL_LockSurface(TheScreen);
		}
	}
}

/**
**  Unlock the screen for write access.
*/
void SdlUnlockScreen(void)
{
	if (!UseOpenGL) {
		if (SDL_MUSTLOCK(TheScreen)) {
			SDL_UnlockSurface(TheScreen);
		}
	}
}

/**
**  Convert a SDLKey to a string
*/
const char *SdlKey2Str(int key)
{
	return Key2Str[key].c_str();
}

/**
**  Convert a string to SDLKey
*/
int Str2SdlKey(const char *str)
{
	InitKey2Str();

	std::map<int, std::string>::iterator i;
	for (i = Key2Str.begin(); i != Key2Str.end(); ++i) {
		if (!strcasecmp(str, (*i).second.c_str())) {
			return (*i).first;
		}
	}
	std::map<std::string, int>::iterator i2;
	for (i2 = Str2Key.begin(); i2 != Str2Key.end(); ++i2) {
		if (!strcasecmp(str, (*i2).first.c_str())) {
			return (*i2).second;
		}
	}
	return 0;
}

/**
**  Check if the mouse is grabbed
*/
bool SdlGetGrabMouse(void)
{
	return SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON;
}

/**
**  Toggle grab mouse.
**
**  @param mode  Wanted mode, 1 grab, -1 not grab, 0 toggle.
*/
void ToggleGrabMouse(int mode)
{
	bool grabbed = SdlGetGrabMouse();

	if (mode <= 0 && grabbed) {
		SDL_WM_GrabInput(SDL_GRAB_OFF);
	} else if (mode >= 0 && !grabbed) {
		SDL_WM_GrabInput(SDL_GRAB_ON);
	}
}

/**
**  Toggle full screen mode.
*/
void ToggleFullScreen(void)
{
#ifdef USE_WIN32
	long framesize;
	SDL_Rect clip;
	Uint32 flags;
	int w;
	int h;
	int bpp;
	unsigned char *pixels = NULL;
	SDL_Color *palette = NULL;
	int ncolors = 0;

	if (!TheScreen) { // don't bother if there's no surface.
		return;
	}

	flags = TheScreen->flags;
	w = TheScreen->w;
	h = TheScreen->h;
	bpp = TheScreen->format->BitsPerPixel;

	if (!SDL_VideoModeOK(w, h, bpp,	flags ^ SDL_FULLSCREEN)) {
		return;
	}

	SDL_GetClipRect(TheScreen, &clip);

	// save the contents of the screen.
	framesize = w * h * TheScreen->format->BytesPerPixel;

	if (!UseOpenGL) {
		if (!(pixels = new unsigned char[framesize])) { // out of memory
			return;
		}
		SDL_LockSurface(TheScreen);
		memcpy(pixels, TheScreen->pixels, framesize);

		if (TheScreen->format->palette) {
			ncolors = TheScreen->format->palette->ncolors;
			if (!(palette = new SDL_Color[ncolors])) {
				delete[] pixels;
				return;
			}
			memcpy(palette, TheScreen->format->palette->colors,
				ncolors * sizeof(SDL_Color));
		}
		SDL_UnlockSurface(TheScreen);
	}

	TheScreen = SDL_SetVideoMode(w, h, bpp, flags ^ SDL_FULLSCREEN);
	if (!TheScreen) {
		TheScreen = SDL_SetVideoMode(w, h, bpp, flags);
		if (!TheScreen) { // completely screwed.
			if (!UseOpenGL) {
				delete[] pixels;
				delete[] palette;
			}
			fprintf(stderr, "Toggle to fullscreen, crashed all\n");
			Exit(-1);
		}
	}

	// Windows shows the SDL cursor when starting in fullscreen mode
	// then switching to window mode.  This hides the cursor again.
	SDL_ShowCursor(SDL_ENABLE);
	SDL_ShowCursor(SDL_DISABLE);

	if (UseOpenGL) {
		ReloadOpenGL();
	} else {
		SDL_LockSurface(TheScreen);
		memcpy(TheScreen->pixels, pixels, framesize);
		delete[] pixels;

		if (TheScreen->format->palette) {
			// !!! FIXME : No idea if that flags param is right.
			SDL_SetPalette(TheScreen, SDL_LOGPAL, palette, 0, ncolors);
			delete[] palette;
		}
		SDL_UnlockSurface(TheScreen);
	}

	SDL_SetClipRect(TheScreen, &clip);

	Invalidate(); // Update display
#else // !USE_WIN32
	SDL_WM_ToggleFullScreen(TheScreen);
#endif

	Video.FullScreen = (TheScreen->flags & SDL_FULLSCREEN) ? 1 : 0;
}

//@}
