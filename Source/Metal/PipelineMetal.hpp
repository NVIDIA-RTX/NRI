// © 2026 NVIDIA Corporation

#if NRI_ENABLE_METAL_SHADER_CONVERTER
#    include <dlfcn.h>
#    include <mach-o/loader.h>
#endif

constexpr uint8_t VERTEX_ATTRIBUTE_UNUSED = 0xFF;

#if NRI_ENABLE_METAL_SHADER_CONVERTER
// Indexed by the "StageBits" bit index (from "INDEX_INPUT" to "CALLABLE_SHADER")
constexpr std::array<IRShaderStage, 18> g_IRShaderStages = {
    IRShaderStageInvalid,       // INDEX_INPUT
    IRShaderStageVertex,        // VERTEX_SHADER
    IRShaderStageHull,          // TESS_CONTROL_SHADER
    IRShaderStageDomain,        // TESS_EVALUATION_SHADER
    IRShaderStageGeometry,      // GEOMETRY_SHADER
    IRShaderStageAmplification, // TASK_SHADER
    IRShaderStageMesh,          // MESH_SHADER
    IRShaderStageFragment,      // FRAGMENT_SHADER
    IRShaderStageInvalid,       // DEPTH_STENCIL_ATTACHMENT
    IRShaderStageInvalid,       // COLOR_ATTACHMENT
    IRShaderStageInvalid,       // SHADING_RATE_ATTACHMENT
    IRShaderStageCompute,       // COMPUTE_SHADER
    IRShaderStageRayGeneration, // RAYGEN_SHADER
    IRShaderStageMiss,          // MISS_SHADER
    IRShaderStageIntersection,  // INTERSECTION_SHADER
    IRShaderStageClosestHit,    // CLOSEST_HIT_SHADER
    IRShaderStageAnyHit,        // ANY_HIT_SHADER
    IRShaderStageCallable,      // CALLABLE_SHADER
};
NRI_VALIDATE_ARRAY(g_IRShaderStages);
static_assert((uint32_t)StageBits::CALLABLE_SHADER == 1u << 17, "Unexpected 'StageBits' layout");

static inline IRShaderStage GetIRShaderStage(StageBits stage) {
    const uint32_t index = (uint32_t)__builtin_ctz((uint32_t)stage);

    return index < g_IRShaderStages.size() ? g_IRShaderStages[index] : IRShaderStageInvalid;
}

// Stage-in synthesis for geometry/tessellation emulation. Vertex formats precede "R9_G9_B9_E5_UFLOAT", which Converter can't synthesize.
// "UNKNOWN" entries are rejected at pipeline creation
constexpr std::array<IRFormat, (size_t)Format::R9_G9_B9_E5_UFLOAT> g_IRVertexFormats = {
    IRFormatUnknown,           // UNKNOWN
    IRFormatR8Unorm,           // R8_UNORM
    IRFormatR8Snorm,           // R8_SNORM
    IRFormatR8Uint,            // R8_UINT
    IRFormatR8Sint,            // R8_SINT
    IRFormatR8G8Unorm,         // RG8_UNORM
    IRFormatR8G8Snorm,         // RG8_SNORM
    IRFormatR8G8Uint,          // RG8_UINT
    IRFormatR8G8Sint,          // RG8_SINT
    IRFormatB8G8R8A8Unorm,     // BGRA8_UNORM
    IRFormatUnknown,           // BGRA8_SRGB
    IRFormatR8G8B8A8Unorm,     // RGBA8_UNORM
    IRFormatUnknown,           // RGBA8_SRGB
    IRFormatR8G8B8A8Snorm,     // RGBA8_SNORM
    IRFormatR8G8B8A8Uint,      // RGBA8_UINT
    IRFormatR8G8B8A8Sint,      // RGBA8_SINT
    IRFormatR16Unorm,          // R16_UNORM
    IRFormatR16Snorm,          // R16_SNORM
    IRFormatR16Uint,           // R16_UINT
    IRFormatR16Sint,           // R16_SINT
    IRFormatR16Float,          // R16_SFLOAT
    IRFormatR16G16Unorm,       // RG16_UNORM
    IRFormatR16G16Snorm,       // RG16_SNORM
    IRFormatR16G16Uint,        // RG16_UINT
    IRFormatR16G16Sint,        // RG16_SINT
    IRFormatR16G16Float,       // RG16_SFLOAT
    IRFormatR16G16B16A16Unorm, // RGBA16_UNORM
    IRFormatR16G16B16A16Snorm, // RGBA16_SNORM
    IRFormatR16G16B16A16Uint,  // RGBA16_UINT
    IRFormatR16G16B16A16Sint,  // RGBA16_SINT
    IRFormatR16G16B16A16Float, // RGBA16_SFLOAT
    IRFormatR32Uint,           // R32_UINT
    IRFormatR32Sint,           // R32_SINT
    IRFormatR32Float,          // R32_SFLOAT
    IRFormatR32G32Uint,        // RG32_UINT
    IRFormatR32G32Sint,        // RG32_SINT
    IRFormatR32G32Float,       // RG32_SFLOAT
    IRFormatR32G32B32Uint,     // RGB32_UINT
    IRFormatR32G32B32Sint,     // RGB32_SINT
    IRFormatR32G32B32Float,    // RGB32_SFLOAT
    IRFormatR32G32B32A32Uint,  // RGBA32_UINT
    IRFormatR32G32B32A32Sint,  // RGBA32_SINT
    IRFormatR32G32B32A32Float, // RGBA32_SFLOAT
    IRFormatUnknown,           // B5_G6_R5_UNORM
    IRFormatUnknown,           // B5_G5_R5_A1_UNORM
    IRFormatUnknown,           // B4_G4_R4_A4_UNORM
    IRFormatR10G10B10A2Unorm,  // R10_G10_B10_A2_UNORM
    IRFormatUnknown,           // R10_G10_B10_A2_UINT (no Metal vertex format)
    IRFormatR11G11B10Float,    // R11_G11_B10_UFLOAT
};
NRI_VALIDATE_ARRAY(g_IRVertexFormats);

static inline IRFormat GetIRVertexFormat(Format format) {
    return (size_t)format < g_IRVertexFormats.size() ? g_IRVertexFormats[(size_t)format] : IRFormatUnknown;
}

constexpr IROperatingSystem CONVERTER_OPERATING_SYSTEM = IROperatingSystem_macOS;

constexpr const char* CONVERTER_DEPLOYMENT_TARGET = "26.0.0"; // Metal 4

constexpr std::array<IRInputTopology, (size_t)Topology::MAX_NUM> g_InputTopologies = {
    IRInputTopologyPoint,    // POINT_LIST
    IRInputTopologyLine,     // LINE_LIST
    IRInputTopologyLine,     // LINE_STRIP
    IRInputTopologyTriangle, // TRIANGLE_LIST
    IRInputTopologyTriangle, // TRIANGLE_STRIP
    IRInputTopologyLine,     // LINE_LIST_WITH_ADJACENCY
    IRInputTopologyLine,     // LINE_STRIP_WITH_ADJACENCY
    IRInputTopologyTriangle, // TRIANGLE_LIST_WITH_ADJACENCY
    IRInputTopologyTriangle, // TRIANGLE_STRIP_WITH_ADJACENCY
    IRInputTopologyPatch,    // PATCH_LIST
};
NRI_VALIDATE_ARRAY(g_InputTopologies);

constexpr std::array<IRRuntimePrimitiveType, (size_t)Topology::MAX_NUM> g_EmulationPrimitives = {
    IRRuntimePrimitiveTypePoint,                  // POINT_LIST
    IRRuntimePrimitiveTypeLine,                   // LINE_LIST
    IRRuntimePrimitiveTypeLineStrip,              // LINE_STRIP
    IRRuntimePrimitiveTypeTriangle,               // TRIANGLE_LIST
    IRRuntimePrimitiveTypeTriangleStrip,          // TRIANGLE_STRIP
    IRRuntimePrimitiveTypeLineWithAdj,            // LINE_LIST_WITH_ADJACENCY
    IRRuntimePrimitiveTypeLineStripWithAdj,       // LINE_STRIP_WITH_ADJACENCY
    IRRuntimePrimitiveTypeTriangleWithAdj,        // TRIANGLE_LIST_WITH_ADJACENCY
    IRRuntimePrimitiveTypeTriangleWithAdj,        // TRIANGLE_STRIP_WITH_ADJACENCY (unsupported)
    IRRuntimePrimitiveType1ControlPointPatchlist, // PATCH_LIST
};
NRI_VALIDATE_ARRAY(g_EmulationPrimitives);

static inline IRGPUFamily GetIRGPUFamily(MTL::Device& device) {
    static constexpr std::pair<MTL::GPUFamily, IRGPUFamily> families[] = {
        {MTL::GPUFamilyApple10, IRGPUFamilyApple10},
        {MTL::GPUFamilyApple9, IRGPUFamilyApple9},
        {MTL::GPUFamilyApple8, IRGPUFamilyApple8},
        {MTL::GPUFamilyApple7, IRGPUFamilyApple7},
        {MTL::GPUFamilyApple6, IRGPUFamilyApple6},
    };

    for (const auto& family : families) {
        if (device.supportsFamily(family.first))
            return family.second;
    }

    return IRGPUFamilyMetal3;
}

// Samplers apply "mipBias" natively only on Apple10+, earlier GPUs take it from descriptor metadata (see "DescriptorMetal")
static inline IRCompatibilityFlags GetIRCompatibilityFlags(MTL::Device& device) {
    return device.supportsFamily(MTL::GPUFamilyApple10) ? IRCompatibilityFlagNone : IRCompatibilityFlagSamplerLODBias;
}

// Bump if NRI conversion settings change (converted shaders are persistent, see "PipelineCacheMetal")
constexpr uint32_t CONVERSION_REVISION = 1;

