// © 2021 NVIDIA Corporation

#include <math.h>

static uint32_t ConvertBotomLevelGeometries(VkAccelerationStructureGeometryKHR* vkGeometries, VkAccelerationStructureBuildRangeInfoKHR* ranges, const BottomLevelGeometryDesc* geometries, uint32_t geometryNum) {
    uint32_t micromapNum = 0;

    for (uint32_t i = 0; i < geometryNum; i++) {
        const BottomLevelGeometryDesc& in = geometries[i];
        VkAccelerationStructureGeometryKHR& out = vkGeometries[i];

        out = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        out.flags = GetGeometryFlags(in.flags);
        out.geometryType = GetGeometryType(in.type);

        ranges[i] = {}; // TODO: review struct fields...

        if (in.type == BottomLevelGeometryType::TRIANGLES) {
            const BottomLevelTrianglesDesc& tris = in.geometry.triangles;

            uint32_t triangleNum = (tris.indexNum ? tris.indexNum : tris.vertexNum) / 3;
            ranges[i].primitiveCount = triangleNum;

            VkAccelerationStructureGeometryTrianglesDataKHR& geometryTriangles = out.geometry.triangles;
            geometryTriangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            geometryTriangles.maxVertex = tris.vertexNum;
            geometryTriangles.vertexData.deviceAddress = GetBufferDeviceAddress(tris.vertexBuffer) + tris.vertexOffset;
            geometryTriangles.vertexStride = tris.vertexStride;
            geometryTriangles.vertexFormat = GetVkFormat(tris.vertexFormat);
            geometryTriangles.transformData.deviceAddress = GetBufferDeviceAddress(tris.transformBuffer) + tris.transformOffset;

            if (tris.indexBuffer) {
                geometryTriangles.indexType = GetIndexType(tris.indexType);
                geometryTriangles.indexData.deviceAddress = GetBufferDeviceAddress(tris.indexBuffer) + tris.indexOffset;
            } else
                geometryTriangles.indexType = VK_INDEX_TYPE_NONE_KHR;

            if (tris.micromap)
                micromapNum++;
        } else if (in.type == BottomLevelGeometryType::AABBS) {
            const BottomLevelAabbsDesc& aabbs = in.geometry.aabbs;

            ranges[i].primitiveCount = aabbs.num;

            VkAccelerationStructureGeometryAabbsDataKHR& geometryAabbs = out.geometry.aabbs;
            geometryAabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
            geometryAabbs.data.deviceAddress = GetBufferDeviceAddress(aabbs.buffer) + aabbs.offset;
            geometryAabbs.stride = aabbs.stride;
        }
    }

    return micromapNum;
}

CommandBufferVK::~CommandBufferVK() {
    if (m_CommandPool == VK_NULL_HANDLE)
        return;

    const auto& vk = m_Device.GetDispatchTable();
    vk.FreeCommandBuffers(m_Device, m_CommandPool, 1, &m_Handle);
}

void CommandBufferVK::Create(VkCommandPool commandPool, VkCommandBuffer commandBuffer, QueueType type) {
    m_CommandPool = commandPool;
    m_Handle = commandBuffer;
    m_Type = type;
}

Result CommandBufferVK::Create(const CommandBufferVKDesc& commandBufferDesc) {
    m_CommandPool = VK_NULL_HANDLE;
    m_Handle = (VkCommandBuffer)commandBufferDesc.vkCommandBuffer;
    m_Type = commandBufferDesc.queueType;

    return Result::SUCCESS;
}

NRI_INLINE void CommandBufferVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)m_Handle, name);
}

NRI_INLINE Result CommandBufferVK::Begin(const DescriptorPool*) {
    VkCommandBufferBeginInfo info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.BeginCommandBuffer(m_Handle, &info);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkBeginCommandBuffer returned %d", (int32_t)result);

    m_PipelineLayout = nullptr;
    m_Pipeline = nullptr;

    return Result::SUCCESS;
}

NRI_INLINE Result CommandBufferVK::End() {
    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.EndCommandBuffer(m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkEndCommandBuffer returned %d", (int32_t)result);

    return Result::SUCCESS;
}

NRI_INLINE void CommandBufferVK::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    Scratch<VkViewport> vkViewports = AllocateScratch(m_Device, VkViewport, viewportNum);
    for (uint32_t i = 0; i < viewportNum; i++) {
        const Viewport& in = viewports[i];
        VkViewport& out = vkViewports[i];
        out.x = in.x;
        out.y = in.y;
        out.width = in.width;
        out.height = in.height;
        out.minDepth = in.depthMin;
        out.maxDepth = in.depthMax;

        // Origin top-left requires flipping
        if (!in.originBottomLeft) {
            out.y += in.height;
            out.height = -in.height;
        }
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetViewportWithCount(m_Handle, viewportNum, vkViewports);
}

NRI_INLINE void CommandBufferVK::SetScissors(const Rect* rects, uint32_t rectNum) {
    Scratch<VkRect2D> vkRects = AllocateScratch(m_Device, VkRect2D, rectNum);
    for (uint32_t i = 0; i < rectNum; i++) {
        const Rect& in = rects[i];
        VkRect2D& out = vkRects[i];
        out.offset.x = in.x;
        out.offset.y = in.y;
        out.extent.width = in.width;
        out.extent.height = in.height;
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetScissorWithCount(m_Handle, rectNum, vkRects);
}

NRI_INLINE void CommandBufferVK::SetDepthBounds(float boundsMin, float boundsMax) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetDepthBounds(m_Handle, boundsMin, boundsMax);
}

NRI_INLINE void CommandBufferVK::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    const auto& vk = m_Device.GetDispatchTable();

    if (frontRef == backRef)
        vk.CmdSetStencilReference(m_Handle, VK_STENCIL_FRONT_AND_BACK, frontRef);
    else {
        vk.CmdSetStencilReference(m_Handle, VK_STENCIL_FACE_FRONT_BIT, frontRef);
        vk.CmdSetStencilReference(m_Handle, VK_STENCIL_FACE_BACK_BIT, backRef);
    }
}

NRI_INLINE void CommandBufferVK::SetSampleLocations(const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    Scratch<VkSampleLocationEXT> sampleLocations = AllocateScratch(m_Device, VkSampleLocationEXT, locationNum);
    for (uint32_t i = 0; i < locationNum; i++)
        sampleLocations[i] = {(float)(locations[i].x + 8) / 16.0f, (float)(locations[i].y + 8) / 16.0f};

    uint32_t gridDim = (uint32_t)sqrtf((float)locationNum / (float)sampleNum);

    VkSampleLocationsInfoEXT sampleLocationsInfo = {VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT};
    sampleLocationsInfo.sampleLocationsPerPixel = (VkSampleCountFlagBits)sampleNum;
    sampleLocationsInfo.sampleLocationGridSize = {gridDim, gridDim};
    sampleLocationsInfo.sampleLocationsCount = locationNum;
    sampleLocationsInfo.pSampleLocations = sampleLocations;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetSampleLocationsEXT(m_Handle, &sampleLocationsInfo);
}

NRI_INLINE void CommandBufferVK::SetBlendConstants(const Color32f& color) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetBlendConstants(m_Handle, &color.x);
}

NRI_INLINE void CommandBufferVK::SetShadingRate(const ShadingRateDesc& shadingRateDesc) {
    VkExtent2D shadingRate = GetShadingRate(shadingRateDesc.shadingRate);
    VkFragmentShadingRateCombinerOpKHR combiners[2] = {
        GetShadingRateCombiner(shadingRateDesc.primitiveCombiner),
        GetShadingRateCombiner(shadingRateDesc.attachmentCombiner),
    };

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdSetFragmentShadingRateKHR(m_Handle, &shadingRate, combiners);
}

