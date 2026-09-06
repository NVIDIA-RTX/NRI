// © 2025 NVIDIA Corporation

// Goal: ImGui rendering

#pragma once

#define NRI_IMGUI_H 1

/*
Requirements:
- ImGui 1.92.9+ with "ImGuiBackendFlags_RendererHasTextures" flag ("IMGUI_DISABLE_OBSOLETE_FUNCTIONS" is recommended),
- unmodified "ImDrawVert" (20 bytes) and "ImDrawIdx" (2 bytes)
- expected "ImTextureData" is 96 bytes
- "ImTextureID_Invalid" = 0

Expected usage:
- the goal of this extension is to support latest ImGui only
- designed only for rendering
- "ImguiRenderData" must be used before "EndStreamerFrame" and before the associated "Streamer" host-data ring buffer wraps
- "StreamerDesc::hostDataCapacity" must accommodate all simultaneously live host-data allocations and alignment padding
- all data referenced by "CopyImguiDataDesc" must remain valid and immutable until "CmdCopyImguiData" returns
- "drawList->AddCallback" functionality is not supported! But there is a special callback, allowing to override "hdrScale":
     drawList->AddCallback(NRI_IMGUI_OVERRIDE_HDR_SCALE(1000.0f)); // to override "DrawImguiDesc::hdrScale"
     drawList->AddCallback(NRI_IMGUI_OVERRIDE_HDR_SCALE(0.0f));    // to revert back to "DrawImguiDesc::hdrScale"
- "ImGui::Image*" functions are supported. "ImTextureID" must be a "SHADER_RESOURCE" descriptor:
     ImGui::Image((ImTextureID)descriptor, ...)

Single-threaded usage:
- configure "StreamerDesc::hostDataCapacity", call "CmdCopyImguiData" and pass the returned "ImguiRenderData" to "CmdDrawImgui"
- record all draws before "EndStreamerFrame" and before the associated "Streamer" host-data ring buffer wraps

Multi-threaded usage:
- one "nri::Imgui" instance may be used to render several "ImGui" contexts from multiple threads; calls are internally serialized
- the copy/draw handoff stores no instance-shared transient state: each copy returns independent "ImguiRenderData"
- each thread must use its own command buffer and returned "ImguiRenderData"; a shared "Streamer" must follow its multi-threaded usage contract
- different "Imgui" instances may share "ImTextureData"; access is internally synchronized, but referenced input must remain immutable until every concurrent copy returns
*/

NonNriForwardStruct(ImDrawList);
NonNriForwardStruct(ImTextureData);

NriNamespaceBegin

NriForwardStruct(Imgui);
NriForwardStruct(Streamer);

NriStruct(ImguiDesc) {
    NriOptional uint32_t descriptorPoolSize;    // upper bound of textures used by Imgui for drawing across all threads: {number of queued frames} * {number of "CmdDrawImgui" calls} * (1 + {"drawList->AddImage*" calls})
};

NriStruct(CopyImguiDataDesc) {
    const ImDrawList* const* drawLists;         // ImDrawData::CmdLists.Data
    uint32_t drawListNum;                       // ImDrawData::CmdLists.Size
    ImTextureData* const* textures;             // ImDrawData::Textures->Data (same as "ImGui::GetPlatformIO().Textures.Data")
    uint32_t textureNum;                        // ImDrawData::Textures->Size (same as "ImGui::GetPlatformIO().Textures.Size")
};

NriStruct(DrawImguiDesc) {
    Nri(Dim2_t) displaySize;                    // ImDrawData::DisplaySize
    float hdrScale;                             // SDR intensity in HDR mode (1 by default)
    Nri(Format) attachmentFormat;               // destination attachment (render target) format
    bool linearColor;                           // apply de-gamma to vertex colors (needed for sRGB attachments and HDR)
};

// Read-only input for "CmdDrawImgui"
NriStruct(ImguiRenderData) {
    NriPtr(Imgui) imgui;                        // producing instance
    const void* drawCommands;                   // in transient Streamer host data
    Nri(BufferOffset) vertices;                 // in transient Streamer buffer
    uint64_t indexBufferOffset;                 // in "vertices.buffer"
    uint32_t drawCmdNum;
};

// Threadsafe: yes, if command buffers are externally synchronized and Streamer thread safety is enabled (or provided externally).
// Access to shared "ImTextureData" is internally synchronized.
NriStruct(ImguiInterface) {
    Nri(Result) (NRI_CALL *CreateImgui)         (NriRef(Device) device, const NriRef(ImguiDesc) imguiDesc, NriOut NriRef(Imgui*) imgui);
    void        (NRI_CALL *DestroyImgui)        (NriPtr(Imgui) imgui);

    // Command buffer
    // {
        // Copy
        void    (NRI_CALL *CmdCopyImguiData)    (NriRef(CommandBuffer) commandBuffer, NriRef(Streamer) streamer, NriRef(Imgui) imgui, const NriRef(CopyImguiDataDesc) copyImguiDataDesc, NriOut NriRef(ImguiRenderData) imguiRenderData);

        // Draw (changes descriptor pool, pipeline layout and pipeline, barriers are externally controlled)
        void    (NRI_CALL *CmdDrawImgui)        (NriRef(CommandBuffer) commandBuffer, const NriRef(ImguiRenderData) imguiRenderData, const NriRef(DrawImguiDesc) drawImguiDesc);
    // }
};

NriNamespaceEnd

#define NRI_IMGUI_OVERRIDE_HDR_SCALE(hdrScale) (ImDrawCallback)1, _NriCastFloatToVoidPtr(hdrScale)

inline void* _NriCastFloatToVoidPtr(float f) {
    // A strange cast is there to get a fast path in Imgui
    return *(void**)&f;
}
