// © 2026 NVIDIA Corporation

static void FillLayoutEntry(WGPUBindGroupLayoutEntry& entry, DescriptorType descriptorType, WGPUShaderStage visibility, WGPUTextureSampleType textureSampleType, WGPUTextureViewDimension textureViewDimension, WGPUBool textureMultisampled, WGPUTextureFormat storageTextureFormat, WGPUTextureViewDimension storageTextureViewDimension, WGPUStorageTextureAccess storageTextureAccess, uint32_t binding, uint32_t bindingArraySize = 0) {
    entry = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
    entry.binding = binding;
    entry.visibility = visibility;
    entry.bindingArraySize = bindingArraySize;

    switch (descriptorType) {
        case DescriptorType::SAMPLER:
            entry.sampler.type = WGPUSamplerBindingType_Filtering;
            break;
        case DescriptorType::TEXTURE:
        case DescriptorType::INPUT_ATTACHMENT:
            // TODO: NRI input attachments are not native WebGPU subpass inputs; keep "shaderFeatures.inputAttachments = false".
            entry.texture.sampleType = textureSampleType;
            entry.texture.viewDimension = textureViewDimension;
            entry.texture.multisampled = textureMultisampled;
            break;
        case DescriptorType::STORAGE_TEXTURE:
            entry.storageTexture.access = storageTextureAccess;
            entry.storageTexture.format = storageTextureFormat == WGPUTextureFormat_Undefined ? WGPUTextureFormat_R32Float : storageTextureFormat;
            entry.storageTexture.viewDimension = storageTextureViewDimension;
            break;
        case DescriptorType::CONSTANT_BUFFER:
            entry.buffer.type = WGPUBufferBindingType_Uniform;
            break;
        case DescriptorType::STORAGE_BUFFER:
        case DescriptorType::STORAGE_STRUCTURED_BUFFER:
            entry.buffer.type = WGPUBufferBindingType_Storage;
            break;
        default:
            entry.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
            break;
    }
}

static void FillLayoutEntry(WGPUBindGroupLayoutEntry& entry, const DescriptorRangeMappingWGPU& range, uint32_t binding, uint32_t bindingArraySize = 0) {
    FillLayoutEntry(entry, range.type, range.visibility, range.textureSampleType, range.textureViewDimension, range.textureMultisampled, range.storageTextureFormat, range.storageTextureViewDimension, range.storageTextureAccess, binding, bindingArraySize);
}

static void FillLayoutEntry(WGPUBindGroupLayoutEntry& entry, const DescriptorRangeDesc& range, uint32_t binding, uint32_t bindingArraySize = 0) {
    FillLayoutEntry(entry, range.descriptorType, GetShaderStageFlags(range.shaderStages), WGPUTextureSampleType_Float, WGPUTextureViewDimension_2D, WGPU_FALSE, WGPUTextureFormat_Undefined, WGPUTextureViewDimension_2D, WGPUStorageTextureAccess_WriteOnly, binding, bindingArraySize);
}

static bool IsDynamicOffsetRootDescriptor(DescriptorType descriptorType) {
    return descriptorType == DescriptorType::CONSTANT_BUFFER || descriptorType == DescriptorType::BUFFER || descriptorType == DescriptorType::STORAGE_BUFFER || descriptorType == DescriptorType::STRUCTURED_BUFFER || descriptorType == DescriptorType::STORAGE_STRUCTURED_BUFFER;
}

static std::array<uint32_t, (size_t)DescriptorType::MAX_NUM> GetBindingOffsets(const DeviceWGPU& device, const PipelineLayoutDesc& pipelineLayoutDesc) {
    bool ignoreGlobalSPIRVOffsets = (pipelineLayoutDesc.flags & PipelineLayoutBits::IGNORE_GLOBAL_SPIRV_OFFSETS) != 0;

    VKBindingOffsets vkBindingOffsets = {};
    if (!ignoreGlobalSPIRVOffsets)
        vkBindingOffsets = device.GetBindingOffsets();

    std::array<uint32_t, (size_t)DescriptorType::MAX_NUM> bindingOffsets = {};
    bindingOffsets[(size_t)DescriptorType::SAMPLER] = vkBindingOffsets.sRegister;
    bindingOffsets[(size_t)DescriptorType::TEXTURE] = vkBindingOffsets.tRegister;
    bindingOffsets[(size_t)DescriptorType::STORAGE_TEXTURE] = vkBindingOffsets.uRegister;
    bindingOffsets[(size_t)DescriptorType::BUFFER] = vkBindingOffsets.tRegister;
    bindingOffsets[(size_t)DescriptorType::STORAGE_BUFFER] = vkBindingOffsets.uRegister;
    bindingOffsets[(size_t)DescriptorType::CONSTANT_BUFFER] = vkBindingOffsets.bRegister;
    bindingOffsets[(size_t)DescriptorType::STRUCTURED_BUFFER] = vkBindingOffsets.tRegister;
    bindingOffsets[(size_t)DescriptorType::STORAGE_STRUCTURED_BUFFER] = vkBindingOffsets.uRegister;
    bindingOffsets[(size_t)DescriptorType::ACCELERATION_STRUCTURE] = vkBindingOffsets.tRegister;

    return bindingOffsets;
}

PipelineLayoutWGPU::~PipelineLayoutWGPU() {
    if (m_RootSamplerBindGroup)
        wgpuBindGroupRelease(m_RootSamplerBindGroup);
    if (m_EmptyBindGroupLayout)
        wgpuBindGroupLayoutRelease(m_EmptyBindGroupLayout);

    for (RootSamplerMappingWGPU& rootSampler : m_RootSamplers) {
        if (rootSampler.sampler)
            wgpuSamplerRelease(rootSampler.sampler);
    }

    for (WGPUBindGroupLayout layout : m_BindGroupLayouts) {
        if (layout)
            wgpuBindGroupLayoutRelease(layout);
    }
}

const DescriptorSetMappingWGPU& PipelineLayoutWGPU::GetDescriptorSetMapping(uint32_t setIndex) const {
    return m_SetMappings[setIndex];
}

