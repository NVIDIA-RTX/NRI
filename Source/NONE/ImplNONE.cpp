// © 2021 NVIDIA Corporation

#include "SharedExternal.h"

using namespace nri;

template <typename T>
constexpr T* DummyObject() {
    return (T*)(size_t)(1);
}

struct DeviceNONE final : public DeviceBase {
    inline DeviceNONE(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks, const AdapterDesc* adapterDesc)
        : DeviceBase(callbacks, allocationCallbacks) {
        if (adapterDesc)
            m_Desc.adapterDesc = *adapterDesc;

        for (uint32_t i = 0; i < (uint32_t)QueueType::MAX_NUM; i++)
            m_Desc.adapterDesc.queueNum[i] = 4;

        m_Desc.graphicsAPI = GraphicsAPI::NONE;
        m_Desc.nriVersion = NRI_VERSION;
        m_Desc.shaderModel = 69;

        m_Desc.viewport.maxNum = 16;
        m_Desc.viewport.boundsMin = -32768;
        m_Desc.viewport.boundsMax = 32767;

        m_Desc.dimensions.attachmentMaxDim = 16384;
        m_Desc.dimensions.attachmentLayerMaxNum = 2048;
        m_Desc.dimensions.texture1DMaxDim = 16384;
        m_Desc.dimensions.texture2DMaxDim = 16384;
        m_Desc.dimensions.texture3DMaxDim = 16384;
        m_Desc.dimensions.textureLayerMaxNum = 16384;
        m_Desc.dimensions.typedBufferMaxDim = uint32_t(-1);

        m_Desc.precision.viewportBits = 8;
        m_Desc.precision.subPixelBits = 8;
        m_Desc.precision.subTexelBits = 8;
        m_Desc.precision.mipmapBits = 8;

        m_Desc.memory.deviceUploadHeapSize = 256 * 1024 * 1024;
        m_Desc.memory.allocationMaxNum = uint32_t(-1);
        m_Desc.memory.samplerAllocationMaxNum = 4096;
        m_Desc.memory.constantBufferMaxRange = 64 * 1024;
        m_Desc.memory.storageBufferMaxRange = uint32_t(-1);
        m_Desc.memory.bufferTextureGranularity = 1;
        m_Desc.memory.bufferMaxSize = uint32_t(-1);

        m_Desc.memoryAlignment.uploadBufferTextureRow = 1;
        m_Desc.memoryAlignment.uploadBufferTextureSlice = 1;
        m_Desc.memoryAlignment.bufferShaderResourceOffset = 1;
        m_Desc.memoryAlignment.constantBufferOffset = 1;
        m_Desc.memoryAlignment.scratchBufferOffset = 1;
        m_Desc.memoryAlignment.shaderBindingTable = 1;
        m_Desc.memoryAlignment.accelerationStructureOffset = 1;
        m_Desc.memoryAlignment.micromapOffset = 1;

        m_Desc.pipelineLayout.descriptorSetMaxNum = 64;
        m_Desc.pipelineLayout.rootConstantMaxSize = 256;
        m_Desc.pipelineLayout.rootDescriptorMaxNum = 64;

        m_Desc.descriptorSet.samplerMaxNum = 1000000;
        m_Desc.descriptorSet.constantBufferMaxNum = 1000000;
        m_Desc.descriptorSet.storageBufferMaxNum = 1000000;
        m_Desc.descriptorSet.textureMaxNum = 1000000;
        m_Desc.descriptorSet.storageTextureMaxNum = 1000000;

        m_Desc.descriptorSet.updateAfterSet.samplerMaxNum = m_Desc.descriptorSet.samplerMaxNum;
        m_Desc.descriptorSet.updateAfterSet.constantBufferMaxNum = m_Desc.descriptorSet.constantBufferMaxNum;
        m_Desc.descriptorSet.updateAfterSet.storageBufferMaxNum = m_Desc.descriptorSet.storageBufferMaxNum;
        m_Desc.descriptorSet.updateAfterSet.textureMaxNum = m_Desc.descriptorSet.textureMaxNum;
        m_Desc.descriptorSet.updateAfterSet.storageTextureMaxNum = m_Desc.descriptorSet.storageTextureMaxNum;

        m_Desc.shaderStage.descriptorSamplerMaxNum = 1000000;
        m_Desc.shaderStage.descriptorConstantBufferMaxNum = 1000000;
        m_Desc.shaderStage.descriptorStorageBufferMaxNum = 1000000;
        m_Desc.shaderStage.descriptorTextureMaxNum = 1000000;
        m_Desc.shaderStage.descriptorStorageTextureMaxNum = 1000000;
        m_Desc.shaderStage.resourceMaxNum = 1000000;

        m_Desc.shaderStage.updateAfterSet.descriptorSamplerMaxNum = m_Desc.shaderStage.descriptorSamplerMaxNum;
        m_Desc.shaderStage.updateAfterSet.descriptorConstantBufferMaxNum = m_Desc.shaderStage.descriptorConstantBufferMaxNum;
        m_Desc.shaderStage.updateAfterSet.descriptorStorageBufferMaxNum = m_Desc.shaderStage.descriptorStorageBufferMaxNum;
        m_Desc.shaderStage.updateAfterSet.descriptorTextureMaxNum = m_Desc.shaderStage.descriptorTextureMaxNum;
        m_Desc.shaderStage.updateAfterSet.descriptorStorageTextureMaxNum = m_Desc.shaderStage.descriptorStorageTextureMaxNum;
        m_Desc.shaderStage.updateAfterSet.resourceMaxNum = m_Desc.shaderStage.resourceMaxNum;

        m_Desc.shaderStage.vertex.attributeMaxNum = 32;
        m_Desc.shaderStage.vertex.streamMaxNum = 32;
        m_Desc.shaderStage.vertex.outputComponentMaxNum = 128;

        m_Desc.shaderStage.tesselationControl.generationMaxLevel = 64.0f;
        m_Desc.shaderStage.tesselationControl.patchPointMaxNum = 32;
        m_Desc.shaderStage.tesselationControl.perVertexInputComponentMaxNum = 128;
        m_Desc.shaderStage.tesselationControl.perVertexOutputComponentMaxNum = 128;
        m_Desc.shaderStage.tesselationControl.perPatchOutputComponentMaxNum = 128;
        m_Desc.shaderStage.tesselationControl.totalOutputComponentMaxNum = 1000000;

        m_Desc.shaderStage.tesselationEvaluation.inputComponentMaxNum = 128;
        m_Desc.shaderStage.tesselationEvaluation.outputComponentMaxNum = 128;

        m_Desc.shaderStage.geometry.invocationMaxNum = 32;
        m_Desc.shaderStage.geometry.inputComponentMaxNum = 128;
        m_Desc.shaderStage.geometry.outputComponentMaxNum = 128;
        m_Desc.shaderStage.geometry.outputVertexMaxNum = 1024;
        m_Desc.shaderStage.geometry.totalOutputComponentMaxNum = 1024;

        m_Desc.shaderStage.fragment.inputComponentMaxNum = 128;
        m_Desc.shaderStage.fragment.attachmentMaxNum = 8;
        m_Desc.shaderStage.fragment.dualSourceAttachmentMaxNum = 1;

        m_Desc.shaderStage.compute.workGroupMaxNum[0] = 64 * 1024;
        m_Desc.shaderStage.compute.workGroupMaxNum[1] = 64 * 1024;
        m_Desc.shaderStage.compute.workGroupMaxNum[2] = 64 * 1024;
        m_Desc.shaderStage.compute.workGroupMaxDim[0] = 64 * 1024;
        m_Desc.shaderStage.compute.workGroupMaxDim[1] = 64 * 1024;
        m_Desc.shaderStage.compute.workGroupMaxDim[2] = 64 * 1024;
        m_Desc.shaderStage.compute.workGroupInvocationMaxNum = 64 * 1024;
        m_Desc.shaderStage.compute.sharedMemoryMaxSize = 64 * 1024;

        m_Desc.shaderStage.rayTracing.shaderGroupIdentifierSize = 32;
        m_Desc.shaderStage.rayTracing.tableMaxStride = (uint32_t)(-1);
        m_Desc.shaderStage.rayTracing.recursionMaxDepth = 31;

        m_Desc.shaderStage.meshControl.sharedMemoryMaxSize = 64 * 1024;
        m_Desc.shaderStage.meshControl.workGroupInvocationMaxNum = 128;
        m_Desc.shaderStage.meshControl.payloadMaxSize = 64 * 1024;

        m_Desc.shaderStage.meshEvaluation.outputVerticesMaxNum = 256;
        m_Desc.shaderStage.meshEvaluation.outputPrimitiveMaxNum = 256;
        m_Desc.shaderStage.meshEvaluation.outputComponentMaxNum = 128;
        m_Desc.shaderStage.meshEvaluation.sharedMemoryMaxSize = 64 * 1024;
        m_Desc.shaderStage.meshEvaluation.workGroupInvocationMaxNum = 128;

        m_Desc.wave.laneMinNum = 32;
        m_Desc.wave.laneMaxNum = 32;
        m_Desc.wave.waveOpsStages = StageBits::ALL_SHADERS;
        m_Desc.wave.derivativeOpsStages = StageBits::ALL_SHADERS;
        m_Desc.wave.quadOpsStages = StageBits::ALL_SHADERS;

        m_Desc.other.timestampFrequencyHz = 1;
        m_Desc.other.micromapSubdivisionMaxLevel = 12;
        m_Desc.other.drawIndirectMaxNum = uint32_t(-1);
        m_Desc.other.samplerLodBiasMax = 16.0f;
        m_Desc.other.samplerAnisotropyMax = 16;
        m_Desc.other.texelOffsetMin = -8;
        m_Desc.other.texelOffsetMax = 7;
        m_Desc.other.texelGatherOffsetMin = -8;
        m_Desc.other.texelGatherOffsetMax = 7;
        m_Desc.other.clipDistanceMaxNum = 8;
        m_Desc.other.cullDistanceMaxNum = 8;
        m_Desc.other.combinedClipAndCullDistanceMaxNum = 8;
        m_Desc.other.viewMaxNum = 4;
        m_Desc.other.shadingRateAttachmentTileSize = 16;

        memset(&m_Desc.tiers, 0xFF, sizeof(m_Desc.tiers));
        memset(&m_Desc.features, 0xFF, sizeof(m_Desc.features));
        memset(&m_Desc.shaderFeatures, 0xFF, sizeof(m_Desc.shaderFeatures));
    }

