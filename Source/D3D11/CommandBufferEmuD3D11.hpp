// © 2021 NVIDIA Corporation

enum OpCode : uint32_t {
    BEGIN,
    END,
    SET_VIEWPORTS,
    SET_SCISSORS,
    SET_DEPTH_BOUNDS,
    SET_STENCIL_REFERENCE,
    SET_SAMPLE_LOCATIONS,
    SET_BLEND_CONSTANTS,
    CLEAR_ATTACHMENTS,
    CLEAR_STORAGE,
    BEGIN_RENDERING,
    END_RENDERING,
    SET_VERTEX_BUFFERS,
    SET_INDEX_BUFFER,
    SET_PIPELINE_LAYOUT,
    SET_PIPELINE,
    SET_DESCRIPTOR_SET,
    SET_ROOT_CONSTANTS,
    SET_ROOT_DESCRIPTOR,
    DRAW,
    DRAW_INDEXED,
    DRAW_INDIRECT,
    DRAW_INDEXED_INDIRECT,
    COPY_BUFFER,
    COPY_TEXTURE,
    UPLOAD_BUFFER_TO_TEXTURE,
    READBACK_TEXTURE_TO_BUFFER,
    ZERO_BUFFER,
    RESOLVE_TEXTURE,
    DISPATCH,
    DISPATCH_INDIRECT,
    BARRIER,
    BEGIN_QUERY,
    END_QUERY,
    COPY_QUERIES,
    BEGIN_ANNOTATION,
    END_ANNOTATION,
    ANNOTATION,

    UNKNOWN
};

NRI_INLINE size_t GetElementNum(size_t dataSize) {
    return (dataSize + sizeof(uint32_t) - 1) / sizeof(uint32_t);
}

template <typename T>
NRI_INLINE void Push(PushBuffer& pushBuffer, const T& data) {
    const size_t bytes = sizeof(T);
    const size_t newElements = GetElementNum(bytes);
    const size_t curr = pushBuffer.size();

    pushBuffer.resize(curr + newElements);

    uint32_t* p = &pushBuffer[curr];
    memcpy(p, &data, bytes);
}

template <typename T>
NRI_INLINE void Push(PushBuffer& pushBuffer, const T* data, uint32_t num) {
    const size_t bytes = sizeof(T) * num;
    const size_t newElements = GetElementNum(sizeof(uint32_t) + bytes);
    const size_t curr = pushBuffer.size();

    pushBuffer.resize(curr + newElements);

    uint32_t* p = &pushBuffer[curr];
    *p++ = num;
    memcpy(p, data, bytes);
}

template <typename T>
NRI_INLINE void Read(PushBuffer& pushBuffer, size_t& i, T& data) {
    data = *(T*)&pushBuffer[i];
    i += GetElementNum(sizeof(T));
}

template <typename T>
NRI_INLINE void Read(PushBuffer& pushBuffer, size_t& i, T*& data, uint32_t& num) {
    num = pushBuffer[i++];
    data = (T*)&pushBuffer[i];
    i += GetElementNum(sizeof(T) * num);
}

//================================================================================================================
// CommandBufferBase
//================================================================================================================

Result CommandBufferEmuD3D11::Create(ID3D11DeviceContext*) {
    m_PushBuffer.reserve(256);

    return Result::SUCCESS;
}

