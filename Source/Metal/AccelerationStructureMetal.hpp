// © 2026 NVIDIA Corporation

static MTL::AccelerationStructureUsage GetAccelerationStructureUsageMetal(AccelerationStructureBits flags) {
    MTL::AccelerationStructureUsage usage = MTL::AccelerationStructureUsageNone;

    if (flags & AccelerationStructureBits::ALLOW_UPDATE)
        usage |= MTL::AccelerationStructureUsageRefit;

    if (flags & AccelerationStructureBits::PREFER_FAST_BUILD)
        usage |= MTL::AccelerationStructureUsagePreferFastBuild;

    if (flags & AccelerationStructureBits::PREFER_FAST_TRACE)
        usage |= MTL::AccelerationStructureUsagePreferFastIntersection;

    if (flags & AccelerationStructureBits::MINIMIZE_MEMORY)
        usage |= MTL::AccelerationStructureUsageMinimizeMemory;

    return usage;
}

static inline MTL::IndexType GetAccelerationStructureIndexTypeMetal(IndexType indexType) {
    constexpr std::array<MTL::IndexType, (size_t)IndexType::MAX_NUM> g_IndexTypes = {
        MTL::IndexTypeUInt16, // UINT16
        MTL::IndexTypeUInt32, // UINT32
    };
    NRI_VALIDATE_ARRAY(g_IndexTypes);

    return g_IndexTypes[(size_t)indexType];
}

// Formats marked "AccelerationStructure" compatible in "NRIDescs.h" (the last one is "R10_G10_B10_A2_UNORM")
constexpr std::array<MTL::AttributeFormat, (size_t)Format::R10_G10_B10_A2_UNORM + 1> g_AccelerationStructureVertexFormats = {
    MTL::AttributeFormatInvalid,               // UNKNOWN
    MTL::AttributeFormatInvalid,               // R8_UNORM
    MTL::AttributeFormatInvalid,               // R8_SNORM
    MTL::AttributeFormatInvalid,               // R8_UINT
    MTL::AttributeFormatInvalid,               // R8_SINT
    MTL::AttributeFormatUChar2Normalized,      // RG8_UNORM
    MTL::AttributeFormatChar2Normalized,       // RG8_SNORM
    MTL::AttributeFormatInvalid,               // RG8_UINT
    MTL::AttributeFormatInvalid,               // RG8_SINT
    MTL::AttributeFormatInvalid,               // BGRA8_UNORM
    MTL::AttributeFormatInvalid,               // BGRA8_SRGB
    MTL::AttributeFormatUChar4Normalized,      // RGBA8_UNORM
    MTL::AttributeFormatInvalid,               // RGBA8_SRGB
    MTL::AttributeFormatChar4Normalized,       // RGBA8_SNORM
    MTL::AttributeFormatInvalid,               // RGBA8_UINT
    MTL::AttributeFormatInvalid,               // RGBA8_SINT
    MTL::AttributeFormatInvalid,               // R16_UNORM
    MTL::AttributeFormatInvalid,               // R16_SNORM
    MTL::AttributeFormatInvalid,               // R16_UINT
    MTL::AttributeFormatInvalid,               // R16_SINT
    MTL::AttributeFormatInvalid,               // R16_SFLOAT
    MTL::AttributeFormatUShort2Normalized,     // RG16_UNORM
    MTL::AttributeFormatShort2Normalized,      // RG16_SNORM
    MTL::AttributeFormatInvalid,               // RG16_UINT
    MTL::AttributeFormatInvalid,               // RG16_SINT
    MTL::AttributeFormatHalf2,                 // RG16_SFLOAT
    MTL::AttributeFormatUShort4Normalized,     // RGBA16_UNORM
    MTL::AttributeFormatShort4Normalized,      // RGBA16_SNORM
    MTL::AttributeFormatInvalid,               // RGBA16_UINT
    MTL::AttributeFormatInvalid,               // RGBA16_SINT
    MTL::AttributeFormatHalf4,                 // RGBA16_SFLOAT
    MTL::AttributeFormatInvalid,               // R32_UINT
    MTL::AttributeFormatInvalid,               // R32_SINT
    MTL::AttributeFormatInvalid,               // R32_SFLOAT
    MTL::AttributeFormatInvalid,               // RG32_UINT
    MTL::AttributeFormatInvalid,               // RG32_SINT
    MTL::AttributeFormatFloat2,                // RG32_SFLOAT
    MTL::AttributeFormatInvalid,               // RGB32_UINT
    MTL::AttributeFormatInvalid,               // RGB32_SINT
    MTL::AttributeFormatFloat3,                // RGB32_SFLOAT
    MTL::AttributeFormatInvalid,               // RGBA32_UINT
    MTL::AttributeFormatInvalid,               // RGBA32_SINT
    MTL::AttributeFormatFloat4,                // RGBA32_SFLOAT
    MTL::AttributeFormatInvalid,               // B5_G6_R5_UNORM
    MTL::AttributeFormatInvalid,               // B5_G5_R5_A1_UNORM
    MTL::AttributeFormatInvalid,               // B4_G4_R4_A4_UNORM
    MTL::AttributeFormatUInt1010102Normalized, // R10_G10_B10_A2_UNORM
};
NRI_VALIDATE_ARRAY(g_AccelerationStructureVertexFormats);

