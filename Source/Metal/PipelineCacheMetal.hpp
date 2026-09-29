// © 2026 NVIDIA Corporation

#include <sys/stat.h>
#include <unistd.h>

// "newArchiveWithURL" can crash on malformed data, thus data without a valid header is rejected as "OUT_OF_DATE".
// Layout: header, archives ("uint64_t" size followed by the archive), converted shaders ("ConvertedShaderEntryMetal" followed by the container)
struct PipelineCacheHeaderMetal {
    uint32_t magic;
    uint32_t version;
    uint64_t archiveSize; // all archives
    uint64_t convertedShaderSize;
    uint64_t hash; // of the following data
};

struct ConvertedShaderEntryMetal {
    uint64_t key;
    uint64_t size;
};

constexpr uint32_t PIPELINE_CACHE_MAGIC_METAL = 0x4D34524E; // "NR4M"
constexpr uint32_t PIPELINE_CACHE_VERSION_METAL = 3;
constexpr char TEMP_FILE_NAME_METAL[] = "nri-cache-XXXXXX";

// Reserves a unique file in the user's temporary directory
static bool CreateTempFileMetal(char (&path)[PATH_MAX], const void* data, size_t size) {
    const size_t length = confstr(_CS_DARWIN_USER_TEMP_DIR, path, sizeof(path));

    if (!length || length + sizeof(TEMP_FILE_NAME_METAL) > sizeof(path)) {
        path[0] = 0;

        return false;
    }

    strcat(path, TEMP_FILE_NAME_METAL);
    const int fd = mkstemp(path);

    if (fd == -1) {
        path[0] = 0;

        return false;
    }

    FILE* file = fdopen(fd, "wb");

    if (!file) {
        close(fd);

        return false;
    }

    const bool written = !size || fwrite(data, 1, size, file) == size;

    return !fclose(file) && written;
}

// Cached containers come from "PipelineMetal::ConvertShader", but the blob is untrusted
static bool IsConvertedShaderValidMetal(const uint8_t* container, uint64_t size) {
    ConvertedShaderHeaderMetal header = {};

    if (size < sizeof(header))
        return false;

    memcpy(&header, container, sizeof(header));

    if (header.magic != CONVERTED_SHADER_MAGIC || header.version != CONVERTED_SHADER_VERSION || header.size != size)
        return false;

    const bool isThreadGroupStage = header.stage == StageBits::COMPUTE_SHADER || header.stage == StageBits::MESH_SHADER || header.stage == StageBits::TASK_SHADER;

    if (!isThreadGroupStage && header.stage != StageBits::VERTEX_SHADER && header.stage != StageBits::FRAGMENT_SHADER)
        return false;

    if (isThreadGroupStage && (!header.threadGroupSize[0] || !header.threadGroupSize[1] || !header.threadGroupSize[2]))
        return false;

    auto isStringValid = [&](uint32_t offset) {
        return offset < size && memchr(container + offset, 0, size - offset) != nullptr;
    };

    if (!isStringValid(header.entryPointOffset) || !isStringValid(header.functionNameOffset))
        return false;

    if (header.vertexInputOffset % sizeof(uint32_t) || header.vertexInputOffset + (uint64_t)header.vertexInputNum * sizeof(ConvertedVertexInputMetal) > size)
        return false;

    for (uint32_t i = 0; i < header.vertexInputNum; i++) {
        ConvertedVertexInputMetal input = {};
        memcpy(&input, container + header.vertexInputOffset + i * sizeof(input), sizeof(input));

        if (!isStringValid(input.nameOffset) || input.attributeIndex >= CONVERTED_VERTEX_ATTRIBUTE_NUM)
            return false;
    }

    return header.metallibSize && header.metallibOffset + (uint64_t)header.metallibSize <= size;
}

static inline NS::URL* GetFileUrlMetal(const char* path) {
    return NS::URL::fileURLWithPath(NS::String::string(path, NS::UTF8StringEncoding));
}

PipelineCacheMetal::PipelineCacheMetal(DeviceMetal& device)
    : m_Device(device), m_Archives(device.GetStdAllocator()), m_ArchiveFiles(device.GetStdAllocator()), m_PendingPipelines(device.GetStdAllocator()), m_ConvertedShaders(device.GetStdAllocator()) {
}

static void ReleasePendingPipelineMetal(const PendingPipelineMetal& pendingPipeline) {
    if (pendingPipeline.render)
        pendingPipeline.render->release();

    if (pendingPipeline.compute)
        pendingPipeline.compute->release();

    if (pendingPipeline.linking)
        pendingPipeline.linking->release();
}

