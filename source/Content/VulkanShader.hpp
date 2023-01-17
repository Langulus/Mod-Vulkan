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
   VulkanShader(VulkanShader&&) noexcept = default;
   VulkanShader& operator = (VulkanShader&&) noexcept = default;
   ~VulkanShader();

   bool operator == (const VulkanShader&) const;

   NOD() Hash GetHash() const;

   NOD() bool IsCompiled() const noexcept { return mCompiled; }
   NOD() auto& GetShader() const noexcept { return mStageDescription; }
   NOD() auto& GetBindings() const noexcept { return mBindings; }
   NOD() auto& GetAttributes() const noexcept { return mAttributes; }
   NOD() auto& GetCode() const noexcept { return mCode; }
   NOD() RRate GetRate() const noexcept { return mStage + RRate::StagesBegin; }

   bool InitializeFromMaterial(ShaderStage::Enum, const Unit*);
   void Uninitialize();
   bool Compile();

   void AddInput(const Trait&);
   VkShaderStageFlagBits GetStageFlagBit() const noexcept;
};