void CommandBufferEmuD3D11::Submit() {
    CommandBufferD3D11 commandBuffer(m_Device);

    OpCode opCode = UNKNOWN;
    size_t i = 0;

    while (i < m_PushBuffer.size()) {
        Read(m_PushBuffer, i, opCode);

        switch (opCode) {
            case BEGIN: {
                DescriptorPool* descriptorPool;
                Read(m_PushBuffer, i, descriptorPool);

                if (descriptorPool)
                    commandBuffer.SetDescriptorPool(*descriptorPool);
            } break;
            case END:
                // we should return default state in emulation mode!
                commandBuffer.SetDepthBounds(0.0f, 1.0f);
                break;
            case SET_VIEWPORTS: {
                uint32_t viewportNum;
                Viewport* viewports;
                Read(m_PushBuffer, i, viewports, viewportNum);

                commandBuffer.SetViewports(viewports, viewportNum);
            } break;
            case SET_SCISSORS: {
                uint32_t rectNum;
                Rect* rects;
                Read(m_PushBuffer, i, rects, rectNum);

                commandBuffer.SetScissors(rects, rectNum);
            } break;
            case SET_DEPTH_BOUNDS: {
                float boundsMin;
                float boundsMax;
                Read(m_PushBuffer, i, boundsMin);
                Read(m_PushBuffer, i, boundsMax);

                commandBuffer.SetDepthBounds(boundsMin, boundsMax);
            } break;
            case SET_STENCIL_REFERENCE: {
                uint8_t frontRef;
                uint8_t backRef;
                Read(m_PushBuffer, i, frontRef);
                Read(m_PushBuffer, i, backRef);

                commandBuffer.SetStencilReference(frontRef, backRef);
            } break;
            case SET_SAMPLE_LOCATIONS: {
                SampleLocation* positions;
                uint32_t positionNum;
                Sample_t sampleNum;
                Read(m_PushBuffer, i, positions, positionNum);
                Read(m_PushBuffer, i, sampleNum);

                commandBuffer.SetSampleLocations(positions, (Sample_t)positionNum, sampleNum);
            } break;
            case SET_BLEND_CONSTANTS: {
                Color32f color;
                Read(m_PushBuffer, i, color);

                commandBuffer.SetBlendConstants(color);
            } break;
            case CLEAR_ATTACHMENTS: {
                ClearDesc* clearDescs;
                uint32_t clearDescNum;
                Read(m_PushBuffer, i, clearDescs, clearDescNum);

                Rect* rects;
                uint32_t rectNum;
                Read(m_PushBuffer, i, rects, rectNum);

                commandBuffer.ClearAttachments(clearDescs, clearDescNum, rects, rectNum);
            } break;
            case CLEAR_STORAGE: {
                ClearStorageDesc clearDesc = {};
                Read(m_PushBuffer, i, clearDesc);

                commandBuffer.ClearStorage(clearDesc);
            } break;
            case BEGIN_RENDERING: {
                AttachmentsDesc attachmentsDesc = {};
                Read(m_PushBuffer, i, attachmentsDesc);
                Read(m_PushBuffer, i, attachmentsDesc.colors, attachmentsDesc.colorNum);

                commandBuffer.BeginRendering(attachmentsDesc);
            } break;
            case END_RENDERING: {
                commandBuffer.ResetAttachments();
            } break;
            case SET_VERTEX_BUFFERS: {
                uint32_t baseSlot;
                Read(m_PushBuffer, i, baseSlot);

                VertexBufferDesc* vertexBufferDescs;
                uint32_t vertexBufferNum;
                Read(m_PushBuffer, i, vertexBufferDescs, vertexBufferNum);

                commandBuffer.SetVertexBuffers(baseSlot, vertexBufferDescs, vertexBufferNum);
            } break;
            case SET_INDEX_BUFFER: {
                Buffer* buffer;
                Read(m_PushBuffer, i, buffer);

                uint64_t offset;
                Read(m_PushBuffer, i, offset);

                IndexType indexType;
                Read(m_PushBuffer, i, indexType);

                commandBuffer.SetIndexBuffer(*buffer, offset, indexType);
            } break;
            case SET_PIPELINE_LAYOUT: {
                BindPoint bindPoint;
                Read(m_PushBuffer, i, bindPoint);

                PipelineLayout* pipelineLayout;
                Read(m_PushBuffer, i, pipelineLayout);

                commandBuffer.SetPipelineLayout(bindPoint, *pipelineLayout);
            } break;
            case SET_PIPELINE: {
                Pipeline* pipeline;
                Read(m_PushBuffer, i, pipeline);

                commandBuffer.SetPipeline(*pipeline);
            } break;
            case SET_DESCRIPTOR_SET: {
                SetDescriptorSetDesc setDescriptorSetDesc = {};
                Read(m_PushBuffer, i, setDescriptorSetDesc);

                commandBuffer.SetDescriptorSet(setDescriptorSetDesc);
            } break;
            case SET_ROOT_CONSTANTS: {
                SetRootConstantsDesc setRootConstantsDesc = {};
                Read(m_PushBuffer, i, setRootConstantsDesc);

                uint8_t* data;
                Read(m_PushBuffer, i, data, setRootConstantsDesc.size);
                setRootConstantsDesc.data = data;

                commandBuffer.SetRootConstants(setRootConstantsDesc);
            } break;
            case SET_ROOT_DESCRIPTOR: {
                SetRootDescriptorDesc setRootDescriptorDesc = {};
                Read(m_PushBuffer, i, setRootDescriptorDesc);

                commandBuffer.SetRootDescriptor(setRootDescriptorDesc);
            } break;
            case DRAW: {
                DrawDesc drawDesc = {};
                Read(m_PushBuffer, i, drawDesc);

                commandBuffer.Draw(drawDesc);
            } break;
            case DRAW_INDEXED: {
                DrawIndexedDesc drawIndexedDesc = {};
                Read(m_PushBuffer, i, drawIndexedDesc);

                commandBuffer.DrawIndexed(drawIndexedDesc);
            } break;
            case DRAW_INDIRECT: {
                Buffer* buffer;
                Read(m_PushBuffer, i, buffer);

                uint64_t offset;
                Read(m_PushBuffer, i, offset);

                uint32_t drawNum;
                Read(m_PushBuffer, i, drawNum);

                uint32_t stride;
                Read(m_PushBuffer, i, stride);

                Buffer* countBuffer;
                Read(m_PushBuffer, i, countBuffer);

                uint64_t countBufferOffset;
                Read(m_PushBuffer, i, countBufferOffset);

                commandBuffer.DrawIndirect(*buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
            } break;
            case DRAW_INDEXED_INDIRECT: {
                Buffer* buffer;
                Read(m_PushBuffer, i, buffer);

                uint64_t offset;
                Read(m_PushBuffer, i, offset);

                uint32_t drawNum;
                Read(m_PushBuffer, i, drawNum);

                uint32_t stride;
                Read(m_PushBuffer, i, stride);

                Buffer* countBuffer;
                Read(m_PushBuffer, i, countBuffer);

                uint64_t countBufferOffset;
                Read(m_PushBuffer, i, countBufferOffset);

                commandBuffer.DrawIndexedIndirect(*buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
            } break;
            case COPY_BUFFER: {
                Buffer* dstBuffer;
                Read(m_PushBuffer, i, dstBuffer);

                uint64_t dstOffset;
                Read(m_PushBuffer, i, dstOffset);

                Buffer* srcBuffer;
                Read(m_PushBuffer, i, srcBuffer);

                uint64_t srcOffset;
                Read(m_PushBuffer, i, srcOffset);

                uint64_t size;
                Read(m_PushBuffer, i, size);

                commandBuffer.CopyBuffer(*dstBuffer, dstOffset, *srcBuffer, srcOffset, size);
            } break;
            case COPY_TEXTURE: {
                Texture* dstTexture;
                Read(m_PushBuffer, i, dstTexture);

                TextureRegionDesc dstRegion = {};
                Read(m_PushBuffer, i, dstRegion);

                Texture* srcTexture;
                Read(m_PushBuffer, i, srcTexture);

                TextureRegionDesc srcRegion = {};
                Read(m_PushBuffer, i, srcRegion);

                commandBuffer.CopyTexture(*dstTexture, &dstRegion, *srcTexture, &srcRegion);
            } break;
            case UPLOAD_BUFFER_TO_TEXTURE: {
                Texture* dstTexture;
                Read(m_PushBuffer, i, dstTexture);

                TextureRegionDesc dstRegion = {};
                Read(m_PushBuffer, i, dstRegion);

                Buffer* srcBuffer;
                Read(m_PushBuffer, i, srcBuffer);

                TextureDataLayoutDesc srcDataLayout = {};
                Read(m_PushBuffer, i, srcDataLayout);

                commandBuffer.UploadBufferToTexture(*dstTexture, dstRegion, *srcBuffer, srcDataLayout);
            } break;
            case READBACK_TEXTURE_TO_BUFFER: {
                Buffer* dstBuffer;
                Read(m_PushBuffer, i, dstBuffer);

                TextureDataLayoutDesc dstDataLayout = {};
                Read(m_PushBuffer, i, dstDataLayout);

                Texture* srcTexture;
                Read(m_PushBuffer, i, srcTexture);

                TextureRegionDesc srcRegion = {};
                Read(m_PushBuffer, i, srcRegion);

                commandBuffer.ReadbackTextureToBuffer(*dstBuffer, dstDataLayout, *srcTexture, srcRegion);
            } break;
            case ZERO_BUFFER: {
                Buffer* buffer;
                Read(m_PushBuffer, i, buffer);

                uint64_t offset;
                Read(m_PushBuffer, i, offset);

                uint64_t size;
                Read(m_PushBuffer, i, size);

                commandBuffer.ZeroBuffer(*buffer, offset, size);
            } break;
            case RESOLVE_TEXTURE: {
                Texture* dstTexture;
                Read(m_PushBuffer, i, dstTexture);

                TextureRegionDesc dstRegion = {};
                Read(m_PushBuffer, i, dstRegion);

                Texture* srcTexture;
                Read(m_PushBuffer, i, srcTexture);

                TextureRegionDesc srcRegion = {};
                Read(m_PushBuffer, i, srcRegion);

                commandBuffer.ResolveTexture(*dstTexture, &dstRegion, *srcTexture, &srcRegion);
            } break;
            case DISPATCH: {
                DispatchDesc dispatchDesc;
                Read(m_PushBuffer, i, dispatchDesc);

                commandBuffer.Dispatch(dispatchDesc);
            } break;
            case DISPATCH_INDIRECT: {
                Buffer* buffer;
                Read(m_PushBuffer, i, buffer);

                uint64_t offset;
                Read(m_PushBuffer, i, offset);

                commandBuffer.DispatchIndirect(*buffer, offset);
            } break;
            case BARRIER: {
                BarrierDesc barrierDesc = {};
                Read(m_PushBuffer, i, barrierDesc.globals, barrierDesc.globalNum);
                Read(m_PushBuffer, i, barrierDesc.buffers, barrierDesc.bufferNum);
                Read(m_PushBuffer, i, barrierDesc.textures, barrierDesc.textureNum);

                commandBuffer.Barrier(barrierDesc);
            } break;
            case BEGIN_QUERY: {
                QueryPool* queryPool;
                Read(m_PushBuffer, i, queryPool);

                uint32_t offset;
                Read(m_PushBuffer, i, offset);

                commandBuffer.BeginQuery(*queryPool, offset);
            } break;
            case END_QUERY: {
                QueryPool* queryPool;
                Read(m_PushBuffer, i, queryPool);

                uint32_t offset;
                Read(m_PushBuffer, i, offset);

                commandBuffer.EndQuery(*queryPool, offset);
            } break;
            case COPY_QUERIES: {
                QueryPool* queryPool;
                Read(m_PushBuffer, i, queryPool);

                uint32_t offset;
                Read(m_PushBuffer, i, offset);

                uint32_t num;
                Read(m_PushBuffer, i, num);

                Buffer* buffer;
                Read(m_PushBuffer, i, buffer);

                uint64_t alignedBufferOffset;
                Read(m_PushBuffer, i, alignedBufferOffset);

                commandBuffer.CopyQueries(*queryPool, offset, num, *buffer, alignedBufferOffset);
            } break;
            case BEGIN_ANNOTATION: {
                uint32_t len;
                const char* name;
                Read(m_PushBuffer, i, name, len);

                uint32_t bgra;
                Read(m_PushBuffer, i, bgra);

                commandBuffer.BeginAnnotation(name, bgra);
            } break;
            case END_ANNOTATION: {
                commandBuffer.EndAnnotation();
            } break;
            case ANNOTATION: {
                uint32_t len;
                const char* name;
                Read(m_PushBuffer, i, name, len);

                uint32_t bgra;
                Read(m_PushBuffer, i, bgra);

                commandBuffer.Annotation(name, bgra);
            } break;
            default:
                CHECK(false, "Unexpected");
                break;
        }
    }
}

NRI_INLINE Result CommandBufferEmuD3D11::Begin(const DescriptorPool* descriptorPool) {
    m_PushBuffer.clear();
    Push(m_PushBuffer, BEGIN);
    Push(m_PushBuffer, descriptorPool);

    return Result::SUCCESS;
}

NRI_INLINE Result CommandBufferEmuD3D11::End() {
    Push(m_PushBuffer, END);

    return Result::SUCCESS;
}

NRI_INLINE void CommandBufferEmuD3D11::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    Push(m_PushBuffer, SET_VIEWPORTS);
    Push(m_PushBuffer, viewports, viewportNum);
}

NRI_INLINE void CommandBufferEmuD3D11::SetScissors(const Rect* rects, uint32_t rectNum) {
    Push(m_PushBuffer, SET_SCISSORS);
    Push(m_PushBuffer, rects, rectNum);
}

NRI_INLINE void CommandBufferEmuD3D11::SetDepthBounds(float boundsMin, float boundsMax) {
    Push(m_PushBuffer, SET_DEPTH_BOUNDS);
    Push(m_PushBuffer, boundsMin);
    Push(m_PushBuffer, boundsMax);
}

NRI_INLINE void CommandBufferEmuD3D11::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    Push(m_PushBuffer, SET_STENCIL_REFERENCE);
    Push(m_PushBuffer, frontRef);
    Push(m_PushBuffer, backRef);
}

