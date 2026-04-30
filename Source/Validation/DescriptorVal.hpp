// © 2021 NVIDIA Corporation

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, DescriptorType type)
    : ObjectVal(device, descriptor)
    , m_Type((DescriptorTypeExt)type) {
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, const BufferViewDesc& bufferViewDesc)
    : ObjectVal(device, descriptor) {
    switch (bufferViewDesc.type) {
        case BufferView::BUFFER:
            m_Type = DescriptorTypeExt::BUFFER;
            break;
        case BufferView::STRUCTURED_BUFFER:
        case BufferView::BYTE_ADDRESS_BUFFER:
            m_Type = DescriptorTypeExt::STRUCTURED_BUFFER;
            break;
        case BufferView::STORAGE_BUFFER:
            m_Type = DescriptorTypeExt::STORAGE_BUFFER;
            break;
        case BufferView::STORAGE_STRUCTURED_BUFFER:
        case BufferView::STORAGE_BYTE_ADDRESS_BUFFER:
            m_Type = DescriptorTypeExt::STORAGE_STRUCTURED_BUFFER;
            break;
        case BufferView::CONSTANT_BUFFER:
            m_Type = DescriptorTypeExt::CONSTANT_BUFFER;
            break;
        default:
            NRI_CHECK(false, "Unexpected 'bufferViewDesc.type'");
            break;
    }
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, const TextureViewDesc& textureViewDesc)
    : ObjectVal(device, descriptor) {
    switch (textureViewDesc.type) {
        case TextureView::TEXTURE:
        case TextureView::TEXTURE_ARRAY:
        case TextureView::TEXTURE_CUBE:
        case TextureView::TEXTURE_CUBE_ARRAY:
            m_Type = DescriptorTypeExt::TEXTURE;
            break;
        case TextureView::STORAGE_TEXTURE:
        case TextureView::STORAGE_TEXTURE_ARRAY:
            m_Type = DescriptorTypeExt::STORAGE_TEXTURE;
            break;
        case TextureView::SUBPASS_INPUT:
            m_Type = DescriptorTypeExt::INPUT_ATTACHMENT;
            break;
        case TextureView::COLOR_ATTACHMENT:
            m_Type = DescriptorTypeExt::COLOR_ATTACHMENT;
            break;
        case TextureView::DEPTH_STENCIL_ATTACHMENT:
            m_Type = DescriptorTypeExt::DEPTH_STENCIL_ATTACHMENT;
            m_IsDepthReadonly = textureViewDesc.planes != PlaneBits::ALL && (textureViewDesc.planes & PlaneBits::DEPTH) == 0;
            m_IsStencilReadonly = textureViewDesc.planes != PlaneBits::ALL && (textureViewDesc.planes & PlaneBits::STENCIL) == 0;
            break;
        case TextureView::SHADING_RATE_ATTACHMENT:
            m_Type = DescriptorTypeExt::SHADING_RATE_ATTACHMENT;
            break;
        default:
            NRI_CHECK(false, "Unexpected 'textureViewDesc.type'");
            break;
    }
}
