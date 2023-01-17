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
///   Camera unit                                                             
///                                                                           
class VulkanCamera : public Unit {
   LANGULUS(PRODUCER) VulkanLayer;
protected:
   friend class VulkanLayer;

   // Whether or not a perspective projection is used                   
   bool mPerspective {true};
   // The projection matrix                                             
   Matrix4 mProjection;
   // Clipping range in all directions, including depth                 
   Range4 mViewport {{0, 0, 0.1, 0}, {720, 480, 1000, 0}};
   // Horizontal field of view, in radians                              
   Radians mFOV {90_deg};
   // Aspect ratio (width / height)                                     
   Real mAspectRatio {Real {720} / Real {480}};
   // Human retina is 32 milimeters (10^-3) across, which means that    
   // we can observe stuff slightly smaller than human octave           
   LevelRange mObservableRange {Level::Default, Level::Max};
   // Eye separation. Stereo if more/less than zero                     
   Real mEyeSeparation {};

   TAny<const Unit*> mInstances;
   Matrix4 mProjectionInverted;
   VkViewport mVulkanViewport {0, 0, 640, 480, 0, 1};
   VkRect2D mVulkanScissor {{0, 0}, {640, 480}};
   Vec2u32 mResolution {640, 480};

public:
   VulkanCamera(VulkanLayer*);

   void Refresh() override;
   void Compile();
   NOD() Matrix4 GetViewTransform(Level) const;
};
