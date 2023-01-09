///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "CVulkanRenderer.hpp"

#define PC_VERBOSE_PIPE(a) pcLogSelfVerbose << a

/// Pipeline construction                                                     
///   @param producer - the pipeline producer                                 
CVulkanPipeline::CVulkanPipeline(CVulkanRenderer* producer)
   : AUnitGraphics {MetaData::Of<CVulkanPipeline>()}
   , TProducedFrom {producer} {
   mSubscribers.emplace_back();
   mGeometries.emplace_back();
}

/// Pipeline destruction                                                      
CVulkanPipeline::~CVulkanPipeline() {
   Uninitialize();
}

/// Get hash                                                                  
///   @return the hash of the pipeline                                        
const Hash& CVulkanPipeline::GetHash() const noexcept {
   return mHash;
}

/// Compare pipelines for equality                                            
///   @param rhs - the pipeline to compare with                               
///   @return true if pipelines are functioanlly the same                     
bool CVulkanPipeline::operator == (const ME& rhs) const noexcept {
   return GetHash() == rhs.GetHash() && mStages == rhs.mStages;
}

/// Compare pipelines for inequality                                          
///   @param rhs - the pipeline to compare with                               
///   @return true if pipelines are functioanlly the same                     
bool CVulkanPipeline::operator != (const ME& rhs) const noexcept {
   return !(*this == rhs);
}

/// Create/destroy stuff inside this pipeline context                         
///   @param verb - creation/destruction verb                                 
void CVulkanPipeline::Create(Verb& verb) {
   if (verb.GetArgument().IsEmpty())
      return;

   pcLogSelfVerbose << "Initializing graphics pipeline from " << verb.GetArgument();
   verb.GetArgument().ForEachDeep([&](Block& group) {
      EitherDoThis
         group.ForEach([&](AFile& file) {
            // Create from shader file interface                        
            auto glslCode = file.ReadAs<GLSL>();
            return glslCode.IsEmpty() || !PrepareFromCode(glslCode);
         })
      OrThis
         group.ForEach([&](const Path& path) {
            // Create from shader file path                             
            auto file = GetProducer()->GetRuntime()->GetFile(path);
            if (file) {
               auto glslCode = file->ReadAs<GLSL>();
               return glslCode.IsEmpty() || !PrepareFromCode(glslCode);
            }
            return true;
         })
      OrThis
         group.ForEach([&](const AMaterial* material) {
            // Create from predefined material generator                
            return !PrepareFromMaterial(material);
         })
      OrThis
         group.ForEach([&](const Construct& construct) {
            // Create from any construct                                
            return !PrepareFromConstruct(construct);
         });

      return mHash == 0;
   });

   if (mHash == 0) {
      // Still not initialized, try wrapping the entire argument        
      // inside a construct                                             
      PrepareFromConstruct(Construct::From<CVulkanPipeline>(verb.GetArgument()));
   }

   if (mHash != 0)
      verb.Done();
}

