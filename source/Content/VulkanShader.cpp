///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include <shaderc/shaderc.hpp>
#include "../VulkanRenderer.hpp"

/// Shader passive construction                                               
///   @param producer - the producer of the shader                            
VulkanShader::VulkanShader(CVulkanRenderer* producer)
   : AUnitGraphics {MetaData::Of<VulkanShader>()}
   , TProducedFrom {producer} { }

/// Component destruction                                                     
VulkanShader::~VulkanShader() {
   Uninitialize();
}

/// Compare two shaders for equality                                          
///   @param other - the shader to compare against                            
///   @return true if shaders are functionally the same                       
bool VulkanShader::operator == (const ME& other) const {
   return CompareHash(other) && mCode == other.mCode && mStage == other.mStage;
}

/// From Hashable                                                             
Hash VulkanShader::GetHash() const {
   if (mHashed)
      return mHash;
   return Hashed::SetHash(mCode.GetHash() | pcHash(mStage));
}

/// Initialize the shader via a stage and code. This does not immediately     
/// compile the shader, so you can add uniforms and inputs                    
///   @param stage - the kind of shader                                       
///   @param material - the material generator                                
///   @return true if shader was generated successfully                       
bool VulkanShader::InitializeFromMaterial(ShaderStage::Enum stage, const AMaterial* material) {
   if (!mCode.IsEmpty()) {
      pcLogSelfWarning << "Overwriting shader stage";
      Uninitialize();
   }

   if (!mInputs.IsAllocated())
      mInputs.Allocate(RefreshRate::StagesCount, true);

   // Copy code from the correct stage                                  
   mStage = stage;
   mCode += material->GetDataAs<Traits::Code, GLSL>(pcptr(stage));

   // Access input mappings                                             
   const auto uniforms = material->GetData<Traits::Trait>();
   if (!uniforms || uniforms->IsEmpty())
      throw Except::Graphics(pcLogSelfError << "No uniforms/inputs provided by generator");

   // Add relevant inputs                                               
   const auto index = RRate(RRate::StagesBegin + stage).GetInputIndex();
   auto& inputs = uniforms->As<TAny<Trait>>(index);
   for (auto& attribute : inputs)
      AddInput(attribute);

   ResetHash();
   ClassValidate();
   return true;
}

/// Reset the shader                                                          
void VulkanShader::Uninitialize() {
   if (!mProducer)
      return;

   if (mStageDescription.module) {
      vkDestroyShaderModule(mProducer->GetDevice(), mStageDescription.module, nullptr);
      mStageDescription = {};
   }

   mCompiled = false;
   mCode.Reset();
   ResetHash();
   ClassInvalidate();
}

/// Compile the shader code                                                   
///   @return true on success                                                 
bool VulkanShader::Compile() {
   if (mCompiled)
      return true;

   auto device = mProducer->GetDevice().Get();
   PCTimer loadTime;
   loadTime.Start();

   // Compiling with optimizing                                         
   shaderc::Compiler compiler;
   shaderc::CompileOptions options;

   static constexpr decltype(shaderc_glsl_infer_from_source) 
   ShadercStageMap[ShaderStage::Counter] = {
      shaderc_glsl_vertex_shader,
      shaderc_glsl_geometry_shader,
      shaderc_glsl_tess_control_shader,
      shaderc_glsl_tess_evaluation_shader,
      shaderc_glsl_fragment_shader,
      shaderc_glsl_compute_shader
   };

   const auto kind = ShadercStageMap[mStage];

   // Like -DMY_DEFINE=1                                                
   //options.AddMacroDefinition("MY_DEFINE", "1");                      

   // Optimize                                                          
   const bool optimize = false; // TODO
   if (optimize)
      options.SetOptimizationLevel(shaderc_optimization_level_size);

   // Compile to binary                                                 
   auto assembly = compiler.CompileGlslToSpv(mCode.Terminate().GetRaw(), kind, "whatever", options);
   if (assembly.GetCompilationStatus() != shaderc_compilation_status_success) {
      pcLogSelfError << "Shader Compilation Error: " << assembly.GetErrorMessage().c_str();
      pcLogSelfError << "For shader:";
      pcLogSelfError << mCode.Pretty();
      Uninitialize();
      return false;
   }

   // Create the vulkan shader module                                   
   VkShaderModuleCreateInfo createInfo = {};
   createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   createInfo.codeSize = pcP2N(assembly.cend()) - pcP2N(assembly.cbegin());
   createInfo.pCode = assembly.cbegin();
   VkShaderModule shaderModule;
   if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
      pcLogSelfError << "Error creating shader module";
      Uninitialize();
      return false;
   }

   // Create the shader stage                                           
   mStageDescription = {};
   mStageDescription.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   mStageDescription.stage = pcShaderToVkStage(mStage);
   mStageDescription.module = shaderModule;
   mStageDescription.pName = "main";
   mCompiled = true;
   
   auto accumulated_time = loadTime.Stop();
   pcLogSelfVerbose << ccGreen << "Compiled shader in " << accumulated_time;
   pcLogSelfVerbose << ccGreen << mCode.Pretty();
   return true;
}

