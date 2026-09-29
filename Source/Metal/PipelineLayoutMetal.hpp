// © 2026 NVIDIA Corporation

PipelineLayoutMetal::PipelineLayoutMetal(DeviceMetal& device)
    : m_Device(device)
    , m_Sets(device.GetStdAllocator())
    , m_ConstantOffsets(device.GetStdAllocator())
    , m_DescriptorOffsets(device.GetStdAllocator())
    , m_SetOffsets(device.GetStdAllocator())
    , m_RootArguments(device.GetStdAllocator())
    , m_RootSamplers(device.GetStdAllocator())
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    , m_RootParameters(device.GetStdAllocator())
    , m_RootRanges(device.GetStdAllocator())
#endif
{
}

PipelineLayoutMetal::~PipelineLayoutMetal() {
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    if (m_RootSignature)
        IRRootSignatureDestroy(m_RootSignature);
#endif

    if (m_RootSamplerBuffer) {
        m_Device.RemoveResidency(m_RootSamplerBuffer);
        m_RootSamplerBuffer->release();
    }

    for (auto* sampler : m_RootSamplers)
        Destroy(sampler);
}

// Root signature hash records
constexpr uint32_t ROOT_HASH_CONSTANTS = 1;  // register, space, num, vertex only
constexpr uint32_t ROOT_HASH_DESCRIPTOR = 2; // type, register, space
constexpr uint32_t ROOT_HASH_RANGE = 3;      // type, num, base register, space, offset (followed by "ROOT_HASH_TABLE")
constexpr uint32_t ROOT_HASH_TABLE = 4;
constexpr uint32_t ROOT_HASH_FLAGS = 5;

// Converter reflection types of root descriptors
constexpr std::array<const char*, (size_t)DescriptorType::MAX_NUM> g_RootDescriptorReflectionTypes = {
    "SRV", // SAMPLER (invalid)
    "SRV", // MUTABLE (invalid)
    "SRV", // TEXTURE (invalid)
    "UAV", // STORAGE_TEXTURE (invalid)
    "SRV", // INPUT_ATTACHMENT (invalid)
    "SRV", // BUFFER (invalid)
    "UAV", // STORAGE_BUFFER (invalid)
    "CBV", // CONSTANT_BUFFER
    "SRV", // STRUCTURED_BUFFER
    "UAV", // STORAGE_STRUCTURED_BUFFER
    "SRV", // ACCELERATION_STRUCTURE
};
NRI_VALIDATE_ARRAY_BY_PTR(g_RootDescriptorReflectionTypes);

#if NRI_ENABLE_METAL_SHADER_CONVERTER
constexpr IRDescriptorRangeType g_DescriptorRangeTypes[] = {
    IRDescriptorRangeTypeSampler, // SAMPLER
    IRDescriptorRangeTypeSRV,     // MUTABLE (no root-table storage)
    IRDescriptorRangeTypeSRV,     // TEXTURE
    IRDescriptorRangeTypeUAV,     // STORAGE_TEXTURE
    IRDescriptorRangeTypeSRV,     // INPUT_ATTACHMENT (no root-table storage)
    IRDescriptorRangeTypeSRV,     // BUFFER
    IRDescriptorRangeTypeUAV,     // STORAGE_BUFFER
    IRDescriptorRangeTypeCBV,     // CONSTANT_BUFFER
    IRDescriptorRangeTypeSRV,     // STRUCTURED_BUFFER
    IRDescriptorRangeTypeUAV,     // STORAGE_STRUCTURED_BUFFER
    IRDescriptorRangeTypeSRV,     // ACCELERATION_STRUCTURE
};
static_assert(GetCountOf(g_DescriptorRangeTypes) == (size_t)DescriptorType::MAX_NUM, "Some elements are missing in 'g_DescriptorRangeTypes'");

