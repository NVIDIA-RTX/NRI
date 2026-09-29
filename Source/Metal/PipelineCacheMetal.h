// © 2026 NVIDIA Corporation

#pragma once

#include <limits.h>

namespace nri {

// A temporary file (the archive API requires a file URL)
struct ArchiveFileMetal {
    char path[PATH_MAX];
};

// A pipeline compiled on a miss, captured in "GetData"
struct PendingPipelineMetal {
    MTL4::PipelineDescriptor* render;
    MTL4::ComputePipelineDescriptor* compute;
    MTL4::PipelineStageDynamicLinkingDescriptor* linking;
};

// Pipelines are looked up in the initial data ("MTL4Archive"), misses are compiled by the device compiler and their descriptors are recorded.
// "GetData" captures recorded pipelines by the cache's compiler ("MTL4PipelineDataSetSerializer") and flushes them into a new archive,
// the data includes all loaded and flushed archives. Converted shaders (DXIL to metallib) are kept in memory and accumulate: "GetData" returns the initial and all added ones
struct PipelineCacheMetal final : public DebugNameBase {
    PipelineCacheMetal(DeviceMetal& device);
    ~PipelineCacheMetal();
    Result Create(const PipelineCacheDesc& desc);
    Result GetData(void* dst, uint64_t& size) const;
    DeviceMetal& GetDevice() const;

    // "linking" is optional. The pipeline is owned by the caller, "nullptr" on failure (a miss if "failOnMiss")
    MTL::ComputePipelineState* NewComputePipeline(const MTL4::ComputePipelineDescriptor* desc, const MTL4::PipelineStageDynamicLinkingDescriptor* linking, bool failOnMiss, NS::Error** error);
    MTL::RenderPipelineState* NewRenderPipeline(const MTL4::PipelineDescriptor* desc, bool failOnMiss, NS::Error** error);

    // Converted shader containers (see "PipelineMetal"). Found data stays valid for the cache's lifetime
    bool FindConvertedShader(uint64_t key, const uint8_t*& data, size_t& size) const;
    void AddConvertedShader(uint64_t key, const uint8_t* data, size_t size);

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    template <typename T, typename Lookup>
    T* FindPipeline(Lookup lookup) const;

    void AddPendingPipeline(const PendingPipelineMetal& pendingPipeline);
    bool CapturePendingPipelines() const;

    DeviceMetal& m_Device;
    MTL4::PipelineDataSetSerializer* m_Serializer = nullptr;
    MTL4::Compiler* m_Compiler = nullptr;
    Vector<MTL4::Archive*> m_Archives;                          // loaded initial data, immutable after "Create"
    mutable Vector<ArchiveFileMetal> m_ArchiveFiles;            // loaded and flushed archives
    mutable Vector<PendingPipelineMetal> m_PendingPipelines;    // owned copies
    UnorderedMap<uint64_t, Vector<uint8_t>> m_ConvertedShaders; // entries are never modified or removed
    mutable Lock m_Lock;                                        // "GetData" ("m_ArchiveFiles", "m_Compiler", "m_Serializer", "m_IsCaptured")
    mutable Lock m_PendingLock;                                 // "m_PendingPipelines"
    mutable Lock m_ConvertedShaderLock;                         // "m_ConvertedShaders"
    mutable bool m_IsCaptured = false;                          // the serializer holds pipelines captured since the last serialization
};

} // namespace nri
