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

// Copied from Imgui
typedef uint16_t ImDrawIdx;

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

struct ImDrawCmd {
    ImVec4 ClipRect;
    uint64_t TextureId;
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
constexpr Format FONT_FORMAT = Format::RGBA8_UNORM; // TODO: add A8_UNORM?

ImguiImpl::~ImguiImpl() {
    for (const PipelineAndProps& entry : m_Pipelines)
        m_iCore.DestroyPipeline(*entry.pipeline);

    m_iCore.DestroyPipelineLayout(*m_PipelineLayout);
    m_iCore.DestroyDescriptorPool(*m_DescriptorPool);
    m_iCore.DestroyDescriptor(*m_Sampler);
    m_iCore.DestroyDescriptor(*m_FontDescriptor);
    m_iCore.DestroyTexture(*m_FontTexture);
}

Result ImguiImpl::Create(const ImguiDesc& imguiDesc) {
    { // Get streamer interface
        Result result = nriGetInterface(m_Device, NRI_INTERFACE(StreamerInterface), &m_iStreamer);
        if (result != Result::SUCCESS)
            return result;
    }

    { // Create font texture
        ResourceAllocatorInterface iResourceAllocator = {};
        Result result = nriGetInterface(m_Device, NRI_INTERFACE(ResourceAllocatorInterface), &iResourceAllocator);
        if (result != Result::SUCCESS)
            return result;

        AllocateTextureDesc textureDesc = {};
        textureDesc.desc.type = TextureType::TEXTURE_2D;
        textureDesc.desc.usage = TextureUsageBits::SHADER_RESOURCE;
        textureDesc.desc.format = FONT_FORMAT;
        textureDesc.desc.width = imguiDesc.fontAtlasDims.w;
        textureDesc.desc.height = imguiDesc.fontAtlasDims.h;
        textureDesc.memoryLocation = MemoryLocation::DEVICE;

        result = iResourceAllocator.AllocateTexture(m_Device, textureDesc, m_FontTexture);
        if (result != Result::SUCCESS)
            return result;
    }

    { // Create font descriptor
        Texture2DViewDesc viewDesc = {};
        viewDesc.texture = m_FontTexture;
        viewDesc.viewType = Texture2DViewType::SHADER_RESOURCE_2D;
        viewDesc.format = FONT_FORMAT;

        Result result = m_iCore.CreateTexture2DView(viewDesc, m_FontDescriptor);
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

    { // Upload data // TODO: allow to merge with "UploadData" requests on user side
        HelperInterface iHelper = {};
        Result result = nriGetInterface(m_Device, NRI_INTERFACE(HelperInterface), &iHelper);
        if (result != Result::SUCCESS)
            return result;

        Queue* graphicsQueue = nullptr;
        result = m_iCore.GetQueue(m_Device, QueueType::GRAPHICS, 0, graphicsQueue);
        if (result != Result::SUCCESS)
            return result;

        const FormatProps& formatProps = GetFormatProps(FONT_FORMAT);

        TextureSubresourceUploadDesc subresource = {};
        subresource.slices = imguiDesc.fontAtlasData;
        subresource.sliceNum = 1;
        subresource.rowPitch = imguiDesc.fontAtlasDims.w * formatProps.stride;
        subresource.slicePitch = imguiDesc.fontAtlasDims.w * imguiDesc.fontAtlasDims.h * formatProps.stride;

        TextureUploadDesc textureUploadDesc = {};
        textureUploadDesc.subresources = &subresource;
        textureUploadDesc.texture = m_FontTexture;
        textureUploadDesc.after = {AccessBits::SHADER_RESOURCE, Layout::SHADER_RESOURCE};

        result = iHelper.UploadData(*graphicsQueue, &textureUploadDesc, 1, nullptr, 0);
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

void ImguiImpl::CmdDraw(CommandBuffer& commandBuffer, Streamer& streamer, const DrawImguiDesc& drawImguiDesc) {
    if (!drawImguiDesc.drawListNum)
        return;

    // Pipeline
    Pipeline* pipeline = nullptr;
    {
        ExclusiveScope lock(m_Lock);

        for (size_t i = 0; i < m_Pipelines.size(); i++) {
            const PipelineAndProps& pipelineAndProps = m_Pipelines[i];
            if (pipelineAndProps.format == drawImguiDesc.attachmentFormat && pipelineAndProps.linearColor == drawImguiDesc.linearColor) {
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
            colorAttachment.colorBlend.func = BlendFunc::ADD,
            colorAttachment.alphaBlend.srcFactor = BlendFactor::ONE_MINUS_SRC_ALPHA,
            colorAttachment.alphaBlend.dstFactor = BlendFactor::ZERO,
            colorAttachment.alphaBlend.func = BlendFunc::ADD,
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
    }

    { // Rendering
        // Stream data
        uint32_t dataChunkNum = drawImguiDesc.drawListNum * 2;
        Scratch<DataSize> dataChunks = AllocateScratch((DeviceBase&)m_Device, DataSize, dataChunkNum);

        StreamBufferDataDesc streamBufferDataDesc = {};
        streamBufferDataDesc.dataChunkNum = dataChunkNum;
        streamBufferDataDesc.dataChunks = dataChunks;
        streamBufferDataDesc.placementAlignment = 4;

        uint64_t totalVertexDataSize = 0;
        for (uint32_t n = 0; n < drawImguiDesc.drawListNum; n++) {
            const ImDrawList* drawList = drawImguiDesc.drawLists[n];

            DataSize& vertexDataChunk = dataChunks[n];
            vertexDataChunk.data = drawList->VtxBuffer.Data;
            vertexDataChunk.size = drawList->VtxBuffer.Size * sizeof(ImDrawVert);

            DataSize& indexDataChunk = dataChunks[drawImguiDesc.drawListNum + n];
            indexDataChunk.data = drawList->IdxBuffer.Data;
            indexDataChunk.size = drawList->IdxBuffer.Size * sizeof(ImDrawIdx);

            totalVertexDataSize += vertexDataChunk.size;
        }

        BufferOffset bufferOffset = m_iStreamer.StreamBufferData(streamer, streamBufferDataDesc);
        uint64_t vbOffset = bufferOffset.offset;
        uint64_t ibOffset = vbOffset + totalVertexDataSize;

        // Setup
        float defaultHdrScale = drawImguiDesc.hdrScale == 0.0f ? 1.0f : drawImguiDesc.hdrScale;

        m_iCore.CmdSetDescriptorPool(commandBuffer, *m_DescriptorPool);
        m_iCore.CmdSetPipelineLayout(commandBuffer, *m_PipelineLayout);
        m_iCore.CmdSetPipeline(commandBuffer, *pipeline);
        m_iCore.CmdSetIndexBuffer(commandBuffer, *bufferOffset.buffer, ibOffset, IndexType::UINT16);
        m_iCore.CmdSetDescriptorSet(commandBuffer, IMGUI_SAMPLER_SET, *m_DescriptorSet0_sampler, nullptr);

        VertexBufferDesc vertexBufferDesc = {};
        vertexBufferDesc.buffer = bufferOffset.buffer;
        vertexBufferDesc.offset = vbOffset;
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
        Descriptor* currentTexture = nullptr;
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
                    Descriptor* textureId = drawCmd.TextureId ? (Descriptor*)drawCmd.TextureId : m_FontDescriptor;
                    if (textureId != currentTexture) {
                        currentTexture = textureId;

                        DescriptorSet* descriptorSet = m_DescriptorSets1[m_DescriptorSetIndex];
                        m_DescriptorSetIndex = (m_DescriptorSetIndex + 1) % m_DescriptorSets1.size();

                        m_iCore.CmdSetDescriptorSet(commandBuffer, IMGUI_TEXTURE_SET, *descriptorSet, nullptr);

                        DescriptorRangeUpdateDesc descriptorRangeUpdateDesc = {&currentTexture, 1};
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
}

#endif
