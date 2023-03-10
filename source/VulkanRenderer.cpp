///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Vulkan.hpp"
#include <set>


/// Descriptor constructor                                                    
///   @param producer - the renderer producer                                 
///   @param descriptor - the renderer descriptor                             
VulkanRenderer::VulkanRenderer(Vulkan* producer, const Any& descriptor)
   : Graphics {MetaOf<VulkanRenderer>(), descriptor}
   , ProducedFrom {producer, descriptor}
   , mSwapchain {*this}
   , mLayers {this}
   , mPipelines {this}
   , mShaders {this}
   , mGeometries {this}
   , mTextures {this} {
   // Retrieve relevant traits from the environment                     
   mWindow = SeekUnitAux<A::Window>(descriptor);
   LANGULUS_ASSERT(mWindow, Construct,
      "No window available for renderer");
   SeekTraitAux<Traits::Size>(descriptor, mResolution);

   // Create native surface                                             
   mSwapchain.CreateSurface(mWindow);

   // Check adapter functionality                                       
   const auto adapter = GetAdapter();
   uint32_t queueCount;
   vkGetPhysicalDeviceQueueFamilyProperties(adapter, &queueCount, nullptr);
   if (queueCount == 0)
      LANGULUS_THROW(Graphics, "No queue families");

   std::vector<VkQueueFamilyProperties> queueProperties(queueCount);
   vkGetPhysicalDeviceQueueFamilyProperties(adapter, &queueCount, &queueProperties[0]);

   // Not all queues support presenting. Find one that does.            
   std::vector<VkBool32> supportsPresent(queueCount);
   for (uint32_t i = 0; i < queueCount; i++) {
      if (vkGetPhysicalDeviceSurfaceSupportKHR(adapter, i, mSwapchain.GetSurface(), &supportsPresent[i]))
         LANGULUS_THROW(Graphics, "vkGetPhysicalDeviceSurfaceSupportKHR failed");
   }

   mGraphicIndex = UINT32_MAX;
   mPresentIndex = UINT32_MAX;
   mTransferIndex = UINT32_MAX;
   for (uint32_t i = 0; i < queueCount; i++) {
      if ((queueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
         if (queueProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT && mTransferIndex == UINT32_MAX)
            mTransferIndex = i;
         if (mGraphicIndex == UINT32_MAX)
            mGraphicIndex = i;

         if (supportsPresent[i] == VK_TRUE) {
            mGraphicIndex = i;
            mPresentIndex = i;
            break;
         }
      }
      else if (queueProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
         mTransferIndex = i;
   }

   if (mGraphicIndex == UINT32_MAX || mPresentIndex == UINT32_MAX) {
      LANGULUS_THROW(Graphics,
         "Your graphical adapter doesn't support rendering or presenting to screen");
   }

   if (mGraphicIndex != mPresentIndex) {
      Logger::Warning(Self(),
         "Performance warning: graphics and present queues are on separate devices. "
         "This means that one device might need to wait for the other to complete buffer copy operations");
   }

   if (mTransferIndex == UINT32_MAX) {
      LANGULUS_THROW(Graphics,
         "Your graphical adapter doesn't support memory transfer operations. "
         "Is this even possible? Aborting just in case, because you can't use your VRAM...");
   }
   else if (mTransferIndex == mGraphicIndex || mTransferIndex == mPresentIndex) {
      Logger::Warning(Self(),
         "Performance warning: you do not have a dedicated memory transfer queue. "
         "This means that VRAM copy operations might wait for other GPU operations to finish first");
   }

   // Create queues for rendering & presenting                          
   std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
   std::set<uint32_t> uniqueQueueFamilies {
      mGraphicIndex, mPresentIndex, mTransferIndex
   };

   float queuePriority = 1.0f;
   for (auto queueFamily : uniqueQueueFamilies) {
      VkDeviceQueueCreateInfo queueCreateInfo {};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
   }

   // Create the device with required graphics & presentation           
   std::vector<const char*> extensions;
   extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

   // Specify required features here                                    
   VkPhysicalDeviceFeatures deviceFeatures {};
   deviceFeatures.fillModeNonSolid = VK_TRUE;

   VkDeviceCreateInfo deviceInfo {};
   deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
   deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
   deviceInfo.pEnabledFeatures = &deviceFeatures;
   deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
   deviceInfo.ppEnabledExtensionNames = extensions.data();

   #if LANGULUS_DEBUG()
      // Add debug layers                                               
      deviceInfo.enabledLayerCount = 
         static_cast<uint32_t>(mProducer->GetValidationLayers().size());
      deviceInfo.ppEnabledLayerNames = 
         mProducer->GetValidationLayers().data();
   #endif

   // Create the logical device                                         
   if (vkCreateDevice(adapter, &deviceInfo, nullptr, &mDevice.Get()))
      LANGULUS_THROW(Graphics, "Could not create logical device for rendering");

   mVRAM.Initialize(adapter, mDevice, mTransferIndex);

   vkGetDeviceQueue(mDevice, mGraphicIndex, 0, &mRenderQueue.Get());
   vkGetDeviceQueue(mDevice, mPresentIndex, 0, &mPresentQueue.Get());

   // Create command pool for rendering                                 
   VkCommandPoolCreateInfo poolInfo {};
   poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   poolInfo.queueFamilyIndex = mGraphicIndex;
   poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

   if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool.Get()))
      LANGULUS_THROW(Graphics, "Can't create command pool for rendering");

   // Create the swap chain                                             
   const auto format = mSwapchain.GetSurfaceFormat();
   mSwapchain.Create(format);

   // Define color attachment for the back buffer                       
   VkAttachmentDescription colorAttachment {};
   colorAttachment.format = format.format;
   colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
   colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
   mPassAttachments << colorAttachment;

   // Define depth attachment for the back buffer                       
   VkAttachmentDescription depthAttachment {};
   depthAttachment.format = VK_FORMAT_D32_SFLOAT;
   depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
   depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
   mPassAttachments << depthAttachment;

   // Subpass description                                               
   VkAttachmentReference colorAttachmentRef {};
   colorAttachmentRef.attachment = 0;
   colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

   VkAttachmentReference depthAttachmentRef {};
   depthAttachmentRef.attachment = 1;
   depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

   VkSubpassDescription subpass {};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.colorAttachmentCount = 1;
   subpass.pColorAttachments = &colorAttachmentRef;
   subpass.pDepthStencilAttachment = &depthAttachmentRef;

   // Create the main render pass                                       
   VkSubpassDependency dependency {};
   dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
   dependency.dstSubpass = 0;
   dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   dependency.srcAccessMask = 0;
   dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

   VkRenderPassCreateInfo renderPassInfo {};
   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   renderPassInfo.attachmentCount = static_cast<uint32_t>(mPassAttachments.GetCount());
   renderPassInfo.pAttachments = mPassAttachments.GetRaw();
   renderPassInfo.subpassCount = 1;
   renderPassInfo.pSubpasses = &subpass;
   renderPassInfo.dependencyCount = 1;
   renderPassInfo.pDependencies = &dependency;

   if (vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mPass.Get()))
      LANGULUS_THROW(Graphics, "Can't create main rendering pass");

   // Get device properties                                             
   vkGetPhysicalDeviceProperties(adapter, &mPhysicalProperties);

   // Get device features                                               
   vkGetPhysicalDeviceFeatures(adapter, &mPhysicalFeatures);
}

