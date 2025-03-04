// © 2021 NVIDIA Corporation

void nri::ConvertGeometryObjectSizesVK(VkAccelerationStructureGeometryKHR* destObjects, uint32_t* primitiveNums, const GeometryObject* sourceObjects, uint32_t objectNum) {
    for (uint32_t i = 0; i < objectNum; i++) {
        const GeometryObject& src = sourceObjects[i];

        uint32_t triangleNum = (src.geometry.triangles.indexNum ? src.geometry.triangles.indexNum : src.geometry.triangles.vertexNum) / 3;
        VkDeviceAddress transform = GetBufferDeviceAddress(src.geometry.triangles.transformBuffer) + src.geometry.triangles.transformOffset;

        primitiveNums[i] = src.type == GeometryType::TRIANGLES ? triangleNum : src.geometry.aabbs.boxNum;

        VkAccelerationStructureGeometryKHR& geometryDst = destObjects[i];
        geometryDst = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geometryDst.flags = GetGeometryFlags(src.flags);
        geometryDst.geometryType = GetGeometryType(src.type);
        geometryDst.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometryDst.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
        geometryDst.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometryDst.geometry.triangles.maxVertex = src.geometry.triangles.vertexNum;
        geometryDst.geometry.triangles.indexType = GetIndexType(src.geometry.triangles.indexType);
        geometryDst.geometry.triangles.vertexFormat = GetVkFormat(src.geometry.triangles.vertexFormat);
        geometryDst.geometry.triangles.transformData.deviceAddress = transform;
    }
}

void nri::ConvertGeometryObjectsVK(VkAccelerationStructureGeometryKHR* destObjects, VkAccelerationStructureBuildRangeInfoKHR* ranges, const GeometryObject* sourceObjects, uint32_t objectNum) {
    for (uint32_t i = 0; i < objectNum; i++) {
        const GeometryObject& src = sourceObjects[i];

        uint32_t triangleNum = (src.geometry.triangles.indexNum ? src.geometry.triangles.indexNum : src.geometry.triangles.vertexNum) / 3;

        VkDeviceAddress aabbs = GetBufferDeviceAddress(src.geometry.aabbs.buffer) + src.geometry.aabbs.offset;
        VkDeviceAddress vertices = GetBufferDeviceAddress(src.geometry.triangles.vertexBuffer) + src.geometry.triangles.vertexOffset;
        VkDeviceAddress indices = GetBufferDeviceAddress(src.geometry.triangles.indexBuffer) + src.geometry.triangles.indexOffset;
        VkDeviceAddress transform = GetBufferDeviceAddress(src.geometry.triangles.transformBuffer) + src.geometry.triangles.transformOffset;

        ranges[i] = {};
        ranges[i].primitiveCount = src.type == GeometryType::TRIANGLES ? triangleNum : src.geometry.aabbs.boxNum;

        VkAccelerationStructureGeometryKHR& geometryDst = destObjects[i];
        geometryDst = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geometryDst.flags = GetGeometryFlags(src.flags);
        geometryDst.geometryType = GetGeometryType(src.type);
        geometryDst.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometryDst.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
        geometryDst.geometry.aabbs.data.deviceAddress = aabbs;
        geometryDst.geometry.aabbs.stride = src.geometry.aabbs.stride;
        geometryDst.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometryDst.geometry.triangles.maxVertex = src.geometry.triangles.vertexNum;
        geometryDst.geometry.triangles.vertexData.deviceAddress = vertices;
        geometryDst.geometry.triangles.vertexStride = src.geometry.triangles.vertexStride;
        geometryDst.geometry.triangles.vertexFormat = GetVkFormat(src.geometry.triangles.vertexFormat);
        geometryDst.geometry.triangles.indexData.deviceAddress = indices;
        geometryDst.geometry.triangles.indexType = GetIndexType(src.geometry.triangles.indexType);
        geometryDst.geometry.triangles.transformData.deviceAddress = transform;
    }
}

TextureType GetTextureTypeVK(uint32_t vkImageType) {
    return GetTextureType((VkImageType)vkImageType);
}

QueryType GetQueryTypeVK(uint32_t queryTypeVK) {
    if (queryTypeVK == VK_QUERY_TYPE_OCCLUSION)
        return QueryType::OCCLUSION;

    if (queryTypeVK == VK_QUERY_TYPE_PIPELINE_STATISTICS)
        return QueryType::PIPELINE_STATISTICS;

    if (queryTypeVK == VK_QUERY_TYPE_TIMESTAMP)
        return QueryType::TIMESTAMP;

    return QueryType::MAX_NUM;
}

