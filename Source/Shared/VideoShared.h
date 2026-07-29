// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

inline uint32_t GetVideoDecodeSetupSlot(const VideoDecodeDesc& desc) {
    const VideoH264DecodePictureDesc* h264PictureDesc = desc.h264PictureDesc;

    if (h264PictureDesc && h264PictureDesc->hasReferenceSlot)
        return h264PictureDesc->referenceSlot;

    return desc.dstSlot;
}

} // namespace nri
