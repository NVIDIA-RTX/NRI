// © 2026 NVIDIA Corporation

Result MemoryWGPU::Create(const AllocateMemoryDesc& allocateMemoryDesc) {
    m_Desc = allocateMemoryDesc;

    return Result::SUCCESS;
}
