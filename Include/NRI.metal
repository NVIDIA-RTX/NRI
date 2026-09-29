// © 2026 NVIDIA Corporation

#ifndef NRI_METAL
#define NRI_METAL

/*
Native Metal shader ABI (metallib shaders, not converted from DXIL).

Argument table:
    buffer(0)       - resource heap, "NriDescriptorEntry" array
    buffer(1)       - sampler heap, "NriDescriptorEntry" array
    buffer(2)       - root data (see below)
    buffer(3)       - "NriMultiview" (multiview pipelines only)
    buffer(4-5)     - reserved (converted shaders)
    buffer(6 + N)   - vertex stream with "bindingSlot = N"
    attribute(i)    - "VertexInputDesc::attributes[i]" (array order)
    color(i)        - input attachment "i" (i.e. "float4 value [[color(1)]]")

Descriptors ("NriDescriptorEntry", declare heaps and tables as arrays of argument-buffer structures with typed fields):
  - buffer: "bufferAddress" - GPU address (view offset applied), "metadata" - see "NriGetBufferSize"
  - typed buffer: "resourceId" - "texture_buffer", "bufferAddress" and "metadata" as for buffers
  - texture: "resourceId"
  - sampler: "bufferAddress" - "sampler", "metadata" - mip bias (see "NriGetSamplerMipBias")

Root data (buffer(2)), declare a matching "constant" structure including padding:
  - "uint baseVertex, baseInstance" - if "ENABLE_DRAW_PARAMETERS_EMULATION" is set and the layout has "VERTEX_SHADER" stage
  - "uint drawIndex" - if "ENABLE_DRAW_INDEX_EMULATION" is set and the layout has "VERTEX_SHADER" stage
  - root constants, tightly packed
  - root descriptors (GPU addresses), aligned to 8 bytes
  - per descriptor set: a pointer to its resource table, then a pointer to its sampler table (each only if the set has such ranges,
    "MUTABLE" and "INPUT_ATTACHMENT" ranges don't count)
  - a pointer to the root sampler table ("NriDescriptorEntry" per root sampler), if "rootSamplerNum != 0"
  - the whole block is padded to 16 bytes
  - within a table ranges keep declaration order, the variable-sized range goes last (offsets don't depend on the allocated count),
    "MUTABLE" and "INPUT_ATTACHMENT" ranges keep their slots

Draw parameters:
  - "[[vertex_id]]" and "[[instance_id]]" include the base vertex / instance: subtract "[[base_vertex]]" / "[[base_instance]]"
    (or the root data fields) to get zero-based NRI IDs
  - "drawIndex" is 0 for direct "Draw" calls

Multiview:
  - "[[amplification_id]]" indexes "NriMultiview::viewIndices", which stores the view indices of the set bits of the view mask
    (sparse masks are supported)
  - "LAYER_BASED" and "VIEWPORT_BASED" offsets are applied by Metal, "FLEXIBLE" shaders write "[[render_target_array_index]]"
    and/or "[[viewport_array_index]]" themselves

Textures:
  - 1D textures are height-one 2D textures: use "texture2d(_array)", Y = 0 for reads / writes, normalized Y = 0.5 for sampling
  - 3D shader views expose the full depth of their mips, "TextureViewDesc" slice ranges apply only to attachments and storage clears

Sample mask:
  - fragment shaders opt into "MultisampleDesc::sampleMask" by combining "NriApplyPipelineSampleMask(shaderMask)" into their
    "[[sample_mask]]" output, otherwise a non-default "sampleMask" makes pipeline creation return "UNSUPPORTED"
  - function constant index 65535 is reserved
*/

struct NriDescriptorEntry {
    ulong bufferAddress;
    ulong resourceId;
    ulong metadata;
};

struct NriMultiview {
    uint viewIndices[32];
};

static_assert(sizeof(NriDescriptorEntry) == 24, "Descriptor ABI mismatch");

// Buffer metadata: bits 0-31 - view size in bytes, bits 32-39 - typed buffer element offset, bit 63 - typed buffer
inline uint NriGetBufferSize(ulong metadata) {
    return uint(metadata);
}

// "texture_buffer" views start at an aligned offset (the alignment depends on the device and format): add this to typed buffer element indices
inline uint NriGetTypedBufferElementOffset(ulong metadata) {
    return uint(metadata >> 32) & 0xFF;
}

// Pass as "metal::bias" when sampling with implicit LOD. 0 on Apple10+, where the sampler applies "mipBias" itself
// (earlier GPUs ignore "MTLSamplerDescriptor::lodBias")
inline float NriGetSamplerMipBias(ulong metadata) {
    return as_type<float>(uint(metadata));
}

constant uint NriPipelineSampleMask [[function_constant(65535)]];

inline uint NriApplyPipelineSampleMask(uint shaderSampleMask = 0xFFFFFFFFu) {
    return shaderSampleMask & NriPipelineSampleMask;
}

#endif
