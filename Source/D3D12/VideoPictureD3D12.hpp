// © 2026 NVIDIA Corporation

NRI_INLINE Result VideoPictureD3D12::Create(const VideoPictureDesc& videoPictureDesc) {
        if (!videoPictureDesc.texture)
            return Result::INVALID_ARGUMENT;

        m_Texture = (TextureD3D12*)videoPictureDesc.texture;
        m_Subresource = videoPictureDesc.subresource;
        return Result::SUCCESS;
    }