    inline ~DeviceNONE() {
    }

    //================================================================================================================
    // DeviceBase
    //================================================================================================================

    inline const DeviceDesc& GetDesc() const override {
        return m_Desc;
    }

    inline void Destruct() override {
        Destroy(GetAllocationCallbacks(), this);
    }

    Result FillFunctionTable(CoreInterface& table) const override;
    Result FillFunctionTable(HelperInterface& table) const override;
    Result FillFunctionTable(LowLatencyInterface& table) const override;
    Result FillFunctionTable(MeshShaderInterface& table) const override;
    Result FillFunctionTable(RayTracingInterface& table) const override;
    Result FillFunctionTable(StreamerInterface& table) const override;
    Result FillFunctionTable(SwapChainInterface& table) const override;
    Result FillFunctionTable(UpscalerInterface& table) const override;

#if NRI_ENABLE_IMGUI_EXTENSION
    Result FillFunctionTable(ImguiInterface& table) const override;
#endif

private:
    DeviceDesc m_Desc = {};
};

Result CreateDeviceNONE(const DeviceCreationDesc& desc, DeviceBase*& device) {
    DeviceNONE* impl = Allocate<DeviceNONE>(desc.allocationCallbacks, desc.callbackInterface, desc.allocationCallbacks, desc.adapterDesc);

    if (!impl) {
        Destroy(desc.allocationCallbacks, impl);
        device = nullptr;

        return Result::FAILURE;
    }

    device = (DeviceBase*)impl;

    return Result::SUCCESS;
}

//============================================================================================================================================================================================
#pragma region[  Core  ]

static const DeviceDesc& NRI_CALL GetDeviceDesc(const Device& device) {
    return ((DeviceNONE&)device).GetDesc();
}

