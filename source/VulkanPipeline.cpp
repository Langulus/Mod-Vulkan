///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Vulkan.hpp"
#include "GLSL.hpp"

#define VERBOSE_PIPELINE(...) Logger::Verbose(Self(), __VA_ARGS__)


/// Descriptor constructor                                                    
///   @param producer - the pipeline producer                                 
///   @param descriptor - the pipeline descriptor                             
VulkanPipeline::VulkanPipeline(VulkanRenderer* producer, const Any& descriptor)
   : GraphicsUnit {MetaOf<VulkanPipeline>(), descriptor}
   , ProducedFrom {producer, descriptor} {

   VERBOSE_PIPELINE("Initializing graphics pipeline from ", descriptor);
   mSubscribers.New();
   mGeometries.New();

   descriptor.ForEachDeep(
      [this](AFile& file) {
         // Create from shader file interface                           
         auto glslCode = file.ReadAs<GLSL>();
         return glslCode.IsEmpty() || !PrepareFromCode(glslCode);
      },
      [this](const Path& path) {
         // Create from shader file path                                
         auto file = GetProducer()->GetRuntime()->GetFile(path);
         if (file) {
            auto glslCode = file->ReadAs<GLSL>();
            return glslCode.IsEmpty() || !PrepareFromCode(glslCode);
         }
         return true;
      },
      [this](const A::Material* material) {
         // Create from predefined material generator                   
         return !PrepareFromMaterial(material);
      },
      [this](const Construct& construct) {
         // Create from any construct                                   
         return !PrepareFromConstruct(construct);
      }
   );

   if (!mHash) {
      // Still not initialized, try wrapping the entire argument        
      // inside a construct                                             
      PrepareFromConstruct(Construct::From<VulkanPipeline>(descriptor));
   }
}

/// Pipeline destruction                                                      
VulkanPipeline::~VulkanPipeline() {
   Uninitialize();
}

/// Get hash                                                                  
///   @return the hash of the pipeline                                        
const Hash& VulkanPipeline::GetHash() const noexcept {
   return mHash;
}

/// Compare pipelines for equality                                            
///   @param rhs - the pipeline to compare with                               
///   @return true if pipelines are functioanlly the same                     
bool VulkanPipeline::operator == (const VulkanPipeline& rhs) const noexcept {
   return GetHash() == rhs.GetHash() && mStages == rhs.mStages;
}


/// Initialize the pipeline from the accumulated construct                    
void VulkanPipeline::Initialize() {
   if (mGenerated)
      return;
   mGenerated = true;

   const auto device = mProducer->mDevice;

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
         LANGULUS_THROW(Graphics, "Unsupported primitive");
      }
   }
   //rasterizer.polygonMode = VK_POLYGON_MODE_LINE; //for debuggery
   rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
   rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

   // These are relevant if polygonMode is VK_POLYGON_MODE_LINE         
   rasterizer.lineWidth = 1.0f;

   // Setup the antialiasing/supersampling mode                         
   VkPipelineMultisampleStateCreateInfo multisampling {};
   multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

   // Define the color blending state                                   
   VkPipelineColorBlendAttachmentState colorBlendAttachment {};
   colorBlendAttachment.colorWriteMask = 
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   colorBlendAttachment.blendEnable = VK_TRUE;
   colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
   colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
   colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
   colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
   colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
   /*colorBlendAttachment.srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;
   colorBlendAttachment.dstAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;*/
   //colorBlendAttachment.alphaBlendOp = VkBlendOp::VK_BLEND_OP_SUBTRACT;
   colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

   VkPipelineColorBlendStateCreateInfo colorBlending {};
   colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   colorBlending.logicOpEnable = VK_FALSE;//TODO
   colorBlending.logicOp = VK_LOGIC_OP_COPY;
   colorBlending.attachmentCount = 1;
   colorBlending.pAttachments = &colorBlendAttachment;

   // Create the uniform buffers                                        
   try { CreateUniformBuffers(); }
   catch (...) {
      Uninitialize();
      LANGULUS_THROW(Graphics, "Can't create uniform buffers");
   }

   // Create the pipeline layouts                                       
   std::vector<UBOLayout> relevantLayouts {
      mStaticUBOLayout, mDynamicUBOLayout
   };

   if (mSamplersUBOLayout)
      relevantLayouts.push_back(mSamplersUBOLayout);

   VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
   pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(relevantLayouts.size());
   pipelineLayoutInfo.pSetLayouts = relevantLayouts.data();

   if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &mPipeLayout.Get())) {
      Uninitialize();
      LANGULUS_THROW(Graphics, "Can't create pipeline layout");
   }

   // Allow viewport to be dynamically set                              
   const VkDynamicState dynamicStates[] {
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
   depthStencil.minDepthBounds = 0.0f;
   depthStencil.maxDepthBounds = 1.0f;

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
   TAny<Shader> stages;
   for (auto stage : mStages) {
      stage.mValue->Compile();
      stages << stage.mValue->GetShader();
   }

   VkGraphicsPipelineCreateInfo pipelineInfo {};
   pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipelineInfo.pViewportState = &viewportState;
   pipelineInfo.pRasterizationState = &rasterizer;
   pipelineInfo.pMultisampleState = &multisampling;
   pipelineInfo.pColorBlendState = &colorBlending;
   pipelineInfo.pDepthStencilState = &depthStencil;
   pipelineInfo.stageCount = uint32_t(stages.GetCount());
   pipelineInfo.pStages = stages.GetRaw();
   pipelineInfo.pVertexInputState = &mInput;
   pipelineInfo.pInputAssemblyState = &mAssembly;
   pipelineInfo.pTessellationState = nullptr;
   pipelineInfo.layout = mPipeLayout;
   pipelineInfo.renderPass = mProducer->mPass;
   pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
   pipelineInfo.pDynamicState = &dynamicState;

   if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline.Get())) {
      Uninitialize();
      LANGULUS_THROW(Graphics, "Can't create graphical pipeline");
   }
}

