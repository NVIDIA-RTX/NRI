// © 2026 NVIDIA Corporation

PipelineCacheVal::PipelineCacheVal(DeviceVal& device, PipelineCache* pipelineCache)
    : ObjectVal(device, pipelineCache) {
}

NRI_INLINE Result PipelineCacheVal::GetData(void* dst, uint64_t& size) {
    return GetCoreInterfaceImpl().GetPipelineCacheData(*GetImpl(), dst, size);
}
