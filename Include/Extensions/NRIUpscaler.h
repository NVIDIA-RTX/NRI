// © 2025 NVIDIA Corporation

// Goal: providing easy-to-use access to modern upscalers: DLSS-SR, DLSS-RR, FSR, XeSS, NIS, MetalFX

#pragma once

#define NRI_UPSCALER_H 1

NriNamespaceBegin

NriForwardStruct(Upscaler);

NriEnum(UpscalerType, uint8_t,  // Name                                     // Notes
    NIS,                        // NVIDIA Image Scaling                     sharpener-upscaler, cross vendor
    FSR,                        // AMD FidelityFX Super Resolution          FSR 4 where supported, FSR 3 otherwise
    XESS,                       // INTEL XeSS Super Resolution              upscaler, cross vendor
    DLSR,                       // NVIDIA Deep Learning Super Resolution    upscaler, NVIDIA only
    DLRR,                       // NVIDIA Deep Learning Ray Reconstruction  upscaler-denoiser, NVIDIA only
    METALFX_SPATIAL,            // Apple MetalFX spatial upscaler           upscaler, Metal only
    METALFX_TEMPORAL,           // Apple MetalFX temporal upscaler          upscaler, Metal only
    METALFX_DENOISER            // Apple MetalFX upscaler-denoiser          upscaler-denoiser, Metal only
);

NriEnum(UpscalerMode, uint8_t,  // Scaling factor       // Min jitter phases (or just use unclamped Halton2D)
    NATIVE,                     // 1.0x                 8
    ULTRA_QUALITY,              // 1.3x                 14
    QUALITY,                    // 1.5x                 18
    BALANCED,                   // 1.7x                 23
    PERFORMANCE,                // 2.0x                 32
    ULTRA_PERFORMANCE           // 3.0x                 72
);

NriBits(UpscalerBits, uint16_t,
    NONE                        = 0,
    HDR                         = NriBit(0),            // "input" uses colors in High-Dynamic Range (HDR)
    SRGB                        = NriBit(1),            // "input" uses Low-Dynamic Range (LDR) colors in sRGB space
    USE_EXPOSURE                = NriBit(2),            // "exposure" texture is provided (automatic exposure otherwise)
    USE_REACTIVE                = NriBit(3),            // "reactive" texture is provided
    DEPTH_INVERTED              = NriBit(4),            // "depth" is inverted, i.e. the near plane is mapped to 1
    DEPTH_INFINITE              = NriBit(5),            // "depth" uses INF far plane
    DEPTH_LINEAR                = NriBit(6),            // "depth" is linear viewZ (HW otherwise) (unsupported by METALFX_TEMPORAL and METALFX_DENOISER)
    MV_UPSCALED                 = NriBit(7),            // "mv" are rendered at upscale resolution (METALFX_TEMPORAL requires macOS 27, unsupported by METALFX_DENOISER)
    MV_JITTERED                 = NriBit(8)             // "mv" include jitter (unsupported by METALFX_TEMPORAL and METALFX_DENOISER)
);

NriBits(DispatchUpscaleBits, uint8_t,
    NONE                        = 0,
    RESET_HISTORY               = NriBit(0),            // restart accumulation
    USE_SPECULAR_MOTION         = NriBit(1)             // ("DLRR" only) if set, "specularMvOrHitT" represents "specular motion" not "hit distance"
);

NriStruct(UpscalerDesc) {
    Nri(Dim2_t) upscaleResolution;                      // output resolution
    Nri(UpscalerType) type;
    Nri(UpscalerMode) mode;                             // selects the scaling factor reported by "GetUpscalerProps"
    Nri(UpscalerBits) flags;
    NriOptional uint8_t preset;                         // 0: default; DLSR/DLRR: NGX preset; XESS: SDK network model + 1
    NriOptional Nri(Format) outputFormat;               // needed only for NIS, if "shaderFeatures.storageWriteWithoutFormat" is unsupported
    NriOptional NriPtr(CommandBuffer) commandBuffer;    // DLSR/DLRR: graphics or compute command buffer in recording state; caller submits it (internal submission and wait otherwise)
};

NriStruct(UpscalerProps) {
    float scalingFactor;                                // scaling factor per dimension
    float mipBias;                                      // mip bias for material textures, computed as "-log2(scalingFactor) - 1" (keep an eye on normal maps)
    Nri(Dim2_t) upscaleResolution;                      // output resolution
    Nri(Dim2_t) renderResolution;                       // optimal render resolution
    Nri(Dim2_t) renderResolutionMin;                    // minimum render resolution (for dynamic resolution scaling)
    uint8_t jitterPhaseNum;                             // minimum number of phases in the jitter sequence, computed as "ceil(8 * scalingFactor ^ 2)" ("Halton(2, 3)" recommended)
};

NriStruct(UpscalerResource) {
    NriPtr(Texture) texture;
    NriPtr(Descriptor) descriptor;                      // "TEXTURE" or "STORAGE_TEXTURE" descriptor, see requirements below
};

