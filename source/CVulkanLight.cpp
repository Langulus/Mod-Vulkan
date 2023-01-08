#include "CVulkanRenderer.hpp"

/// Create light																					
///	@param producer - the producer of the light										
CVulkanLight::CVulkanLight(CVulkanLayer* producer)
	: ALight{ MetaData::Of<CVulkanLight>() }
	, TProducedFrom{ producer } {
	ClassValidate();
}

/// Draw the light when using deferred rendering										
void CVulkanLight::Draw() {
	TODO();
}