// Converted shaders are identified by the Converter header version and the "LC_UUID" of the loaded library
static uint64_t GetConverterHash(DeviceMetal& device) {
    static const uint64_t s_Hash = [&device] {
        const uint32_t version[] = {CONVERSION_REVISION, IR_VERSION_MAJOR, IR_VERSION_MINOR, IR_VERSION_PATCH};
        uint64_t hash = HashMetal(version, sizeof(version));

        Dl_info info = {};

        if (!dladdr((const void*)&IRCompilerCreate, &info) || !info.dli_fbase) {
            NRI_REPORT_WARNING(&device, "Metal Shader Converter image is unknown, cached conversions may be stale after a Converter update");

            return hash;
        }

        const mach_header_64* image = (const mach_header_64*)info.dli_fbase;
        const uint8_t* command = (const uint8_t*)(image + 1);

        for (uint32_t i = 0; image->magic == MH_MAGIC_64 && i < image->ncmds; i++) {
            load_command loadCommand = {};
            memcpy(&loadCommand, command, sizeof(loadCommand));

            if (loadCommand.cmd == LC_UUID) {
                uuid_command uuidCommand = {};
                memcpy(&uuidCommand, command, sizeof(uuidCommand));

                return HashMetal(uuidCommand.uuid, sizeof(uuidCommand.uuid), hash);
            }

            command += loadCommand.cmdsize;
        }

        NRI_REPORT_WARNING(&device, "Metal Shader Converter image has no 'LC_UUID', cached conversions may be stale after a Converter update");

        return hash;
    }();

    return s_Hash;
}

// Shared by graphics and compute pipelines
static inline IRCompiler* CreateIRCompiler(MTL::Device& device, const PipelineLayoutMetal& layout) {
    IRCompiler* compiler = IRCompilerCreate();
    IRCompilerSetCompatibilityFlags(compiler, GetIRCompatibilityFlags(device));
    IRCompilerSetMinimumGPUFamily(compiler, GetIRGPUFamily(device));
    IRCompilerSetMinimumDeploymentTarget(compiler, CONVERTER_OPERATING_SYSTEM, CONVERTER_DEPLOYMENT_TARGET);
    IRCompilerSetGlobalRootSignature(compiler, layout.GetRootSignature());

    return compiler;
}

#endif

static inline bool IsDXIL(const ShaderDesc& shader) {
    return shader.size >= 4 && memcmp(shader.bytecode, "DXBC", 4) == 0;
}

static_assert(sizeof(ConvertedShaderHeaderMetal) == 80 && sizeof(ConvertedVertexInputMetal) == 8, "Converted shader layout is fixed");

// ShaderMake Metal converter bundle (see "NRIWrapperMetal.h")
static inline bool IsMetalBundle(const ShaderDesc& shader) {
    return shader.size >= 4 && memcmp(shader.bytecode, "SMMB", 4) == 0;
}

// DXIL or pre-converted DXIL (uses Converter's binding ABI)
static inline bool IsConvertedShader(const ShaderDesc& shader) {
    return IsDXIL(shader) || IsMetalBundle(shader);
}

struct ConvertedVertexInputDescMetal {
    const char* name;
    size_t nameLength;
    uint32_t attributeIndex;
};

// Fills "header" offsets and sizes, returns the metallib destination
static uint8_t* WriteConvertedShader(Vector<uint8_t>& storage, ConvertedShaderHeaderMetal& header, const char* entryPoint, const char* functionName, size_t functionNameLength,
    const Vector<ConvertedVertexInputDescMetal>& vertexInputs, size_t metallibSize) {
    auto append = [&](const char* string, size_t length) {
        const uint32_t offset = (uint32_t)storage.size();
        storage.insert(storage.end(), (const uint8_t*)string, (const uint8_t*)string + length);
        storage.push_back(0);

        return offset;
    };

    header.magic = CONVERTED_SHADER_MAGIC;
    header.version = CONVERTED_SHADER_VERSION;
    header.vertexInputOffset = sizeof(header);
    header.vertexInputNum = (uint32_t)vertexInputs.size();

    storage.resize(sizeof(header) + vertexInputs.size() * sizeof(ConvertedVertexInputMetal));
    header.entryPointOffset = append(entryPoint, strlen(entryPoint));
    header.functionNameOffset = append(functionName, functionNameLength);

    for (size_t i = 0; i < vertexInputs.size(); i++) {
        ConvertedVertexInputMetal vertexInput = {};
        vertexInput.nameOffset = append(vertexInputs[i].name, vertexInputs[i].nameLength);
        vertexInput.attributeIndex = vertexInputs[i].attributeIndex;

        memcpy(storage.data() + header.vertexInputOffset + i * sizeof(vertexInput), &vertexInput, sizeof(vertexInput));
    }

    storage.resize(Align(storage.size(), sizeof(uint64_t)));
    header.metallibOffset = (uint32_t)storage.size();
    header.metallibSize = (uint32_t)metallibSize;
    storage.resize(storage.size() + metallibSize);

    header.size = (uint32_t)storage.size();
    memcpy(storage.data(), &header, sizeof(header));

    return storage.data() + header.metallibOffset;
}

// Minimal reader for Converter reflection JSON. Values are "[begin, end)" spans, escaped strings are not supported
struct JsonValueMetal {
    const char* begin;
    const char* end;
};

