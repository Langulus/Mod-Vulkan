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
class CVulkanCamera : public ACamera, public TProducedFrom<CVulkanLayer> {
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
