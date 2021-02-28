#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <set>
#include <map>

namespace vulkan
{

static std::map<VkResult, const char*> result_to_name = 
{
    { VK_SUCCESS , "VK_SUCCESS" },
    { VK_NOT_READY , "VK_NOT_READY" },
    { VK_TIMEOUT , "VK_TIMEOUT" },
    { VK_EVENT_SET , "VK_EVENT_SET" },
    { VK_EVENT_RESET , "VK_EVENT_RESET" },
    { VK_INCOMPLETE , "VK_INCOMPLETE" },
    { VK_ERROR_OUT_OF_HOST_MEMORY , "VK_ERROR_OUT_OF_HOST_MEMORY" },
    { VK_ERROR_OUT_OF_DEVICE_MEMORY , "VK_ERROR_OUT_OF_DEVICE_MEMORY" },
    { VK_ERROR_INITIALIZATION_FAILED , "VK_ERROR_INITIALIZATION_FAILED" },
    { VK_ERROR_DEVICE_LOST , "VK_ERROR_DEVICE_LOST" },
    { VK_ERROR_MEMORY_MAP_FAILED , "VK_ERROR_MEMORY_MAP_FAILED" },
    { VK_ERROR_LAYER_NOT_PRESENT , "VK_ERROR_LAYER_NOT_PRESENT" },
    { VK_ERROR_EXTENSION_NOT_PRESENT , "VK_ERROR_EXTENSION_NOT_PRESENT" },
    { VK_ERROR_FEATURE_NOT_PRESENT , "VK_ERROR_FEATURE_NOT_PRESENT" },
    { VK_ERROR_INCOMPATIBLE_DRIVER , "VK_ERROR_INCOMPATIBLE_DRIVER" },
    { VK_ERROR_TOO_MANY_OBJECTS , "VK_ERROR_TOO_MANY_OBJECTS" },
    { VK_ERROR_FORMAT_NOT_SUPPORTED , "VK_ERROR_FORMAT_NOT_SUPPORTED" },
    { VK_ERROR_FRAGMENTED_POOL , "VK_ERROR_FRAGMENTED_POOL" },
    { VK_ERROR_UNKNOWN , "VK_ERROR_UNKNOWN" },
    { VK_ERROR_OUT_OF_POOL_MEMORY , "VK_ERROR_OUT_OF_POOL_MEMORY" },
    { VK_ERROR_INVALID_EXTERNAL_HANDLE , "VK_ERROR_INVALID_EXTERNAL_HANDLE" },
    { VK_ERROR_FRAGMENTATION , "VK_ERROR_FRAGMENTATION" },
    { VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS , "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS" },
    { VK_ERROR_SURFACE_LOST_KHR , "VK_ERROR_SURFACE_LOST_KHR" },
    { VK_ERROR_NATIVE_WINDOW_IN_USE_KHR , "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" },
    { VK_SUBOPTIMAL_KHR , "VK_SUBOPTIMAL_KHR" },
    { VK_ERROR_OUT_OF_DATE_KHR , "VK_ERROR_OUT_OF_DATE_KHR" },
    { VK_ERROR_INCOMPATIBLE_DISPLAY_KHR , "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" },
    { VK_ERROR_VALIDATION_FAILED_EXT , "VK_ERROR_VALIDATION_FAILED_EXT" },
    { VK_ERROR_INVALID_SHADER_NV , "VK_ERROR_INVALID_SHADER_NV" },
    { VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT , "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT" },
    { VK_ERROR_NOT_PERMITTED_EXT , "VK_ERROR_NOT_PERMITTED_EXT" },
    { VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT , "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT" },
    { VK_THREAD_IDLE_KHR , "VK_THREAD_IDLE_KHR" },
    { VK_THREAD_DONE_KHR , "VK_THREAD_DONE_KHR" },
    { VK_OPERATION_DEFERRED_KHR , "VK_OPERATION_DEFERRED_KHR" },
    { VK_OPERATION_NOT_DEFERRED_KHR , "VK_OPERATION_NOT_DEFERRED_KHR" },
    { VK_PIPELINE_COMPILE_REQUIRED_EXT , "VK_PIPELINE_COMPILE_REQUIRED_EXT" },
};

/*  A form of exception that holds a VkResult (since many C-based Vulkan
    functions use a VkResult to indicate success / error-type.  This way
    we can encode in an exception the error code and a string message
    indicating where the error occurred. */
class VulkanException : public std::runtime_error
{
public:
    VulkanException(VkResult result, std::string message);

    VkResult code() const;
    std::string enum_name() const;
    std::string message() const;

private:
    VkResult result;
};

/*  Class representing a surface rendered to by a device.  Interally represented
    as a VkInstance */
class Surface
{
    friend class Instance;
    friend class PhysicalDevice;

    Surface(VkSurfaceKHR surface);

    VkSurfaceKHR surface;
};

/*  Wrapper for VkDevice, generated by the PhysicalDevice by calling
    create_logical_device() */
class LogicalDevice
{
    friend class PhysicalDevice;

private:
    LogicalDevice(VkDevice device, uint32_t queue_family_index);
    VkDevice device;
    uint32_t queue_family_index;
};

/*  Mimics the structure pointed to by VkExtensionProperties, except that it
    uses std::string for name and description */
struct ExtensionInfo
{
    std::string name;
    uint32_t specVersion;
};

/*  Mimics the structure pointed to by VkLayerProperties, except that it uses
    std::string for name and description. */
struct LayerInfo
{
    std::string name;
    std::string description;
    uint32_t specVersion;
    uint32_t implementationVersion;
};

/*  Subclass this and override to specify details on how the  */
class IRequestLayerAndExtensions
{
public:
    virtual std::vector<LayerInfo> get_requested_layers() const = 0;
    virtual std::vector<ExtensionInfo> get_requested_extensions() const = 0;
};

/*  Wrapper for VkPhysicalDevice, stores a VkPhysical device as well
    as the index into the queue-family where the physical device can
    be found. */
class PhysicalDevice
{
    friend class Instance;
    friend class Surface;

public:
    /*  Takes an object called parameters with getters that establish
        requested layers and extensions.  Tries to create a device with
        those layers and extensions.  Throws if it cannot.  */

    LogicalDevice create_logical_device(
        const IRequestLayerAndExtensions& parameters);

    bool is_surface_supported(const Surface& surface);

private:
    PhysicalDevice(
        VkPhysicalDevice physical_device,
        uint32_t queue_family_index);

    VkPhysicalDevice physical_device;
    uint32_t queue_family_index;
};

/*  This class is establishes requirements and requested options when
    creating an Instance. */
class ICreateInstanceParameters : public IRequestLayerAndExtensions
{
public:
    virtual std::string get_application_name() const = 0;
    virtual int get_application_version() const = 0;
    virtual std::string get_engine_name() const = 0;
    virtual int get_engine_version() const = 0;
};


/*  Wrapper for VkInstance */
class Instance
{
public:
    explicit Instance(const ICreateInstanceParameters& parameters);
    virtual ~Instance();

    PhysicalDevice select_gpu();
    Surface create_surface(SDL_Window* window);

private:
    VkInstance instance;
};

/*  Gets a list of available layers straight from the vulkan engine. */
std::vector<LayerInfo> get_layer_infos();

/*  Gets a list of available extensions straight from the vulkan engine. */
std::vector<ExtensionInfo> get_extension_infos();

/*  Takes an SDK_Window pointer an extracts a list of names of Vulkan
    extensions as std::strings. */
std::vector<std::string> get_extension_names(SDL_Window* window);

}