Result PipelineLayoutWGPU::Create(const PipelineLayoutDesc& pipelineLayoutDesc) {
    const auto bindingOffsets = GetBindingOffsets(m_Device, pipelineLayoutDesc);

    WGPUBindGroupLayoutDescriptor emptyLayoutDesc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    m_EmptyBindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_Device, &emptyLayoutDesc);
    if (!m_EmptyBindGroupLayout)
        return Result::FAILURE;

    m_ImmediateDataSize = 0;
    m_RootConstantOffsets.resize(pipelineLayoutDesc.rootConstantNum);
    for (uint32_t i = 0; i < pipelineLayoutDesc.rootConstantNum; i++) {
        m_RootConstantOffsets[i] = m_ImmediateDataSize;
        m_ImmediateDataSize += pipelineLayoutDesc.rootConstants[i].size;
    }

    uint32_t bindGroupNum = 0;
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++)
        bindGroupNum = std::max(bindGroupNum, pipelineLayoutDesc.descriptorSets[i].registerSpace + 1);
    if (pipelineLayoutDesc.rootSamplerNum || pipelineLayoutDesc.rootDescriptorNum)
        bindGroupNum = std::max(bindGroupNum, pipelineLayoutDesc.rootRegisterSpace + 1);

    m_BindGroupLayouts.resize(bindGroupNum);
    m_SetMappings.reserve(pipelineLayoutDesc.descriptorSetNum);
    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++)
        m_SetMappings.emplace_back(m_Device.GetStdAllocator());

    for (uint32_t i = 0; i < pipelineLayoutDesc.descriptorSetNum; i++) {
        const DescriptorSetDesc& set = pipelineLayoutDesc.descriptorSets[i];
        DescriptorSetMappingWGPU& mapping = m_SetMappings[i];
        mapping.ranges.resize(set.rangeNum);
        mapping.bindGroupIndex = set.registerSpace;

        uint32_t entryNum = 0;
        for (uint32_t j = 0; j < set.rangeNum; j++) {
            const DescriptorRangeDesc& range = set.ranges[j];
            entryNum += (range.flags & DescriptorRangeBits::ARRAY) ? 1 : range.descriptorNum;
        }

        Scratch<WGPUBindGroupLayoutEntry> entries = NRI_ALLOCATE_SCRATCH(m_Device, WGPUBindGroupLayoutEntry, entryNum);
        uint32_t entryOffset = 0;
        uint32_t descriptorOffset = 0;
        for (uint32_t j = 0; j < set.rangeNum; j++) {
            const DescriptorRangeDesc& range = set.ranges[j];
            DescriptorRangeMappingWGPU& rangeMapping = mapping.ranges[j];
            uint32_t bindingBase = range.baseRegisterIndex + bindingOffsets[(size_t)range.descriptorType];
            bool isArray = (range.flags & DescriptorRangeBits::ARRAY) != 0;
            rangeMapping.type = range.descriptorType;
            rangeMapping.descriptorOffset = descriptorOffset;
            rangeMapping.bindingBase = bindingBase;
            rangeMapping.descriptorNum = range.descriptorNum;
            rangeMapping.visibility = GetShaderStageFlags(range.shaderStages);
            rangeMapping.storageTextureFormat = range.descriptorType == DescriptorType::STORAGE_TEXTURE ? WGPUTextureFormat_R32Float : WGPUTextureFormat_Undefined;
            rangeMapping.isArray = isArray;

            if (isArray)
                FillLayoutEntry(entries[entryOffset++], rangeMapping, bindingBase, range.descriptorNum);
            else {
                for (uint32_t k = 0; k < range.descriptorNum; k++)
                    FillLayoutEntry(entries[entryOffset++], rangeMapping, bindingBase + k);
            }

            descriptorOffset += range.descriptorNum;
        }

        WGPUBindGroupLayoutDescriptor layoutDesc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
        layoutDesc.entryCount = entryNum;
        layoutDesc.entries = entries;

        mapping.layout = wgpuDeviceCreateBindGroupLayout(m_Device, &layoutDesc);
        if (!mapping.layout)
            return Result::FAILURE;

        m_BindGroupLayouts[set.registerSpace] = mapping.layout;
    }

    if (pipelineLayoutDesc.rootSamplerNum || pipelineLayoutDesc.rootDescriptorNum) {
        uint32_t rootEntryNum = pipelineLayoutDesc.rootSamplerNum + pipelineLayoutDesc.rootDescriptorNum;
        Scratch<WGPUBindGroupLayoutEntry> entries = NRI_ALLOCATE_SCRATCH(m_Device, WGPUBindGroupLayoutEntry, rootEntryNum);
        Scratch<WGPUBindGroupEntry> bindGroupEntries = NRI_ALLOCATE_SCRATCH(m_Device, WGPUBindGroupEntry, pipelineLayoutDesc.rootSamplerNum);

        m_RootSamplers.reserve(pipelineLayoutDesc.rootSamplerNum);
        m_RootDescriptors.reserve(pipelineLayoutDesc.rootDescriptorNum);

        for (uint32_t i = 0; i < pipelineLayoutDesc.rootSamplerNum; i++) {
            const RootSamplerDesc& rootSampler = pipelineLayoutDesc.rootSamplers[i];
            uint32_t binding = rootSampler.registerIndex + bindingOffsets[(size_t)DescriptorType::SAMPLER];

            DescriptorRangeDesc range = {};
            range.baseRegisterIndex = binding;
            range.descriptorNum = 1;
            range.descriptorType = DescriptorType::SAMPLER;
            range.shaderStages = rootSampler.shaderStages;
            FillLayoutEntry(entries[i], range, binding);
            if (rootSampler.desc.compareOp != CompareOp::NONE)
                entries[i].sampler.type = WGPUSamplerBindingType_Comparison;

            WGPUSamplerDescriptor samplerDesc = WGPU_SAMPLER_DESCRIPTOR_INIT;
            samplerDesc.addressModeU = GetAddressMode(rootSampler.desc.addressModes.u);
            samplerDesc.addressModeV = GetAddressMode(rootSampler.desc.addressModes.v);
            samplerDesc.addressModeW = GetAddressMode(rootSampler.desc.addressModes.w);
            samplerDesc.magFilter = GetFilterMode(rootSampler.desc.filters.mag);
            samplerDesc.minFilter = GetFilterMode(rootSampler.desc.filters.min);
            samplerDesc.mipmapFilter = GetMipmapFilterMode(rootSampler.desc.filters.mip);
            samplerDesc.lodMinClamp = rootSampler.desc.mipMin;
            samplerDesc.lodMaxClamp = rootSampler.desc.mipMax == 0.0f ? 1000.0f : rootSampler.desc.mipMax;
            samplerDesc.maxAnisotropy = std::max<uint16_t>(rootSampler.desc.anisotropy, 1);
            WGPUSampler sampler = wgpuDeviceCreateSampler(m_Device, &samplerDesc);
            if (!sampler)
                return Result::FAILURE;

            m_RootSamplers.push_back({sampler, GetShaderStageFlags(rootSampler.shaderStages), binding});

            bindGroupEntries[i] = WGPU_BIND_GROUP_ENTRY_INIT;
            bindGroupEntries[i].binding = binding;
            bindGroupEntries[i].sampler = sampler;
        }

        for (uint32_t i = 0; i < pipelineLayoutDesc.rootDescriptorNum; i++) {
            const RootDescriptorDesc& rootDescriptor = pipelineLayoutDesc.rootDescriptors[i];
            uint32_t binding = rootDescriptor.registerIndex + bindingOffsets[(size_t)rootDescriptor.descriptorType];
            bool hasDynamicOffset = IsDynamicOffsetRootDescriptor(rootDescriptor.descriptorType);

            DescriptorRangeDesc range = {};
            range.baseRegisterIndex = binding;
            range.descriptorNum = 1;
            range.descriptorType = rootDescriptor.descriptorType;
            range.shaderStages = rootDescriptor.shaderStages;
            WGPUBindGroupLayoutEntry& entry = entries[pipelineLayoutDesc.rootSamplerNum + i];
            FillLayoutEntry(entry, range, binding);
            entry.buffer.hasDynamicOffset = hasDynamicOffset ? WGPU_TRUE : WGPU_FALSE;

            m_RootDescriptors.push_back({GetShaderStageFlags(rootDescriptor.shaderStages), binding, uint32_t(-1), rootDescriptor.descriptorType});
        }

        for (;;) {
            uint32_t selected = uint32_t(-1);
            uint32_t selectedBinding = uint32_t(-1);
            for (uint32_t i = 0; i < (uint32_t)m_RootDescriptors.size(); i++) {
                RootDescriptorMappingWGPU& rootDescriptor = m_RootDescriptors[i];
                if (rootDescriptor.dynamicOffsetIndex == uint32_t(-1) && IsDynamicOffsetRootDescriptor(rootDescriptor.type) && rootDescriptor.binding < selectedBinding) {
                    selected = i;
                    selectedBinding = rootDescriptor.binding;
                }
            }

            if (selected == uint32_t(-1))
                break;

            m_RootDescriptors[selected].dynamicOffsetIndex = m_RootDynamicOffsetNum++;
        }

        WGPUBindGroupLayoutDescriptor layoutDesc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
        layoutDesc.entryCount = rootEntryNum;
        layoutDesc.entries = entries;

        m_RootSamplerLayout = wgpuDeviceCreateBindGroupLayout(m_Device, &layoutDesc);
        if (!m_RootSamplerLayout)
            return Result::FAILURE;

        if (!pipelineLayoutDesc.rootDescriptorNum) {
            WGPUBindGroupDescriptor bindGroupDesc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
            bindGroupDesc.layout = m_RootSamplerLayout;
            bindGroupDesc.entryCount = pipelineLayoutDesc.rootSamplerNum;
            bindGroupDesc.entries = bindGroupEntries;
            m_RootSamplerBindGroup = wgpuDeviceCreateBindGroup(m_Device, &bindGroupDesc);
            if (!m_RootSamplerBindGroup)
                return Result::FAILURE;
        }

        m_RootSamplerGroupIndex = pipelineLayoutDesc.rootRegisterSpace;
        m_BindGroupLayouts[m_RootSamplerGroupIndex] = m_RootSamplerLayout;
    }

    for (WGPUBindGroupLayout& layout : m_BindGroupLayouts) {
        if (!layout) {
            WGPUBindGroupLayoutDescriptor layoutDesc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
            layout = wgpuDeviceCreateBindGroupLayout(m_Device, &layoutDesc);
            if (!layout)
                return Result::FAILURE;
        }
    }

    return Result::SUCCESS;
}

