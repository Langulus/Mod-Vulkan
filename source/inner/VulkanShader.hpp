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
   // Shader stage                                                      
   Shader mStageDescription {};
   ShaderStage::Enum mStage {ShaderStage::Pixel};
   // Uniform/input bindings for each shader stage                      
   TAny<Trait> mInputs[RefreshRate::StagesCount];
   // True if shader is compiled                                        
   bool mCompiled {};
   // True if shader is auto-completed                                  
   bool mCompleted {};

public:
   VulkanShader(VulkanRenderer*, const Any&);
   ~VulkanShader();

   NOD() bool IsCompiled() const noexcept;
   NOD() auto& GetShader() const noexcept;
   NOD() auto& GetBindings() const noexcept;
   NOD() auto& GetAttributes() const noexcept;
   NOD() auto& GetCode() const noexcept;
   NOD() Rate GetRate() const noexcept;

   void Compile();
   void AddInput(const Trait&);
   VkShaderStageFlagBits GetStageFlagBit() const noexcept;
};
