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
///   GLSL VULKAN SHADER                                                      
///                                                                           
/// Handles shader compilation using shaderc                                  
///                                                                           
class CVulkanShader {
   LANGULUS(PRODUCER) CVulkanRenderer;

private:
   std::vector<VertexBinding> mBindings;
   std::vector<VertexAttribute> mAttributes;

   // Shader code                                                       
   GLSL mCode;

   // Shader stage                                                      
   Shader mStageDescription = {};
   ShaderStage::Enum mStage = ShaderStage::Pixel;

   // Uniform/input bindings in piception format                        
   TAny<TAny<Trait>> mInputs;

   // True if shader is compiled                                        
   bool mCompiled = false;

   // True if shader is auto-completed                                  
   bool mCompleted = false;

public:
   CVulkanShader(CVulkanRenderer*);
   CVulkanShader(CVulkanShader&&) noexcept = default;
   CVulkanShader& operator = (CVulkanShader&&) noexcept = default;
   ~CVulkanShader();

   bool operator == (const ME&) const;

   NOD() Hash GetHash() const override;

   NOD() inline bool IsCompiled() const noexcept { return mCompiled; }
   NOD() inline auto& GetShader() const noexcept { return mStageDescription; }
   NOD() inline auto& GetBindings() const noexcept { return mBindings; }
   NOD() inline auto& GetAttributes() const noexcept { return mAttributes; }
   NOD() inline auto& GetCode() const noexcept { return mCode; }
   NOD() inline RRate GetRate() const noexcept { return mStage + RRate::StagesBegin; }

   bool InitializeFromMaterial(ShaderStage::Enum, const AMaterial*);
   void Uninitialize();
   bool Compile();

   void AddInput(const Trait&);
   VkShaderStageFlagBits GetStageFlagBit() const noexcept;
};
