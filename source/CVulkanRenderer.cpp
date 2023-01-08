#include "MVulkan.hpp"
#include <iterator>

//#define PC_REPORT_RENDER_STATISTICS

/// Vulkan renderer construction																
///	@param producer - producer of the renderer										
CVulkanRenderer::CVulkanRenderer(MVulkan* producer)
	: ARenderer {MetaData::Of<CVulkanRenderer>()}
	, TProducedFrom {producer}
	, mLayers {this}
	, mPipelines {this}
	, mShaders {this}
	, mGeometries {this}
	, mTextures {this} {
	ClassValidate();
}

/// Renderer destruction																		
CVulkanRenderer::~CVulkanRenderer() {
	Uninitialize();
}

/// Initialize the renderer, creating the swapchain									
///	@param window - the window to use for the renderer								
void CVulkanRenderer::Initialize(AWindow* window) {
	if (mRendererInitialized)
		return;

	mWindow = window;

	if (!mWindow && !mWindow->GetNativeWindowHandle()) {
		Uninitialize();
		throw Except::Graphics(pcLogSelfError
			<< "Provided window handle is invalid");
	}

	// Copy resolution																	
	mResolution = mWindow->GetSize();

	// Create surface																		
	if (!pcCreateNativeVulkanSurfaceKHR(mProducer->GetInstance(), mWindow, mSurface)) {
		Uninitialize();
		throw Except::Graphics(pcLogSelfError
			<< "Error creating window surface. Aborting...");
	}

	// Check adapter functionality													
	auto adapter = mProducer->GetAdapter();
	uint32_t queueCount;
	vkGetPhysicalDeviceQueueFamilyProperties(adapter, &queueCount, nullptr);
	if (queueCount == 0) {
		Uninitialize();
		throw Except::Graphics(pcLogSelfError
			<< "Your adapter is broken... i think. Aborting...");
	}

	std::vector<VkQueueFamilyProperties> queueProperties(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(adapter, &queueCount, &queueProperties[0]);

	// Not all queues support presenting. Find one that does.				
	std::vector<VkBool32> supportsPresent(queueCount);
	for (uint32_t i = 0; i < queueCount; i++) {
		if (vkGetPhysicalDeviceSurfaceSupportKHR(adapter, i, mSurface, &supportsPresent[i])) {
			Uninitialize();
			throw Except::Graphics(pcLogSelfError
				<< "vkGetPhysicalDeviceSurfaceSupportKHR failed. Aborting...");
		}
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
		Uninitialize();
		throw Except::Graphics(pcLogSelfError
			<< "Your graphical adapter doesn't support rendering or presenting to screen. "
			<< "Are you using it only for mining BitCoin? Aborting, because we can't render with it...");
	}

	if (mGraphicIndex != mPresentIndex) {
		pcLogSelfWarning
			<< "Performance warning: graphics and present queues are on separate devices. "
			<< "This means that one device might need to wait for the other to complete buffer copy operations";
	}

	if (mTransferIndex == UINT32_MAX) {
		Uninitialize();
		throw Except::Graphics(pcLogSelfError
			<< "Your graphical adapter doesn't support memory transfer operations. "
			<< "Is this even possible? Aborting just in case, because you can't use your VRAM...");
	}
	else if (mTransferIndex == mGraphicIndex || mTransferIndex == mPresentIndex) {
		pcLogSelfWarning
			<< "Performance warning: you do not have a dedicated memory transfer queue. "
			<< "This means that VRAM copy operations might wait for other GPU operations to finish first";
	}

	// Create queues for rendering & presenting									
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {
		mGraphicIndex,
		mPresentIndex,
		mTransferIndex
	};

	float queuePriority = 1.0f;
	for (auto queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
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
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = VK_TRUE;

	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceInfo.queueCreateInfoCount = uint32_t(queueCreateInfos.size());
	deviceInfo.pEnabledFeatures = &deviceFeatures;
	deviceInfo.enabledExtensionCount = uint32_t(extensions.size());
	deviceInfo.ppEnabledExtensionNames = extensions.data();

	#if LANGULUS_DEBUG()
		// Add debug layers																
		deviceInfo.enabledLayerCount = uint32_t(mProducer->GetValidationLayers().size());
		deviceInfo.ppEnabledLayerNames = mProducer->GetValidationLayers().data();
	#else
		deviceInfo.enabledLayerCount = 0;
	#endif

	// Create the logical device														
	if (vkCreateDevice(adapter, &deviceInfo, nullptr, &mDevice.mValue)) {
		Uninitialize();
		throw Except::Graphics(pcLogSelfError
			<< "Could not create logical device for rendering. Aborting...");
	}

	if (!mVRAM.Initialize(adapter, mDevice, mTransferIndex)) {
		Uninitialize();
		throw Except::Graphics(pcLogSelfError
			<< "Could not inspect device memory...");
	}

	vkGetDeviceQueue(mDevice, mGraphicIndex, 0, &mRenderQueue.mValue);
	vkGetDeviceQueue(mDevice, mPresentIndex, 0, &mPresentQueue.mValue);

	// Create command pool for rendering											
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = mGraphicIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool.mValue)) {
		Uninitialize();
		throw Except::Graphics(pcLogSelfError
			<< "Can't create command pool for rendering. Aborting...");
	}

	// Create sync primitives															
	VkSemaphoreCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(mDevice, &fenceInfo, nullptr, &mNewFrameFence.mValue)) {
		Uninitialize();
		throw Except::Graphics(pcLogSelfError
			<< "Can't create new frame semaphore. Aborting...");
	}

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mFrameFinished.mValue)) {
		Uninitialize();
		throw Except::Graphics(pcLogSelfError
			<< "Can't create frame finished semaphore. Aborting...");
	}

	VkSurfaceFormatKHR format = GetSurfaceFormat();

	// Define color attachment for the back buffer								
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = format.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	mPassAttachments.push_back(colorAttachment);

	// Define depth attachment for the back buffer								
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = VK_FORMAT_D32_SFLOAT;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	mPassAttachments.push_back(depthAttachment);

	// Subpass description																
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	// Create the main render pass													
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(mPassAttachments.size());
	renderPassInfo.pAttachments = mPassAttachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	if (vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mPass.mValue)) {
		Uninitialize();
		throw Except::Graphics(pcLogSelfError
			<< "Can't create main rendering pass. Aborting...");
	}

	// Create the swap chain															
	if (!CreateSwapchain(format)) {
		Uninitialize();
		throw Except::Graphics(pcLogSelfError
			<< "Can't create graphical pipeline. Aborting...");
	}

	// Get device properties															
	vkGetPhysicalDeviceProperties(mProducer->GetAdapter(), &mPhysicalProperties);

	// Get device features																
	vkGetPhysicalDeviceFeatures(mProducer->GetAdapter(), &mPhysicalFeatures);
	mRendererInitialized = true;
}