NRI_INLINE void CommandBufferEmuD3D11::SetSampleLocations(const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    Push(m_PushBuffer, SET_SAMPLE_LOCATIONS);
    Push(m_PushBuffer, locations, (uint32_t)locationNum);
    Push(m_PushBuffer, sampleNum);
}

NRI_INLINE void CommandBufferEmuD3D11::SetBlendConstants(const Color32f& color) {
    Push(m_PushBuffer, SET_BLEND_CONSTANTS);
    Push(m_PushBuffer, color);
}

NRI_INLINE void CommandBufferEmuD3D11::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    Push(m_PushBuffer, CLEAR_ATTACHMENTS);
    Push(m_PushBuffer, clearDescs, clearDescNum);
    Push(m_PushBuffer, rects, rectNum);
}

NRI_INLINE void CommandBufferEmuD3D11::ClearStorage(const ClearStorageDesc& clearDesc) {
    Push(m_PushBuffer, CLEAR_STORAGE);
    Push(m_PushBuffer, clearDesc);
}

NRI_INLINE void CommandBufferEmuD3D11::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    Push(m_PushBuffer, BEGIN_RENDERING);
    Push(m_PushBuffer, attachmentsDesc);
    Push(m_PushBuffer, attachmentsDesc.colors, attachmentsDesc.colorNum);
}