/// Initialize the pipeline from geometry content                             
///   @param stuff - the content to use for material generation               
///   @return true on success                                                 
bool VulkanPipeline::PrepareFromConstruct(const Construct& stuff) {
   // Let's build a material construct                                  
   VERBOSE_PIPELINE("Generating material from: ", stuff);

   Any material;
   Offset textureId {};
   stuff.ForEachDeep([&](const Block& group) {
      group.ForEach([&](const A::Geometry& geometry) {
         // Create from geometry                                        
         // Make sure the geometry is generated, so that we have the    
         // input bindings                                              
         geometry.Generate();

         mPrimitive = AsVkPrimitive(geometry.GetTopology());
         
         // Scan the input attributes (inside geometry data)            
         for (auto& trait : geometry.GetDataList()) {
            if (trait.template TraitIs<Traits::Index>())
               // Skip indices                                          
               continue;

            if (trait.template TraitIs<Traits::Place>()) {
               // Create a position input and project it in vertex      
               // shader                                                
               Code code =
                  "Create^PerVertex(Input(Place("_code + trait.GetType() + ")))"
                  ".Project^PerVertex(ViewProjectTransform, ModelTransform)";
               VERBOSE_PIPELINE("Incorporating: ", code);
               material << Abandon(code);
            }
            else if (trait.template TraitIs<Traits::Aim>()) {
               // Create a normal/tangent/bitangent input and           
               // project it in vertex   shader                         
               Code code =
                  "Create^PerVertex(Input(Aim("_code + trait.GetType() + ")))"
                  ".Project^PerVertex(ModelTransform)";
               VERBOSE_PIPELINE("Incorporating: ", code);
               material << Abandon(code);
            }
            else if (trait.template TraitIs<Traits::Sampler>()) {
               // Create a texture coord input and project it in        
               // vertex shader if required                             
               Code code = "Create^PerVertex(Input("_code + trait.GetTraitID() + '(' + trait.GetToken() + ")))";
               if (geometry.GetTextureMapper() == MapMode::World)
                  code += ".Project^PerVertex(ModelTransform)";
               else if (geometry.GetTextureMapper() == MapMode::Screen)
                  code += ".Project^PerVertex(ModelTransform)";
               VERBOSE_PIPELINE("Incorporating: ", code);
               material << Abandon(code);
            }
            else if (trait.template TraitIs<Traits::Transformation>()) {
               // Create hardware instancing inside geometry shader     
               TODO();
            }
            else {
               // Just declare input and forward it                     
               Code code = "Create^PerVertex(Input("_code + trait.GetTraitID() + '(' + trait.GetToken() + ")))";
               VERBOSE_PIPELINE("Incorporating: ", code);
               material << Abandon(code);
            }
         }

         // Geometry can contain textures (inside geometry descriptor)  
         geometry.GetConstruct().ForEachDeep([&](const Trait& t) {
            if (t.template TraitIs<Traits::Texture>()) {
               // Add a texture uniform                                 
               //TODO check texture format
               Code code = "Texturize^PerPixel("_code + textureId + ')';
               VERBOSE_PIPELINE("Incorporating: ", code);
               material << Abandon(code);
               ++textureId;
            }
            else if (t.template TraitIs<Traits::Color>()) {
               // Add constant colorization                             
               Code code = "Texturize^PerPixel("_code + t.AsCast<RGBA>() + ')';
               VERBOSE_PIPELINE("Incorporating: ", code);
               material << Abandon(code);
            }
         });

         // Rasterize the geometry, using the standard rasterizer       
         material << "Rasterize^PerVertex()"_code;

         // Only one geometry definition allowed                        
         return false;
      },
      [&](const A::Texture& texture) {
         // Add texturization, if construct contains any textures       
         Code code1 = "Create^PerPixel(Input(Texture(" + texture.GetFormat() + ")))";
         Code code2 = "Texturize^PerPixel("_code + Code {textureId} + ')';
         VERBOSE_PIPELINE("Incorporating: ", code1);
         VERBOSE_PIPELINE("Incorporating: ", code2);
         material << Abandon(code1) << Abandon(code2);
         ++textureId;
      },
      [&](const Trait& t) {
         // Add various global traits, such as texture/color            
         if (t.template TraitIs<Traits::Texture>()) {
            // Add a texture uniform                                    
            Code code = "Texturize^PerPixel("_code + textureId + ')';
            VERBOSE_PIPELINE("Incorporating: ", code);
            material << Abandon(code);
            ++textureId;
         }
         else if (t.template TraitIs<Traits::Color>()) {
            // Add constant colorization                                
            Code code = "Texturize^PerPixel("_code + t.AsCast<RGBA>() + ')';
            VERBOSE_PIPELINE("Incorporating: ", code);
            material << Abandon(code);
         }
      },
      [&](const RGBA& color) {
         // Add a constant color                                        
         Code code = "Texturize^PerPixel("_code + color + ')';
         VERBOSE_PIPELINE("Incorporating: ", code);
         material << Abandon(code);
      },
      [&](const VulkanLayer& layer) {
         // Add layer preferences                                       
         VERBOSE_PIPELINE("Incorporating layer preferences from ", layer);
         if (layer.GetStyle() & VisualLayer::Hierarchical)
            mDepth = false;
      });
   });

   if (material.IsEmpty())
      return false;

   // Create the pixel shader output according to the rendering pass    
   // attachment format requirements                                    
   for (const auto& attachment : mProducer->mPassAttachments) {
      switch (attachment.format) {
      case VK_FORMAT_B8G8R8A8_UNORM: {
         Code outputCode = "Create^PerPixel(Output(Color(RGBA)))";
         VERBOSE_PIPELINE("Incorporating: ", outputCode);
         material << Abandon(outputCode);
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
   auto generator = mProducer->GetOwners()[0]->CreateUnit<AMaterial>(Move(material));
   return PrepareFromMaterial(generator);
}

/// Initialize the pipeline from any GLSL code                                
///   @param code - the shader code                                           
///   @return true on success                                                 
bool VulkanPipeline::PrepareFromCode(const GLSL& code) {
   // Let's build a material generator from available code              
   VERBOSE_PIPELINE("Generating material from code snippet: ");
   VERBOSE_PIPELINE(code.Pretty());
   auto owner = mProducer->GetOwners()[0];
   auto generator = owner->CreateUnit<AMaterial>(code);
   return PrepareFromMaterial(generator);
}

/// Initialize the pipeline from material content                             
///   @param material - the content to use                                    
///   @return true on success                                                 
bool VulkanPipeline::PrepareFromMaterial(const Unit* material) {
   // Generate the shader stages via the material content               
   // No way around this, unfortunately (it's cached, though)           
   material->Generate();

   // Iterate all stages and create vulkan shaders for them             
   for (Count i = 0; i < ShaderStage::Counter; ++i) {
      if (material->GetDataAs<Traits::Code, GLSL>(i).IsEmpty())
         continue;

      // Add the stage                                                  
      const auto stage = ShaderStage::Enum(i);
      VulkanShader shader(mProducer);
      if (!shader.InitializeFromMaterial(stage, material))
         return false;

      mStages[stage] = mProducer->Cache(Move(shader));
      mStages[stage]->Reference(1);
   }

   mHash = HashData(material->GetHash(), mBlendMode, mDepth);
   mOriginalContent = material;
   return true;
}

/// Uninitialize the pipeline                                                 
void VulkanPipeline::Uninitialize() {
   // Remove shaders                                                    
   mStages.Clear();

   // Destroy the uniform buffer objects                                
   mRelevantDynamicDescriptors.Clear();

   for (auto& ubo : mStaticUBO)
      ubo.Destroy();
   for (auto& ubo : mDynamicUBO)
      ubo.Destroy();

   mSamplerUBO.Reset();

   // Destroy uniform buffer object pool                                
   const auto device = mProducer->mDevice;
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
void VulkanPipeline::CreateDescriptorLayoutAndSet(
   const Bindings& bindings, UBOLayout* layout, VkDescriptorSet* set
) {
   const auto device = mProducer->mDevice;

   // Combine all bindings in a single layout                           
   VkDescriptorSetLayoutCreateInfo layoutInfo {};
   layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   layoutInfo.bindingCount = uint32_t(bindings.GetCount());
   layoutInfo.pBindings = bindings.GetRaw();

   if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, layout))
      LANGULUS_THROW(Graphics, "vkCreateDescriptorSetLayout failed");

   // Create a descriptor set                                           
   VkDescriptorSetAllocateInfo allocInfo {};
   allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   allocInfo.descriptorPool = mUBOPool;
   allocInfo.descriptorSetCount = 1;
   allocInfo.pSetLayouts = layout;

   if (vkAllocateDescriptorSets(device, &allocInfo, set))
      LANGULUS_THROW(Graphics, "vkAllocateDescriptorSets failed");
}

/// Create a new sampler set                                                  
void VulkanPipeline::CreateNewSamplerSet() {
   if (!mSamplersUBOLayout)
      return;

   // If last two samplers match, there is not need to create new one   
   // Just reuse the last one                                           
   if (mSamplerUBO.GetCount() > 1 && mSamplerUBO[-1] == mSamplerUBO[-2])
      return;

   // Copy the previous sampler set                                     
   // This will copy the samplers layout                                
   mSamplerUBO << mSamplerUBO.Last();
   auto& ubo = mSamplerUBO.Last();
   ubo.mSamplersUBOSet = 0;

   // Create a new descriptor set                                       
   VkDescriptorSetAllocateInfo allocInfo {};
   allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   allocInfo.descriptorPool = mUBOPool;
   allocInfo.descriptorSetCount = 1;
   allocInfo.pSetLayouts = &mSamplersUBOLayout.Get();

   if (vkAllocateDescriptorSets(mProducer->mDevice, &allocInfo, &ubo.mSamplersUBOSet.Get())) {
      LANGULUS_THROW(Graphics,
         "vkAllocateDescriptorSets failed - either reserve more "
         "items in descriptor pool, or make more pools on demand"
      );
   }

   mSubscribers.Last().samplerSet = mSamplerUBO.GetCount() - 1;
}

/// Create a new geometry set                                                 
void VulkanPipeline::CreateNewGeometrySet() {
   // If last two geometries match, there is not need to create new one 
   // Just reuse the last one                                           
   if (mGeometries.GetCount() > 1 && mGeometries[-1] == mGeometries[-2])
      return;

   // For now, only cached geometry is kept, no real set of             
   // properties is required per geometry                               
   mGeometries << nullptr;

   mSubscribers.Last().geometrySet = mGeometries.GetCount() - 1;
}

/// Create uniform buffers                                                    
void VulkanPipeline::CreateUniformBuffers() {
   const auto device = mProducer->mDevice;
   const auto uniforms = mOriginalContent->GetData<Traits::Trait>();
   if (!uniforms || uniforms->IsEmpty())
      LANGULUS_THROW(Graphics, "No uniforms/inputs provided by generator");

   // Create descriptor pools                                           
   static constinit VkDescriptorPoolSize poolSizes[] {
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Rate::StaticUniformCount },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, Rate::DynamicUniformCount },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 8 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 8 }
   };

   // All sets use the same pool                                        
   VkDescriptorPoolCreateInfo pool {};
   pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   pool.poolSizeCount = sizeof(poolSizes) / sizeof(VkDescriptorPoolSize);
   pool.pPoolSizes = poolSizes;
   pool.maxSets = 8;
   pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

   if (vkCreateDescriptorPool(device, &pool, nullptr, &mUBOPool.Get())) {
      LANGULUS_THROW(Graphics,
         "Can't create UBO pool, so creation of vulkan material fails");
   }

   // Create the static sets (0)                                        
   {
      // For each static uniform rate (set = 0)...                      
      Bindings bindings;
      for (Count rate = 0; rate < Rate::StaticUniformCount; ++rate) {
         auto& ubo = mStaticUBO[rate];

         // Add relevant inputs                                         
         const auto index = Rate(Rate::StaticUniformBegin + rate).GetInputIndex();
         auto& inputs = uniforms->As<TAny<Trait>>(index);
         for (auto& trait : inputs) {
            if (trait.template TraitIs<Traits::Texture>())
               continue;
            ubo.mUniforms.Emplace(0, trait);
         }

         if (ubo.mUniforms.IsEmpty())
            continue;

         // Find out where the UBO is used                              
         for (auto& uniform : ubo.mUniforms) {
            for (const auto stage : mStages) {
               if (stage->GetCode().Find(GLSL {uniform.mTrait.GetTrait()}))
                  ubo.mStages |= stage->GetStageFlagBit();
            }
         }

         // Allocate VRAM for the buffer and copy initial values if any 
         ubo.Create(mProducer);

         // Create the binding for this rate                            
         VkDescriptorSetLayoutBinding binding {};
         binding.binding = decltype(binding.binding) (rate);
         binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
         binding.descriptorCount = 1;
         binding.stageFlags = ubo.mStages;
         bindings << binding;
      }

      CreateDescriptorLayoutAndSet(bindings, &mStaticUBOLayout.Get(), &mStaticUBOSet.Get());
   }

   // Create the dynamic sets (0)                                       
   {
      Bindings bindings;
      for (Offset rate = 0; rate < Rate::DynamicUniformCount; ++rate) {
         auto& ubo = mDynamicUBO[rate];

         // Add relevant inputs                                         
         const auto index = Rate(Rate::DynamicUniformBegin + rate).GetInputIndex();
         auto& inputs = uniforms->As<TAny<Trait>>(index);
         for (auto& trait : inputs) {
            if (trait.template TraitIs<Traits::Texture>())
               continue;
            ubo.mUniforms.Emplace(0, trait);
         }

         if (ubo.mUniforms.IsEmpty())
            continue;

         // Find out where the UBO is used                              
         for (auto& uniform : ubo.mUniforms) {
            for (const auto stage : mStages) {
               if (stage->GetCode().Find(GLSL {uniform.mTrait.GetTrait()}))
                  ubo.mStages |= stage->GetStageFlagBit();
            }
         }

         // Allocate VRAM for the buffer and copy initial values if any 
         ubo.Create(mProducer);

         // Create the binding for this rate                            
         VkDescriptorSetLayoutBinding binding {};
         binding.binding = decltype(binding.binding) (rate);
         binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
         binding.descriptorCount = 1;
         binding.stageFlags = ubo.mStages;
         bindings << binding;
         mRelevantDynamicDescriptors << &ubo;
      }

      CreateDescriptorLayoutAndSet(bindings, &mDynamicUBOLayout.Get(), &mDynamicUBOSet.Get());
   }

   // Finally, set the samplers for Rate::PerRenderable only (set = 2) 
   {
      // Add relevant inputs                                            
      SamplerUBO ubo;
      const auto index = Rate(Rate::Renderable).GetInputIndex();
      auto& inputs = uniforms->As<TAny<Trait>>(index);
      for (auto& trait : inputs) {
         if (!trait.template TraitIs<Traits::Texture>())
            continue;
         ubo.mUniforms << Uniform {0, trait};
      }

      if (!ubo.mUniforms.IsEmpty()) {
         // Set any default samplers if available                       
         Bindings bindings;
         ubo.Create(mProducer, mUBOPool);

         for (auto& uniform : ubo.mUniforms) {
            // Find out where the UBO is used                           
            VkShaderStageFlags mask {};
            for (const auto stage : mStages) {
               const auto token = 
                  GLSL {uniform.mTrait.GetTrait()}
                + GLSL {bindings.GetCount()};

               if (stage->GetCode().Find(token))
                  mask |= stage->GetStageFlagBit();
            }

            // Create the bindings for each texture                     
            VkDescriptorSetLayoutBinding binding {};
            binding.binding = static_cast<uint32_t>(bindings.GetCount());
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.descriptorCount = 1;
            binding.stageFlags = mask;
            bindings << binding;
         }

         mSamplerUBO << Abandon(ubo);

         CreateDescriptorLayoutAndSet(
            bindings, 
            &mSamplersUBOLayout.Get(), 
            &mSamplerUBO.Last().mSamplersUBOSet.Get()
         );
      }
   }

   UpdateUniformBuffers();
}

