// © 2025 NVIDIA Corporation

#if NRI_ENABLE_IMGUI_EXTENSION

#    include "ShaderMake/ShaderBlob.h"

#    if NRI_ENABLE_D3D11_SUPPORT
#        include "Imgui.fs.dxbc.h"
#        include "Imgui.vs.dxbc.h"
#    endif

#    if NRI_ENABLE_D3D12_SUPPORT
#        include "Imgui.fs.dxil.h"
#        include "Imgui.vs.dxil.h"
#    endif

#    if NRI_ENABLE_VK_SUPPORT
#        include "Imgui.fs.spirv.h"
#        include "Imgui.vs.spirv.h"
#    endif

#    include "../Shaders/Imgui.fs.hlsl"
#    include "../Shaders/Imgui.vs.hlsl"

// Copied from Imgui // TODO: always keep in sync with latest

typedef uint16_t ImDrawIdx;
typedef uint64_t ImTextureID;

constexpr ImTextureID ImTextureID_Invalid = 0;

enum ImTextureFormat {
    ImTextureFormat_RGBA32,
    ImTextureFormat_Alpha8,
};

enum ImTextureStatus {
    ImTextureStatus_OK,
    ImTextureStatus_Destroyed,
    ImTextureStatus_WantCreate,
    ImTextureStatus_WantUpdates,
    ImTextureStatus_WantDestroy,
};

template <typename T>
struct ImVector {
    int32_t Size;
    int32_t Capacity;
    T* Data;
};

struct ImVec4 {
    float x, y, z, w;
};

struct ImVec2 {
    float x, y;
};

struct ImDrawVert {
    ImVec2 pos;
    ImVec2 uv;
    uint32_t col;
};

struct ImTextureRect {
    uint16_t x, y, w, h;
};

struct ImTextureData {
    int UniqueID;
    ImTextureStatus Status;
    void* BackendUserData;
    ImTextureID TexID;
    ImTextureFormat Format;
    int Width;
    int Height;
    int BytesPerPixel;
    unsigned char* Pixels;
    ImTextureRect UsedRect;
    ImTextureRect UpdateRect;
    ImVector<ImTextureRect> Updates;
    int UnusedFrames;
    unsigned short RefCount;
    bool UseColors;
    bool WantDestroyNextFrame;
};

struct ImTextureRef {
    ImTextureData* TexData;
    ImTextureID TexID;
};

struct ImDrawCmd {
    ImVec4 ClipRect;
    ImTextureRef TexRef;
    uint32_t VtxOffset;
    uint32_t IdxOffset;
    uint32_t ElemCount;
    void* UserCallback;
    void* UserCallbackData;
    int32_t UserCallbackDataSize;
    int32_t UserCallbackDataOffset;
};

struct ImDrawList {
    ImVector<ImDrawCmd> CmdBuffer;
    ImVector<ImDrawIdx> IdxBuffer;
    ImVector<ImDrawVert> VtxBuffer;

    // rest is unreferenced
};

// Implementation

ImguiImpl::~ImguiImpl() {
    for (auto& entry : m_Textures) {
        m_iCore.DestroyDescriptor(*entry.second.descriptor);
        m_iCore.DestroyTexture(*entry.second.texture);

        ImTextureData* imguiTextureData = (ImTextureData*)entry.first;
        imguiTextureData->BackendUserData = nullptr;
        imguiTextureData->TexID = ImTextureID_Invalid;
        imguiTextureData->Status = ImTextureStatus_Destroyed;
    }

    for (const ImguiPipeline& entry : m_Pipelines)
        m_iCore.DestroyPipeline(*entry.pipeline);

    m_iCore.DestroyPipelineLayout(*m_PipelineLayout);
    m_iCore.DestroyDescriptorPool(*m_DescriptorPool);
    m_iCore.DestroyDescriptor(*m_Sampler);
}