PipelineCacheMetal::~PipelineCacheMetal() {
    for (const PendingPipelineMetal& pendingPipeline : m_PendingPipelines)
        ReleasePendingPipelineMetal(pendingPipeline);

    for (MTL4::Archive* archive : m_Archives)
        archive->release();

    if (m_Compiler)
        m_Compiler->release();

    if (m_Serializer)
        m_Serializer->release();

    for (const ArchiveFileMetal& file : m_ArchiveFiles)
        unlink(file.path);
}

Result PipelineCacheMetal::Create(const PipelineCacheDesc& desc) {
    AutoreleasePoolMetal autoreleasePool; // "NS::Error" is autoreleased

    MTL4::PipelineDataSetSerializerDescriptor* serializerDesc = MTL4::PipelineDataSetSerializerDescriptor::alloc()->init();
    serializerDesc->setConfiguration(MTL4::PipelineDataSetSerializerConfigurationCaptureBinaries);
    m_Serializer = m_Device.GetNativeObject()->newPipelineDataSetSerializer(serializerDesc);
    serializerDesc->release();

    if (!m_Serializer)
        return Result::FAILURE;

    MTL4::CompilerDescriptor* compilerDesc = MTL4::CompilerDescriptor::alloc()->init();
    compilerDesc->setPipelineDataSetSerializer(m_Serializer);

    NS::Error* error = nullptr;
    m_Compiler = m_Device.GetNativeObject()->newCompiler(compilerDesc, &error);
    compilerDesc->release();

    if (!m_Compiler) {
        NRI_REPORT_ERROR(&m_Device, "newCompiler() failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");

        return Result::FAILURE;
    }

    if (!desc.data || !desc.size)
        return Result::SUCCESS;

    PipelineCacheHeaderMetal header = {};

    if (desc.size < sizeof(header))
        return Result::OUT_OF_DATE;

    memcpy(&header, desc.data, sizeof(header));

    const uint8_t* archives = (const uint8_t*)desc.data + sizeof(header);
    const size_t dataSize = desc.size - sizeof(header);

    if (header.magic != PIPELINE_CACHE_MAGIC_METAL || header.version != PIPELINE_CACHE_VERSION_METAL || header.archiveSize > dataSize || header.convertedShaderSize != dataSize - header.archiveSize || header.hash != HashMetal(archives, dataSize))
        return Result::OUT_OF_DATE;

    for (size_t offset = 0; offset < header.archiveSize;) {
        uint64_t archiveSize = 0;

        if (header.archiveSize - offset < sizeof(archiveSize))
            return Result::OUT_OF_DATE;

        memcpy(&archiveSize, archives + offset, sizeof(archiveSize));
        offset += sizeof(archiveSize);

        if (!archiveSize || archiveSize > header.archiveSize - offset)
            return Result::OUT_OF_DATE;

        offset += archiveSize;
    }

    const uint8_t* convertedShaders = archives + header.archiveSize;

    for (size_t offset = 0; offset < header.convertedShaderSize;) {
        ConvertedShaderEntryMetal entry = {};

        if (header.convertedShaderSize - offset < sizeof(entry))
            return Result::OUT_OF_DATE;

        memcpy(&entry, convertedShaders + offset, sizeof(entry));
        offset += sizeof(entry);

        const uint8_t* container = convertedShaders + offset;

        if (entry.size > header.convertedShaderSize - offset || !IsConvertedShaderValidMetal(container, entry.size))
            return Result::OUT_OF_DATE;

        m_ConvertedShaders.emplace(entry.key, Vector<uint8_t>(container, container + entry.size, m_Device.GetStdAllocator()));
        offset += entry.size;
    }

    for (size_t offset = 0; offset < header.archiveSize;) {
        uint64_t archiveSize = 0;
        memcpy(&archiveSize, archives + offset, sizeof(archiveSize));

        const uint8_t* archiveData = archives + offset + sizeof(archiveSize);
        offset += sizeof(archiveSize) + archiveSize;

        ArchiveFileMetal file = {};

        if (!CreateTempFileMetal(file.path, archiveData, archiveSize)) {
            if (file.path[0])
                unlink(file.path);

            return Result::FAILURE;
        }

        // Incompatible archives (i.e. from another OS version) are reported via "error" and dropped. Converted shaders are still usable
        MTL4::Archive* archive = m_Device.GetNativeObject()->newArchive(GetFileUrlMetal(file.path), &error);

        if (!archive) {
            NRI_REPORT_WARNING(&m_Device, "Pipeline cache archive is out of date: %s", error ? error->localizedDescription()->utf8String() : "unknown error");
            unlink(file.path);

            continue;
        }

        m_Archives.push_back(archive);
        m_ArchiveFiles.push_back(file);
    }

    return Result::SUCCESS;
}