/// Upload uniform buffers to VRAM                                            
void VulkanPipeline::UpdateUniformBuffers() const {
   BufferUpdates writes;

   // Gather required static updates                                    
   Offset binding {};
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

   if (!writes.IsEmpty()) {
      // Commit all gathered updates to VRAM                            
      vkUpdateDescriptorSets(
         mProducer->mDevice,
         static_cast<uint32_t>(writes.GetCount()),
         writes.GetRaw(), 0, nullptr
      );
   }
}

/// Draw all subscribers of a given level (used in batched rendering)         
///   @param offset - the subscriber to start from                            
Count VulkanPipeline::RenderLevel(const Offset& offset) const {
   // Bind the pipeline                                                 
   vkCmdBindPipeline(mProducer->GetRenderCB(), 
      VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

   // Bind static uniform buffer (set 0)                                
   vkCmdBindDescriptorSets(
      mProducer->GetRenderCB(),
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      mPipeLayout, 0, 1, &mStaticUBOSet.Get(), 0, nullptr
   );

   // Get the initial state to check for interrupts                     
   const auto& initial = mSubscribers[offset];
   const auto r = GetRelevantDynamicUBOIndexOfRate<Rate::Level>();

   // And for each subscriber...                                        
   Offset i = offset;
   for (; i < mSubscribers.GetCount() - 1; ++i) {
      auto& sub = mSubscribers[i];

      // Abort immediately on lower than level rate change              
      for (Offset s = 0; s <= r; ++s) {
         if (initial.offsets[s] != sub.offsets[s])
            return i;
      }

      // Bind dynamic uniform buffers (set 1)                           
      vkCmdBindDescriptorSets(
         mProducer->GetRenderCB(),
         VK_PIPELINE_BIND_POINT_GRAPHICS,
         mPipeLayout, 1, 1, &mDynamicUBOSet.Get(),
         static_cast<uint32_t>(mRelevantDynamicDescriptors.GetCount()),
         sub.offsets
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
            &mSamplerUBO[sub.samplerSet].mSamplersUBOSet.Get(),
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
void VulkanPipeline::RenderSubscriber(const PipeSubscriber& sub) const {
   // Bind the pipeline                                                 
   vkCmdBindPipeline(mProducer->GetRenderCB(), 
      VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

   // Bind static uniform buffer (set 0)                                
   vkCmdBindDescriptorSets(
      mProducer->GetRenderCB(),
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      mPipeLayout, 0, 1, &mStaticUBOSet.Get(), 0, nullptr
   );

   // Bind dynamic uniform buffers (set 1)                              
   vkCmdBindDescriptorSets(
      mProducer->GetRenderCB(),
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      mPipeLayout, 1, 1, &mDynamicUBOSet.Get(),
      static_cast<uint32_t>(mRelevantDynamicDescriptors.GetCount()),
      sub.offsets
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
         &mSamplerUBO[sub.samplerSet].mSamplersUBOSet.Get(),
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
void VulkanPipeline::ResetUniforms() {
   if (!mSamplerUBO.IsEmpty()) {
      // Always keep one sampler set in use, free the rest back to pool 
      mSamplerUBO.Clear();
      mSamplerUBO.New(1);
   }

   // Reset dynamic UBO's usage, keep the allocated space and values    
   for (auto ubo : mRelevantDynamicDescriptors)
      ubo->mUsedCount = 0;

   // Subscribers should be always at least one                         
   mSubscribers.Clear();
   mSubscribers.New(1);

   // Same applies to geometry sets                                     
   mGeometries.Clear();
   mGeometries << nullptr;
}