Result ImguiImpl::Create(const ImguiDesc& imguiDesc) {
    { // Get streamer interface
        Result result = nriGetInterface(m_Device, NRI_INTERFACE(StreamerInterface), &m_iStreamer);
        if (result != Result::SUCCESS)
            return result;
    }

    { // Create sampller
        SamplerDesc viewDesc = {};
        viewDesc.filters.min = Filter::LINEAR;
        viewDesc.filters.mag = Filter::LINEAR;
        viewDesc.addressModes.u = AddressMode::REPEAT;
        viewDesc.addressModes.v = AddressMode::REPEAT;

        Result result = m_iCore.CreateSampler(m_Device, viewDesc, m_Sampler);
        if (result != Result::SUCCESS)
            return result;
    }

    { // Pipeline layout
        RootConstantDesc rootConstants = {};
        rootConstants.registerIndex = 0;
        rootConstants.shaderStages = StageBits::VERTEX_SHADER;
        rootConstants.size = sizeof(ImguiConstants);

        const DescriptorRangeDesc descriptorSet0Ranges[] = {
            {1, 1, DescriptorType::SAMPLER, StageBits::FRAGMENT_SHADER},
        };

        const DescriptorRangeDesc descriptorSet1Ranges[] = {
            {0, 1, DescriptorType::TEXTURE, StageBits::FRAGMENT_SHADER, DescriptorRangeBits::ALLOW_UPDATE_AFTER_SET},
        };

        DescriptorSetDesc descriptorSetDescs[2] = {};
        {
            descriptorSetDescs[IMGUI_SAMPLER_SET].registerSpace = IMGUI_SAMPLER_SET;
            descriptorSetDescs[IMGUI_SAMPLER_SET].ranges = descriptorSet0Ranges;
            descriptorSetDescs[IMGUI_SAMPLER_SET].rangeNum = GetCountOf(descriptorSet0Ranges);

            descriptorSetDescs[IMGUI_TEXTURE_SET].registerSpace = IMGUI_TEXTURE_SET;
            descriptorSetDescs[IMGUI_TEXTURE_SET].ranges = descriptorSet1Ranges;
            descriptorSetDescs[IMGUI_TEXTURE_SET].rangeNum = GetCountOf(descriptorSet1Ranges);
            descriptorSetDescs[IMGUI_TEXTURE_SET].flags = DescriptorSetBits::ALLOW_UPDATE_AFTER_SET;
        }

        PipelineLayoutDesc pipelineLayoutDesc = {};
        pipelineLayoutDesc.rootRegisterSpace = 0;
        pipelineLayoutDesc.rootConstants = &rootConstants;
        pipelineLayoutDesc.rootConstantNum = 1;
        pipelineLayoutDesc.descriptorSets = descriptorSetDescs;
        pipelineLayoutDesc.descriptorSetNum = GetCountOf(descriptorSetDescs);
        pipelineLayoutDesc.shaderStages = StageBits::VERTEX_SHADER | StageBits::FRAGMENT_SHADER;
        pipelineLayoutDesc.flags = PipelineLayoutBits::IGNORE_GLOBAL_SPIRV_OFFSETS;

        Result result = m_iCore.CreatePipelineLayout(m_Device, pipelineLayoutDesc, m_PipelineLayout);
        if (result != Result::SUCCESS)
            return result;
    }

    { // Descriptor pool
        DescriptorPoolDesc descriptorPoolDesc = {};

        // Static
        descriptorPoolDesc.descriptorSetMaxNum = 1;
        descriptorPoolDesc.samplerMaxNum = 1;

        // Dynamic
        uint32_t dynamicPoolSize = imguiDesc.descriptorPoolSize ? imguiDesc.descriptorPoolSize : 128;
        m_DescriptorSets1.resize(dynamicPoolSize);

        descriptorPoolDesc.descriptorSetMaxNum += dynamicPoolSize;
        descriptorPoolDesc.textureMaxNum += dynamicPoolSize;
        descriptorPoolDesc.flags = DescriptorPoolBits::ALLOW_UPDATE_AFTER_SET;

        Result result = m_iCore.CreateDescriptorPool(m_Device, descriptorPoolDesc, m_DescriptorPool);
        if (result != Result::SUCCESS)
            return result;
    }

    { // Descriptor sets
        Result result = m_iCore.AllocateDescriptorSets(*m_DescriptorPool, *m_PipelineLayout, IMGUI_SAMPLER_SET, &m_DescriptorSet0_sampler, 1, 0);
        if (result != Result::SUCCESS)
            return result;

        result = m_iCore.AllocateDescriptorSets(*m_DescriptorPool, *m_PipelineLayout, IMGUI_TEXTURE_SET, m_DescriptorSets1.data(), (uint32_t)m_DescriptorSets1.size(), 0);
        if (result != Result::SUCCESS)
            return result;
    }

    { // Update static set with sampler
        DescriptorRangeUpdateDesc descriptorRangeUpdateDesc = {&m_Sampler, 1};
        m_iCore.UpdateDescriptorRanges(*m_DescriptorSet0_sampler, 0, 1, &descriptorRangeUpdateDesc);
    }

    return Result::SUCCESS;
}