static WGPUTextureFormat GetStorageTextureFormatFromSpirv(uint32_t imageFormat) {
    switch (imageFormat) {
        case 1:
            return WGPUTextureFormat_RGBA32Float;
        case 2:
            return WGPUTextureFormat_RGBA16Float;
        case 3:
            return WGPUTextureFormat_R32Float;
        case 4:
            return WGPUTextureFormat_RGBA8Unorm;
        case 5:
            return WGPUTextureFormat_RGBA8Snorm;
        case 6:
            return WGPUTextureFormat_RG32Float;
        case 7:
            return WGPUTextureFormat_RG16Float;
        case 9:
            return WGPUTextureFormat_R16Float;
        case 21:
            return WGPUTextureFormat_RGBA32Sint;
        case 22:
            return WGPUTextureFormat_RGBA16Sint;
        case 23:
            return WGPUTextureFormat_RGBA8Sint;
        case 24:
            return WGPUTextureFormat_R32Sint;
        case 25:
            return WGPUTextureFormat_RG32Sint;
        case 26:
            return WGPUTextureFormat_RG16Sint;
        case 27:
            return WGPUTextureFormat_RG8Sint;
        case 28:
            return WGPUTextureFormat_R16Sint;
        case 29:
            return WGPUTextureFormat_R8Sint;
        case 30:
            return WGPUTextureFormat_RGBA32Uint;
        case 31:
            return WGPUTextureFormat_RGBA16Uint;
        case 32:
            return WGPUTextureFormat_RGBA8Uint;
        case 33:
            return WGPUTextureFormat_R32Uint;
        case 35:
            return WGPUTextureFormat_RG32Uint;
        case 36:
            return WGPUTextureFormat_RG16Uint;
        case 37:
            return WGPUTextureFormat_RG8Uint;
        case 38:
            return WGPUTextureFormat_R16Uint;
        case 39:
            return WGPUTextureFormat_R8Uint;
        default:
            return WGPUTextureFormat_Undefined;
    }
}

struct TextureBindingWGPU {
    uint32_t set = 0;
    uint32_t binding = 0;
    DescriptorType type = DescriptorType::TEXTURE;
    WGPUTextureSampleType sampleType = WGPUTextureSampleType_Float;
    WGPUTextureViewDimension viewDimension = WGPUTextureViewDimension_2D;
    WGPUBool multisampled = WGPU_FALSE;
    WGPUTextureFormat format = WGPUTextureFormat_Undefined;
    WGPUStorageTextureAccess access = WGPUStorageTextureAccess_WriteOnly;
};

static WGPUTextureViewDimension GetTextureViewDimensionFromSpirv(uint32_t dim, uint32_t arrayed) {
    if (dim == 0)
        return WGPUTextureViewDimension_1D;
    if (dim == 2)
        return WGPUTextureViewDimension_3D;
    if (dim == 3)
        return arrayed ? WGPUTextureViewDimension_CubeArray : WGPUTextureViewDimension_Cube;

    return arrayed ? WGPUTextureViewDimension_2DArray : WGPUTextureViewDimension_2D;
}

static WGPUTextureViewDimension GetStorageTextureViewDimensionFromSpirv(uint32_t dim, uint32_t arrayed) {
    if (dim == 0)
        return WGPUTextureViewDimension_1D;
    if (dim == 2)
        return WGPUTextureViewDimension_3D;

    return arrayed ? WGPUTextureViewDimension_2DArray : WGPUTextureViewDimension_2D;
}

