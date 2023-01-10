///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Vulkan.hpp"

//TODO automate this


/*REFLECT_BEGIN(MVulkan)
   REFLECT_INFO("Vulkan graphics module")
   REFLECT_BEGIN_BASES
      REFLECT_BASE(AModuleGraphics)
   REFLECT_END_BASES
   REFLECT_BEGIN_MEMBERS
      REFLECT_MEMBER_TRAIT(mRenderers, Factory)
   REFLECT_END_MEMBERS
   REFLECT_BEGIN_ABILITIES
      REFLECT_ABILITY(Create)
   REFLECT_END_ABILITIES
REFLECT_END

LANGULUS_DEFINE_MODULE(MVulkan);

REFLECT_BEGIN(CVulkanRenderer)
   REFLECT_INFO("Vulkan renderer component")
   REFLECT_PRODUCER(MVulkan)
   REFLECT_BEGIN_BASES
      REFLECT_BASE(ARenderer)
   REFLECT_END_BASES
   REFLECT_BEGIN_ABILITIES
      REFLECT_ABILITY(Create),
      REFLECT_ABILITY(Interpret)
   REFLECT_END_ABILITIES
REFLECT_END

REFLECT_BEGIN(CVulkanLayer)
   REFLECT_INFO("Vulkan layer component")
   REFLECT_PRODUCER(CVulkanRenderer)
   REFLECT_BEGIN_BASES
      REFLECT_BASE(AVisualLayer)
   REFLECT_END_BASES
   REFLECT_BEGIN_MEMBERS
      REFLECT_MEMBER_TRAIT(mCameras, Factory),
      REFLECT_MEMBER_TRAIT(mRenderables, Factory),
      REFLECT_MEMBER_TRAIT(mLights, Factory)
   REFLECT_END_MEMBERS
   REFLECT_BEGIN_ABILITIES
      REFLECT_ABILITY(Create)
   REFLECT_END_ABILITIES
REFLECT_END

REFLECT_BEGIN(CVulkanPipeline)
   REFLECT_INFO("Vulkan pipeline component")
   REFLECT_PRODUCER(CVulkanRenderer)
   REFLECT_FILE("glsl,frag,vert,geom,comp,tesse,tessc")
   REFLECT_BEGIN_BASES
      REFLECT_BASE(AUnitGraphics)
   REFLECT_END_BASES
   REFLECT_BEGIN_ABILITIES
      REFLECT_ABILITY(Create)
   REFLECT_END_ABILITIES
REFLECT_END

REFLECT_BEGIN(CVulkanCamera)
   REFLECT_INFO("Vulkan camera component")
   REFLECT_PRODUCER(CVulkanLayer)
   REFLECT_BEGIN_BASES
      REFLECT_BASE(ACamera)
   REFLECT_END_BASES
REFLECT_END

REFLECT_BEGIN(CVulkanLight)
   REFLECT_INFO("Vulkan light component")
   REFLECT_PRODUCER(CVulkanLayer)
   REFLECT_BEGIN_BASES
      REFLECT_BASE(ALight)
   REFLECT_END_BASES
REFLECT_END

REFLECT_BEGIN(CVulkanRenderable)
   REFLECT_INFO("Vulkan renderable component")
   REFLECT_PRODUCER(CVulkanLayer)
   REFLECT_BEGIN_BASES
      REFLECT_BASE(ARenderable)
   REFLECT_END_BASES*/
   /*REFLECT_BEGIN_MEMBERS
      REFLECT_MEMBER_TRAIT(mGeometry, Model),
      REFLECT_MEMBER_TRAIT(mTexture, Texture),
      REFLECT_MEMBER_TRAIT(mMaterial, Material)
   REFLECT_END_MEMBERS*/
/*REFLECT_END

REFLECT_BEGIN(CVulkanGeometry)
   REFLECT_INFO("Vulkan VRAM geometry content mirror")
   REFLECT_PRODUCER(CVulkanRenderer)
   REFLECT_BEGIN_ABILITIES
      REFLECT_ABILITY(Create)
   REFLECT_END_ABILITIES
   REFLECT_BEGIN_BASES
      REFLECT_BASE(IContentVRAM)
   REFLECT_END_BASES
REFLECT_END

REFLECT_BEGIN(CVulkanShader)
   REFLECT_INFO("Vulkan shader")
   REFLECT_PRODUCER(CVulkanRenderer)
   REFLECT_BEGIN_BASES
      REFLECT_BASE(AUnitGraphics)
   REFLECT_END_BASES
   REFLECT_BEGIN_MEMBERS
      REFLECT_MEMBER_TRAIT(mCode, Code)
   REFLECT_END_MEMBERS
REFLECT_END

REFLECT_BEGIN(CVulkanTexture)
   REFLECT_INFO("Vulkan VRAM texture content mirror")
   REFLECT_PRODUCER(CVulkanRenderer)
   REFLECT_BEGIN_ABILITIES
      REFLECT_ABILITY(Create)
   REFLECT_END_ABILITIES
   REFLECT_BEGIN_BASES
      REFLECT_BASE(IContentVRAM)
   REFLECT_END_BASES
REFLECT_END


/// Links the module with the framework                                       
void LANGULUS_MODULE_LINKER() {
   MetaData::REGISTER<MVulkan>();
   MetaData::REGISTER<CVulkanRenderer>();
   MetaData::REGISTER<CVulkanPipeline>();
   MetaData::REGISTER<CVulkanRenderable>();
   MetaData::REGISTER<CVulkanCamera>();
   MetaData::REGISTER<CVulkanLight>();
   MetaData::REGISTER<CVulkanGeometry>();
   MetaData::REGISTER<CVulkanShader>();
   MetaData::REGISTER<CVulkanTexture>();

   MetaTrait::REGISTER<Traits::Shader>();
}

/// Unlinks the module                                                         
void LANGULUS_MODULE_DESTROYER() {
   MetaTrait::UNREGISTER<Traits::Shader>();

   MetaData::UNREGISTER<CVulkanTexture>();
   MetaData::UNREGISTER<CVulkanShader>();
   MetaData::UNREGISTER<CVulkanGeometry>();
   MetaData::UNREGISTER<CVulkanLight>();
   MetaData::UNREGISTER<CVulkanCamera>();
   MetaData::UNREGISTER<CVulkanRenderable>();
   MetaData::UNREGISTER<CVulkanPipeline>();
   MetaData::UNREGISTER<CVulkanRenderer>();
   MetaData::UNREGISTER<MVulkan>();
}*/
