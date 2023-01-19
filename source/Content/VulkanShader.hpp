///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "VulkanBuffer.hpp"
#include "../GLSL.hpp"


///                                                                           
///   Vulkan shader                                                           
///                                                                           
class VulkanShader {
   LANGULUS(PRODUCER) VulkanRenderer;

private:
   std::vector<VertexBinding> mBindings;
   std::vector<VertexAttribute> mAttributes;

   // Shader code                                                       
   GLSL mCode;

   // Shader stage                                                      
   Shader mStageDescription {};
   ShaderStage::Enum mStage {ShaderStage::Pixel};

   // Uniform/input bindings in piception format                        
   TAny<TAny<Trait>> mInputs;

   // True if shader is compiled                                        
   bool mCompiled {};

   // True if shader is auto-completed                                  
   bool mCompleted {};

public:
   VulkanShader(VulkanRenderer*);
   ~VulkanShader();

   bool operator == (const VulkanShader&) const;

   NOD() Hash GetHash() const;
   NOD() bool IsCompiled() const noexcept;
   NOD() auto& GetShader() const noexcept;
   NOD() auto& GetBindings() const noexcept;
   NOD() auto& GetAttributes() const noexcept;
   NOD() auto& GetCode() const noexcept;
   NOD() Rate GetRate() const noexcept;

   bool InitializeFromMaterial(ShaderStage::Enum, const Unit*);
   void Uninitialize();
   bool Compile();

   void AddInput(const Trait&);
   VkShaderStageFlagBits GetStageFlagBit() const noexcept;
};
