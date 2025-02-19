// © 2021 NVIDIA Corporation

#pragma once

NriNamespaceBegin

NriForwardStruct(AccelerationStructure);

NriEnum(GeometryType, uint8_t,
    TRIANGLES,
    AABBS
);

NriEnum(AccelerationStructureType, uint8_t,
    TOP_LEVEL,
    BOTTOM_LEVEL
);

NriEnum(CopyMode, uint8_t,
    CLONE,
    COMPACT
);

NriBits(BottomLevelGeometryBits, uint8_t,
    NONE                                = 0,
    OPAQUE_GEOMETRY                     = NriBit(0),
    NO_DUPLICATE_ANY_HIT_INVOCATION     = NriBit(1)
);

NriBits(TopLevelInstanceBits, uint32_t,
    NONE                                = 0,
    TRIANGLE_CULL_DISABLE               = NriBit(0),
    TRIANGLE_FRONT_COUNTERCLOCKWISE     = NriBit(1),
    FORCE_OPAQUE                        = NriBit(2),
    FORCE_NON_OPAQUE                    = NriBit(3)
);

NriBits(AccelerationStructureBuildBits, uint8_t,
    NONE                                = 0,
    ALLOW_UPDATE                        = NriBit(0),
    ALLOW_COMPACTION                    = NriBit(1),
    PREFER_FAST_TRACE                   = NriBit(2),
    PREFER_FAST_BUILD                   = NriBit(3),
    MINIMIZE_MEMORY                     = NriBit(4),
    ALLOW_DATA_ACCESS                   = NriBit(5)     // requires "isRayTracingPositionFetchSupported"
);

NriStruct(ShaderLibrary) {
    const NriPtr(ShaderDesc) shaders;
    uint32_t shaderNum;
};

NriStruct(ShaderGroupDesc) {
    // Use cases:
    //  - general: RAYGEN_SHADER, MISS_SHADER or CALLABLE_SHADER
    //  - HitGroup: CLOSEST_HIT_SHADER and/or ANY_HIT_SHADER in any order
    //  - HitGroup with an intersection shader: INTERSECTION_SHADER + CLOSEST_HIT_SHADER and/or ANY_HIT_SHADER in any order
    uint32_t shaderIndices[3]; // in ShaderLibrary, starting with 1 (0 - unused)
};

NriStruct(RayTracingPipelineDesc) {
    const NriPtr(PipelineLayout) pipelineLayout;
    const NriPtr(ShaderLibrary) shaderLibrary;
    const NriPtr(ShaderGroupDesc) shaderGroupDescs;
    uint32_t shaderGroupDescNum;
    uint32_t recursionDepthMax;
    uint32_t payloadAttributeSizeMax;
    uint32_t intersectionAttributeSizeMax;
    NriOptional Nri(Robustness) robustness;
};

NriStruct(Triangles) {
    NriPtr(Buffer) vertexBuffer;
    uint64_t vertexOffset;
    uint32_t vertexNum;
    uint64_t vertexStride;
    Nri(Format) vertexFormat;
    NriPtr(Buffer) indexBuffer;
    uint64_t indexOffset;
    uint32_t indexNum;
    Nri(IndexType) indexType;
    NriPtr(Buffer) transformBuffer;
    uint64_t transformOffset;
};

NriStruct(AABBs) {
    NriPtr(Buffer) buffer;
    uint32_t boxNum;
    uint32_t stride;
    uint64_t offset;
};

NriStruct(GeometryObject) {
    Nri(GeometryType) type;
    Nri(BottomLevelGeometryBits) flags;
    union {
        Nri(Triangles) triangles;
        Nri(AABBs) aabbs;
    } geometry;
};

NriStruct(GeometryObjectInstance) {
    float transform[3][4];
    uint32_t instanceId : 24;
    uint32_t mask : 8;
    uint32_t shaderBindingTableLocalOffset : 24;
    Nri(TopLevelInstanceBits) flags : 8;
    uint64_t accelerationStructureHandle;
};

NriStruct(AccelerationStructureDesc) {
    Nri(AccelerationStructureType) type;
    Nri(AccelerationStructureBuildBits) flags;
    uint32_t instanceOrGeometryObjectNum;
    const NriPtr(GeometryObject) geometryObjects; // needed only for BOTTOM_LEVEL
};

NriStruct(AccelerationStructureMemoryBindingDesc) {
    NriPtr(Memory) memory;
    NriPtr(AccelerationStructure) accelerationStructure;
    uint64_t offset;
};

NriStruct(StridedBufferRegion) {
    const NriPtr(Buffer) buffer;
    uint64_t offset;
    uint64_t size;
    uint64_t stride;
};

NriStruct(DispatchRaysDesc) {
    Nri(StridedBufferRegion) raygenShader;
    Nri(StridedBufferRegion) missShaders;
    Nri(StridedBufferRegion) hitShaderGroups;
    Nri(StridedBufferRegion) callableShaders;

    uint32_t x, y, z;
};

NriStruct(DispatchRaysIndirectDesc) {
    uint64_t raygenShaderRecordAddress;
    uint64_t raygenShaderRecordSize;

    uint64_t missShaderBindingTableAddress;
    uint64_t missShaderBindingTableSize;
    uint64_t missShaderBindingTableStride;

    uint64_t hitShaderBindingTableAddress;
    uint64_t hitShaderBindingTableSize;
    uint64_t hitShaderBindingTableStride;

    uint64_t callableShaderBindingTableAddress;
    uint64_t callableShaderBindingTableSize;
    uint64_t callableShaderBindingTableStride;

    uint32_t x, y, z;
};

