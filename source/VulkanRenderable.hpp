///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright (c) 2020 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
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
   TMany<const A::Instance*> mInstances;
   TRange<Level> mLevelRange;
   Ref<A::Material> mMaterialContent;
   Ref<A::Mesh> mGeometryContent;
   Ref<A::Image> mTextureContent;
   mutable Ref<VulkanPipeline> mPredefinedPipeline;

   // Precompiled content, updated on Refresh()                         
   mutable struct {
      Ref<VulkanGeometry> mGeometry;
      Ref<VulkanTexture> mTexture;
      Ref<VulkanPipeline> mPipeline;
   } mLOD[LOD::IndexCount];

public:
   VulkanRenderable(VulkanLayer*, Describe);
   ~VulkanRenderable();

   NOD() VulkanRenderer* GetRenderer() const noexcept;
   NOD() VulkanGeometry* GetGeometry(const LOD&) const;
   NOD() VulkanTexture*  GetTexture(const LOD&) const;
   NOD() VulkanPipeline* GetOrCreatePipeline(const LOD&, const VulkanLayer*) const;

   void Refresh();
   void Detach();
};