/// Initialize the pipeline from the accumulated construct                    
///   @return true on success                                                 
void CVulkanPipeline::Initialize() {
   if (mGenerated)
      return;

   mGenerated = true;
   auto device = mProducer->GetDevice().Get();

   // Copy the viewport state                                           
   VkViewport viewport {};
   VkRect2D scissor {};
   VkPipelineViewportStateCreateInfo viewportState {};
   viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewportState.viewportCount = 1;
   viewportState.pViewports = &viewport;
   viewportState.scissorCount = 1;
   viewportState.pScissors = &scissor;

   // Tweak the rasterizer                                              
   VkPipelineRasterizationStateCreateInfo rasterizer {};
   rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   rasterizer.depthClampEnable = VK_FALSE;
   rasterizer.rasterizerDiscardEnable = VK_FALSE;
   rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
   if (mStages[ShaderStage::Vertex]) {
      // Override polygon mode if a vertex shader stage is available    
      switch (mPrimitive) {
      case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
         rasterizer.polygonMode = VK_POLYGON_MODE_POINT;
         break;
      case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
      case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
      case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
         rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
         break;
      case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
      case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
         rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
         break;
      default:
         throw Except::Graphics(pcLogSelfError << "Unsupported primitive");
      }
   }
   //rasterizer.polygonMode = VK_POLYGON_MODE_LINE; //for debuggery
   rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
   rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

   // These are relevant if polygonMode is VK_POLYGON_MODE_LINE         
   rasterizer.lineWidth = 1.0f;
   rasterizer.depthBiasEnable = VK_FALSE;
   rasterizer.depthBiasConstantFactor = 0;
   rasterizer.depthBiasClamp = 0;
   rasterizer.depthBiasSlopeFactor = 0;

   // Setup the antialiasing/supersampling mode                         
   VkPipelineMultisampleStateCreateInfo multisampling {};
   multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisampling.sampleShadingEnable = VK_FALSE;
   multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

   // Define the color blending state                                   
   VkPipelineColorBlendAttachmentState colorBlendAttachment {};
   colorBlendAttachment.colorWriteMask = 
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   colorBlendAttachment.blendEnable = VK_TRUE;
   colorBlendAttachment.srcColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;
   colorBlendAttachment.dstColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
   colorBlendAttachment.colorBlendOp = VkBlendOp::VK_BLEND_OP_ADD;
   colorBlendAttachment.srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE;
   colorBlendAttachment.dstAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO;
   /*colorBlendAttachment.srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;
   colorBlendAttachment.dstAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;*/
   //colorBlendAttachment.alphaBlendOp = VkBlendOp::VK_BLEND_OP_SUBTRACT;
   colorBlendAttachment.alphaBlendOp = VkBlendOp::VK_BLEND_OP_ADD;

   VkPipelineColorBlendStateCreateInfo colorBlending {};
   colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   colorBlending.logicOpEnable = VK_FALSE;
   colorBlending.logicOp = VK_LOGIC_OP_COPY;
   colorBlending.attachmentCount = 1;
   colorBlending.pAttachments = &colorBlendAttachment;

   // Create the uniform buffers                                        
   try { CreateUniformBuffers(); }
   catch (...) {
      Uninitialize();
      throw Except::Graphics(pcLogSelfError
         << "Can't create uniform buffers...");
   }

   // Create the pipeline layouts                                       
   std::vector<UBOLayout> relevantLayouts;
   relevantLayouts.push_back(mStaticUBOLayout);
   relevantLayouts.push_back(mDynamicUBOLayout);
   if (mSamplersUBOLayout)
      relevantLayouts.push_back(mSamplersUBOLayout);

   VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
   pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(relevantLayouts.size());
   pipelineLayoutInfo.pSetLayouts = relevantLayouts.data();
   if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &mPipeLayout.mValue)) {
      Uninitialize();
      throw Except::Graphics(pcLogSelfError
         << "Can't create pipeline layout...");
   }

   // Allow viewport to be dynamically set                              
   const VkDynamicState dynamicStates[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR
   };

   VkPipelineDynamicStateCreateInfo dynamicState {};
   dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamicState.dynamicStateCount = 2;
   dynamicState.pDynamicStates = dynamicStates;

   VkPipelineDepthStencilStateCreateInfo depthStencil {};
   depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
   depthStencil.depthTestEnable = mDepth ? VK_TRUE : VK_FALSE;
   depthStencil.depthWriteEnable = mDepth ? VK_TRUE : VK_FALSE;
   depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
   depthStencil.depthBoundsTestEnable = VK_FALSE;
   depthStencil.minDepthBounds = 0.0f;
   depthStencil.maxDepthBounds = 1.0f;
   depthStencil.stencilTestEnable = VK_FALSE;

   // By default empty input and assembly states are used               
   mInput = {};
   mInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   if (mStages[ShaderStage::Vertex]) {
      mInput.vertexBindingDescriptionCount 
         = uint32_t(mStages[ShaderStage::Vertex]->GetBindings().size());
      mInput.pVertexBindingDescriptions 
         = mStages[ShaderStage::Vertex]->GetBindings().data();
      mInput.vertexAttributeDescriptionCount 
         = uint32_t(mStages[ShaderStage::Vertex]->GetAttributes().size());
      mInput.pVertexAttributeDescriptions 
         = mStages[ShaderStage::Vertex]->GetAttributes().data();
   }

   // Input assembly                                                    
   mAssembly = {};
   mAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   mAssembly.topology = mPrimitive;
   mAssembly.primitiveRestartEnable = VK_FALSE;

   // Create the pipeline                                               
   std::vector<Shader> stages;
   for (int i = 0; i < ShaderStage::Counter; ++i) {
      if (!mStages[i])
         continue;
      mStages[i]->Compile();
      stages.push_back(mStages[i]->GetShader());
   }

   VkGraphicsPipelineCreateInfo pipelineInfo {};
   pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipelineInfo.pViewportState = &viewportState;
   pipelineInfo.pRasterizationState = &rasterizer;
   pipelineInfo.pMultisampleState = &multisampling;
   pipelineInfo.pColorBlendState = &colorBlending;
   pipelineInfo.pDepthStencilState = &depthStencil;
   pipelineInfo.stageCount = uint32_t(stages.size());
   pipelineInfo.pStages = stages.data();
   pipelineInfo.pVertexInputState = &mInput;
   pipelineInfo.pInputAssemblyState = &mAssembly;
   pipelineInfo.pTessellationState = nullptr;
   pipelineInfo.layout = mPipeLayout;
   pipelineInfo.renderPass = mProducer->GetPass();
   pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
   pipelineInfo.pDynamicState = &dynamicState;
   if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline.mValue)) {
      Uninitialize();
      throw Except::Graphics(pcLogSelfError
         << "Can't create graphical pipeline. Aborting...");
   }

   ClassValidate();
}