constexpr IRRootParameterType g_RootDescriptorTypes[] = {
    IRRootParameterTypeSRV, // SAMPLER (invalid)
    IRRootParameterTypeSRV, // MUTABLE (invalid)
    IRRootParameterTypeSRV, // TEXTURE (invalid)
    IRRootParameterTypeUAV, // STORAGE_TEXTURE (invalid)
    IRRootParameterTypeSRV, // INPUT_ATTACHMENT (invalid)
    IRRootParameterTypeSRV, // BUFFER (invalid)
    IRRootParameterTypeUAV, // STORAGE_BUFFER (invalid)
    IRRootParameterTypeCBV, // CONSTANT_BUFFER
    IRRootParameterTypeSRV, // STRUCTURED_BUFFER
    IRRootParameterTypeUAV, // STORAGE_STRUCTURED_BUFFER
    IRRootParameterTypeSRV, // ACCELERATION_STRUCTURE
};
static_assert(GetCountOf(g_RootDescriptorTypes) == (size_t)DescriptorType::MAX_NUM, "Some elements are missing in 'g_RootDescriptorTypes'");

static inline IRRootParameter1 GetRootConstantsParameter(uint32_t registerIndex, uint32_t space, uint32_t num, IRShaderVisibility visibility) {
    IRRootParameter1 parameter = {};
    parameter.ParameterType = IRRootParameterType32BitConstants;
    parameter.Constants = {registerIndex, space, num};
    parameter.ShaderVisibility = visibility;

    return parameter;
}

static inline IRRootParameter1 GetDescriptorTableParameter(uint32_t rangeNum, IRDescriptorRange1* ranges) {
    IRRootParameter1 parameter = {};
    parameter.ParameterType = IRRootParameterTypeDescriptorTable;
    parameter.DescriptorTable = {rangeNum, ranges};
    parameter.ShaderVisibility = IRShaderVisibilityAll;

    return parameter;
}
#endif