NRI_INLINE void CommandBufferVK::SetDepthBias(const DepthBiasDesc& depthBiasDesc) {
    if (!m_Pipeline || IsDepthBiasEnabled(m_Pipeline->GetDepthBias())) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.CmdSetDepthBias(m_Handle, depthBiasDesc.constant, depthBiasDesc.clamp, depthBiasDesc.slope);
    }
}

NRI_INLINE void CommandBufferVK::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    static_assert(sizeof(VkClearValue) == sizeof(ClearValue), "Sizeof mismatch");

    if (!clearDescNum)
        return;

    // Attachments
    uint32_t attachmentNum = 0;
    Scratch<VkClearAttachment> attachments = AllocateScratch(m_Device, VkClearAttachment, clearDescNum);

    for (uint32_t i = 0; i < clearDescNum; i++) {
        const ClearDesc& desc = clearDescs[i];

        VkImageAspectFlags aspectMask = 0;
        if (desc.planes & PlaneBits::COLOR)
            aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
        if ((desc.planes & PlaneBits::DEPTH) && m_DepthStencil->IsDepthWritable())
            aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
        if ((desc.planes & PlaneBits::STENCIL) && m_DepthStencil->IsStencilWritable())
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

        if (aspectMask) {
            VkClearAttachment& attachment = attachments[attachmentNum++];

            attachment = {};
            attachment.aspectMask = aspectMask;
            attachment.colorAttachment = desc.colorAttachmentIndex;
            attachment.clearValue = *(VkClearValue*)&desc.value;
        }
    }

    // Rects
    bool hasRects = rectNum != 0;
    if (!hasRects)
        rectNum = 1;

    Scratch<VkClearRect> clearRects = AllocateScratch(m_Device, VkClearRect, rectNum);
    for (uint32_t i = 0; i < rectNum; i++) {
        VkClearRect& clearRect = clearRects[i];

        clearRect = {};

        // TODO: allow layer specification for clears?
        clearRect.baseArrayLayer = 0;
        clearRect.layerCount = m_ViewMask ? 1 : m_RenderLayerNum; // per VK spec...

        if (hasRects) {
            const Rect& rect = rects[i];
            clearRect.rect = {{rect.x, rect.y}, {rect.width, rect.height}};
        } else
            clearRect.rect = {{0, 0}, {m_RenderWidth, m_RenderHeight}};
    }

    if (attachmentNum) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.CmdClearAttachments(m_Handle, attachmentNum, attachments, rectNum, clearRects);
    }
}

NRI_INLINE void CommandBufferVK::ClearStorage(const ClearStorageDesc& clearDesc) {
    const DescriptorVK& storage = *(DescriptorVK*)clearDesc.storage;

    const auto& vk = m_Device.GetDispatchTable();
    if (storage.GetType() == DescriptorTypeVK::BUFFER_VIEW) {
        const DescriptorBufDesc& bufDesc = storage.GetBufDesc();
        vk.CmdFillBuffer(m_Handle, bufDesc.handle, bufDesc.offset, bufDesc.size, clearDesc.value.ui.x);
    } else {
        static_assert(sizeof(VkClearColorValue) == sizeof(clearDesc.value), "Unexpected sizeof");

        const VkClearColorValue* value = (VkClearColorValue*)&clearDesc.value;
        VkImageSubresourceRange range = storage.GetImageSubresourceRange();
        vk.CmdClearColorImage(m_Handle, storage.GetImage(), IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, value, 1, &range);
    }
}

NRI_INLINE void CommandBufferVK::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    const DeviceDesc& deviceDesc = m_Device.GetDesc();

    // TODO: if there are no attachments, render area has max dimensions. It can be suboptimal even on desktop. It's a no-go on tiled architectures
    m_RenderLayerNum = deviceDesc.attachmentLayerMaxNum;
    m_RenderWidth = deviceDesc.attachmentMaxDim;
    m_RenderHeight = deviceDesc.attachmentMaxDim;

    // Color
    Scratch<VkRenderingAttachmentInfo> colors = AllocateScratch(m_Device, VkRenderingAttachmentInfo, attachmentsDesc.colorNum);
    for (uint32_t i = 0; i < attachmentsDesc.colorNum; i++) {
        const DescriptorVK& descriptor = *(DescriptorVK*)attachmentsDesc.colors[i];
        const DescriptorTexDesc& desc = descriptor.GetTexDesc();

        VkRenderingAttachmentInfo& color = colors[i];
        color = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        color.imageView = descriptor.GetImageView();
        color.imageLayout = descriptor.GetTexDesc().layout;
        color.resolveMode = VK_RESOLVE_MODE_NONE;
        color.resolveImageView = VK_NULL_HANDLE;
        color.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.clearValue = {};

        Dim_t w = desc.texture->GetSize(0, desc.mipOffset);
        Dim_t h = desc.texture->GetSize(1, desc.mipOffset);

        m_RenderLayerNum = std::min(m_RenderLayerNum, desc.layerNum);
        m_RenderWidth = std::min(m_RenderWidth, w);
        m_RenderHeight = std::min(m_RenderHeight, h);
    }

    // Depth-stencil
    VkRenderingAttachmentInfo depthStencil = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    bool hasStencil = false;
    if (attachmentsDesc.depthStencil) {
        const DescriptorVK& descriptor = *(DescriptorVK*)attachmentsDesc.depthStencil;
        const DescriptorTexDesc& desc = descriptor.GetTexDesc();

        depthStencil.imageView = descriptor.GetImageView();
        depthStencil.imageLayout = desc.layout;
        depthStencil.resolveMode = VK_RESOLVE_MODE_NONE;
        depthStencil.resolveImageView = VK_NULL_HANDLE;
        depthStencil.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthStencil.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthStencil.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthStencil.clearValue = {};

        Dim_t w = desc.texture->GetSize(0, desc.mipOffset);
        Dim_t h = desc.texture->GetSize(1, desc.mipOffset);

        m_RenderLayerNum = std::min(m_RenderLayerNum, desc.layerNum);
        m_RenderWidth = std::min(m_RenderWidth, w);
        m_RenderHeight = std::min(m_RenderHeight, h);

        hasStencil = HasStencil(descriptor.GetTexture().GetDesc().format);

        m_DepthStencil = &descriptor;
    } else
        m_DepthStencil = nullptr;

    // Shading rate
    VkRenderingFragmentShadingRateAttachmentInfoKHR shadingRate = {VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR};
    if (attachmentsDesc.shadingRate) {
        uint32_t tileSize = m_Device.GetDesc().shadingRateAttachmentTileSize;
        const DescriptorVK& descriptor = *(DescriptorVK*)attachmentsDesc.shadingRate;

        shadingRate.imageView = descriptor.GetImageView();
        shadingRate.imageLayout = descriptor.GetTexDesc().layout;
        shadingRate.shadingRateAttachmentTexelSize = {tileSize, tileSize};
    }

    bool hasAttachment = attachmentsDesc.depthStencil || attachmentsDesc.colors;
    if (!hasAttachment)
        m_RenderLayerNum = 1;

    VkRenderingInfo renderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    renderingInfo.flags = 0;
    renderingInfo.renderArea = {{0, 0}, {m_RenderWidth, m_RenderHeight}};
    renderingInfo.layerCount = m_RenderLayerNum;
    renderingInfo.viewMask = attachmentsDesc.viewMask;
    renderingInfo.colorAttachmentCount = attachmentsDesc.colorNum;
    renderingInfo.pColorAttachments = colors;
    renderingInfo.pDepthAttachment = attachmentsDesc.depthStencil ? &depthStencil : nullptr;
    renderingInfo.pStencilAttachment = hasStencil ? &depthStencil : nullptr;

    if (attachmentsDesc.shadingRate)
        renderingInfo.pNext = &shadingRate;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBeginRendering(m_Handle, &renderingInfo);

    m_ViewMask = attachmentsDesc.viewMask;
}

