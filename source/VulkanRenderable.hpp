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
///   Vulkan renderable element                                               
///                                                                           
/// Gives things the ability to be drawn to screen. The unit gathers relevant 
/// graphical resources from the context, and generates a graphical pipeline  
/// capable of visualizing them                                               
///                                                                           
struct VulkanRenderable final : A::Renderable, ProducedFrom<VulkanLayer> {
   LANGULUS(ABSTRACT) false;
   LANGULUS(PRODUCER) VulkanLayer;
   LANGULUS_BASES(A::Renderable);

protected:
   friend struct VulkanLayer;

   // Precompiled instances and levels, updated on Refresh()            
   TAny<const A::Instance*> mInstances;
   TRange<Level> mLevelRange;
   Ptr<A::Material> mMaterialContent;
   Ptr<A::Mesh> mGeometryContent;
   Ptr<A::Image> mTextureContent;
   mutable Ptr<VulkanPipeline> mPredefinedPipeline;

   // Precompiled content, updated on Refresh()                         
   mutable struct {
      Ptr<VulkanGeometry> mGeometry;
      Ptr<VulkanTexture> mTexture;
      Ptr<VulkanPipeline> mPipeline;
   } mLOD[LOD::IndexCount];

   void ResetRenderable();

public:
   VulkanRenderable(VulkanLayer*, const Neat&);

   NOD() VulkanRenderer* GetRenderer() const noexcept;
   NOD() VulkanGeometry* GetGeometry(const LOD&) const;
   NOD() VulkanTexture*  GetTexture(const LOD&) const;
   NOD() VulkanPipeline* GetOrCreatePipeline(const LOD&, const VulkanLayer*) const;

   void Refresh();
};