/// Initialize the pipeline from geometry content                             
///   @param stuff - the content to use for material generation               
///   @return true on success                                                 
bool CVulkanPipeline::PrepareFromConstruct(const Construct& stuff) {
   // Let's build a material construct                                  
   PC_VERBOSE_PIPE("Generating material from: " << stuff);
   Any material;
   pcptr textureId = 0;
   stuff.GetAll().ForEachDeep([&](const Block& group) {
      EitherDoThis
      group.ForEach([&](const AGeometry& geometry) {
         // Create from geometry                                        
         // Make sure the geometry is generated, so that we have the    
         // input bindings                                              
         geometry.Generate();

         mPrimitive = pcPrimitiveToVkPrimitive(geometry.GetTopology());
         
         // Scan the input attributes (inside geometry data)            
         for (auto& trait : geometry.GetDataList()) {
            if (trait.TraitIs<Traits::Index>())
               // Skip indices                                          
               continue;

            if (trait.TraitIs<Traits::Position>()) {
               // Create a position input and project it in vertex      
               // shader                                                
               GASM code = 
                  "Create^PerVertex(ut"_gasm + Traits::Input::ID + "(ut"_gasm + trait.GetTraitID() + "(ud" + trait.GetToken() + ")))"
                  ".Project^PerVertex(ViewProjectTransform, ModelTransform)";
               PC_VERBOSE_PIPE("Incorporating: " << code);
               material << pcMove(code);
            }
            else if (trait.TraitIs<Traits::Aim>()) {
               // Create a normal/tangent/bitangent input and           
               // project it in vertex   shader                         
               GASM code = 
                  "Create^PerVertex(ut"_gasm + Traits::Input::ID + "(ut"_gasm + trait.GetTraitID() + "(ud" + trait.GetToken() + ")))"
                  ".Project^PerVertex(ModelTransform)";
               PC_VERBOSE_PIPE("Incorporating: " << code);
               material << pcMove(code);
            }
            else if (trait.TraitIs<Traits::Sampler>()) {
               // Create a texture coord input and project it in        
               // vertex shader if required                             
               GASM code = 
                  "Create^PerVertex(ut"_gasm + Traits::Input::ID + "(ut"_gasm + trait.GetTraitID() + "(ud" + trait.GetToken() + ")))";
               if (geometry.GetTextureMapper() == Mapper::World)
                  code += ".Project^PerVertex(ModelTransform)";
               else if (geometry.GetTextureMapper() == Mapper::Screen)
                  code += ".Project^PerVertex(ModelTransform)";
               PC_VERBOSE_PIPE("Incorporating: " << code);
               material << pcMove(code);
            }
            else if (trait.TraitIs<Traits::ModelTransform>()) {
               // Create hardware instancing inside geometry shader     
               TODO();
            }
            else {
               // Just declare input and forward it                     
               GASM code = 
                  "Create^PerVertex(ut"_gasm + Traits::Input::ID + "(ut"_gasm + trait.GetTraitID() + "(ud" + trait.GetToken() + ")))";
               PC_VERBOSE_PIPE("Incorporating: " << code);
               material << pcMove(code);
            }
         }

         // Geometry can contain textures (inside geometry descriptor)  
         geometry.GetConstruct().GetAll().ForEachDeep([&](const Trait& t) {
            if (t.TraitIs<Traits::Texture>()) {
               // Add a texture uniform                                 
               //TODO check texture format
               GASM code = "Texturize^PerPixel("_gasm + textureId + ")";
               PC_VERBOSE_PIPE("Incorporating: " << code);
               material << pcMove(code);
               ++textureId;
            }
            else if (t.TraitIs<Traits::Color>()) {
               // Add constant colorization                             
               GASM code = "Texturize^PerPixel("_gasm + t.AsCast<rgba>() + ")";
               PC_VERBOSE_PIPE("Incorporating: " << code);
               material << pcMove(code);
            }
         });

         // Rasterize the geometry, using the standard rasterizer       
         material << "Rasterize^PerVertex()"_gasm;

         // Only one geometry definition allowed                        
         return false;
      })
      OrThis
      group.ForEach([&](const ATexture& texture) {
         // Add texturization, if construct contains any textures       
         GASM code1 = "Create^PerPixel(ut"_gasm + Traits::Input::ID + "(ut"_gasm + Traits::Texture::ID + "(ud" + texture.GetFormat() + ")))";
         GASM code2 = "Texturize^PerPixel("_gasm + textureId + ")";
         PC_VERBOSE_PIPE("Incorporating: " << code1);
         PC_VERBOSE_PIPE("Incorporating: " << code2);
         material << pcMove(code1) << pcMove(code2);
         ++textureId;
      })
      OrThis
      group.ForEach([&](const Trait& t) {
         // Add various global traits, such as texture/color            
         if (t.TraitIs<Traits::Texture>()) {
            // Add a texture uniform                                    
            GASM code = "Texturize^PerPixel("_gasm + textureId + ")";
            PC_VERBOSE_PIPE("Incorporating: " << code);
            material << pcMove(code);
            ++textureId;
         }
         else if (t.TraitIs<Traits::Color>()) {
            // Add constant colorization                                
            GASM code = "Texturize^PerPixel("_gasm + t.AsCast<rgba>() + ")";
            PC_VERBOSE_PIPE("Incorporating: " << code);
            material << pcMove(code);
         }
      })
      OrThis
      group.ForEach([&](const rgba& color) {
         // Add a constant color                                        
         GASM code = "Texturize^PerPixel("_gasm + color + ")";
         PC_VERBOSE_PIPE("Incorporating: " << code);
         material << pcMove(code);
      })
      OrThis
      group.ForEach([&](const CVulkanLayer& layer) {
         // Add layer preferences                                       
         PC_VERBOSE_PIPE("Incorporating layer preferences from " << layer);
         if (layer.GetStyle() & AVisualLayer::Hierarchical)
            mDepth = false;
      });
   });

   if (material.IsEmpty())
      return false;

   // Create the pixel shader output according to the rendering pass    
   // attachment format requirements                                    
   for (const auto& attachment : mProducer->GetPassAttachments()) {
      switch (attachment.format) {
      case VK_FORMAT_B8G8R8A8_UNORM: {
         GASM outputCode =
            "Create^PerPixel(ut"_gasm + Traits::Output::ID + "(ut" + Traits::Color::ID + "(udrgba)))";
         PC_VERBOSE_PIPE("Incorporating: " << outputCode);
         material << pcMove(outputCode);
         break;
      }
      case VK_FORMAT_D32_SFLOAT:
      case VK_FORMAT_D16_UNORM:
         // Skip depth                                                  
         break;
      default:
         TODO();
      }
   }

   // Now create generator and pipeline from it                         
   auto generator = mProducer->GetOwners()[0]->CreateUnit<AMaterial>(pcMove(material));
   return PrepareFromMaterial(generator);
}