/// Introduce renderables, cameras, lights, shaders, textures, geometry			
/// Also initialized the renderer if a window is provided							
///	@param verb - creation verb															
void CVulkanRenderer::Create(Verb& verb) {
	verb.GetArgument().ForEachDeep([&](AWindow* window) {
		Initialize(window);
		verb.Done();
	});

	mLayers.Create(verb);
	mPipelines.Create(verb);
	mShaders.Create(verb);
	mGeometries.Create(verb);
	mTextures.Create(verb);
}

/// Deinitialize the renderer																	
void CVulkanRenderer::Uninitialize() {
	if (mDevice) {
		vkDeviceWaitIdle(mDevice);

		DestroySwapchain();

		if (mPass) {
			vkDestroyRenderPass(mDevice, mPass, nullptr);
			mPass.Reset();
		}

		if (mFrameFinished) {
			vkDestroySemaphore(mDevice, mFrameFinished, nullptr);
			mFrameFinished.Reset();
		}

		if (mNewFrameFence) {
			vkDestroySemaphore(mDevice, mNewFrameFence, nullptr);
			mNewFrameFence.Reset();
		}

		if (mCommandPool) {
			vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
			mCommandPool.Reset();
		}

		mVRAM.Destroy();

		vkDestroyDevice(mDevice, nullptr);
		mDevice.Reset();
	}

	if (mSurface) {
		vkDestroySurfaceKHR(mProducer->GetInstance(), mSurface, nullptr);
		mSurface.Reset();
	}
}

