#include "vulkan.h"

#include <algorithm>
#include "stdlib.h"

namespace vulkan
{
VulkanException::VulkanException(VkResult result, std::string message)
    : result(result)
    , std::runtime_error(std::move(message))
{
}

VkResult VulkanException::code() const
{
    return result;
}

std::string VulkanException::enum_name() const
{
    std::map<VkResult, const char*>::iterator itr = result_to_name.find(result);
    if( itr == result_to_name.end() )
    {
        return "???";
    }
    else
    {
        return std::string(itr->second);
    }
}

PhysicalDevice::PhysicalDevice(
    VkPhysicalDevice physical_device,
    uint32_t queue_family_index
)
: physical_device(physical_device)
, queue_family_index(queue_family_index)
{
}

bool PhysicalDevice::is_surface_supported(const Surface& surface)
{
    // Make sure the surface is compatible with the queue family and gpu
    VkBool32 supported = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_index, surface.surface, &supported);
    return static_cast<bool>(supported);
}

std::vector<LayerInfo> get_layer_infos()
{
    uint32_t instance_layer_count = 0;
    VkResult result;

    result = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
    if( result != VK_SUCCESS )
    {
        throw VulkanException(result, "Error getting number of layers");
    }

    if( instance_layer_count == 0 )
    {
        throw std::runtime_error("Layer count non-positive");
    }

    VkLayerProperties* layer_properties = new VkLayerProperties[instance_layer_count];
    result = vkEnumerateInstanceLayerProperties(&instance_layer_count, layer_properties);

    std::vector<LayerInfo> layer_infos;

    for( size_t i = 0; i < instance_layer_count; ++i )
    {
        LayerInfo info;
        info.name = layer_properties[i].layerName;
        info.description = layer_properties[i].description;
        info.specVersion = layer_properties[i].specVersion;
        info.implementationVersion = layer_properties[i].implementationVersion;
        layer_infos.push_back(info);
    }

    delete[] layer_properties;

    return layer_infos;
}

std::vector<ExtensionInfo> get_extension_infos()
{
    VkResult result;

    uint32_t num_properties = 0;
    result = vkEnumerateInstanceExtensionProperties(nullptr, &num_properties, nullptr);
    if( result != VK_SUCCESS )
    {
        throw VulkanException(result, "Error getting number of extensions");
    }

    if( num_properties == 0 )
    {
        throw std::runtime_error("Extension count zero");
    }

    VkExtensionProperties* properties = new VkExtensionProperties[num_properties];
    result = vkEnumerateInstanceExtensionProperties(nullptr, &num_properties, properties);

    std::vector<ExtensionInfo> infos;

    for( size_t i = 0; i < num_properties; ++i )
    {
        ExtensionInfo info;
        info.name = properties[i].extensionName;
        info.specVersion = properties[i].specVersion;
        infos.push_back(info);
    }

    delete[] properties;

    return infos;
}

std::vector<std::string> get_extension_names(SDL_Window* window)
{
    SDL_bool success;
    uint32_t num_extensions;

    success = SDL_Vulkan_GetInstanceExtensions(window, &num_extensions, nullptr);
    if( !success )
    {
        throw std::runtime_error("SDL_Vulkan failed to get number of instance extensions\n");
    }

    const char** extension_name_c_strings = new const char*[num_extensions];
    success = SDL_Vulkan_GetInstanceExtensions(window, &num_extensions, extension_name_c_strings);
    if( !success )
    {
        throw std::runtime_error("SDL_Vulkan failed to get names of instance extensions\n");
    }

    std::vector<std::string> extension_names;
    for( size_t i = 0; i < num_extensions; ++i )
    {
        const char* name = extension_name_c_strings[i];
        extension_names.emplace_back(name);
    }

    delete[] extension_name_c_strings;
    return extension_names;
}

template<typename Info>
struct NamesArray
{
    uint32_t count;
    char** c_strings;

    explicit NamesArray(const std::vector<Info>& infos)
    {
        c_strings = static_cast<char**>(new char*[infos.size()]);
        count = 0;
        for( const Info& info : infos )
        {
            size_t size = info.name.size() + 1; // For the 0 at the end
            c_strings[count] = new char[size];
            memcpy(c_strings[count], info.name.c_str(), size);
            count++;
        }
    }

    ~NamesArray()
    {
        while( count )
        {
            count--;
            delete[] c_strings[count];
        }
        delete[] c_strings;
    }
};

Instance::Instance(const ICreateInstanceParameters& parameters)
{
    NamesArray<LayerInfo> requested_layer_names(parameters.get_requested_layers());
    NamesArray<ExtensionInfo> requested_extension_names(parameters.get_requested_extensions());

    std::string application_name(parameters.get_application_name());
    int application_version(parameters.get_application_version());

    std::string engine_name(parameters.get_engine_name());
    int engine_version(parameters.get_engine_version());

    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;

    app_info.pApplicationName = application_name.c_str();
    app_info.applicationVersion = application_version;
    app_info.pEngineName = engine_name.c_str();
    app_info.engineVersion = engine_version;

    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.pApplicationInfo = &app_info;

    create_info.enabledExtensionCount = requested_extension_names.count;
    create_info.ppEnabledExtensionNames = requested_extension_names.c_strings;

    create_info.enabledLayerCount = requested_layer_names.count;
    create_info.ppEnabledLayerNames = requested_layer_names.c_strings;

    VkResult result = vkCreateInstance(&create_info, nullptr, &instance);
    if( result != VK_SUCCESS )
    {
        throw VulkanException(result, "Error while creating vulkan instance");
    }
}