/// Initialize the pipeline from any GLSL code                                
///   @param code - the shader code                                           
///   @return true on success                                                 
bool CVulkanPipeline::PrepareFromCode(const GLSL& code) {
   // Let's build a material generator from available code              
   PC_VERBOSE_PIPE("Generating material from code snippet: ");
   PC_VERBOSE_PIPE(code.Pretty());
   auto owner = mProducer->GetOwners()[0];
   auto generator = owner->CreateUnit<AMaterial>(code);
   return PrepareFromMaterial(generator);
}

/// Initialize the pipeline from material content                             
///   @param contentRAM - the content to use                                  
///   @return true on success                                                 
bool CVulkanPipeline::PrepareFromMaterial(const AMaterial* contentRAM) {
   // Generate the shader stages via the material content               
   // No way around this, unfortunately (it's cached, though)           
   contentRAM->Generate();

   if (!mStages.IsAllocated())
      mStages.Allocate(ShaderStage::Counter, true);

   // Iterate all stages and create vulkan shaders for them             
   for (pcptr i = 0; i < ShaderStage::Counter; ++i) {
      if (contentRAM->GetDataAs<Traits::Code, GLSL>(i).IsEmpty())
         continue;

      // Add the stage                                                  
      const auto stage = ShaderStage::Enum(i);
      CVulkanShader shader(mProducer);
      if (!shader.InitializeFromMaterial(stage, contentRAM))
         return false;

      mStages[stage] = mProducer->Cache(pcMove(shader));
      mStages[stage]->Reference(1);
   }

   mHash = contentRAM->GetHash() | pcHash(mBlendMode) | pcHash(mDepth);
   mOriginalContent = contentRAM;
   return true;
}

