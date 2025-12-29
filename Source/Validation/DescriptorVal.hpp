// © 2021 NVIDIA Corporation

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, DescriptorType type)
    : ObjectVal(device, descriptor)
    , m_Type((DescriptorTypeExt)type) {
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, const BufferViewDesc& bufferViewDesc)
    : ObjectVal(device, descriptor) {
    switch (bufferViewDesc.viewType) {
        case BufferViewType::CONSTANT:
            m_Type = DescriptorTypeExt::CONSTANT_BUFFER;
            break;
        case BufferViewType::SHADER_RESOURCE:
            m_Type = bufferViewDesc.format == Format::UNKNOWN ? DescriptorTypeExt::STRUCTURED_BUFFER : DescriptorTypeExt::BUFFER;
            break;
        case BufferViewType::SHADER_RESOURCE_STORAGE:
            m_Type = bufferViewDesc.format == Format::UNKNOWN ? DescriptorTypeExt::STORAGE_STRUCTURED_BUFFER : DescriptorTypeExt::STORAGE_BUFFER;
            break;
        default:
            NRI_CHECK(false, "unexpected 'bufferViewDesc.viewType'");
            break;
    }
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture1DViewDesc& textureViewDesc)
    : ObjectVal(device, descriptor) {
    switch (textureViewDesc.viewType) {
        case Texture1DViewType::SHADER_RESOURCE:
        case Texture1DViewType::SHADER_RESOURCE_ARRAY:
            m_Type = DescriptorTypeExt::TEXTURE;
            break;
        case Texture1DViewType::SHADER_RESOURCE_STORAGE:
        case Texture1DViewType::SHADER_RESOURCE_STORAGE_ARRAY:
            m_Type = DescriptorTypeExt::STORAGE_TEXTURE;
            break;
        case Texture1DViewType::COLOR_ATTACHMENT:
            m_Type = DescriptorTypeExt::COLOR_ATTACHMENT;
            break;
        case Texture1DViewType::DEPTH_STENCIL_ATTACHMENT:
            m_Type = DescriptorTypeExt::DEPTH_STENCIL_ATTACHMENT;
            m_IsDepthReadonly = (textureViewDesc.readonlyPlanes & PlaneBits::DEPTH) != 0;
            m_IsStencilReadonly = (textureViewDesc.readonlyPlanes & PlaneBits::STENCIL) != 0;
            break;
        default:
            NRI_CHECK(false, "unexpected 'textureViewDesc.viewType'");
            break;
    }
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture2DViewDesc& textureViewDesc)
    : ObjectVal(device, descriptor) {
    switch (textureViewDesc.viewType) {
        case Texture2DViewType::SHADER_RESOURCE:
        case Texture2DViewType::SHADER_RESOURCE_ARRAY:
            m_Type = DescriptorTypeExt::TEXTURE;
            break;
        case Texture2DViewType::SHADER_RESOURCE_CUBE:
        case Texture2DViewType::SHADER_RESOURCE_CUBE_ARRAY:
            m_Type = DescriptorTypeExt::TEXTURE;
            break;
        case Texture2DViewType::SHADER_RESOURCE_STORAGE:
        case Texture2DViewType::SHADER_RESOURCE_STORAGE_ARRAY:
            m_Type = DescriptorTypeExt::STORAGE_TEXTURE;
            break;
        case Texture2DViewType::INPUT_ATTACHMENT:
            m_Type = DescriptorTypeExt::INPUT_ATTACHMENT;
            break;
        case Texture2DViewType::COLOR_ATTACHMENT:
            m_Type = DescriptorTypeExt::COLOR_ATTACHMENT;
            break;
        case Texture2DViewType::DEPTH_STENCIL_ATTACHMENT:
            m_Type = DescriptorTypeExt::DEPTH_STENCIL_ATTACHMENT;
            m_IsDepthReadonly = (textureViewDesc.readonlyPlanes & PlaneBits::DEPTH) != 0;
            m_IsStencilReadonly = (textureViewDesc.readonlyPlanes & PlaneBits::STENCIL) != 0;
            break;
        case Texture2DViewType::SHADING_RATE_ATTACHMENT:
            m_Type = DescriptorTypeExt::SHADING_RATE_ATTACHMENT;
            break;
        default:
            NRI_CHECK(false, "unexpected 'textureViewDesc.viewType'");
            break;
    }
}

DescriptorVal::DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture3DViewDesc& textureViewDesc)
    : ObjectVal(device, descriptor) {
    switch (textureViewDesc.viewType) {
        case Texture3DViewType::SHADER_RESOURCE:
            m_Type = DescriptorTypeExt::TEXTURE;
            break;
        case Texture3DViewType::SHADER_RESOURCE_STORAGE:
            m_Type = DescriptorTypeExt::STORAGE_TEXTURE;
            break;
        case Texture3DViewType::COLOR_ATTACHMENT:
            m_Type = DescriptorTypeExt::COLOR_ATTACHMENT;
            break;
        default:
            NRI_CHECK(false, "unexpected 'textureViewDesc.viewType'");
            break;
    }
}
