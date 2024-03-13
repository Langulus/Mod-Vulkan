///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "Common.hpp"
#include <Math/Range.hpp>
#include <Math/Angle.hpp>
#include <Math/Scale.hpp>
 
using LevelRange = TRange<Level>;


///                                                                           
///   Camera unit                                                             
///                                                                           
struct VulkanCamera final : A::Graphics, ProducedFrom<VulkanLayer> {
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::Graphics);

protected:
   friend struct VulkanLayer;

   // Whether or not a perspective projection is used                   
   bool mPerspective {true};
   // The projection matrix                                             
   Mat4 mProjection;
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

   TAny<const A::Instance*> mInstances;
   Mat4 mProjectionInverted;
   VkViewport mVulkanViewport {0, 0, 640, 480, 0, 1};
   VkRect2D mVulkanScissor {{0, 0}, {640, 480}};
   Scale2u32 mResolution {640, 480};

public:
   VulkanCamera(VulkanLayer*, const Neat&);

   void Refresh();
   void Compile();
   NOD() Mat4 GetViewTransform(const LOD&) const;
   NOD() Mat4 GetViewTransform(const Level& = {}) const;
};
