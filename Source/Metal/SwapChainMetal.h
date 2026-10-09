// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct SwapChainMetal final : public DebugNameBase {
    inline SwapChainMetal(DeviceMetal& device)
        : m_Device(device)
        , m_Textures(device.GetStdAllocator())
        , m_DrawableTextures(device.GetStdAllocator()) {
    }

    ~SwapChainMetal();

    Result Create(const SwapChainDesc& desc);

    inline DeviceMetal& GetDevice() const {
        return m_Device;
    }

    Texture* const* GetTextures(uint32_t& textureNum) const;
    Result AcquireNextTexture(FenceMetal& fence, uint32_t& textureIndex);
    Result Present(FenceMetal& fence, uint64_t presentId);
    Result WaitForPresent(uint64_t presentId);
    Result GetDisplayDesc(DisplayDesc& desc);

private:
    void ReleaseDrawable();

    DeviceMetal& m_Device;
    QueueMetal* m_Queue = nullptr;
    CA::MetalLayer* m_Layer = nullptr;
    CA::MetalDrawable* m_Drawable = nullptr;
    MTL::ResidencySet* m_DrawableResidency = nullptr;
    MTL::SharedEvent* m_PresentEvent = nullptr; // the latest presented "presentId" (retained by presented handlers)
    Vector<TextureMetal*> m_Textures;
    Vector<const MTL::Texture*> m_DrawableTextures; // identity of the drawable texture last seen in each slot (not retained)
    TextureDesc m_TextureDesc = {};
    uint32_t m_TextureIndex = 0;
    uint32_t m_NextSlot = 0;
    bool m_IsWaitable = false;
};

} // namespace nri
