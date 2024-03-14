///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Vulkan.hpp"


/// Descriptor constructor                                                    
///   @param producer - the pipeline producer                                 
///   @param descriptor - the pipeline descriptor                             
VulkanPipeline::VulkanPipeline(VulkanRenderer* producer, const Neat& descriptor)
   : Graphics {MetaOf<VulkanPipeline>()}
   , ProducedFrom {producer, descriptor} {
   VERBOSE_VULKAN("Initializing graphics pipeline from: ", descriptor);
   mSubscribers.New();
   mGeometries.New();

   bool predefinedMaterial = false;
   descriptor.ForEach(
      [this](const VulkanLayer& layer) {
         // Add layer preferences                                       
         if (layer.GetStyle() & VulkanLayer::Hierarchical)
            mDepth = false;
         return Loop::NextLoop;
      },
      [this, &predefinedMaterial](const A::Material& material) {
         // Create from predefined material generator                   
         GenerateShaders(material);
         predefinedMaterial = true;
         return Loop::Break;
      }
   );

   if (not predefinedMaterial) {
      // We must generate the material ourselves                        
      Construct material;
      descriptor.ForEach(
         [&](const A::File& file) {
            // Create from file                                         
            material = FromFile(file);
            return Loop::Break;
         },
         [&](const Text& text) {
            // Create from any text, including filenames and code       
            const auto file = GetRuntime()->GetFile(text);
            material = file ? FromFile(*file) : FromCode(text);
            return Loop::Break;
         },
         [&](const A::Mesh& mesh) {
            // Adapt to a mesh                                          
            material = FromMesh(mesh);
            return Loop::Break;
         },
         [&](const A::Image& image) {
            // Adapt to an image                                        
            material = FromImage(image);
            return Loop::Break;
         }
      );

      LANGULUS_ASSERT(material.GetDescriptor(), Graphics,
         "Couldn't generate material request for pipeline");

      // Create the pixel shader output according to the rendering pass 
      // attachment format requirements                                 
      for (const auto& attachment : mProducer->mPassAttachments) {
         switch (attachment.format) {
         case VK_FORMAT_B8G8R8A8_UNORM:
            material << Traits::Output {Rate::Pixel, MetaOf<RGBA>()};
            break;
         case VK_FORMAT_D32_SFLOAT:
         case VK_FORMAT_D16_UNORM:
            // Skip depth                                               
            break;
         default:
            TODO();
         }
      }
         
      // Now create generator, and the pipeline from it                 
      VERBOSE_VULKAN("Pipeline material will be generated from: ", material);
      Verbs::Create creator {Abandon(material)};
      Run(creator)->ForEach([this](const A::Material& generator) {
         GenerateShaders(generator);
      });
   }

   // Proceed with initializing the pipeline                            
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
         LANGULUS_OOPS(Graphics, "Unsupported primitive");
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
   CreateUniformBuffers();

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

   if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &mPipeLayout.Get()))
      LANGULUS_OOPS(Graphics, "Can't create pipeline layout");

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
   if (mStages[ShaderStage::Vertex])
      mInput = mStages[ShaderStage::Vertex]->CreateVertexInputState();

   // Input assembly                                                    
   mAssembly = {};
   mAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   mAssembly.topology = mPrimitive;
   mAssembly.primitiveRestartEnable = VK_FALSE;

   // Create the pipeline                                               
   TAny<Shader> stages;
   for (auto shader : mStages)
      stages << shader->Compile();

   VkGraphicsPipelineCreateInfo pipelineInfo {};
   pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipelineInfo.pViewportState = &viewportState;
   pipelineInfo.pRasterizationState = &rasterizer;
   pipelineInfo.pMultisampleState = &multisampling;
   pipelineInfo.pColorBlendState = &colorBlending;
   pipelineInfo.pDepthStencilState = &depthStencil;
   pipelineInfo.stageCount = static_cast<uint32_t>(stages.GetCount());
   pipelineInfo.pStages = stages.GetRaw();
   pipelineInfo.pVertexInputState = &mInput;
   pipelineInfo.pInputAssemblyState = &mAssembly;
   pipelineInfo.pTessellationState = nullptr;
   pipelineInfo.layout = mPipeLayout;
   pipelineInfo.renderPass = mProducer->mPass;
   pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
   pipelineInfo.pDynamicState = &dynamicState;

   if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline.Get()))
      LANGULUS_OOPS(Graphics, "Can't create graphical pipeline");
}