NRI_INLINE void CommandBufferEmuD3D11::EndRendering() {
    Push(m_PushBuffer, END_RENDERING);
}

NRI_INLINE void CommandBufferEmuD3D11::SetVertexBuffers(uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum) {
    Push(m_PushBuffer, SET_VERTEX_BUFFERS);
    Push(m_PushBuffer, baseSlot);
    Push(m_PushBuffer, vertexBufferDescs, vertexBufferNum);
}

NRI_INLINE void CommandBufferEmuD3D11::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    Push(m_PushBuffer, SET_INDEX_BUFFER);
    Push(m_PushBuffer, &buffer);
    Push(m_PushBuffer, offset);
    Push(m_PushBuffer, indexType);
}

NRI_INLINE void CommandBufferEmuD3D11::SetPipelineLayout(BindPoint bindPoint, const PipelineLayout& pipelineLayout) {
    Push(m_PushBuffer, SET_PIPELINE_LAYOUT);
    Push(m_PushBuffer, bindPoint);
    Push(m_PushBuffer, &pipelineLayout);
}

NRI_INLINE void CommandBufferEmuD3D11::SetPipeline(const Pipeline& pipeline) {
    Push(m_PushBuffer, SET_PIPELINE);
    Push(m_PushBuffer, &pipeline);
}