/// Renderer destruction                                                      
VulkanRenderer::~VulkanRenderer() {
   if (!mDevice)
      return;

   // Destroy anything that was produced by the VkDevice                
   vkDeviceWaitIdle(mDevice);

   mSwapchain.Destroy();

   if (mPass)
      vkDestroyRenderPass(mDevice, mPass, nullptr);

   if (mCommandPool)
      vkDestroyCommandPool(mDevice, mCommandPool, nullptr);

   mVRAM.Destroy();

   vkDestroyDevice(mDevice, nullptr);
}

/// Introduce renderables, cameras, lights, shaders, textures, geometry       
/// Also initialized the renderer if a window is provided                     
///   @param verb - creation verb                                             
void VulkanRenderer::Create(Verb& verb) {
   mLayers.Create(verb);
   mPipelines.Create(verb);
   mShaders.Create(verb);
   mGeometries.Create(verb);
   mTextures.Create(verb);
}

/// Resize the swapchain                                                      
///   @param size - the new size                                              
void VulkanRenderer::Resize(const Vec2& size) {
   if (mResolution != size) {
      mResolution = size;
      mSwapchain.Recreate();
      vkDeviceWaitIdle(mDevice);
   }
}

/// Render an object, along with all of its children                          
/// Rendering pipeline depends on each entity's components                    
void VulkanRenderer::Draw() {
   if (mWindow->IsMinimized())
      return;

   // Wait for previous present to finish                               
   vkQueueWaitIdle(mPresentQueue);

   // Get some global values, like time, mouse position, mouse scroll   
   // But do it in the context of the window, as this renderer is       
   // most likely just its sideproduct and is somewhere else            
   TimeGradient timeGradient;
   SAFETY(if (!mWindow->SeekValue<Traits::Time>(timeGradient))
      Logger::Warning(Self(), "No time gradient found, so shader timers will be all zero");

   auto timeFromInit = timeGradient.Current().SecondsReal();
   Grad2v2 mousePosition;
   mWindow->SeekValue<Traits::MousePosition>(mousePosition);
   Grad2v2 mouseScroll;
   mWindow->SeekValue<Traits::MouseScroll>(mouseScroll);

   // Reset all pipelines that already exist                            
   for (auto& pipe : mPipelines)
      pipe.ResetUniforms();

   // Generate the draw lists for all layers                            
   // This will populate uniform buffers for all relevant pipelines     
   PipelineSet relevantPipes;
   for (auto& layer : mLayers)
      layer.Generate(relevantPipes);

   // Upload any uniform buffer changes to VRAM                         
   // Once this data is uploaded, we're free to prepare the next frame  
   for (auto pipe : relevantPipes) {
      pipe->SetUniform<PerTick, Traits::Time>(timeFromInit);
      pipe->SetUniform<PerTick, Traits::MousePosition>(mousePosition.Current());
      pipe->SetUniform<PerTick, Traits::MouseScroll>(mouseScroll.Current());
      pipe->UpdateUniformBuffers();
   }

   // The actual drawing starts here                                    
   if (!StartRendering())
      return;

   RenderConfig config {
      GetRenderCB(), mPass,
      mFramebuffer[mCurrentFrame]
   };

   config.mColorClear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
   config.mDepthClear.depthStencil = {1.0f, 0};
   config.mDepthSweep.colorAttachment = VK_ATTACHMENT_UNUSED;
   config.mDepthSweep.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
   config.mDepthSweep.clearValue.depthStencil = config.mDepthClear.depthStencil;
   config.mPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   config.mPassBeginInfo.renderPass = config.mPass;
   config.mPassBeginInfo.framebuffer = config.mFrame;
   config.mPassBeginInfo.renderArea.offset = {0, 0};
   config.mPassBeginInfo.renderArea.extent.width = static_cast<uint32_t>(mResolution[0]);
   config.mPassBeginInfo.renderArea.extent.height = static_cast<uint32_t>(mResolution[1]);
   config.mPassBeginInfo.clearValueCount = 2;
   config.mPassBeginInfo.pClearValues = &config.mColorClear;

   // Render all layers                                                 
   for (const auto& layer : mLayers)
      layer.Render(config);

   // Swap buffers and conclude this frame                              
   EndRendering();
}

/// Get the vulkan library instance                                           
///   @return the instance handle                                             
VkInstance VulkanRenderer::GetVulkanInstance() const noexcept {
   return mProducer->mInstance;
}

/// Get the chosen GPU adapter                                                
///   @return the physical device handle                                      
VkPhysicalDevice VulkanRenderer::GetAdapter() const noexcept {
   return mProducer->mAdapter;
}

/// Get hardware dependent UBO outer alignment for dynamic buffers            
///   @return the alignment                                                   
Size VulkanRenderer::GetOuterUBOAlignment() const noexcept {
   return static_cast<Size>(
      mPhysicalProperties.limits.minUniformBufferOffsetAlignment
   );
}

/// Get the command buffer for the current frame                              
///   @return the command buffer                                              
VkCommandBuffer VulkanRenderer::GetRenderCB() const noexcept {
   return mSwapchain.GetRenderCB();
}