static MTL::AttributeFormat GetAccelerationStructureVertexFormatMetal(Format format) {
    return (size_t)format < g_AccelerationStructureVertexFormats.size() ? g_AccelerationStructureVertexFormats[(size_t)format] : MTL::AttributeFormatInvalid;
}

// "sizing" replaces buffer addresses with placeholders, since buffers are optional in "AccelerationStructureDesc"
static MTL4::AccelerationStructureDescriptor* CreateBottomLevelDescriptorMetal(DeviceMetal& device, AccelerationStructureBits flags, const BottomLevelGeometryDesc* geometries, uint32_t geometryNum, bool sizing) {
    auto* descriptor = MTL4::PrimitiveAccelerationStructureDescriptor::alloc()->init();
    descriptor->setUsage(GetAccelerationStructureUsageMetal(flags));

    const auto address = [sizing](const Buffer* buffer, uint64_t offset) -> MTL::GPUAddress {
        if (sizing)
            return 1;

        return buffer ? ((const BufferMetal*)buffer)->GetGpuAddress() + offset : 0;
    };

    Scratch<NS::Object*> objects = NRI_ALLOCATE_SCRATCH(device, NS::Object*, geometryNum);

    for (uint32_t i = 0; i < geometryNum; i++) {
        const BottomLevelGeometryDesc& geometry = geometries[i];
        const bool isOpaque = (geometry.flags & BottomLevelGeometryBits::OPAQUE_GEOMETRY) != 0;
        const bool allowDuplicateAnyHit = (geometry.flags & BottomLevelGeometryBits::NO_DUPLICATE_ANY_HIT_INVOCATION) == 0;

        if (geometry.type == BottomLevelGeometryType::TRIANGLES) {
            const BottomLevelTrianglesDesc& triangles = geometry.triangles;

            // Indexed-ness is determined by "indexNum", like in VK, to match between sizing and building
            const bool isIndexed = triangles.indexNum != 0;

            auto* metalGeometry = MTL4::AccelerationStructureTriangleGeometryDescriptor::alloc()->init();
            metalGeometry->setVertexBuffer(MTL4::BufferRange(address(triangles.vertexBuffer, triangles.vertexOffset)));
            metalGeometry->setVertexStride(triangles.vertexStride);
            metalGeometry->setVertexFormat(GetAccelerationStructureVertexFormatMetal(triangles.vertexFormat));
            metalGeometry->setTriangleCount((isIndexed ? triangles.indexNum : triangles.vertexNum) / 3);
            metalGeometry->setOpaque(isOpaque);
            metalGeometry->setAllowDuplicateIntersectionFunctionInvocation(allowDuplicateAnyHit);

            if (isIndexed) {
                metalGeometry->setIndexBuffer(MTL4::BufferRange(address(triangles.indexBuffer, triangles.indexOffset)));
                metalGeometry->setIndexType(GetAccelerationStructureIndexTypeMetal(triangles.indexType));
            }

            if (triangles.transformBuffer) {
                metalGeometry->setTransformationMatrixBuffer(MTL4::BufferRange(address(triangles.transformBuffer, triangles.transformOffset)));
                metalGeometry->setTransformationMatrixLayout(MTL::MatrixLayoutRowMajor);
            }

            objects[i] = metalGeometry;
        } else {
            const BottomLevelAabbsDesc& aabbs = geometry.aabbs;

            auto* metalGeometry = MTL4::AccelerationStructureBoundingBoxGeometryDescriptor::alloc()->init();
            metalGeometry->setBoundingBoxBuffer(MTL4::BufferRange(address(aabbs.buffer, aabbs.offset)));
            metalGeometry->setBoundingBoxCount(aabbs.num);
            metalGeometry->setBoundingBoxStride(aabbs.stride);
            metalGeometry->setIntersectionFunctionTableOffset(1); // procedural indirect intersection function
            metalGeometry->setOpaque(isOpaque);
            metalGeometry->setAllowDuplicateIntersectionFunctionInvocation(allowDuplicateAnyHit);

            objects[i] = metalGeometry;
        }
    }

    NS::Array* array = NS::Array::alloc()->init(objects, geometryNum);
    descriptor->setGeometryDescriptors(array);
    array->release();

    for (uint32_t i = 0; i < geometryNum; i++)
        objects[i]->release();

    return descriptor;
}

