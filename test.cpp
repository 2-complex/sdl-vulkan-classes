#include "vulkan.h"
#include "sdl.h"

#include <stdexcept>
#include <vector>
#include <string>
#include <stdio.h>

using namespace vulkan;
using namespace sdl;


class CreateInstanceParameters : public ICreateInstanceParameters
{
    std::vector<LayerInfo> requested_layers;
    std::vector<ExtensionInfo> requested_extensions;

public:
    CreateInstanceParameters(
        const std::vector<LayerInfo>& requested_layers,
        const std::vector<ExtensionInfo>& requested_extensions)
        : requested_layers(requested_layers)
        , requested_extensions(requested_extensions)
    {
    }

    virtual std::vector<LayerInfo> get_requested_layers() const;
    virtual std::vector<ExtensionInfo> get_requested_extensions() const;
    virtual std::string get_application_name() const;
    virtual int get_application_version() const;
    virtual std::string get_engine_name() const;
    virtual int get_engine_version() const;
};

std::vector<LayerInfo> CreateInstanceParameters::get_requested_layers() const
{
    return requested_layers;
}

std::vector<ExtensionInfo> CreateInstanceParameters::get_requested_extensions() const
{
    return requested_extensions;
}

std::string CreateInstanceParameters::get_application_name() const
{
    return "My Application";
}

int CreateInstanceParameters::get_application_version() const
{
    return 0;
}

std::string CreateInstanceParameters::get_engine_name() const
{
    return "Cello";
}

int CreateInstanceParameters::get_engine_version() const
{
    return 0;
}


class CreateLogicalDeviceParameters : public IRequestLayerAndExtensions
{
public:
    std::vector<LayerInfo> requested_layers;
    std::vector<ExtensionInfo> requested_extensions;

public:
    CreateLogicalDeviceParameters(
        const std::vector<LayerInfo>& requested_layers,
        const std::vector<ExtensionInfo>& requested_extensions)
        : requested_layers(requested_layers)
        , requested_extensions(requested_extensions)
    {
    }

    std::vector<LayerInfo> get_requested_layers() const;
    std::vector<ExtensionInfo> get_requested_extensions() const;
};

std::vector<LayerInfo> CreateLogicalDeviceParameters::get_requested_layers() const
{
    return requested_layers;
}

std::vector<ExtensionInfo> CreateLogicalDeviceParameters::get_requested_extensions() const
{
    return requested_extensions;
}


template<typename Info>
std::vector<Info> filter_by_name(
    const std::vector<Info>& infos,
    const std::vector<std::string>& names_vec)
{
    std::set<std::string> names;
    for( const std::string& name : names_vec )
    {
        names.insert(name);
    }

    std::vector<Info> result_infos;
    for( const Info& info : infos )
    {
        if( names.find(info.name) != names.end() )
        {
            result_infos.push_back(info);
        }
    }

    return result_infos;
}


int main(int argc, char** args)
{
    try
    {
        SDL_Window* window = get_vulkan_sdk_window();
        std::vector<LayerInfo> layer_infos = get_layer_infos();

        printf("Engine Layers:\n");
        for( LayerInfo& info : layer_infos )
        {
            printf(" - %s\n", info.name.c_str());
            printf("      %s\n" ,info.description.c_str());
            printf("      spec vers : %d\n" ,info.specVersion);
            printf("      impl vers : %d\n" ,info.implementationVersion);
        }

        printf( "Engine Extensions:\n" );
        std::vector<ExtensionInfo> extension_infos = get_extension_infos();
        for( ExtensionInfo& info : extension_infos )
        {
            printf(" - %s\n", info.name.c_str());
            printf("      spec vers : %d\n" ,info.specVersion);
        }

        std::vector<std::string> extension_names = get_extension_names(window);
        printf( "Window Extension names:\n" );
        for (std::string& name : extension_names)
        {
            printf(" - %s\n", name.c_str());
        }

        std::vector<ExtensionInfo> window_extension_infos =
            filter_by_name<ExtensionInfo>(extension_infos, extension_names);

        Instance instance(CreateInstanceParameters(layer_infos, window_extension_infos));

        PhysicalDevice physical_device = instance.select_gpu();

        std::vector<ExtensionInfo> device_extension_infos =
            filter_by_name<ExtensionInfo>(extension_infos, {VK_KHR_SWAPCHAIN_EXTENSION_NAME});

        physical_device.create_logical_device(
            CreateLogicalDeviceParameters(layer_infos, device_extension_infos));

        Surface surface = instance.create_surface(window);

        printf( "Is surface supported: %d\n", physical_device.is_surface_supported(surface) );
    }
    catch(VulkanException& e)
    {
        printf("Vulkan exception with error code: %d (%s) message: %s\n", e.code(), e.enum_name().c_str(), e.what());
    }
    catch(std::runtime_error& e)
    {
        printf("Runtime error: %s\n", e.what());
    }
    catch(...)
    {
        printf( "Had to have been thrown\n" );
    }

    SDL_Quit();
    return 0;
}

