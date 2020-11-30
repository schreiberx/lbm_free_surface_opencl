/*
 * Copyright 2010 Martin Schreiber
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * info: OpenGL 3.2 initialization code taken from khronos website
 */

#ifndef CRENDERWINDOW_HPP_
#define CRENDERWINDOW_HPP_

#include "libgl/incgl3.h"

extern "C" {
	#include <SDL.h>
	#include <SDL_thread.h>
}


/*
 * CMath has to be included before SDL!!!
 */
#include "libmath/CMath.hpp"


#include <iostream>
#include <sstream>
#include <signal.h>
#include "lib/CError.hpp"
#include "lib/CBitmap.hpp"
#include "lib/CError.hpp"


/**
 * \brief callback handlers for gui events
 */
class RenderWindowEventCallbacks
{
public:
	virtual void callback_key_down(int key, int mod, int scancode, int unicode) = 0;	///< key down event
	virtual void callback_key_up(int key, int mod, int scancode, int unicode) = 0;		///< key up event
	virtual void callback_quit() = 0;							///< quit event
	virtual void callback_mouse_motion(int x, int y) = 0;		///< mouse position in absolute coordinates
	virtual void callback_mouse_button_down(int button) = 0;	///< button press event
	virtual void callback_mouse_button_up(int button) = 0;		///< button release event
	virtual void callback_viewport_changed(int width, int height) = 0;	///< viewport changed (window resized)
	virtual void callback_mouse_wheel(int x, int y) = 0;		///< viewport changed (window resized)

	virtual ~RenderWindowEventCallbacks()
	{
	}
};



/**
 * \brief initialize gui and rendering context
 */
class CRenderWindow
{
	bool sdl_initialized;

	RenderWindowEventCallbacks *cEventCallbackImplementation;

	SDL_Thread *save_bitmap_thread;
	std::string save_bitmap_filename;	///< bitmap filename when storing bitmaps using threads
	CBitmap24 save_bitmap;


	SDL_Window *window;
	SDL_GLContext glContext;


public:
	CError error;		///< error handler
	CMessage message;	///< message handler

	bool fullscreen_active;	///< true, if fullscreen is active

	int window_width;				///< width of viewport/window
	int window_height;				///< height of viewport/window
	float aspect_ratio;			///< current aspect ratio (window/height)

	enum
	{
		MOUSE_BUTTON_LEFT = SDL_BUTTON_LEFT,
		MOUSE_BUTTON_RIGHT = SDL_BUTTON_RIGHT,
		MOUSE_BUTTON_MIDDLE = SDL_BUTTON_MIDDLE//,
//		MOUSE_BUTTON_WHEEL_UP = SDL_BUTTON_WHEELUP,
//		MOUSE_BUTTON_WHEEL_DOWN = SDL_BUTTON_WHEELDOWN
	};

	enum
	{
		KEY_PAGEUP = SDLK_PAGEUP,
		KEY_PAGEDOWN = SDLK_PAGEDOWN,
		KEY_F1 = SDLK_F1,
		KEY_F2 = SDLK_F2,
		KEY_F3 = SDLK_F3,
		KEY_F4 = SDLK_F4,
		KEY_F5 = SDLK_F5,
		KEY_F6 = SDLK_F6,
		KEY_F7 = SDLK_F7,
		KEY_F8 = SDLK_F8,
		KEY_F9 = SDLK_F9,
		KEY_F10 = SDLK_F10,
		KEY_F11 = SDLK_F11,
		KEY_F12 = SDLK_F12,

#if WIN32
		KEY_MOD_SHIFT = KMOD_LSHIFT
#else
		KEY_MOD_SHIFT = (KMOD_LSHIFT | KMOD_RSHIFT)
#endif
	};

	/**
	 * update viewport with current width and height values and update apsect ratio
	 */
	void updateViewport()
	{
		glViewport(0, 0, window_width, window_height);
		aspect_ratio = (float)window_width / (float)window_height;
	}

	/**
	 * set fullscreen mode or deactivate
	 */
	void setWindowFullscreenState(
			bool p_fullscreen		///< set to true, if fullscreen should be activated - false otherwise
	)
	{
		if (p_fullscreen)
		{
			if (SDL_SetWindowFullscreen(window, SDL_TRUE) == -1)
			{
				std::cerr << "Failed to initialize fullscreen mode: " << SDL_GetError() << std::endl;
				return;
			}
		}
		else
		{
			if (SDL_SetWindowFullscreen(window, SDL_FALSE) == -1)
			{
				std::cerr << "Failed to initialize window mode: "; std::cerr << SDL_GetError() << std::endl;
				return;
			}
		}

		fullscreen_active = p_fullscreen;

/*
 * viewport and other variables are updated by SDL EVENT execution!
 */

//		SDL_ShowWindow(window);
//		SDL_GetWindowSize(window, &window_width, &window_height);

//		updateViewport();
//		cEventCallbackImplementation->callback_viewport_changed(window_width, window_height);
	}