/// Pipeline destruction                                                      
VulkanPipeline::~VulkanPipeline() {
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
      mUBOPool.Reset();
   }

   // Destroy pipeline                                                  
   if (mPipeline) {
      vkDestroyPipeline(device, mPipeline, nullptr);
      mPipeline.Reset();
   }

   // Destroy pipeline layout                                           
   if (mPipeLayout) {
      vkDestroyPipelineLayout(device, mPipeLayout, nullptr);
      mPipeLayout.Reset();
   }

   // Destroy every UBO layout                                          
   if (mStaticUBOLayout) {
      vkDestroyDescriptorSetLayout(device, mStaticUBOLayout, nullptr);
      mStaticUBOLayout.Reset();
   }
   if (mDynamicUBOLayout) {
      vkDestroyDescriptorSetLayout(device, mDynamicUBOLayout, nullptr);
      mDynamicUBOLayout.Reset();
   }
   if (mSamplersUBOLayout) {
      vkDestroyDescriptorSetLayout(device, mSamplersUBOLayout, nullptr);
      mSamplersUBOLayout.Reset();
   }
}

/// Create a pipeline from a file                                             
///   @param file - the file interface                                        
Construct VulkanPipeline::FromFile(const A::File& file) {
   try {
      // Check if file is a text file, and attempt using it             
      return FromCode(file.ReadAs<Text>());
   }
   catch (...) { throw; }

   //TODO handle other kinds of files
}

/// Create a pipeline capable of rendering a mesh                             
///   @attention geometry will be generated, in order to access data types    
///   @param mesh - the mesh content generator                                
Construct VulkanPipeline::FromMesh(const A::Mesh& mesh) {
   // We use the geometry's properties to define a material             
   // generator, which we later use to initialize this pipeline         
   auto request = Construct::From<A::Material>(
      Traits::Topology {mesh.GetTopology()}
   );

   const auto instances = mesh.GetData<Traits::Transform>();
   if (instances) {
      // Create geometry shader hardware instancing input               
      request << Traits::Input {
         Rate::Primitive, Traits::Transform {instances->GetType()}
      };
   }

   const auto positions = mesh.GetData<Traits::Place>();
   if (positions) {
      // Create a vertex position input and project it                  
      request << Traits::Input {Rate::Vertex, Traits::Place {positions->GetType()}}
              << Traits::Input {Rate::Level,  Traits::View {positions->GetType()}}
              << Traits::Input {Rate::Camera, Traits::Projection {positions->GetType()}};

      // If not using hardware-instancing, use the Thing's instances    
      if (!instances) {
         request << Traits::Input {
            Rate::Instance, Traits::Transform {positions->GetType()}
         };
      }
   }
         
   const auto normals = mesh.GetData<Traits::Aim>();
   if (normals) {
      // Create a vertex normals input                                  
      request << Traits::Input {
         Rate::Vertex, Traits::Aim {normals->GetType()}
      };
   }
         
   const auto textureCoords = mesh.GetData<Traits::Sampler>();
   if (textureCoords) {
      // Create a vertex texture coordinates input                      
      request << Traits::Input {
         Rate::Vertex, Traits::Sampler {textureCoords->GetType()}
      };
   }
         
   const auto materialIds = mesh.GetData<Traits::Material>();
   if (materialIds) {
      // Create a vertex material ids input                             
      request << Traits::Input {
         Rate::Vertex, Traits::Material {materialIds->GetType()}
      };
   }

   return request;
}