NRI_INLINE void CommandBufferVK::EndRendering() {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdEndRendering(m_Handle);

    m_DepthStencil = nullptr;
}

NRI_INLINE void CommandBufferVK::SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    Scratch<VkBuffer> handles = AllocateScratch(m_Device, VkBuffer, bufferNum);
    Scratch<VkDeviceSize> fixedOffsets = AllocateScratch(m_Device, VkDeviceSize, bufferNum);
    Scratch<VkDeviceSize> sizes = AllocateScratch(m_Device, VkDeviceSize, bufferNum);
    Scratch<VkDeviceSize> strides = AllocateScratch(m_Device, VkDeviceSize, bufferNum);

    for (uint32_t i = 0; i < bufferNum; i++) {
        if (buffers[i]) {
            const BufferVK& buffer = *(BufferVK*)buffers[i];
            uint64_t offset = offsets ? offsets[i] : 0;
            handles[i] = buffer.GetHandle();
            fixedOffsets[i] = offset;
            sizes[i] = buffer.GetDesc().size - offset;
            strides[i] = m_Pipeline->GetVertexStreamStride(baseSlot + i);
        } else {
            handles[i] = VK_NULL_HANDLE;
            fixedOffsets[i] = 0;
            sizes[i] = 0;
            strides[i] = 0;
        }
    }
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBindVertexBuffers2(m_Handle, baseSlot, bufferNum, handles, fixedOffsets, sizes, strides);
}

NRI_INLINE void CommandBufferVK::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    const BufferVK& bufferVK = (const BufferVK&)buffer;

    const auto& vk = m_Device.GetDispatchTable();

    if (m_Device.m_IsSupported.maintenance5) {
        uint64_t size = bufferVK.GetDesc().size - offset;
        vk.CmdBindIndexBuffer2KHR(m_Handle, bufferVK.GetHandle(), offset, size, GetIndexType(indexType));
    } else
        vk.CmdBindIndexBuffer(m_Handle, bufferVK.GetHandle(), offset, GetIndexType(indexType));
}

NRI_INLINE void CommandBufferVK::SetPipelineLayout(const PipelineLayout& pipelineLayout) {
    const PipelineLayoutVK& pipelineLayoutVK = (const PipelineLayoutVK&)pipelineLayout;
    m_PipelineLayout = &pipelineLayoutVK;
}

NRI_INLINE void CommandBufferVK::SetPipeline(const Pipeline& pipeline) {
    const PipelineVK& pipelineImpl = (const PipelineVK&)pipeline;
    m_Pipeline = &pipelineImpl;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBindPipeline(m_Handle, pipelineImpl.GetBindPoint(), pipelineImpl);

    // In D3D12 dynamic depth bias overrides pipeline values...
    const DepthBiasDesc& depthBias = pipelineImpl.GetDepthBias();
    if (IsDepthBiasEnabled(depthBias))
        vk.CmdSetDepthBias(m_Handle, depthBias.constant, depthBias.clamp, depthBias.slope);
}

NRI_INLINE void CommandBufferVK::SetDescriptorPool(const DescriptorPool& descriptorPool) {
    MaybeUnused(descriptorPool);
}

NRI_INLINE void CommandBufferVK::SetDescriptorSet(uint32_t setIndex, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    const DescriptorSetVK& descriptorSetImpl = (DescriptorSetVK&)descriptorSet;
    VkDescriptorSet vkDescriptorSet = descriptorSetImpl.GetHandle();
    uint32_t dynamicConstantBufferNum = descriptorSetImpl.GetDynamicConstantBufferNum();

    const auto& bindingInfo = m_PipelineLayout->GetBindingInfo();
    uint32_t space = bindingInfo.descriptorSetDescs[setIndex].registerSpace;

    VkPipelineLayout pipelineLayout = *m_PipelineLayout;
    VkPipelineBindPoint pipelineBindPoint = m_PipelineLayout->GetPipelineBindPoint();

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBindDescriptorSets(m_Handle, pipelineBindPoint, pipelineLayout, space, 1, &vkDescriptorSet, dynamicConstantBufferNum, dynamicConstantBufferOffsets);
}

NRI_INLINE void CommandBufferVK::SetRootConstants(uint32_t rootConstantIndex, const void* data, uint32_t size) {
    const auto& bindingInfo = m_PipelineLayout->GetBindingInfo();
    const PushConstantBindingDesc& pushConstantBindingDesc = bindingInfo.pushConstantBindings[rootConstantIndex];

    VkPipelineLayout pipelineLayout = *m_PipelineLayout;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdPushConstants(m_Handle, pipelineLayout, pushConstantBindingDesc.stages, pushConstantBindingDesc.offset, size, data);
}

NRI_INLINE void CommandBufferVK::SetRootDescriptor(uint32_t rootDescriptorIndex, Descriptor& descriptor) {
    const DescriptorVK& descriptorVK = (DescriptorVK&)descriptor;
    DescriptorTypeVK descriptorType = descriptorVK.GetType();

    const auto& bindingInfo = m_PipelineLayout->GetBindingInfo();
    const PushDescriptorBindingDesc& pushDescriptorBindingDesc = bindingInfo.pushDescriptorBindings[rootDescriptorIndex];

    VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstSet = VK_NULL_HANDLE;
    descriptorWrite.dstBinding = pushDescriptorBindingDesc.registerIndex;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorCount = 1;

    VkDescriptorBufferInfo bufferInfo = descriptorVK.GetBufferInfo();

    // Let's match D3D12 spec (no textures, no typed buffers)
    if (descriptorType == DescriptorTypeVK::BUFFER_VIEW) {
        const DescriptorBufDesc& bufDesc = descriptorVK.GetBufDesc();
        descriptorWrite.descriptorType = bufDesc.viewType == BufferViewType::CONSTANT ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrite.pBufferInfo = &bufferInfo;
    }

    VkPipelineLayout pipelineLayout = *m_PipelineLayout;
    VkPipelineBindPoint pipelineBindPoint = m_PipelineLayout->GetPipelineBindPoint();

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdPushDescriptorSetKHR(m_Handle, pipelineBindPoint, pipelineLayout, pushDescriptorBindingDesc.registerSpace, 1, &descriptorWrite);
}

NRI_INLINE void CommandBufferVK::Draw(const DrawDesc& drawDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDraw(m_Handle, drawDesc.vertexNum, drawDesc.instanceNum, drawDesc.baseVertex, drawDesc.baseInstance);
}

NRI_INLINE void CommandBufferVK::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDrawIndexed(m_Handle, drawIndexedDesc.indexNum, drawIndexedDesc.instanceNum, drawIndexedDesc.baseIndex, drawIndexedDesc.baseVertex, drawIndexedDesc.baseInstance);
}

NRI_INLINE void CommandBufferVK::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const BufferVK& bufferVK = (const BufferVK&)buffer;
    const auto& vk = m_Device.GetDispatchTable();

    if (countBuffer) {
        const BufferVK& countBufferImpl = *(BufferVK*)countBuffer;
        vk.CmdDrawIndirectCount(m_Handle, bufferVK.GetHandle(), offset, countBufferImpl.GetHandle(), countBufferOffset, drawNum, stride);
    } else
        vk.CmdDrawIndirect(m_Handle, bufferVK.GetHandle(), offset, drawNum, stride);
}