NRI_INLINE void CommandBufferEmuD3D11::SetDescriptorSet(const SetDescriptorSetDesc& setDescriptorSetDesc) {
    Push(m_PushBuffer, SET_DESCRIPTOR_SET);
    Push(m_PushBuffer, setDescriptorSetDesc);
}

NRI_INLINE void CommandBufferEmuD3D11::SetRootConstants(const SetRootConstantsDesc& setRootConstantsDesc) {
    Push(m_PushBuffer, SET_ROOT_CONSTANTS);
    Push(m_PushBuffer, setRootConstantsDesc);
    Push(m_PushBuffer, (uint8_t*)setRootConstantsDesc.data, setRootConstantsDesc.size);
}

NRI_INLINE void CommandBufferEmuD3D11::SetRootDescriptor(const SetRootDescriptorDesc& setRootDescriptorDesc) {
    Push(m_PushBuffer, SET_ROOT_DESCRIPTOR);
    Push(m_PushBuffer, setRootDescriptorDesc);
}

NRI_INLINE void CommandBufferEmuD3D11::Draw(const DrawDesc& drawDesc) {
    Push(m_PushBuffer, DRAW);
    Push(m_PushBuffer, drawDesc);
}

NRI_INLINE void CommandBufferEmuD3D11::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    Push(m_PushBuffer, DRAW_INDEXED);
    Push(m_PushBuffer, drawIndexedDesc);
}

