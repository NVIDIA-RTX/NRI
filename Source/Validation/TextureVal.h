// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct MemoryVal;

struct TextureVal final : public ObjectVal {
    TextureVal(DeviceVal& device, Texture* texture, bool isBoundToMemory)
        : ObjectVal(device, texture)
        , m_IsBoundToMemory(isBoundToMemory) {
        m_Desc = GetCoreInterfaceImpl().GetTextureDesc(*texture);
    }

    ~TextureVal();

    inline Texture* GetImpl() const {
        return (Texture*)m_Impl;
    }

    inline const TextureDesc& GetDesc() const {
        return m_Desc;
    }

    inline uint64_t GetNativeObject() const {
        return GetCoreInterfaceImpl().GetTextureNativeObject(GetImpl());
    }

    inline bool IsBoundToMemory() const {
        return m_IsBoundToMemory;
    }

    inline void SetBoundToMemory(MemoryVal* memory) {
        m_Memory = memory;
        m_IsBoundToMemory = true;
    }

private:
    TextureDesc m_Desc = {}; // (only for) .natvis
    MemoryVal* m_Memory = nullptr;
    bool m_IsBoundToMemory = false;
};

} // namespace nri
