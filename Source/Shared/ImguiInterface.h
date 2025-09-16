// © 2025 NVIDIA Corporation

#pragma once

#if NRI_ENABLE_IMGUI_EXTENSION

namespace nri {

struct ImguiPipeline {
    Pipeline* pipeline = nullptr;
    Format format = Format::UNKNOWN;
};

struct ImguiTexture {
    Texture* texture = nullptr;
    Descriptor* descriptor = nullptr;
    uint64_t updateTick = 0;
};

struct ImguiImpl final : public DebugNameBase {
    inline ImguiImpl(Device& device, const CoreInterface& NRI)
        : m_Device(device)
        , m_iCore(NRI)
        , m_Textures(((DeviceBase&)device).GetStdAllocator())
        , m_Pipelines(((DeviceBase&)device).GetStdAllocator())
        , m_DescriptorSets1(((DeviceBase&)device).GetStdAllocator()) {
    }

    ~ImguiImpl();

    inline Device& GetDevice() {
        return m_Device;
    }

    Result Create(const ImguiDesc& imguiDesc);
    void CmdCopyData(CommandBuffer& commandBuffer, Streamer& streamer, const CopyImguiDataDesc& copyImguiDataDesc);
    void CmdDraw(CommandBuffer& commandBuffer, const DrawImguiDesc& drawImguiDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        m_iCore.SetDebugName(m_Sampler, name);
        m_iCore.SetDebugName(m_DescriptorPool, name);
        m_iCore.SetDebugName(m_PipelineLayout, name);
    }

private:
    Device& m_Device;
    const CoreInterface& m_iCore;
    StreamerInterface m_iStreamer = {};
    UnorderedMap<uint64_t, ImguiTexture> m_Textures;
    Vector<ImguiPipeline> m_Pipelines;
    Vector<DescriptorSet*> m_DescriptorSets1;
    Descriptor* m_Sampler = nullptr;
    DescriptorPool* m_DescriptorPool = nullptr;
    PipelineLayout* m_PipelineLayout = nullptr;
    DescriptorSet* m_DescriptorSet0_sampler = nullptr;
    Buffer* m_CurrentBuffer = nullptr;
    uint64_t m_VbOffset = 0;
    uint64_t m_IbOffset = 0;
    uint32_t m_DescriptorSetIndex = 0;
    Lock m_Lock;
};

} // namespace nri

#endif