NRI_INLINE void CommandBufferVK::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    const BufferVK& bufferVK = (const BufferVK&)buffer;
    const auto& vk = m_Device.GetDispatchTable();

    if (countBuffer) {
        const BufferVK& countBufferImpl = *(BufferVK*)countBuffer;
        vk.CmdDrawIndexedIndirectCount(m_Handle, bufferVK.GetHandle(), offset, countBufferImpl.GetHandle(), countBufferOffset, drawNum, stride);
    } else
        vk.CmdDrawIndexedIndirect(m_Handle, bufferVK.GetHandle(), offset, drawNum, stride);
}

NRI_INLINE void CommandBufferVK::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    const BufferVK& src = (const BufferVK&)srcBuffer;
    const BufferVK& dstBufferImpl = (const BufferVK&)dstBuffer;

    VkBufferCopy2 region = {VK_STRUCTURE_TYPE_BUFFER_COPY_2};
    region.srcOffset = srcOffset;
    region.dstOffset = dstOffset;
    region.size = size == WHOLE_SIZE ? src.GetDesc().size : size;

    VkCopyBufferInfo2 info = {VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2};
    info.srcBuffer = src.GetHandle();
    info.dstBuffer = dstBufferImpl.GetHandle();
    info.regionCount = 1;
    info.pRegions = &region;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyBuffer2(m_Handle, &info);
}

NRI_INLINE void CommandBufferVK::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    const TextureVK& src = (const TextureVK&)srcTexture;
    const TextureVK& dst = (const TextureVK&)dstTexture;
    const TextureDesc& dstDesc = dst.GetDesc();
    const TextureDesc& srcDesc = src.GetDesc();

    bool isWholeResource = !dstRegionDesc && !srcRegionDesc;
    uint32_t regionNum = isWholeResource ? dstDesc.mipNum : 1;
    Scratch<VkImageCopy2> regions = AllocateScratch(m_Device, VkImageCopy2, regionNum);

    if (isWholeResource) {
        for (Mip_t i = 0; i < dstDesc.mipNum; i++) {
            regions[i] = {VK_STRUCTURE_TYPE_IMAGE_COPY_2};
            regions[i].srcSubresource = {src.GetImageAspectFlags(), i, 0, srcDesc.layerNum};
            regions[i].srcOffset = {};
            regions[i].dstSubresource = {dst.GetImageAspectFlags(), i, 0, dstDesc.layerNum};
            regions[i].dstOffset = {};
            regions[i].extent = dst.GetExtent();
        }
    } else {
        TextureRegionDesc wholeResource = {};
        if (!srcRegionDesc)
            srcRegionDesc = &wholeResource;
        if (!dstRegionDesc)
            dstRegionDesc = &wholeResource;

        regions[0] = {VK_STRUCTURE_TYPE_IMAGE_COPY_2};
        regions[0].srcSubresource = {
            src.GetImageAspectFlags(),
            srcRegionDesc->mipOffset,
            srcRegionDesc->layerOffset,
            1,
        };
        regions[0].srcOffset = {
            (int32_t)srcRegionDesc->x,
            (int32_t)srcRegionDesc->y,
            (int32_t)srcRegionDesc->z,
        };
        regions[0].dstSubresource = {
            dst.GetImageAspectFlags(),
            dstRegionDesc->mipOffset,
            dstRegionDesc->layerOffset,
            1,
        };
        regions[0].dstOffset = {
            (int32_t)dstRegionDesc->x,
            (int32_t)dstRegionDesc->y,
            (int32_t)dstRegionDesc->z,
        };
        regions[0].extent = {
            (srcRegionDesc->width == WHOLE_SIZE) ? src.GetSize(0, srcRegionDesc->mipOffset) : srcRegionDesc->width,
            (srcRegionDesc->height == WHOLE_SIZE) ? src.GetSize(1, srcRegionDesc->mipOffset) : srcRegionDesc->height,
            (srcRegionDesc->depth == WHOLE_SIZE) ? src.GetSize(2, srcRegionDesc->mipOffset) : srcRegionDesc->depth,
        };
    }

    VkCopyImageInfo2 info = {VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2};
    info.srcImage = src.GetHandle();
    info.srcImageLayout = IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    info.dstImage = dst.GetHandle();
    info.dstImageLayout = IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    info.regionCount = regionNum;
    info.pRegions = regions;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyImage2(m_Handle, &info);
}

NRI_INLINE void CommandBufferVK::ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    const TextureVK& src = (const TextureVK&)srcTexture;
    const TextureVK& dst = (const TextureVK&)dstTexture;
    const TextureDesc& dstDesc = dst.GetDesc();
    const TextureDesc& srcDesc = src.GetDesc();

    bool isWholeResource = !dstRegionDesc && !srcRegionDesc;
    uint32_t regionNum = isWholeResource ? dstDesc.mipNum : 1;
    Scratch<VkImageResolve2> regions = AllocateScratch(m_Device, VkImageResolve2, dstDesc.mipNum);

    if (isWholeResource) {
        for (Mip_t i = 0; i < dstDesc.mipNum; i++) {
            regions[i] = {VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2};
            regions[i].srcSubresource = {src.GetImageAspectFlags(), i, 0, srcDesc.layerNum};
            regions[i].srcOffset = {};
            regions[i].dstSubresource = {dst.GetImageAspectFlags(), i, 0, dstDesc.layerNum};
            regions[i].dstOffset = {};
            regions[i].extent = dst.GetExtent();
        }
    } else {
        TextureRegionDesc wholeResource = {};
        if (!srcRegionDesc)
            srcRegionDesc = &wholeResource;
        if (!dstRegionDesc)
            dstRegionDesc = &wholeResource;

        regions[0] = {VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2};
        regions[0].srcSubresource = {
            src.GetImageAspectFlags(),
            srcRegionDesc->mipOffset,
            srcRegionDesc->layerOffset,
            1,
        };
        regions[0].srcOffset = {
            (int32_t)srcRegionDesc->x,
            (int32_t)srcRegionDesc->y,
            (int32_t)srcRegionDesc->z,
        };
        regions[0].dstSubresource = {
            dst.GetImageAspectFlags(),
            dstRegionDesc->mipOffset,
            dstRegionDesc->layerOffset,
            1,
        };
        regions[0].dstOffset = {
            (int32_t)dstRegionDesc->x,
            (int32_t)dstRegionDesc->y,
            (int32_t)dstRegionDesc->z,
        };
        regions[0].extent = {
            (srcRegionDesc->width == WHOLE_SIZE) ? src.GetSize(0, srcRegionDesc->mipOffset) : srcRegionDesc->width,
            (srcRegionDesc->height == WHOLE_SIZE) ? src.GetSize(1, srcRegionDesc->mipOffset) : srcRegionDesc->height,
            (srcRegionDesc->depth == WHOLE_SIZE) ? src.GetSize(2, srcRegionDesc->mipOffset) : srcRegionDesc->depth,
        };
    }

    VkResolveImageInfo2 info = {VK_STRUCTURE_TYPE_RESOLVE_IMAGE_INFO_2};
    info.srcImage = src.GetHandle();
    info.srcImageLayout = IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    info.dstImage = dst.GetHandle();
    info.dstImageLayout = IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    info.regionCount = regionNum;
    info.pRegions = regions;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdResolveImage2(m_Handle, &info);
}