// Guide textures
NriStruct(UpscalerGuides) {                             // For FSR, XESS, DLSR, METALFX_TEMPORAL
    Nri(UpscalerResource) mv;                           // .xy - surface motion
    Nri(UpscalerResource) depth;                        // .x - HW depth
    NriOptional Nri(UpscalerResource) exposure;         // .x - 1x1 exposure
    NriOptional Nri(UpscalerResource) reactive;         // .x - bias towards "input"
};

NriStruct(DenoiserGuides) {                             // For DLRR, METALFX_DENOISER
    Nri(UpscalerResource) mv;                           // .xy - surface motion
    Nri(UpscalerResource) depth;                        // .x - hardware depth (DLRR also supports linear view-space Z via "DEPTH_LINEAR")
    Nri(UpscalerResource) normalRoughness;              // .xyz - world-space normal (not encoded), .w - linear roughness
    Nri(UpscalerResource) diffuseAlbedo;                // .xyz - diffuse albedo (LDR sky color for sky)
    Nri(UpscalerResource) specularAlbedo;               // .xyz - specular albedo (environment BRDF)
    Nri(UpscalerResource) specularMvOrHitT;             // .xy - specular virtual motion when "USE_SPECULAR_MOTION" is set; .x - specular hit distance otherwise (METALFX_DENOISER requires hit distance)
    NriOptional Nri(UpscalerResource) exposure;         // .x - 1x1 exposure
    NriOptional Nri(UpscalerResource) reactive;         // .x - bias towards "input"

    // DLRR only
    NriOptional Nri(UpscalerResource) preTransparency;  // .xyz - color before transparency (aka transparency guide)
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
    float worldToViewMatrix[16];                        // {Xx, Yx, Zx, 0, Xy, Yy, Zy, 0, Xz, Yz, Zz, 0, Tx, Ty, Tz, 1}, where {X, Y, Z} - axes, T - translation
    float viewToClipMatrix[16];                         // {-, -, -, 0, -, -, -, 0, -, -, -, A, -, -, -, B}, where {A; B} = {0; 1} for ortho or {-1/+1; 0} for perspective projections
};

NriStruct(DispatchUpscaleDesc) {
    // Output (requires "SHADER_RESOURCE_STORAGE" state and a "STORAGE_TEXTURE" descriptor)
    // - MetalFX: requires "COLOR_ATTACHMENT" usage
    Nri(UpscalerResource) output;                       // .xyz - upscaled RGB color

    // Input (requires "SHADER_RESOURCE" state and a "TEXTURE" descriptor)
    Nri(UpscalerResource) input;                        // .xyz - input RGB color

    // Guides (require "SHADER_RESOURCE" states and "TEXTURE" descriptors)
    union {                                             // Chosen based on "UpscalerType" passed during creation
        Nri(UpscalerGuides) upscaler;                   //      FSR, XESS, DLSR, METALFX_TEMPORAL
        Nri(DenoiserGuides) denoiser;                   //      DLRR, METALFX_DENOISER (sRGB not supported)
    } guides;

    // Settings
    union {                                             // Chosen based on "UpscalerType" passed during creation
        Nri(NISSettings) nis;                           //      NIS settings
        Nri(FSRSettings) fsr;                           //      FSR settings
        Nri(DLRRSettings) dlrr;                         //      DLRR and METALFX_DENOISER settings
    } settings;

    Nri(Dim2_t) currentResolution;                      // active input resolution, renderResolutionMin <= currentResolution <= renderResolution
    Nri(Float2_t) cameraJitter;                         // jitter offset in pixels, towards the pixel center, each component in [-0.5; 0.5]
    Nri(Float2_t) mvScale;                              // component-wise multiplier converting motion vectors to pixel space
    NriOptional float preExposure;                      // multiplier already applied to the "input" color, 0 means 1 (FSR, XESS, DLSR, DLRR, METALFX_TEMPORAL, METALFX_DENOISER)
    Nri(DispatchUpscaleBits) flags;
};

// Threadsafe: yes
NriStruct(UpscalerInterface) {
    Nri(Result)     (NRI_CALL *CreateUpscaler)          (NriRef(Device) device, const NriRef(UpscalerDesc) upscalerDesc, NriOut NriRef(Upscaler*) upscaler);
    void            (NRI_CALL *DestroyUpscaler)         (NriPtr(Upscaler) upscaler);

    bool            (NRI_CALL *IsUpscalerSupported)     (const NriRef(Device) device, Nri(UpscalerType) type);
    void            (NRI_CALL *GetUpscalerProps)        (const NriRef(Upscaler) upscaler, NriOut NriRef(UpscalerProps) upscalerProps);

    // Command buffer
    // {
        // Dispatch (changes descriptor pool, pipeline layout and pipeline, barriers are externally controlled)
        // MetalFX:
        // - Native creation occurs on the first dispatch and may cause a hitch
        // - Resource dimensions and formats must remain unchanged after the first dispatch
        void        (NRI_CALL *CmdDispatchUpscale)      (NriRef(CommandBuffer) commandBuffer, NriRef(Upscaler) upscaler, const NriRef(DispatchUpscaleDesc) dispatchUpscaleDesc);
    // }
};

NriNamespaceEnd