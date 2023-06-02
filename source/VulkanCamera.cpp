///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Vulkan.hpp"

/// Descriptor constructor                                                    
///   @param producer - the camera producer                                   
///   @param descriptor - the camera descriptor                               
VulkanCamera::VulkanCamera(VulkanLayer* producer, const Descriptor& descriptor)
   : Graphics {MetaOf<VulkanCamera>(), descriptor} 
   , ProducedFrom {producer, descriptor} {
   TODO();
}

/// Compile the camera                                                        
void VulkanCamera::Compile() {
   mResolution = mProducer->mProducer->mWindow->GetSize();

   if (mResolution[0] <= 1)
      mResolution[0] = 1;
   if (mResolution[1] <= 1)
      mResolution[1] = 1;

   mAspectRatio = static_cast<Real>(mResolution[0])
                / static_cast<Real>(mResolution[1]);
   mViewport.mMax.xy() = mResolution;

   if (mPerspective) {
      // Perspective is enabled, so use FOV, aspect ratio, and viewport 
      // Also, Vulkan uses a flipped coordinate system                  
      // The final projection coordinates should look like that:        
      //                                                                
      //                  +Aspect*Y                                     
      //                      ^    ^ looking at +Z (towards the screen) 
      //                      |   /                                     
      //               -X+Y   |  /      +X+Y                            
      //                      | /                                       
      //                      |/                                        
      //   -1X <--------------+--------------> +1X                      
      //                screen center                                   
      //                      |                                         
      //               -X-Y   |         +X-Y                            
      //                      v                                         
      //                  -Aspect*Y                                     
      //                                                                
      constexpr auto vulkanAdapter = Mat4::Scalar(
         Vec3 {Real {1}, Real {-1}, Real {-1}}
      );
      mProjection = vulkanAdapter * Mat4::PerspectiveFOV(
         mFOV, mAspectRatio, mViewport.mMin[2], mViewport.mMax[2]
      );
   }
   else {
      // Orthographic is enabled, so use only viewport                  
      // Origin shall be at the top-left, x/y increasing bottom-right   
      // The final projection coordinates should look like that:        
      //                                                                
      //   top-left screen corner                                       
      //     +--------------> +X                                        
      //     |                      looking at +Z (towards the screen)  
      //     |         +X+Y                                             
      //     v                                                          
      //   +Aspect*Y                                                    
      //                                                                
      mProjection = Mat4::Orthographic(
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
void VulkanCamera::Refresh() {
   mInstances = GatherUnits<A::Instance, Seek::Here>();
}

/// Get view transformation for a given LOD state                             
///   @param lod - the level-of-detail state                                  
///   @return the view transformation for the camera                          
Mat4 VulkanCamera::GetViewTransform(const LOD& lod) const {
   if (mInstances.IsEmpty())
      return {};

   return mInstances[0]->GetViewTransform(lod);
}

/// Get view transformation for a given level                                 
///   @param level - the level                                                
///   @return the view transformation for the camera                          
Mat4 VulkanCamera::GetViewTransform(const Level& level) const {
   if (mInstances.IsEmpty())
      return {};

   return mInstances[0]->GetViewTransform(level);
}