Result PipelineLayoutMetal::Create(const PipelineLayoutDesc& desc) {
    uint32_t offset = 0;

    // The root signature hash identifies converted shaders in pipeline caches
    auto hashRootValues = [&](std::initializer_list<uint32_t> values) {
        m_RootSignatureHash = HashMetal(values.begin(), values.size() * sizeof(uint32_t), m_RootSignatureHash);
    };

#if NRI_ENABLE_METAL_SHADER_CONVERTER
    Vector<IRRootParameter1>& parameters = m_RootParameters;
    Vector<IRDescriptorRange1>& ranges = m_RootRanges;
    uint32_t rangeNum = desc.rootSamplerNum;

    for (uint32_t i = 0; i < desc.descriptorSetNum; i++)
        rangeNum += desc.descriptorSets[i].rangeNum;

    // Descriptor tables point into "ranges", which must not reallocate (kept for "GetRootSignature")
    ranges.reserve(rangeNum);
#endif

    // Draw emulation constants precede application root constants in both native and converted layouts.
    // They exist only if the corresponding "PipelineLayoutBits" are requested, regardless of the build configuration
    const bool vertexStage = (desc.shaderStages & StageBits::VERTEX_SHADER) != 0;

    if ((desc.flags & PipelineLayoutBits::ENABLE_DRAW_PARAMETERS_EMULATION) && vertexStage) {
        m_DrawParametersOffset = offset;
        m_RootArguments.push_back({"Constant", offset, 2 * sizeof(uint32_t), 0, DRAW_EMULATION_SPACE, 0});
        offset += 2 * sizeof(uint32_t);
        hashRootValues({ROOT_HASH_CONSTANTS, 0, DRAW_EMULATION_SPACE, 2, 1});

#if NRI_ENABLE_METAL_SHADER_CONVERTER
        parameters.push_back(GetRootConstantsParameter(0, DRAW_EMULATION_SPACE, 2, IRShaderVisibilityVertex));
#endif
    }

    if ((desc.flags & PipelineLayoutBits::ENABLE_DRAW_INDEX_EMULATION) && vertexStage) {
        m_DrawIndexOffset = offset;
        m_RootArguments.push_back({"Constant", offset, sizeof(uint32_t), 1, DRAW_EMULATION_SPACE, 0});
        offset += sizeof(uint32_t);
        hashRootValues({ROOT_HASH_CONSTANTS, 1, DRAW_EMULATION_SPACE, 1, 1});

#if NRI_ENABLE_METAL_SHADER_CONVERTER
        parameters.push_back(GetRootConstantsParameter(1, DRAW_EMULATION_SPACE, 1, IRShaderVisibilityVertex));
#endif
    }

    for (uint32_t i = 0; i < desc.rootConstantNum; i++) {
        m_ConstantOffsets.push_back(offset);
        m_RootArguments.push_back({"Constant", offset, desc.rootConstants[i].size, desc.rootConstants[i].registerIndex, desc.rootRegisterSpace, 0});
        offset += desc.rootConstants[i].size;
        hashRootValues({ROOT_HASH_CONSTANTS, desc.rootConstants[i].registerIndex, desc.rootRegisterSpace, desc.rootConstants[i].size / 4, 0});

#if NRI_ENABLE_METAL_SHADER_CONVERTER
        parameters.push_back(GetRootConstantsParameter(desc.rootConstants[i].registerIndex, desc.rootRegisterSpace, desc.rootConstants[i].size / 4, IRShaderVisibilityAll));
#endif
    }

    offset = Align(offset, (uint32_t)sizeof(uint64_t));

    for (uint32_t i = 0; i < desc.rootDescriptorNum; i++) {
        m_DescriptorOffsets.push_back(offset);
        m_RootArguments.push_back({g_RootDescriptorReflectionTypes[(uint32_t)desc.rootDescriptors[i].descriptorType], offset, sizeof(uint64_t), desc.rootDescriptors[i].registerIndex, desc.rootRegisterSpace, 0});
        offset += sizeof(uint64_t);
        hashRootValues({ROOT_HASH_DESCRIPTOR, (uint32_t)desc.rootDescriptors[i].descriptorType, desc.rootDescriptors[i].registerIndex, desc.rootRegisterSpace});

#if NRI_ENABLE_METAL_SHADER_CONVERTER
        const RootDescriptorDesc& root = desc.rootDescriptors[i];
        IRRootParameter1 parameter = {};
        parameter.ParameterType = g_RootDescriptorTypes[(uint32_t)root.descriptorType];
        parameter.Descriptor = {root.registerIndex, desc.rootRegisterSpace, IRRootDescriptorFlagDataVolatile};
        parameter.ShaderVisibility = IRShaderVisibilityAll;
        parameters.push_back(parameter);
#endif
    }

    for (uint32_t i = 0; i < desc.descriptorSetNum; i++) {
        m_Sets.emplace_back(m_Device.GetStdAllocator());
        DescriptorSetMappingMetal& mapping = m_Sets.back();
        const DescriptorSetDesc& set = desc.descriptorSets[i];

        for (uint32_t j = 0; j < set.rangeNum; j++) {
            const DescriptorRangeDesc& range = set.ranges[j];

            DescriptorRangeMappingMetal m = {};
            m.sampler = range.descriptorType == DescriptorType::SAMPLER;
            m.offset = m.sampler ? mapping.samplerNum : mapping.resourceNum;
            m.descriptorNum = range.descriptorNum;

            if (m.sampler)
                mapping.samplerNum += range.descriptorNum;
            else
                mapping.resourceNum += range.descriptorNum;

            mapping.ranges.push_back(m);

            if (range.flags & DescriptorRangeBits::VARIABLE_SIZED_ARRAY)
                mapping.variableRange = j;
        }

        // The variable-sized range is moved to the end of its heap group
        if (mapping.variableRange != UINT32_MAX) {
            DescriptorRangeMappingMetal& variable = mapping.ranges[mapping.variableRange];

            for (uint32_t j = mapping.variableRange + 1; j < set.rangeNum; j++) {
                if (mapping.ranges[j].sampler == variable.sampler)
                    mapping.ranges[j].offset -= variable.descriptorNum;
            }

            variable.offset = (variable.sampler ? mapping.samplerNum : mapping.resourceNum) - variable.descriptorNum;
        }

        // D3D root tables separate resources from samplers. Give each nonempty group one pointer, resources first
        for (uint32_t sampler = 0; sampler < 2; sampler++) {
            bool present = false;

#if NRI_ENABLE_METAL_SHADER_CONVERTER
            const size_t first = ranges.size();
#endif

            for (uint32_t j = 0; j < set.rangeNum; j++) {
                const DescriptorRangeDesc& source = set.ranges[j];
                const DescriptorRangeMappingMetal& mappingRange = mapping.ranges[j];

                // "MUTABLE" ranges are directly indexed heaps. Input attachments are framebuffer fetches in a reserved space.
                // Neither uses root-table storage, but both keep their descriptor slots to preserve subsequent range offsets
                if (mappingRange.sampler != bool(sampler) || source.descriptorType == DescriptorType::MUTABLE || source.descriptorType == DescriptorType::INPUT_ATTACHMENT)
                    continue;

                present = true;
                hashRootValues({ROOT_HASH_RANGE, (uint32_t)source.descriptorType, source.descriptorNum, source.baseRegisterIndex, set.registerSpace, mappingRange.offset});

#if NRI_ENABLE_METAL_SHADER_CONVERTER
                IRDescriptorRange1 range = {};
                range.RangeType = g_DescriptorRangeTypes[(uint32_t)source.descriptorType];
                range.NumDescriptors = source.descriptorNum;
                range.BaseShaderRegister = source.baseRegisterIndex;
                range.RegisterSpace = set.registerSpace;
                range.OffsetInDescriptorsFromTableStart = mappingRange.offset;
                ranges.push_back(range);
#endif
            }

            m_SetOffsets.push_back(present ? offset : UINT32_MAX);

            if (present) {
                m_RootArguments.push_back({"Table", offset, sizeof(uint64_t), UINT32_MAX, UINT32_MAX, sampler ? mapping.samplerNum : mapping.resourceNum});
                offset += sizeof(uint64_t);
                hashRootValues({ROOT_HASH_TABLE});

#if NRI_ENABLE_METAL_SHADER_CONVERTER
                parameters.push_back(GetDescriptorTableParameter((uint32_t)(ranges.size() - first), ranges.data() + first));
#endif
            }
        }
    }

    if (desc.rootSamplerNum) {
        m_RootSamplerOffset = offset;
        m_RootArguments.push_back({"Table", offset, sizeof(uint64_t), UINT32_MAX, UINT32_MAX, desc.rootSamplerNum});
        offset += sizeof(uint64_t);

        m_RootSamplerBuffer = m_Device.GetNativeObject()->newBuffer(desc.rootSamplerNum * DESCRIPTOR_ENTRY_SIZE, MTL::ResourceStorageModeShared);

        if (!m_RootSamplerBuffer)
            return Result::OUT_OF_MEMORY;

        m_Device.AddResidency(m_RootSamplerBuffer);

#if NRI_ENABLE_METAL_SHADER_CONVERTER
        const size_t first = ranges.size();
#endif

        for (uint32_t i = 0; i < desc.rootSamplerNum; i++) {
            DescriptorMetal* sampler = nullptr;
            Result result = m_Device.CreateImplementation<DescriptorMetal>(sampler, desc.rootSamplers[i].desc);

            if (result != Result::SUCCESS)
                return result;

            m_RootSamplers.push_back(sampler);
            sampler->WriteEntry((uint8_t*)m_RootSamplerBuffer->contents() + i * DESCRIPTOR_ENTRY_SIZE);
            hashRootValues({ROOT_HASH_RANGE, (uint32_t)DescriptorType::SAMPLER, 1, desc.rootSamplers[i].registerIndex, desc.rootRegisterSpace, i});

#if NRI_ENABLE_METAL_SHADER_CONVERTER
            ranges.push_back({IRDescriptorRangeTypeSampler, 1, desc.rootSamplers[i].registerIndex, desc.rootRegisterSpace, IRDescriptorRangeFlagNone, i});
#endif
        }

        hashRootValues({ROOT_HASH_TABLE});

#if NRI_ENABLE_METAL_SHADER_CONVERTER
        parameters.push_back(GetDescriptorTableParameter(desc.rootSamplerNum, ranges.data() + first));
#endif
    }

    m_RootDataSize = Align(offset, 16u);
    hashRootValues({ROOT_HASH_FLAGS, (desc.flags & PipelineLayoutBits::RESOURCE_HEAP_DIRECTLY_INDEXED) ? 1u : 0u, (desc.flags & PipelineLayoutBits::SAMPLER_HEAP_DIRECTLY_INDEXED) ? 1u : 0u});

#if NRI_ENABLE_METAL_SHADER_CONVERTER
    uint32_t flags = IRRootSignatureFlagAllowInputAssemblerInputLayout;

    if (desc.flags & PipelineLayoutBits::RESOURCE_HEAP_DIRECTLY_INDEXED)
        flags |= IRRootSignatureFlagCBVSRVUAVHeapDirectlyIndexed;

    if (desc.flags & PipelineLayoutBits::SAMPLER_HEAP_DIRECTLY_INDEXED)
        flags |= IRRootSignatureFlagSamplerHeapDirectlyIndexed;

    m_RootSignatureFlags = (IRRootSignatureFlags)flags;

    const IRVersionedRootSignatureDescriptor root = GetRootSignatureDesc();

    IRError* error = nullptr;
    m_RootSignature = IRRootSignatureCreateFromDescriptor(&root, &error);

    if (error) {
        NRI_REPORT_ERROR(&m_Device, "Metal root signature creation failed (converter error %u)", IRErrorGetCode(error));
        IRErrorDestroy(error);
    }

    if (!m_RootSignature)
        return Result::FAILURE;
#endif

    return Result::SUCCESS;
}