NRI_INLINE void CommandBufferVK::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    const BufferVK& src = (const BufferVK&)srcBuffer;
    const TextureVK& dst = (const TextureVK&)dstTexture;
    const FormatProps& formatProps = GetFormatProps(dst.GetDesc().format);

    uint32_t rowBlockNum = srcDataLayoutDesc.rowPitch / formatProps.stride;
    uint32_t bufferRowLength = rowBlockNum * formatProps.blockWidth;

    uint32_t sliceRowNum = srcDataLayoutDesc.slicePitch / srcDataLayoutDesc.rowPitch;
    uint32_t bufferImageHeight = sliceRowNum * formatProps.blockWidth;

    VkBufferImageCopy2 region = {VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2};
    region.bufferOffset = srcDataLayoutDesc.offset;
    region.bufferRowLength = bufferRowLength;
    region.bufferImageHeight = bufferImageHeight;
    region.imageSubresource = VkImageSubresourceLayers{
        dst.GetImageAspectFlags(),
        dstRegionDesc.mipOffset,
        dstRegionDesc.layerOffset,
        1,
    };
    region.imageOffset = VkOffset3D{
        dstRegionDesc.x,
        dstRegionDesc.y,
        dstRegionDesc.z,
    };
    region.imageExtent = VkExtent3D{
        (dstRegionDesc.width == WHOLE_SIZE) ? dst.GetSize(0, dstRegionDesc.mipOffset) : dstRegionDesc.width,
        (dstRegionDesc.height == WHOLE_SIZE) ? dst.GetSize(1, dstRegionDesc.mipOffset) : dstRegionDesc.height,
        (dstRegionDesc.depth == WHOLE_SIZE) ? dst.GetSize(2, dstRegionDesc.mipOffset) : dstRegionDesc.depth,
    };

    VkCopyBufferToImageInfo2 info = {VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2};
    info.srcBuffer = src.GetHandle();
    info.dstImage = dst.GetHandle();
    info.dstImageLayout = IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    info.regionCount = 1;
    info.pRegions = &region;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyBufferToImage2(m_Handle, &info);
}

NRI_INLINE void CommandBufferVK::ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    const TextureVK& src = (const TextureVK&)srcTexture;
    const BufferVK& dst = (const BufferVK&)dstBuffer;
    const FormatProps& formatProps = GetFormatProps(src.GetDesc().format);

    uint32_t rowBlockNum = dstDataLayoutDesc.rowPitch / formatProps.stride;
    uint32_t bufferRowLength = rowBlockNum * formatProps.blockWidth;

    uint32_t sliceRowNum = dstDataLayoutDesc.slicePitch / dstDataLayoutDesc.rowPitch;
    uint32_t bufferImageHeight = sliceRowNum * formatProps.blockWidth;

    VkBufferImageCopy2 region = {VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2};
    region.bufferOffset = dstDataLayoutDesc.offset;
    region.bufferRowLength = bufferRowLength;
    region.bufferImageHeight = bufferImageHeight;
    region.imageSubresource = VkImageSubresourceLayers{
        src.GetImageAspectFlags(),
        srcRegionDesc.mipOffset,
        srcRegionDesc.layerOffset,
        1,
    };
    region.imageOffset = VkOffset3D{
        srcRegionDesc.x,
        srcRegionDesc.y,
        srcRegionDesc.z,
    };
    region.imageExtent = VkExtent3D{
        (srcRegionDesc.width == WHOLE_SIZE) ? src.GetSize(0, srcRegionDesc.mipOffset) : srcRegionDesc.width,
        (srcRegionDesc.height == WHOLE_SIZE) ? src.GetSize(1, srcRegionDesc.mipOffset) : srcRegionDesc.height,
        (srcRegionDesc.depth == WHOLE_SIZE) ? src.GetSize(2, srcRegionDesc.mipOffset) : srcRegionDesc.depth,
    };

    VkCopyImageToBufferInfo2 info = {VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2};
    info.srcImage = src.GetHandle();
    info.srcImageLayout = IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    info.dstBuffer = dst.GetHandle();
    info.regionCount = 1;
    info.pRegions = &region;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyImageToBuffer2(m_Handle, &info);
}

NRI_INLINE void CommandBufferVK::ZeroBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    BufferVK& dst = (BufferVK&)buffer;

    if (size == WHOLE_SIZE)
        size = dst.GetDesc().size;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdFillBuffer(m_Handle, dst.GetHandle(), offset, size, 0);
}

NRI_INLINE void CommandBufferVK::Dispatch(const DispatchDesc& dispatchDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDispatch(m_Handle, dispatchDesc.x, dispatchDesc.y, dispatchDesc.z);
}

NRI_INLINE void CommandBufferVK::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    static_assert(sizeof(DispatchDesc) == sizeof(VkDispatchIndirectCommand));

    const BufferVK& bufferVK = (const BufferVK&)buffer;
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDispatchIndirect(m_Handle, bufferVK.GetHandle(), offset);
}

static inline VkAccessFlags2 GetAccessFlags(AccessBits accessBits) {
    VkAccessFlags2 flags = 0;

    if (accessBits & AccessBits::INDEX_BUFFER)
        flags |= VK_ACCESS_2_INDEX_READ_BIT;

    if (accessBits & AccessBits::VERTEX_BUFFER)
        flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;

    if (accessBits & AccessBits::CONSTANT_BUFFER)
        flags |= VK_ACCESS_2_UNIFORM_READ_BIT;

    if (accessBits & AccessBits::ARGUMENT_BUFFER)
        flags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;

    if (accessBits & AccessBits::SCRATCH_BUFFER)
        flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

    if (accessBits & AccessBits::COLOR_ATTACHMENT)
        flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

    if (accessBits & AccessBits::SHADING_RATE_ATTACHMENT)
        flags |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;

    if (accessBits & AccessBits::DEPTH_STENCIL_ATTACHMENT_READ)
        flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    if (accessBits & AccessBits::DEPTH_STENCIL_ATTACHMENT_WRITE)
        flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    if (accessBits & AccessBits::ACCELERATION_STRUCTURE_READ)
        flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;

    if (accessBits & AccessBits::ACCELERATION_STRUCTURE_WRITE)
        flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

    if (accessBits & AccessBits::MICROMAP_READ)
        flags |= VK_ACCESS_2_MICROMAP_READ_BIT_EXT;

    if (accessBits & AccessBits::MICROMAP_WRITE)
        flags |= VK_ACCESS_2_MICROMAP_WRITE_BIT_EXT;

    if (accessBits & AccessBits::SHADER_RESOURCE)
        flags |= VK_ACCESS_2_SHADER_READ_BIT;

    if (accessBits & AccessBits::SHADER_RESOURCE_STORAGE)
        flags |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;

    if (accessBits & (AccessBits::COPY_SOURCE | AccessBits::RESOLVE_SOURCE))
        flags |= VK_ACCESS_2_TRANSFER_READ_BIT;

    if (accessBits & (AccessBits::COPY_DESTINATION | AccessBits::RESOLVE_DESTINATION))
        flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;

    return flags;
}