void ImguiImpl::CmdCopyData(CommandBuffer& commandBuffer, Streamer& streamer, const CopyImguiDataDesc& copyImguiDataDesc) {
    ExclusiveScope lock(m_Lock);

    if (!copyImguiDataDesc.drawListNum)
        return;

    m_UpdateTick++;

    Scratch<TextureBarrierDesc> textureBarriers = AllocateScratch((DeviceBase&)m_Device, TextureBarrierDesc, copyImguiDataDesc.textureNum);
    uint32_t textureBarrierNum = 0;

    const AccessLayoutStage copyState = {AccessBits::COPY_DESTINATION, Layout::COPY_DESTINATION, StageBits::COPY};
    const AccessLayoutStage drawState = {AccessBits::SHADER_RESOURCE, Layout::SHADER_RESOURCE, StageBits::FRAGMENT_SHADER};

    // Update textures
    for (uint32_t i = 0; i < copyImguiDataDesc.textureNum; i++) {
        ImTextureData* imguiTextureData = copyImguiDataDesc.textures[i];
        uint64_t key = (uint64_t)imguiTextureData; // TODO: can't use a better "key", because of "(ImTextureData*)entry.first"

        { // Phase 1: satisfy ImGui - which naively assumes that the only one device renders a UI instance
            CHECK(imguiTextureData->Status != ImTextureStatus_Destroyed, "Unexpected");

            // Destroy
            if (imguiTextureData->Status == ImTextureStatus_WantDestroy && imguiTextureData->UnusedFrames > 8) { // TODO: keep an eye on 8...
                imguiTextureData->TexID = ImTextureID_Invalid;
                imguiTextureData->BackendUserData = nullptr;
                imguiTextureData->Status = ImTextureStatus_Destroyed;
            }

            // Create or update
            if (imguiTextureData->Status == ImTextureStatus_WantCreate || imguiTextureData->Status == ImTextureStatus_WantUpdates) {
                imguiTextureData->TexID = key;
                imguiTextureData->BackendUserData = (void*)m_UpdateTick;
                imguiTextureData->Status = ImTextureStatus_OK;
            }
        }

        { // Phase 2: real logic - NRI supports rendering of the same UI instance on multiple devices
            auto entry = m_Textures.find(key);
            if (entry == m_Textures.end())
                entry = m_Textures.insert({key, {}}).first;

            ImguiTexture& imguiTexture = entry->second;
            Format format = imguiTextureData->Format == ImTextureFormat_RGBA32 ? Format::RGBA8_UNORM : Format::R8_UNORM;
            uint64_t updateTick = (uint64_t)imguiTextureData->BackendUserData;

            // Destroy
            if (imguiTextureData->Status == ImTextureStatus_Destroyed) {
                m_iCore.DestroyDescriptor(*imguiTexture.descriptor);
                m_iCore.DestroyTexture(*imguiTexture.texture);

                m_Textures.erase(key);

                continue;
            }

            // Create
            bool isCreated = false;
            if (!imguiTexture.texture) {
                { // Create texture
                    ResourceAllocatorInterface iResourceAllocator = {};
                    Result result = nriGetInterface(m_Device, NRI_INTERFACE(ResourceAllocatorInterface), &iResourceAllocator);
                    CHECK(result == Result::SUCCESS, "Unexpected");

                    AllocateTextureDesc textureDesc = {};
                    textureDesc.desc.type = TextureType::TEXTURE_2D;
                    textureDesc.desc.usage = TextureUsageBits::SHADER_RESOURCE;
                    textureDesc.desc.format = format;
                    textureDesc.desc.width = (Dim_t)imguiTextureData->Width;
                    textureDesc.desc.height = (Dim_t)imguiTextureData->Height;
                    textureDesc.memoryLocation = MemoryLocation::DEVICE;

                    result = iResourceAllocator.AllocateTexture(m_Device, textureDesc, imguiTexture.texture);
                    CHECK(result == Result::SUCCESS, "Unexpected");
                }

                { // Create descriptor
                    Texture2DViewDesc viewDesc = {};
                    viewDesc.texture = imguiTexture.texture;
                    viewDesc.viewType = Texture2DViewType::SHADER_RESOURCE_2D;
                    viewDesc.format = format;

                    Result result = m_iCore.CreateTexture2DView(viewDesc, imguiTexture.descriptor);
                    CHECK(result == Result::SUCCESS, "Unexpected");
                }

                isCreated = true; // ImGui doesn't provide anything in "Updates" on creation
            }

            // Update
            if (imguiTexture.updateTick < updateTick) {
                const FormatProps& formatProps = GetFormatProps(format);

                if (isCreated || !imguiTextureData->Updates.Size) {
                    // Full update
                    StreamTextureDataDesc streamTextureDataDesc = {};
                    streamTextureDataDesc.data = imguiTextureData->Pixels;
                    streamTextureDataDesc.dataRowPitch = imguiTextureData->Width * formatProps.stride;
                    streamTextureDataDesc.dataSlicePitch = imguiTextureData->Height * streamTextureDataDesc.dataRowPitch;
                    streamTextureDataDesc.dstTexture = imguiTexture.texture;

                    m_iStreamer.StreamTextureData(streamer, streamTextureDataDesc);
                } else {
                    // Partial updates // TODO: what if some updates are skipped due to inactivity?
                    for (int32_t j = 0; j < imguiTextureData->Updates.Size; j++) {
                        const ImTextureRect& rect = imguiTextureData->Updates.Data[j];

                        StreamTextureDataDesc streamTextureDataDesc = {};
                        streamTextureDataDesc.data = imguiTextureData->Pixels + (rect.x + rect.y * imguiTextureData->Width) * formatProps.stride;
                        streamTextureDataDesc.dataRowPitch = imguiTextureData->Width * formatProps.stride;
                        streamTextureDataDesc.dataSlicePitch = imguiTextureData->Height * streamTextureDataDesc.dataRowPitch;
                        streamTextureDataDesc.dstTexture = imguiTexture.texture;
                        streamTextureDataDesc.dstRegion.x = rect.x;
                        streamTextureDataDesc.dstRegion.y = rect.y;
                        streamTextureDataDesc.dstRegion.width = rect.w;
                        streamTextureDataDesc.dstRegion.height = rect.h;

                        m_iStreamer.StreamTextureData(streamer, streamTextureDataDesc);
                    }
                }

                { // Add a barrier
                    TextureBarrierDesc& textureBarrier = textureBarriers[textureBarrierNum++];

                    textureBarrier = {};
                    textureBarrier.texture = imguiTexture.texture;
                    textureBarrier.before = drawState;
                    textureBarrier.after = copyState;

                    if (isCreated)
                        textureBarrier.before = {};
                }

                imguiTexture.updateTick = updateTick;
            }
        }
    }

    { // Stream buffer data
        uint32_t dataChunkNum = copyImguiDataDesc.drawListNum * 2;
        Scratch<DataSize> dataChunks = AllocateScratch((DeviceBase&)m_Device, DataSize, dataChunkNum);

        StreamBufferDataDesc streamBufferDataDesc = {};
        streamBufferDataDesc.dataChunkNum = dataChunkNum;
        streamBufferDataDesc.dataChunks = dataChunks;
        streamBufferDataDesc.placementAlignment = 4;

        uint64_t totalVertexDataSize = 0;
        for (uint32_t n = 0; n < copyImguiDataDesc.drawListNum; n++) {
            const ImDrawList* drawList = copyImguiDataDesc.drawLists[n];

            DataSize& vertexDataChunk = dataChunks[n];
            vertexDataChunk.data = drawList->VtxBuffer.Data;
            vertexDataChunk.size = drawList->VtxBuffer.Size * sizeof(ImDrawVert);

            DataSize& indexDataChunk = dataChunks[copyImguiDataDesc.drawListNum + n];
            indexDataChunk.data = drawList->IdxBuffer.Data;
            indexDataChunk.size = drawList->IdxBuffer.Size * sizeof(ImDrawIdx);

            totalVertexDataSize += vertexDataChunk.size;
        }

        BufferOffset bufferOffset = m_iStreamer.StreamBufferData(streamer, streamBufferDataDesc);

        m_VbOffset = bufferOffset.offset;
        m_IbOffset = m_VbOffset + totalVertexDataSize;
        m_CurrentBuffer = bufferOffset.buffer;
    }

    // Copy texture data
    if (textureBarrierNum) {
        BarrierGroupDesc barrierGroupDesc = {};
        barrierGroupDesc.textureNum = textureBarrierNum;
        barrierGroupDesc.textures = textureBarriers;

        m_iCore.CmdBarrier(commandBuffer, barrierGroupDesc);

        m_iStreamer.CmdCopyStreamedData(commandBuffer, streamer);

        for (uint32_t i = 0; i < textureBarrierNum; i++) {
            textureBarriers[i].before = copyState;
            textureBarriers[i].after = drawState;
        }

        m_iCore.CmdBarrier(commandBuffer, barrierGroupDesc);
    }
}

