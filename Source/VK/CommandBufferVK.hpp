// © 2021 NVIDIA Corporation

#include <math.h>

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

NRI_INLINE void CommandBufferVK::ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc) {
    const DescriptorVK& descriptor = *(const DescriptorVK*)clearDesc.storageBuffer;
    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdFillBuffer(m_Handle, descriptor.GetBuffer(), 0, VK_WHOLE_SIZE, clearDesc.value);
}

NRI_INLINE void CommandBufferVK::ClearStorageTexture(const ClearStorageTextureDesc& clearDesc) {
    const DescriptorVK& descriptor = *(const DescriptorVK*)clearDesc.storageTexture;
    const VkClearColorValue* value = (const VkClearColorValue*)&clearDesc.value;

    VkImageSubresourceRange range = descriptor.GetImageSubresourceRange();

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdClearColorImage(m_Handle, descriptor.GetImage(), VK_IMAGE_LAYOUT_GENERAL, value, 1, &range);
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

    if (accessBits & AccessBits::VERTEX_BUFFER)
        flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;

    if (accessBits & AccessBits::INDEX_BUFFER)
        flags |= VK_ACCESS_2_INDEX_READ_BIT;

    if (accessBits & AccessBits::CONSTANT_BUFFER)
        flags |= VK_ACCESS_2_UNIFORM_READ_BIT;

    if (accessBits & AccessBits::ARGUMENT_BUFFER)
        flags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;

    if (accessBits & AccessBits::SHADER_RESOURCE)
        flags |= VK_ACCESS_2_SHADER_READ_BIT;

    if (accessBits & AccessBits::SHADER_RESOURCE_STORAGE)
        flags |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;

    if (accessBits & AccessBits::COLOR_ATTACHMENT)
        flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

    if (accessBits & AccessBits::DEPTH_STENCIL_ATTACHMENT_WRITE)
        flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    if (accessBits & AccessBits::DEPTH_STENCIL_ATTACHMENT_READ)
        flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    if (accessBits & (AccessBits::COPY_SOURCE | AccessBits::RESOLVE_SOURCE))
        flags |= VK_ACCESS_2_TRANSFER_READ_BIT;

    if (accessBits & (AccessBits::COPY_DESTINATION | AccessBits::RESOLVE_DESTINATION))
        flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;

    if (accessBits & AccessBits::ACCELERATION_STRUCTURE_READ)
        flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;

    if (accessBits & AccessBits::ACCELERATION_STRUCTURE_WRITE)
        flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

    if (accessBits & AccessBits::SHADING_RATE_ATTACHMENT)
        flags |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;

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

NRI_INLINE void CommandBufferVK::BuildTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {
    static_assert(sizeof(VkAccelerationStructureInstanceKHR) == sizeof(GeometryObjectInstance), "Mismatched sizeof");

    const VkAccelerationStructureKHR dstASHandle = ((const AccelerationStructureVK&)dst).GetHandle();
    const VkDeviceAddress scratchAddress = ((BufferVK&)scratch).GetDeviceAddress() + scratchOffset;
    const VkDeviceAddress bufferAddress = ((BufferVK&)buffer).GetDeviceAddress() + bufferOffset;

    VkAccelerationStructureGeometryKHR geometry = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.data.deviceAddress = bufferAddress;

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildGeometryInfo.flags = GetAccelerationStructureBuildFlags(flags);
    buildGeometryInfo.dstAccelerationStructure = dstASHandle;
    buildGeometryInfo.geometryCount = 1;
    buildGeometryInfo.pGeometries = &geometry;
    buildGeometryInfo.scratchData.deviceAddress = scratchAddress;

    VkAccelerationStructureBuildRangeInfoKHR range = {};
    range.primitiveCount = instanceNum;

    const VkAccelerationStructureBuildRangeInfoKHR* rangeArrays[1] = {&range};

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBuildAccelerationStructuresKHR(m_Handle, 1, &buildGeometryInfo, rangeArrays);
}

NRI_INLINE void CommandBufferVK::BuildBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {
    const VkAccelerationStructureKHR dstASHandle = ((const AccelerationStructureVK&)dst).GetHandle();
    const VkDeviceAddress scratchAddress = ((BufferVK&)scratch).GetDeviceAddress() + scratchOffset;

    Scratch<VkAccelerationStructureGeometryKHR> geometries = AllocateScratch(m_Device, VkAccelerationStructureGeometryKHR, geometryObjectNum);
    Scratch<VkAccelerationStructureBuildRangeInfoKHR> ranges = AllocateScratch(m_Device, VkAccelerationStructureBuildRangeInfoKHR, geometryObjectNum);

    ConvertGeometryObjectsVK(geometries, ranges, geometryObjects, geometryObjectNum);

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildGeometryInfo.flags = GetAccelerationStructureBuildFlags(flags);
    buildGeometryInfo.dstAccelerationStructure = dstASHandle;
    buildGeometryInfo.geometryCount = geometryObjectNum;
    buildGeometryInfo.pGeometries = geometries;
    buildGeometryInfo.scratchData.deviceAddress = scratchAddress;

    const VkAccelerationStructureBuildRangeInfoKHR* rangeArrays[1] = {ranges};

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBuildAccelerationStructuresKHR(m_Handle, 1, &buildGeometryInfo, rangeArrays);
}

NRI_INLINE void CommandBufferVK::UpdateTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags,
    AccelerationStructure& dst, const AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {
    const VkAccelerationStructureKHR srcASHandle = ((const AccelerationStructureVK&)src).GetHandle();
    const VkAccelerationStructureKHR dstASHandle = ((const AccelerationStructureVK&)dst).GetHandle();
    const VkDeviceAddress scratchAddress = ((BufferVK&)scratch).GetDeviceAddress() + scratchOffset;
    const VkDeviceAddress bufferAddress = ((BufferVK&)buffer).GetDeviceAddress() + bufferOffset;

    VkAccelerationStructureGeometryKHR geometry = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.data.deviceAddress = bufferAddress;

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildGeometryInfo.flags = GetAccelerationStructureBuildFlags(flags);
    buildGeometryInfo.srcAccelerationStructure = srcASHandle;
    buildGeometryInfo.dstAccelerationStructure = dstASHandle;
    buildGeometryInfo.geometryCount = 1;
    buildGeometryInfo.pGeometries = &geometry;
    buildGeometryInfo.scratchData.deviceAddress = scratchAddress;

    VkAccelerationStructureBuildRangeInfoKHR range = {};
    range.primitiveCount = instanceNum;

    const VkAccelerationStructureBuildRangeInfoKHR* rangeArrays[1] = {&range};

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBuildAccelerationStructuresKHR(m_Handle, 1, &buildGeometryInfo, rangeArrays);
}

NRI_INLINE void CommandBufferVK::UpdateBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags,
    AccelerationStructure& dst, const AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {
    const VkAccelerationStructureKHR srcASHandle = ((const AccelerationStructureVK&)src).GetHandle();
    const VkAccelerationStructureKHR dstASHandle = ((const AccelerationStructureVK&)dst).GetHandle();
    const VkDeviceAddress scratchAddress = ((BufferVK&)scratch).GetDeviceAddress() + scratchOffset;

    Scratch<VkAccelerationStructureGeometryKHR> geometries = AllocateScratch(m_Device, VkAccelerationStructureGeometryKHR, geometryObjectNum);
    Scratch<VkAccelerationStructureBuildRangeInfoKHR> ranges = AllocateScratch(m_Device, VkAccelerationStructureBuildRangeInfoKHR, geometryObjectNum);

    ConvertGeometryObjectsVK(geometries, ranges, geometryObjects, geometryObjectNum);

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildGeometryInfo.flags = GetAccelerationStructureBuildFlags(flags);
    buildGeometryInfo.srcAccelerationStructure = srcASHandle;
    buildGeometryInfo.dstAccelerationStructure = dstASHandle;
    buildGeometryInfo.geometryCount = geometryObjectNum;
    buildGeometryInfo.pGeometries = geometries;
    buildGeometryInfo.scratchData.deviceAddress = scratchAddress;

    const VkAccelerationStructureBuildRangeInfoKHR* rangeArrays[1] = {ranges};

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdBuildAccelerationStructuresKHR(m_Handle, 1, &buildGeometryInfo, rangeArrays);
}

NRI_INLINE void CommandBufferVK::CopyAccelerationStructure(AccelerationStructure& dst, const AccelerationStructure& src, CopyMode copyMode) {
    const VkAccelerationStructureKHR dstASHandle = ((const AccelerationStructureVK&)dst).GetHandle();
    const VkAccelerationStructureKHR srcASHandle = ((const AccelerationStructureVK&)src).GetHandle();

    VkCopyAccelerationStructureInfoKHR info = {VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
    info.src = srcASHandle;
    info.dst = dstASHandle;
    info.mode = GetCopyMode(copyMode);

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdCopyAccelerationStructureKHR(m_Handle, &info);
}

NRI_INLINE void CommandBufferVK::WriteAccelerationStructureSize(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    Scratch<VkAccelerationStructureKHR> ASes = AllocateScratch(m_Device, VkAccelerationStructureKHR, accelerationStructureNum);

    for (uint32_t i = 0; i < accelerationStructureNum; i++)
        ASes[i] = ((const AccelerationStructureVK*)accelerationStructures[i])->GetHandle();

    const VkQueryPool queryPoolHandle = ((const QueryPoolVK&)queryPool).GetHandle();

    const auto& vk = m_Device.GetDispatchTable();
    vk.CmdWriteAccelerationStructuresPropertiesKHR(m_Handle, accelerationStructureNum, ASes, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPoolHandle, queryPoolOffset);
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
