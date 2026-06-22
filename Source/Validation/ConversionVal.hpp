// © 2021 NVIDIA Corporation

constexpr std::array<const char*, (size_t)DescriptorType::MAX_NUM> g_descriptorTypeNames = {
    "SAMPLER",                      // SAMPLER
    "MUTABLE",                      // MUTABLE
    "TEXTURE",                      // TEXTURE
    "STORAGE_TEXTURE",              // STORAGE_TEXTURE
    "INPUT_ATTACHMENT",             // INPUT_ATTACHMENT
    "BUFFER",                       // BUFFER
    "STORAGE_BUFFER",               // STORAGE_BUFFER
    "CONSTANT_BUFFER",              // CONSTANT_BUFFER
    "STRUCTURED_BUFFER",            // STRUCTURED_BUFFER
    "STORAGE_STRUCTURED_BUFFER",    // STORAGE_STRUCTURED_BUFFER
    "ACCELERATION_STRUCTURE",       // ACCELERATION_STRUCTURE
};
NRI_VALIDATE_ARRAY_BY_PTR(g_descriptorTypeNames);

const char* nri::GetDescriptorTypeName(DescriptorType descriptorType) {
    return g_descriptorTypeNames[(uint32_t)descriptorType];
}

void nri::ConvertBotomLevelGeometries(const BottomLevelGeometryDesc* geometries, uint32_t geometryNum, BottomLevelGeometryDesc*& outGeometries, BottomLevelMicromapDesc*& outMicromaps) {
    for (uint32_t i = 0; i < geometryNum; i++) {
        const BottomLevelGeometryDesc& src = geometries[i];

        BottomLevelGeometryDesc& dst = *outGeometries++;
        dst = src;

        if (src.type == BottomLevelGeometryType::TRIANGLES) {
            dst.triangles.vertexBuffer = NRI_GET_IMPL(Buffer, src.triangles.vertexBuffer);
            dst.triangles.indexBuffer = NRI_GET_IMPL(Buffer, src.triangles.indexBuffer);
            dst.triangles.transformBuffer = NRI_GET_IMPL(Buffer, src.triangles.transformBuffer);

            if (src.triangles.micromap) {
                dst.triangles.micromap = outMicromaps++;

                *dst.triangles.micromap = *src.triangles.micromap;
                dst.triangles.micromap->micromap = NRI_GET_IMPL(Micromap, src.triangles.micromap->micromap);
                dst.triangles.micromap->indexBuffer = NRI_GET_IMPL(Buffer, src.triangles.micromap->indexBuffer);
            }
        } else
            dst.aabbs.buffer = NRI_GET_IMPL(Buffer, src.aabbs.buffer);
    }
}