void ImguiImpl::CmdDraw(CommandBuffer& commandBuffer, const DrawImguiDesc& drawImguiDesc) {
    ExclusiveScope lock(m_Lock);

    if (!drawImguiDesc.drawListNum)
        return;

    // Pipeline
    Pipeline* pipeline = nullptr;
    for (size_t i = 0; i < m_Pipelines.size(); i++) {
        const ImguiPipeline& imguiPipeline = m_Pipelines[i];
        if (imguiPipeline.format == drawImguiDesc.attachmentFormat && imguiPipeline.linearColor == drawImguiDesc.linearColor) {
            pipeline = m_Pipelines[i].pipeline;
            break;
        }
    }

    if (!pipeline) {
        const DeviceDesc& deviceDesc = m_iCore.GetDeviceDesc(m_Device);

        ShaderMake::ShaderConstant define = {"IMGUI_LINEAR_COLOR", drawImguiDesc.linearColor ? "1" : "0"};

        ShaderDesc shaders[] = {
            {StageBits::VERTEX_SHADER, nullptr, 0},
            {StageBits::FRAGMENT_SHADER, nullptr, 0},
        };

        // Temporary variables for size_t conversion on macOS ARM64
        size_t vsSize = 0;
        size_t fsSize = 0;

        bool shaderMakeResult = false;
#    if NRI_ENABLE_D3D11_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D11) {
            shaderMakeResult = ShaderMake::FindPermutationInBlob(g_Imgui_vs_dxbc, GetCountOf(g_Imgui_vs_dxbc), &define, 1, &shaders[0].bytecode, &vsSize);
            shaderMakeResult |= ShaderMake::FindPermutationInBlob(g_Imgui_fs_dxbc, GetCountOf(g_Imgui_fs_dxbc), nullptr, 0, &shaders[1].bytecode, &fsSize);
            shaders[0].size = vsSize;
            shaders[1].size = fsSize;
        }
#    endif
#    if NRI_ENABLE_D3D12_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12) {
            shaderMakeResult = ShaderMake::FindPermutationInBlob(g_Imgui_vs_dxil, GetCountOf(g_Imgui_vs_dxil), &define, 1, &shaders[0].bytecode, &vsSize);
            shaderMakeResult |= ShaderMake::FindPermutationInBlob(g_Imgui_fs_dxil, GetCountOf(g_Imgui_fs_dxil), nullptr, 0, &shaders[1].bytecode, &fsSize);
            shaders[0].size = vsSize;
            shaders[1].size = fsSize;
        }