NRI_INLINE void CommandBufferVK::Barrier(const BarrierGroupDesc& barrierGroupDesc) {
    // Global
    Scratch<VkMemoryBarrier2> memoryBarriers = AllocateScratch(m_Device, VkMemoryBarrier2, barrierGroupDesc.globalNum);
    for (uint32_t i = 0; i < barrierGroupDesc.globalNum; i++) {
        const GlobalBarrierDesc& in = barrierGroupDesc.globals[i];

        VkMemoryBarrier2& out = memoryBarriers[i];
        out = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
        out.srcStageMask = GetPipelineStageFlags(in.before.stages);
        out.srcAccessMask = GetAccessFlags(in.before.access);
        out.dstStageMask = GetPipelineStageFlags(in.after.stages);
        out.dstAccessMask = GetAccessFlags(in.after.access);
    }

    // Buffer
    Scratch<VkBufferMemoryBarrier2> bufferBarriers = AllocateScratch(m_Device, VkBufferMemoryBarrier2, barrierGroupDesc.bufferNum);
    for (uint32_t i = 0; i < barrierGroupDesc.bufferNum; i++) {
        const BufferBarrierDesc& in = barrierGroupDesc.buffers[i];
        const BufferVK& bufferVK = *(const BufferVK*)in.buffer;

        VkBufferMemoryBarrier2& out = bufferBarriers[i];
        out = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
        out.srcStageMask = GetPipelineStageFlags(in.before.stages);
        out.srcAccessMask = GetAccessFlags(in.before.access);
        out.dstStageMask = GetPipelineStageFlags(in.after.stages);
        out.dstAccessMask = GetAccessFlags(in.after.access);
        out.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // TODO: VK_SHARING_MODE_EXCLUSIVE could be used instead of VK_SHARING_MODE_CONCURRENT with queue ownership transfers
        out.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        out.buffer = bufferVK.GetHandle();
        out.offset = 0;
        out.size = VK_WHOLE_SIZE;
    }

    // Texture
    Scratch<VkImageMemoryBarrier2> textureBarriers = AllocateScratch(m_Device, VkImageMemoryBarrier2, barrierGroupDesc.textureNum);
    for (uint32_t i = 0; i < barrierGroupDesc.textureNum; i++) {
        const TextureBarrierDesc& in = barrierGroupDesc.textures[i];
        const TextureVK& textureImpl = *(const TextureVK*)in.texture;

        VkImageAspectFlags aspectFlags = 0;
        if (in.planes == PlaneBits::ALL)
            aspectFlags = textureImpl.GetImageAspectFlags();
        else {
            if (in.planes & PlaneBits::COLOR)
                aspectFlags |= VK_IMAGE_ASPECT_COLOR_BIT;
            if (in.planes & PlaneBits::DEPTH)
                aspectFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;
            if (in.planes & PlaneBits::STENCIL)
                aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        VkImageMemoryBarrier2& out = textureBarriers[i];
        out = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        out.srcStageMask = GetPipelineStageFlags(in.before.stages);
        out.srcAccessMask = in.before.layout == Layout::PRESENT ? VK_ACCESS_2_MEMORY_READ_BIT : GetAccessFlags(in.before.access);
        out.dstStageMask = GetPipelineStageFlags(in.after.stages);
        out.dstAccessMask = in.after.layout == Layout::PRESENT ? VK_ACCESS_2_MEMORY_READ_BIT : GetAccessFlags(in.after.access);
        out.oldLayout = GetImageLayout(in.before.layout);
        out.newLayout = GetImageLayout(in.after.layout);
        out.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // TODO: VK_SHARING_MODE_EXCLUSIVE could be used instead of VK_SHARING_MODE_CONCURRENT with queue ownership transfers
        out.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        out.image = textureImpl.GetHandle();
        out.subresourceRange = {
            aspectFlags,
            in.mipOffset,
            (in.mipNum == REMAINING_MIPS) ? VK_REMAINING_MIP_LEVELS : in.mipNum,
            in.layerOffset,
            (in.layerNum == REMAINING_LAYERS) ? VK_REMAINING_ARRAY_LAYERS : in.layerNum,
        };
    }

    // Submit
    VkDependencyInfo dependencyInfo = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependencyInfo.memoryBarrierCount = barrierGroupDesc.globalNum;
    dependencyInfo.pMemoryBarriers = memoryBarriers;
    dependencyInfo.bufferMemoryBarrierCount = barrierGroupDesc.bufferNum;
    dependencyInfo.pBufferMemoryBarriers = bufferBarriers;
    dependencyInfo.imageMemoryBarrierCount = barrierGroupDesc.textureNum;
    dependencyInfo.pImageMemoryBarriers = textureBarriers;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdPipelineBarrier2(m_Handle, &dependencyInfo);
}

NRI_INLINE void CommandBufferVK::BeginQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolVK& queryPoolImpl = (QueryPoolVK&)queryPool;
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBeginQuery(m_Handle, queryPoolImpl.GetHandle(), offset, (VkQueryControlFlagBits)0);
}

NRI_INLINE void CommandBufferVK::EndQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolVK& queryPoolImpl = (QueryPoolVK&)queryPool;
    const auto& vk = m_Device.GetDispatchTable();

    if (queryPoolImpl.GetType() == VK_QUERY_TYPE_TIMESTAMP)
        vk.CmdWriteTimestamp2(m_Handle, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, queryPoolImpl.GetHandle(), offset);
    else
        vk.CmdEndQuery(m_Handle, queryPoolImpl.GetHandle(), offset);
}

NRI_INLINE void CommandBufferVK::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    const QueryPoolVK& queryPoolImpl = (const QueryPoolVK&)queryPool;
    const BufferVK& bufferVK = (const BufferVK&)dstBuffer;

    // TODO: wait is questionable here, but it's needed to ensure that CopyQueries copies to the destination buffer "complete" values (perf seems unaffected)
    VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyQueryPoolResults(m_Handle, queryPoolImpl.GetHandle(), offset, num, bufferVK.GetHandle(), dstOffset, queryPoolImpl.GetQuerySize(), flags);
}

NRI_INLINE void CommandBufferVK::ResetQueries(QueryPool& queryPool, uint32_t offset, uint32_t num) {
    QueryPoolVK& queryPoolImpl = (QueryPoolVK&)queryPool;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdResetQueryPool(m_Handle, queryPoolImpl.GetHandle(), offset, num);
}

NRI_INLINE void CommandBufferVK::BeginAnnotation(const char* name, uint32_t bgra) {
    VkDebugUtilsLabelEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    info.pLabelName = name;
    info.color[0] = ((bgra >> 16) & 0xFF) / 255.0f;
    info.color[1] = ((bgra >> 8) & 0xFF) / 255.0f;
    info.color[2] = ((bgra >> 0) & 0xFF) / 255.0f;
    info.color[3] = 1.0f; // PIX sets alpha to 1

    const auto& vk = m_Device.GetDispatchTable();
    if (vk.CmdBeginDebugUtilsLabelEXT)
        vk.CmdBeginDebugUtilsLabelEXT(m_Handle, &info);
}

NRI_INLINE void CommandBufferVK::EndAnnotation() {
    const auto& vk = m_Device.GetDispatchTable();
    if (vk.CmdEndDebugUtilsLabelEXT)
        vk.CmdEndDebugUtilsLabelEXT(m_Handle);
}

NRI_INLINE void CommandBufferVK::Annotation(const char* name, uint32_t bgra) {
    VkDebugUtilsLabelEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    info.pLabelName = name;
    info.color[0] = ((bgra >> 16) & 0xFF) / 255.0f;
    info.color[1] = ((bgra >> 8) & 0xFF) / 255.0f;
    info.color[2] = ((bgra >> 0) & 0xFF) / 255.0f;
    info.color[3] = 1.0f; // PIX sets alpha to 1

    const auto& vk = m_Device.GetDispatchTable();
    if (vk.CmdInsertDebugUtilsLabelEXT)
        vk.CmdInsertDebugUtilsLabelEXT(m_Handle, &info);
}

