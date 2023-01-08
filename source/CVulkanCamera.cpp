#include "CVulkanRenderer.hpp"

/// Create camera																					
///	@param producer - the camera producer												
CVulkanCamera::CVulkanCamera(CVulkanLayer* producer)
	: ACamera {MetaData::Of<CVulkanCamera>()}
	, TProducedFrom {producer} {
	ClassValidate();
}

/// Compile the camera																			
void CVulkanCamera::Compile() {
	mResolution = mProducer->GetWindow()->GetSize();
	if (mResolution[0] <= 1)
		mResolution[0] = 1;
	if (mResolution[1] <= 1)
		mResolution[1] = 1;

	mAspectRatio = mResolution[0] / mResolution[1];
	mViewport.mMax.xy() = mResolution;

	if (GetPerspective()) {
		// Perspective is enabled, so use FOV, aspect ratio, and viewport	
		// Also, Vulkan uses a flipped coordinate system						
		// The final projection coordinates should look like that:			
		//						  																
		//						  +Aspect*Y													
		//								^	  ^ looking at +Z (towards the screen)	
		//								|	 /													
		//					-X+Y		|	/		+X+Y										
		//								| /													
		//								|/														
		//	  -1X	<--------------+--------------> +1X								
		//						screen center												
		//								|														
		//					-X-Y		|			+X-Y										
		//								v														
		//						  -Aspect*Y													
		//						  																
		static const mat4 vulkanAdapter = mat4::Scalar(vec3(1, -1, -1));
		mProjection = vulkanAdapter * mat4::PerspectiveFOV(
			mFOV, mAspectRatio, mViewport.mMin[2], mViewport.mMax[2]
		);
	}
	else {
		// Orthographic is enabled, so use only viewport						
		// Origin shall be at the top-left, x/y increasing bottom-right	
		// The final projection coordinates should look like that:			
		//						  																
		//	top-left screen corner														
		//	  +--------------> +X														
		//	  |								looking at +Z (towards the screen)	
		//	  |			+X+Y																
		//	  v																				
		//	+Aspect*Y																		
		//						  																
		mProjection = mat4::Orthographic(
			mViewport.mMax[0], mViewport.mMax[1], 
			mViewport.mMin[2], mViewport.mMax[2]
		);
	}

	mProjectionInverted = mProjection.Invert();

	const auto viewport = mViewport.Length();
	const auto offset = mViewport.mMin;
	mVulkanViewport.width = viewport[0];
	mVulkanViewport.height = viewport[1];
	mVulkanViewport.x = offset[0];
	mVulkanViewport.y = offset[1];
	mVulkanScissor.extent.height = uint32_t(viewport[1]);
	mVulkanScissor.extent.width = uint32_t(viewport[0]);
	mVulkanScissor.offset.x = int32_t(offset[0]);
	mVulkanScissor.offset.y = int32_t(offset[1]);
}

/// Recompile the camera																		
void CVulkanCamera::Refresh() {
	// Gather all instances for this camera										
	mInstances.Clear();
	for (auto owner : GetOwners()) {
		for (auto unit : owner->GetUnits()) {
			auto instance = dynamic_cast<const AInstance*>(unit);
			if (!instance)
				continue;
			mInstances << instance;
		}
	}
}

/// Get view transformation for a given level											
///	@param level - the level																
///	@return the view transformation for the camera									
mat4 CVulkanCamera::GetViewTransform(Level level) const {
	if (mInstances.IsEmpty())
		return mat4::Identity();

	return mInstances[0]->GetViewTransform(level);
}
