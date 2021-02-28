#include <SDL2/SDL.h>

#include <stdexcept>
#include <string>

namespace sdl
{

class SDLException : public std::runtime_error
{
public:
    SDLException(int result, std::string message)
        : std::runtime_error(std::move(message))
    {
    }
};

SDL_Window* get_vulkan_sdk_window();

}