NRI_INLINE void CommandBufferEmuD3D11::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    Push(m_PushBuffer, DRAW_INDIRECT);
    Push(m_PushBuffer, &buffer);
    Push(m_PushBuffer, offset);
    Push(m_PushBuffer, drawNum);
    Push(m_PushBuffer, stride);
    Push(m_PushBuffer, countBuffer);
    Push(m_PushBuffer, countBufferOffset);
}

NRI_INLINE void CommandBufferEmuD3D11::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    Push(m_PushBuffer, DRAW_INDEXED_INDIRECT);
    Push(m_PushBuffer, &buffer);
    Push(m_PushBuffer, offset);
    Push(m_PushBuffer, drawNum);
    Push(m_PushBuffer, stride);
    Push(m_PushBuffer, countBuffer);
    Push(m_PushBuffer, countBufferOffset);
}

NRI_INLINE void CommandBufferEmuD3D11::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    Push(m_PushBuffer, COPY_BUFFER);
    Push(m_PushBuffer, &dstBuffer);
    Push(m_PushBuffer, dstOffset);
    Push(m_PushBuffer, &srcBuffer);
    Push(m_PushBuffer, srcOffset);
    Push(m_PushBuffer, size);
}

NRI_INLINE void CommandBufferEmuD3D11::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    TextureRegionDesc wholeResource = {};
    wholeResource.mipOffset = NULL_TEXTURE_REGION_DESC;

    if (!dstRegion)
        dstRegion = &wholeResource;
    if (!srcRegion)
        srcRegion = &wholeResource;

    Push(m_PushBuffer, COPY_TEXTURE);
    Push(m_PushBuffer, &dstTexture);
    Push(m_PushBuffer, *dstRegion);
    Push(m_PushBuffer, &srcTexture);
    Push(m_PushBuffer, *srcRegion);
}

NRI_INLINE void CommandBufferEmuD3D11::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegion, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayout) {
    Push(m_PushBuffer, UPLOAD_BUFFER_TO_TEXTURE);
    Push(m_PushBuffer, &dstTexture);
    Push(m_PushBuffer, dstRegion);
    Push(m_PushBuffer, &srcBuffer);
    Push(m_PushBuffer, srcDataLayout);
}