/// Get backbuffer surface format															
///	@return the surface format of the swap chain										
VkSurfaceFormatKHR CVulkanRenderer::GetSurfaceFormat() const noexcept {
	// Check supported image formats													
	const VkSurfaceFormatKHR error = {
		VK_FORMAT_MAX_ENUM, VK_COLOR_SPACE_MAX_ENUM_KHR
	};

	auto adapter = mProducer->GetAdapter();
	std::vector<VkSurfaceFormatKHR> surface_formats;
	uint32_t formatCount;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(adapter, mSurface, &formatCount, nullptr)) {
		pcLogSelfError << "vkGetPhysicalDeviceSurfaceFormatsKHR failed. Aborting...";
		return error;
	}

	if (formatCount != 0) {
		surface_formats.resize(formatCount);
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(adapter, mSurface, &formatCount, surface_formats.data())) {
			pcLogSelfError << "vkGetPhysicalDeviceSurfaceFormatsKHR failed. Aborting...";
			return error;
		}
	}

	if (surface_formats.empty()) {
		pcLogSelfError << "Could not create swap chain. Aborting...";
		return error;
	}

	// Choose image format																
	VkSurfaceFormatKHR surfaceFormat = { VK_FORMAT_MAX_ENUM, VK_COLOR_SPACE_MAX_ENUM_KHR };
	if (surface_formats.size() == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
		surfaceFormat = {
			VK_FORMAT_B8G8R8A8_UNORM,
			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		};
	}
	else for (const auto& availableFormat : surface_formats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
			surfaceFormat = availableFormat;
			break;
		}
	}

	if (surfaceFormat.colorSpace == VK_COLOR_SPACE_MAX_ENUM_KHR) {
		pcLogSelfError << "Incompatible surface format and color space for swap chain. Aborting...";
		return error;
	}

	return surfaceFormat;
}