template <typename T, typename Lookup>
T* PipelineCacheMetal::FindPipeline(Lookup lookup) const {
    for (MTL4::Archive* archive : m_Archives) {
        NS::Error* lookupError = nullptr;
        T* pipeline = lookup(archive, &lookupError);

        if (pipeline)
            return pipeline;
    }

    return nullptr;
}

void PipelineCacheMetal::AddPendingPipeline(const PendingPipelineMetal& pendingPipeline) {
    ExclusiveScope lock(m_PendingLock);
    m_PendingPipelines.push_back(pendingPipeline);
}

// A capturing compiler doesn't populate the system shader cache, but uses it. Thus misses are compiled once by the device compiler, and captures in "GetData"
// are usually system shader cache hits. "CaptureDescriptors" is not enough, since "serializeAsArchiveAndFlushToURL" requires "CaptureBinaries"
bool PipelineCacheMetal::CapturePendingPipelines() const {
    Vector<PendingPipelineMetal> pendingPipelines(m_Device.GetStdAllocator());
    {
        ExclusiveScope lock(m_PendingLock);
        pendingPipelines.swap(m_PendingPipelines);
    }

    for (const PendingPipelineMetal& pendingPipeline : pendingPipelines) {
        NS::Error* error = nullptr;
        NS::Object* pipeline = nullptr;

        if (pendingPipeline.render)
            pipeline = m_Compiler->newRenderPipelineState(pendingPipeline.render, nullptr, &error);
        else if (pendingPipeline.linking)
            pipeline = m_Compiler->newComputePipelineState(pendingPipeline.compute, pendingPipeline.linking, nullptr, &error);
        else
            pipeline = m_Compiler->newComputePipelineState(pendingPipeline.compute, nullptr, &error);

        if (pipeline) {
            pipeline->release();
            m_IsCaptured = true;
        } else // the pipeline is not cached
            NRI_REPORT_WARNING(&m_Device, "Pipeline cache update failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");

        ReleasePendingPipelineMetal(pendingPipeline);
    }

    return m_IsCaptured;
}

bool PipelineCacheMetal::FindConvertedShader(uint64_t key, const uint8_t*& data, size_t& size) const {
    ExclusiveScope lock(m_ConvertedShaderLock);

    const auto it = m_ConvertedShaders.find(key);

    if (it == m_ConvertedShaders.end())
        return false;

    data = it->second.data();
    size = it->second.size();

    return true;
}

void PipelineCacheMetal::AddConvertedShader(uint64_t key, const uint8_t* data, size_t size) {
    Vector<uint8_t> container(data, data + size, m_Device.GetStdAllocator());

    ExclusiveScope lock(m_ConvertedShaderLock);
    m_ConvertedShaders.emplace(key, std::move(container));
}

MTL::ComputePipelineState* PipelineCacheMetal::NewComputePipeline(const MTL4::ComputePipelineDescriptor* desc, const MTL4::PipelineStageDynamicLinkingDescriptor* linking, bool failOnMiss, NS::Error** error) {
    MTL::ComputePipelineState* pipeline = FindPipeline<MTL::ComputePipelineState>([&](MTL4::Archive* archive, NS::Error** lookupError) {
        return linking ? archive->newComputePipelineState(desc, linking, lookupError) : archive->newComputePipelineState(desc, lookupError);
    });

    if (pipeline || failOnMiss)
        return pipeline;

    MTL4::Compiler* compiler = m_Device.GetCompiler();
    pipeline = linking ? compiler->newComputePipelineState(desc, linking, nullptr, error) : compiler->newComputePipelineState(desc, nullptr, error);

    if (pipeline)
        AddPendingPipeline({nullptr, desc->copy(), linking ? linking->copy() : nullptr});

    return pipeline;
}

MTL::RenderPipelineState* PipelineCacheMetal::NewRenderPipeline(const MTL4::PipelineDescriptor* desc, bool failOnMiss, NS::Error** error) {
    MTL::RenderPipelineState* pipeline = FindPipeline<MTL::RenderPipelineState>([&](MTL4::Archive* archive, NS::Error** lookupError) {
        return archive->newRenderPipelineState(desc, lookupError);
    });

    if (pipeline || failOnMiss)
        return pipeline;

    pipeline = m_Device.GetCompiler()->newRenderPipelineState(desc, nullptr, error);

    if (pipeline)
        AddPendingPipeline({desc->copy(), nullptr, nullptr});

    return pipeline;
}