	void checkSDLError(int line = -1)
	{
	#ifndef NDEBUG
		const char *error = SDL_GetError();
		if (*error != '\0')
		{
			printf("SDL Error: %s\n", error);
			if (line != -1)
				printf(" + line: %i\n", line);
			SDL_ClearError();
		}
	#endif
	}


	/**
	 * setup an opengl render context
	 */
	bool setupOpenGLRenderContext(
			const char* p_initial_window_title,
			int p_initial_window_width = 800,
			int p_initial_window_height = 600,
			int p_request_opengl_major_version = 3,		///< major version of opengl context to request
			int p_request_opengl_minor_version = 3		///< minor version of opengl context to request
	)
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE) < 0) /* Initialize SDL's Video subsystem */
		{
			std::cerr << "Unable to initialize SDL: "; std::cerr << SDL_GetError() << std::endl;
			return false;
		}

#if !WIN32
		// activate ctrl-c abort handling
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
#endif

		checkSDLError(__LINE__);

		/*
		 * set the opengl version number to create the context for (p_request_opengl_major_version, p_request_opengl_minor_version)
		 */
		if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, p_request_opengl_major_version) == -1)
		{
			error << "unable to set major OpenGL version to " << p_request_opengl_major_version << std::endl;
			return false;
		}

		if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, p_request_opengl_minor_version) == -1)
		{
			error << "unable to set minor OpenGL version to " << p_request_opengl_minor_version << std::endl;
			return false;
		}

		/*
		 * Turn on double buffering with a 24bit Z buffer.
		 * You may need to change this to 16 or 32 for your system.
		 */
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

		// disable vsync
//		SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

		checkSDLError(__LINE__);

		message << "Get current video driver: " << SDL_GetCurrentVideoDriver() << CMessage::endl;

		window = SDL_CreateWindow(
						p_initial_window_title,
						SDL_WINDOWPOS_CENTERED,
						SDL_WINDOWPOS_CENTERED,
						p_initial_window_width,
						p_initial_window_height,
//						(fullscreen_active ?
//								(SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS) :
								(SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)
//						)
			);

		checkSDLError(__LINE__);

		if (!window) /* Die if creation failed */
		{
			error << "Unable to create window: " << SDL_GetError() << std::endl;
			return false;
		}


		/*
		 * Create our OpenGL context and attach it to our window
		 */
		glContext = SDL_GL_CreateContext(window);
		if (!glContext)
		{
			error << "Unable to create GL context: " << SDL_GetError() << std::endl;
			SDL_DestroyWindow(window);
			return false;
		}

		checkSDLError(__LINE__);

		/*
		 * Set update intervals to 0 for immediate updates without caring about vsyncs
		 */
		SDL_GL_SetSwapInterval(0);

		/*
		 * execute event loop to get events important for setup
		 */
//		eventLoop();

		if (fullscreen_active)
		{
			setWindowFullscreenState(true);
		}

		checkSDLError(__LINE__);

#ifndef NDEBUG
		std::cout << "GL Version: " << glGetString(GL_VERSION) << std::endl;
#endif

		return true;
	}



