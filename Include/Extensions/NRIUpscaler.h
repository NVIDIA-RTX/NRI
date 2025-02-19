// © 2025 NVIDIA Corporation

#pragma once

NriNamespaceBegin

NriForwardStruct(Upscaler);

NriEnum(UpscalerType, uint8_t,  // Name                                     // Notes
    NIS,                        // NVIDIA Image Scaling                     sharpener-upscaler, cross vendor
    FSR,                        // AMD FidelityFX Super Resolution          upscaler, cross vendor
    DLSR,                       // NVIDIA Deep Learning Super Resolution    upscaler, NVIDIA only
    DLRR                        // NVIDIA Deep Learning Ray Reconstruction  upscaler-denoiser, NVIDIA only
);

NriEnum(UpscalerMode, uint8_t,  // Scaling factor       // Min jitter phases
    NATIVE,                     // 1.0x                 8
    QUALITY,                    // 1.5x                 18
    BALANCED,                   // 1.7x                 23
    PERFORMANCE,                // 2.0x                 32
    ULTRA_PERFORMANCE           // 3.0x                 72
);

NriBits(UpscalerBits, uint8_t,
    NONE                        = 0,
    HDR                         = NriBit(0),            // "input" uses colors in High-Dynamic Range
    NON_LINEAR                  = NriBit(1),            // "input" uses perceptual (gamma corrected) colors
    AUTO_EXPOSURE               = NriBit(2),            // automatic exposure ("exposure" texture is ignored)
    DEPTH_INVERTED              = NriBit(3),            // "depth" is inverted, i.e. the near plane is mapped to 1
    DEPTH_INFINITE              = NriBit(4),            // "depth" uses INF far plane
    DEPTH_LINEAR                = NriBit(5),            // "depth" is linear viewZ (HW otherwise)
    UPSCALE_RES_MV              = NriBit(6)             // motion vectors ("mv") are rendered at upscale resolution (not render)
);

NriBits(DispatchUpscaleBits, uint8_t,
    NONE                        = 0,
    RESET_HISTORY               = NriBit(0),            // restart accumulation
    USE_SPECULAR_MOTION         = NriBit(1)             // if set, "specularMvOrHitT" represents "specular motion" not "hit distance"
);

NriStruct(UpscalerDesc) {
    Nri(Dim2) upscaleResolution;                        // output resolution
    Nri(UpscalerType) type;
    Nri(UpscalerMode) mode;                             // not needed for "NIS"
    Nri(UpscalerBits) flags;
    NriOptional uint8_t preset;                         // "DLSR" and "DLRR" only (0 default, >1 presets A, B, C...)
    NriOptional NriPtr(CommandBuffer) commandBuffer;    // a non-copy-only command buffer in opened state, submission must be done manually ("wait for idle" executed, if not provided)
};

NriStruct(UpscalerProps) {
    float scalingFactor;                                // per dimension scaling factor
    float mipBias;                                      // mip bias for materials textures, computed as "-log2(scalingFactor) - 1" (keep an eye on normal maps)
    Nri(Dim2) upscaleResolution;                        // output resolution
    Nri(Dim2) renderResolution;                         // optimal render resolution
    Nri(Dim2) renderResolutionMin;                      // minimal render resolution (for Dynamic Resolution Scaling)
    uint8_t jitterPhaseNum;                             // minimal number of phases in the jitter sequence, computed as "ceil(8 * scalingFactor ^ 2)" ("Halton(2, 3)" recommended)
};

NriStruct(UpscalerResource) {
    NriPtr(Texture) texture;
    NriPtr(Descriptor) descriptor;                      // "SHADER_RESOURCE" or "SHADER_RESOURCE_STORAGE", see comments below
};

// Guide buffers
NriStruct(FSRGuides) {
    Nri(UpscalerResource) mv;                           // .xy - surface motion
    Nri(UpscalerResource) depth;                        // .x - HW depth
    NriOptional Nri(UpscalerResource) exposure;         // .x - 1x1 exposure
    NriOptional Nri(UpscalerResource) reactive;         // .x - bias towards "input"
};

NriStruct(DLSRGuides) {
    Nri(UpscalerResource) mv;                           // .xy - surface motion
    Nri(UpscalerResource) depth;                        // .x - HW or linear depth
    NriOptional Nri(UpscalerResource) exposure;         // .x - 1x1 exposure
    NriOptional Nri(UpscalerResource) reactive;         // .x - bias towards "input"
};

