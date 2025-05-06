// © 2021 NVIDIA Corporation

PipelineVal::PipelineVal(DeviceVal& device, Pipeline* pipeline)
    : ObjectVal(device, pipeline) {
}

PipelineVal::PipelineVal(DeviceVal& device, Pipeline* pipeline, const GraphicsPipelineDesc& graphicsPipelineDesc)
    : ObjectVal(device, pipeline)
    , m_PipelineLayout(graphicsPipelineDesc.pipelineLayout) {
    m_WritesToDepth = graphicsPipelineDesc.outputMerger.depth.write;
    m_WritesToStencil = graphicsPipelineDesc.outputMerger.stencil.front.writeMask != 0 || graphicsPipelineDesc.outputMerger.stencil.back.writeMask != 0;
}

PipelineVal::PipelineVal(DeviceVal& device, Pipeline* pipeline, const ComputePipelineDesc& computePipelineDesc)
    : ObjectVal(device, pipeline)
    , m_PipelineLayout(computePipelineDesc.pipelineLayout) {
}

PipelineVal::PipelineVal(DeviceVal& device, Pipeline* pipeline, const RayTracingPipelineDesc& rayTracingPipelineDesc)
    : ObjectVal(device, pipeline)
    , m_PipelineLayout(rayTracingPipelineDesc.pipelineLayout) {
}

NRI_INLINE Result PipelineVal::WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* dst) {
    return GetRayTracingInterfaceImpl().WriteShaderGroupIdentifiers(*GetImpl(), baseShaderGroupIndex, shaderGroupNum, dst);
}