/// Uninitialize the pipeline                                                 
void CVulkanPipeline::Uninitialize() {
   ClassInvalidate();

   // Dereference valid shaders                                         
   for (auto& shader : mStages) {
      if (shader)
         shader->Reference(-1);
   }

   // Destroy the uniform buffer objects                                
   mRelevantDynamicDescriptors.clear();
   for (auto& ubo : mStaticUBO)
      ubo.Destroy();
   for (auto& ubo : mDynamicUBO)
      ubo.Destroy();
   mSamplerUBO.Reset();

   // Destroy uniform buffer object pool                                
   auto device = mProducer->GetDevice();
   if (mUBOPool) {
      vkDestroyDescriptorPool(device, mUBOPool, nullptr);
      mUBOPool = 0;
   }

   // Destroy pipeline                                                  
   if (mPipeline) {
      vkDestroyPipeline(device, mPipeline, nullptr);
      mPipeline = 0;
   }

   // Destroy pipeline layout                                           
   if (mPipeLayout) {
      vkDestroyPipelineLayout(device, mPipeLayout, nullptr);
      mPipeLayout = 0;
   }

   // Destroy every UBO layout                                          
   if (mStaticUBOLayout) {
      vkDestroyDescriptorSetLayout(device, mStaticUBOLayout, nullptr);
      mStaticUBOLayout = 0;
   }
   if (mDynamicUBOLayout) {
      vkDestroyDescriptorSetLayout(device, mDynamicUBOLayout, nullptr);
      mDynamicUBOLayout = 0;
   }
   if (mSamplersUBOLayout) {
      vkDestroyDescriptorSetLayout(device, mSamplersUBOLayout, nullptr);
      mSamplersUBOLayout = 0;
   }
}

/// Create a UBO descriptor set                                               
///   @param bindings - the bindings to combine in a single layout            
///   @param layout - [out] the resulting layout                              
///   @param set - [out] the resulting set                                    
void CVulkanPipeline::CreateDescriptorLayoutAndSet(
   const Bindings& bindings, UBOLayout* layout, VkDescriptorSet* set
) {
   auto device = mProducer->GetDevice().Get();

   // Combine all bindings in a single layout                           
   VkDescriptorSetLayoutCreateInfo layoutInfo = {};
   layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   layoutInfo.bindingCount = uint32_t(bindings.size());
   layoutInfo.pBindings = bindings.data();
   if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, layout))
      throw Except::Graphics(pcLogSelfError
         << "vkCreateDescriptorSetLayout failed");

   // Create a descriptor set                                           
   VkDescriptorSetAllocateInfo allocInfo = {};
   allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   allocInfo.descriptorPool = mUBOPool;
   allocInfo.descriptorSetCount = 1;
   allocInfo.pSetLayouts = layout;
   if (vkAllocateDescriptorSets(device, &allocInfo, set))
      throw Except::Graphics(pcLogSelfError
         << "vkAllocateDescriptorSets failed");
}

/// Create a new sampler set                                                  
void CVulkanPipeline::CreateNewSamplerSet() {
   if (!mSamplersUBOLayout)
      return;

   // If last two samplers match, there is not need to create new one   
   // Just reuse the last one                                           
   if (mSamplerUBO.GetCount() > 1 && mSamplerUBO[Index(-1)] == mSamplerUBO[Index(-2)])
      return;

   // Copy the previous sampler set                                     
   // This will copy the samplers layout                                
   mSamplerUBO << mSamplerUBO.Last();
   auto& ubo = mSamplerUBO.Last();
   ubo.mSamplersUBOSet = 0;

   // Create a new descriptor set                                       
   VkDescriptorSetAllocateInfo allocInfo = {};
   allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   allocInfo.descriptorPool = mUBOPool;
   allocInfo.descriptorSetCount = 1;
   allocInfo.pSetLayouts = &mSamplersUBOLayout.mValue;
   if (vkAllocateDescriptorSets(mProducer->GetDevice(), &allocInfo, &ubo.mSamplersUBOSet.mValue)) {
      throw Except::Graphics(pcLogSelfError
         << "TODO: vkAllocateDescriptorSets failed - either reserve more "
         << "items in descriptor pool, or make more pools on demand");
   }

   mSubscribers.back().samplerSet = mSamplerUBO.GetCount() - 1;
}