static const BufferDesc& NRI_CALL GetBufferDesc(const Buffer&) {
    static const BufferDesc bufferDesc = {1};

    return bufferDesc;
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture&) {
    static const TextureDesc textureDesc = {TextureType::TEXTURE_1D, TextureUsageBits::NONE, Format::R8_UNORM, 1, 1, 1, 1, 1, 1};

    return textureDesc;
}

static FormatSupportBits NRI_CALL GetFormatSupport(const Device&, Format) {
    return (FormatSupportBits)(-1);
}

static uint32_t NRI_CALL GetQuerySize(const QueryPool&) {
    return 0;
}

static uint64_t NRI_CALL GetFenceValue(Fence&) {
    return 0;
}

static Result NRI_CALL GetQueue(Device&, QueueType, uint32_t, Queue*& queue) {
    queue = DummyObject<Queue>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateCommandAllocator(Queue&, CommandAllocator*& commandAllocator) {
    commandAllocator = DummyObject<CommandAllocator>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateCommandBuffer(CommandAllocator&, CommandBuffer*& commandBuffer) {
    commandBuffer = DummyObject<CommandBuffer>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateFence(Device&, uint64_t, Fence*& fence) {
    fence = DummyObject<Fence>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateDescriptorPool(Device&, const DescriptorPoolDesc&, DescriptorPool*& descriptorPool) {
    descriptorPool = DummyObject<DescriptorPool>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreatePipelineLayout(Device&, const PipelineLayoutDesc&, PipelineLayout*& pipelineLayout) {
    pipelineLayout = DummyObject<PipelineLayout>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateGraphicsPipeline(Device&, const GraphicsPipelineDesc&, Pipeline*& pipeline) {
    pipeline = DummyObject<Pipeline>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateComputePipeline(Device&, const ComputePipelineDesc&, Pipeline*& pipeline) {
    pipeline = DummyObject<Pipeline>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateQueryPool(Device&, const QueryPoolDesc&, QueryPool*& queryPool) {
    queryPool = DummyObject<QueryPool>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateSampler(Device&, const SamplerDesc&, Descriptor*& sampler) {
    sampler = DummyObject<Descriptor>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateBufferView(const BufferViewDesc&, Descriptor*& bufferView) {
    bufferView = DummyObject<Descriptor>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateTexture1DView(const Texture1DViewDesc&, Descriptor*& textureView) {
    textureView = DummyObject<Descriptor>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateTexture2DView(const Texture2DViewDesc&, Descriptor*& textureView) {
    textureView = DummyObject<Descriptor>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateTexture3DView(const Texture3DViewDesc&, Descriptor*& textureView) {
    textureView = DummyObject<Descriptor>();

    return Result::SUCCESS;
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator*) {
}

static void NRI_CALL DestroyCommandBuffer(CommandBuffer*) {
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool*) {
}

static void NRI_CALL DestroyBuffer(Buffer*) {
}

static void NRI_CALL DestroyTexture(Texture*) {
}

static void NRI_CALL DestroyDescriptor(Descriptor*) {
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout*) {
}

static void NRI_CALL DestroyPipeline(Pipeline*) {
}

static void NRI_CALL DestroyQueryPool(QueryPool*) {
}

static void NRI_CALL DestroyFence(Fence*) {
}

static Result NRI_CALL AllocateMemory(Device&, const AllocateMemoryDesc&, Memory*& memory) {
    memory = DummyObject<Memory>();

    return Result::SUCCESS;
}

static void NRI_CALL FreeMemory(Memory*) {
}

static Result NRI_CALL CreateBuffer(Device&, const BufferDesc&, Buffer*& buffer) {
    buffer = DummyObject<Buffer>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateTexture(Device&, const TextureDesc&, Texture*& texture) {
    texture = DummyObject<Texture>();

    return Result::SUCCESS;
}

static void NRI_CALL GetBufferMemoryDesc(const Buffer&, MemoryLocation, MemoryDesc& memoryDesc) {
    memoryDesc = {1};
}

static void NRI_CALL GetTextureMemoryDesc(const Texture&, MemoryLocation, MemoryDesc& memoryDesc) {
    memoryDesc = {1};
}

static Result NRI_CALL BindBufferMemory(const BindBufferMemoryDesc*, uint32_t) {
    return Result::SUCCESS;
}

static Result NRI_CALL BindTextureMemory(const BindTextureMemoryDesc*, uint32_t) {
    return Result::SUCCESS;
}

static void NRI_CALL GetBufferMemoryDesc2(const Device&, const BufferDesc&, MemoryLocation, MemoryDesc& memoryDesc) {
    memoryDesc = {1};
}

static void NRI_CALL GetTextureMemoryDesc2(const Device&, const TextureDesc&, MemoryLocation, MemoryDesc& memoryDesc) {
    memoryDesc = {1};
}

static Result NRI_CALL CreateCommittedBuffer(Device&, MemoryLocation, float, const BufferDesc&, Buffer*& buffer) {
    buffer = DummyObject<Buffer>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateCommittedTexture(Device&, MemoryLocation, float, const TextureDesc&, Texture*& texture) {
    texture = DummyObject<Texture>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreatePlacedBuffer(Device&, Memory*, uint64_t, const BufferDesc&, Buffer*& buffer) {
    buffer = DummyObject<Buffer>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreatePlacedTexture(Device&, Memory*, uint64_t, const TextureDesc&, Texture*& texture) {
    texture = DummyObject<Texture>();

    return Result::SUCCESS;
}

static Result NRI_CALL AllocateDescriptorSets(DescriptorPool&, const PipelineLayout&, uint32_t, DescriptorSet**, uint32_t, uint32_t) {
    return Result::SUCCESS;
}

static void NRI_CALL UpdateDescriptorRanges(const UpdateDescriptorRangeDesc*, uint32_t) {
}

static void NRI_CALL CopyDescriptorRanges(const CopyDescriptorRangeDesc*, uint32_t) {
}

static void NRI_CALL ResetDescriptorPool(DescriptorPool&) {
}

static Result NRI_CALL BeginCommandBuffer(CommandBuffer&, const DescriptorPool*) {
    return Result::SUCCESS;
}

static void NRI_CALL CmdSetDescriptorPool(CommandBuffer&, const DescriptorPool&) {
}

static void NRI_CALL CmdSetPipelineLayout(CommandBuffer&, BindPoint, const PipelineLayout&) {
}

static void NRI_CALL CmdSetDescriptorSet(CommandBuffer&, const SetDescriptorSetDesc&) {
}

static void NRI_CALL CmdSetRootConstants(CommandBuffer&, const SetRootConstantsDesc&) {
}

static void NRI_CALL CmdSetRootDescriptor(CommandBuffer&, const SetRootDescriptorDesc&) {
}

static void NRI_CALL CmdSetPipeline(CommandBuffer&, const Pipeline&) {
}

static void NRI_CALL CmdBarrier(CommandBuffer&, const BarrierDesc&) {
}

static void NRI_CALL CmdSetIndexBuffer(CommandBuffer&, const Buffer&, uint64_t, IndexType) {
}

static void NRI_CALL CmdSetVertexBuffers(CommandBuffer&, uint32_t, const VertexBufferDesc*, uint32_t) {
}

static void NRI_CALL CmdSetViewports(CommandBuffer&, const Viewport*, uint32_t) {
}

static void NRI_CALL CmdSetScissors(CommandBuffer&, const Rect*, uint32_t) {
}

static void NRI_CALL CmdSetStencilReference(CommandBuffer&, uint8_t, uint8_t) {
}

static void NRI_CALL CmdSetDepthBounds(CommandBuffer&, float, float) {
}

static void NRI_CALL CmdSetBlendConstants(CommandBuffer&, const Color32f&) {
}

static void NRI_CALL CmdSetSampleLocations(CommandBuffer&, const SampleLocation*, Sample_t, Sample_t) {
}

static void NRI_CALL CmdSetShadingRate(CommandBuffer&, const ShadingRateDesc&) {
}

static void NRI_CALL CmdSetDepthBias(CommandBuffer&, const DepthBiasDesc&) {
}

static void NRI_CALL CmdBeginRendering(CommandBuffer&, const AttachmentsDesc&) {
}

static void NRI_CALL CmdClearAttachments(CommandBuffer&, const ClearDesc*, uint32_t, const Rect*, uint32_t) {
}

static void NRI_CALL CmdDraw(CommandBuffer&, const DrawDesc&) {
}

static void NRI_CALL CmdDrawIndexed(CommandBuffer&, const DrawIndexedDesc&) {
}

static void NRI_CALL CmdDrawIndirect(CommandBuffer&, const Buffer&, uint64_t, uint32_t, uint32_t, const Buffer*, uint64_t) {
}

static void NRI_CALL CmdDrawIndexedIndirect(CommandBuffer&, const Buffer&, uint64_t, uint32_t, uint32_t, const Buffer*, uint64_t) {
}

static void NRI_CALL CmdEndRendering(CommandBuffer&) {
}

static void NRI_CALL CmdDispatch(CommandBuffer&, const DispatchDesc&) {
}

static void NRI_CALL CmdDispatchIndirect(CommandBuffer&, const Buffer&, uint64_t) {
}

static void NRI_CALL CmdCopyBuffer(CommandBuffer&, Buffer&, uint64_t, const Buffer&, uint64_t, uint64_t) {
}

static void NRI_CALL CmdCopyTexture(CommandBuffer&, Texture&, const TextureRegionDesc*, const Texture&, const TextureRegionDesc*) {
}

static void NRI_CALL CmdUploadBufferToTexture(CommandBuffer&, Texture&, const TextureRegionDesc&, const Buffer&, const TextureDataLayoutDesc&) {
}

static void NRI_CALL CmdReadbackTextureToBuffer(CommandBuffer&, Buffer&, const TextureDataLayoutDesc&, const Texture&, const TextureRegionDesc&) {
}

static void NRI_CALL CmdZeroBuffer(CommandBuffer&, Buffer&, uint64_t, uint64_t) {
}

static void NRI_CALL CmdResolveTexture(CommandBuffer&, Texture&, const TextureRegionDesc*, const Texture&, const TextureRegionDesc*) {
}

static void NRI_CALL CmdClearStorage(CommandBuffer&, const ClearStorageDesc&) {
}

static void NRI_CALL CmdResetQueries(CommandBuffer&, QueryPool&, uint32_t, uint32_t) {
}

static void NRI_CALL CmdBeginQuery(CommandBuffer&, QueryPool&, uint32_t) {
}

static void NRI_CALL CmdEndQuery(CommandBuffer&, QueryPool&, uint32_t) {
}

static void NRI_CALL CmdCopyQueries(CommandBuffer&, const QueryPool&, uint32_t, uint32_t, Buffer&, uint64_t) {
}

static void NRI_CALL CmdBeginAnnotation(CommandBuffer&, const char*, uint32_t) {
}

static void NRI_CALL CmdEndAnnotation(CommandBuffer&) {
}

static void NRI_CALL CmdAnnotation(CommandBuffer&, const char*, uint32_t) {
}

static Result NRI_CALL EndCommandBuffer(CommandBuffer&) {
    return Result::SUCCESS;
}

static void NRI_CALL QueueBeginAnnotation(Queue&, const char*, uint32_t) {
}

static void NRI_CALL QueueEndAnnotation(Queue&) {
}

static void NRI_CALL QueueAnnotation(Queue&, const char*, uint32_t) {
}

static void NRI_CALL ResetQueries(QueryPool&, uint32_t, uint32_t) {
}

static Result NRI_CALL QueueSubmit(Queue&, const QueueSubmitDesc&) {
    return Result::SUCCESS;
}

static Result NRI_CALL DeviceWaitIdle(Device*) {
    return Result::SUCCESS;
}

static Result NRI_CALL QueueWaitIdle(Queue*) {
    return Result::SUCCESS;
}

static void NRI_CALL Wait(Fence&, uint64_t) {
}

static void NRI_CALL ResetCommandAllocator(CommandAllocator&) {
}

static void* NRI_CALL MapBuffer(Buffer&, uint64_t, uint64_t) {
    return nullptr;
}

static void NRI_CALL UnmapBuffer(Buffer&) {
}

static void NRI_CALL SetDebugName(Object*, const char*) {
}

static void* NRI_CALL GetDeviceNativeObject(const Device*) {
    return nullptr;
}

static void* NRI_CALL GetQueueNativeObject(const Queue*) {
    return nullptr;
}

static void* NRI_CALL GetCommandBufferNativeObject(const CommandBuffer*) {
    return nullptr;
}

static uint64_t NRI_CALL GetBufferNativeObject(const Buffer*) {
    return 0;
}

static uint64_t NRI_CALL GetTextureNativeObject(const Texture*) {
    return 0;
}

static uint64_t NRI_CALL GetDescriptorNativeObject(const Descriptor*) {
    return 0;
}

Result DeviceNONE::FillFunctionTable(CoreInterface& table) const {
    table.GetDeviceDesc = ::GetDeviceDesc;
    table.GetBufferDesc = ::GetBufferDesc;
    table.GetTextureDesc = ::GetTextureDesc;
    table.GetFormatSupport = ::GetFormatSupport;
    table.GetQuerySize = ::GetQuerySize;
    table.GetFenceValue = ::GetFenceValue;
    table.GetQueue = ::GetQueue;
    table.CreateCommandAllocator = ::CreateCommandAllocator;
    table.CreateCommandBuffer = ::CreateCommandBuffer;
    table.CreateDescriptorPool = ::CreateDescriptorPool;
    table.CreateBufferView = ::CreateBufferView;
    table.CreateTexture1DView = ::CreateTexture1DView;
    table.CreateTexture2DView = ::CreateTexture2DView;
    table.CreateTexture3DView = ::CreateTexture3DView;
    table.CreateSampler = ::CreateSampler;
    table.CreatePipelineLayout = ::CreatePipelineLayout;
    table.CreateGraphicsPipeline = ::CreateGraphicsPipeline;
    table.CreateComputePipeline = ::CreateComputePipeline;
    table.CreateQueryPool = ::CreateQueryPool;
    table.CreateFence = ::CreateFence;
    table.DestroyCommandAllocator = ::DestroyCommandAllocator;
    table.DestroyCommandBuffer = ::DestroyCommandBuffer;
    table.DestroyDescriptorPool = ::DestroyDescriptorPool;
    table.DestroyBuffer = ::DestroyBuffer;
    table.DestroyTexture = ::DestroyTexture;
    table.DestroyDescriptor = ::DestroyDescriptor;
    table.DestroyPipelineLayout = ::DestroyPipelineLayout;
    table.DestroyPipeline = ::DestroyPipeline;
    table.DestroyQueryPool = ::DestroyQueryPool;
    table.DestroyFence = ::DestroyFence;
    table.AllocateMemory = ::AllocateMemory;
    table.FreeMemory = ::FreeMemory;
    table.CreateBuffer = ::CreateBuffer;
    table.CreateTexture = ::CreateTexture;
    table.GetBufferMemoryDesc = ::GetBufferMemoryDesc;
    table.GetTextureMemoryDesc = ::GetTextureMemoryDesc;
    table.BindBufferMemory = ::BindBufferMemory;
    table.BindTextureMemory = ::BindTextureMemory;
    table.GetBufferMemoryDesc2 = ::GetBufferMemoryDesc2;
    table.GetTextureMemoryDesc2 = ::GetTextureMemoryDesc2;
    table.CreateCommittedBuffer = ::CreateCommittedBuffer;
    table.CreateCommittedTexture = ::CreateCommittedTexture;
    table.CreatePlacedBuffer = ::CreatePlacedBuffer;
    table.CreatePlacedTexture = ::CreatePlacedTexture;
    table.AllocateDescriptorSets = ::AllocateDescriptorSets;
    table.UpdateDescriptorRanges = ::UpdateDescriptorRanges;
    table.CopyDescriptorRanges = ::CopyDescriptorRanges;
    table.ResetDescriptorPool = ::ResetDescriptorPool;
    table.BeginCommandBuffer = ::BeginCommandBuffer;
    table.CmdSetDescriptorPool = ::CmdSetDescriptorPool;
    table.CmdSetDescriptorSet = ::CmdSetDescriptorSet;
    table.CmdSetPipelineLayout = ::CmdSetPipelineLayout;
    table.CmdSetPipeline = ::CmdSetPipeline;
    table.CmdSetRootConstants = ::CmdSetRootConstants;
    table.CmdSetRootDescriptor = ::CmdSetRootDescriptor;
    table.CmdBarrier = ::CmdBarrier;
    table.CmdSetIndexBuffer = ::CmdSetIndexBuffer;
    table.CmdSetVertexBuffers = ::CmdSetVertexBuffers;
    table.CmdSetViewports = ::CmdSetViewports;
    table.CmdSetScissors = ::CmdSetScissors;
    table.CmdSetStencilReference = ::CmdSetStencilReference;
    table.CmdSetDepthBounds = ::CmdSetDepthBounds;
    table.CmdSetBlendConstants = ::CmdSetBlendConstants;
    table.CmdSetSampleLocations = ::CmdSetSampleLocations;
    table.CmdSetShadingRate = ::CmdSetShadingRate;
    table.CmdSetDepthBias = ::CmdSetDepthBias;
    table.CmdBeginRendering = ::CmdBeginRendering;
    table.CmdClearAttachments = ::CmdClearAttachments;
    table.CmdDraw = ::CmdDraw;
    table.CmdDrawIndexed = ::CmdDrawIndexed;
    table.CmdDrawIndirect = ::CmdDrawIndirect;
    table.CmdDrawIndexedIndirect = ::CmdDrawIndexedIndirect;
    table.CmdEndRendering = ::CmdEndRendering;
    table.CmdDispatch = ::CmdDispatch;
    table.CmdDispatchIndirect = ::CmdDispatchIndirect;
    table.CmdCopyBuffer = ::CmdCopyBuffer;
    table.CmdCopyTexture = ::CmdCopyTexture;
    table.CmdUploadBufferToTexture = ::CmdUploadBufferToTexture;
    table.CmdReadbackTextureToBuffer = ::CmdReadbackTextureToBuffer;
    table.CmdZeroBuffer = ::CmdZeroBuffer;
    table.CmdResolveTexture = ::CmdResolveTexture;
    table.CmdClearStorage = ::CmdClearStorage;
    table.CmdResetQueries = ::CmdResetQueries;
    table.CmdBeginQuery = ::CmdBeginQuery;
    table.CmdEndQuery = ::CmdEndQuery;
    table.CmdCopyQueries = ::CmdCopyQueries;
    table.CmdBeginAnnotation = ::CmdBeginAnnotation;
    table.CmdEndAnnotation = ::CmdEndAnnotation;
    table.CmdAnnotation = ::CmdAnnotation;
    table.EndCommandBuffer = ::EndCommandBuffer;
    table.QueueBeginAnnotation = ::QueueBeginAnnotation;
    table.QueueEndAnnotation = ::QueueEndAnnotation;
    table.QueueAnnotation = ::QueueAnnotation;
    table.ResetQueries = ::ResetQueries;
    table.QueueSubmit = ::QueueSubmit;
    table.QueueWaitIdle = ::QueueWaitIdle;
    table.DeviceWaitIdle = ::DeviceWaitIdle;
    table.Wait = ::Wait;
    table.ResetCommandAllocator = ::ResetCommandAllocator;
    table.MapBuffer = ::MapBuffer;
    table.UnmapBuffer = ::UnmapBuffer;
    table.SetDebugName = ::SetDebugName;
    table.GetDeviceNativeObject = ::GetDeviceNativeObject;
    table.GetQueueNativeObject = ::GetQueueNativeObject;
    table.GetCommandBufferNativeObject = ::GetCommandBufferNativeObject;
    table.GetBufferNativeObject = ::GetBufferNativeObject;
    table.GetTextureNativeObject = ::GetTextureNativeObject;
    table.GetDescriptorNativeObject = ::GetDescriptorNativeObject;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Helper  ]

static uint32_t NRI_CALL CalculateAllocationNumber(const Device&, const ResourceGroupDesc&) {
    return 0;
}

static Result NRI_CALL AllocateAndBindMemory(Device&, const ResourceGroupDesc&, Memory**) {
    return Result::SUCCESS;
}

static Result NRI_CALL UploadData(Queue&, const TextureUploadDesc*, uint32_t, const BufferUploadDesc*, uint32_t) {
    return Result::SUCCESS;
}

static Result NRI_CALL QueryVideoMemoryInfo(const Device&, MemoryLocation, VideoMemoryInfo& videoMemoryInfo) {
    videoMemoryInfo = {};

    return Result::SUCCESS;
}

Result DeviceNONE::FillFunctionTable(HelperInterface& table) const {
    table.CalculateAllocationNumber = ::CalculateAllocationNumber;
    table.AllocateAndBindMemory = ::AllocateAndBindMemory;
    table.UploadData = ::UploadData;
    table.QueryVideoMemoryInfo = ::QueryVideoMemoryInfo;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Imgui  ]

#if NRI_ENABLE_IMGUI_EXTENSION

static Result NRI_CALL CreateImgui(Device&, const ImguiDesc&, Imgui*& imgui) {
    imgui = DummyObject<Imgui>();

    return Result::SUCCESS;
}

static void NRI_CALL DestroyImgui(Imgui*) {
}

static void NRI_CALL CmdCopyImguiData(CommandBuffer&, Streamer&, Imgui&, const CopyImguiDataDesc&) {
}

static void NRI_CALL CmdDrawImgui(CommandBuffer&, Imgui&, const DrawImguiDesc&) {
}

Result DeviceNONE::FillFunctionTable(ImguiInterface& table) const {
    table.CreateImgui = ::CreateImgui;
    table.DestroyImgui = ::DestroyImgui;
    table.CmdCopyImguiData = ::CmdCopyImguiData;
    table.CmdDrawImgui = ::CmdDrawImgui;

    return Result::SUCCESS;
}

#endif

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  LowLatency  ]

static Result NRI_CALL SetLatencySleepMode(SwapChain&, const LatencySleepMode&) {
    return Result::SUCCESS;
}

static Result NRI_CALL SetLatencyMarker(SwapChain&, LatencyMarker) {
    return Result::SUCCESS;
}

static Result NRI_CALL LatencySleep(SwapChain&) {
    return Result::SUCCESS;
}

static Result NRI_CALL GetLatencyReport(const SwapChain&, LatencyReport&) {
    return Result::SUCCESS;
}

Result DeviceNONE::FillFunctionTable(LowLatencyInterface& table) const {
    table.SetLatencySleepMode = ::SetLatencySleepMode;
    table.SetLatencyMarker = ::SetLatencyMarker;
    table.LatencySleep = ::LatencySleep;
    table.GetLatencyReport = ::GetLatencyReport;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  MeshShader  ]

static void NRI_CALL CmdDrawMeshTasks(CommandBuffer&, const DrawMeshTasksDesc&) {
}

static void NRI_CALL CmdDrawMeshTasksIndirect(CommandBuffer&, const Buffer&, uint64_t, uint32_t, uint32_t, const Buffer*, uint64_t) {
}

Result DeviceNONE::FillFunctionTable(MeshShaderInterface& table) const {
    table.CmdDrawMeshTasks = ::CmdDrawMeshTasks;
    table.CmdDrawMeshTasksIndirect = ::CmdDrawMeshTasksIndirect;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  RayTracing  ]

static Result NRI_CALL CreateRayTracingPipeline(Device&, const RayTracingPipelineDesc&, Pipeline*& pipeline) {
    pipeline = DummyObject<Pipeline>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateAccelerationStructureDescriptor(const AccelerationStructure&, Descriptor*& descriptor) {
    descriptor = DummyObject<Descriptor>();

    return Result::SUCCESS;
}

static uint64_t NRI_CALL GetAccelerationStructureHandle(const AccelerationStructure&) {
    return 0;
}

static uint64_t NRI_CALL GetAccelerationStructureUpdateScratchBufferSize(const AccelerationStructure&) {
    return 0;
}

static uint64_t NRI_CALL GetAccelerationStructureBuildScratchBufferSize(const AccelerationStructure&) {
    return 0;
}

static uint64_t NRI_CALL GetMicromapBuildScratchBufferSize(const Micromap&) {
    return 0;
}

static Buffer* NRI_CALL GetAccelerationStructureBuffer(const AccelerationStructure&) {
    return DummyObject<Buffer>();
}

static Buffer* NRI_CALL GetMicromapBuffer(const Micromap&) {
    return DummyObject<Buffer>();
}

static void NRI_CALL DestroyAccelerationStructure(AccelerationStructure*) {
}

static void NRI_CALL DestroyMicromap(Micromap*) {
}

static Result NRI_CALL CreateAccelerationStructure(Device&, const AccelerationStructureDesc&, AccelerationStructure*& accelerationStructure) {
    accelerationStructure = DummyObject<AccelerationStructure>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateMicromap(Device&, const MicromapDesc&, Micromap*& micromap) {
    micromap = DummyObject<Micromap>();

    return Result::SUCCESS;
}

static void NRI_CALL GetAccelerationStructureMemoryDesc(const AccelerationStructure&, MemoryLocation, MemoryDesc& memoryDesc) {
    memoryDesc = {1};
}

static void NRI_CALL GetMicromapMemoryDesc(const Micromap&, MemoryLocation, MemoryDesc& memoryDesc) {
    memoryDesc = {1};
}

static Result NRI_CALL BindAccelerationStructureMemory(const BindAccelerationStructureMemoryDesc*, uint32_t) {
    return Result::SUCCESS;
}

static Result NRI_CALL BindMicromapMemory(const BindMicromapMemoryDesc*, uint32_t) {
    return Result::SUCCESS;
}

static void NRI_CALL GetAccelerationStructureMemoryDesc2(const Device&, const AccelerationStructureDesc&, MemoryLocation, MemoryDesc& memoryDesc) {
    memoryDesc = {1};
}

static void NRI_CALL GetMicromapMemoryDesc2(const Device&, const MicromapDesc&, MemoryLocation, MemoryDesc& memoryDesc) {
    memoryDesc = {1};
}

static Result NRI_CALL CreateCommittedAccelerationStructure(Device&, MemoryLocation, float, const AccelerationStructureDesc&, AccelerationStructure*& accelerationStructure) {
    accelerationStructure = DummyObject<AccelerationStructure>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreateCommittedMicromap(Device&, MemoryLocation, float, const MicromapDesc&, Micromap*& micromap) {
    micromap = DummyObject<Micromap>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreatePlacedAccelerationStructure(Device&, Memory*, uint64_t, const AccelerationStructureDesc&, AccelerationStructure*& accelerationStructure) {
    accelerationStructure = DummyObject<AccelerationStructure>();

    return Result::SUCCESS;
}

static Result NRI_CALL CreatePlacedMicromap(Device&, Memory*, uint64_t, const MicromapDesc&, Micromap*& micromap) {
    micromap = DummyObject<Micromap>();

    return Result::SUCCESS;
}

static Result NRI_CALL WriteShaderGroupIdentifiers(const Pipeline&, uint32_t, uint32_t, void*) {
    return Result::SUCCESS;
}

static void NRI_CALL CmdBuildTopLevelAccelerationStructures(CommandBuffer&, const BuildTopLevelAccelerationStructureDesc*, uint32_t) {
}

static void NRI_CALL CmdBuildBottomLevelAccelerationStructures(CommandBuffer&, const BuildBottomLevelAccelerationStructureDesc*, uint32_t) {
}

static void NRI_CALL CmdBuildMicromaps(CommandBuffer&, const BuildMicromapDesc*, uint32_t) {
}

static void NRI_CALL CmdDispatchRays(CommandBuffer&, const DispatchRaysDesc&) {
}

static void NRI_CALL CmdDispatchRaysIndirect(CommandBuffer&, const Buffer&, uint64_t) {
}

static void NRI_CALL CmdWriteAccelerationStructuresSizes(CommandBuffer&, const AccelerationStructure* const*, uint32_t, QueryPool&, uint32_t) {
}

static void NRI_CALL CmdWriteMicromapsSizes(CommandBuffer&, const Micromap* const*, uint32_t, QueryPool&, uint32_t) {
}

static void NRI_CALL CmdCopyAccelerationStructure(CommandBuffer&, AccelerationStructure&, const AccelerationStructure&, CopyMode) {
}

static void NRI_CALL CmdCopyMicromap(CommandBuffer&, Micromap&, const Micromap&, CopyMode) {
}

static uint64_t NRI_CALL GetAccelerationStructureNativeObject(const AccelerationStructure*) {
    return 0;
}

static uint64_t NRI_CALL GetMicromapNativeObject(const Micromap*) {
    return 0;
}

Result DeviceNONE::FillFunctionTable(RayTracingInterface& table) const {
    table.CreateRayTracingPipeline = ::CreateRayTracingPipeline;
    table.CreateAccelerationStructureDescriptor = ::CreateAccelerationStructureDescriptor;
    table.GetAccelerationStructureHandle = ::GetAccelerationStructureHandle;
    table.GetAccelerationStructureUpdateScratchBufferSize = ::GetAccelerationStructureUpdateScratchBufferSize;
    table.GetAccelerationStructureBuildScratchBufferSize = ::GetAccelerationStructureBuildScratchBufferSize;
    table.GetMicromapBuildScratchBufferSize = ::GetMicromapBuildScratchBufferSize;
    table.GetAccelerationStructureBuffer = ::GetAccelerationStructureBuffer;
    table.GetMicromapBuffer = ::GetMicromapBuffer;
    table.DestroyAccelerationStructure = ::DestroyAccelerationStructure;
    table.DestroyMicromap = ::DestroyMicromap;
    table.CreateAccelerationStructure = ::CreateAccelerationStructure;
    table.CreateMicromap = ::CreateMicromap;
    table.GetAccelerationStructureMemoryDesc = ::GetAccelerationStructureMemoryDesc;
    table.GetMicromapMemoryDesc = ::GetMicromapMemoryDesc;
    table.BindAccelerationStructureMemory = ::BindAccelerationStructureMemory;
    table.BindMicromapMemory = ::BindMicromapMemory;
    table.GetAccelerationStructureMemoryDesc2 = ::GetAccelerationStructureMemoryDesc2;
    table.GetMicromapMemoryDesc2 = ::GetMicromapMemoryDesc2;
    table.CreateCommittedAccelerationStructure = ::CreateCommittedAccelerationStructure;
    table.CreateCommittedMicromap = ::CreateCommittedMicromap;
    table.CreatePlacedAccelerationStructure = ::CreatePlacedAccelerationStructure;
    table.CreatePlacedMicromap = ::CreatePlacedMicromap;
    table.WriteShaderGroupIdentifiers = ::WriteShaderGroupIdentifiers;
    table.CmdBuildTopLevelAccelerationStructures = ::CmdBuildTopLevelAccelerationStructures;
    table.CmdBuildBottomLevelAccelerationStructures = ::CmdBuildBottomLevelAccelerationStructures;
    table.CmdBuildMicromaps = ::CmdBuildMicromaps;
    table.CmdDispatchRays = ::CmdDispatchRays;
    table.CmdDispatchRaysIndirect = ::CmdDispatchRaysIndirect;
    table.CmdWriteAccelerationStructuresSizes = ::CmdWriteAccelerationStructuresSizes;
    table.CmdWriteMicromapsSizes = ::CmdWriteMicromapsSizes;
    table.CmdCopyAccelerationStructure = ::CmdCopyAccelerationStructure;
    table.CmdCopyMicromap = ::CmdCopyMicromap;
    table.GetAccelerationStructureNativeObject = ::GetAccelerationStructureNativeObject;
    table.GetMicromapNativeObject = ::GetMicromapNativeObject;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Streamer  ]

static Result NRI_CALL CreateStreamer(Device&, const StreamerDesc&, Streamer*& streamer) {
    streamer = DummyObject<Streamer>();

    return Result::SUCCESS;
}

static void NRI_CALL DestroyStreamer(Streamer*) {
}

static Buffer* NRI_CALL GetStreamerConstantBuffer(Streamer&) {
    return nullptr;
}

static uint32_t NRI_CALL StreamConstantData(Streamer&, const void*, uint32_t) {
    return 0;
}

static BufferOffset NRI_CALL StreamBufferData(Streamer&, const StreamBufferDataDesc&) {
    return {};
}

static BufferOffset NRI_CALL StreamTextureData(Streamer&, const StreamTextureDataDesc&) {
    return {};
}

static void NRI_CALL EndStreamerFrame(Streamer&) {
}

static void NRI_CALL CmdCopyStreamedData(CommandBuffer&, Streamer&) {
}

Result DeviceNONE::FillFunctionTable(StreamerInterface& table) const {
    table.CreateStreamer = ::CreateStreamer;
    table.DestroyStreamer = ::DestroyStreamer;
    table.GetStreamerConstantBuffer = ::GetStreamerConstantBuffer;
    table.StreamBufferData = ::StreamBufferData;
    table.StreamTextureData = ::StreamTextureData;
    table.StreamConstantData = ::StreamConstantData;
    table.EndStreamerFrame = ::EndStreamerFrame;
    table.CmdCopyStreamedData = ::CmdCopyStreamedData;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  SwapChain  ]

static Result NRI_CALL CreateSwapChain(Device&, const SwapChainDesc&, SwapChain*& swapChain) {
    swapChain = DummyObject<SwapChain>();

    return Result::SUCCESS;
}

static void NRI_CALL DestroySwapChain(SwapChain*) {
}

static Texture* const* NRI_CALL GetSwapChainTextures(const SwapChain&, uint32_t& textureNum) {
    static const void* textures[1] = {};
    textureNum = 1;

    return (Texture**)textures;
}

static Result NRI_CALL GetDisplayDesc(SwapChain&, DisplayDesc& displayDesc) {
    displayDesc = {};

    return Result::SUCCESS;
}

static Result NRI_CALL AcquireNextTexture(SwapChain&, Fence&, uint32_t& textureIndex) {
    textureIndex = 0;

    return Result::SUCCESS;
}

static Result NRI_CALL WaitForPresent(SwapChain&) {
    return Result::SUCCESS;
}

static Result NRI_CALL QueuePresent(SwapChain&, Fence&) {
    return Result::SUCCESS;
}

Result DeviceNONE::FillFunctionTable(SwapChainInterface& table) const {
    table.CreateSwapChain = ::CreateSwapChain;
    table.DestroySwapChain = ::DestroySwapChain;
    table.GetSwapChainTextures = ::GetSwapChainTextures;
    table.GetDisplayDesc = ::GetDisplayDesc;
    table.AcquireNextTexture = ::AcquireNextTexture;
    table.WaitForPresent = ::WaitForPresent;
    table.QueuePresent = ::QueuePresent;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Upscaler  ]

static Result NRI_CALL CreateUpscaler(Device&, const UpscalerDesc&, Upscaler*& upscaler) {
    upscaler = DummyObject<Upscaler>();

    return Result::SUCCESS;
}

static void NRI_CALL DestroyUpscaler(Upscaler*) {
}

static bool NRI_CALL IsUpscalerSupported(const Device&, UpscalerType) {
    return true;
}

static void NRI_CALL GetUpscalerProps(const Upscaler&, UpscalerProps& upscalerProps) {
    upscalerProps = {1.0f, 0.0f, {1, 1}, {1, 1}, {1, 1}, 1};
}

static void NRI_CALL CmdDispatchUpscale(CommandBuffer&, Upscaler&, const DispatchUpscaleDesc&) {
}

Result DeviceNONE::FillFunctionTable(UpscalerInterface& table) const {
    table.CreateUpscaler = ::CreateUpscaler;
    table.DestroyUpscaler = ::DestroyUpscaler;
    table.IsUpscalerSupported = ::IsUpscalerSupported;
    table.GetUpscalerProps = ::GetUpscalerProps;
    table.CmdDispatchUpscale = ::CmdDispatchUpscale;

    return Result::SUCCESS;
}

#pragma endregion