NRI_INLINE void CommandBufferEmuD3D11::ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayout, const Texture& srcTexture, const TextureRegionDesc& srcRegion) {
    Push(m_PushBuffer, READBACK_TEXTURE_TO_BUFFER);
    Push(m_PushBuffer, &dstBuffer);
    Push(m_PushBuffer, dstDataLayout);
    Push(m_PushBuffer, &srcTexture);
    Push(m_PushBuffer, srcRegion);
}

NRI_INLINE void CommandBufferEmuD3D11::ZeroBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    Push(m_PushBuffer, ZERO_BUFFER);
    Push(m_PushBuffer, &buffer);
    Push(m_PushBuffer, offset);
    Push(m_PushBuffer, size);
}

NRI_INLINE void CommandBufferEmuD3D11::ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    TextureRegionDesc wholeResource = {};
    wholeResource.mipOffset = NULL_TEXTURE_REGION_DESC;

    if (!dstRegion)
        dstRegion = &wholeResource;
    if (!srcRegion)
        srcRegion = &wholeResource;

    Push(m_PushBuffer, RESOLVE_TEXTURE);
    Push(m_PushBuffer, &dstTexture);
    Push(m_PushBuffer, *dstRegion);
    Push(m_PushBuffer, &srcTexture);
    Push(m_PushBuffer, *srcRegion);
}

NRI_INLINE void CommandBufferEmuD3D11::Dispatch(const DispatchDesc& dispatchDesc) {
    Push(m_PushBuffer, DISPATCH);
    Push(m_PushBuffer, dispatchDesc);
}

NRI_INLINE void CommandBufferEmuD3D11::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    Push(m_PushBuffer, DISPATCH_INDIRECT);
    Push(m_PushBuffer, &buffer);
    Push(m_PushBuffer, offset);
}

NRI_INLINE void CommandBufferEmuD3D11::Barrier(const BarrierDesc& barrierDesc) {
    Push(m_PushBuffer, BARRIER);
    Push(m_PushBuffer, barrierDesc.globals, barrierDesc.globalNum);
    Push(m_PushBuffer, barrierDesc.buffers, barrierDesc.bufferNum);
    Push(m_PushBuffer, barrierDesc.textures, barrierDesc.textureNum);
}

NRI_INLINE void CommandBufferEmuD3D11::BeginQuery(QueryPool& queryPool, uint32_t offset) {
    Push(m_PushBuffer, BEGIN_QUERY);
    Push(m_PushBuffer, &queryPool);
    Push(m_PushBuffer, offset);
}

NRI_INLINE void CommandBufferEmuD3D11::EndQuery(QueryPool& queryPool, uint32_t offset) {
    Push(m_PushBuffer, END_QUERY);
    Push(m_PushBuffer, &queryPool);
    Push(m_PushBuffer, offset);
}

NRI_INLINE void CommandBufferEmuD3D11::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    Push(m_PushBuffer, COPY_QUERIES);
    Push(m_PushBuffer, &queryPool);
    Push(m_PushBuffer, offset);
    Push(m_PushBuffer, num);
    Push(m_PushBuffer, &dstBuffer);
    Push(m_PushBuffer, dstOffset);
}

NRI_INLINE void CommandBufferEmuD3D11::BeginAnnotation(const char* name, uint32_t bgra) {
    uint32_t len = (uint32_t)std::strlen(name) + 1;

    Push(m_PushBuffer, BEGIN_ANNOTATION);
    Push(m_PushBuffer, name, len);
    Push(m_PushBuffer, bgra);
}

NRI_INLINE void CommandBufferEmuD3D11::EndAnnotation() {
    Push(m_PushBuffer, END_ANNOTATION);
}

NRI_INLINE void CommandBufferEmuD3D11::Annotation(const char* name, uint32_t bgra) {
    uint32_t len = (uint32_t)std::strlen(name) + 1;

    Push(m_PushBuffer, ANNOTATION);
    Push(m_PushBuffer, name, len);
    Push(m_PushBuffer, bgra);
}