static inline const char* SkipJsonSpaces(const char* s, const char* end) {
    while (s < end && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n'))
        s++;

    return s;
}

// Returns the end of the value at "s" or "nullptr"
static const char* SkipJsonValue(const char* s, const char* end, uint32_t depth);

// Calls "callback(key, value)" for object members or "callback(value, value)" for array elements
template <typename F>
static bool ForEachJsonValue(JsonValueMetal container, F callback, uint32_t depth = 0) {
    const char* s = container.begin;

    if (s >= container.end || (*s != '{' && *s != '['))
        return false;

    const bool isObject = *s == '{';
    const char close = isObject ? '}' : ']';
    s = SkipJsonSpaces(s + 1, container.end);

    if (s < container.end && *s == close)
        return true;

    while (s < container.end) {
        JsonValueMetal key = {s, nullptr};

        if (isObject) {
            if (*s != '"')
                return false;

            key.end = SkipJsonValue(s, container.end, depth + 1);
            s = key.end ? SkipJsonSpaces(key.end, container.end) : container.end;

            if (s >= container.end || *s != ':')
                return false;

            s = SkipJsonSpaces(s + 1, container.end);
        }

        JsonValueMetal value = {s, SkipJsonValue(s, container.end, depth + 1)};

        if (!value.end || !callback(isObject ? key : value, value))
            return false;

        s = SkipJsonSpaces(value.end, container.end);

        if (s < container.end && *s == close)
            return true;

        if (s >= container.end || *s != ',')
            return false;

        s = SkipJsonSpaces(s + 1, container.end);
    }

    return false;
}

static const char* SkipJsonValue(const char* s, const char* end, uint32_t depth) {
    if (s >= end || depth > 16)
        return nullptr;

    if (*s == '"') {
        for (s++; s < end; s++) {
            if (*s == '\\')
                s++;
            else if (*s == '"')
                return s + 1;
        }

        return nullptr;
    }

    if (*s == '{' || *s == '[') {
        const char close = *s == '{' ? '}' : ']';
        const char* last = nullptr;

        if (!ForEachJsonValue({s, end}, [&](JsonValueMetal, JsonValueMetal value) { last = value.end; return true; }, depth))
            return nullptr;

        s = SkipJsonSpaces(last ? last : s + 1, end);

        return s < end && *s == close ? s + 1 : nullptr;
    }

    // Numbers and literals
    const char* begin = s;

    while (s < end && (isalnum((unsigned char)*s) || *s == '-' || *s == '+' || *s == '.'))
        s++;

    return s != begin ? s : nullptr;
}

static inline bool FindJsonMember(JsonValueMetal object, const char* name, JsonValueMetal& value) {
    const size_t nameLength = strlen(name);
    bool isFound = false;

    ForEachJsonValue(object, [&](JsonValueMetal key, JsonValueMetal member) {
        isFound = size_t(key.end - key.begin) == nameLength + 2 && !memcmp(key.begin + 1, name, nameLength);

        if (isFound)
            value = member;

        return !isFound;
    });

    return isFound;
}

static inline bool GetJsonString(JsonValueMetal value, const char*& string, size_t& length) {
    if (value.end - value.begin < 2 || *value.begin != '"' || memchr(value.begin, '\\', value.end - value.begin))
        return false;

    string = value.begin + 1;
    length = value.end - value.begin - 2;

    return true;
}

static inline bool GetJsonUint(JsonValueMetal value, uint32_t& result) {
    const ptrdiff_t length = value.end - value.begin;
    uint64_t number = 0;

    if (length < 1 || length > 10)
        return false;

    for (const char* s = value.begin; s < value.end; s++) {
        if (*s < '0' || *s > '9')
            return false;

        number = number * 10 + uint64_t(*s - '0');
    }

    result = (uint32_t)number;

    return number <= UINT32_MAX;
}

static inline bool GetJsonMemberString(JsonValueMetal object, const char* name, const char*& string, size_t& length) {
    JsonValueMetal value = {};

    return FindJsonMember(object, name, value) && GetJsonString(value, string, length);
}

static inline bool GetJsonMemberUint(JsonValueMetal object, const char* name, uint32_t& result) {
    JsonValueMetal value = {};

    return FindJsonMember(object, name, value) && GetJsonUint(value, result);
}

static inline bool GetJsonMemberUints(JsonValueMetal object, const char* name, uint32_t* results, uint32_t num) {
    JsonValueMetal value = {};
    uint32_t i = 0;

    if (!FindJsonMember(object, name, value))
        return false;

    const bool isParsed = ForEachJsonValue(value, [&](JsonValueMetal, JsonValueMetal element) {
        return i < num && GetJsonUint(element, results[i++]);
    });

    return isParsed && i == num;
}

// Reflection names converted vertex inputs as lower-case "semantic name + semantic index", i.e. "texcoord1".
// Without reflected inputs the default slots (array order) are kept
static inline void MapConvertedVertexAttributes(const VertexInputDesc& vertexInput, const uint8_t* container, const ConvertedShaderHeaderMetal& header, uint8_t* slots) {
    if (!header.vertexInputNum)
        return;

    for (uint32_t i = 0; i < vertexInput.attributeNum; i++) {
        const VertexAttributeDesc& attribute = vertexInput.attributes[i];
        const char* semanticName = attribute.d3d.semanticName ? attribute.d3d.semanticName : "";
        const size_t semanticNameLength = strlen(semanticName);

        char semanticIndex[16];
        snprintf(semanticIndex, sizeof(semanticIndex), "%u", attribute.d3d.semanticIndex);

        slots[i] = VERTEX_ATTRIBUTE_UNUSED;

        for (uint32_t j = 0; j < header.vertexInputNum; j++) {
            ConvertedVertexInputMetal input = {};
            memcpy(&input, container + header.vertexInputOffset + j * sizeof(input), sizeof(input));

            const char* name = (const char*)container + input.nameOffset;

            if (!strncasecmp(name, semanticName, semanticNameLength) && !strcmp(name + semanticNameLength, semanticIndex)) {
                slots[i] = uint8_t(CONVERTED_VERTEX_ATTRIBUTE_BASE + input.attributeIndex);
                break;
            }
        }
    }
}

static inline bool IsDualSourceBlendFactor(BlendFactor factor) {
    return factor == BlendFactor::SRC1_COLOR || factor == BlendFactor::ONE_MINUS_SRC1_COLOR || factor == BlendFactor::SRC1_ALPHA || factor == BlendFactor::ONE_MINUS_SRC1_ALPHA;
}

static inline MTL::Size GetNativeThreadGroupSize(const ShaderDesc& shader) {
    // Validation requires non-zero sizes for native compute, mesh and task shaders
    return MTL::Size(std::max(shader.threadGroupSizeX, (Dim_t)1), std::max(shader.threadGroupSizeY, (Dim_t)1), std::max(shader.threadGroupSizeZ, (Dim_t)1));
}

constexpr std::array<MTL::BlendFactor, (size_t)BlendFactor::MAX_NUM> g_BlendFactors = {
    MTL::BlendFactorZero,                     // ZERO
    MTL::BlendFactorOne,                      // ONE
    MTL::BlendFactorSourceColor,              // SRC_COLOR
    MTL::BlendFactorOneMinusSourceColor,      // ONE_MINUS_SRC_COLOR
    MTL::BlendFactorDestinationColor,         // DST_COLOR
    MTL::BlendFactorOneMinusDestinationColor, // ONE_MINUS_DST_COLOR
    MTL::BlendFactorSourceAlpha,              // SRC_ALPHA
    MTL::BlendFactorOneMinusSourceAlpha,      // ONE_MINUS_SRC_ALPHA
    MTL::BlendFactorDestinationAlpha,         // DST_ALPHA
    MTL::BlendFactorOneMinusDestinationAlpha, // ONE_MINUS_DST_ALPHA
    MTL::BlendFactorBlendColor,               // CONSTANT_COLOR
    MTL::BlendFactorOneMinusBlendColor,       // ONE_MINUS_CONSTANT_COLOR
    MTL::BlendFactorBlendAlpha,               // CONSTANT_ALPHA
    MTL::BlendFactorOneMinusBlendAlpha,       // ONE_MINUS_CONSTANT_ALPHA
    MTL::BlendFactorSourceAlphaSaturated,     // SRC_ALPHA_SATURATE
    MTL::BlendFactorSource1Color,             // SRC1_COLOR
    MTL::BlendFactorOneMinusSource1Color,     // ONE_MINUS_SRC1_COLOR
    MTL::BlendFactorSource1Alpha,             // SRC1_ALPHA
    MTL::BlendFactorOneMinusSource1Alpha,     // ONE_MINUS_SRC1_ALPHA
};
NRI_VALIDATE_ARRAY(g_BlendFactors);

constexpr std::array<MTL::BlendOperation, (size_t)BlendOp::MAX_NUM> g_BlendOps = {
    MTL::BlendOperationAdd,             // ADD
    MTL::BlendOperationSubtract,        // SUBTRACT
    MTL::BlendOperationReverseSubtract, // REVERSE_SUBTRACT
    MTL::BlendOperationMin,             // MIN
    MTL::BlendOperationMax,             // MAX
};
NRI_VALIDATE_ARRAY(g_BlendOps);

constexpr std::array<MTL::StencilOperation, (size_t)StencilOp::MAX_NUM> g_StencilOps = {
    MTL::StencilOperationKeep,           // KEEP
    MTL::StencilOperationZero,           // ZERO
    MTL::StencilOperationReplace,        // REPLACE
    MTL::StencilOperationIncrementClamp, // INCREMENT_AND_CLAMP
    MTL::StencilOperationDecrementClamp, // DECREMENT_AND_CLAMP
    MTL::StencilOperationInvert,         // INVERT
    MTL::StencilOperationIncrementWrap,  // INCREMENT_AND_WRAP
    MTL::StencilOperationDecrementWrap,  // DECREMENT_AND_WRAP
};
NRI_VALIDATE_ARRAY(g_StencilOps);

constexpr std::array<MTL::CullMode, (size_t)CullMode::MAX_NUM> g_CullModes = {
    MTL::CullModeNone,  // NONE
    MTL::CullModeFront, // FRONT
    MTL::CullModeBack,  // BACK
};
NRI_VALIDATE_ARRAY(g_CullModes);

// Adjacency and patch topologies are drawn only via Converter geometry/tessellation emulation, which doesn't use these
constexpr std::array<MTL::PrimitiveType, (size_t)Topology::MAX_NUM> g_PrimitiveTypes = {
    MTL::PrimitiveTypePoint,         // POINT_LIST
    MTL::PrimitiveTypeLine,          // LINE_LIST
    MTL::PrimitiveTypeLineStrip,     // LINE_STRIP
    MTL::PrimitiveTypeTriangle,      // TRIANGLE_LIST
    MTL::PrimitiveTypeTriangleStrip, // TRIANGLE_STRIP
    MTL::PrimitiveTypeLine,          // LINE_LIST_WITH_ADJACENCY
    MTL::PrimitiveTypeLineStrip,     // LINE_STRIP_WITH_ADJACENCY
    MTL::PrimitiveTypeTriangle,      // TRIANGLE_LIST_WITH_ADJACENCY
    MTL::PrimitiveTypeTriangleStrip, // TRIANGLE_STRIP_WITH_ADJACENCY
    MTL::PrimitiveTypeTriangle,      // PATCH_LIST
};
NRI_VALIDATE_ARRAY(g_PrimitiveTypes);

constexpr std::array<MTL::PrimitiveTopologyClass, (size_t)Topology::MAX_NUM> g_TopologyClasses = {
    MTL::PrimitiveTopologyClassPoint,    // POINT_LIST
    MTL::PrimitiveTopologyClassLine,     // LINE_LIST
    MTL::PrimitiveTopologyClassLine,     // LINE_STRIP
    MTL::PrimitiveTopologyClassTriangle, // TRIANGLE_LIST
    MTL::PrimitiveTopologyClassTriangle, // TRIANGLE_STRIP
    MTL::PrimitiveTopologyClassLine,     // LINE_LIST_WITH_ADJACENCY
    MTL::PrimitiveTopologyClassLine,     // LINE_STRIP_WITH_ADJACENCY
    MTL::PrimitiveTopologyClassTriangle, // TRIANGLE_LIST_WITH_ADJACENCY
    MTL::PrimitiveTopologyClassTriangle, // TRIANGLE_STRIP_WITH_ADJACENCY
    MTL::PrimitiveTopologyClassTriangle, // PATCH_LIST
};
NRI_VALIDATE_ARRAY(g_TopologyClasses);

// A face with "CompareOp::NONE" keeps the default descriptor (always passes, keeps the value)
static inline void SetStencilMetal(MTL::StencilDescriptor* dst, const StencilDesc& src) {
    if (src.compareOp == CompareOp::NONE)
        return;

    dst->setStencilCompareFunction(GetCompareMetal(src.compareOp));
    dst->setStencilFailureOperation(g_StencilOps[(uint32_t)src.failOp]);
    dst->setDepthStencilPassOperation(g_StencilOps[(uint32_t)src.passOp]);
    dst->setDepthFailureOperation(g_StencilOps[(uint32_t)src.depthFailOp]);
    dst->setReadMask(src.compareMask);
    dst->setWriteMask(src.writeMask);
}

// "TopLevelArgumentBuffer" must match the pipeline layout, descriptor table accesses must fit into tables
static bool IsRootSignatureMatchingMetal(const PipelineLayoutMetal& layout, JsonValueMetal reflection) {
    const Vector<RootArgumentMetal>& arguments = layout.GetRootArguments();
    JsonValueMetal value = {};
    size_t index = 0;

    bool isMatching = FindJsonMember(reflection, "TopLevelArgumentBuffer", value) && ForEachJsonValue(value, [&](JsonValueMetal, JsonValueMetal element) {
        if (index >= arguments.size())
            return false;

        const RootArgumentMetal& argument = arguments[index++];
        const char* type = nullptr;
        size_t typeLength = 0;
        uint32_t offset = 0;
        uint32_t size = 0;
        uint32_t registerIndex = 0;
        uint32_t space = 0;

        if (!GetJsonMemberString(element, "Type", type, typeLength) || !GetJsonMemberUint(element, "EltOffset", offset) || !GetJsonMemberUint(element, "Size", size))
            return false;

        if (!GetJsonMemberUint(element, "Slot", registerIndex) || !GetJsonMemberUint(element, "Space", space))
            return false;

        return typeLength == strlen(argument.type) && !memcmp(type, argument.type, typeLength) && offset == argument.offset && size == argument.size && registerIndex == argument.registerIndex && space == argument.space;
    });

    isMatching = isMatching && index == arguments.size();

    return isMatching && FindJsonMember(reflection, "UsedResources", value) && ForEachJsonValue(value, [&](JsonValueMetal, JsonValueMetal element) {
        uint32_t argumentIndex = 0;
        uint32_t tableOffset = 0;
        uint32_t tableLength = 0;

        if (!GetJsonMemberUint(element, "bindingIndex", argumentIndex) || !GetJsonMemberUint(element, "tableStartIndex", tableOffset) || !GetJsonMemberUint(element, "tableLength", tableLength))
            return false;

        if (argumentIndex >= arguments.size())
            return false;

        const RootArgumentMetal& argument = arguments[argumentIndex];

        if (tableOffset == UINT32_MAX)
            return argument.registerIndex != UINT32_MAX;

        if (argument.registerIndex != UINT32_MAX)
            return false;

        // Unbounded arrays have "UINT32_MAX" length
        return tableLength == UINT32_MAX ? tableOffset < argument.descriptorNum : tableOffset + (uint64_t)tableLength <= argument.descriptorNum;
    });
}

// Converter reflection "ShaderType"
static inline const char* GetReflectionShaderTypeMetal(StageBits stage) {
    if (stage == StageBits::VERTEX_SHADER)
        return "Vertex";

    if (stage == StageBits::FRAGMENT_SHADER)
        return "Fragment";

    if (stage == StageBits::COMPUTE_SHADER)
        return "Compute";

    if (stage == StageBits::TASK_SHADER)
        return "Amplification";

    if (stage == StageBits::MESH_SHADER)
        return "Mesh";

    return nullptr;
}

// Bundles are structurally checked by validation. Converter settings, which are not reflected, can't be checked
static Result LoadMetalBundle(DeviceMetal& device, const PipelineLayoutMetal& layout, const ShaderDesc& shader, const ShaderLoadDescMetal& load, Vector<uint8_t>& storage) {
    uint32_t bundle[6] = {}; // "magic", "version", "metallibOffset", "metallibSize", "reflectionOffset", "reflectionSize"
    NRI_CHECK(shader.size >= sizeof(bundle), "Unexpected bundle size");
    memcpy(bundle, shader.bytecode, sizeof(bundle));

    if (load.emulation) {
        NRI_REPORT_ERROR(&device, "Metal converter bundle: geometry / tessellation emulation is unsupported");

        return Result::UNSUPPORTED;
    }

    // Converter options have no sample mask and input topology
    if (shader.stage == StageBits::FRAGMENT_SHADER && load.sampleMask != ALL) {
        NRI_REPORT_ERROR(&device, "Metal converter bundle: 'sampleMask' must be 'ALL'");

        return Result::INVALID_ARGUMENT;
    }

    if (shader.stage == StageBits::VERTEX_SHADER && load.topology == Topology::POINT_LIST) {
        NRI_REPORT_ERROR(&device, "Metal converter bundle: 'POINT_LIST' topology is unsupported");

        return Result::INVALID_ARGUMENT;
    }

    const uint8_t* data = (const uint8_t*)shader.bytecode;
    const JsonValueMetal reflection = {(const char*)data + bundle[4], (const char*)data + bundle[4] + bundle[5]};
    const char* shaderType = GetReflectionShaderTypeMetal(shader.stage);

    ConvertedShaderHeaderMetal header = {};
    header.stage = shader.stage;
    header.sampleMask = UINT32_MAX;
    header.threadGroupSize[0] = 1;
    header.threadGroupSize[1] = 1;
    header.threadGroupSize[2] = 1;

    Vector<ConvertedVertexInputDescMetal> vertexInputs(device.GetStdAllocator());
    const char* type = nullptr;
    const char* functionName = nullptr;
    size_t typeLength = 0;
    size_t functionNameLength = 0;
    JsonValueMetal state = {};

    bool isValid = shaderType && GetJsonMemberString(reflection, "ShaderType", type, typeLength) && typeLength == strlen(shaderType) && !memcmp(type, shaderType, typeLength);
    isValid = isValid && GetJsonMemberString(reflection, "EntryPoint", functionName, functionNameLength) && FindJsonMember(reflection, "state", state);

    if (isValid && shader.stage == StageBits::COMPUTE_SHADER)
        isValid = GetJsonMemberUints(state, "tg_size", header.threadGroupSize, 3);
    else if (isValid && (shader.stage == StageBits::MESH_SHADER || shader.stage == StageBits::TASK_SHADER))
        isValid = GetJsonMemberUints(state, "num_threads", header.threadGroupSize, 3) && GetJsonMemberUint(state, "max_payload_size_in_bytes", header.payloadSize);
    else if (isValid && shader.stage == StageBits::VERTEX_SHADER) {
        JsonValueMetal inputs = {};

        isValid = !FindJsonMember(state, "vertex_inputs", inputs) || ForEachJsonValue(inputs, [&](JsonValueMetal, JsonValueMetal input) {
            ConvertedVertexInputDescMetal vertexInput = {};
            const bool isParsed = GetJsonMemberString(input, "name", vertexInput.name, vertexInput.nameLength) && GetJsonMemberUint(input, "index", vertexInput.attributeIndex);
            vertexInputs.push_back(vertexInput);

            return isParsed && vertexInput.attributeIndex < CONVERTED_VERTEX_ATTRIBUTE_NUM && vertexInputs.size() <= CONVERTED_VERTEX_ATTRIBUTE_NUM;
        });
    }

    isValid = isValid && header.threadGroupSize[0] && header.threadGroupSize[1] && header.threadGroupSize[2];

    if (!isValid) {
        NRI_REPORT_ERROR(&device, "Metal converter bundle: the reflection is invalid or doesn't match 'stage'");

        return Result::INVALID_ARGUMENT;
    }

    if (!IsRootSignatureMatchingMetal(layout, reflection)) {
        NRI_REPORT_ERROR(&device, "Metal converter bundle: the root signature doesn't match the pipeline layout");

        return Result::INVALID_ARGUMENT;
    }

    const char* entryPoint = shader.entryPointName ? shader.entryPointName : "main";
    uint8_t* metallib = WriteConvertedShader(storage, header, entryPoint, functionName, functionNameLength, vertexInputs, bundle[3]);
    memcpy(metallib, data + bundle[2], bundle[3]);

    return Result::SUCCESS;
}

PipelineMetal::PipelineMetal(DeviceMetal& device)
    : m_Device(device) {
}

PipelineMetal::~PipelineMetal() {
    if (m_Render)
        m_Render->release();

    if (m_Compute)
        m_Compute->release();

    if (m_DepthStencil)
        m_DepthStencil->release();
}

#if NRI_ENABLE_METAL_SHADER_CONVERTER
Result PipelineMetal::ConvertShader(const ShaderDesc& shader, const ShaderLoadDescMetal& load, Vector<uint8_t>& storage, const uint8_t*& container) {
    const IRShaderStage stage = GetIRShaderStage(shader.stage);

    if (stage == IRShaderStageInvalid)
        return Result::UNSUPPORTED;

    MTL::Device& device = *m_Device.GetNativeObject();
    const char* entryPoint = shader.entryPointName ? shader.entryPointName : "main";

    ConvertedShaderHeaderMetal header = {};
    header.version = CONVERTED_SHADER_VERSION;
    header.rootSignatureHash = m_Layout->GetRootSignatureHash();
    header.stage = shader.stage;
    header.flags = (GetIRCompatibilityFlags(device) & IRCompatibilityFlagSamplerLODBias) ? CONVERTED_SHADER_SAMPLER_LOD_BIAS : 0;
    header.gpuFamily = GetIRGPUFamily(device);
    header.inputTopology = IRInputTopologyUndefined;
    header.sampleMask = UINT32_MAX;
    header.threadGroupSize[0] = 1;
    header.threadGroupSize[1] = 1;
    header.threadGroupSize[2] = 1;

    // Stage-irrelevant settings stay default, so shared shaders are converted (and cached) once
    if (stage == IRShaderStageVertex && load.topology != Topology::MAX_NUM)
        header.inputTopology = g_InputTopologies[(uint32_t)load.topology];

    if (stage == IRShaderStageFragment) {
        if (load.sampleMask != ALL)
            header.sampleMask = load.sampleMask;

        if (load.dualSourceBlending)
            header.flags |= CONVERTED_SHADER_DUAL_SOURCE_BLENDING;
    }

    // Emulation stages depend on other stages and the vertex input layout, thus they are not cached
    PipelineCacheMetal* cache = load.emulation ? nullptr : load.cache;
    uint64_t key = 0;

    if (cache) {
        // Everything affecting the output
        const uint32_t settings[] = {header.version, (uint32_t)header.stage, (uint32_t)header.flags, header.gpuFamily, header.inputTopology, header.sampleMask, FRAMEBUFFER_FETCH_SPACE, (uint32_t)CONVERTER_OPERATING_SYSTEM};
        key = HashMetal(settings, sizeof(settings), GetConverterHash(m_Device));
        key = HashMetal(&header.rootSignatureHash, sizeof(header.rootSignatureHash), key);
        key = HashMetal(CONVERTER_DEPLOYMENT_TARGET, strlen(CONVERTER_DEPLOYMENT_TARGET) + 1, key);
        key = HashMetal(entryPoint, strlen(entryPoint) + 1, key);
        key = HashMetal(shader.bytecode, shader.size, key);

        // Entries are structurally valid (see "PipelineCacheMetal::Create"), a mismatch is a key collision
        size_t size = 0;

        if (cache->FindConvertedShader(key, container, size)) {
            ConvertedShaderHeaderMetal cached = {};
            memcpy(&cached, container, sizeof(cached));

            if (cached.stage == header.stage && cached.rootSignatureHash == header.rootSignatureHash && !strcmp((const char*)container + cached.entryPointOffset, entryPoint))
                return Result::SUCCESS;

            container = nullptr;
        }

        if (load.failOnCacheMiss)
            return Result::FAILURE;
    }

    IRCompiler* compiler = CreateIRCompiler(device, *m_Layout);
    IRCompilerSetFramebufferFetchResourceSpace(compiler, FRAMEBUFFER_FETCH_SPACE);
    IRCompilerEnableGeometryAndTessellationEmulation(compiler, load.emulation);
    IRCompilerSetSampleMask(compiler, header.sampleMask);

    // Required for point rendering
    if (header.inputTopology != IRInputTopologyUndefined)
        IRCompilerSetInputTopology(compiler, (IRInputTopology)header.inputTopology);

    if (header.flags & CONVERTED_SHADER_DUAL_SOURCE_BLENDING)
        IRCompilerSetDualSourceBlendingConfiguration(compiler, IRDualSourceBlendingConfigurationForceEnabled);

    if (load.stageInLibrary)
        IRCompilerSetStageInGenerationMode(compiler, IRStageInCodeGenerationModeUseSeparateStageInFunction);

    IRObject* input = IRObjectCreateFromDXIL((const uint8_t*)shader.bytecode, shader.size, IRBytecodeOwnershipNone);

    IRError* error = nullptr;
    IRObject* output = IRCompilerAllocCompileAndLink(compiler, entryPoint, input, &error);
    IRObjectDestroy(input);

    if (!output) {
        NRI_REPORT_ERROR(&m_Device, "DXIL conversion failed for '%s' (converter error %u)", entryPoint, error ? IRErrorGetCode(error) : 0);

        if (error)
            IRErrorDestroy(error);

        IRCompilerDestroy(compiler);

        return Result::FAILURE;
    }

    if (error)
        IRErrorDestroy(error);

    IRMetalLibBinary* binary = IRMetalLibBinaryCreate();
    IRShaderReflection* reflection = IRShaderReflectionCreate();
    IRVersionedVSInfo vertexInfo = {};
    bool hasVertexInfo = false;
    const char* functionName = nullptr;
    Result result = Result::SUCCESS;

    if (!IRObjectGetMetalLibBinary(output, stage, binary))
        result = Result::FAILURE;
    else if (IRObjectGetReflection(output, stage, reflection)) {
        functionName = IRShaderReflectionGetEntryPointFunctionName(reflection);

        if (stage == IRShaderStageCompute) {
            IRVersionedCSInfo info = {};

            if (IRShaderReflectionCopyComputeInfo(reflection, IRReflectionVersion_1_0, &info)) {
                memcpy(header.threadGroupSize, info.info_1_0.tg_size, sizeof(header.threadGroupSize));
                IRShaderReflectionReleaseComputeInfo(&info);
            }
        } else if (stage == IRShaderStageMesh) {
            IRVersionedMSInfo info = {};

            if (IRShaderReflectionCopyMeshInfo(reflection, IRReflectionVersion_1_0, &info)) {
                memcpy(header.threadGroupSize, info.info_1_0.num_threads, sizeof(header.threadGroupSize));
                header.payloadSize = info.info_1_0.max_payload_size_in_bytes;
                IRShaderReflectionReleaseMeshInfo(&info);
            }
        } else if (stage == IRShaderStageAmplification) {
            IRVersionedASInfo info = {};

            if (IRShaderReflectionCopyAmplificationInfo(reflection, IRReflectionVersion_1_0, &info)) {
                memcpy(header.threadGroupSize, info.info_1_0.num_threads, sizeof(header.threadGroupSize));
                header.payloadSize = info.info_1_0.max_payload_size_in_bytes;
                IRShaderReflectionReleaseAmplificationInfo(&info);
            }
        } else if (stage == IRShaderStageVertex) {
            hasVertexInfo = IRShaderReflectionCopyVertexInfo(reflection, IRReflectionVersion_1_0, &vertexInfo);

            if (hasVertexInfo) {
                m_GeometryConfig.gsVertexSizeInBytes = vertexInfo.info_1_0.vertex_output_size_in_bytes;
                m_TessellationConfig.vsOutputSizeInBytes = vertexInfo.info_1_0.vertex_output_size_in_bytes;
            }
        } else if (stage == IRShaderStageGeometry) {
            IRVersionedGSInfo info = {};

            if (IRShaderReflectionCopyGeometryInfo(reflection, IRReflectionVersion_1_0, &info)) {
                m_GeometryConfig.gsMaxInputPrimitivesPerMeshThreadgroup = info.info_1_0.max_input_primitives_per_mesh_threadgroup;
                m_TessellationConfig.gsMaxInputPrimitivesPerMeshThreadgroup = info.info_1_0.max_input_primitives_per_mesh_threadgroup;
                m_TessellationConfig.gsInstanceCount = info.info_1_0.instance_count;
                m_MeshPayloadSize = info.info_1_0.max_payload_size_in_bytes;
                IRShaderReflectionReleaseGeometryInfo(&info);
            }
        } else if (stage == IRShaderStageHull) {
            IRVersionedHSInfo info = {};

            if (IRShaderReflectionCopyHullInfo(reflection, IRReflectionVersion_1_0, &info)) {
                m_TessellationConfig.outputPrimitiveType = (IRRuntimeTessellatorOutputPrimitive)info.info_1_0.tessellator_output_primitive;
                m_TessellationConfig.hsMaxPatchesPerObjectThreadgroup = info.info_1_0.max_patches_per_object_threadgroup;
                m_TessellationConfig.hsInputControlPointCount = info.info_1_0.input_control_point_count;
                m_TessellationConfig.hsMaxObjectThreadsPerThreadgroup = info.info_1_0.max_object_threads_per_patch;
                m_TessellationConfig.hsMaxTessellationFactor = info.info_1_0.max_tessellation_factor;
                IRShaderReflectionReleaseHullInfo(&info);
            }
        } else if (stage == IRShaderStageDomain) {
            IRVersionedDSInfo info = {};

            if (IRShaderReflectionCopyDomainInfo(reflection, IRReflectionVersion_1_0, &info)) {
                if (!m_GeometryEmulation)
                    m_TessellationConfig.gsMaxInputPrimitivesPerMeshThreadgroup = info.info_1_0.max_input_prims_per_mesh_threadgroup;

                IRShaderReflectionReleaseDomainInfo(&info);
            }
        }
    }

    if (result == Result::SUCCESS && load.stageInLibrary) {
        IRVersionedInputLayoutDescriptor layout = {};
        layout.version = IRInputLayoutDescriptorVersion_1;
        layout.desc_1_0.numElements = load.vertexInput ? load.vertexInput->attributeNum : 0;

        for (uint32_t i = 0; i < layout.desc_1_0.numElements; i++) {
            const VertexAttributeDesc& attribute = load.vertexInput->attributes[i];
            const VertexStreamDesc& stream = load.vertexInput->streams[attribute.streamIndex];
            const bool perInstance = stream.stepRate == VertexStreamStepRate::PER_INSTANCE;

            layout.desc_1_0.semanticNames[i] = attribute.d3d.semanticName;
            layout.desc_1_0.inputElementDescs[i] = {attribute.d3d.semanticIndex, GetIRVertexFormat(attribute.format), stream.bindingSlot, attribute.offset, perInstance ? 1u : 0u, perInstance ? IRInputClassificationPerInstanceData : IRInputClassificationPerVertexData};
        }

        IRMetalLibBinary* stageIn = IRMetalLibBinaryCreate();

        if (IRMetalLibSynthesizeStageInFunction(compiler, reflection, &layout, stageIn)) {
            NS::Error* stageInError = nullptr;
            *load.stageInLibrary = device.newLibrary(IRMetalLibGetBytecodeData(stageIn), &stageInError);

            if (!*load.stageInLibrary) {
                NRI_REPORT_ERROR(&m_Device, "Metal stage-in library creation failed: %s", stageInError ? stageInError->localizedDescription()->utf8String() : "unknown error");
                result = Result::FAILURE;
            }
        } else
            result = Result::FAILURE;

        IRMetalLibBinaryDestroy(stageIn);
    }

    // Container
    if (result == Result::SUCCESS) {
        const uint32_t vertexInputNum = hasVertexInfo ? (uint32_t)vertexInfo.info_1_0.num_vertex_inputs : 0;
        Vector<ConvertedVertexInputDescMetal> vertexInputs(m_Device.GetStdAllocator());

        for (uint32_t i = 0; i < vertexInputNum; i++) {
            const IRVertexInputInfo_1_0& source = vertexInfo.info_1_0.vertex_inputs[i];
            const char* name = source.name ? source.name : "";

            vertexInputs.push_back({name, strlen(name), source.attributeIndex});
        }

        const char* name = functionName ? functionName : entryPoint;
        uint8_t* metallib = WriteConvertedShader(storage, header, entryPoint, name, strlen(name), vertexInputs, IRMetalLibGetBytecodeSize(binary));
        IRMetalLibGetBytecode(binary, metallib);

        if (cache)
            cache->AddConvertedShader(key, storage.data(), storage.size());

        container = storage.data();
    }

    if (hasVertexInfo)
        IRShaderReflectionReleaseVertexInfo(&vertexInfo);

    IRShaderReflectionDestroy(reflection);
    IRMetalLibBinaryDestroy(binary);
    IRObjectDestroy(output);
    IRCompilerDestroy(compiler);

    return result;
}
#endif

Result PipelineMetal::LoadFunction(const ShaderDesc& shader, MTL::Library*& library, MTL4::FunctionDescriptor*& function, const ShaderLoadDescMetal& load) {
    const char* functionName = shader.entryPointName ? shader.entryPointName : "main";
    const uint8_t* bytecode = (const uint8_t*)shader.bytecode;
    size_t bytecodeSize = shader.size;
    const uint8_t* container = nullptr;
    Vector<uint8_t> storage(m_Device.GetStdAllocator());

#if NRI_ENABLE_METAL_SHADER_CONVERTER
    if (IsDXIL(shader)) {
        const Result result = ConvertShader(shader, load, storage, container);

        if (result != Result::SUCCESS)
            return result;
    }
#else
    if (IsDXIL(shader)) {
        NRI_REPORT_ERROR(&m_Device, "DXIL requires Metal Shader Converter ('NRI_ENABLE_METAL_SHADER_CONVERTER'), pass a pre-converted shader instead");

        return Result::UNSUPPORTED;
    }
#endif

    if (IsMetalBundle(shader)) {
        const Result result = LoadMetalBundle(m_Device, *m_Layout, shader, load, storage);

        if (result != Result::SUCCESS)
            return result;

        container = storage.data();
    }

    if (container) {
        ConvertedShaderHeaderMetal header = {};
        memcpy(&header, container, sizeof(header));

        const MTL::Size threadGroup = MTL::Size(header.threadGroupSize[0], header.threadGroupSize[1], header.threadGroupSize[2]);

        if (header.stage == StageBits::COMPUTE_SHADER)
            m_ThreadGroup = threadGroup;
        else if (header.stage == StageBits::MESH_SHADER) {
            m_MeshGroup = threadGroup;
            m_MeshPayloadSize = std::max(m_MeshPayloadSize, header.payloadSize);
        } else if (header.stage == StageBits::TASK_SHADER) {
            m_TaskGroup = threadGroup;
            m_MeshPayloadSize = std::max(m_MeshPayloadSize, header.payloadSize);
        } else if (header.stage == StageBits::VERTEX_SHADER && load.vertexInput && load.vertexAttributeSlots)
            MapConvertedVertexAttributes(*load.vertexInput, container, header, load.vertexAttributeSlots);

        functionName = (const char*)container + header.functionNameOffset;
        bytecode = container + header.metallibOffset;
        bytecodeSize = header.metallibSize;
        m_Converted = true;
    }

    // Pipelines are built by "MTL4Compiler", but metallibs are still loaded by the device ("MTL4LibraryDescriptor" compiles source only)
    dispatch_data_t data = dispatch_data_create(bytecode, bytecodeSize, dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), DISPATCH_DATA_DESTRUCTOR_DEFAULT);

    NS::Error* error = nullptr;
    library = m_Device.GetNativeObject()->newLibrary(data, &error);
    dispatch_release(data);

    if (!library)
        NRI_REPORT_ERROR(&m_Device, "Metal library creation failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");

    // Native fragment shaders opt into the sample mask ABI by referencing "NriPipelineSampleMask" ("NRI.metal")
    bool unsupportedSampleMask = false;

    if (library) {
        if (!container && shader.stage == StageBits::FRAGMENT_SHADER) {
            MTL::Function* reflected = library->newFunction(NS::String::string(functionName, NS::UTF8StringEncoding));

            if (reflected) {
                constexpr NS::UInteger sampleMaskConstantIndex = 65535;

                auto* constant = (MTL::FunctionConstant*)reflected->functionConstantsDictionary()->object(NS::String::string("NriPipelineSampleMask", NS::UTF8StringEncoding));
                const bool conforms = constant && constant->index() == sampleMaskConstantIndex && constant->type() == MTL::DataTypeUInt;
                reflected->release();

                if (conforms) {
                    const uint32_t nativeSampleMask = load.sampleMask == ALL ? UINT32_MAX : load.sampleMask;

                    MTL::FunctionConstantValues* constants = MTL::FunctionConstantValues::alloc()->init();
                    constants->setConstantValue(&nativeSampleMask, MTL::DataTypeUInt, sampleMaskConstantIndex);
                    function = NewFunctionDescriptorMetal(library, functionName, constants);
                    constants->release();
                } else if (load.sampleMask != ALL)
                    unsupportedSampleMask = true;
                else
                    function = NewFunctionDescriptorMetal(library, functionName);
            } else
                NRI_REPORT_ERROR(&m_Device, "Metal entry point '%s' was not found", functionName);
        } else
            function = NewFunctionDescriptorMetal(library, functionName);
    }

    if (!library)
        return Result::FAILURE;

    if (unsupportedSampleMask)
        return Result::UNSUPPORTED;

    return function ? Result::SUCCESS : Result::FAILURE;
}

