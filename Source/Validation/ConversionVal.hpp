// © 2021 NVIDIA Corporation

void nri::ConvertGeometryObjectsVal(BottomLevelGeometryDesc* destObjects, const BottomLevelGeometryDesc* sourceObjects, uint32_t objectNum) {
    for (uint32_t i = 0; i < objectNum; i++) {
        const BottomLevelGeometryDesc& src = sourceObjects[i];
        BottomLevelGeometryDesc& dst = destObjects[i];

        dst = src;
        if (src.type == BottomLevelGeometryType::TRIANGLES) {
            dst.triangles.vertexBuffer = NRI_GET_IMPL(Buffer, src.triangles.vertexBuffer);
            dst.triangles.indexBuffer = NRI_GET_IMPL(Buffer, src.triangles.indexBuffer);
            dst.triangles.transformBuffer = NRI_GET_IMPL(Buffer, src.triangles.transformBuffer);
            dst.triangles.micromap.micromap = NRI_GET_IMPL(Micromap, src.triangles.micromap.micromap);
            dst.triangles.micromap.indexBuffer = NRI_GET_IMPL(Buffer, src.triangles.micromap.indexBuffer);
        } else
            dst.aabbs.buffer = NRI_GET_IMPL(Buffer, src.aabbs.buffer);
    }
}