#    endif
#    if NRI_ENABLE_VK_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::VK) {
            shaderMakeResult = ShaderMake::FindPermutationInBlob(g_Imgui_vs_spirv, GetCountOf(g_Imgui_vs_spirv), &define, 1, &shaders[0].bytecode, &vsSize);
            shaderMakeResult |= ShaderMake::FindPermutationInBlob(g_Imgui_fs_spirv, GetCountOf(g_Imgui_fs_spirv), nullptr, 0, &shaders[1].bytecode, &fsSize);
            shaders[0].size = vsSize;
            shaders[1].size = fsSize;
        }
#    endif
        CHECK(shaderMakeResult, "Unexpected");

        const VertexAttributeDesc vertexAttributeDesc[] = {
            {{"POSITION", 0}, {0}, GetOffsetOf(&ImDrawVert::pos), Format::RG32_SFLOAT},
            {{"TEXCOORD", 0}, {1}, GetOffsetOf(&ImDrawVert::uv), Format::RG32_SFLOAT},
            {{"COLOR", 0}, {2}, GetOffsetOf(&ImDrawVert::col), Format::RGBA8_UNORM},
        };

        VertexStreamDesc stream = {};
        stream.bindingSlot = 0;

        VertexInputDesc vertexInput = {};
        vertexInput.attributes = vertexAttributeDesc;
        vertexInput.attributeNum = (uint8_t)GetCountOf(vertexAttributeDesc);
        vertexInput.streams = &stream;
        vertexInput.streamNum = 1;

        ColorAttachmentDesc colorAttachment = {};
        colorAttachment.format = drawImguiDesc.attachmentFormat;
        colorAttachment.colorBlend.srcFactor = BlendFactor::SRC_ALPHA,
        colorAttachment.colorBlend.dstFactor = BlendFactor::ONE_MINUS_SRC_ALPHA,
        colorAttachment.colorBlend.op = BlendOp::ADD,
        colorAttachment.alphaBlend.srcFactor = BlendFactor::ONE_MINUS_SRC_ALPHA,
        colorAttachment.alphaBlend.dstFactor = BlendFactor::ZERO,
        colorAttachment.alphaBlend.op = BlendOp::ADD,
        colorAttachment.colorWriteMask = ColorWriteBits::RGB;
        colorAttachment.blendEnabled = true;

        GraphicsPipelineDesc graphicsPipelineDesc = {};
        graphicsPipelineDesc.pipelineLayout = m_PipelineLayout;
        graphicsPipelineDesc.vertexInput = &vertexInput;
        graphicsPipelineDesc.inputAssembly.topology = Topology::TRIANGLE_LIST;
        graphicsPipelineDesc.rasterization.fillMode = FillMode::SOLID;
        graphicsPipelineDesc.rasterization.cullMode = CullMode::NONE;
        graphicsPipelineDesc.outputMerger.colors = &colorAttachment;
        graphicsPipelineDesc.outputMerger.colorNum = 1;
        graphicsPipelineDesc.shaders = shaders;
        graphicsPipelineDesc.shaderNum = GetCountOf(shaders);

        Result result = m_iCore.CreateGraphicsPipeline(m_Device, graphicsPipelineDesc, pipeline);
        CHECK(result == Result::SUCCESS, "Unexpected");

        m_Pipelines.push_back({pipeline, drawImguiDesc.attachmentFormat, drawImguiDesc.linearColor});
    }

    // Setup
    float defaultHdrScale = drawImguiDesc.hdrScale == 0.0f ? 1.0f : drawImguiDesc.hdrScale;

    m_iCore.CmdSetDescriptorPool(commandBuffer, *m_DescriptorPool);
    m_iCore.CmdSetPipelineLayout(commandBuffer, *m_PipelineLayout);
    m_iCore.CmdSetPipeline(commandBuffer, *pipeline);
    m_iCore.CmdSetIndexBuffer(commandBuffer, *m_CurrentBuffer, m_IbOffset, IndexType::UINT16);
    m_iCore.CmdSetDescriptorSet(commandBuffer, IMGUI_SAMPLER_SET, *m_DescriptorSet0_sampler, nullptr);

    VertexBufferDesc vertexBufferDesc = {};
    vertexBufferDesc.buffer = m_CurrentBuffer;
    vertexBufferDesc.offset = m_VbOffset;
    vertexBufferDesc.stride = sizeof(ImDrawVert);

    m_iCore.CmdSetVertexBuffers(commandBuffer, 0, &vertexBufferDesc, 1);

    Viewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = drawImguiDesc.displaySize.w;
    viewport.height = drawImguiDesc.displaySize.h;
    viewport.depthMin = 0.0f;
    viewport.depthMax = 1.0f;

    m_iCore.CmdSetViewports(commandBuffer, &viewport, 1);

    ImguiConstants constants = {};
    constants.hdrScale = defaultHdrScale;
    constants.invDisplayWidth = 1.0f / viewport.width;
    constants.invDisplayHeight = 1.0f / viewport.height;

    m_iCore.CmdSetRootConstants(commandBuffer, 0, &constants, sizeof(constants));

    // For each draw list
    Descriptor* currentDescriptor = nullptr;
    float currentHdrScale = -1.0f;
    float hdrScale = 0.0f;
    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;

    for (uint32_t n = 0; n < drawImguiDesc.drawListNum; n++) {
        const ImDrawList* drawList = drawImguiDesc.drawLists[n];

        // For each draw command
        for (int32_t i = 0; i < drawList->CmdBuffer.Size; i++) {
            const ImDrawCmd& drawCmd = drawList->CmdBuffer.Data[i];

            // Clipped?
            const ImVec4& clipRect = drawCmd.ClipRect; // min.x, min.y, max.x, max.y
            if (clipRect.z <= clipRect.x || clipRect.w <= clipRect.y)
                continue;

            if (drawCmd.UserCallback) {
                // Nothing to render, just update HDR scale
                hdrScale = *(float*)&drawCmd.UserCallbackData;
            } else {
                // Change HDR scale
                if (hdrScale != currentHdrScale) {
                    currentHdrScale = hdrScale;

                    constants.hdrScale = currentHdrScale == 0.0f ? defaultHdrScale : currentHdrScale;

                    m_iCore.CmdSetRootConstants(commandBuffer, 0, &constants, sizeof(constants));
                }

                // Change texture
                Descriptor* descriptor = nullptr;
                if (drawCmd.TexRef.TexData) {
                    // Font atlas texture
                    ImTextureID key = drawCmd.TexRef.TexData->TexID;
                    descriptor = m_Textures[key].descriptor;
                } else {
                    // User passed texture
                    descriptor = (Descriptor*)drawCmd.TexRef.TexID;
                }

                if (descriptor != currentDescriptor) {
                    currentDescriptor = descriptor;

                    DescriptorSet* descriptorSet = m_DescriptorSets1[m_DescriptorSetIndex];
                    m_DescriptorSetIndex = (m_DescriptorSetIndex + 1) % m_DescriptorSets1.size();

                    m_iCore.CmdSetDescriptorSet(commandBuffer, IMGUI_TEXTURE_SET, *descriptorSet, nullptr);

                    DescriptorRangeUpdateDesc descriptorRangeUpdateDesc = {&currentDescriptor, 1};
                    m_iCore.UpdateDescriptorRanges(*descriptorSet, 0, 1, &descriptorRangeUpdateDesc);
                }

                // Draw
                DrawIndexedDesc drawIndexedDesc = {};
                drawIndexedDesc.indexNum = drawCmd.ElemCount;
                drawIndexedDesc.instanceNum = 1;
                drawIndexedDesc.baseIndex = drawCmd.IdxOffset + indexOffset;
                drawIndexedDesc.baseVertex = drawCmd.VtxOffset + vertexOffset;

                Rect rect = {
                    (int16_t)clipRect.x,
                    (int16_t)clipRect.y,
                    (Dim_t)(clipRect.z - clipRect.x),
                    (Dim_t)(clipRect.w - clipRect.y),
                };

                m_iCore.CmdSetScissors(commandBuffer, &rect, 1);
                m_iCore.CmdDrawIndexed(commandBuffer, drawIndexedDesc);
            }
        }

        vertexOffset += drawList->VtxBuffer.Size;
        indexOffset += drawList->IdxBuffer.Size;
    }
}

#endif
