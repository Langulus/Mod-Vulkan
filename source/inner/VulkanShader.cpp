///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include <shaderc/shaderc.hpp>
#include "../Vulkan.hpp"

#define VERBOSE_SHADER(...) //Logger::Verbose(Self(), __VA_ARGS__)


/// Descriptor constructor                                                    
///   @param producer - the shader producer                                   
///   @param descriptor - the shader descriptor                               
VulkanShader::VulkanShader(VulkanRenderer* producer, const Any& descriptor)
   : GraphicsUnit {MetaOf<VulkanShader>(), descriptor} 
   , ProducedFrom {producer, descriptor} {
   TODO();
}

/// Component destruction                                                     
VulkanShader::~VulkanShader() {
   Uninitialize();
}

/// Compare two shaders for equality                                          
///   @param other - the shader to compare against                            
///   @return true if shaders are functionally the same                       
bool VulkanShader::operator == (const VulkanShader& other) const {
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
bool VulkanShader::InitializeFromMaterial(ShaderStage::Enum stage, const A::Material* material) {
   if (!mCode.IsEmpty()) {
      Logger::Warning(Self(), "Overwriting shader stage");
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
      LANGULUS_THROW(Graphics, "No uniforms/inputs provided by generator");

   // Add relevant inputs                                               
   const auto index = Rate(Rate::StagesBegin + stage).GetInputIndex();
   auto& inputs = uniforms->As<TAny<Trait>>(index);
   for (auto& attribute : inputs)
      AddInput(attribute);

   ResetHash();
   return true;
}

/// Reset the shader                                                          
void VulkanShader::Uninitialize() {
   if (!mProducer)
      return;

   if (mStageDescription.module) {
      vkDestroyShaderModule(mProducer->mDevice, mStageDescription.module, nullptr);
      mStageDescription = {};
   }

   mCompiled = false;
   mCode.Reset();
   ResetHash();
}

/// Compile the shader code                                                   
///   @return true on success                                                 
bool VulkanShader::Compile() {
   if (mCompiled)
      return true;

   const auto device = mProducer->mDevice;
   const auto startTime = SteadyClock::now();

   // Compiling with optimizing                                         
   shaderc::Compiler compiler;
   shaderc::CompileOptions options;

   static constexpr decltype(shaderc_glsl_infer_from_source) 
   ShadercStageMap[ShaderStage::Counter] {
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
      Logger::Error(Self(), "Shader Compilation Error: ", assembly.GetErrorMessage());
      Logger::Error(Self(), "For shader:");
      Logger::Error(Self(), mCode.Pretty());
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
      Logger::Error(Self(), "Error creating shader module");
      Uninitialize();
      return false;
   }

   // Create the shader stage                                           
   mStageDescription = {};
   mStageDescription.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   mStageDescription.stage = AsVkStage(mStage);
   mStageDescription.module = shaderModule;
   mStageDescription.pName = "main";
   mCompiled = true;
   
   VERBOSE_SHADER(Logger::Green, "Compiled shader in ", SteadyClock::now() - startTime);
   VERBOSE_SHADER(Logger::Green, mCode.Pretty());
   return true;
}

/// Bind vertex input                                                         
///   @param input - input type to bind                                       
void VulkanShader::AddInput(const Trait& input) {
   // On non-vertex shaders - only populate the inputs without          
   // generating code                                                   
   switch (mStage) {
   case ShaderStage::Pixel:
      mInputs[Rate(Rate::Pixel).GetStageIndex()] << input;
      return;
   case ShaderStage::Geometry:
      mInputs[Rate(Rate::Primitive).GetStageIndex()] << input;
      return;
   case ShaderStage::Vertex:
      mInputs[Rate(Rate::Vertex).GetStageIndex()] << input;
      // Note this is the only case where we continue   after the switch
      break;
   default:
      return;
   }

   const auto uidx = static_cast<uint32_t>(mBindings.size());
   const auto meta = input.GetType();
   RTTI::Base inputBase;
   VkFormat vkt = VK_FORMAT_UNDEFINED;

   // Single precision                                                  
   if (meta->GetBase<Vec4f>(0, inputBase) && inputBase.mBinaryCompatible)
      vkt = AsVkFormat(inputBase.mType);
   else if (meta->GetBase<Vec3f>(0, inputBase) && inputBase.mBinaryCompatible)
      vkt = AsVkFormat(inputBase.mType);
   else if (meta->GetBase<Vec2f>(0, inputBase) && inputBase.mBinaryCompatible)
      vkt = AsVkFormat(inputBase.mType);
   else if (meta->GetBase<Float>(0, inputBase) && inputBase.mBinaryCompatible)
      vkt = AsVkFormat(inputBase.mType);
   // Double precision                                                  
   else if (meta->GetBase<Vec4d>(0, inputBase) && inputBase.mBinaryCompatible)
      vkt = AsVkFormat(inputBase.mType);
   else if (meta->GetBase<Vec3d>(0, inputBase) && inputBase.mBinaryCompatible)
      vkt = AsVkFormat(inputBase.mType);
   else if (meta->GetBase<Vec2d>(0, inputBase) && inputBase.mBinaryCompatible)
      vkt = AsVkFormat(inputBase.mType);
   else if (meta->GetBase<Double>(0, inputBase) && inputBase.mBinaryCompatible)
      vkt = AsVkFormat(inputBase.mType);
   
   if (vkt == VK_FORMAT_UNDEFINED)
      LANGULUS_THROW(Graphics, "Unsupported base for shader attribute");

   // Then create the input bindings and attributes                     
   VertexBinding bindingDescription {};
   bindingDescription.binding = uidx;
   bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
   bindingDescription.stride = static_cast<uint32_t>(inputBase.mType->mSize);
   mBindings.push_back(bindingDescription);

   VertexAttribute attributeDescription {};
   attributeDescription.binding = uidx;
   attributeDescription.location = uidx;
   attributeDescription.format = vkt;
   mAttributes.push_back(attributeDescription);
}

/// Get a VkShaderStageFlagBits corresponding the the this shader's stage     
///   @return the flag                                                        
LANGULUS(ALWAYSINLINE)
VkShaderStageFlagBits VulkanShader::GetStageFlagBit() const noexcept {
   static constexpr VkShaderStageFlagBits StageMap[ShaderStage::Counter] {
      VK_SHADER_STAGE_VERTEX_BIT,
      VK_SHADER_STAGE_GEOMETRY_BIT,
      VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
      VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      VK_SHADER_STAGE_COMPUTE_BIT
   };

   return StageMap[mStage];
}

LANGULUS(ALWAYSINLINE)
bool VulkanShader::IsCompiled() const noexcept {
   return mCompiled;
}

LANGULUS(ALWAYSINLINE)
auto& VulkanShader::GetShader() const noexcept {
   return mStageDescription;
}

LANGULUS(ALWAYSINLINE)
auto& VulkanShader::GetBindings() const noexcept {
   return mBindings;
}

LANGULUS(ALWAYSINLINE)
auto& VulkanShader::GetAttributes() const noexcept {
   return mAttributes;
}

LANGULUS(ALWAYSINLINE)
auto& VulkanShader::GetCode() const noexcept {
   return mCode;
}

LANGULUS(ALWAYSINLINE)
Rate VulkanShader::GetRate() const noexcept {
   return mStage + Rate::StagesBegin;
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