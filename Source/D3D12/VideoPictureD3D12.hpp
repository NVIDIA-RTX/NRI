// © 2026 NVIDIA Corporation

NRI_INLINE Result VideoPictureD3D12::Create(const VideoPictureDesc& videoPictureDesc) {
    if (!videoPictureDesc.texture)
        return Result::INVALID_ARGUMENT;

    m_Texture = (TextureD3D12*)videoPictureDesc.texture;

    const TextureDesc& textureDesc = m_Texture->GetDesc();
    m_Subresource = GetSubresourceIndex(videoPictureDesc.layer, textureDesc.layerNum, 0, textureDesc.mipNum, PlaneBits::COLOR);

    return Result::SUCCESS;
}
