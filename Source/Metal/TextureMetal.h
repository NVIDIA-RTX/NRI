// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct TextureMetal final : public DebugNameBase {
    inline TextureMetal(DeviceMetal& device) : m_Device(device) {
    }

    ~TextureMetal();

    inline DeviceMetal& GetDevice() const {
        return m_Device;
    }

    inline MTL::Texture* GetNativeObject() const {
        return m_Texture;
    }

    inline const TextureDesc& GetDesc() const {
        return m_Desc;
    }

    inline bool IsDrawable() const {
        return m_IsDrawable;
    }

    Result Create(const TextureDesc& desc);
    Result Create(const TextureDesc& desc, MemoryLocation location);
    Result Create(const TextureMetalDesc& desc);
    Result Bind(MemoryMetal& memory, uint64_t offset);
    void GetMemoryDesc(MemoryLocation location, MemoryDesc& memoryDesc) const;
    void SetDrawableTexture(MTL::Texture* texture, const TextureDesc& desc);
    MTL::Texture* GetDrawableView(MTL::PixelFormat format); // the current drawable texture, viewed as "format"
    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

    static bool IsAtomicFormat(const DeviceMetal& device, Format format);

private:
    struct DrawableView {
        MTL::PixelFormat format;
        MTL::Texture* view;
    };

    MTL::TextureDescriptor* NewNativeDesc(MemoryLocation location) const; // caller releases
    void Release();
    void ReleaseDrawableViews();

    DeviceMetal& m_Device;
    MTL::Texture* m_Texture = nullptr;
    TextureDesc m_Desc = {};
    bool m_IsResident = false;

    // Swap chain textures: views of the drawable texture, which is assigned to this slot. They stay valid while the layer reuses the drawable texture
    std::mutex m_DrawableViewLock;
    DrawableView m_DrawableViews[4] = {};
    const MTL::Texture* m_DrawableViewParent = nullptr; // identity only, retained by the views
    uint32_t m_DrawableViewNum = 0;
    bool m_IsDrawable = false;
};

} // namespace nri
