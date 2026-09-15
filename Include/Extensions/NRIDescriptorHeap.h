// © 2026 NVIDIA Corporation

// Goal: true bindless resource access through directly indexed descriptor heaps
// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html
// https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_descriptor_heap.html

#pragma once

#define NRI_DESCRIPTOR_HEAP_H 1

NriNamespaceBegin

NriForwardStruct(DescriptorHeap);       // layout-free directly indexed descriptor heap

// At least one capacity must be non-zero
NriStruct(DescriptorHeapDesc) {
    uint32_t resourceDescriptorNum;     // 0 - no resource heap
    uint32_t samplerDescriptorNum;      // 0 - no sampler heap
};

NriStruct(WriteResourceDescriptorsDesc) {
    const NriPtr(Descriptor) resource;  // any descriptor except "MUTABLE", "INPUT_ATTACHMENT" and "SAMPLER"
    uint32_t descriptorIndex;
};

NriStruct(WriteSamplerDescriptorsDesc) {
    const NriPtr(Descriptor) sampler;   // must be a SAMPLER
    uint32_t descriptorIndex;
};

// Threadsafe: yes, except writes to overlapping descriptor indices require external synchronization
NriStruct(DescriptorHeapInterface) {
    // Create
    Nri(Result)     (NRI_CALL *CreateDescriptorHeap)            (NriRef(Device) device, const NriRef(DescriptorHeapDesc) descriptorHeapDesc, NriOut NriRef(DescriptorHeap*) descriptorHeap);

    // Destroy
    void            (NRI_CALL *DestroyDescriptorHeap)           (NriPtr(DescriptorHeap) descriptorHeap);

    // Updates (on HOST)
    // GPU-side updates are not supported because D3D12 has no equivalent functionality
    Nri(Result)     (NRI_CALL *WriteResourceDescriptors)        (NriRef(DescriptorHeap) descriptorHeap, const NriPtr(WriteResourceDescriptorsDesc) writeDescs, uint32_t writeDescNum);
    Nri(Result)     (NRI_CALL *WriteSamplerDescriptors)         (NriRef(DescriptorHeap) descriptorHeap, const NriPtr(WriteSamplerDescriptorsDesc) writeDescs, uint32_t writeDescNum);

    // Command buffer
    // {
        void            (NRI_CALL *CmdSetDescriptorHeap)        (NriRef(CommandBuffer) commandBuffer, const NriRef(DescriptorHeap) descriptorHeap);
    // }
};

NriNamespaceEnd