// "failOnMiss" without a cache fails, since there is nothing to look up in
static MTL::ComputePipelineState* NewComputePipelineMetal(DeviceMetal& device, const PipelineCache* cache, bool failOnMiss, const MTL4::ComputePipelineDescriptor* desc, const MTL4::PipelineStageDynamicLinkingDescriptor* linking, NS::Error** error) {
    if (cache)
        return ((PipelineCacheMetal*)cache)->NewComputePipeline(desc, linking, failOnMiss, error);

    if (failOnMiss)
        return nullptr;

    MTL4::Compiler* compiler = device.GetCompiler();

    return linking ? compiler->newComputePipelineState(desc, linking, nullptr, error) : compiler->newComputePipelineState(desc, nullptr, error);
}

static MTL::RenderPipelineState* NewRenderPipelineMetal(DeviceMetal& device, const PipelineCache* cache, bool failOnMiss, const MTL4::PipelineDescriptor* desc, NS::Error** error) {
    if (cache)
        return ((PipelineCacheMetal*)cache)->NewRenderPipeline(desc, failOnMiss, error);

    if (failOnMiss)
        return nullptr;

    return device.GetCompiler()->newRenderPipelineState(desc, nullptr, error);
}

Result PipelineMetal::Create(const ComputePipelineDesc& desc) {
    AutoreleasePoolMetal autoreleasePool; // "NS::Error" is autoreleased

    m_Layout = (const PipelineLayoutMetal*)desc.pipelineLayout;

    const bool failOnCacheMiss = desc.flags & ComputePipelineBits::FAIL_ON_CACHE_MISS;

    ShaderLoadDescMetal loadDesc = {};
    loadDesc.cache = (PipelineCacheMetal*)desc.cache;
    loadDesc.failOnCacheMiss = failOnCacheMiss;

    MTL::Library* library = nullptr;
    MTL4::FunctionDescriptor* function = nullptr;
    Result result = LoadFunction(desc.shader, library, function, loadDesc);

    if (result == Result::SUCCESS) {
        if (!m_Converted)
            m_ThreadGroup = GetNativeThreadGroupSize(desc.shader);

        // Dispatches always use the pipeline threadgroup size
        MTL4::ComputePipelineDescriptor* pipelineDesc = MTL4::ComputePipelineDescriptor::alloc()->init();
        pipelineDesc->setComputeFunctionDescriptor(function);
        pipelineDesc->setRequiredThreadsPerThreadgroup(m_ThreadGroup);

        NS::Error* error = nullptr;
        m_Compute = NewComputePipelineMetal(m_Device, desc.cache, failOnCacheMiss, pipelineDesc, nullptr, &error);

        if (!m_Compute) {
            result = Result::FAILURE;

            if (!failOnCacheMiss)
                NRI_REPORT_ERROR(&m_Device, "Metal compute pipeline creation failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");
        }

        pipelineDesc->release();
    }

    if (function)
        function->release();

    if (library)
        library->release();

    return result;
}

