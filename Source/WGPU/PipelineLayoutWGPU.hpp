// © 2026 NVIDIA Corporation

static void FillLayoutEntry(WGPUBindGroupLayoutEntry& entry, DescriptorType descriptorType, WGPUShaderStage visibility, WGPUTextureFormat storageTextureFormat, WGPUTextureViewDimension storageTextureViewDimension, uint32_t binding, uint32_t bindingArraySize = 0) {
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
            entry.texture.sampleType = WGPUTextureSampleType_Float;
            entry.texture.viewDimension = WGPUTextureViewDimension_2D;
            entry.texture.multisampled = WGPU_FALSE;
            break;
        case DescriptorType::STORAGE_TEXTURE:
            entry.storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
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

static void FillLayoutEntry(WGPUBindGroupLayoutEntry& entry, const DescriptorRangeDesc& range, uint32_t binding, uint32_t bindingArraySize = 0) {
    FillLayoutEntry(entry, range.descriptorType, GetShaderStageFlags(range.shaderStages), WGPUTextureFormat_Undefined, WGPUTextureViewDimension_2D, binding, bindingArraySize);
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
    if (m_PipelineLayout)
        wgpuPipelineLayoutRelease(m_PipelineLayout);

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

    m_ImmediateDataSize = 0;
    for (uint32_t i = 0; i < pipelineLayoutDesc.rootConstantNum; i++)
        m_ImmediateDataSize = std::max(m_ImmediateDataSize, pipelineLayoutDesc.rootConstants[i].size);

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
                FillLayoutEntry(entries[entryOffset++], range, bindingBase, range.descriptorNum);
            else {
                for (uint32_t k = 0; k < range.descriptorNum; k++)
                    FillLayoutEntry(entries[entryOffset++], range, bindingBase + k);
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

            m_RootSamplers.push_back({sampler, binding});

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

            m_RootDescriptors.push_back({rootDescriptor.descriptorType, binding});
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

    WGPUPipelineLayoutExtras extras = {};
    extras.chain.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras;
    extras.immediateDataSize = m_ImmediateDataSize;

    WGPUPipelineLayoutDescriptor desc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    desc.nextInChain = m_ImmediateDataSize ? &extras.chain : nullptr;
    desc.bindGroupLayoutCount = m_BindGroupLayouts.size();
    desc.bindGroupLayouts = m_BindGroupLayouts.data();

    m_PipelineLayout = wgpuDeviceCreatePipelineLayout(m_Device, &desc);

    return m_PipelineLayout ? Result::SUCCESS : Result::FAILURE;
}

static WGPUTextureFormat GetStorageTextureFormatFromSpirv(uint32_t imageFormat) {
    switch (imageFormat) {
        case 1: return WGPUTextureFormat_RGBA32Float;
        case 2: return WGPUTextureFormat_RGBA16Float;
        case 3: return WGPUTextureFormat_R32Float;
        case 4: return WGPUTextureFormat_RGBA8Unorm;
        case 5: return WGPUTextureFormat_RGBA8Snorm;
        case 6: return WGPUTextureFormat_RG32Float;
        case 7: return WGPUTextureFormat_RG16Float;
        case 9: return WGPUTextureFormat_R16Float;
        case 21: return WGPUTextureFormat_RGBA32Sint;
        case 22: return WGPUTextureFormat_RGBA16Sint;
        case 23: return WGPUTextureFormat_RGBA8Sint;
        case 24: return WGPUTextureFormat_R32Sint;
        case 25: return WGPUTextureFormat_RG32Sint;
        case 26: return WGPUTextureFormat_RG16Sint;
        case 27: return WGPUTextureFormat_RG8Sint;
        case 28: return WGPUTextureFormat_R16Sint;
        case 29: return WGPUTextureFormat_R8Sint;
        case 30: return WGPUTextureFormat_RGBA32Uint;
        case 31: return WGPUTextureFormat_RGBA16Uint;
        case 32: return WGPUTextureFormat_RGBA8Uint;
        case 33: return WGPUTextureFormat_R32Uint;
        case 35: return WGPUTextureFormat_RG32Uint;
        case 36: return WGPUTextureFormat_RG16Uint;
        case 37: return WGPUTextureFormat_RG8Uint;
        case 38: return WGPUTextureFormat_R16Uint;
        case 39: return WGPUTextureFormat_R8Uint;
        default: return WGPUTextureFormat_Undefined;
    }
}

struct StorageTextureBindingWGPU {
    uint32_t set = 0;
    uint32_t binding = 0;
    WGPUTextureFormat format = WGPUTextureFormat_Undefined;
    WGPUTextureViewDimension viewDimension = WGPUTextureViewDimension_2D;
};

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

static void ReflectStorageTexturesFromWgsl(const ShaderDesc& shaderDesc, Vector<StorageTextureBindingWGPU>& storageTextures) {
    const char* source = (const char*)shaderDesc.bytecode;
    if (!source || shaderDesc.size == 0)
        return;

    const char* sourceEnd = source + shaderDesc.size;
    const char* token = "texture_storage_";
    const char* it = source;
    while ((it = FindTokenWGPU(it, sourceEnd, token)) != nullptr) {
        const char* declarationBegin = it - source > 512 ? it - 512 : source;
        uint32_t set = 0;
        uint32_t binding = 0;
        if (!ParseUintAttributeWGPU(declarationBegin, it, "@group", set) || !ParseUintAttributeWGPU(declarationBegin, it, "@binding", binding)) {
            it += strlen(token);
            continue;
        }

        const char* dimensionBegin = it + strlen(token);
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
        if (format != WGPUTextureFormat_Undefined)
            storageTextures.push_back({set, binding, format, GetStorageTextureViewDimensionFromWgsl(dimensionBegin, dimensionEnd)});

        it = formatEnd;
    }
}

static void ReflectStorageTextures(const ShaderDesc& shaderDesc, Vector<StorageTextureBindingWGPU>& storageTextures) {
    const uint32_t* code = (const uint32_t*)shaderDesc.bytecode;
    uint32_t wordNum = (uint32_t)(shaderDesc.size / sizeof(uint32_t));
    if (!code || wordNum < 5 || code[0] != 0x07230203) {
        ReflectStorageTexturesFromWgsl(shaderDesc, storageTextures);
        return;
    }

    struct TypeInfo {
        uint32_t imageFormat = 0;
        WGPUTextureViewDimension viewDimension = WGPUTextureViewDimension_2D;
        bool isStorageTexture = false;
    };

    uint32_t idBound = code[3];
    Vector<uint32_t> descriptorSets(storageTextures.get_allocator());
    Vector<uint32_t> bindings(storageTextures.get_allocator());
    Vector<uint32_t> pointerTypes(storageTextures.get_allocator());
    Vector<TypeInfo> types(storageTextures.get_allocator());
    descriptorSets.resize(idBound, uint32_t(-1));
    bindings.resize(idBound, uint32_t(-1));
    pointerTypes.resize(idBound, uint32_t(-1));
    types.resize(idBound);

    for (uint32_t word = 5; word < wordNum;) {
        uint32_t instruction = code[word];
        uint32_t op = instruction & 0xFFFF;
        uint32_t count = instruction >> 16;
        const uint32_t* operands = code + word + 1;

        if (count == 0 || word + count > wordNum)
            break;

        if (op == 71 && count >= 4) {
            uint32_t target = operands[0];
            uint32_t decoration = operands[1];
            if (target < idBound && decoration == 33)
                bindings[target] = operands[2];
            else if (target < idBound && decoration == 34)
                descriptorSets[target] = operands[2];
        } else if (op == 25 && count >= 9) {
            uint32_t resultId = operands[0];
            uint32_t dim = operands[2];
            uint32_t arrayed = operands[4];
            uint32_t sampled = operands[6];
            uint32_t imageFormat = operands[7];
            if (resultId < idBound && sampled == 2 && dim != 5 && imageFormat != 0) {
                types[resultId].imageFormat = imageFormat;
                types[resultId].viewDimension = GetStorageTextureViewDimensionFromSpirv(dim, arrayed);
                types[resultId].isStorageTexture = true;
            }
        } else if (op == 32 && count >= 4) {
            uint32_t resultId = operands[0];
            uint32_t typeId = operands[2];
            if (resultId < idBound)
                pointerTypes[resultId] = typeId;
        } else if (op == 59 && count >= 4) {
            uint32_t resultTypeId = operands[0];
            uint32_t resultId = operands[1];
            if (resultId < idBound && resultTypeId < idBound && pointerTypes[resultTypeId] < idBound) {
                const TypeInfo& type = types[pointerTypes[resultTypeId]];
                if (type.isStorageTexture && descriptorSets[resultId] != uint32_t(-1) && bindings[resultId] != uint32_t(-1)) {
                    WGPUTextureFormat format = GetStorageTextureFormatFromSpirv(type.imageFormat);
                    if (format != WGPUTextureFormat_Undefined)
                        storageTextures.push_back({descriptorSets[resultId], bindings[resultId], format, type.viewDimension});
                }
            }
        }

        word += count;
    }
}

Result PipelineLayoutWGPU::UpdateStorageTextureFormats(const ShaderDesc* shaderDescs, uint32_t shaderDescNum) {
    Vector<StorageTextureBindingWGPU> storageTextures(m_Device.GetStdAllocator());
    for (uint32_t i = 0; i < shaderDescNum; i++)
        ReflectStorageTextures(shaderDescs[i], storageTextures);

    bool changed = false;
    for (const StorageTextureBindingWGPU& storageTexture : storageTextures) {
        for (DescriptorSetMappingWGPU& mapping : m_SetMappings) {
            if (mapping.bindGroupIndex != storageTexture.set)
                continue;

            for (DescriptorRangeMappingWGPU& range : mapping.ranges) {
                if (range.type == DescriptorType::STORAGE_TEXTURE && storageTexture.binding >= range.bindingBase && storageTexture.binding < range.bindingBase + range.descriptorNum && (range.storageTextureFormat != storageTexture.format || range.storageTextureViewDimension != storageTexture.viewDimension)) {
                    range.storageTextureFormat = storageTexture.format;
                    range.storageTextureViewDimension = storageTexture.viewDimension;
                    changed = true;
                }
            }
        }
    }

    if (!changed)
        return Result::SUCCESS;

    for (DescriptorSetMappingWGPU& mapping : m_SetMappings) {
        bool hasStorageTexture = false;
        uint32_t entryNum = 0;
        for (const DescriptorRangeMappingWGPU& range : mapping.ranges) {
            hasStorageTexture |= range.type == DescriptorType::STORAGE_TEXTURE;
            entryNum += range.isArray ? 1 : range.descriptorNum;
        }

        if (!hasStorageTexture)
            continue;

        Scratch<WGPUBindGroupLayoutEntry> entries = NRI_ALLOCATE_SCRATCH(m_Device, WGPUBindGroupLayoutEntry, entryNum);
        uint32_t entryOffset = 0;
        for (const DescriptorRangeMappingWGPU& range : mapping.ranges) {
            if (range.isArray)
                FillLayoutEntry(entries[entryOffset++], range.type, range.visibility, range.storageTextureFormat, range.storageTextureViewDimension, range.bindingBase, range.descriptorNum);
            else {
                for (uint32_t i = 0; i < range.descriptorNum; i++)
                    FillLayoutEntry(entries[entryOffset++], range.type, range.visibility, range.storageTextureFormat, range.storageTextureViewDimension, range.bindingBase + i);
            }
        }

        WGPUBindGroupLayoutDescriptor layoutDesc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
        layoutDesc.entryCount = entryNum;
        layoutDesc.entries = entries;

        WGPUBindGroupLayout layout = wgpuDeviceCreateBindGroupLayout(m_Device, &layoutDesc);
        if (!layout)
            return Result::FAILURE;

        wgpuBindGroupLayoutRelease(mapping.layout);
        mapping.layout = layout;
        m_BindGroupLayouts[mapping.bindGroupIndex] = layout;
    }

    if (m_PipelineLayout)
        wgpuPipelineLayoutRelease(m_PipelineLayout);

    WGPUPipelineLayoutExtras extras = {};
    extras.chain.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras;
    extras.immediateDataSize = m_ImmediateDataSize;

    WGPUPipelineLayoutDescriptor desc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    desc.nextInChain = m_ImmediateDataSize ? &extras.chain : nullptr;
    desc.bindGroupLayoutCount = m_BindGroupLayouts.size();
    desc.bindGroupLayouts = m_BindGroupLayouts.data();

    m_PipelineLayout = wgpuDeviceCreatePipelineLayout(m_Device, &desc);

    return m_PipelineLayout ? Result::SUCCESS : Result::FAILURE;
}