Result PipelineCacheMetal::GetData(void* dst, uint64_t& size) const {
    ExclusiveScope lock(m_Lock);

    // "Flush" clears captured pipelines, thus each serialization is a new archive. The serializer refuses to serialize if nothing has been captured
    {
        AutoreleasePoolMetal autoreleasePool; // "NS::Error" is autoreleased

        if (CapturePendingPipelines()) {
            ArchiveFileMetal file = {};
            NS::Error* error = nullptr;

            if (!CreateTempFileMetal(file.path, nullptr, 0) || !m_Serializer->serializeAsArchiveAndFlushToURL(GetFileUrlMetal(file.path), &error)) {
                if (file.path[0])
                    unlink(file.path);

                NRI_REPORT_ERROR(&m_Device, "Pipeline cache serialization failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");

                return Result::FAILURE;
            }

            m_ArchiveFiles.push_back(file);
            m_IsCaptured = false;
        }
    }

    uint64_t archiveSize = 0;

    for (const ArchiveFileMetal& file : m_ArchiveFiles) {
        struct stat fileStat = {};

        if (stat(file.path, &fileStat))
            return Result::FAILURE;

        archiveSize += sizeof(uint64_t) + (uint64_t)fileStat.st_size;
    }

    // Converted shaders follow the archives
    uint8_t* archives = dst ? (uint8_t*)dst + sizeof(PipelineCacheHeaderMetal) : nullptr;
    uint64_t convertedShaderSize = 0;
    uint64_t dataSize = 0;
    {
        ExclusiveScope convertedShaderLock(m_ConvertedShaderLock);

        for (const auto& it : m_ConvertedShaders)
            convertedShaderSize += sizeof(ConvertedShaderEntryMetal) + it.second.size();

        dataSize = archiveSize || convertedShaderSize ? sizeof(PipelineCacheHeaderMetal) + archiveSize + convertedShaderSize : 0;

        if (archives && dataSize && size >= dataSize) {
            uint8_t* convertedShaders = archives + archiveSize;

            for (const auto& it : m_ConvertedShaders) {
                const ConvertedShaderEntryMetal entry = {it.first, it.second.size()};
                memcpy(convertedShaders, &entry, sizeof(entry));
                memcpy(convertedShaders + sizeof(entry), it.second.data(), it.second.size());
                convertedShaders += sizeof(entry) + it.second.size();
            }
        }
    }

    size_t offset = 0;

    for (size_t i = 0; archives && dataSize && size >= dataSize && i < m_ArchiveFiles.size(); i++) {
        FILE* file = fopen(m_ArchiveFiles[i].path, "rb");

        if (!file)
            return Result::FAILURE;

        const bool isRead = !fseek(file, 0, SEEK_END) && ftell(file) >= 0;
        const uint64_t fileSize = isRead ? (uint64_t)ftell(file) : 0;
        rewind(file);

        const bool isCopied = isRead && offset + sizeof(fileSize) + fileSize <= archiveSize && fread(archives + offset + sizeof(fileSize), 1, fileSize, file) == fileSize;
        fclose(file);

        if (!isCopied)
            return Result::FAILURE;

        memcpy(archives + offset, &fileSize, sizeof(fileSize));
        offset += sizeof(fileSize) + fileSize;
    }

    Result result = Result::SUCCESS;

    if (archives && dataSize) {
        if (size < dataSize)
            result = Result::OUT_OF_MEMORY;
        else if (offset != archiveSize)
            result = Result::FAILURE;
        else {
            PipelineCacheHeaderMetal header = {};
            header.magic = PIPELINE_CACHE_MAGIC_METAL;
            header.version = PIPELINE_CACHE_VERSION_METAL;
            header.archiveSize = archiveSize;
            header.convertedShaderSize = convertedShaderSize;
            header.hash = HashMetal(archives, (size_t)(dataSize - sizeof(header)));

            memcpy(dst, &header, sizeof(header));
        }
    }

    size = dataSize;

    return result;
}

DeviceMetal& PipelineCacheMetal::GetDevice() const {
    return m_Device;
}

void PipelineCacheMetal::SetDebugName(const char* name) {
    // The compiler label is immutable, archive ones aren't
    NS::String* label = NS::String::alloc()->init(name, NS::UTF8StringEncoding);

    for (MTL4::Archive* archive : m_Archives)
        archive->setLabel(label);

    label->release();
}
