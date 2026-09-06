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

NRI_INLINE Result PipelineVal::WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, uint32_t dstStride, void* dst) {
    const uint32_t identifierSize = m_Device.GetDesc().shaderStage.rayTracing.shaderGroupIdentifierSize;

    NRI_RETURN_ON_FAILURE(&m_Device, dstStride >= identifierSize, Result::INVALID_ARGUMENT, "'dstStride' is less than 'shaderStage.rayTracing.shaderGroupIdentifierSize'");

    return GetRayTracingInterfaceImpl().WriteShaderGroupIdentifiers(*GetImpl(), baseShaderGroupIndex, shaderGroupNum, dstStride, dst);
}
