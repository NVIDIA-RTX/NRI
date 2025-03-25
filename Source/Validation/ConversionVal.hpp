// © 2021 NVIDIA Corporation

void nri::ConvertGeometryObjectsVal(BottomLevelGeometryDesc* destObjects, const BottomLevelGeometryDesc* sourceObjects, uint32_t objectNum) {
    for (uint32_t i = 0; i < objectNum; i++) {
        const BottomLevelGeometryDesc& src = sourceObjects[i];
        BottomLevelGeometryDesc& dst = destObjects[i];

        dst = src;
        if (src.type == BottomLevelGeometryType::TRIANGLES) {
            dst.geometry.triangles.vertexBuffer = NRI_GET_IMPL(Buffer, src.geometry.triangles.vertexBuffer);
            dst.geometry.triangles.indexBuffer = NRI_GET_IMPL(Buffer, src.geometry.triangles.indexBuffer);
            dst.geometry.triangles.transformBuffer = NRI_GET_IMPL(Buffer, src.geometry.triangles.transformBuffer);
        } else
            dst.geometry.aabbs.buffer = NRI_GET_IMPL(Buffer, src.geometry.aabbs.buffer);
    }
}
