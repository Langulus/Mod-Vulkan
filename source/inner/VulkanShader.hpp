///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "VulkanBuffer.hpp"


///                                                                           
///   Vulkan shader                                                           
///                                                                           
struct VulkanShader : A::Graphics, ProducedFrom<VulkanRenderer> {
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::Graphics);

private:
   std::vector<VertexBinding> mBindings;
   std::vector<VertexAttribute> mAttributes;

   // Shader code                                                       
   Text mCode;
   // Shader stage description                                          
   mutable Shader mStageDescription {};
   // Shader stage description                                          
   mutable bool mCompiled {};
   // Shader stage                                                      
   ShaderStage::Enum mStage {ShaderStage::Pixel};
   // Uniform/input bindings for each shader stage                      
   TAny<Trait> mInputs[RefreshRate::StagesCount];

public:
   VulkanShader(VulkanRenderer*, const Neat&);
   ~VulkanShader();

   NOD() RefreshRate GetRate() const noexcept;
   NOD() VertexInput CreateVertexInputState() const noexcept;

   const Shader& Compile() const;
   void AddInput(const Trait&);
   VkShaderStageFlagBits GetStageFlagBit() const noexcept;
   ShaderStage::Enum GetStage() const noexcept;
   const Text& GetCode() const noexcept;
};