Result PipelineMetal::Create(const GraphicsPipelineDesc& desc) {
    m_Layout = (const PipelineLayoutMetal*)desc.pipelineLayout;
    m_Multiview = desc.outputMerger.multiview;
    m_ViewMask = desc.outputMerger.viewMask;

    bool hasVertex = false;
    bool hasMesh = false;
    bool hasGeometry = false;
    bool hasTessellation = false;
    bool hasFragment = false;
    bool hasNativeShaders = false;
    bool hasConvertedShaders = false;
    bool hasConvertedVertex = false;

    for (uint32_t i = 0; i < desc.shaderNum; i++) {
        const ShaderDesc& shader = desc.shaders[i];
        const bool isConverted = IsConvertedShader(shader);

        hasNativeShaders |= !isConverted;
        hasConvertedShaders |= isConverted;
        hasConvertedVertex |= isConverted && shader.stage == StageBits::VERTEX_SHADER;
        hasVertex |= shader.stage == StageBits::VERTEX_SHADER;
        hasMesh |= shader.stage == StageBits::MESH_SHADER;
        hasGeometry |= shader.stage == StageBits::GEOMETRY_SHADER;
        hasTessellation |= shader.stage == StageBits::TESS_CONTROL_SHADER || shader.stage == StageBits::TESS_EVALUATION_SHADER;
        hasFragment |= shader.stage == StageBits::FRAGMENT_SHADER;
    }

    // Converter lacks "SV_ViewID". Native and converted stages share the binding ABI, but view indices are provided only for native pipelines
    if (hasConvertedShaders && m_ViewMask && (hasNativeShaders || m_Multiview == Multiview::FLEXIBLE))
        return Result::UNSUPPORTED;

    // Geometry/tessellation emulation requires converted stages and a vertex shader
    if ((hasGeometry || hasTessellation) && (hasNativeShaders || !hasVertex || hasMesh))
        return Result::UNSUPPORTED;

    if (!hasFragment && desc.multisample && desc.multisample->sampleMask != ALL)
        return Result::UNSUPPORTED;

    // Metal has no adjacency or patch primitives. They are consumed only by the geometry/tessellation emulation, which has no "strip with adjacency" type
    const Topology topology = desc.inputAssembly.topology;
    const bool isAdjacency = topology >= Topology::LINE_LIST_WITH_ADJACENCY && topology <= Topology::TRIANGLE_STRIP_WITH_ADJACENCY;

    if (!hasMesh) {
        if ((isAdjacency && !hasGeometry) || (topology == Topology::PATCH_LIST && !hasTessellation) || topology == Topology::TRIANGLE_STRIP_WITH_ADJACENCY)
            return Result::UNSUPPORTED;
    }

#if NRI_ENABLE_METAL_SHADER_CONVERTER
    m_GeometryEmulation = hasGeometry;
    m_TessellationEmulation = hasTessellation;
    m_TessellationConfig.gsInstanceCount = 1;
    m_EmulationPrimitive = g_EmulationPrimitives[(uint32_t)topology];
#endif

    const bool emulation = hasGeometry || hasTessellation;
    const bool isMesh = hasMesh || emulation;

    // Vertex formats not reported as "VERTEX_BUFFER" (or not synthesizable by the emulation stage-in)
    const uint32_t vertexAttributeNum = desc.vertexInput && !hasMesh ? desc.vertexInput->attributeNum : 0;

    for (uint32_t i = 0; i < vertexAttributeNum; i++) {
        const Format format = desc.vertexInput->attributes[i].format;

        if (GetVertexFormatMetal(format) == MTL::VertexFormatInvalid)
            return Result::UNSUPPORTED;

#if NRI_ENABLE_METAL_SHADER_CONVERTER
        if (emulation && GetIRVertexFormat(format) == IRFormatUnknown)
            return Result::UNSUPPORTED;
#endif
    }

    bool dualSourceBlending = false;

    for (uint32_t i = 0; i < desc.outputMerger.colorNum; i++) {
        const ColorAttachmentDesc& color = desc.outputMerger.colors[i];

        if (color.blendEnabled) {
            dualSourceBlending |= IsDualSourceBlendFactor(color.colorBlend.srcFactor) || IsDualSourceBlendFactor(color.colorBlend.dstFactor);
            dualSourceBlending |= IsDualSourceBlendFactor(color.alphaBlend.srcFactor) || IsDualSourceBlendFactor(color.alphaBlend.dstFactor);
        }
    }

    // Vertex attribute slots: native - array order, converted - reflected by semantic (array order if no vertex inputs are reflected)
    const uint32_t attributeNum = desc.vertexInput && !isMesh ? desc.vertexInput->attributeNum : 0;
    uint8_t attributeSlots[256] = {};

    if (hasConvertedVertex && attributeNum > CONVERTED_VERTEX_ATTRIBUTE_NUM) {
        NRI_REPORT_ERROR(&m_Device, "Converted vertex shaders support up to %u vertex attributes", CONVERTED_VERTEX_ATTRIBUTE_NUM);

        return Result::UNSUPPORTED;
    }

    for (uint32_t i = 0; i < attributeNum; i++)
        attributeSlots[i] = uint8_t(hasConvertedVertex ? CONVERTED_VERTEX_ATTRIBUTE_BASE + i : i);

    AutoreleasePoolMetal autoreleasePool; // "NS::Error" is autoreleased

    MTL4::RenderPipelineDescriptor* pd = isMesh ? nullptr : MTL4::RenderPipelineDescriptor::alloc()->init();
    MTL4::MeshRenderPipelineDescriptor* mpd = isMesh ? MTL4::MeshRenderPipelineDescriptor::alloc()->init() : nullptr;
    Vector<MTL::Library*> libraries(m_Device.GetStdAllocator());
    Vector<MTL4::FunctionDescriptor*> functions(m_Device.GetStdAllocator());
    MTL::Library* stageInLibrary = nullptr;
    bool hasFragmentFunction = false;
    Result result = Result::SUCCESS;

    for (uint32_t i = 0; i < desc.shaderNum; i++) {
        const ShaderDesc& shader = desc.shaders[i];

        ShaderLoadDescMetal loadDesc = {};
        loadDesc.cache = (PipelineCacheMetal*)desc.cache;
        loadDesc.vertexInput = desc.vertexInput;
        loadDesc.vertexAttributeSlots = attributeSlots;
        loadDesc.stageInLibrary = shader.stage == StageBits::VERTEX_SHADER && emulation ? &stageInLibrary : nullptr;
        loadDesc.sampleMask = desc.multisample ? desc.multisample->sampleMask : ALL;
        loadDesc.topology = hasMesh || emulation ? Topology::MAX_NUM : topology;
        loadDesc.emulation = emulation;
        loadDesc.dualSourceBlending = dualSourceBlending;
        loadDesc.failOnCacheMiss = desc.flags & GraphicsPipelineBits::FAIL_ON_CACHE_MISS;

        MTL::Library* library = nullptr;
        MTL4::FunctionDescriptor* function = nullptr;
        result = LoadFunction(shader, library, function, loadDesc);

        if (result != Result::SUCCESS) {
            if (function)
                function->release();

            if (library)
                library->release();

            break;
        }

        libraries.push_back(library);
        functions.push_back(function);

        const bool isNative = !IsConvertedShader(shader);

        if (shader.stage == StageBits::VERTEX_SHADER) {
            if (!isMesh)
                pd->setVertexFunctionDescriptor(function);
        } else if (shader.stage == StageBits::FRAGMENT_SHADER) {
            hasFragmentFunction = true;

            if (isMesh)
                mpd->setFragmentFunctionDescriptor(function);
            else
                pd->setFragmentFunctionDescriptor(function);
        } else if (shader.stage == StageBits::MESH_SHADER) {
            mpd->setMeshFunctionDescriptor(function);

            if (isNative)
                m_MeshGroup = GetNativeThreadGroupSize(shader);
        } else if (shader.stage == StageBits::TASK_SHADER) {
            mpd->setObjectFunctionDescriptor(function);

            if (isNative)
                m_TaskGroup = GetNativeThreadGroupSize(shader);
        }
    }

    // Vertex amplification IDs are dense: "[[amplification_id]]" indexes the set bits of the view mask.
    // Metal ignores view mappings if the max amplification count is 1, so a single sparse view still needs 2
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    uint32_t amplificationCount = 1;

    if (m_Multiview == Multiview::VIEWPORT_BASED) // the pipeline "viewMask" is unused
        amplificationCount = deviceDesc.other.viewMaxNum;
    else if (m_ViewMask)
        amplificationCount = std::min(std::max((uint32_t)__builtin_popcount(m_ViewMask), 2u), (uint32_t)deviceDesc.other.viewMaxNum);

    const uint32_t sampleNum = desc.multisample ? desc.multisample->sampleNum : 1;
    const MTL4::AlphaToCoverageState alphaToCoverage = desc.multisample && desc.multisample->alphaToCoverage ? MTL4::AlphaToCoverageStateEnabled : MTL4::AlphaToCoverageStateDisabled;

    if (isMesh) {
        // Metal mesh pipelines require a fragment function even for depth-only draws
        if (!hasFragmentFunction)
            mpd->setFragmentFunctionDescriptor(m_Device.GetInternalShaders().GetDepthOnlyFragmentFunction());

        mpd->setMaxVertexAmplificationCount(amplificationCount);
        mpd->setRasterSampleCount(sampleNum);
        mpd->setAlphaToCoverageState(alphaToCoverage);

        if (emulation) {
            mpd->setMaxTotalThreadsPerMeshThreadgroup(256);
            mpd->setMaxTotalThreadsPerObjectThreadgroup(256);
        } else {
            mpd->setMaxTotalThreadsPerMeshThreadgroup(m_MeshGroup.width * m_MeshGroup.height * m_MeshGroup.depth);
            mpd->setRequiredThreadsPerMeshThreadgroup(m_MeshGroup);

            if (mpd->objectFunctionDescriptor()) {
                mpd->setMaxTotalThreadsPerObjectThreadgroup(m_TaskGroup.width * m_TaskGroup.height * m_TaskGroup.depth);
                mpd->setRequiredThreadsPerObjectThreadgroup(m_TaskGroup);
            }
        }

        mpd->setPayloadMemoryLength(emulation ? 16384 : m_MeshPayloadSize);
    } else {
        pd->setInputPrimitiveTopology(g_TopologyClasses[(uint32_t)topology]);
        pd->setMaxVertexAmplificationCount(amplificationCount);
        pd->setRasterSampleCount(sampleNum);
        pd->setAlphaToCoverageState(alphaToCoverage);

        if (attributeNum) {
            MTL::VertexDescriptor* vertex = MTL::VertexDescriptor::alloc()->init();

            for (uint32_t i = 0; i < attributeNum; i++) {
                if (attributeSlots[i] == VERTEX_ATTRIBUTE_UNUSED)
                    continue;

                const VertexAttributeDesc& source = desc.vertexInput->attributes[i];

                MTL::VertexAttributeDescriptor* attribute = vertex->attributes()->object(attributeSlots[i]);
                attribute->setFormat(GetVertexFormatMetal(source.format));
                attribute->setOffset(source.offset);
                attribute->setBufferIndex(ARGUMENT_SLOT_VERTEX_BUFFER_BASE + desc.vertexInput->streams[source.streamIndex].bindingSlot);
            }

            for (uint32_t i = 0; i < desc.vertexInput->streamNum; i++) {
                const VertexStreamDesc& source = desc.vertexInput->streams[i];

                MTL::VertexBufferLayoutDescriptor* layout = vertex->layouts()->object(ARGUMENT_SLOT_VERTEX_BUFFER_BASE + source.bindingSlot);
                layout->setStride(MTL::BufferLayoutStrideDynamic);
                layout->setStepFunction(source.stepRate == VertexStreamStepRate::PER_INSTANCE ? MTL::VertexStepFunctionPerInstance : MTL::VertexStepFunctionPerVertex);
                layout->setStepRate(1);
            }

            pd->setVertexDescriptor(vertex);
            vertex->release();
        }
    }

    // Metal 4 pipelines don't include depth / stencil attachment formats
    MTL4::RenderPipelineColorAttachmentDescriptorArray* colorAttachments = isMesh ? mpd->colorAttachments() : pd->colorAttachments();

    for (uint32_t i = 0; i < desc.outputMerger.colorNum; i++) {
        const ColorAttachmentDesc& color = desc.outputMerger.colors[i];

        MTL::ColorWriteMask mask = MTL::ColorWriteMaskNone;

        if (color.colorWriteMask & ColorWriteBits::R)
            mask |= MTL::ColorWriteMaskRed;

        if (color.colorWriteMask & ColorWriteBits::G)
            mask |= MTL::ColorWriteMaskGreen;

        if (color.colorWriteMask & ColorWriteBits::B)
            mask |= MTL::ColorWriteMaskBlue;

        if (color.colorWriteMask & ColorWriteBits::A)
            mask |= MTL::ColorWriteMaskAlpha;

        MTL4::RenderPipelineColorAttachmentDescriptor* attachment = colorAttachments->object(i);
        attachment->setPixelFormat(GetPixelFormatMetal(color.format));
        attachment->setBlendingState(color.blendEnabled ? MTL4::BlendStateEnabled : MTL4::BlendStateDisabled);
        attachment->setWriteMask(mask);
        attachment->setSourceRGBBlendFactor(g_BlendFactors[(uint32_t)color.colorBlend.srcFactor]);
        attachment->setDestinationRGBBlendFactor(g_BlendFactors[(uint32_t)color.colorBlend.dstFactor]);
        attachment->setRgbBlendOperation(g_BlendOps[(uint32_t)color.colorBlend.op]);
        attachment->setSourceAlphaBlendFactor(g_BlendFactors[(uint32_t)color.alphaBlend.srcFactor]);
        attachment->setDestinationAlphaBlendFactor(g_BlendFactors[(uint32_t)color.alphaBlend.dstFactor]);
        attachment->setAlphaBlendOperation(g_BlendOps[(uint32_t)color.alphaBlend.op]);
    }

    if (result == Result::SUCCESS) {
        const bool failOnCacheMiss = desc.flags & GraphicsPipelineBits::FAIL_ON_CACHE_MISS;
        NS::Error* error = nullptr;

#if NRI_ENABLE_METAL_SHADER_CONVERTER
        if (emulation) {
            auto findLibrary = [&](StageBits stage) -> MTL::Library* {
                for (uint32_t i = 0; i < desc.shaderNum; i++) {
                    if (desc.shaders[i].stage == stage)
                        return libraries[i];
                }

                return nullptr;
            };

            auto findName = [&](StageBits stage) -> const char* {
                for (uint32_t i = 0; i < desc.shaderNum; i++) {
                    if (desc.shaders[i].stage == stage)
                        return desc.shaders[i].entryPointName ? desc.shaders[i].entryPointName : "main";
                }

                return nullptr;
            };

            // Converter runtime ABI: the emulation stages are specialized and linked here, so archive lookups and pipelines without fragment shaders go through NRI's pipeline path
            auto specialize = [&](MTL::Library* library, const char* name, MTL::FunctionConstantValues* constants = nullptr) -> MTL4::FunctionDescriptor* {
                if (!library || !name) {
                    result = Result::FAILURE;

                    return nullptr;
                }

                MTL4::FunctionDescriptor* function = NewFunctionDescriptorMetal(library, name, constants);
                functions.push_back(function);

                return function;
            };

            MTL::FunctionConstantValues* constants = MTL::FunctionConstantValues::alloc()->init();
            constants->setConstantValue(&hasTessellation, MTL::DataTypeBool, NS::String::string("tessellationEnabled", NS::UTF8StringEncoding));

            // "FunctionConstantValues" are copied by the specialized function descriptor
            const std::string objectName = std::string(findName(StageBits::VERTEX_SHADER)) + ".dxil_irconverter_object_shader";
            mpd->setObjectFunctionDescriptor(specialize(findLibrary(StageBits::VERTEX_SHADER), objectName.c_str(), constants));

            const uint32_t vertexSize = hasTessellation ? m_TessellationConfig.vsOutputSizeInBytes : m_GeometryConfig.gsVertexSizeInBytes;
            constants->setConstantValue(&vertexSize, MTL::DataTypeInt, NS::String::string("vertex_shader_output_size_fc", NS::UTF8StringEncoding));

            const bool streamOut = false;
            constants->setConstantValue(&streamOut, MTL::DataTypeBool, NS::String::string("streamOutEnabled", NS::UTF8StringEncoding));

            MTL4::FunctionDescriptor* objectFunctions[2] = {specialize(stageInLibrary, "irconverter_stage_in_shader"), nullptr};
            MTL4::FunctionDescriptor* meshFunctions[2] = {};

            if (hasTessellation) {
                constants->setConstantValue(&m_TessellationConfig.hsMaxTessellationFactor, MTL::DataTypeFloat, NS::String::string("max_tessellation_factor_fc", NS::UTF8StringEncoding));
                objectFunctions[1] = specialize(findLibrary(StageBits::TESS_CONTROL_SHADER), "irconverter_hull_shader", constants);
                meshFunctions[0] = specialize(findLibrary(StageBits::TESS_CONTROL_SHADER), "irconverter_tessellator", constants);
                meshFunctions[1] = specialize(findLibrary(StageBits::TESS_EVALUATION_SHADER), "irconverter_dxil_domain_shader");
            }

            if (hasGeometry)
                mpd->setMeshFunctionDescriptor(specialize(findLibrary(StageBits::GEOMETRY_SHADER), findName(StageBits::GEOMETRY_SHADER), constants));
            else {
                const char* passthrough = kIRTrianglePassthroughGeometryShader;

                if (m_TessellationConfig.outputPrimitiveType == IRRuntimeTessellatorOutputPoint)
                    passthrough = kIRPointPassthroughGeometryShader;
                else if (m_TessellationConfig.outputPrimitiveType == IRRuntimeTessellatorOutputLine)
                    passthrough = kIRLinePassthroughGeometryShader;

                mpd->setMeshFunctionDescriptor(specialize(findLibrary(StageBits::TESS_EVALUATION_SHADER), passthrough));
            }

            constants->release();

            if (result == Result::SUCCESS) {
                MTL4::StaticLinkingDescriptor* linking = MTL4::StaticLinkingDescriptor::alloc()->init();
                linking->setFunctionDescriptors(NS::Array::array((const NS::Object* const*)objectFunctions, hasTessellation ? 2 : 1));
                mpd->setObjectStaticLinkingDescriptor(linking);
                linking->release();

                if (hasTessellation) {
                    linking = MTL4::StaticLinkingDescriptor::alloc()->init();
                    linking->setFunctionDescriptors(NS::Array::array((const NS::Object* const*)meshFunctions, 2));
                    mpd->setMeshStaticLinkingDescriptor(linking);
                    linking->release();
                }
            }
        }
#endif

        if (result == Result::SUCCESS) {
            m_Render = NewRenderPipelineMetal(m_Device, desc.cache, failOnCacheMiss, isMesh ? (MTL4::PipelineDescriptor*)mpd : (MTL4::PipelineDescriptor*)pd, &error);

            if (!m_Render) {
                result = Result::FAILURE;

                if (!failOnCacheMiss)
                    NRI_REPORT_ERROR(&m_Device, "Metal render pipeline creation failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");
            }
        }
    }

    // "CompareOp::NONE" disables the test, including depth writes and stencil operations
    const DepthAttachmentDesc& depth = desc.outputMerger.depth;
    const StencilAttachmentDesc& stencil = desc.outputMerger.stencil;

    MTL::DepthStencilDescriptor* dd = MTL::DepthStencilDescriptor::alloc()->init();
    dd->setDepthCompareFunction(GetCompareMetal(depth.compareOp));
    dd->setDepthWriteEnabled(depth.compareOp != CompareOp::NONE && depth.write);

    if (stencil.front.compareOp != CompareOp::NONE || stencil.back.compareOp != CompareOp::NONE) {
        MTL::StencilDescriptor* front = MTL::StencilDescriptor::alloc()->init();
        SetStencilMetal(front, stencil.front);

        MTL::StencilDescriptor* back = MTL::StencilDescriptor::alloc()->init();
        SetStencilMetal(back, stencil.back);

        dd->setFrontFaceStencil(front);
        dd->setBackFaceStencil(back);

        front->release();
        back->release();
    }

    m_DepthStencil = m_Device.GetNativeObject()->newDepthStencilState(dd);
    dd->release();

    if (pd)
        pd->release();

    if (mpd)
        mpd->release();

    if (stageInLibrary)
        stageInLibrary->release();

    for (MTL4::FunctionDescriptor* function : functions)
        function->release();

    for (MTL::Library* library : libraries)
        library->release();

    m_Primitive = g_PrimitiveTypes[(uint32_t)topology];
    m_Cull = g_CullModes[(uint32_t)desc.rasterization.cullMode];
    m_Winding = desc.rasterization.frontCounterClockwise ? MTL::WindingCounterClockwise : MTL::WindingClockwise;
    m_Fill = desc.rasterization.fillMode == FillMode::WIREFRAME ? MTL::TriangleFillModeLines : MTL::TriangleFillModeFill;
    m_DepthClip = desc.rasterization.depthClamp ? MTL::DepthClipModeClamp : MTL::DepthClipModeClip;
    m_DepthBounds = desc.outputMerger.depth.boundsTest;
    m_SampleLocations = desc.multisample && desc.multisample->sampleLocations;
    m_DepthBias = desc.rasterization.depthBias;

    return result;
}