NriStruct(DLRRGuides) {
    Nri(UpscalerResource) mv;                           // .xy - surface motion
    Nri(UpscalerResource) depth;                        // .x - HW or linear depth
    Nri(UpscalerResource) normalRoughness;              // .xyz - world-space normal (not encoded), .w - linear roughness
    Nri(UpscalerResource) diffuseAlbedo;                // .xyz - diffuse albedo (LDR sky color for sky)
    Nri(UpscalerResource) specularAlbedo;               // .xyz - specular albedo (environment BRDF)
    Nri(UpscalerResource) specularMvOrHitT;             // .xy - specular virtual motion of the reflected world, or .x - specular hit distance otherwise
    NriOptional Nri(UpscalerResource) exposure;         // .x - 1x1 exposure
    NriOptional Nri(UpscalerResource) reactive;         // .x - bias towards "input"
    NriOptional Nri(UpscalerResource) sss;              // .x - subsurface scattering, computed as "Luminance(colorAfterSSS - colorBeforeSSS)"
};

// Settings
NriStruct(NISSettings) {
    float sharpness;                                    // [0; 1]
};

NriStruct(FSRSettings) {
    float zNear;                                        // distance to the near plane (units)
    float zFar;                                         // distance to the far plane, unused if "DEPTH_INFINITE" is set (units)
    float verticalFov;                                  // vertical field of view angle (radians)
    float frameTime;                                    // the time elapsed since the last frame (ms)
    float viewSpaceToMetersFactor;                      // for converting view space units to meters (m/unit)
    float sharpness;                                    // [0; 1]
};

NriStruct(DLRRSettings) {
    float worldToViewMatrix[16];                        // {Xx, Yx, Zx, 0, Xy, Yy, Zy, 0, Xz, Yz, Zz, 0, Tx, Ty, Tz, 1}, where {X, Y, Z} - axises, T - translation
    float viewToClipMatrix[16];                         // {-, -, -, 0, -, -, -, 0, -, -, -, A, -, -, -, B}, where {A; B} = {0; 1} for ortho or {-1/+1; 0} for perspective projections
};

NriStruct(DispatchUpscaleDesc) {
    // Output (required "SHADER_RESOURCE_STORAGE" for resource state & descriptor)
    Nri(UpscalerResource) output;                       // .xyz - upscaled RGB color

    // Input (required "SHADER_RESOURCE" for resource state & descriptor)
    Nri(UpscalerResource) input;                        // .xyz - input RGB color

    // Guides (required "SHADER_RESOURCE" for resource states & descriptors)
    union {                                             // Choosen based on "UpscalerType" passed during creation
        Nri(FSRGuides) fsr;                             //      "FSR" guides
        Nri(DLSRGuides) dlsr;                           //      "DLSR" guides
        Nri(DLRRGuides) dlrr;                           //      "DLRR" guides (sRGB not supported)
    } guides;

    // Settings
    union {                                             // Choosen based on "UpscalerType" passed during creation
        Nri(NISSettings) nis;                           //      "NIS" settings
        Nri(FSRSettings) fsr;                           //      "FSR" settings
        Nri(DLRRSettings) dlrr;                         //      "DLRR" settings
    } settings;

    Nri(Dim2) currentResolution;                        // current render resolution for inputs and guides, renderResolutionMin <= currentResolution <= renderResolution
    Nri(Float2) cameraJitter;                           // pointing towards the pixel center, in [-0.5; 0.5] range
    Nri(Float2) mvScale;                                // used to convert motion vectors to pixel space
    Nri(DispatchUpscaleBits) flags;
};

NriStruct(UpscalerInterface) {
    Nri(Result)     (NRI_CALL *CreateUpscaler)          (NriRef(Device) device, const NriRef(UpscalerDesc) upscalerDesc, NriOut NriRef(Upscaler*) upscaler);
    void            (NRI_CALL *DestroyUpscaler)         (NriRef(Upscaler) upscaler);

    bool            (NRI_CALL *IsUpscalerSupported)     (const NriRef(Device) device, Nri(UpscalerType) type);
    void            (NRI_CALL *GetUpscalerProps)        (const NriRef(Upscaler) upscaler, NriOut NriRef(UpscalerProps) upscalerProps);

    // Changes descriptor pool, pipeline layout and pipeline. Barriers are externally controlled
    void            (NRI_CALL *CmdDispatchUpscale)      (NriRef(CommandBuffer) commandBuffer, NriRef(Upscaler) upscaler, const NriRef(DispatchUpscaleDesc) dispatchUpscaleDesc);
};

NriNamespaceEnd