static MTL4::AccelerationStructureDescriptor* CreateTopLevelDescriptorMetal(AccelerationStructureBits flags, MTL::GPUAddress convertedInstanceAddress, uint32_t instanceNum) {
    auto* descriptor = MTL4::InstanceAccelerationStructureDescriptor::alloc()->init();
    descriptor->setUsage(GetAccelerationStructureUsageMetal(flags));
    descriptor->setInstanceCount(instanceNum);
    descriptor->setInstanceDescriptorBuffer(MTL4::BufferRange(convertedInstanceAddress));
    descriptor->setInstanceDescriptorStride(sizeof(MTL::IndirectAccelerationStructureInstanceDescriptor));
    descriptor->setInstanceDescriptorType(MTL::AccelerationStructureInstanceDescriptorTypeIndirect);
    descriptor->setInstanceTransformationMatrixLayout(MTL::MatrixLayoutRowMajor);

    return descriptor;
}

static MTL::AccelerationStructureSizes GetAccelerationStructureSizesMetal(DeviceMetal& device, const AccelerationStructureDesc& desc) {
    MTL4::AccelerationStructureDescriptor* descriptor = nullptr;

    if (desc.type == AccelerationStructureType::BOTTOM_LEVEL)
        descriptor = CreateBottomLevelDescriptorMetal(device, desc.flags, desc.geometries, desc.geometryOrInstanceNum, true);
    else
        descriptor = CreateTopLevelDescriptorMetal(desc.flags, 1, desc.geometryOrInstanceNum);

    MTL::AccelerationStructureSizes sizes = device.GetNativeObject()->accelerationStructureSizes(descriptor);
    descriptor->release();

    if (desc.optimizedSize)
        sizes.accelerationStructureSize = std::min<NS::UInteger>(sizes.accelerationStructureSize, desc.optimizedSize);

    return sizes;
}

static inline uint64_t GetTopLevelHeaderSizeMetal(uint32_t instanceNum) {
    return TOP_LEVEL_HEADER_SIZE + uint64_t(instanceNum) * sizeof(uint32_t);
}

// TLAS memory = AS storage + header (with instance contributions) placed after it, inside the same NRI memory
static void GetAccelerationStructureMemoryDescMetal(DeviceMetal& device, AccelerationStructureType type, uint64_t size, uint32_t instanceNum, MemoryLocation memoryLocation, MemoryDesc& memoryDesc, uint64_t& headerOffset) {
    const MTL::SizeAndAlign accelerationStructure = device.GetNativeObject()->heapAccelerationStructureSizeAndAlign((NS::UInteger)size);

    memoryDesc = {};
    memoryDesc.size = accelerationStructure.size;
    memoryDesc.alignment = (uint32_t)accelerationStructure.align;
    memoryDesc.type = (MemoryType)memoryLocation;
    headerOffset = 0;

    if (type == AccelerationStructureType::TOP_LEVEL) {
        const MTL::SizeAndAlign header = device.GetNativeObject()->heapBufferSizeAndAlign((NS::UInteger)GetTopLevelHeaderSizeMetal(instanceNum), MTL::ResourceStorageModePrivate);

        headerOffset = Align(accelerationStructure.size, header.align);
        memoryDesc.size = headerOffset + header.size;
        memoryDesc.alignment = std::max(memoryDesc.alignment, (uint32_t)header.align);
    }
}

AccelerationStructureMetal::~AccelerationStructureMetal() {
    Release();
}

void AccelerationStructureMetal::Release() {
    if (m_IsCommitted) {
        if (m_AccelerationStructure)
            m_Device.RemoveResidency(m_AccelerationStructure);

        if (m_ShaderBindingHeader)
            m_Device.RemoveResidency(m_ShaderBindingHeader);
    }

    if (m_AccelerationStructure)
        m_AccelerationStructure->release();

    if (m_ShaderBindingHeader)
        m_ShaderBindingHeader->release();

    m_AccelerationStructure = nullptr;
    m_ShaderBindingHeader = nullptr;
    m_IsCommitted = false;
}