// Each depth/stencil format is only compatible with itself in VK
constexpr std::array<VkFormat, (size_t)Format::MAX_NUM> g_Formats = {
    VK_FORMAT_UNDEFINED,                // UNKNOWN
    VK_FORMAT_R8_UNORM,                 // R8_UNORM
    VK_FORMAT_R8_SNORM,                 // R8_SNORM
    VK_FORMAT_R8_UINT,                  // R8_UINT
    VK_FORMAT_R8_SINT,                  // R8_SINT
    VK_FORMAT_R8G8_UNORM,               // RG8_UNORM
    VK_FORMAT_R8G8_SNORM,               // RG8_SNORM
    VK_FORMAT_R8G8_UINT,                // RG8_UINT
    VK_FORMAT_R8G8_SINT,                // RG8_SINT
    VK_FORMAT_B8G8R8A8_UNORM,           // BGRA8_UNORM
    VK_FORMAT_B8G8R8A8_SRGB,            // BGRA8_SRGB
    VK_FORMAT_R8G8B8A8_UNORM,           // RGBA8_UNORM
    VK_FORMAT_R8G8B8A8_SRGB,            // RGBA8_SRGB
    VK_FORMAT_R8G8B8A8_SNORM,           // RGBA8_SNORM
    VK_FORMAT_R8G8B8A8_UINT,            // RGBA8_UINT
    VK_FORMAT_R8G8B8A8_SINT,            // RGBA8_SINT
    VK_FORMAT_R16_UNORM,                // R16_UNORM
    VK_FORMAT_R16_SNORM,                // R16_SNORM
    VK_FORMAT_R16_UINT,                 // R16_UINT
    VK_FORMAT_R16_SINT,                 // R16_SINT
    VK_FORMAT_R16_SFLOAT,               // R16_SFLOAT
    VK_FORMAT_R16G16_UNORM,             // RG16_UNORM
    VK_FORMAT_R16G16_SNORM,             // RG16_SNORM
    VK_FORMAT_R16G16_UINT,              // RG16_UINT
    VK_FORMAT_R16G16_SINT,              // RG16_SINT
    VK_FORMAT_R16G16_SFLOAT,            // RG16_SFLOAT
    VK_FORMAT_R16G16B16A16_UNORM,       // RGBA16_UNORM
    VK_FORMAT_R16G16B16A16_SNORM,       // RGBA16_SNORM
    VK_FORMAT_R16G16B16A16_UINT,        // RGBA16_UINT
    VK_FORMAT_R16G16B16A16_SINT,        // RGBA16_SINT
    VK_FORMAT_R16G16B16A16_SFLOAT,      // RGBA16_SFLOAT
    VK_FORMAT_R32_UINT,                 // R32_UINT
    VK_FORMAT_R32_SINT,                 // R32_SINT
    VK_FORMAT_R32_SFLOAT,               // R32_SFLOAT
    VK_FORMAT_R32G32_UINT,              // RG32_UINT
    VK_FORMAT_R32G32_SINT,              // RG32_SINT
    VK_FORMAT_R32G32_SFLOAT,            // RG32_SFLOAT
    VK_FORMAT_R32G32B32_UINT,           // RGB32_UINT
    VK_FORMAT_R32G32B32_SINT,           // RGB32_SINT
    VK_FORMAT_R32G32B32_SFLOAT,         // RGB32_SFLOAT
    VK_FORMAT_R32G32B32A32_UINT,        // RGB32_UINT
    VK_FORMAT_R32G32B32A32_SINT,        // RGB32_SINT
    VK_FORMAT_R32G32B32A32_SFLOAT,      // RGB32_SFLOAT
    VK_FORMAT_R5G6B5_UNORM_PACK16,      // B5_G6_R5_UNORM
    VK_FORMAT_A1R5G5B5_UNORM_PACK16,    // B5_G5_R5_A1_UNORM
    VK_FORMAT_A4R4G4B4_UNORM_PACK16,    // B4_G4_R4_A4_UNORM
    VK_FORMAT_A2B10G10R10_UNORM_PACK32, // R10_G10_B10_A2_UNORM
    VK_FORMAT_A2B10G10R10_UINT_PACK32,  // R10_G10_B10_A2_UINT
    VK_FORMAT_B10G11R11_UFLOAT_PACK32,  // R11_G11_B10_UFLOAT
    VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,   // R9_G9_B9_E5_UFLOAT
    VK_FORMAT_BC1_RGBA_UNORM_BLOCK,     // BC1_RGBA_UNORM
    VK_FORMAT_BC1_RGBA_SRGB_BLOCK,      // BC1_RGBA_SRGB
    VK_FORMAT_BC2_UNORM_BLOCK,          // BC2_RGBA_UNORM
    VK_FORMAT_BC2_SRGB_BLOCK,           // BC2_RGBA_SRGB
    VK_FORMAT_BC3_UNORM_BLOCK,          // BC3_RGBA_UNORM
    VK_FORMAT_BC3_SRGB_BLOCK,           // BC3_RGBA_SRGB
    VK_FORMAT_BC4_UNORM_BLOCK,          // BC4_R_UNORM
    VK_FORMAT_BC4_SNORM_BLOCK,          // BC4_R_SNORM
    VK_FORMAT_BC5_UNORM_BLOCK,          // BC5_RG_UNORM
    VK_FORMAT_BC5_SNORM_BLOCK,          // BC5_RG_SNORM
    VK_FORMAT_BC6H_UFLOAT_BLOCK,        // BC6H_RGB_UFLOAT
    VK_FORMAT_BC6H_SFLOAT_BLOCK,        // BC6H_RGB_SFLOAT
    VK_FORMAT_BC7_UNORM_BLOCK,          // BC7_RGBA_UNORM
    VK_FORMAT_BC7_SRGB_BLOCK,           // BC7_RGBA_SRGB
    VK_FORMAT_D16_UNORM,                // D16_UNORM
    VK_FORMAT_D24_UNORM_S8_UINT,        // D24_UNORM_S8_UINT
    VK_FORMAT_D32_SFLOAT,               // D32_SFLOAT
    VK_FORMAT_D32_SFLOAT_S8_UINT,       // D32_SFLOAT_S8_UINT_X24
    VK_FORMAT_D24_UNORM_S8_UINT,        // R24_UNORM_X8
    VK_FORMAT_D24_UNORM_S8_UINT,        // X24_G8_UINT
    VK_FORMAT_D32_SFLOAT_S8_UINT,       // R32_SFLOAT_X8_X24
    VK_FORMAT_D32_SFLOAT_S8_UINT,       // X32_G8_UINT_X24
};
VALIDATE_ARRAY(g_Formats);

uint32_t NRIFormatToVKFormat(Format format) {
    return (uint32_t)g_Formats[(uint32_t)format];
}