DeviceMetal& PipelineLayoutMetal::GetDevice() const {
    return m_Device;
}

uint64_t PipelineLayoutMetal::GetRootSignatureHash() const {
    return m_RootSignatureHash;
}

const Vector<RootArgumentMetal>& PipelineLayoutMetal::GetRootArguments() const {
    return m_RootArguments;
}

uint32_t PipelineLayoutMetal::GetRootDataSize() const {
    return m_RootDataSize;
}

uint32_t PipelineLayoutMetal::GetRootConstantOffset(uint32_t index) const {
    return m_ConstantOffsets[index];
}

uint32_t PipelineLayoutMetal::GetRootDescriptorOffset(uint32_t index) const {
    return m_DescriptorOffsets[index];
}

uint32_t PipelineLayoutMetal::GetDrawParametersOffset() const {
    return m_DrawParametersOffset;
}

uint32_t PipelineLayoutMetal::GetDrawIndexOffset() const {
    return m_DrawIndexOffset;
}

bool PipelineLayoutMetal::IsDrawParametersEmulationEnabled() const {
    return m_DrawParametersOffset != UINT32_MAX;
}

bool PipelineLayoutMetal::IsDrawIndexEmulationEnabled() const {
    return m_DrawIndexOffset != UINT32_MAX;
}

