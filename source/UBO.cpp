#include "UBO.hpp"
#include "CVulkanRenderer.hpp"
#include "Content/CVulkanTexture.hpp"
#include "Content/CVulkanGeometry.hpp"

/// Free VRAM																						
UBO::~UBO() {
	Destroy();
}

/// Free VRAM																						
void UBO::Destroy() {
	if (!mBuffer.IsValid())
		return;
	mRenderer->GetVRAM().DestroyBuffer(mBuffer);
	mRAM.Reset();
}

/// Calculate aligned range, as well as individual uniform byte offsets			
void UBO::CalculateSizes() {
	// Calculate required UBO buffer sizes for the whole pipeline			
	pcptr range = 0;
	for (auto& it : mUniforms) {
		auto concrete = it.mTrait.GetMeta()->GetConcreteMeta();
		if (concrete->IsAbstract()) {
			throw Except::Graphics(pcLogFuncError
				<< "Abstract uniform trait couldn't be concertized: " << it.mTrait);
		}
		else if (!concrete->IsPOD()) {
			throw Except::Graphics(pcLogFuncError
				<< "Uniform trait is not POD: " << it.mTrait);
		}
		else it.mTrait.SetDataID(concrete, false);

		// Info about base alignment in Vulkan Spec								
		//  15.6.4. Offset and Stride Assignment - Alignment Requirements	
		pcptr base_alignment;
		if (  it.mTrait.InterpretsAs<ANumber>(1)
			|| it.mTrait.InterpretsAs<ANumber>(2)
			|| it.mTrait.InterpretsAs<ANumber>(4)) {
			// 1. A scalar has a base alignment equal to its scalar			
			// alignment. A scalar of size N has a scalar alignment of N	
			// 2. A two-component vector has a base alignment equal to		
			// twice its scalar alignment												
			base_alignment = it.mTrait.GetStride();
		}
		else if (it.mTrait.InterpretsAs<ANumber>(3)) {
			// A three- or four-component vector has a base alignment		
			// equal to four times its scalar alignment							
			base_alignment = 4 * it.mTrait.GetMember(nullptr, 0).GetStride();
		}
		else {
			// A structure has a base alignment equal to the largest base	
			// alignment of any of its members. Which coincides with the	
			// alignof() operator in C++11 and later (reflected)				
			base_alignment = it.mTrait.GetMeta()->GetAlignment();
		}

		if (base_alignment == 0)
			throw Except::Graphics(pcLogFuncError << "Bad uniform alignment");

		it.mPosition = pcAlign(range, base_alignment);
		range = it.mPosition + it.mTrait.GetStride();
	}

	if (range) {
		mStride = pcAlign(range, mRenderer->GetOuterUBOAlignment());
		mDescriptor.range = mStride;
	}
}

/// Reallocate a dynamic uniform buffer object											
///	@param elements - the number of buffer elements to allocate					
void UBO::Reallocate(pcptr elements) {
	if (!IsValid() || mAllocated >= elements) {
		// Once allocated enough size, don't do it again						
		return;
	}

	if (mBuffer.IsValid()) {
		// No way to resize VRAM in place, so free the previous buffer		
		mRenderer->GetVRAM().DestroyBuffer(mBuffer);
	}

	// Create the buffer in VRAM														
	mAllocated = elements;
	const auto byteSize = mStride * mAllocated;
	mBuffer = mRenderer->GetVRAM().CreateBuffer(nullptr, VkDeviceSize(byteSize),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	);
	mDescriptor.buffer = mBuffer.mBuffer;

	// Resize the RAM data, retaining contained data							
	mRAM.Allocate(byteSize, false, true);
}

/// Initialize a dynamic uniform buffer object											
///	@param renderer - the renderer														
template<>
void DataUBO<true>::Create(CVulkanRenderer* renderer) {
	mRenderer = renderer;
	CalculateSizes();
	Reallocate(1);
}

