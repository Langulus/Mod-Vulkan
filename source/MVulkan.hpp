#pragma once
#include "CVulkanRenderer.hpp"

///																									
///	THE VULKAN RENDERING MODULE															
///																									
/// Manages and produces renderers. By default, the module itself can convey	
/// GPU computations, if hardware allows it.												
///																									
class MVulkan : public AModuleGraphics {
protected:
	// The vulkan instance																
	Own<VkInstance> mInstance;
	// The physical device																
	Own<VkPhysicalDevice> mAdapter;
	// The logical device																
	Own<VkDevice> mDevice;
	// The computation queue															
	Own<VkQueue> mComputer;

	#if LANGULUS_DEBUG()
		std::vector<const char*> mDebugLayers;
		VkDebugReportCallbackEXT mLogRelay;	// Relays debug messages		
	#endif

	// True if hardware has GPU compute												
	bool mSupportsComputation = false;
	// True if hardware has dedicated transfer queue							
	bool mSupportsTransfer = false;
	// True if hardware has sparse binding support								
	bool mSupportsSparseBinding = false;

	// List of renderer components													
	TFactory<CVulkanRenderer> mRenderers;

public:
	REFLECT(MVulkan);

	MVulkan(CRuntime*, PCLIB);
	~MVulkan();

	void Update(PCTime) final;
	PC_VERB(Create);

	PC_GET(Instance);
	PC_GET(Adapter);

	// Helpers																				
	std::vector<const char*> GetRequiredExtensions() const;
	pcptr RateDevice(const VkPhysicalDevice& device) const;
	VkPhysicalDevice PickAdapter() const;
	#if LANGULUS_DEBUG()
		const std::vector<const char*>& GetValidationLayers() const noexcept;
		bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers) const;
	#endif
};

/// Module entry & exit																			
LANGULUS_DECLARE_MODULE(11, "MVulkan", "The vulkan module is used to draw 3D graphics in real time, and/or do GPU computation.", "Vulkan/", AModuleGraphics);