const DescriptorSetMappingMetal& PipelineLayoutMetal::GetDescriptorSetMapping(uint32_t index) const {
    return m_Sets[index];
}

void PipelineLayoutMetal::InitRootData(void* data) const {
    if (m_RootDataSize)
        memset(data, 0, m_RootDataSize);

    if (m_RootSamplerBuffer) {
        uint64_t address = m_RootSamplerBuffer->gpuAddress();
        memcpy((uint8_t*)data + m_RootSamplerOffset, &address, sizeof(address));
    }
}

void PipelineLayoutMetal::WriteSetPointers(void* data, uint32_t index, const DescriptorSetMetal& set) const {
    const uint64_t addresses[] = {set.GetResourceAddress(), set.GetSamplerAddress()};

    for (uint32_t i = 0; i < 2; i++) {
        uint32_t offset = m_SetOffsets[index * 2 + i];

        if (offset != UINT32_MAX)
            memcpy((uint8_t*)data + offset, &addresses[i], sizeof(uint64_t));
    }
}

#if NRI_ENABLE_METAL_SHADER_CONVERTER
IRVersionedRootSignatureDescriptor PipelineLayoutMetal::GetRootSignatureDesc() const {
    IRVersionedRootSignatureDescriptor root = {};
    root.version = IRRootSignatureVersion_1_1;
    root.desc_1_1.NumParameters = (uint32_t)m_RootParameters.size();
    root.desc_1_1.pParameters = const_cast<IRRootParameter1*>(m_RootParameters.data()); // not modified
    root.desc_1_1.Flags = m_RootSignatureFlags;

    return root;
}

IRRootSignature* PipelineLayoutMetal::GetRootSignature() const {
    return m_RootSignature;
}

#endif

Result PipelineLayoutMetal::GetRootSignature(char* json, uint64_t& size) const {
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    const IRVersionedRootSignatureDescriptor root = GetRootSignatureDesc();
    const char* string = IRVersionedRootSignatureDescriptorCopyJSONString(&root);

    if (!string)
        return Result::FAILURE;

    const uint64_t length = strlen(string) + 1;
    Result result = Result::SUCCESS;

    if (json) {
        if (size < length)
            result = Result::OUT_OF_MEMORY;
        else
            memcpy(json, string, length);
    }

    IRVersionedRootSignatureDescriptorReleaseString(string);

    size = length;

    return result;
#else
    // JSON requires Converter
    if (json)
        return Result::UNSUPPORTED;

    size = 0;

    return Result::SUCCESS;
#endif
}
