///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../Vulkan.hpp"
#include <Anyness/Path.hpp>
#include <shaderc/shaderc.hpp>

#define VERBOSE_SHADER(...) //Logger::Verbose(Self(), __VA_ARGS__)


/// Descriptor constructor                                                    
///   @param producer - the shader producer                                   
///   @param descriptor - the shader descriptor                               
VulkanShader::VulkanShader(VulkanRenderer* producer, const Any& descriptor)
   : Graphics {MetaOf<VulkanShader>(), descriptor} 
   , ProducedFrom {producer, descriptor} {
   // Configure the shader, but don't compile it yet                    
   descriptor.ForEachDeep(
      [this](const ShaderStage::Enum& stage) {
         mStage = stage;
      },
      [this](const A::File& file) {
         // Create from file                                            
         mCode = file.ReadAs<Text>();
      },
      [this](const Path& path) {
         // Create from file path                                       
         const auto file = GetRuntime()->GetFile(path);
         if (file)
            mCode = file->ReadAs<Text>();
      },
      [this](const Text& code) {
         mCode = code;
      },
      [this](const A::Material* material) {
         // Access input mappings                                       
         const auto uniforms = material->template GetDataList<Traits::Trait>();
         if (uniforms && !*uniforms) {
            // Add relevant inputs                                      
            const auto index = Rate(Rate::StagesBegin + mStage).GetInputIndex();
            auto& inputs = uniforms->template As<TAny<Trait>>(index);
            for (auto& attribute : inputs)
               AddInput(attribute);
         }
      }
   );
}

/// Component destruction                                                     
VulkanShader::~VulkanShader() {
   if (mStageDescription.module) {
      vkDestroyShaderModule(
         mProducer->mDevice, 
         mStageDescription.module, 
         nullptr
      );
   }
}

/// Compile the shader code                                                   
///   @return the compiled vulkan shader                                      
const Shader& VulkanShader::Compile() const {
   if (mCompiled)
      return mStageDescription;

   const auto device = mProducer->mDevice;
   const auto startTime = SteadyClock::Now();

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
   const auto assembly = compiler.CompileGlslToSpv(
      mCode.Terminate().GetRaw(), kind, "whatever", options
   );

   if (assembly.GetCompilationStatus() != shaderc_compilation_status_success) {
      Logger::Error(Self(), "Shader Compilation Error: ", assembly.GetErrorMessage());
      Logger::Error(Self(), "For shader:");
      Logger::Error(Self(), mCode);
      LANGULUS_THROW(Graphics, "Shader compilation failed");
   }

   // Create the vulkan shader module                                   
   VkShaderModuleCreateInfo createInfo {};
   createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   createInfo.codeSize = (assembly.cend() - assembly.cbegin()) * sizeof(Decay<decltype(assembly.cbegin())>);
   createInfo.pCode = assembly.cbegin();

   VkShaderModule shaderModule;
   if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
      LANGULUS_THROW(Graphics, "vkCreateShaderModule failed");

   // Create the shader stage                                           
   mStageDescription = {};
   mStageDescription.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   mStageDescription.stage = AsVkStage(mStage);
   mStageDescription.module = shaderModule;
   mStageDescription.pName = "main";
   mCompiled = true;
   
   VERBOSE_SHADER(Logger::Green, "Compiled shader in ", SteadyClock::now() - startTime);
   VERBOSE_SHADER(Logger::Green, mCode.Pretty());
   return mStageDescription;
}

/// Bind vertex input                                                         
///   @param input - input type to bind                                       
void VulkanShader::AddInput(const Trait& input) {
   // On non-vertex shaders - only populate the inputs without          
   // generating code                                                   
   switch (mStage) {
   case ShaderStage::Pixel:
      mInputs[PerPixel.GetStageIndex()] << input;
      return;
   case ShaderStage::Geometry:
      mInputs[PerPrimitive.GetStageIndex()] << input;
      return;
   case ShaderStage::Vertex:
      mInputs[PerVertex.GetStageIndex()] << input;
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

/// Get the vertex stage index                                                
///   @return the vertex stage                                                
ShaderStage::Enum VulkanShader::GetStage() const noexcept {
   return mStage;
}

/// Get the shader code                                                       
const Text& VulkanShader::GetCode() const noexcept {
   return mCode;
}

/// Get a VkShaderStageFlagBits corresponding the this shader's stage         
///   @return the flag                                                        
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

/// Get the rate index of the shader stage                                    
///   @return the rate                                                        
Rate VulkanShader::GetRate() const noexcept {
   return mStage + Rate::StagesBegin;
}

/// Get a vertex input descriptor for vulkan use                              
///   @return the descriptor                                                  
VertexInput VulkanShader::CreateVertexInputState() const noexcept {
   VertexInput result {};
   result.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   result.vertexBindingDescriptionCount = static_cast<uint32_t>(mBindings.size());
   result.pVertexBindingDescriptions = mBindings.data();
   result.vertexAttributeDescriptionCount = static_cast<uint32_t>(mAttributes.size());
   result.pVertexAttributeDescriptions = mAttributes.data();
   return result;
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