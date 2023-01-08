#include "VRAM.hpp"

/// Initialize the VRAM interface															
///	@param adapter - the physical device												
///	@param logicalDevice - the logical device											
///	@param transferIndex - the transfer index											
bool VRAM::Initialize(VkPhysicalDevice adapter, VkDevice logicalDevice, uint32_t transferIndex) {
	mAdapter = adapter;
	mDevice = logicalDevice;

	// Query memory capabilities														
	vkGetPhysicalDeviceMemoryProperties(adapter, &mVRAM);

	// Get transfer queue																
	vkGetDeviceQueue(mDevice, transferIndex, 0, &mTransferer);

	// Create command pool for transferring										
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = transferIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mTransferPool)) {
		pcLogFuncError << "Can't create command pool for transferring. Aborting...";
		return false;
	}

	// Create dedicated transfer command buffer									
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = mTransferPool;
	allocInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(mDevice, &allocInfo, &mTransferBuffer);

	pcLogFuncInfo << mVRAM.memoryHeapCount << " memory heaps available.";
	pcLogFuncInfo << mVRAM.memoryTypeCount << " memory types available.";
	return true;
}

/// Destroy the VRAM interface																
void VRAM::Destroy() {
	// Destroy transfer buffer															
	vkFreeCommandBuffers(mDevice, mTransferPool, 1, &mTransferBuffer);
	mTransferBuffer = nullptr;

	// Destroy transfer pool															
	vkDestroyCommandPool(mDevice, mTransferPool, nullptr);
	mTransferPool = 0;
}

/// Destroy an allocated buffer																
///	@param buffer - buffer to destroy													
void VRAM::DestroyBuffer(VRAMBuffer& buffer) const {
	if (buffer.mDevice) {
		if (buffer.mBuffer)
			vkDestroyBuffer(buffer.mDevice, buffer.mBuffer, nullptr);
		if (buffer.mMemory)
			vkFreeMemory(buffer.mDevice, buffer.mMemory, nullptr);
	}

	buffer.Reset();
}

/// Check if a given format is supported by the hardware								
///	@param format - check this format for support									
///	@param tiling - the requested tiling to check for support					
///	@param features - features to check for											
///	@returns true if features are supported for the given tiling & format	
bool VRAM::CheckFormatSupport(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features) const {
	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(mAdapter, format, &props);
	if (tiling == VK_IMAGE_TILING_LINEAR)
		return (props.linearTilingFeatures & features) == features;
	if (tiling == VK_IMAGE_TILING_OPTIMAL)
		return (props.optimalTilingFeatures & features) == features;
	return false;
}

