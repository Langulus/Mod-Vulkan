///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Vulkan.hpp"

LANGULUS_DEFINE_MODULE(
   Vulkan, 11, "Vulkan",
   "Vulkan graphics module and GPU computation", "",
   Vulkan, VulkanRenderer, VulkanLayer, VulkanCamera, VulkanRenderable, VulkanLight
)

/// Whether or not to enable verbose and info logging of vulkan messages      
#define VERBOSE_VULKAN(a) //a

#if LANGULUS(DEBUG)
   /// Debug relay for vulkan messages                                        
   static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanLogRelay(
      VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT,
      uint64_t, size_t, int32_t, const char*, const char* msg, void*)
   {
      // Different message types will be colored in a different way     
      if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
         Logger::Error() << "Vulkan: " << msg;
      else if ((flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) || (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
         Logger::Warning() << "Vulkan: " << msg;
      VERBOSE_VULKAN(
         else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
            Logger::Info() << "Vulkan: " << msg;
         else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
            Logger::Verbose() << "Vulkan: " << msg;
      )
      return VK_FALSE;
   }

   /// Check validation layer support                                         
   ///   @param layers - the requested layers                                 
   ///   @return true if all required layers are supported                    
   void Vulkan::CheckValidationLayerSupport(const TokenSet& layers) const {
      // Get all validation layers supported                            
      uint32_t layerCount;
      vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
      TAny<VkLayerProperties> availableLayers;
      availableLayers.New(layerCount);
      vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.GetRaw());

      // Check if requested layers are supported                        
      for (auto layerName : layers) {
         bool found = false;
         for (auto& available : availableLayers) {
            if (0 == strcmp(available.layerName, layerName)) {
               found = true;
               break;
            }
         }

         if (!found) {
            Logger::Error("Missing validation layer for debugging: ", layerName);
            LANGULUS_THROW(Graphics, "Vulkan module failed to initialize");
         }
      }
   }

   /// Get validation layers                                                  
   ///   @return a reference to the available validation layers               
   auto& Vulkan::GetValidationLayers() const noexcept {
      return mValidationLayers;
   }
#endif


/// Vulkan module construction                                                
///   @param system - the system that owns the module instance                
///   @param handle - the library handle                                      
Vulkan::Vulkan(Runtime* runtime, const Any&)
   : Module {MetaOf<Vulkan>(), runtime}
   , mRenderers {this} {
   Logger::Verbose(Self(), "Initializing...");

   VkApplicationInfo appInfo {};
   appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   appInfo.pApplicationName = "Langulus";
   appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.pEngineName = "Langulus";
   appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.apiVersion = VK_API_VERSION_1_0;

   // Get required extensions                                           
   auto extensions = GetRequiredExtensions();

   // Setup the required validation layers                              
   #if LANGULUS_DEBUG()
      Logger::Warning(Self(), "Vulkan will work in debug mode "
         "- performance warning due to validation layers");

      mValidationLayers.emplace_back("VK_LAYER_KHRONOS_validation");
      CheckValidationLayerSupport(mValidationLayers);
   #endif

   // Instance creation info                                            
   VkInstanceCreateInfo createInfo = {};
   createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   createInfo.pApplicationInfo = &appInfo;
   createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
   createInfo.ppEnabledExtensionNames = extensions.data();
   #if LANGULUS_DEBUG()
      createInfo.enabledLayerCount = static_cast<uint32_t>(mValidationLayers.size());
      createInfo.ppEnabledLayerNames = mValidationLayers.data();
   #else
      createInfo.enabledLayerCount = 0;
   #endif

   // Create instance                                                   
   auto result = vkCreateInstance(&createInfo, nullptr, &mInstance.Get());
   if (result != VK_SUCCESS) {
      Logger::Error(Self(), "Error creating Vulkan instance");

      if (result == VK_ERROR_EXTENSION_NOT_PRESENT) {
         Logger::Error(Self(), "There was at least one unsupported extension, analyzing...");

         // Get all supported extensions                                
         uint32_t extensionCount = 0;
         vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
         std::vector<VkExtensionProperties> available(extensionCount);
         vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, available.data());

         // Test each GLFW requirement against them                     
         for (const auto& test : extensions) {
            bool found = false;
            for (const auto& ext : available) {
               if (0 == strcmp(test, ext.extensionName)) {
                  found = true;
                  break;
               }
            }

            if (!found)
               Logger::Error(Self(), " - Missing extension: ", test);
         }
      }
      else if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
         Logger::Error(Self(), 
            "You're either out of HOST memory, or your CPU/GPU doesn't support vulkan");
         Logger::Error(Self(), 
            "Beware, that only 4th+ generation CPUs with integrated video adapters "
            "are supported, when specific drivers are provided");
      }
      else if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
         Logger::Error(Self(), "Your driver is incompatible");
      }

      LANGULUS_THROW(Graphics, "Vulkan couldn't setup");
   }

   VERBOSE_VULKAN(Logger::Verbose(Self(),
      "Vulkan instance created"));

   #if LANGULUS_DEBUG()
      // Setup the debug relay                                          
      VkDebugReportCallbackCreateInfoEXT relayInfo = {};
      relayInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;

      #ifdef LANGULUS_LOGGER_ENABLE_ERRORS
         relayInfo.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
      #endif

      #ifdef LANGULUS_LOGGER_ENABLE_WARNINGS
         relayInfo.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT
                         |  VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
      #endif

      #ifdef LANGULUS_LOGGER_ENABLE_VERBOSE
         relayInfo.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
      #endif

      #ifdef LANGULUS_LOGGER_ENABLE_INFOS
         relayInfo.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
      #endif

      relayInfo.pfnCallback = VulkanLogRelay;

      // Get the debug relay extension                                  
      auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugReportCallbackEXT");
      if (func)
         func(mInstance, &relayInfo, nullptr, &mLogRelay);
      else
         throw Except::Graphics(pcLogSelfError
            << "Can not find vkCreateDebugReportCallbackEXT function - build as release "
            << "or change PC_DEBUG to PC_DISABLED, if you don't need this");
   #endif

   // Pick good hardware                                                
   mAdapter = PickAdapter();
   if (!mAdapter)
      throw Except::Graphics(pcLogSelfError
         << "Error picking graphics adapter - vulkan module is unusable");

   // Show some info                                                    
   VkPhysicalDeviceProperties adapter_info;
   vkGetPhysicalDeviceProperties(mAdapter, &adapter_info);
   VERBOSE_VULKAN(Logger::Verbose(Self(),
      "Best rated adapter: ", adapter_info.deviceName));

   // Check adapter functionality                                       
   uint32_t queueCount;
   vkGetPhysicalDeviceQueueFamilyProperties(mAdapter, &queueCount, nullptr);
   if (queueCount == 0)
      throw Except::Graphics(pcLogSelfError
         << "Your adapter is broken... i think - vulkan module is unusable");

   std::vector<VkQueueFamilyProperties> queueProperties(queueCount);
   vkGetPhysicalDeviceQueueFamilyProperties(mAdapter, &queueCount, &queueProperties[0]);
   uint32_t computeIndex = UINT32_MAX;
   for (uint32_t i = 0; i < queueCount; i++) {
      if (queueProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT && computeIndex == UINT32_MAX) {
         mSupportsComputation = true;
         computeIndex = i;
      }
      if (queueProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
         mSupportsTransfer = true;
      if (queueProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
         mSupportsSparseBinding = true;
   }

   if (mSupportsComputation)
      pcLogSelfVerbose << "Your GPU supports data computation - it will be used for large-scale computation";
   if (mSupportsTransfer)
      pcLogSelfVerbose << "Your GPU supports asynchronous queues";
   if (mSupportsSparseBinding)
      pcLogSelfVerbose << "Your GPU supports sparse binding";

   // Create queues for rendering & presenting                          
   std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
   std::set<uint32_t> uniqueQueueFamilies = { computeIndex };
   float queuePriority = 1.0f;
   for (int queueFamily : uniqueQueueFamilies) {
      VkDeviceQueueCreateInfo queueCreateInfo = {};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
   }

   VkPhysicalDeviceFeatures deviceFeatures = {};
   deviceFeatures.samplerAnisotropy = VK_TRUE;

   VkDeviceCreateInfo deviceInfo = {};
   deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
   deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
   deviceInfo.pEnabledFeatures = &deviceFeatures;
   deviceInfo.enabledExtensionCount = 0;

   #if LANGULUS_DEBUG()
      deviceInfo.enabledLayerCount = static_cast<uint32_t>(GetValidationLayers().size());
      deviceInfo.ppEnabledLayerNames = GetValidationLayers().data();
   #else
      deviceInfo.enabledLayerCount = 0;
   #endif

   // Create the computation device                                     
   if (vkCreateDevice(mAdapter, &deviceInfo, nullptr, &mDevice.mValue))
      throw Except::Graphics(pcLogSelfError
         << "Could not create logical device for rendering - vulkan module is unusable");

   // Get computation queue                                             
   vkGetDeviceQueue(mDevice, computeIndex, 0, &mComputer.mValue);

   pcLogSelfVerbose << "Initialized";
   ClassValidate();
}

/// Vulkan module destruction                                                 
Vulkan::~Vulkan() {
   if (mDevice) {
      vkDestroyDevice(mDevice, nullptr);
      mDevice = nullptr;
   }

   #if LANGULUS_DEBUG()
      auto func = (PFN_vkDestroyDebugReportCallbackEXT)
         vkGetInstanceProcAddr(mInstance, "vkDestroyDebugReportCallbackEXT");
      if (func)
         func(mInstance, mLogRelay, nullptr);
   #endif

   if (mInstance) {
      vkDestroyInstance(mInstance, nullptr);
      mInstance = nullptr;
   }
}

/// Rate a graphical adapter                                                  
///   @param device - the physical adapter to rate                            
///   @return the rating                                                      
unsigned Vulkan::RateDevice(const VkPhysicalDevice& device) const {
   // Get device properties                                             
   VkPhysicalDeviceProperties deviceProperties;
   vkGetPhysicalDeviceProperties(device, &deviceProperties);

   // Get device features                                               
   VkPhysicalDeviceFeatures deviceFeatures;
   vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

   unsigned score = 0;

   // Discrete GPUs have a significant performance advantage            
   if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
      score += 1000;

   // Maximum possible size of textures affects graphics quality        
   score += deviceProperties.limits.maxImageDimension2D;

   // Application can't function without geometry shaders               
   if (!deviceFeatures.geometryShader) {
      pcLogSelfError << "Device doesn't support geometry shaders";
      return 0;
   }

   // Application can't function without anistropic filtering           
   if (!deviceFeatures.samplerAnisotropy) {
      pcLogSelfError << "Device doesn't support anistropic filtering";
      return 0;
   }

   return score;
}

/// Pick the best rated adapter                                               
///   @return the physical device with the best rating                        
VkPhysicalDevice Vulkan::PickAdapter() const {
   pcu32 deviceCount = 0;
   vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);

   if (deviceCount == 0) {
      pcLogSelfError << "No graphical device was detected in your computer - vulkan module is unusable";
      return nullptr;
   }

   std::vector<VkPhysicalDevice> devices(deviceCount);
   vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

   // Rate all devices, pick best                                       
   pcptr best_rated = 0;
   VkPhysicalDevice result = nullptr;
   for (const auto& device : devices) {
      pcptr newdevice = RateDevice(device);
      if (newdevice > best_rated) {
         best_rated = newdevice;
         result = device;
      }
   }

   if (best_rated == 0)
      pcLogSelfWarning << "The graphical hardware you have is quite shitty";

   return result;
}

/// Get required extension layers                                             
std::vector<const char*> Vulkan::GetRequiredExtensions() const {
   std::vector<const char*> extensions;
   #if LANGULUS_DEBUG()
      extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
   #endif
   
   extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
   #if LANGULUS_OS_IS(WINDOWS)
      extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
   #endif
   return extensions;
}

/// Module update routine                                                     
///   @param dt - time from last update                                       
void Vulkan::Update(PCTime) {
   for (auto& renderer : mRenderers)
      renderer.Update();
}

/// Create/destroy renderers                                                  
///   @param verb - the creation/destruction verb                             
void Vulkan::Create(Verb& verb) {
   mRenderers.Create(verb);
}
