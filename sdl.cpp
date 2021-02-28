#include "sdl.h"

namespace sdl
{

SDL_Window* get_vulkan_sdk_window()
{
    int sdl_init_result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if( sdl_init_result != 0 )
    {
        throw SDLException(sdl_init_result, "SDL failed to initailize");
    }

    SDL_Window* window = SDL_CreateWindow(
        "My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);

    if ( !window )
    {
        printf( "SDL Window failed to allocate\n" );
        return NULL;
    }

    return window;
}

}