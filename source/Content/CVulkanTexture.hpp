#pragma once
#include "PCVRAMBuffer.hpp"

///																									
///	VULKAN VRAM TEXTURE																		
///																									
/// Handles hardware pixel/voxel buffers													
///																									
class CVulkanTexture 
	: public IContentVRAM
	, public TProducedFrom<CVulkanRenderer> {
	// Original content texture view													
	PixelView mView;
	// Image																					
	VRAMImage mImage;
	// Image view																			
	Own<VkImageView> mImageView;
	// Image sampler																		
	Own<VkSampler> mSampler;

public:
	REFLECT(CVulkanTexture);
	CVulkanTexture(CVulkanRenderer*);
	CVulkanTexture(CVulkanTexture&&) noexcept = default;
	CVulkanTexture& operator = (CVulkanTexture&&) noexcept = default;
	~CVulkanTexture();

	void Initialize();
	void Uninitialize();

	PC_VERB(Create);

	PC_GET(ImageView);
	PC_GET(Sampler);

	using IContentVRAM::operator ==;
	using IContentVRAM::operator !=;
};