/// Create an image																				
///	@param view - image view																
///	@param flags - image usage flags														
///	@return an image instance																
VRAMImage VRAM::CreateImage(const PixelView& view, VkImageUsageFlags flags) const {
	// Precheck some simple constraints												
	if (view.mFormat == nullptr || (view.mWidth * view.mHeight * view.mDepth * view.mFrames == 0))
		throw Except::Graphics(pcLogFuncError << "Wrong texture descriptor");

	// Check if image format is supported. Perform conversion if possible
	auto meta = view.mFormat;
	auto imageFormat = pcTypeToVkFormat(view.mFormat);
	bool using_optimal_tiling = true;
	if (!CheckFormatSupport(imageFormat, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
		if (!CheckFormatSupport(imageFormat, VK_IMAGE_TILING_LINEAR, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
			// Attempt to remedy the situation by converting 24bit			
			// textures to 32bit ones. Even GTX 980 Ti doesn't support		
			// 24bit textures. This might seem ridiculous, but it's not.	
			// OpenGL just hides the details.										
			switch (imageFormat) {
			case VK_FORMAT_B8G8R8_UNORM:
				imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
				break;
			case VK_FORMAT_R8G8B8_UNORM:
				imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
				break;
			case VK_FORMAT_R16G16B16_UNORM:
				imageFormat = VK_FORMAT_R16G16B16A16_UNORM;
				break;
			case VK_FORMAT_R32G32B32_UINT:
				imageFormat = VK_FORMAT_R32G32B32A32_UINT;
				break;
			case VK_FORMAT_R64G64B64_UINT:
				imageFormat = VK_FORMAT_R64G64B64A64_UINT;
				break;
			case VK_FORMAT_B8G8R8_SNORM:
				imageFormat = VK_FORMAT_B8G8R8A8_SNORM;
				break;
			case VK_FORMAT_R8G8B8_SNORM:
				imageFormat = VK_FORMAT_R8G8B8A8_SNORM;
				break;
			case VK_FORMAT_R16G16B16_SNORM:
				imageFormat = VK_FORMAT_R16G16B16A16_SNORM;
				break;
			case VK_FORMAT_R32G32B32_SINT:
				imageFormat = VK_FORMAT_R32G32B32A32_SINT;
				break;
			case VK_FORMAT_R64G64B64_SINT:
				imageFormat = VK_FORMAT_R64G64B64A64_SINT;
				break;
			case VK_FORMAT_R32G32B32_SFLOAT:
				imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
				break;
			case VK_FORMAT_R64G64B64_SFLOAT:
				imageFormat = VK_FORMAT_R64G64B64A64_SFLOAT;
				break;
			default:
				throw Except::Graphics(pcLogFuncError
					<< "Unsupported texture format: " << view.mFormat->GetToken());
			}

			using_optimal_tiling = true;
			if (!CheckFormatSupport(imageFormat, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
				if (!CheckFormatSupport(imageFormat, VK_IMAGE_TILING_LINEAR, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
					throw Except::Graphics(pcLogFuncError
						<< "Unsupported texture format: " << view.mFormat->GetToken());
				}
				else using_optimal_tiling = false;
			}

			UNUSED() auto unusedReversed = false;
			meta = pcVkFormatToDataID(imageFormat, unusedReversed).GetMeta();
			pcLogFuncWarning << "Texture format automatically changed from "
				<< view.mFormat->GetToken() << " to " << meta->GetToken();
		}
		else using_optimal_tiling = false;
	}

	// Create image																		
	VRAMImage image;
	image.mView = view;
	image.mView.mFormat = meta;
	image.mDevice = mDevice;
	image.mInfo = {};
	image.mInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	switch (view.CountDimensions()) {
	case 1:
		image.mInfo.imageType = VK_IMAGE_TYPE_1D;
		break;
	case 2:
		image.mInfo.imageType = VK_IMAGE_TYPE_2D;
		break;
	case 3:
		image.mInfo.imageType = VK_IMAGE_TYPE_3D;
		break;
	default:
		throw Except::Graphics(pcLogFuncError
			<< "Wrong number of dimensions: " << view.CountDimensions());
	}

	image.mInfo.extent.width = uint32_t(view.mWidth);
	image.mInfo.extent.height = uint32_t(view.mHeight);
	image.mInfo.extent.depth = uint32_t(view.mDepth);
	image.mInfo.mipLevels = 1;
	image.mInfo.arrayLayers = uint32_t(view.mFrames);
	image.mInfo.format = imageFormat;
	image.mInfo.tiling = using_optimal_tiling? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR;
	image.mInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	image.mInfo.usage = flags;
	image.mInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image.mInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	image.mInfo.flags = 0; // Optional
	if (vkCreateImage(mDevice, &image.mInfo, nullptr, &image.mBuffer.mValue))
		return {};

	// Allocate VRAM																		
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(mDevice, image.mBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = ChooseMemory(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &image.mMemory.mValue)) {
		vkDestroyImage(mDevice, image.mBuffer, nullptr);
		throw Except::Graphics(pcLogFuncError
			<< "Couldn't allocate VRAM of " 
			<< ByteCount(static_cast<pcptr>(memRequirements.size)));
	}

	// Bind memory with the image														
	vkBindImageMemory(mDevice, image.mBuffer, image.mMemory, 0);
	return image;
}

/// Destroy an allocated image																
///	@param buffer - buffer to destroy													
void VRAM::DestroyImage(VRAMImage& buffer) const {
	if (buffer.mDevice) {
		if (buffer.mBuffer)
			vkDestroyImage(buffer.mDevice, buffer.mBuffer, nullptr);
		if (buffer.mMemory)
			vkFreeMemory(buffer.mDevice, buffer.mMemory, nullptr);
	}

	buffer.Reset();
}

/// Create image view																			
///	@param image - image to create view for											
///	@param view - pc image view															
///	@param aspectFlags - image aspect													
///	@return the image view																	
VkImageView VRAM::CreateImageView(const VkImage& image, const PixelView& view, VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	switch (view.CountDimensions()) {
	case 1:
		viewInfo.viewType = view.mFrames > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
		break;
	case 2:
		viewInfo.viewType = view.mFrames > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
		break;
	case 3:
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
		break;
	default:
		throw Except::Graphics(pcLogFuncError << "Wrong number of dimensions");
	}
	viewInfo.format = pcTypeToVkFormat(view.mFormat, view.mReverseFormat);
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = uint32_t(view.mFrames);

	VkImageView imageView = {};
	if (vkCreateImageView(mDevice, &viewInfo, nullptr, &imageView))
		return 0;

	return imageView;
}

/// Create image view																			
///	@param image - image to create view for											
///	@param view - pc image view															
///	@return the image view																	
VkImageView VRAM::CreateImageView(const VkImage& image, const PixelView& view) {
	const VkFormat fmt = pcTypeToVkFormat(view.mFormat);
	const bool depthusage =	fmt == VK_FORMAT_D32_SFLOAT || fmt == VK_FORMAT_D32_SFLOAT_S8_UINT
		|| fmt == VK_FORMAT_D16_UNORM	|| fmt == VK_FORMAT_D16_UNORM_S8_UINT
		|| fmt == VK_FORMAT_D24_UNORM_S8_UINT;
	return CreateImageView(image, view, depthusage 
		? VK_IMAGE_ASPECT_DEPTH_BIT 
		: VK_IMAGE_ASPECT_COLOR_BIT
	);
}

/// Pick memory pool in VRAM																	
///	@param type - type of memory to seek												
///	@param properties - memory usage to choose										
///	@return heap index for allocation													
uint32_t VRAM::ChooseMemory(uint32_t type, VkMemoryPropertyFlags properties) const {
	// Pick adequate memory																
	for (uint32_t i = 0; i < mVRAM.memoryTypeCount; i++) {
		if (type & (1 << i) && (mVRAM.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	// Error occured																		
	throw Except::Graphics(pcLogFuncError << "Failed to choose suitable memory type");
}

/// Create a buffer																				
///	@param meta - buffer data type														
///	@param size - bytes to allocate														
///	@param usage - type of buffer to allocate											
///	@param properties - type of memory for buffer									
///	@return a buffer instance																
VRAMBuffer VRAM::CreateBuffer(DMeta meta, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const {
	// Create buffer																		
	VRAMBuffer result;
	result.mDevice = mDevice;
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.flags = 0;
	if (vkCreateBuffer(mDevice, &bufferInfo, nullptr, &result.mBuffer.mValue))
		throw Except::Graphics(pcLogFuncError << "Can't create VRAM buffer");

	// Get memory requirements															
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(mDevice, result.mBuffer, &memRequirements);

	// Allocate VRAM																		
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = uint32_t(ChooseMemory(memRequirements.memoryTypeBits, properties));
	if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &result.mMemory.mValue)) {
		vkDestroyBuffer(mDevice, result.mBuffer, nullptr);
		throw Except::Graphics(pcLogFuncError << "Can't create VRAM buffer");
	}

	// Associate memory with the buffer												
	vkBindBufferMemory(mDevice, result.mBuffer, result.mMemory, 0);
	result.mMeta = meta;
	return result;
}

/// Transfer an image in VRAM from one layout to another								
///	@param img - VRAM image																	
///	@param from - starting layout															
///	@param to - ending layout																
void VRAM::ImageTransfer(VRAMImage& img, VkImageLayout from, VkImageLayout to) {
	ImageTransfer(img.mBuffer, from, to);
}

/// Transfer an image in VRAM from one layout to another								
///	@param img - VRAM image																	
///	@param from - starting layout															
///	@param to - ending layout																
void VRAM::ImageTransfer(VkImage& img, VkImageLayout from, VkImageLayout to) {
	// Copy data to VRAM																	
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(mTransferBuffer, &beginInfo);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = from;
	barrier.newLayout = to;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = img;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	// Pick optimal and non-conflicting transition								
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (
		from == VK_IMAGE_LAYOUT_UNDEFINED && 
		to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (
		from == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && 
		to == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (
		from == VK_IMAGE_LAYOUT_UNDEFINED && 
		to == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if (
		from == VK_IMAGE_LAYOUT_UNDEFINED && 
		to == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR &&
		to == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (from == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
		to == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else {
		throw Except::Graphics(pcLogFuncError << "Unsupported layout transition");
	}
	
	// Create the barrier																
	vkCmdPipelineBarrier(mTransferBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	vkEndCommandBuffer(mTransferBuffer);

	// Submit																				
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mTransferBuffer;
	vkQueueSubmit(mTransferer, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(mTransferer);
}

/// Upload a memory block to VRAM															
///	@param memory - the memory block to upload										
///	@param usage - the intended use for the memory									
///	@return a VRAM buffer wrapper															
VRAMBuffer VRAM::Upload(const Block& memory, VkBufferUsageFlags usage) {
	const auto bytesize = memory.GetCount() * memory.GetStride();
	if (bytesize == 0) {
		pcLogFuncError << "Can't upload data of size zero.";
		return {};
	}

	// Create staging buffer															
	auto stager = CreateBuffer(
		memory.GetMeta(), bytesize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);
	if (!stager.IsValid()) {
		pcLogFuncError << "Error creating VRAM vertex buffer for staging.";
		return {};
	}

	// Copy data to staging buffer													
	if (!stager.Upload(0, bytesize, memory.GetRaw())) {
		DestroyBuffer(stager);
		pcLogFuncError << "Error uploading VRAM vertex buffer.";
		return {};
	}

	auto final = CreateBuffer(
		memory.GetMeta(), bytesize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);
	if (!final.IsValid()) {
		DestroyBuffer(stager);
		pcLogFuncError << "Error creating VRAM vertex buffer.";
		return {};
	}

	// Copy data to VRAM																	
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = bytesize;

	// Copy bytes																			
	vkBeginCommandBuffer(mTransferBuffer, &beginInfo);
	vkCmdCopyBuffer(mTransferBuffer, stager.mBuffer, final.mBuffer, 1, &copyRegion);
	vkEndCommandBuffer(mTransferBuffer);

	// Submit																				
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mTransferBuffer;
	vkQueueSubmit(mTransferer, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(mTransferer);

	// Don't forget to clean up the staging buffer								
	DestroyBuffer(stager);
	return final;
}