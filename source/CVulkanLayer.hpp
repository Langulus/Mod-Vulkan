#pragma once
#include "VRAM.hpp"
#include "CVulkanCamera.hpp"
#include "CVulkanRenderable.hpp"
#include "CVulkanLight.hpp"
#include <unordered_set>

class MVulkan;

using LevelSet = std::set<Level>;
using CameraSet = std::unordered_set<const CVulkanCamera*>;
using PipelineSet = std::unordered_set<CVulkanPipeline*>;

///																									
///	LAYER COMPONENT																			
///																									
/// Manages cameras, renderables and lights in a specific way						
///																									
class CVulkanLayer : public AVisualLayer, public TProducedFrom<CVulkanRenderer> {
	// List of cameras																	
	TFactory<CVulkanCamera> mCameras;
	// List of rendererables															
	TFactory<CVulkanRenderable> mRenderables;
	// List of lights																		
	TFactory<CVulkanLight> mLights;
	// A cache of relevant pipelines													
	PipelineSet mRelevantPipelines;
	// A cache of relevant levels														
	LevelSet mRelevantLevels;
	// A cache of relevant cameras													
	CameraSet mRelevantCameras;

	// Subscribers, used only for hierarchical styled layers					
	// Otherwise, CVulkanPipeline::Subscriber is used							
	std::vector<LayerSubscriber> mSubscribers;
	std::vector<pcptr> mSubscriberCountPerLevel;
	std::vector<pcptr> mSubscriberCountPerCamera;

public:
	REFLECT(CVulkanLayer);
	CVulkanLayer(CVulkanRenderer*);

	PC_VERB(Create);

	bool Generate(PipelineSet&);
	void Render(VkCommandBuffer, const VkRenderPass&, const VkFramebuffer&) const;
	const AWindow* GetWindow() const;

private:
	void CompileCameras();
	pcptr CompileLevelBatched(const mat4&, const mat4&, Level, PipelineSet&);
	pcptr CompileLevelHierarchical(const mat4&, const mat4&, Level, PipelineSet&);
	pcptr CompileEntity(const Entity*, LodState&, PipelineSet&);
	CVulkanPipeline* CompileInstance(CVulkanRenderable*, const AInstance*, LodState&);
	pcptr CompileLevels();
	void RenderBatched(VkCommandBuffer, const VkRenderPass&, const VkFramebuffer&) const;
	void RenderHierarchical(VkCommandBuffer, const VkRenderPass&, const VkFramebuffer&) const;
};