/// Create a new geometry set                                                 
void CVulkanPipeline::CreateNewGeometrySet() {
   // If last two geometries match, there is not need to create new one 
   // Just reuse the last one                                           
   if (mGeometries.size() > 1 && mGeometries[mGeometries.size() - 1] == mGeometries[mGeometries.size() - 2])
      return;

   // For now, only cached geometry is kept, no real set of             
   // properties is required per geometry                               
   mGeometries.emplace_back();

   mSubscribers.back().geometrySet = mGeometries.size() - 1;
}

/// Create uniform buffers                                                    
void CVulkanPipeline::CreateUniformBuffers() {
   auto device = mProducer->GetDevice();
   const auto uniforms = mOriginalContent->GetData<Traits::Trait>();
   if (!uniforms || uniforms->IsEmpty()) {
      throw Except::Graphics(pcLogSelfError
         << "No uniforms/inputs provided by generator: " << mOriginalContent);
   }

   // Create descriptor pools                                           
   static constinit VkDescriptorPoolSize poolSizes[] = {
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, RRate::StaticUniformCount },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, RRate::DynamicUniformCount },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 8 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 8 }
   };

   // All sets use the same pool                                        
   VkDescriptorPoolCreateInfo pool = {};
   pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   pool.poolSizeCount = sizeof(poolSizes) / sizeof(VkDescriptorPoolSize);
   pool.pPoolSizes = poolSizes;
   pool.maxSets = 8;
   pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
   if (vkCreateDescriptorPool(device, &pool, nullptr, &mUBOPool.mValue)) {
      throw Except::Graphics(pcLogSelfError
         << "Can't create UBO pool, so creation of vulkan material fails");
   }

   // Create the static sets (0)                                        
   {
      Bindings bindings;

      // For each static uniform rate (set = 0)...                      
      for (pcptr rate = 0; rate < RRate::StaticUniformCount; ++rate) {
         auto& ubo = mStaticUBO[rate];

         // Add relevant inputs                                         
         const auto index = RRate(RRate::StaticUniformBegin + rate).GetInputIndex();
         auto& inputs = uniforms->As<TAny<Trait>>(index);
         for (auto& trait : inputs) {
            if (trait.TraitIs<Traits::Texture>())
               continue;
            ubo.mUniforms.push_back({ 0, trait });
         }

         if (ubo.mUniforms.empty())
            continue;

         // Find out where the UBO is used                              
         for (auto& uniform : ubo.mUniforms) {
            for (const auto stage : mStages) {
               if (!stage)
                  continue;
               if (stage->GetCode().Find(GLSL(TraitID::Prefix) + uniform.mTrait.GetTraitMeta()->GetToken()))
                  ubo.mStages |= stage->GetStageFlagBit();
            }
         }

         // Allocate VRAM for the buffer and copy initial values if any 
         ubo.Create(mProducer);

         // Create the binding for this rate                            
         VkDescriptorSetLayoutBinding binding = {};
         binding.binding = decltype(binding.binding) (rate);
         binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
         binding.descriptorCount = 1;
         binding.stageFlags = ubo.mStages;
         bindings.push_back(binding);
      }

      CreateDescriptorLayoutAndSet(bindings, &mStaticUBOLayout.mValue, &mStaticUBOSet.mValue);
   }

   // Create the dynamic sets (0)                                       
   {
      Bindings bindings;

      for (pcptr rate = 0; rate < RRate::DynamicUniformCount; ++rate) {
         auto& ubo = mDynamicUBO[rate];

         // Add relevant inputs                                         
         const auto index = RRate(RRate::DynamicUniformBegin + rate).GetInputIndex();
         auto& inputs = uniforms->As<TAny<Trait>>(index);
         for (auto& trait : inputs) {
            if (trait.TraitIs<Traits::Texture>())
               continue;
            ubo.mUniforms.push_back({ 0, trait });
         }

         if (ubo.mUniforms.empty())
            continue;

         // Find out where the UBO is used                              
         for (auto& uniform : ubo.mUniforms) {
            for (const auto stage : mStages) {
               if (!stage)
                  continue;
               if (stage->GetCode().Find(GLSL(TraitID::Prefix) + uniform.mTrait.GetTraitMeta()->GetToken()))
                  ubo.mStages |= stage->GetStageFlagBit();
            }
         }

         // Allocate VRAM for the buffer and copy initial values if any 
         ubo.Create(mProducer);

         // Create the binding for this rate                            
         VkDescriptorSetLayoutBinding binding = {};
         binding.binding = decltype(binding.binding) (rate);
         binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
         binding.descriptorCount = 1;
         binding.stageFlags = ubo.mStages;
         bindings.push_back(binding);
         mRelevantDynamicDescriptors.push_back(&ubo);
      }

      CreateDescriptorLayoutAndSet(bindings, &mDynamicUBOLayout.mValue, &mDynamicUBOSet.mValue);
   }

   // Finally, set the samplers for RRate::PerRenderable only (set = 2) 
   {
      // Add relevant inputs                                            
      SamplerUBO ubo;
      const auto index = RRate(RRate::PerRenderable).GetInputIndex();
      auto& inputs = uniforms->As<TAny<Trait>>(index);
      for (auto& trait : inputs) {
         if (!trait.TraitIs<Traits::Texture>())
            continue;
         ubo.mUniforms << Uniform { 0, trait };
      }

      if (!ubo.mUniforms.IsEmpty()) {
         // Set any default samplers if available                       
         Bindings bindings;
         ubo.Create(mProducer, mUBOPool);

         for (auto& uniform : ubo.mUniforms) {
            // Find out where the UBO is used                           
            VkShaderStageFlags mask = {};
            for (const auto stage : mStages) {
               if (!stage)
                  continue;

               const auto token = GLSL(TraitID::Prefix) + uniform.mTrait.GetTraitMeta()->GetToken() + bindings.size();
               if (stage->GetCode().Find(token))
                  mask |= stage->GetStageFlagBit();
            }

            // Create the bindings for each texture                     
            VkDescriptorSetLayoutBinding binding = {};
            binding.binding = static_cast<uint32_t>(bindings.size());
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.descriptorCount = 1;
            binding.stageFlags = mask;
            bindings.push_back(binding);
         }

         mSamplerUBO << pcMove(ubo);
         CreateDescriptorLayoutAndSet(bindings, &mSamplersUBOLayout.mValue, &mSamplerUBO.Last().mSamplersUBOSet.mValue);
      }
   }

   UpdateUniformBuffers();
}