static const char* FindTokenWGPU(const char* begin, const char* end, const char* token) {
    size_t tokenLength = strlen(token);
    if (!tokenLength || end - begin < (ptrdiff_t)tokenLength)
        return nullptr;

    for (const char* it = begin; it <= end - tokenLength; it++) {
        if (strncmp(it, token, tokenLength) == 0)
            return it;
    }

    return nullptr;
}

static const char* FindTokenReverseWGPU(const char* begin, const char* end, const char* token) {
    size_t tokenLength = strlen(token);
    if (!tokenLength || end - begin < (ptrdiff_t)tokenLength)
        return nullptr;

    for (const char* it = end - tokenLength; it >= begin; it--) {
        if (strncmp(it, token, tokenLength) == 0)
            return it;
        if (it == begin)
            break;
    }

    return nullptr;
}

static const char* SkipSpacesWGPU(const char* it, const char* end) {
    while (it < end && (*it == ' ' || *it == '\t' || *it == '\r' || *it == '\n'))
        it++;

    return it;
}

static bool ParseUintAttributeWGPU(const char* searchBegin, const char* searchEnd, const char* attribute, uint32_t& value) {
    const char* it = FindTokenReverseWGPU(searchBegin, searchEnd, attribute);
    if (!it)
        return false;

    it += strlen(attribute);
    it = SkipSpacesWGPU(it, searchEnd);
    if (it == searchEnd || *it != '(')
        return false;

    it = SkipSpacesWGPU(it + 1, searchEnd);
    if (it == searchEnd || *it < '0' || *it > '9')
        return false;

    value = 0;
    while (it < searchEnd && *it >= '0' && *it <= '9')
        value = value * 10 + uint32_t(*it++ - '0');

    return true;
}