NRI_INLINE void CommandBufferVK::BuildTopLevelAccelerationStructure(const BuildTopLevelAccelerationStructureDesc* buildTopLevelAccelerationStructureDescs, uint32_t buildTopLevelAccelerationStructureDescNum) {
    static_assert(sizeof(VkAccelerationStructureInstanceKHR) == sizeof(TopLevelInstance), "Mismatched sizeof");

    Scratch<VkAccelerationStructureBuildGeometryInfoKHR> infos = AllocateScratch(m_Device, VkAccelerationStructureBuildGeometryInfoKHR, buildTopLevelAccelerationStructureDescNum);
    Scratch<const VkAccelerationStructureBuildRangeInfoKHR*> pRanges = AllocateScratch(m_Device, const VkAccelerationStructureBuildRangeInfoKHR*, buildTopLevelAccelerationStructureDescNum);
    Scratch<VkAccelerationStructureGeometryKHR> geometries = AllocateScratch(m_Device, VkAccelerationStructureGeometryKHR, buildTopLevelAccelerationStructureDescNum);
    Scratch<VkAccelerationStructureBuildRangeInfoKHR> ranges = AllocateScratch(m_Device, VkAccelerationStructureBuildRangeInfoKHR, buildTopLevelAccelerationStructureDescNum);

    for (uint32_t i = 0; i < buildTopLevelAccelerationStructureDescNum; i++) {
        const BuildTopLevelAccelerationStructureDesc& in = buildTopLevelAccelerationStructureDescs[i];

        AccelerationStructureVK* dst = (AccelerationStructureVK*)in.dst;
        AccelerationStructureVK* src = (AccelerationStructureVK*)in.src;
        BufferVK* scratchBuffer = (BufferVK*)in.scratchBuffer;
        BufferVK* instanceBuffer = (BufferVK*)in.instanceBuffer;

        // Range
        VkAccelerationStructureBuildRangeInfoKHR& range = ranges[i];
        range = {};
        range.primitiveCount = in.instanceNum;

        pRanges[i] = &ranges[i];

        // Geometry
        VkAccelerationStructureGeometryKHR& geometry = geometries[i];
        geometry = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometry.geometry.instances.data.deviceAddress = instanceBuffer->GetDeviceAddress() + in.instanceOffset;

        // Info
        VkAccelerationStructureBuildGeometryInfoKHR& info = infos[i];
        info = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        info.flags = GetBuildAccelerationStructureFlags(dst->GetFlags());
        info.dstAccelerationStructure = dst->GetHandle();
        info.geometryCount = 1;
        info.pGeometries = &geometry;
        info.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress() + in.scratchOffset;

        if (in.src) {
            info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
            info.srcAccelerationStructure = src->GetHandle();
        } else
            info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBuildAccelerationStructuresKHR(m_Handle, buildTopLevelAccelerationStructureDescNum, infos, pRanges);
}

NRI_INLINE void CommandBufferVK::BuildBottomLevelAccelerationStructure(const BuildBottomLevelAccelerationStructureDesc* buildBottomLevelAccelerationStructureDescs, uint32_t buildBottomLevelAccelerationStructureDescNum) {
    uint32_t totalGeometryObjectNum = 0;
    for (uint32_t i = 0; i < buildBottomLevelAccelerationStructureDescNum; i++)
        totalGeometryObjectNum += buildBottomLevelAccelerationStructureDescs[i].geometryNum;

    Scratch<VkAccelerationStructureBuildGeometryInfoKHR> infos = AllocateScratch(m_Device, VkAccelerationStructureBuildGeometryInfoKHR, buildBottomLevelAccelerationStructureDescNum);
    Scratch<const VkAccelerationStructureBuildRangeInfoKHR*> pRanges = AllocateScratch(m_Device, const VkAccelerationStructureBuildRangeInfoKHR*, buildBottomLevelAccelerationStructureDescNum);
    Scratch<VkAccelerationStructureGeometryKHR> geometriesScratch = AllocateScratch(m_Device, VkAccelerationStructureGeometryKHR, totalGeometryObjectNum);
    Scratch<VkAccelerationStructureBuildRangeInfoKHR> rangesScratch = AllocateScratch(m_Device, VkAccelerationStructureBuildRangeInfoKHR, totalGeometryObjectNum);

    VkAccelerationStructureGeometryKHR* geometries = geometriesScratch;
    VkAccelerationStructureBuildRangeInfoKHR* ranges = rangesScratch;

    uint32_t micromapNum = 0;
    for (uint32_t i = 0; i < buildBottomLevelAccelerationStructureDescNum; i++) {
        const BuildBottomLevelAccelerationStructureDesc& in = buildBottomLevelAccelerationStructureDescs[i];

        // Ranges and geometries
        micromapNum += ConvertBotomLevelGeometries(geometries, ranges, in.geometries, in.geometryNum);

        pRanges[i] = ranges;

        // Info
        AccelerationStructureVK* dst = (AccelerationStructureVK*)in.dst;
        AccelerationStructureVK* src = (AccelerationStructureVK*)in.src;

        BufferVK* scratchBuffer = (BufferVK*)in.scratchBuffer;

        VkAccelerationStructureBuildGeometryInfoKHR& info = infos[i];
        info = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        info.flags = GetBuildAccelerationStructureFlags(dst->GetFlags());
        info.dstAccelerationStructure = dst->GetHandle();
        info.geometryCount = in.geometryNum;
        info.pGeometries = geometries;
        info.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress() + in.scratchOffset;

        if (in.src) {
            info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
            info.srcAccelerationStructure = src->GetHandle();
        } else
            info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

        geometries += in.geometryNum;
        ranges += in.geometryNum;
    }

    // Add micromaps
    Scratch<VkAccelerationStructureTrianglesOpacityMicromapEXT> trianglesMicromapsScratch = AllocateScratch(m_Device, VkAccelerationStructureTrianglesOpacityMicromapEXT, micromapNum);
    VkAccelerationStructureTrianglesOpacityMicromapEXT* trianglesMicromaps = trianglesMicromapsScratch;

    if (micromapNum) {
        for (uint32_t i = 0; i < buildBottomLevelAccelerationStructureDescNum; i++) {
            const BuildBottomLevelAccelerationStructureDesc& in = buildBottomLevelAccelerationStructureDescs[i];

            VkAccelerationStructureGeometryKHR* pGeometry = (VkAccelerationStructureGeometryKHR*)infos[i].pGeometries;

            for (uint32_t j = 0; j < in.geometryNum; j++) {
                const BottomLevelGeometryDesc& geometry = in.geometries[j];

                const BottomLevelMicromapDesc* botomLevelMicromap = geometry.geometry.triangles.micromap;
                if (geometry.type == BottomLevelGeometryType::TRIANGLES && botomLevelMicromap) {
                    pGeometry[j].geometry.triangles.pNext = trianglesMicromaps;

                    BufferVK* indexBuffer = (BufferVK*)botomLevelMicromap->indexBuffer;
                    MicromapVK* micromap = (MicromapVK*)botomLevelMicromap->micromap;

                    VkAccelerationStructureTrianglesOpacityMicromapEXT& trianglesMicromap = *trianglesMicromaps;
                    trianglesMicromap = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_TRIANGLES_OPACITY_MICROMAP_EXT};
                    trianglesMicromap.usageCountsCount = micromap->GetUsageNum();
                    trianglesMicromap.pUsageCounts = micromap->GetUsages();
                    trianglesMicromap.micromap = micromap->GetHandle();

                    if (indexBuffer) {
                        trianglesMicromap.indexType = GetIndexType(botomLevelMicromap->indexType);
                        trianglesMicromap.indexBuffer.deviceAddress = indexBuffer->GetDeviceAddress() + botomLevelMicromap->indexOffset;
                        trianglesMicromap.indexStride = botomLevelMicromap->indexType == IndexType::UINT32 ? sizeof(uint32_t) : sizeof(uint16_t);
                        trianglesMicromap.baseTriangle = botomLevelMicromap->baseTriangle;
                    } else
                        trianglesMicromap.indexType = VK_INDEX_TYPE_NONE_KHR;

                    trianglesMicromaps++;
                }
            }
        }
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBuildAccelerationStructuresKHR(m_Handle, buildBottomLevelAccelerationStructureDescNum, infos, pRanges);
}

NRI_INLINE void CommandBufferVK::BuildMicromaps(const BuildMicromapDesc* buildMicromapDescs, uint32_t buildMicromapDescNum) {
    static_assert(sizeof(MicromapTriangle) == sizeof(VkMicromapTriangleEXT), "Mismatched sizeof");

    Scratch<VkMicromapBuildInfoEXT> infos = AllocateScratch(m_Device, VkMicromapBuildInfoEXT, buildMicromapDescNum);
    for (uint32_t i = 0; i < buildMicromapDescNum; i++) {
        const BuildMicromapDesc& in = buildMicromapDescs[i];

        MicromapVK* dst = (MicromapVK*)in.dst;
        BufferVK* scratchBuffer = (BufferVK*)in.scratchBuffer;
        BufferVK* triangleBuffer = (BufferVK*)in.triangleBuffer;
        BufferVK* dataBuffer = (BufferVK*)in.dataBuffer;

        VkMicromapBuildInfoEXT& out = infos[i];
        out = {VK_STRUCTURE_TYPE_MICROMAP_BUILD_INFO_EXT};
        out.type = VK_MICROMAP_TYPE_OPACITY_MICROMAP_EXT;
        out.flags = GetBuildMicromapFlags(dst->GetFlags());
        out.mode = VK_BUILD_MICROMAP_MODE_BUILD_EXT;
        out.dstMicromap = dst->GetHandle();
        out.usageCountsCount = dst->GetUsageNum();
        out.pUsageCounts = dst->GetUsages();
        out.data.deviceAddress = dataBuffer->GetDeviceAddress() + in.dataOffset;
        out.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress() + in.scratchOffset;
        out.triangleArray.deviceAddress = triangleBuffer->GetDeviceAddress() + in.triangleOffset;
        out.triangleArrayStride = sizeof(MicromapTriangle);
    }

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBuildMicromapsEXT(m_Handle, buildMicromapDescNum, infos);
}

NRI_INLINE void CommandBufferVK::CopyAccelerationStructure(AccelerationStructure& dst, const AccelerationStructure& src, CopyMode copyMode) {
    VkAccelerationStructureKHR dstHandle = ((AccelerationStructureVK&)dst).GetHandle();
    VkAccelerationStructureKHR srcHandle = ((AccelerationStructureVK&)src).GetHandle();

    VkCopyAccelerationStructureInfoKHR info = {VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
    info.src = srcHandle;
    info.dst = dstHandle;
    info.mode = GetAccelerationStructureCopyMode(copyMode);

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyAccelerationStructureKHR(m_Handle, &info);
}

NRI_INLINE void CommandBufferVK::CopyMicromap(Micromap& dst, const Micromap& src, CopyMode copyMode) {
    VkMicromapEXT dstHandle = ((MicromapVK&)dst).GetHandle();
    VkMicromapEXT srcHandle = ((MicromapVK&)src).GetHandle();

    VkCopyMicromapInfoEXT info = {VK_STRUCTURE_TYPE_COPY_MICROMAP_INFO_EXT};
    info.src = srcHandle;
    info.dst = dstHandle;
    info.mode = GetMicromapCopyMode(copyMode);

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyMicromapEXT(m_Handle, &info);
}

NRI_INLINE void CommandBufferVK::WriteAccelerationStructuresSizes(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    Scratch<VkAccelerationStructureKHR> handles = AllocateScratch(m_Device, VkAccelerationStructureKHR, accelerationStructureNum);
    for (uint32_t i = 0; i < accelerationStructureNum; i++)
        handles[i] = ((AccelerationStructureVK*)accelerationStructures[i])->GetHandle();

    const QueryPoolVK& queryPoolVK = (QueryPoolVK&)queryPool;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdWriteAccelerationStructuresPropertiesKHR(m_Handle, accelerationStructureNum, handles, queryPoolVK.GetType(), queryPoolVK.GetHandle(), queryPoolOffset);
}

NRI_INLINE void CommandBufferVK::WriteMicromapsSizes(const Micromap* const* micromaps, uint32_t micromapNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    Scratch<VkMicromapEXT> handles = AllocateScratch(m_Device, VkMicromapEXT, micromapNum);
    for (uint32_t i = 0; i < micromapNum; i++)
        handles[i] = ((MicromapVK*)micromaps[i])->GetHandle();

    const QueryPoolVK& queryPoolVK = (QueryPoolVK&)queryPool;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdWriteMicromapsPropertiesEXT(m_Handle, micromapNum, handles, queryPoolVK.GetType(), queryPoolVK.GetHandle(), queryPoolOffset);
}

NRI_INLINE void CommandBufferVK::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc) {
    VkStridedDeviceAddressRegionKHR raygen = {};
    raygen.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.raygenShader.buffer) + dispatchRaysDesc.raygenShader.offset;
    raygen.size = dispatchRaysDesc.raygenShader.size;
    raygen.stride = dispatchRaysDesc.raygenShader.stride;

    VkStridedDeviceAddressRegionKHR miss = {};
    miss.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.missShaders.buffer) + dispatchRaysDesc.missShaders.offset;
    miss.size = dispatchRaysDesc.missShaders.size;
    miss.stride = dispatchRaysDesc.missShaders.stride;

    VkStridedDeviceAddressRegionKHR hit = {};
    hit.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.hitShaderGroups.buffer) + dispatchRaysDesc.hitShaderGroups.offset;
    hit.size = dispatchRaysDesc.hitShaderGroups.size;
    hit.stride = dispatchRaysDesc.hitShaderGroups.stride;

    VkStridedDeviceAddressRegionKHR callable = {};
    callable.deviceAddress = GetBufferDeviceAddress(dispatchRaysDesc.callableShaders.buffer) + dispatchRaysDesc.callableShaders.offset;
    callable.size = dispatchRaysDesc.callableShaders.size;
    callable.stride = dispatchRaysDesc.callableShaders.stride;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdTraceRaysKHR(m_Handle, &raygen, &miss, &hit, &callable, dispatchRaysDesc.x, dispatchRaysDesc.y, dispatchRaysDesc.z);
}

