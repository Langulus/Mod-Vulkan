///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright (c) 2020 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "VulkanBuffer.hpp"


///                                                                           
///   Vulkan shader                                                           
struct VulkanShader : Things::Graphics, ProducedFrom<VulkanRenderer> {
   using CTTI_Abstract  = No;
   using CTTI_Bases     = Things::Graphics;

private:
   std::vector<VertexBinding> mBindings;
   std::vector<VertexAttribute> mAttributes;

   // Shader code                                                       
   Text mCode;
   // Shader stage description                                          
   mutable Shader mStageDescription {};
   // Shader stage description                                          
   mutable bool mCompiled = false;
   // Shader stage                                                      
   ShaderStage::Enum mStage = ShaderStage::Pixel;
   // Uniform/input bindings for each shader stage                      
   TMany<Trait> mInputs[RRate::StagesCount];

public:
   VulkanShader(VulkanRenderer*, Neat const&);
   ~VulkanShader();

   auto GetRate() const noexcept -> RRate;
   auto CreateVertexInputState() const noexcept -> VertexInput;

   auto Compile() const -> const Shader&;
   void AddInput(const Trait&);
   auto GetStageFlagBit() const noexcept -> VkShaderStageFlagBits;
   auto GetStage() const noexcept -> ShaderStage::Enum;
   auto GetCode() const noexcept -> Text const&;
};
