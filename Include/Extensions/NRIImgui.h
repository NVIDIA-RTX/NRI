// © 2025 NVIDIA Corporation

#pragma once

#define NRI_IMGUI_H 1

// Expected usage:
// - designed only for rendering
// - compatible only with unmodified "ImDrawVert" (20 bytes) and "ImDrawIdx" (2 bytes)
// - "drawList->AddCallback" functionality is not supported! But there is a special callback, allowing to override "hdrScale":
//      drawList->AddCallback(NRI_IMGUI_OVERRIDE_HDR_SCALE(1000.0f)); // to override "DrawImguiDesc::hdrScale"
//      drawList->AddCallback(NRI_IMGUI_OVERRIDE_HDR_SCALE(0.0f));    // to revert back to "DrawImguiDesc::hdrScale"
// - "ImGui::Image*" functions are supported. "ImTextureID" must be a "SHADER_RESOURCE" descriptor:
//      ImGui::Image((ImTextureID)descriptor, ...)
// - only one "Imgui" instance is needed per device

NonNriForwardStruct(ImDrawList);

NriNamespaceBegin

NriForwardStruct(Imgui);
NriForwardStruct(Streamer);

NriStruct(ImguiDesc) {
    const uint8_t* fontAtlasData;               // use "GetTexDataAsRGBA32"
    Nri(Dim2) fontAtlasDims;                    // font texture atlas dimensions
    NriOptional uint32_t descriptorPoolSize;    // upper bound of textures used by Imgui for drawing:
                                                //      {number of frames in flight} * {number of "CmdDrawImgui" calls} * (1 + {"drawList->AddImage*" calls})
};

NriStruct(DrawImguiDesc) {
    const ImDrawList* const* drawLists;         // ImDrawData::CmdLists.Data
    uint32_t drawListNum;                       // ImDrawData::CmdLists.Size
    Nri(Dim2) displaySize;                      // ImDrawData::DisplaySize
    float hdrScale;                             // SDR intensity in HDR mode (1 by default)
    Nri(Format) attachmentFormat;               // destination attachment (render target) format
    bool linearColor;                           // apply de-gamma to vertex colors (needed for sRGB attachments and HDR)
};

// Threadsafe: yes
NriStruct(ImguiInterface) {
    Nri(Result)     (NRI_CALL *CreateImgui)     (NriRef(Device) device, const NriRef(ImguiDesc) imguiDesc, NriOut NriRef(Imgui*) imgui);
    void            (NRI_CALL *DestroyImgui)    (NriRef(Imgui) imgui);

    // Command buffer
    // {
            // Draw (changes descriptor pool, pipeline layout and pipeline, barriers are externally controlled)
            void    (NRI_CALL *CmdDrawImgui)    (NriRef(CommandBuffer) commandBuffer, NriRef(Imgui) imgui, NriRef(Streamer) streamer, const NriRef(DrawImguiDesc) drawImguiDesc);
    // }
};

NriNamespaceEnd

#define NRI_IMGUI_OVERRIDE_HDR_SCALE(hdrScale) (ImDrawCallback)1, _NriCastFloatToVoidPtr(hdrScale)

inline void* _NriCastFloatToVoidPtr(float f) {
    // A strange cast is there to get a fast path in Imgui
    return *(void**)&f;
}