/// Upload uniform buffers to VRAM                                            
void CVulkanPipeline::UpdateUniformBuffers() const {
   BufferUpdates writes;

   // Gather required static updates                                    
   pcptr binding = 0;
   for (auto& it : mStaticUBO) {
      it.Update(binding, mStaticUBOSet, writes);
      ++binding;
   }

   // Gather required dynamic updates                                   
   binding = 0;
   for (auto& it : mDynamicUBO) {
      it.Update(binding, mDynamicUBOSet, writes);
      ++binding;
   }

   // Gather required sampler set updates                               
   for (auto& samplerSet : mSamplerUBO)
      samplerSet.Update(writes);

   if (!writes.empty()) {
      // Commit all gathered updates to VRAM                            
      vkUpdateDescriptorSets(
         mProducer->GetDevice(),
         uint32_t(writes.size()), writes.data(), 0, nullptr
      );
   }
}

/// Draw all subscribers of a given level (used in batched rendering)         
///   @param offset - the subscriber to start from                            
pcptr CVulkanPipeline::RenderLevel(const pcptr& offset) const {
   // Bind the pipeline                                                 
   vkCmdBindPipeline(mProducer->GetRenderCB(), 
      VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

   // Bind static uniform buffer (set 0)                                
   vkCmdBindDescriptorSets(
      mProducer->GetRenderCB(),
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      mPipeLayout, 0, 1, &mStaticUBOSet.mValue, 0, nullptr
   );

   // Get the initial state to check for interrupts                     
   const auto& initial = mSubscribers[offset];
   const auto r = GetRelevantDynamicUBOIndexOfRate<RRate::PerLevel>();

   // And for each subscriber...                                        
   pcptr i = offset;
   for (; i < mSubscribers.size() - 1; ++i) {
      auto& sub = mSubscribers[i];

      // Abort immediately on lower than level rate change              
      for (pcptr s = 0; s <= r; ++s) {
         if (initial.offsets[s] != sub.offsets[s])
            return i;
      }

      // Bind dynamic uniform buffers (set 1)                           
      vkCmdBindDescriptorSets(
         mProducer->GetRenderCB(),
         VK_PIPELINE_BIND_POINT_GRAPHICS,
         mPipeLayout, 1, 1, &mDynamicUBOSet.mValue,
         static_cast<uint32_t>(mRelevantDynamicDescriptors.size()), sub.offsets
      );

      // Bind the sampler states (set 2)                                
      //TODO optimize this via push constants, instead of using a       
      // dedicated sampler set for each draw call :(                    
      // read more about the forementioned implementation:              
      // http://kylehalladay.com/blog/tutorial/vulkan/2018/01/28/Textue-Arrays-Vulkan.html
      if (mSamplersUBOLayout) {
         vkCmdBindDescriptorSets(
            mProducer->GetRenderCB(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            mPipeLayout, 2, 1, 
            &mSamplerUBO[sub.samplerSet].mSamplersUBOSet.mValue,
            0, nullptr
         );
      }

      if (mGeometries[sub.geometrySet]) {
         // Vertex/index buffers available, draw them                   
         //TODO bind any geometry-dependent uniforms here               
         mGeometries[sub.geometrySet]->Bind();
         mGeometries[sub.geometrySet]->Render();
      }
      else {
         // No geometry available, so simulate a triangle draw          
         // Pipelines usually have a streamless vertex shader bound in  
         // such cases, that draws a zoomed in fullscreen triangle      
         auto& cmdbuffer = mProducer->GetRenderCB();
         vkCmdDraw(cmdbuffer, 3, 1, 0, 0);
      }
   }

   return i;
}

/// Draw a single subscriber (used in hierarchical drawing)                   
///   @param sub - the subscriber to render                                   
///   @attention sub should contain byte offsets for this pipeline's UBOs     
void CVulkanPipeline::RenderSubscriber(const PipeSubscriber& sub) const {
   // Bind the pipeline                                                 
   vkCmdBindPipeline(mProducer->GetRenderCB(), 
      VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

   // Bind static uniform buffer (set 0)                                
   vkCmdBindDescriptorSets(
      mProducer->GetRenderCB(),
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      mPipeLayout, 0, 1, &mStaticUBOSet.mValue, 0, nullptr
   );

   // Bind dynamic uniform buffers (set 1)                              
   vkCmdBindDescriptorSets(
      mProducer->GetRenderCB(),
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      mPipeLayout, 1, 1, &mDynamicUBOSet.mValue,
      static_cast<uint32_t>(mRelevantDynamicDescriptors.size()), sub.offsets
   );

   // Bind the sampler states (set 2)                                   
   //TODO optimize this via push constants, instead of using a          
   // dedicated sampler set for each draw call :(                       
   // read more about the forementioned implementation:                 
   // http://kylehalladay.com/blog/tutorial/vulkan/2018/01/28/Textue-Arrays-Vulkan.html
   if (mSamplersUBOLayout) {
      vkCmdBindDescriptorSets(
         mProducer->GetRenderCB(),
         VK_PIPELINE_BIND_POINT_GRAPHICS,
         mPipeLayout, 2, 1, 
         &mSamplerUBO[sub.samplerSet].mSamplersUBOSet.mValue,
         0, nullptr
      );
   }

   if (mGeometries[sub.geometrySet]) {
      // Vertex/index buffers available, draw them                      
      //TODO bind any geometry-dependent uniforms here                  
      mGeometries[sub.geometrySet]->Bind();
      mGeometries[sub.geometrySet]->Render();
   }
   else {
      // No geometry available, so simulate a triangle draw             
      // Pipelines usually have a streamless vertex shader bound in     
      // such cases, that draws a zoomed in fullscreen triangle         
      auto& cmdbuffer = mProducer->GetRenderCB();
      vkCmdDraw(cmdbuffer, 3, 1, 0, 0);
   }
}

/// Reset the used dynamic uniform buffers and the subscribers                
void CVulkanPipeline::ResetUniforms() {
   // Always keep one sampler set in use, free the rest back to pool    
   if (!mSamplerUBO.IsEmpty())
      mSamplerUBO.Allocate(1);

   // Reset dynamic UBO's usage, keep the allocated space and values    
   for (auto ubo : mRelevantDynamicDescriptors)
      ubo->mUsedCount = 0;

   // Subscribers should be always at least one                         
   mSubscribers.clear();
   mSubscribers.emplace_back();

   // Same applies to geometry sets                                     
   mGeometries.clear();
   mGeometries.emplace_back();
}
