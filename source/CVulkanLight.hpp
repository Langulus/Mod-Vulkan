#pragma once
#include "IncludeVulkan.hpp"


///																									
///	VULKAN LIGHT COMPONENT																	
///																									
class CVulkanLight : public ALight, public TProducedFrom<CVulkanLayer> {
	REFLECT(CVulkanLight);
	CVulkanLight(CVulkanLayer*);

	void Draw();
};
