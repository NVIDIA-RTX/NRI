// © 2021 NVIDIA Corporation

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