DeviceMetal& PipelineMetal::GetDevice() const {
    return m_Device;
}

MTL::RenderPipelineState* PipelineMetal::GetRenderPipeline() const {
    return m_Render;
}

MTL::ComputePipelineState* PipelineMetal::GetComputePipeline() const {
    return m_Compute;
}

Multiview PipelineMetal::GetMultiview() const {
    return m_Multiview;
}

uint32_t PipelineMetal::GetViewMask() const {
    return m_ViewMask;
}

MTL::DepthStencilState* PipelineMetal::GetDepthStencilState() const {
    return m_DepthStencil;
}

MTL::PrimitiveType PipelineMetal::GetPrimitiveType() const {
    return m_Primitive;
}

MTL::CullMode PipelineMetal::GetCullMode() const {
    return m_Cull;
}

MTL::Winding PipelineMetal::GetWinding() const {
    return m_Winding;
}

MTL::TriangleFillMode PipelineMetal::GetFillMode() const {
    return m_Fill;
}

MTL::DepthClipMode PipelineMetal::GetDepthClipMode() const {
    return m_DepthClip;
}

bool PipelineMetal::IsDepthBoundsEnabled() const {
    return m_DepthBounds;
}

bool PipelineMetal::HasSampleLocations() const {
    return m_SampleLocations;
}