void AccelerationStructureMetal::GetMemoryDesc(DeviceMetal& device, const AccelerationStructureDesc& desc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const MTL::AccelerationStructureSizes sizes = GetAccelerationStructureSizesMetal(device, desc);
    const uint32_t instanceNum = desc.type == AccelerationStructureType::TOP_LEVEL ? desc.geometryOrInstanceNum : 0;

    uint64_t headerOffset = 0;
    GetAccelerationStructureMemoryDescMetal(device, desc.type, sizes.accelerationStructureSize, instanceNum, memoryLocation, memoryDesc, headerOffset);
}

Result AccelerationStructureMetal::Create(const AccelerationStructureDesc& desc) {
    m_Flags = desc.flags;
    m_Type = desc.type;
    m_InstanceNum = desc.type == AccelerationStructureType::TOP_LEVEL ? desc.geometryOrInstanceNum : 0;
    m_Sizes = GetAccelerationStructureSizesMetal(m_Device, desc);
    m_Size = m_Sizes.accelerationStructureSize;

    GetAccelerationStructureMemoryDescMetal(m_Device, m_Type, m_Size, m_InstanceNum, MemoryLocation::DEVICE, m_MemoryDesc, m_HeaderOffset);

    BufferDesc barrierDesc = {};
    barrierDesc.size = m_Size;

    return m_BarrierBuffer.Create(barrierDesc);
}

Result AccelerationStructureMetal::Create(const AccelerationStructureDesc& desc, MemoryLocation location) {
    MaybeUnused(location); // acceleration structures always live in device memory

    Result result = Create(desc);

    if (result != Result::SUCCESS)
        return result;

    return Allocate();
}

Result AccelerationStructureMetal::Allocate() {
    Release();

    m_IsCommitted = true;

    m_AccelerationStructure = m_Device.GetNativeObject()->newAccelerationStructure((NS::UInteger)m_Size);

    if (!m_AccelerationStructure)
        return Result::OUT_OF_MEMORY;

    m_Device.AddResidency(m_AccelerationStructure);

    if (m_Type == AccelerationStructureType::TOP_LEVEL) {
        m_ShaderBindingHeader = m_Device.GetNativeObject()->newBuffer((NS::UInteger)GetTopLevelHeaderSizeMetal(m_InstanceNum), MTL::ResourceStorageModePrivate | MTL::ResourceHazardTrackingModeUntracked);

        if (!m_ShaderBindingHeader)
            return Result::OUT_OF_MEMORY;

        m_Device.AddResidency(m_ShaderBindingHeader);
    }

    return Result::SUCCESS;
}

Result AccelerationStructureMetal::Bind(MemoryMetal& memory, uint64_t offset) {
    Release();

    // Heap residency covers placed resources
    MTL::Heap* heap = memory.GetNativeObject();
    m_AccelerationStructure = heap->newAccelerationStructure((NS::UInteger)m_Size, (NS::UInteger)offset);

    if (!m_AccelerationStructure)
        return Result::FAILURE;

    if (m_Type == AccelerationStructureType::TOP_LEVEL) {
        m_ShaderBindingHeader = heap->newBuffer((NS::UInteger)GetTopLevelHeaderSizeMetal(m_InstanceNum), heap->resourceOptions(), (NS::UInteger)(offset + m_HeaderOffset));

        if (!m_ShaderBindingHeader)
            return Result::FAILURE;
    }

    return Result::SUCCESS;
}

void AccelerationStructureMetal::GetMemoryDesc(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    memoryDesc = m_MemoryDesc;
    memoryDesc.type = (MemoryType)memoryLocation;
}

void AccelerationStructureMetal::SetDebugName(const char* name) {
    NS::String* label = NS::String::alloc()->init(name, NS::UTF8StringEncoding);

    if (m_AccelerationStructure)
        m_AccelerationStructure->setLabel(label);

    if (m_ShaderBindingHeader)
        m_ShaderBindingHeader->setLabel(label);

    label->release();
}

MTL4::AccelerationStructureDescriptor* AccelerationStructureMetal::CreateBuildDescriptor(const BottomLevelGeometryDesc* geometries, uint32_t geometryNum, MTL::GPUAddress convertedInstanceAddress, uint32_t instanceNum) const {
    if (m_Type == AccelerationStructureType::BOTTOM_LEVEL)
        return CreateBottomLevelDescriptorMetal(m_Device, m_Flags, geometries, geometryNum, false);

    return CreateTopLevelDescriptorMetal(m_Flags, convertedInstanceAddress, instanceNum);
}
