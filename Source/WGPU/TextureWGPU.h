// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct TextureWGPU final : public DebugNameBase {
    inline TextureWGPU(DeviceWGPU& device)
        : m_Device(device) {
    }

    ~TextureWGPU();

    inline operator WGPUTexture() const {
        return m_Texture;
    }

    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    inline const TextureDesc& GetDesc() const {
        return m_Desc;
    }

    inline uint64_t GetVersion() const {
        return m_Version;
    }

    inline bool IsSurfaceTexture() const {
        return m_IsSurfaceTexture;
    }

    Result Create(const TextureDesc& textureDesc);
    void SetSurfaceTexture(WGPUTexture texture, Format format, Dim_t width, Dim_t height);
    void DetachSurfaceTexture();

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        MaybeUnused(name);
    }

private:
    void Release();

private:
    DeviceWGPU& m_Device;
    WGPUTexture m_Texture = nullptr;
    TextureDesc m_Desc = {};
    uint64_t m_Version = 1;
    bool m_OwnsTexture = true;
    bool m_IsSurfaceTexture = false;
};

} // namespace nri