static bool IsWgslIdentifierCharWGPU(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static bool IsWgslTokenWGPU(const char* begin, const char* end, const char* token) {
    size_t tokenLength = strlen(token);
    return end - begin == (ptrdiff_t)tokenLength && strncmp(begin, token, tokenLength) == 0;
}

static WGPUTextureFormat GetStorageTextureFormatFromWgsl(const char* begin, const char* end) {
    if (IsWgslTokenWGPU(begin, end, "rgba32float"))
        return WGPUTextureFormat_RGBA32Float;
    if (IsWgslTokenWGPU(begin, end, "rgba16float"))
        return WGPUTextureFormat_RGBA16Float;
    if (IsWgslTokenWGPU(begin, end, "r32float"))
        return WGPUTextureFormat_R32Float;
    if (IsWgslTokenWGPU(begin, end, "rgba8unorm"))
        return WGPUTextureFormat_RGBA8Unorm;
    if (IsWgslTokenWGPU(begin, end, "bgra8unorm"))
        return WGPUTextureFormat_BGRA8Unorm;
    if (IsWgslTokenWGPU(begin, end, "rgba8snorm"))
        return WGPUTextureFormat_RGBA8Snorm;
    if (IsWgslTokenWGPU(begin, end, "rg32float"))
        return WGPUTextureFormat_RG32Float;
    if (IsWgslTokenWGPU(begin, end, "rg16float"))
        return WGPUTextureFormat_RG16Float;
    if (IsWgslTokenWGPU(begin, end, "r16float"))
        return WGPUTextureFormat_R16Float;
    if (IsWgslTokenWGPU(begin, end, "rgba32sint"))
        return WGPUTextureFormat_RGBA32Sint;
    if (IsWgslTokenWGPU(begin, end, "rgba16sint"))
        return WGPUTextureFormat_RGBA16Sint;
    if (IsWgslTokenWGPU(begin, end, "rgba8sint"))
        return WGPUTextureFormat_RGBA8Sint;
    if (IsWgslTokenWGPU(begin, end, "r32sint"))
        return WGPUTextureFormat_R32Sint;
    if (IsWgslTokenWGPU(begin, end, "rg32sint"))
        return WGPUTextureFormat_RG32Sint;
    if (IsWgslTokenWGPU(begin, end, "rg16sint"))
        return WGPUTextureFormat_RG16Sint;
    if (IsWgslTokenWGPU(begin, end, "rg8sint"))
        return WGPUTextureFormat_RG8Sint;
    if (IsWgslTokenWGPU(begin, end, "r16sint"))
        return WGPUTextureFormat_R16Sint;
    if (IsWgslTokenWGPU(begin, end, "r8sint"))
        return WGPUTextureFormat_R8Sint;
    if (IsWgslTokenWGPU(begin, end, "rgba32uint"))
        return WGPUTextureFormat_RGBA32Uint;
    if (IsWgslTokenWGPU(begin, end, "rgba16uint"))
        return WGPUTextureFormat_RGBA16Uint;
    if (IsWgslTokenWGPU(begin, end, "rgba8uint"))
        return WGPUTextureFormat_RGBA8Uint;
    if (IsWgslTokenWGPU(begin, end, "r32uint"))
        return WGPUTextureFormat_R32Uint;
    if (IsWgslTokenWGPU(begin, end, "rg32uint"))
        return WGPUTextureFormat_RG32Uint;
    if (IsWgslTokenWGPU(begin, end, "rg16uint"))
        return WGPUTextureFormat_RG16Uint;
    if (IsWgslTokenWGPU(begin, end, "rg8uint"))
        return WGPUTextureFormat_RG8Uint;
    if (IsWgslTokenWGPU(begin, end, "r16uint"))
        return WGPUTextureFormat_R16Uint;
    if (IsWgslTokenWGPU(begin, end, "r8uint"))
        return WGPUTextureFormat_R8Uint;

    return WGPUTextureFormat_Undefined;
}

static WGPUTextureViewDimension GetStorageTextureViewDimensionFromWgsl(const char* begin, const char* end) {
    if (IsWgslTokenWGPU(begin, end, "1d"))
        return WGPUTextureViewDimension_1D;
    if (IsWgslTokenWGPU(begin, end, "3d"))
        return WGPUTextureViewDimension_3D;
    if (IsWgslTokenWGPU(begin, end, "2d_array"))
        return WGPUTextureViewDimension_2DArray;

    return WGPUTextureViewDimension_2D;
}

static WGPUStorageTextureAccess GetStorageTextureAccessFromWgsl(const char* begin, const char* end) {
    if (IsWgslTokenWGPU(begin, end, "read"))
        return WGPUStorageTextureAccess_ReadOnly;
    if (IsWgslTokenWGPU(begin, end, "read_write"))
        return WGPUStorageTextureAccess_ReadWrite;

    return WGPUStorageTextureAccess_WriteOnly;
}

static WGPUTextureSampleType GetTextureSampleTypeFromWgsl(const char* begin, const char* end) {
    if (IsWgslTokenWGPU(begin, end, "i32"))
        return WGPUTextureSampleType_Sint;
    if (IsWgslTokenWGPU(begin, end, "u32"))
        return WGPUTextureSampleType_Uint;

    return WGPUTextureSampleType_Float;
}

static WGPUTextureViewDimension GetTextureViewDimensionFromWgsl(const char* begin, const char* end) {
    if (IsWgslTokenWGPU(begin, end, "1d"))
        return WGPUTextureViewDimension_1D;
    if (IsWgslTokenWGPU(begin, end, "2d_array") || IsWgslTokenWGPU(begin, end, "depth_2d_array"))
        return WGPUTextureViewDimension_2DArray;
    if (IsWgslTokenWGPU(begin, end, "3d"))
        return WGPUTextureViewDimension_3D;
    if (IsWgslTokenWGPU(begin, end, "cube"))
        return WGPUTextureViewDimension_Cube;
    if (IsWgslTokenWGPU(begin, end, "cube_array"))
        return WGPUTextureViewDimension_CubeArray;

    return WGPUTextureViewDimension_2D;
}

static bool IsDepthTextureTypeWgsl(const char* begin, const char* end) {
    return end - begin > 6 && strncmp(begin, "depth_", 6) == 0;
}

static bool IsMultisampledTextureTypeWgsl(const char* begin, const char* end) {
    return IsWgslTokenWGPU(begin, end, "multisampled_2d") || IsWgslTokenWGPU(begin, end, "depth_multisampled_2d");
}

static void AddTextureBindingWGPU(TextureBindingWGPU* textureBindings, uint32_t& textureBindingNum, uint32_t textureBindingMaxNum, const TextureBindingWGPU& textureBinding) {
    if (textureBindingNum < textureBindingMaxNum)
        textureBindings[textureBindingNum++] = textureBinding;
}

static void ReflectTexturesFromWgsl(const ShaderDesc& shaderDesc, TextureBindingWGPU* textureBindings, uint32_t& textureBindingNum, uint32_t textureBindingMaxNum) {
    // TODO: This is a small source parser, not full WGSL reflection. It intentionally recognizes only direct texture declarations.
    const char* source = (const char*)shaderDesc.bytecode;
    if (!source || shaderDesc.size == 0)
        return;

    const char* sourceEnd = source + shaderDesc.size;
    const char* storageToken = "texture_storage_";
    const char* it = source;
    while ((it = FindTokenWGPU(it, sourceEnd, storageToken)) != nullptr) {
        const char* declarationBegin = it - source > 512 ? it - 512 : source;
        uint32_t set = 0;
        uint32_t binding = 0;
        if (!ParseUintAttributeWGPU(declarationBegin, it, "@group", set) || !ParseUintAttributeWGPU(declarationBegin, it, "@binding", binding)) {
            it += strlen(storageToken);
            continue;
        }

        const char* dimensionBegin = it + strlen(storageToken);
        const char* dimensionEnd = dimensionBegin;
        while (dimensionEnd < sourceEnd && IsWgslIdentifierCharWGPU(*dimensionEnd))
            dimensionEnd++;

        const char* declarationEnd = FindTokenWGPU(dimensionEnd, sourceEnd, ";");
        if (!declarationEnd)
            declarationEnd = sourceEnd;

        const char* formatBegin = FindTokenWGPU(dimensionEnd, declarationEnd, "<");
        if (!formatBegin) {
            it = dimensionEnd;
            continue;
        }

        formatBegin = SkipSpacesWGPU(formatBegin + 1, declarationEnd);
        const char* formatEnd = formatBegin;
        while (formatEnd < declarationEnd && IsWgslIdentifierCharWGPU(*formatEnd))
            formatEnd++;

        WGPUTextureFormat format = GetStorageTextureFormatFromWgsl(formatBegin, formatEnd);
        if (format != WGPUTextureFormat_Undefined) {
            WGPUStorageTextureAccess access = WGPUStorageTextureAccess_WriteOnly;
            const char* accessBegin = FindTokenWGPU(formatEnd, declarationEnd, ",");
            if (accessBegin) {
                accessBegin = SkipSpacesWGPU(accessBegin + 1, declarationEnd);
                const char* accessEnd = accessBegin;
                while (accessEnd < declarationEnd && IsWgslIdentifierCharWGPU(*accessEnd))
                    accessEnd++;

                access = GetStorageTextureAccessFromWgsl(accessBegin, accessEnd);
            }

            TextureBindingWGPU textureBinding = {};
            textureBinding.set = set;
            textureBinding.binding = binding;
            textureBinding.type = DescriptorType::STORAGE_TEXTURE;
            textureBinding.format = format;
            textureBinding.viewDimension = GetStorageTextureViewDimensionFromWgsl(dimensionBegin, dimensionEnd);
            textureBinding.access = access;
            AddTextureBindingWGPU(textureBindings, textureBindingNum, textureBindingMaxNum, textureBinding);
        }

        it = formatEnd;
    }

    const char* textureToken = "texture_";
    it = source;
    while ((it = FindTokenWGPU(it, sourceEnd, textureToken)) != nullptr) {
        if (FindTokenWGPU(it, std::min(it + strlen(storageToken), sourceEnd), storageToken) == it) {
            it += strlen(storageToken);
            continue;
        }

        const char* typeBegin = it + strlen(textureToken);
        const char* typeEnd = typeBegin;
        while (typeEnd < sourceEnd && IsWgslIdentifierCharWGPU(*typeEnd))
            typeEnd++;

        const char* declarationBegin = it - source > 512 ? it - 512 : source;
        uint32_t set = 0;
        uint32_t binding = 0;
        if (!ParseUintAttributeWGPU(declarationBegin, it, "@group", set) || !ParseUintAttributeWGPU(declarationBegin, it, "@binding", binding)) {
            it = typeEnd;
            continue;
        }

        WGPUTextureSampleType sampleType = IsDepthTextureTypeWgsl(typeBegin, typeEnd) ? WGPUTextureSampleType_Depth : WGPUTextureSampleType_Float;
        const char* declarationEnd = FindTokenWGPU(typeEnd, sourceEnd, ";");
        if (!declarationEnd)
            declarationEnd = sourceEnd;

        const char* sampleTypeBegin = FindTokenWGPU(typeEnd, declarationEnd, "<");
        if (sampleTypeBegin && sampleType != WGPUTextureSampleType_Depth) {
            sampleTypeBegin = SkipSpacesWGPU(sampleTypeBegin + 1, declarationEnd);
            const char* sampleTypeEnd = sampleTypeBegin;
            while (sampleTypeEnd < declarationEnd && IsWgslIdentifierCharWGPU(*sampleTypeEnd))
                sampleTypeEnd++;

            sampleType = GetTextureSampleTypeFromWgsl(sampleTypeBegin, sampleTypeEnd);
        }

        TextureBindingWGPU textureBinding = {};
        textureBinding.set = set;
        textureBinding.binding = binding;
        textureBinding.type = DescriptorType::TEXTURE;
        textureBinding.sampleType = sampleType;
        textureBinding.viewDimension = GetTextureViewDimensionFromWgsl(typeBegin, typeEnd);
        textureBinding.multisampled = IsMultisampledTextureTypeWgsl(typeBegin, typeEnd) ? WGPU_TRUE : WGPU_FALSE;
        AddTextureBindingWGPU(textureBindings, textureBindingNum, textureBindingMaxNum, textureBinding);

        it = typeEnd;
    }
}

static WGPUTextureSampleType GetTextureSampleTypeFromSpirv(const uint32_t* scalarTypes, uint32_t sampledType, uint32_t depth, uint32_t idBound) {
    if (depth == 1)
        return WGPUTextureSampleType_Depth;

    if (sampledType < idBound) {
        uint32_t scalarType = scalarTypes[sampledType];
        if (scalarType == 1)
            return WGPUTextureSampleType_Sint;
        if (scalarType == 2)
            return WGPUTextureSampleType_Uint;
    }

    return WGPUTextureSampleType_Float;
}

static WGPUStorageTextureAccess GetStorageTextureAccessFromSpirv(uint32_t accessQualifier) {
    if (accessQualifier == 0)
        return WGPUStorageTextureAccess_ReadOnly;
    if (accessQualifier == 2)
        return WGPUStorageTextureAccess_ReadWrite;

    return WGPUStorageTextureAccess_WriteOnly;
}

static uint32_t GetSourceVariable(const uint32_t* ids, uint32_t id, uint32_t idBound) {
    if (id >= idBound)
        return uint32_t(-1);

    return ids[id] == uint32_t(-1) ? id : ids[id];
}

static void ReflectTextures(DeviceWGPU& device, const ShaderDesc& shaderDesc, TextureBindingWGPU* textureBindings, uint32_t& textureBindingNum, uint32_t textureBindingMaxNum) {
    const uint32_t* code = (const uint32_t*)shaderDesc.bytecode;
    uint32_t wordNum = (uint32_t)(shaderDesc.size / sizeof(uint32_t));
    if (!code || wordNum < 5 || code[0] != 0x07230203) {
        ReflectTexturesFromWgsl(shaderDesc, textureBindings, textureBindingNum, textureBindingMaxNum);
        return;
    }

    // TODO: SPIR-V reflection is intentionally minimal and extracts only texture binding metadata needed by WGPU bind-group layouts.
    struct TypeInfo {
        DescriptorType type = DescriptorType::MAX_NUM;
        uint32_t imageFormat = 0;
        WGPUTextureSampleType sampleType = WGPUTextureSampleType_Float;
        WGPUTextureViewDimension viewDimension = WGPUTextureViewDimension_2D;
        WGPUBool multisampled = WGPU_FALSE;
        WGPUStorageTextureAccess access = WGPUStorageTextureAccess_Undefined;
    };

    uint32_t idBound = code[3];
    Scratch<uint32_t> scalarTypes = NRI_ALLOCATE_SCRATCH(device, uint32_t, idBound);
    Scratch<uint32_t> descriptorSets = NRI_ALLOCATE_SCRATCH(device, uint32_t, idBound);
    Scratch<uint32_t> bindings = NRI_ALLOCATE_SCRATCH(device, uint32_t, idBound);
    Scratch<uint32_t> pointerTypes = NRI_ALLOCATE_SCRATCH(device, uint32_t, idBound);
    Scratch<uint32_t> resourceTypes = NRI_ALLOCATE_SCRATCH(device, uint32_t, idBound);
    Scratch<uint32_t> pointerSourceVariables = NRI_ALLOCATE_SCRATCH(device, uint32_t, idBound);
    Scratch<uint32_t> imageSourceVariables = NRI_ALLOCATE_SCRATCH(device, uint32_t, idBound);
    Scratch<uint8_t> nonWritableVariables = NRI_ALLOCATE_SCRATCH(device, uint8_t, idBound);
    Scratch<uint8_t> nonReadableVariables = NRI_ALLOCATE_SCRATCH(device, uint8_t, idBound);
    Scratch<uint8_t> readVariables = NRI_ALLOCATE_SCRATCH(device, uint8_t, idBound);
    Scratch<uint8_t> writeVariables = NRI_ALLOCATE_SCRATCH(device, uint8_t, idBound);
    Scratch<TypeInfo> types = NRI_ALLOCATE_SCRATCH(device, TypeInfo, idBound);
    for (uint32_t i = 0; i < idBound; i++) {
        scalarTypes[i] = 0;
        descriptorSets[i] = uint32_t(-1);
        bindings[i] = uint32_t(-1);
        pointerTypes[i] = uint32_t(-1);
        resourceTypes[i] = uint32_t(-1);
        pointerSourceVariables[i] = uint32_t(-1);
        imageSourceVariables[i] = uint32_t(-1);
        nonWritableVariables[i] = 0;
        nonReadableVariables[i] = 0;
        readVariables[i] = 0;
        writeVariables[i] = 0;
        types[i] = {};
    }

    for (uint32_t word = 5; word < wordNum;) {
        uint32_t instruction = code[word];
        uint32_t op = instruction & 0xFFFF;
        uint32_t count = instruction >> 16;
        const uint32_t* operands = code + word + 1;

        if (count == 0 || word + count > wordNum)
            break;

        if (op == 21 && count >= 4) {
            uint32_t resultId = operands[0];
            uint32_t signedness = operands[2];
            if (resultId < idBound)
                scalarTypes[resultId] = signedness ? 1 : 2;
        } else if (op == 22 && count >= 3) {
            uint32_t resultId = operands[0];
            if (resultId < idBound)
                scalarTypes[resultId] = 3;
        } else if (op == 71 && count >= 4) {
            uint32_t target = operands[0];
            uint32_t decoration = operands[1];
            if (target < idBound && decoration == 33)
                bindings[target] = operands[2];
            else if (target < idBound && decoration == 34)
                descriptorSets[target] = operands[2];
            else if (target < idBound && decoration == 24)
                nonWritableVariables[target] = 1;
            else if (target < idBound && decoration == 25)
                nonReadableVariables[target] = 1;
        } else if (op == 25 && count >= 9) {
            uint32_t resultId = operands[0];
            uint32_t sampledType = operands[1];
            uint32_t dim = operands[2];
            uint32_t depth = operands[3];
            uint32_t arrayed = operands[4];
            uint32_t ms = operands[5];
            uint32_t sampled = operands[6];
            uint32_t imageFormat = operands[7];
            if (resultId < idBound && dim != 5 && dim != 6) {
                TypeInfo& type = types[resultId];
                if (sampled == 2 && imageFormat != 0) {
                    type.type = DescriptorType::STORAGE_TEXTURE;
                    type.imageFormat = imageFormat;
                    type.viewDimension = GetStorageTextureViewDimensionFromSpirv(dim, arrayed);
                    type.access = count >= 10 ? GetStorageTextureAccessFromSpirv(operands[8]) : WGPUStorageTextureAccess_Undefined;
                } else {
                    type.type = DescriptorType::TEXTURE;
                    type.sampleType = GetTextureSampleTypeFromSpirv(scalarTypes, sampledType, depth, idBound);
                    type.viewDimension = GetTextureViewDimensionFromSpirv(dim, arrayed);
                    type.multisampled = ms ? WGPU_TRUE : WGPU_FALSE;
                }
            }
        } else if (op == 32 && count >= 4) {
            uint32_t resultId = operands[0];
            uint32_t typeId = operands[2];
            if (resultId < idBound)
                pointerTypes[resultId] = typeId;
        } else if (op == 59 && count >= 4) {
            uint32_t resultTypeId = operands[0];
            uint32_t resultId = operands[1];
            if (resultId < idBound && resultTypeId < idBound && pointerTypes[resultTypeId] < idBound && types[pointerTypes[resultTypeId]].type != DescriptorType::MAX_NUM) {
                resourceTypes[resultId] = pointerTypes[resultTypeId];
                pointerSourceVariables[resultId] = resultId;
            }
        } else if ((op == 65 || op == 66) && count >= 4) {
            uint32_t resultId = operands[1];
            uint32_t baseId = operands[2];
            if (resultId < idBound)
                pointerSourceVariables[resultId] = GetSourceVariable(pointerSourceVariables, baseId, idBound);
        } else if (op == 61 && count >= 4) {
            uint32_t resultId = operands[1];
            uint32_t pointerId = operands[2];
            if (resultId < idBound)
                imageSourceVariables[resultId] = GetSourceVariable(pointerSourceVariables, pointerId, idBound);
        } else if (op == 98 && count >= 4) {
            uint32_t imageId = operands[2];
            uint32_t variableId = GetSourceVariable(imageSourceVariables, imageId, idBound);
            if (variableId < idBound)
                readVariables[variableId] = 1;
        } else if (op == 99 && count >= 3) {
            uint32_t imageId = operands[0];
            uint32_t variableId = GetSourceVariable(imageSourceVariables, imageId, idBound);
            if (variableId < idBound)
                writeVariables[variableId] = 1;
        }

        word += count;
    }

    for (uint32_t i = 0; i < idBound; i++) {
        uint32_t typeId = resourceTypes[i];
        if (typeId >= idBound || descriptorSets[i] == uint32_t(-1) || bindings[i] == uint32_t(-1))
            continue;

        const TypeInfo& type = types[typeId];
        TextureBindingWGPU textureBinding = {};
        textureBinding.set = descriptorSets[i];
        textureBinding.binding = bindings[i];
        textureBinding.type = type.type;
        textureBinding.sampleType = type.sampleType;
        textureBinding.viewDimension = type.viewDimension;
        textureBinding.multisampled = type.multisampled;

        if (type.type == DescriptorType::STORAGE_TEXTURE) {
            textureBinding.format = GetStorageTextureFormatFromSpirv(type.imageFormat);
            if (textureBinding.format == WGPUTextureFormat_Undefined)
                continue;

            textureBinding.access = type.access;
            if (textureBinding.access == WGPUStorageTextureAccess_Undefined) {
                if (nonReadableVariables[i])
                    textureBinding.access = WGPUStorageTextureAccess_WriteOnly;
                else if (nonWritableVariables[i])
                    textureBinding.access = WGPUStorageTextureAccess_ReadOnly;
                else if (readVariables[i] && writeVariables[i])
                    textureBinding.access = WGPUStorageTextureAccess_ReadWrite;
                else if (readVariables[i])
                    textureBinding.access = WGPUStorageTextureAccess_ReadOnly;
                else
                    textureBinding.access = WGPUStorageTextureAccess_WriteOnly;
            }
        }

        AddTextureBindingWGPU(textureBindings, textureBindingNum, textureBindingMaxNum, textureBinding);
    }
}

Result PipelineLayoutWGPU::UpdateTextureBindings(Vector<DescriptorSetMappingWGPU>& setMappings, const ShaderDesc* shaderDescs, uint32_t shaderDescNum) const {
    uint32_t textureBindingMaxNum = 0;
    for (uint32_t i = 0; i < shaderDescNum; i++)
        textureBindingMaxNum += (uint32_t)std::max<size_t>(shaderDescs[i].size, 1);

    Scratch<TextureBindingWGPU> textureBindings = NRI_ALLOCATE_SCRATCH(m_Device, TextureBindingWGPU, textureBindingMaxNum);
    uint32_t textureBindingNum = 0;
    for (uint32_t i = 0; i < shaderDescNum; i++)
        ReflectTextures(m_Device, shaderDescs[i], textureBindings, textureBindingNum, textureBindingMaxNum);

    for (uint32_t i = 0; i < textureBindingNum; i++) {
        const TextureBindingWGPU& textureBinding = textureBindings[i];
        for (DescriptorSetMappingWGPU& mapping : setMappings) {
            if (mapping.bindGroupIndex != textureBinding.set)
                continue;

            for (DescriptorRangeMappingWGPU& range : mapping.ranges) {
                if (textureBinding.binding < range.bindingBase || textureBinding.binding >= range.bindingBase + range.descriptorNum)
                    continue;

                if ((range.type == DescriptorType::TEXTURE || range.type == DescriptorType::INPUT_ATTACHMENT) && textureBinding.type == DescriptorType::TEXTURE) {
                    if (range.textureSampleType != textureBinding.sampleType || range.textureViewDimension != textureBinding.viewDimension || range.textureMultisampled != textureBinding.multisampled) {
                        range.textureSampleType = textureBinding.sampleType;
                        range.textureViewDimension = textureBinding.viewDimension;
                        range.textureMultisampled = textureBinding.multisampled;
                    }
                } else if (range.type == DescriptorType::STORAGE_TEXTURE && textureBinding.type == DescriptorType::STORAGE_TEXTURE) {
                    if (range.storageTextureFormat != textureBinding.format || range.storageTextureViewDimension != textureBinding.viewDimension || range.storageTextureAccess != textureBinding.access) {
                        range.storageTextureFormat = textureBinding.format;
                        range.storageTextureViewDimension = textureBinding.viewDimension;
                        range.storageTextureAccess = textureBinding.access;
                    }
                }
            }
        }
    }

    return Result::SUCCESS;
}

bool PipelineLayoutWGPU::HasBindGroup(uint32_t bindGroupIndex, WGPUShaderStage visibility) const {
    for (const DescriptorSetMappingWGPU& mapping : m_SetMappings) {
        if (mapping.bindGroupIndex != bindGroupIndex)
            continue;

        for (const DescriptorRangeMappingWGPU& range : mapping.ranges) {
            if (range.visibility & visibility)
                return true;
        }
    }

    if (bindGroupIndex == m_RootSamplerGroupIndex)
        return m_RootSamplerLayout != nullptr;

    return false;
}

static bool HasPipelineBindGroupWGPU(const Vector<DescriptorSetMappingWGPU>& setMappings, uint32_t bindGroupIndex, WGPUShaderStage visibility) {
    for (const DescriptorSetMappingWGPU& mapping : setMappings) {
        if (mapping.bindGroupIndex != bindGroupIndex)
            continue;

        for (const DescriptorRangeMappingWGPU& range : mapping.ranges) {
            if (range.visibility & visibility)
                return true;
        }
    }

    return false;
}

static void CopyPipelineSetMappings(const Vector<DescriptorSetMappingWGPU>& srcMappings, Vector<DescriptorSetMappingWGPU>& dstMappings, const StdAllocator<uint8_t>& allocator) {
    dstMappings.clear();
    dstMappings.reserve(srcMappings.size());

    for (const DescriptorSetMappingWGPU& srcMapping : srcMappings) {
        dstMappings.emplace_back(allocator);
        DescriptorSetMappingWGPU& dstMapping = dstMappings.back();
        dstMapping.bindGroupIndex = srcMapping.bindGroupIndex;
        dstMapping.layoutVersion = srcMapping.layoutVersion;

        uint32_t rangeNum = 0;
        for (const DescriptorRangeMappingWGPU& srcRange : srcMapping.ranges)
            rangeNum += srcRange.isArray ? 1 : srcRange.descriptorNum;

        dstMapping.ranges.reserve(rangeNum);
        for (const DescriptorRangeMappingWGPU& srcRange : srcMapping.ranges) {
            if (srcRange.isArray) {
                dstMapping.ranges.push_back(srcRange);
                continue;
            }

            for (uint32_t i = 0; i < srcRange.descriptorNum; i++) {
                DescriptorRangeMappingWGPU dstRange = srcRange;
                dstRange.descriptorOffset += i;
                dstRange.bindingBase += i;
                dstRange.descriptorNum = 1;
                dstMapping.ranges.push_back(dstRange);
            }
        }
    }
}

Result PipelineLayoutWGPU::CreatePipelineLayout(const ShaderDesc* shaderDescs, uint32_t shaderDescNum, WGPUShaderStage visibility, Vector<DescriptorSetMappingWGPU>& setMappings, WGPUPipelineLayout& pipelineLayout) const {
    CopyPipelineSetMappings(m_SetMappings, setMappings, m_Device.GetStdAllocator());

    Result result = UpdateTextureBindings(setMappings, shaderDescs, shaderDescNum);
    if (result != Result::SUCCESS)
        return result;

    for (DescriptorSetMappingWGPU& mapping : setMappings) {
        if (!HasPipelineBindGroupWGPU(setMappings, mapping.bindGroupIndex, visibility))
            continue;

        uint32_t entryNum = 0;
        for (const DescriptorRangeMappingWGPU& range : mapping.ranges)
            entryNum += range.isArray ? 1 : range.descriptorNum;

        Scratch<WGPUBindGroupLayoutEntry> entries = NRI_ALLOCATE_SCRATCH(m_Device, WGPUBindGroupLayoutEntry, entryNum);
        uint32_t entryOffset = 0;
        for (const DescriptorRangeMappingWGPU& range : mapping.ranges) {
            if (range.isArray)
                FillLayoutEntry(entries[entryOffset++], range, range.bindingBase, range.descriptorNum);
            else {
                for (uint32_t i = 0; i < range.descriptorNum; i++)
                    FillLayoutEntry(entries[entryOffset++], range, range.bindingBase + i);
            }
        }

        WGPUBindGroupLayoutDescriptor layoutDesc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
        layoutDesc.entryCount = entryNum;
        layoutDesc.entries = entries;

        mapping.layout = wgpuDeviceCreateBindGroupLayout(m_Device, &layoutDesc);
        if (!mapping.layout)
            return Result::FAILURE;
    }

    uint32_t bindGroupLayoutNum = 0;
    for (uint32_t i = 0; i < (uint32_t)m_BindGroupLayouts.size(); i++) {
        if (HasPipelineBindGroupWGPU(setMappings, i, visibility) || i == m_RootSamplerGroupIndex)
            bindGroupLayoutNum = i + 1;
    }

    Scratch<WGPUBindGroupLayout> bindGroupLayouts = NRI_ALLOCATE_SCRATCH(m_Device, WGPUBindGroupLayout, bindGroupLayoutNum);
    for (uint32_t i = 0; i < bindGroupLayoutNum; i++) {
        bindGroupLayouts[i] = m_EmptyBindGroupLayout;
        if (i == m_RootSamplerGroupIndex) {
            bindGroupLayouts[i] = m_RootSamplerLayout;
            continue;
        }

        for (const DescriptorSetMappingWGPU& mapping : setMappings) {
            if (mapping.bindGroupIndex == i && mapping.layout) {
                bindGroupLayouts[i] = mapping.layout;
                break;
            }
        }
    }

    WGPUPipelineLayoutExtras extras = {};
    extras.chain.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras;
    // TODO: Immediate data is a wgpu-native extension used to emulate NRI root constants.
    extras.immediateDataSize = m_ImmediateDataSize;

    WGPUPipelineLayoutDescriptor desc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    desc.nextInChain = m_ImmediateDataSize ? &extras.chain : nullptr;
    desc.bindGroupLayoutCount = bindGroupLayoutNum;
    desc.bindGroupLayouts = bindGroupLayoutNum ? (WGPUBindGroupLayout*)bindGroupLayouts : nullptr;

    pipelineLayout = wgpuDeviceCreatePipelineLayout(m_Device, &desc);

    return pipelineLayout ? Result::SUCCESS : Result::FAILURE;
}