/// Bind vertex input                                                         
///   @param input - input type to bind                                       
void VulkanShader::AddInput(const Trait& input) {
   // On non-vertex shaders - only populate the inputs without          
   // generating code                                                   
   switch (mStage) {
   case ShaderStage::Pixel:
      mInputs[RRate(RRate::PerPixel).GetStageIndex()] << input;
      return;
   case ShaderStage::Geometry:
      mInputs[RRate(RRate::PerPrimitive).GetStageIndex()] << input;
      return;
   case ShaderStage::Vertex:
      mInputs[RRate(RRate::PerVertex).GetStageIndex()] << input;
      // Note this is the only case where we continue   after the switch
      break;
   default:
      return;
   }

   const auto uidx = static_cast<uint32_t>(mBindings.size());
   VkFormat vkt = VK_FORMAT_UNDEFINED;
   LinkedBase inputBase;
   // Single precision                                                  
   if (input.GetMeta()->GetBase<vec4f>(0, inputBase) && inputBase.mStaticBase.mMapping)
      vkt = pcTypeToVkFormat(inputBase.mBase);
   else if (input.GetMeta()->GetBase<vec3f>(0, inputBase) && inputBase.mStaticBase.mMapping)
      vkt = pcTypeToVkFormat(inputBase.mBase);
   else if (input.GetMeta()->GetBase<vec2f>(0, inputBase) && inputBase.mStaticBase.mMapping)
      vkt = pcTypeToVkFormat(inputBase.mBase);
   else if (input.GetMeta()->GetBase<pcr32>(0, inputBase) && inputBase.mStaticBase.mMapping)
      vkt = pcTypeToVkFormat(inputBase.mBase);
   // Double precision                                                  
   else if (input.GetMeta()->GetBase<vec4d>(0, inputBase) && inputBase.mStaticBase.mMapping)
      vkt = pcTypeToVkFormat(inputBase.mBase);
   else if (input.GetMeta()->GetBase<vec3d>(0, inputBase) && inputBase.mStaticBase.mMapping)
      vkt = pcTypeToVkFormat(inputBase.mBase);
   else if (input.GetMeta()->GetBase<vec2d>(0, inputBase) && inputBase.mStaticBase.mMapping)
      vkt = pcTypeToVkFormat(inputBase.mBase);
   else if (input.GetMeta()->GetBase<pcr64>(0, inputBase) && inputBase.mStaticBase.mMapping)
      vkt = pcTypeToVkFormat(inputBase.mBase);
   
   if (vkt == VK_FORMAT_UNDEFINED) {
      throw Except::Graphics(pcLogSelfError
         << "Unsupported base for shader attribute " << input.GetTraitMeta()->GetToken()
         << ": " << inputBase.mStaticBase.mCount
         << " elements of " << inputBase.mBase->GetToken()
         << " (decayed from " << input.GetMeta()->GetToken() << ")");
   }

   // Then create the input bindings and attributes                     
   VertexBinding bindingDescription = {};
   bindingDescription.binding = uidx;
   bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
   bindingDescription.stride = static_cast<uint32_t>(inputBase.mBase->GetStride());
   mBindings.push_back(bindingDescription);

   VertexAttribute attributeDescription = {};
   attributeDescription.binding = uidx;
   attributeDescription.location = uidx;
   attributeDescription.offset = 0;
   attributeDescription.format = vkt;
   mAttributes.push_back(attributeDescription);
}

/// Get a VkShaderStageFlagBits corresponding the the this shader's stage     
///   @return the flag                                                        
VkShaderStageFlagBits VulkanShader::GetStageFlagBit() const noexcept {
   static constexpr VkShaderStageFlagBits StageMap[ShaderStage::Counter] = {
      VK_SHADER_STAGE_VERTEX_BIT,
      VK_SHADER_STAGE_GEOMETRY_BIT,
      VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
      VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      VK_SHADER_STAGE_COMPUTE_BIT
   };

   return StageMap[mStage];
}

// in order to mix standard rasterizer with ray marcher we must first linearize depth
// uniform float near;
//uniform float far;
//float LinearizeDepth(float depth) {
//   float z = depth * 2.0 - 1.0; // back to NDC
//   return (2.0 * near * far) / (far + near - z * (far - near));
//}
// ... float depth = LinearizeDepth(gl_FragCoord.z);
// https://computergraphics.stackexchange.com/questions/7674/how-to-align-ray-marching-on-top-of-traditional-3d-rasterization