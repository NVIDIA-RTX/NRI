/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "SharedVK.h"
#include "BufferVK.h"

using namespace nri;

void nri::ConvertGeometryObjectSizesVK(uint32_t physicalDeviceIndex, VkAccelerationStructureGeometryKHR* destObjects, uint32_t* primitiveNums,
    const GeometryObject* sourceObjects, uint32_t objectNum)
{
    for (uint32_t i = 0; i < objectNum; i++)
    {
        const GeometryObject& geometrySrc = sourceObjects[i];

        uint32_t triangleNum = geometrySrc.triangles.indexNum / 3;
        triangleNum += (geometrySrc.triangles.indexNum == 0) ? geometrySrc.triangles.vertexNum / 3 : 0;

        const uint32_t boxNum = geometrySrc.boxes.boxNum;
        primitiveNums[i] = geometrySrc.type == GeometryType::TRIANGLES ? triangleNum : boxNum;

        const VkDeviceAddress transform = GetBufferDeviceAddress(geometrySrc.triangles.transformBuffer, physicalDeviceIndex) +
            geometrySrc.triangles.transformOffset;

        VkAccelerationStructureGeometryKHR& geometryDst = destObjects[i];
        geometryDst = {};
        geometryDst.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometryDst.flags = GetGeometryFlags(geometrySrc.flags);
        geometryDst.geometryType = GetGeometryType(geometrySrc.type);
        geometryDst.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometryDst.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
        geometryDst.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometryDst.geometry.triangles.maxVertex = geometrySrc.triangles.vertexNum;
        geometryDst.geometry.triangles.indexType = GetIndexType(geometrySrc.triangles.indexType);
        geometryDst.geometry.triangles.vertexFormat = GetVkFormat(geometrySrc.triangles.vertexFormat);
        geometryDst.geometry.triangles.transformData.deviceAddress = transform;
    }
}

void nri::ConvertGeometryObjectsVK(uint32_t physicalDeviceIndex, VkAccelerationStructureGeometryKHR* destObjects,
    VkAccelerationStructureBuildRangeInfoKHR* ranges, const GeometryObject* sourceObjects, uint32_t objectNum)
{
    for (uint32_t i = 0; i < objectNum; i++)
    {
        const GeometryObject& geometrySrc = sourceObjects[i];

        uint32_t triangleNum = geometrySrc.triangles.indexNum / 3;
        triangleNum += (geometrySrc.triangles.indexNum == 0) ? geometrySrc.triangles.vertexNum / 3 : 0;

        const uint32_t boxNum = geometrySrc.boxes.boxNum;

        ranges[i] = {};
        ranges[i].primitiveCount = geometrySrc.type == GeometryType::TRIANGLES ? triangleNum : boxNum;

        const VkDeviceAddress aabbs = GetBufferDeviceAddress(geometrySrc.boxes.buffer, physicalDeviceIndex) +
            geometrySrc.boxes.offset;

        const VkDeviceAddress vertices = GetBufferDeviceAddress(geometrySrc.triangles.vertexBuffer, physicalDeviceIndex) +
            geometrySrc.triangles.vertexOffset;

        const VkDeviceAddress indices = GetBufferDeviceAddress(geometrySrc.triangles.indexBuffer, physicalDeviceIndex) +
            geometrySrc.triangles.indexOffset;

        const VkDeviceAddress transform = GetBufferDeviceAddress(geometrySrc.triangles.transformBuffer, physicalDeviceIndex) +
            geometrySrc.triangles.transformOffset;

        VkAccelerationStructureGeometryKHR& geometryDst = destObjects[i];
        geometryDst = {};
        geometryDst.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometryDst.flags = GetGeometryFlags(geometrySrc.flags);
        geometryDst.geometryType = GetGeometryType(geometrySrc.type);
        geometryDst.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometryDst.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
        geometryDst.geometry.aabbs.data.deviceAddress = aabbs;
        geometryDst.geometry.aabbs.stride = geometrySrc.boxes.stride;
        geometryDst.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometryDst.geometry.triangles.maxVertex = geometrySrc.triangles.vertexNum;
        geometryDst.geometry.triangles.vertexData.deviceAddress = vertices;
        geometryDst.geometry.triangles.vertexStride = geometrySrc.triangles.vertexStride;
        geometryDst.geometry.triangles.vertexFormat = GetVkFormat(geometrySrc.triangles.vertexFormat);
        geometryDst.geometry.triangles.indexData.deviceAddress = indices;
        geometryDst.geometry.triangles.indexType = GetIndexType(geometrySrc.triangles.indexType);
        geometryDst.geometry.triangles.transformData.deviceAddress = transform;
    }
}

TextureType GetTextureTypeVK(uint32_t vkImageType)
{
    return GetTextureType((VkImageType)vkImageType);
}

QueryType GetQueryTypeVK(uint32_t queryTypeVK)
{
    if (queryTypeVK == VK_QUERY_TYPE_OCCLUSION)
        return QueryType::OCCLUSION;

    if (queryTypeVK == VK_QUERY_TYPE_PIPELINE_STATISTICS)
        return QueryType::PIPELINE_STATISTICS;

    if (queryTypeVK == VK_QUERY_TYPE_TIMESTAMP)
        return QueryType::TIMESTAMP;

    return QueryType::MAX_NUM;
}