public:
	/**
	 * initialize GUI
	 */
	CRenderWindow(
			class RenderWindowEventCallbacks &p_cEventCallbackImplementation,	///< class which implements the gui interface for callback events
			const char *p_initial_window_title,					///< title of window to display in the window title bar
			int p_window_width = 800,					///< initial width of window in window mode
			int p_window_height = 600,					///< initial height of window in window mode
			bool p_fullscreen = false,					///< set to true to enable fullscreen mode on window creation
			int p_request_opengl_major_version = 3,		///< major version of opengl context to request
			int p_request_opengl_minor_version = 2		///< minor version of opengl context to request
	)
	{
		cEventCallbackImplementation = &p_cEventCallbackImplementation;
		save_bitmap_thread = NULL;
		fullscreen_active = p_fullscreen;

		window_width = p_window_width;
		window_height = p_window_height;

		sdl_initialized = setupOpenGLRenderContext(
				p_initial_window_title,
				p_window_width,
				p_window_height,
				p_request_opengl_major_version,
				p_request_opengl_minor_version
			);
	}


	virtual ~CRenderWindow()
	{
		if (sdl_initialized)
		{
			// delete context
			SDL_GL_DeleteContext(glContext);

			// destroy window
			SDL_DestroyWindow(window);

			// quit SDL
			SDL_Quit();
		}
	}

	/**
	 * set the window title
	 */
	void setWindowTitle(
			const char *p_title	// string of title
	)
	{
		SDL_SetWindowTitle(window, p_title);
	}

	/**
	 * this function has to be called by the rendering loop on every iteration to catch the events and
	 * forward them to the class implementing the callback functions CGuiInterface
	 */
	void eventLoop() 
	{
		SDL_Event event;
		// process pending events
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_WINDOWEVENT:

					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_CLOSE:
							cEventCallbackImplementation->callback_quit();
							break;

						case SDL_WINDOWEVENT_RESIZED:
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							window_width = event.window.data1;
							window_height = event.window.data2;

							if (window_width <= 0)	window_width = 1;
							if (window_height <= 0)	window_height = 1;

							updateViewport();
							cEventCallbackImplementation->callback_viewport_changed(window_width, window_height);
							break;


						case SDL_WINDOWEVENT_SHOWN:
							SDL_GetWindowSize(window, &window_width, &window_height);
							updateViewport();
							cEventCallbackImplementation->callback_viewport_changed(window_width, window_height);
							break;
					}
					break;

				case SDL_KEYDOWN:
					cEventCallbackImplementation->callback_key_down(event.key.keysym.sym, event.key.keysym.mod, event.key.keysym.scancode, 0 /*event.key.keysym.unicode*/);
					break;

				case SDL_KEYUP:
					cEventCallbackImplementation->callback_key_up(event.key.keysym.sym, event.key.keysym.mod, event.key.keysym.scancode, 0 /*event.key.keysym.unicode*/);
					break;

				case SDL_QUIT:
					cEventCallbackImplementation->callback_quit();
					break;

				case SDL_MOUSEMOTION:
					cEventCallbackImplementation->callback_mouse_motion(event.motion.x, event.motion.y);
					break;

				case SDL_MOUSEBUTTONDOWN:
					cEventCallbackImplementation->callback_mouse_button_down(event.button.button);
					break;

				case SDL_MOUSEBUTTONUP:
					cEventCallbackImplementation->callback_mouse_button_up(event.button.button);
					break;

				case SDL_MOUSEWHEEL:
					cEventCallbackImplementation->callback_mouse_wheel(event.wheel.x, event.wheel.y);
					break;
			}
		}
	}

	/**
	 * return the ticks as an integer number (DEPRECATED)
	 */
	signed int getTicksInt()
	{
		return SDL_GetTicks();
	}

	/**
	 * return the ticks in seconds
	 */
	double getTicks()
	{
		return (double)SDL_GetTicks()*(1.0/1000.0);
	}

	/**
	 * save a screenshot to a bitmap file
	 */
	bool saveScreenshot(	const std::string &filename	///< filepath to store the screenshot to
			)
	{
		CBitmap24 bitmap(window_width, window_height);

		glReadPixels(0, 0, window_width, window_height, GL_BGR, GL_UNSIGNED_BYTE, bitmap.data);

		if (!bitmap.save(filename))
		{
			error << bitmap.error.getString();
			return false;
		}

		return true;
	}


private:
	/**
	 * thread which saves the screenshot data
	 */
	static int saveScreenshotThread(void *user_data)
	{
		CRenderWindow &g = *(CRenderWindow*)user_data;

		if (!g.save_bitmap.save(g.save_bitmap_filename))
		{
			std::cout << "ERROR: " << g.save_bitmap.error.getString() << std::endl;
//			error << g.bitmap.error.getString();
//			TODO: error handling
			return false;
		}
		return true;
	}

public:
	/**
	 * save a screenshot to a bitmap file starting a thread after loading the screenshot data from framebuffer
	 */
	bool saveScreenshotWithThread(
			const std::string &filename	///< filepath to store the screenshot to
	)
	{
		if (save_bitmap_thread != NULL)
		{
			int status;
			SDL_WaitThread(save_bitmap_thread, &status);
		}

		save_bitmap.resize(window_width, window_height);
		glReadPixels(0, 0, window_width, window_height, GL_BGR, GL_UNSIGNED_BYTE, save_bitmap.data);

		save_bitmap_filename = filename;
		save_bitmap_thread = SDL_CreateThread(&saveScreenshotThread, "saveBitmapThread", this);

		if (save_bitmap_thread == NULL)
		{
			std::cout << "failed to create thread to store bitmap" << std::endl;
			return false;
		}

		return true;
	}

	/**
	 * this function has to be called on the end of the rendering loop to swap the front with back buffer
	 */
	void swapBuffer()
	{
		SDL_GL_SwapWindow(window);
	}
};

#endif
