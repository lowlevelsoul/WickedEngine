#pragma once
// This file includes platform, os specific libraries and supplies common platform specific resources

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <SDKDDKVer.h>
#include <windows.h>

#if WINAPI_FAMILY == WINAPI_FAMILY_GAMES
#define PLATFORM_XBOX
#else
#define PLATFORM_WINDOWS_DESKTOP
#endif // WINAPI_FAMILY_GAMES
#define wiLoadLibrary(name) LoadLibraryA(name)
#define wiGetProcAddress(handle,name) GetProcAddress(handle, name)
#elif defined(__SCE__)
#define PLATFORM_PS5
#elif PLATFORM_APPLE
#	define PLATFORM_MACOS 1
#	include <dlfcn.h>
#	define wiLoadLibrary(name) dlopen(name, RTLD_LAZY)
#	define wiGetProcAddress(handle,name) dlsym(handle, name)
typedef void* HMODULE;
#else
#define PLATFORM_LINUX
#include <dlfcn.h>
#define wiLoadLibrary(name) dlopen(name, RTLD_LAZY)
#define wiGetProcAddress(handle,name) dlsym(handle, name)
typedef void* HMODULE;
#endif // _WIN32

#if SDL3
#	define SDL2 1
#	include <SDL3/SDL.h>
#	include <SDL3/SDL_vulkan.h>
#	include <SDL3/SDL.h>
#elif SDL2
#	include <SDL2/SDL.h>
#	include <SDL_vulkan.h>
#	include "sdl2.h"
#endif

namespace wi::platform
{
#ifdef _WIN32
	using window_type = HWND;
	using error_type = HRESULT;
#elif SDL2
	using window_type = SDL_Window*;
	using error_type = int;
#else
	using window_type = void*;
	using error_type = int;
#endif // _WIN32

	inline void Exit()
	{
#ifdef _WIN32
		PostQuitMessage(0);
#endif // _WIN32
#if SDL3
		SDL_Event quit_event;
		quit_event.type = SDL_EVENT_QUIT;
		SDL_PushEvent(&quit_event);
#elif SDL2
		SDL_Event quit_event;
		quit_event.type = SDL_QUIT;
		SDL_PushEvent(&quit_event);
#endif
	}

	struct WindowProperties
	{
		int width = 0;
		int height = 0;
		float dpi = 96;
	};
	inline void GetWindowProperties(window_type window, WindowProperties* dest)
	{
#ifdef PLATFORM_WINDOWS_DESKTOP
		dest->dpi = (float)GetDpiForWindow(window);
#endif // WINDOWS_DESKTOP

#ifdef PLATFORM_XBOX
		dest->dpi = 96.f;
#endif // PLATFORM_XBOX

#if defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_XBOX)
		RECT rect;
		GetClientRect(window, &rect);
		dest->width = int(rect.right - rect.left);
		dest->height = int(rect.bottom - rect.top);
#endif // PLATFORM_WINDOWS_DESKTOP || PLATFORM_XBOX
		
#if defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
		int window_width, window_height;
		SDL_GetWindowSize(window, &window_width, &window_height);
#if SDL3
		SDL_GetWindowSizeInPixels( window, &dest->width, &dest->height );
#else
		SDL_Vulkan_GetDrawableSize(window, &dest->width, &dest->height);
#endif
		dest->dpi = ((float)dest->width / (float)window_width) * 96.f;
#endif // PLATFORM_LINUX || PLATFORM_APPLE
	}
}
