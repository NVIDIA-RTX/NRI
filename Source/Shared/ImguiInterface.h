// © 2025 NVIDIA Corporation

#pragma once

#if NRI_ENABLE_IMGUI_EXTENSION

namespace nri {

struct PipelineAndProps {
    Pipeline* pipeline;
    Format format;
    bool linearColor;
};

struct ImguiImpl : public DebugNameBase {
    inline ImguiImpl(Device& device, const CoreInterface& NRI)
        : m_Device(device)
        , m_iCore(NRI)
        , m_Pipelines(((DeviceBase&)device).GetStdAllocator())
        , m_DescriptorSets1(((DeviceBase&)device).GetStdAllocator()) {
    }

    ~ImguiImpl();

    inline Device& GetDevice() {
        return m_Device;
    }

    Result Create(const ImguiDesc& imguiDesc);
    void CmdDraw(CommandBuffer& commandBuffer, Streamer& streamer, const DrawImguiDesc& drawImguiDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        m_iCore.SetDebugName(m_FontTexture, name);
        m_iCore.SetDebugName(m_FontDescriptor, name);
        m_iCore.SetDebugName(m_Sampler, name);
        m_iCore.SetDebugName(m_DescriptorPool, name);
        m_iCore.SetDebugName(m_PipelineLayout, name);
    }

private:
    Device& m_Device;
    const CoreInterface& m_iCore;
    StreamerInterface m_iStreamer = {};
    Vector<PipelineAndProps> m_Pipelines;
    Vector<DescriptorSet*> m_DescriptorSets1;
    Texture* m_FontTexture = nullptr;
    Descriptor* m_FontDescriptor = nullptr;
    Descriptor* m_Sampler = nullptr;
    DescriptorPool* m_DescriptorPool = nullptr;
    PipelineLayout* m_PipelineLayout = nullptr;
    DescriptorSet* m_DescriptorSet0_sampler = nullptr;
    uint32_t m_DescriptorSetIndex = 0;
    Lock m_Lock;
};

} // namespace nri

#endif