NRI_INLINE void CommandBufferVK::DispatchRaysIndirect(const Buffer& buffer, uint64_t offset) {
    static_assert(sizeof(DispatchRaysIndirectDesc) == sizeof(VkTraceRaysIndirectCommand2KHR));

    uint64_t deviceAddress = GetBufferDeviceAddress(&buffer) + offset;

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdTraceRaysIndirect2KHR(m_Handle, deviceAddress);
}

NRI_INLINE void CommandBufferVK::DrawMeshTasks(const DrawMeshTasksDesc& drawMeshTasksDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdDrawMeshTasksEXT(m_Handle, drawMeshTasksDesc.x, drawMeshTasksDesc.y, drawMeshTasksDesc.z);
}

NRI_INLINE void CommandBufferVK::DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    static_assert(sizeof(DrawMeshTasksDesc) == sizeof(VkDrawMeshTasksIndirectCommandEXT));

    const BufferVK& bufferVK = (const BufferVK&)buffer;
    const auto& vk = m_Device.GetDispatchTable();

    if (countBuffer) {
        const BufferVK& countBufferImpl = *(BufferVK*)countBuffer;
        vk.CmdDrawMeshTasksIndirectCountEXT(m_Handle, bufferVK.GetHandle(), offset, countBufferImpl.GetHandle(), countBufferOffset, drawNum, stride);
    } else
        vk.CmdDrawMeshTasksIndirectEXT(m_Handle, bufferVK.GetHandle(), offset, drawNum, stride);
}