/// Create the swapchain																		
///	@param format - surface format														
///	@return true on success																	
bool CVulkanRenderer::CreateSwapchain(const VkSurfaceFormatKHR& format) {
	// Resolution																			
	auto adapter = mProducer->GetAdapter();
	const real resx = mResolution[0];
	const real resy = mResolution[1];
	const auto resxuint = uint32_t(resx);
	const auto resyuint = uint32_t(resy);
	if (resxuint == 0 || resyuint == 0) {
		throw Except::Graphics(pcLogSelfError
			<< "Bad resolution: " << resxuint << "x" << resyuint);
	}

	// Create swap chain																	
	VkSurfaceCapabilitiesKHR surface_caps;
	memset(&surface_caps, 0, sizeof(VkSurfaceCapabilitiesKHR));
	std::vector<VkPresentModeKHR>	surface_presentModes;
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(adapter, mSurface, &surface_caps)) {
		pcLogSelfError << "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed. Aborting...";
		return false;
	}

	// Check supported present modes													
	uint32_t presentModeCount;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(adapter, mSurface, &presentModeCount, nullptr)) {
		pcLogSelfError << "vkGetPhysicalDeviceSurfacePresentModesKHR failed. Aborting...";
		return false;
	}

	if (presentModeCount != 0) {
		surface_presentModes.resize(presentModeCount);
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(adapter, mSurface, &presentModeCount, surface_presentModes.data())) {
			pcLogSelfError << "vkGetPhysicalDeviceSurfacePresentModesKHR failed. Aborting...";
			return false;
		}
	}
	
	if (surface_presentModes.empty()) {
		pcLogSelfError << "Could not create swap chain. Aborting...";
		return false;
	}

	// Choose present mode																
	auto surfacePresentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
	for (const auto& availablePresentMode : surface_presentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			surfacePresentMode = availablePresentMode;
	}

	VkExtent2D extent = { resxuint, resyuint };
	if (surface_caps.currentExtent.width != VK_INDEFINITELY)
		extent = surface_caps.currentExtent;
	else {
		extent.width = pcMax(surface_caps.minImageExtent.width, pcMin(surface_caps.maxImageExtent.width, extent.width));
		extent.height = pcMax(surface_caps.minImageExtent.height, pcMin(surface_caps.maxImageExtent.height, extent.height));
	}

	uint32_t imageCount = surface_caps.minImageCount + 1;
	if (surface_caps.maxImageCount > 0 && imageCount > surface_caps.maxImageCount)
		imageCount = surface_caps.maxImageCount;

	// Setup the swapchain																
	VkSwapchainCreateInfoKHR swapInfo = {};
	swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapInfo.surface = mSurface;
	swapInfo.minImageCount = imageCount;
	swapInfo.imageFormat = format.format;
	swapInfo.imageColorSpace = format.colorSpace;
	swapInfo.imageExtent = extent;
	swapInfo.imageArrayLayers = 1;	// 2 if stereoscopic						
	swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	const std::set<uint32_t> families = { mGraphicIndex, mPresentIndex, mTransferIndex };
	std::vector<uint32_t> familiesvec;
	std::copy(families.begin(), families.end(), ::std::back_inserter(familiesvec));
	swapInfo.imageSharingMode = familiesvec.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
	swapInfo.queueFamilyIndexCount = uint32_t(familiesvec.size());
	swapInfo.pQueueFamilyIndices = familiesvec.data();
	swapInfo.preTransform = surface_caps.currentTransform;
	swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapInfo.presentMode = surfacePresentMode;
	swapInfo.clipped = VK_TRUE;
	swapInfo.oldSwapchain = VK_NULL_HANDLE;
	if (vkCreateSwapchainKHR(mDevice, &swapInfo, nullptr, &mSwapChain.mValue)) {
		pcLogSelfError << "Can't create swap chain. Aborting...";
		return false;
	}

	// Get images inside swapchain													
	if (vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, nullptr)) {
		pcLogSelfError << "vkGetSwapchainImagesKHR fails. Aborting...";
		return false;
	}

	mSwapChainImages.resize(imageCount);
	if (vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, mSwapChainImages.data())) {
		pcLogSelfError << "vkGetSwapchainImagesKHR fails. Aborting...";
		return false;
	}

	// Create the image views for the swap chain. They will all be			
	// single layer, 2D images, with no mipmaps.									
	bool reverseFormat;
	const auto viewf = pcVkFormatToDataID(format.format, reverseFormat).GetMeta();
	PixelView colorview(extent.width, extent.height, 1, 1, viewf, reverseFormat);
	mFrame.resize(mSwapChainImages.size());
	for (uint32_t i = 0; i < mSwapChainImages.size(); i++) {
		mVRAM.ImageTransfer(mSwapChainImages[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		mVRAM.ImageTransfer(mSwapChainImages[i], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		mFrame[i] = mVRAM.CreateImageView(mSwapChainImages[i], colorview, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	// Create the depth buffer image and view										
	PixelView depthview(extent.width, extent.height, 1, 1, DataID::Reflect<depth32>());
	mDepthImage = mVRAM.CreateImage(depthview, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	mDepthImageView = mVRAM.CreateImageView(mDepthImage.mBuffer, depthview);
	mVRAM.ImageTransfer(mDepthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		
	// Create framebuffers																
	mFramebuffer.resize(mFrame.size());
	for (uint32_t i = 0; i < mFrame.size(); ++i) {
		VkImageView attachments[2] = { mFrame[i], mDepthImageView };
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = mPass;
		framebufferInfo.attachmentCount = 2;
		framebufferInfo.pAttachments = static_cast<const VkImageView*>(attachments);
		framebufferInfo.width = resxuint;
		framebufferInfo.height = resyuint;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &mFramebuffer[i])) {
			pcLogSelfError << "Can't create framebuffer. Aborting...";
			return false;
		}
	}

	// Create a command buffer for each framebuffer								
	mCommandBuffer.resize(mFramebuffer.size());
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = mCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = uint32_t(mCommandBuffer.size());
	if (vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffer.data())) {
		pcLogSelfError << "Can't create command buffers. Aborting...";
		return false;
	}

	// Create command buffer fences													
	if (mNewBufferFence.empty()) {
		std::vector<VkFenceCreateInfo> fenceInfo(mFramebuffer.size());
		mNewBufferFence.resize(mFramebuffer.size());
		uint32_t bf = 0;
		for (auto& it : fenceInfo) {
			it = {};
			it.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			if (vkCreateFence(mDevice, &it, nullptr, &mNewBufferFence[bf])) {
				pcLogSelfError << "Can't create buffer fence. Aborting...";
				return false;
			}
			++bf;
		}
	}

	mCurrentFrame = 0;
	ClassValidate();
	return true;
}

/// Recreate the swapchain (usually on window resize)									
///	@return true on success																	
bool CVulkanRenderer::RecreateSwapchain() {
	if (!mDevice)
		return false;

	vkDeviceWaitIdle(mDevice);
	DestroySwapchain();
	return CreateSwapchain(GetSurfaceFormat());
}

/// Destroy the current swapchain															
void CVulkanRenderer::DestroySwapchain() {
	// Destroy fences																		
	for (auto& it : mNewBufferFence)
		vkDestroyFence(mDevice, it, nullptr);
	mNewBufferFence.clear();

	// Destroy the depth buffer images												
	vkDestroyImageView(mDevice, mDepthImageView, nullptr);
	mDepthImageView = 0;
	mVRAM.DestroyImage(mDepthImage);

	// Destroy framebuffers																
	for (auto &it : mFramebuffer)
		vkDestroyFramebuffer(mDevice, it, nullptr);
	mFramebuffer.clear();
	
	// Destroy command buffers. The command pool remains intact				
	vkFreeCommandBuffers(mDevice, mCommandPool, uint32_t(mCommandBuffer.size()), mCommandBuffer.data());
	mCommandBuffer.clear();
	
	// Destroy image views																
	for (auto &it : mFrame)
		vkDestroyImageView(mDevice, it, nullptr);
	mFrame.clear();
	
	// Destroy swapchain																	
	vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
	mSwapChain = 0;
	mSwapChainImages.clear();
}

/// Interpret anything as graphics															
/// This verb can either mirror content in VRAM, or interpret geometry into	
/// pixels. In other words - this is essentially the rendering routine!			
///	@param verb - interpretation verb													
void CVulkanRenderer::Interpret(Verb&) {
	//TODO
}

/// Pick a back buffer and start writing the command buffer							
///	@return true if something was rendered												
bool CVulkanRenderer::StartRendering() {
	// Set next frame from the swapchain											
	// This changes mCurrentFrame globally for this renderer					
	auto result = vkAcquireNextImageKHR(
		mDevice, mSwapChain, VK_INDEFINITELY,
		mFrameFinished, VK_NULL_HANDLE, &mCurrentFrame
	);

	// Check if resolution has changed												
	if (result == VK_ERROR_OUT_OF_DATE_KHR || (result && result != VK_SUBOPTIMAL_KHR)) {
		// Le strange error occurs														
		pcLogSelfError << "Vulkan failed to resize swapchain";
		return false;
	}

	// Turn the present source to color attachment								
	mVRAM.ImageTransfer(
		mSwapChainImages[mCurrentFrame], 
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	);

	// Begin writing to command buffer												
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	vkBeginCommandBuffer(mCommandBuffer[mCurrentFrame], &beginInfo);
	return true;
}

/// Submit command buffers and present														
///	@return true if something was rendered												
bool CVulkanRenderer::EndRendering() {
	// Command buffer ends																
	if (vkEndCommandBuffer(mCommandBuffer[mCurrentFrame]))
		pcLogSelfError << "Can't end command buffer";

	// Submit command buffer to GPU													
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = { mFrameFinished };

	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = static_cast<const VkSemaphore*>(waitSemaphores);
	submitInfo.pWaitDstStageMask = static_cast<const VkPipelineStageFlags*>(waitStages);
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mCommandBuffer[mCurrentFrame];

	VkSemaphore signalSemaphores[] = { mNewFrameFence };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = static_cast<const VkSemaphore*>(signalSemaphores);
	if (vkQueueSubmit(mRenderQueue, 1, &submitInfo, VK_NULL_HANDLE)) {
		pcLogSelfError << "Vulkan failed to submit render buffer";
		return false;
	}

	// Present and return																
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = static_cast<const VkSemaphore*>(signalSemaphores);

	VkSwapchainKHR swapChains[] = { mSwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = static_cast<const VkSwapchainKHR*>(swapChains);
	presentInfo.pImageIndices = &mCurrentFrame;

	mVRAM.ImageTransfer(
		mSwapChainImages[mCurrentFrame],
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	);

	if (vkQueuePresentKHR(mPresentQueue, &presentInfo))
		pcLogSelfError << "Vulkan failed to present - the frame will probably be lost";
	return true;
}

/// Resize the swapchain																		
///	@param size - the new size																
void CVulkanRenderer::Resize(const vec2& size) {
	if (!mSwapChain || mResolution != size) {
		mResolution = size;

		if (!RecreateSwapchain()) {
			throw Except::Graphics(pcLogSelfError
				<< "Swapchain recreation failed");
		}

		// Wait for resize to finish													
		vkDeviceWaitIdle(mDevice);
	}
}

/// Render an object, along with all of its children									
/// Rendering pipeline depends on each entity's components							
void CVulkanRenderer::Update() {
	if (!mWindow || mWindow->IsMinimized())
		return;

	// Wait for previous present to finish											
	vkQueueWaitIdle(mPresentQueue);

	// Get some global values, like time, mouse position, mouse scroll	
	// But do it in the context of the window, as this renderer is			
	// most likely just its sideproduct and is somewhere else				
	TimeGradient timeGradient;
	SAFETY(if (!mWindow->SeekValue<Traits::Time>(timeGradient)) pcLogSelfWarning
		<< "No time gradient found, so shader timers will be all zero");
	auto timeFromInit = timeGradient.Current().SecondsReal();
	TGrad2<vec2> mousePosition;
	mWindow->SeekValue<Traits::MousePosition>(mousePosition);
	TGrad2<vec2> mouseScroll;
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
		pipe->SetUniform<RRate::PerTick, Traits::Time>(timeFromInit);
		pipe->SetUniform<RRate::PerTick, Traits::MousePosition>(mousePosition.Current());
		pipe->SetUniform<RRate::PerTick, Traits::MouseScroll>(mouseScroll.Current());
		pipe->UpdateUniformBuffers();
	}

	// The actual drawing starts here												
	if (!StartRendering())
		return;

	// Render all layers																	
	for (const auto& layer : mLayers)
		layer.Render(mCommandBuffer[mCurrentFrame], mPass, mFramebuffer[mCurrentFrame]);

	// Swap buffers and conclude this frame										
	EndRendering();
}