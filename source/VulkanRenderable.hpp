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
struct VulkanRenderable final : Things::Renderable, ProducedFrom<VulkanLayer> {
   using CTTI_Abstract  = No;
   using CTTI_Producer  = VulkanLayer;
   using CTTI_Bases     = Things::Renderable;

protected:
   friend struct VulkanLayer;

   // Precompiled instances and levels, updated on Refresh()            
   TMany<Things::Instance const*> mInstances;
   TRange<Level> mLevelRange;
   Ref<Things::Material> mMaterialContent;
   Ref<Things::Mesh> mGeometryContent;
   Ref<Things::Image> mTextureContent;
   mutable Ref<VulkanPipeline> mPredefinedPipeline;

   // Precompiled content, updated on Refresh()                         
   mutable struct {
      Ref<VulkanGeometry> mGeometry;
      Ref<VulkanTexture> mTexture;
      Ref<VulkanPipeline> mPipeline;
   } mLOD[LOD::IndexCount];

public:
   VulkanRenderable(VulkanLayer*, Neat const&);
   ~VulkanRenderable();

   auto GetRenderer() const noexcept -> VulkanRenderer*;
   auto GetGeometry(LOD const&) const -> VulkanGeometry*;
   auto GetTexture(LOD const&) const -> VulkanTexture*;
   auto GetOrCreatePipeline(LOD const&, VulkanLayer const*) const -> VulkanPipeline*;

   void Refresh();
   void Detach();
};
