// © 2026 NVIDIA Corporation

Result MemoryWGPU::Create(const AllocateMemoryDesc& allocateMemoryDesc) {
    // TODO: WebGPU owns resource memory allocation. This object only preserves NRI bookkeeping semantics.
    m_Desc = allocateMemoryDesc;

    return Result::SUCCESS;
}