Instance::~Instance()
{
}

PhysicalDevice Instance::select_gpu()
{
    uint32_t num_physical_devices = -1;

    VkResult result;

    result = vkEnumeratePhysicalDevices(
        instance, &num_physical_devices, nullptr);

    if( result != VK_SUCCESS )
    {
        throw VulkanException(result, "Error getting number of physical devices");
    }

    VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[num_physical_devices];

    result = vkEnumeratePhysicalDevices(
        instance,
        &num_physical_devices,
        physicalDevices);

    if( result != VK_SUCCESS )
    {
        throw VulkanException(result, "Error getting list of physical devices");
    }

    // TODO: use a heuristic to pick the _best_ device instead of picking device 0
    int index = 0;
    VkPhysicalDevice selected = physicalDevices[index];
    delete[] physicalDevices;
    return PhysicalDevice(selected, index);
}

LogicalDevice::LogicalDevice(VkDevice device, uint32_t queue_family_index)
    : device(device)
    , queue_family_index(queue_family_index)
{
}

LogicalDevice PhysicalDevice::create_logical_device(
    const IRequestLayerAndExtensions& parameters)
{
    VkResult result;

    uint32_t num_device_extension_properties;
    result = vkEnumerateDeviceExtensionProperties(
        physical_device, nullptr, &num_device_extension_properties, nullptr);

    if( result != VK_SUCCESS )
    {
        throw VulkanException(result, "Error while getting size of properties list for device");
    }

    if( num_device_extension_properties == 0 )
    {
        throw std::runtime_error("No properties array found for physical device");
    }

    VkExtensionProperties* extension_properties = new VkExtensionProperties[num_device_extension_properties];

    result = vkEnumerateDeviceExtensionProperties(
        physical_device, nullptr, &num_device_extension_properties, extension_properties);

    if( result != VK_SUCCESS )
    {
        throw VulkanException(result, "Error while getting list of device extension properties");
    }

    std::vector<ExtensionInfo> requested_extensions = parameters.get_requested_extensions();
    std::set<std::string> requested_extension_name_set;

    for( ExtensionInfo& info : requested_extensions )
    {
        requested_extension_name_set.insert(info.name);
    }

    std::set<std::string> found_extension_names;
    for( size_t i = 0; i < num_device_extension_properties; ++i )
    {
        std::string extension_name(extension_properties[i].extensionName);
        if( requested_extension_name_set.find(extension_name) != requested_extension_name_set.end() )
        {
            found_extension_names.insert(extension_name);
        }
    }

    if( found_extension_names.size() != requested_extension_name_set.size() )
    {
        std::vector<std::string> not_found_names(
            requested_extension_name_set.size() - found_extension_names.size() );

        std::set_difference(
            requested_extension_name_set.begin(), requested_extension_name_set.end(),
            found_extension_names.begin(), found_extension_names.end(),
            not_found_names.begin());

        std::string not_found_names_str = "";
        for( std::string& name : not_found_names )
        {
            not_found_names_str += " " + name;
        }

        throw std::runtime_error("Not all extensions found:" + not_found_names_str);
    }

    NamesArray<LayerInfo> requested_layer_names(parameters.get_requested_layers());
    NamesArray<ExtensionInfo> requested_extension_names(requested_extensions);

    VkDeviceQueueCreateInfo queue_create_info;
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family_index;
    queue_create_info.queueCount = 1;
    float priority_array[] = {1.0f};
    queue_create_info.pQueuePriorities = priority_array;
    queue_create_info.pNext = nullptr;
    queue_create_info.flags = 0;

    VkDeviceCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_create_info;

    create_info.enabledLayerCount = requested_layer_names.count;
    create_info.ppEnabledLayerNames = requested_layer_names.c_strings;

    create_info.enabledExtensionCount = requested_extension_names.count;
    create_info.ppEnabledExtensionNames = requested_extension_names.c_strings;

    create_info.pNext = nullptr;
    create_info.pEnabledFeatures = nullptr;
    create_info.flags = 0;

    VkDevice device;

    result = vkCreateDevice(physical_device, &create_info, nullptr, &device);
    if( result != VK_SUCCESS )
    {
        throw VulkanException(result, "Error while creating logical devive");
    }

    delete[] extension_properties;
    return LogicalDevice(device, queue_family_index);
}

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
    if( size <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    char* buf = new char[size]; 
    snprintf( buf, size, format.c_str(), args ... );
    std::string s( buf, buf + size - 1 );
    delete[] buf;
    return s;
}

Surface Instance::create_surface(SDL_Window* window)
{
    VkSurfaceKHR surface;
    if( ! SDL_Vulkan_CreateSurface(window, instance, &surface) )
    {
        throw std::runtime_error(string_format("SDL could not create surface with window: %p", window));
    }

    return Surface(surface);
}

Surface::Surface(VkSurfaceKHR surface)
    : surface(surface)
{
}

}
