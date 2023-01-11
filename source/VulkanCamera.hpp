///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "Common.hpp"


///                                                                           
///   THE VULKAN CAMERA COMPONENT                                             
///                                                                           
/// Represents a camera's view of the world                                   
///                                                                           
class CVulkanCamera : public TProducedFrom<CVulkanLayer> {

   // Whether or not a perspective projection is used						
   bool mPerspective = true;
   // The projection matrix														
   mat4 mProjection;
   // Clipping range in all directions, including depth					
   range4 mViewport = {{0, 0, 0.1, 0}, {720, 480, 1000, 0}};
   // Horizontal field of view, in radians									
   real mFOV = pcD2R(90.0f);
   // Aspect ratio (width / height)												
   real mAspectRatio = 720.0f / 480.0f;
   // Human retina is 32 milimeters (10^-3) across, which means that	
   // we can observe stuff slightly smaller than human octave			
   LevelRange mObservableRange {Level::Default, Level::Max};
   // Eye separation. Stereo if more/less than zero						
   real mEyeSeparation = 0;

   TAny<const AInstance*> mInstances;
   mat4 mProjectionInverted;
   VkViewport mVulkanViewport {0, 0, 640, 480, 0, 1};
   VkRect2D mVulkanScissor {{0, 0}, {640, 480}};
   vec2 mResolution {640, 480};

public:
   REFLECT(CVulkanCamera);
   CVulkanCamera(CVulkanLayer*);

   void Refresh() override;
   void Compile();

   PC_GET(Instances);
   PC_GET(ProjectionInverted);
   PC_GET(VulkanViewport);
   PC_GET(VulkanScissor);
   PC_GET(Resolution);

   NOD() mat4 GetViewTransform(Level) const;
};