/// Initialize a static uniform buffer object											
///	@param renderer - the renderer														
template<>
void DataUBO<false>::Create(CVulkanRenderer* renderer) {
	mRenderer = renderer;
	CalculateSizes();
	Reallocate(1);

	// Set predefined data if available												
	for (auto& it : mUniforms) {
		if (it.mTrait.IsEmpty())
			continue;
		pcCopyMemory(it.mTrait.GetRaw(), mRAM.GetBytes() + it.mPosition, it.mTrait.GetStride());
	}
}

/// Update a dynamic uniform buffer in VRAM												
///	@param binding - binding index														
///	@param set - the set to update														
///	@param output - [out] where updates are registered								
template<>
void DataUBO<true>::Update(pcptr binding, const VkDescriptorSet& set, BufferUpdates& output) const {
	if (!IsValid() || !mUsedCount)
		return;

	output.emplace_back(VkWriteDescriptorSet{});
	auto& write = output.back();
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = set;
	write.dstBinding = decltype(write.dstBinding) (binding);
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	write.descriptorCount = 1;
	write.pBufferInfo = &mDescriptor;
	write.pImageInfo = nullptr;
	write.pTexelBufferView = nullptr;
	mBuffer.Upload(0, mUsedCount * mStride, mRAM.GetRaw());
}

/// Update a static uniform buffer in VRAM												
///	@param binding - binding index														
///	@param set - the set to update														
///	@param output - [out] where updates are registered								
template<>
void DataUBO<false>::Update(pcptr binding, const VkDescriptorSet& set, BufferUpdates& output) const {
	if (!IsValid())
		return;

	output.emplace_back(VkWriteDescriptorSet{});
	auto& write = output.back();
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = set;
	write.dstBinding = decltype(write.dstBinding) (binding);
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo = &mDescriptor;
	write.pImageInfo = nullptr;
	write.pTexelBufferView = nullptr;
	mBuffer.Upload(0, mStride, mRAM.GetRaw());
}

/// Initialize a sampler uniform buffer object											
///	@param renderer - the renderer														
///	@param pool - the pool used for UBOs												
void SamplerUBO::Create(CVulkanRenderer* renderer, VkDescriptorPool pool) {
	mRenderer = renderer;
	mPool = pool;
	mSamplers.Allocate(mUniforms.size(), true);

	// Set predefined samplers															
	for (pcptr id = 0; id < mUniforms.size(); ++id) {
		auto& it = mUniforms[id];
		if (it.mTrait.IsEmpty())
			continue;

		if (it.mTrait.InterpretsAs<CVulkanTexture>())
			Set(it.mTrait.As<CVulkanTexture*>(), id);
		else if (it.mTrait.InterpretsAs<ATexture>())
			Set(it.mTrait.As<ATexture*>(), id);
		else throw Except::Graphics(pcLogFuncError
			<< "Unknown sampler type " << it.mTrait.GetToken() << " for unit " << id);
	}
}

/// Update the sampler buffer in VRAM														
///	@param output - [out] where updates are registered								
void SamplerUBO::Update(BufferUpdates& output) const {
	for (pcptr i = 0; i < mSamplers.size(); ++i) {
		if (!mSamplers[i].sampler)
			continue;

		output.emplace_back(VkWriteDescriptorSet{});
		auto& write = output.back();
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = mSamplersUBOSet;
		write.dstBinding = static_cast<uint32_t>(i);
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = 1;
		write.pBufferInfo = nullptr;
		write.pImageInfo = &mSamplers[i];
		write.pTexelBufferView = nullptr;
	}
}

/// Free up a sampler set																		
SamplerUBO::~SamplerUBO() {
	if (mSamplersUBOSet) {
		vkFreeDescriptorSets(mRenderer->GetDevice(), mPool, 1, &mSamplersUBOSet.mValue);
		mSamplersUBOSet.Reset();
	}
}

/// Cache and initialize the texture														
///	@param value - texture to cache and initialize									
///	@return the VRAM texture																
CVulkanTexture* SamplerUBO::PrepareTexture(ATexture* value) const {
	auto cached = mRenderer->Cache(value);
	cached->Initialize();
	return cached;
}