/// Create a pipeline capable of rendering an image                           
///   @attention image will be generated, in order to access data types       
///   @param image - the image content generator                              
Construct VulkanPipeline::FromImage(const A::Image& image) {
   // We use the image's properties to define a material                
   // generator, which we later use to initialize this pipeline         
   auto request = Construct::From<A::Material>(
      Traits::Topology {MetaOf<A::TriangleStrip>()}
   );

   const auto colors = image.GetData<Traits::Color>();
   if (colors) {
      // Create pixel shader texture input with the image view          
      request << Traits::Input {
         Rate::Pixel, Traits::Image {image.GetView()}
      };
   }

   return request;
}

/// Initialize the pipeline from any code                                     
///   @param code - the shader code                                           
Construct VulkanPipeline::FromCode(const Text& code) {
   // We use the code to define a material generator, which we later    
   // use to initialize this pipeline                                   
   auto request = Construct::From<A::Material>(
      Traits::Topology {MetaOf<A::TriangleStrip>()},
      code
   );

   return request;
}

/// Use material generator to generate all shader stages as GLSL              
///   @param material - the content to use                                    
void VulkanPipeline::GenerateShaders(const A::Material& material) {
   const auto shaders = material.GetDataList<Traits::Shader>();
   LANGULUS_ASSERT(shaders, Graphics, "No shaders provided in material");

   for (auto stage : *shaders) {
      // Create a vulkan shader for each material-provided shader       
      Verbs::Create creator {Construct::From<VulkanShader>(stage)};
      mProducer->Create(creator);
      auto vkshader = creator->template As<const VulkanShader*>();
      mStages[vkshader->GetStage()] = vkshader;
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
   layoutInfo.bindingCount = static_cast<uint32_t>(bindings.GetCount());
   layoutInfo.pBindings = bindings.GetRaw();

   if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, layout))
      LANGULUS_OOPS(Graphics, "vkCreateDescriptorSetLayout failed");

   // Create a descriptor set                                           
   VkDescriptorSetAllocateInfo allocInfo {};
   allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   allocInfo.descriptorPool = mUBOPool;
   allocInfo.descriptorSetCount = 1;
   allocInfo.pSetLayouts = layout;

   if (vkAllocateDescriptorSets(device, &allocInfo, set))
      LANGULUS_OOPS(Graphics, "vkAllocateDescriptorSets failed");
}

/// Create a new sampler set                                                  
void VulkanPipeline::CreateNewSamplerSet() {
   if (not mSamplersUBOLayout)
      return;

   // If last two samplers match, there is not need to create new one   
   // Just reuse the last one                                           
   if (mSamplerUBO.GetCount() > 1 and mSamplerUBO[-1] == mSamplerUBO[-2])
      return;

   // Copy the previous sampler set                                     
   // This will copy the samplers layout                                
   mSamplerUBO << mSamplerUBO.Last();
   auto& ubo = mSamplerUBO.Last();
   ubo.mSamplersUBOSet.Reset();

   // Create a new descriptor set                                       
   VkDescriptorSetAllocateInfo allocInfo {};
   allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   allocInfo.descriptorPool = mUBOPool;
   allocInfo.descriptorSetCount = 1;
   allocInfo.pSetLayouts = &mSamplersUBOLayout.Get();

   if (vkAllocateDescriptorSets(mProducer->mDevice, &allocInfo, &ubo.mSamplersUBOSet.Get())) {
      LANGULUS_OOPS(Graphics,
         "vkAllocateDescriptorSets failed - either reserve more "
         "items in descriptor pool, or make more pools on demand"
      );
   }

   mSubscribers.Last().samplerSet =
      static_cast<uint32_t>(mSamplerUBO.GetCount() - 1);
}