NriStruct(RayTracingInterface) {
    // Get                    
    uint64_t        (NRI_CALL *GetAccelerationStructureUpdateScratchBufferSize)     (const NriRef(AccelerationStructure) accelerationStructure);
    uint64_t        (NRI_CALL *GetAccelerationStructureBuildScratchBufferSize)      (const NriRef(AccelerationStructure) accelerationStructure);
    uint64_t        (NRI_CALL *GetAccelerationStructureHandle)                      (const NriRef(AccelerationStructure) accelerationStructure);
    NriPtr(Buffer)  (NRI_CALL *GetAccelerationStructureBuffer)                      (const NriRef(AccelerationStructure) accelerationStructure); // needed for barriers
    void            (NRI_CALL *GetAccelerationStructureMemoryDesc)                  (const NriRef(AccelerationStructure) accelerationStructure, Nri(MemoryLocation) memoryLocation, NriOut NriRef(MemoryDesc) memoryDesc);
    void            (NRI_CALL *GetAccelerationStructureMemoryDesc2)                 (const NriRef(Device) device, const NriRef(AccelerationStructureDesc) accelerationStructureDesc,
                                                                                        Nri(MemoryLocation) memoryLocation, NriOut NriRef(MemoryDesc) memoryDesc); // requires "isGetMemoryDesc2Supported"

    // Create
    Nri(Result)     (NRI_CALL *CreateRayTracingPipeline)                            (NriRef(Device) device, const NriRef(RayTracingPipelineDesc) rayTracingPipelineDesc, NriOut NriRef(Pipeline*) pipeline);
    Nri(Result)     (NRI_CALL *CreateAccelerationStructure)                         (NriRef(Device) device, const NriRef(AccelerationStructureDesc) accelerationStructureDesc, NriOut NriRef(AccelerationStructure*) accelerationStructure);
    Nri(Result)     (NRI_CALL *CreateAccelerationStructureDescriptor)               (const NriRef(AccelerationStructure) accelerationStructure, NriOut NriRef(Descriptor*) descriptor);

    // Destroy
    void            (NRI_CALL *DestroyAccelerationStructure)                        (NriRef(AccelerationStructure) accelerationStructure);

    // Memory
    Nri(Result)     (NRI_CALL *BindAccelerationStructureMemory)                     (NriRef(Device) device, const NriPtr(AccelerationStructureMemoryBindingDesc) memoryBindingDescs, uint32_t memoryBindingDescNum);

    // Shader table
    Nri(Result)     (NRI_CALL *WriteShaderGroupIdentifiers)                         (const NriRef(Pipeline) pipeline, uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* buffer); // TODO: add stride

    // Compute
    void            (NRI_CALL *CmdBuildTopLevelAccelerationStructure)               (NriRef(CommandBuffer) commandBuffer, uint32_t instanceNum, const NriRef(Buffer) buffer, uint64_t bufferOffset, Nri(AccelerationStructureBuildBits) flags,
                                                                                        NriRef(AccelerationStructure) dst, NriRef(Buffer) scratch, uint64_t scratchOffset);

    void            (NRI_CALL *CmdBuildBottomLevelAccelerationStructure)            (NriRef(CommandBuffer) commandBuffer, uint32_t geometryObjectNum, const NriPtr(GeometryObject) geometryObjects, Nri(AccelerationStructureBuildBits) flags,
                                                                                        NriRef(AccelerationStructure) dst, NriRef(Buffer) scratch, uint64_t scratchOffset);

    void            (NRI_CALL *CmdUpdateTopLevelAccelerationStructure)              (NriRef(CommandBuffer) commandBuffer, uint32_t instanceNum, const NriRef(Buffer) buffer, uint64_t bufferOffset, Nri(AccelerationStructureBuildBits) flags,
                                                                                        NriRef(AccelerationStructure) dst, const NriRef(AccelerationStructure) src, NriRef(Buffer) scratch, uint64_t scratchOffset);

    void            (NRI_CALL *CmdUpdateBottomLevelAccelerationStructure)           (NriRef(CommandBuffer) commandBuffer, uint32_t geometryObjectNum, const NriPtr(GeometryObject) geometryObjects, Nri(AccelerationStructureBuildBits) flags,
                                                                                        NriRef(AccelerationStructure) dst, const NriRef(AccelerationStructure) src, NriRef(Buffer) scratch, uint64_t scratchOffset);

    void            (NRI_CALL *CmdDispatchRays)                                     (NriRef(CommandBuffer) commandBuffer, const NriRef(DispatchRaysDesc) dispatchRaysDesc);
    void            (NRI_CALL *CmdDispatchRaysIndirect)                             (NriRef(CommandBuffer) commandBuffer, const NriRef(Buffer) buffer, uint64_t offset); // buffer contains "DispatchRaysIndirectDesc" commands

    // Copy
    void            (NRI_CALL *CmdCopyAccelerationStructure)                        (NriRef(CommandBuffer) commandBuffer, NriRef(AccelerationStructure) dst, const NriRef(AccelerationStructure) src, Nri(CopyMode) copyMode);
    void            (NRI_CALL *CmdWriteAccelerationStructureSize)                   (NriRef(CommandBuffer) commandBuffer, const NriPtr(AccelerationStructure) const* accelerationStructures,
                                                                                    uint32_t accelerationStructureNum, NriRef(QueryPool) queryPool, uint32_t queryPoolOffset);

    // Native object
    uint64_t        (NRI_CALL* GetAccelerationStructureNativeObject)                (const NriRef(AccelerationStructure) accelerationStructure); // ID3D12Resource* or VkAccelerationStructureKHR
};

NriNamespaceEnd