const DepthBiasDesc& PipelineMetal::GetDepthBias() const {
    return m_DepthBias;
}

MTL::Size PipelineMetal::GetThreadGroupSize() const {
    return m_ThreadGroup;
}

MTL::Size PipelineMetal::GetMeshThreadGroupSize() const {
    return m_MeshGroup;
}

MTL::Size PipelineMetal::GetTaskThreadGroupSize() const {
    return m_TaskGroup;
}

bool PipelineMetal::IsConverted() const {
    return m_Converted;
}

#if NRI_ENABLE_METAL_SHADER_CONVERTER
bool PipelineMetal::IsGeometryEmulation() const {
    return m_GeometryEmulation;
}

bool PipelineMetal::IsTessellationEmulation() const {
    return m_TessellationEmulation;
}

const IRRuntimeGeometryPipelineConfig& PipelineMetal::GetGeometryConfig() const {
    return m_GeometryConfig;
}

const IRRuntimeTessellationPipelineConfig& PipelineMetal::GetTessellationConfig() const {
    return m_TessellationConfig;
}

IRRuntimePrimitiveType PipelineMetal::GetEmulationPrimitive() const {
    return m_EmulationPrimitive;
}
#endif

void PipelineMetal::SetDebugName(const char* name) {
    // Pipeline state labels are immutable after creation.
    MaybeUnused(name);
}