/// Create a new geometry set                                                 
void VulkanPipeline::CreateNewGeometrySet() {
   // If last two geometries match, there is not need to create new one 
   // Just reuse the last one                                           
   if (mGeometries.GetCount() > 1 and mGeometries[-1] == mGeometries[-2])
      return;

   // For now, only cached geometry is kept, no real set of             
   // properties is required per geometry                               
   mGeometries << nullptr;
   mSubscribers.Last().geometrySet =
      static_cast<uint32_t>(mGeometries.GetCount() - 1);
}

/// Create uniform buffers                                                    
void VulkanPipeline::CreateUniformBuffers() {
   const auto device = mProducer->mDevice;
   LANGULUS_ASSERT(mUniforms, Graphics, "No uniforms/inputs provided by generator");

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
      LANGULUS_OOPS(Graphics,
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
         for (const auto& trait : mUniforms[index]) {
            if (trait.template IsTrait<Traits::Image>())
               continue;
            ubo.mUniforms.Emplace(IndexBack, 0, trait);
         }

         if (!ubo.mUniforms)
            continue;

         // Find out where the UBO is used                              
         //TODO precompute this in asset module!!!
         /*for (auto& uniform : ubo.mUniforms) {
            for (const auto stage : mStages) {
               if (stage->GetCode().Find(GLSL {uniform.mTrait.GetTrait()}))
                  ubo.mStages |= stage->GetStageFlagBit();
            }
         }*/

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
         for (const auto& trait : mUniforms[index]) {
            if (trait.template IsTrait<Traits::Image>())
               continue;
            ubo.mUniforms.Emplace(IndexBack, 0, trait);
         }

         if (not ubo.mUniforms)
            continue;

         // Find out where the UBO is used                              
         //TODO precompute this in asset module!!!
         /*for (auto& uniform : ubo.mUniforms) {
            for (const auto stage : mStages) {
               if (stage->GetCode().Find(GLSL {uniform.mTrait.GetTrait()}))
                  ubo.mStages |= stage->GetStageFlagBit();
            }
         }*/

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
      for (const auto& trait : mUniforms[PerRenderable.GetInputIndex()]) {
         if (not trait.template IsTrait<Traits::Image>())
            continue;
         ubo.mUniforms.Emplace(IndexBack, 0, trait);
      }

      if (ubo.mUniforms) {
         // Set any default samplers if available                       
         Bindings bindings;
         ubo.Create(mProducer, mUBOPool);

         for (auto& uniform : ubo.mUniforms) {
            // Find out where the UBO is used                           
            VkShaderStageFlags mask {};
            //TODO precompute this in asset module!!!
            /*for (const auto stage : mStages) {
               const auto token = 
                  GLSL {uniform.mTrait.GetTrait()}
                + GLSL {bindings.GetCount()};

               if (stage->GetCode().Find(token))
                  mask |= stage->GetStageFlagBit();
            }*/

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
   uint32_t binding {};
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

   if (writes) {
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
///   @return the number of rendered subscribers                              
Count VulkanPipeline::RenderLevel(const Offset& offset) const {
   // Bind the pipeline                                                 
   vkCmdBindPipeline(
      mProducer->GetRenderCB(), 
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      mPipeline
   );

   // Bind static uniform buffer (set 0)                                
   vkCmdBindDescriptorSets(
      mProducer->GetRenderCB(),
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      mPipeLayout, 0, 1, &mStaticUBOSet.Get(), 0, nullptr
   );

   // Get the initial state to check for interrupts                     
   const auto& initial = mSubscribers[offset];
   const auto r = GetRelevantDynamicUBOIndexOfRate<PerLevel>();

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
         auto cmdbuffer = mProducer->GetRenderCB();
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
   vkCmdBindPipeline(
      mProducer->GetRenderCB(), 
      VK_PIPELINE_BIND_POINT_GRAPHICS, 
      mPipeline
   );

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
      auto cmdbuffer = mProducer->GetRenderCB();
      vkCmdDraw(cmdbuffer, 3, 1, 0, 0);
   }
}

/// Reset the used dynamic uniform buffers and the subscribers                
void VulkanPipeline::ResetUniforms() {
   if (mSamplerUBO) {
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
