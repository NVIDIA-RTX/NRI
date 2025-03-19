// © 2021 NVIDIA Corporation

#pragma once

#define VULKAN_FUNCTION(name) PFN_vk##name name

struct DispatchTable {
    //==========================================================================
    // Instance
    //==========================================================================
    VULKAN_FUNCTION(GetInstanceProcAddr);
    VULKAN_FUNCTION(CreateInstance);
    VULKAN_FUNCTION(EnumerateInstanceExtensionProperties);
    VULKAN_FUNCTION(EnumerateInstanceLayerProperties);

    VULKAN_FUNCTION(GetDeviceProcAddr);
    VULKAN_FUNCTION(DestroyInstance);
    VULKAN_FUNCTION(DestroyDevice);
    VULKAN_FUNCTION(GetPhysicalDeviceMemoryProperties2);
    VULKAN_FUNCTION(GetDeviceGroupPeerMemoryFeatures);
    VULKAN_FUNCTION(GetPhysicalDeviceFormatProperties2);
    VULKAN_FUNCTION(CreateDevice);
    VULKAN_FUNCTION(GetDeviceQueue2);
    VULKAN_FUNCTION(EnumeratePhysicalDeviceGroups);
    VULKAN_FUNCTION(GetPhysicalDeviceProperties2);
    VULKAN_FUNCTION(GetPhysicalDeviceFeatures2);
    VULKAN_FUNCTION(GetPhysicalDeviceQueueFamilyProperties2);
    VULKAN_FUNCTION(EnumerateDeviceExtensionProperties);

    // VK_KHR_get_surface_capabilities2
    VULKAN_FUNCTION(GetPhysicalDeviceSurfaceFormats2KHR);
    VULKAN_FUNCTION(GetPhysicalDeviceSurfaceCapabilities2KHR);

    // VK_KHR_surface
    VULKAN_FUNCTION(GetPhysicalDeviceSurfaceSupportKHR);
    VULKAN_FUNCTION(GetPhysicalDeviceSurfacePresentModesKHR);
    VULKAN_FUNCTION(DestroySurfaceKHR);

#if VK_USE_PLATFORM_WIN32_KHR
    VULKAN_FUNCTION(CreateWin32SurfaceKHR);
    VULKAN_FUNCTION(GetMemoryWin32HandlePropertiesKHR);
#endif
#if VK_USE_PLATFORM_METAL_EXT
    VULKAN_FUNCTION(CreateMetalSurfaceEXT);
#endif
#if VK_USE_PLATFORM_XLIB_KHR
    VULKAN_FUNCTION(CreateXlibSurfaceKHR);
#endif
#if VK_USE_PLATFORM_WAYLAND_KHR
    VULKAN_FUNCTION(CreateWaylandSurfaceKHR);
#endif

    // VK_EXT_debug_utils
    VULKAN_FUNCTION(SetDebugUtilsObjectNameEXT);
    VULKAN_FUNCTION(CmdBeginDebugUtilsLabelEXT);
    VULKAN_FUNCTION(CmdEndDebugUtilsLabelEXT);
    VULKAN_FUNCTION(CmdInsertDebugUtilsLabelEXT);
    VULKAN_FUNCTION(QueueBeginDebugUtilsLabelEXT);
    VULKAN_FUNCTION(QueueEndDebugUtilsLabelEXT);
    VULKAN_FUNCTION(QueueInsertDebugUtilsLabelEXT);

    //==========================================================================
    // Device                                  Thread safety | Accounted
    //==========================================================================
    VULKAN_FUNCTION(CreateBuffer);                      // + | +
    VULKAN_FUNCTION(CreateImage);                       // + | +
    VULKAN_FUNCTION(CreateBufferView);                  // + | +
    VULKAN_FUNCTION(CreateImageView);                   // + | +
    VULKAN_FUNCTION(CreateSampler);                     // + | +
    VULKAN_FUNCTION(CreateQueryPool);                   // + | +
    VULKAN_FUNCTION(CreateCommandPool);                 // + | +
    VULKAN_FUNCTION(CreateSemaphore);                   // + | +
    VULKAN_FUNCTION(CreateDescriptorPool);              // + | +
    VULKAN_FUNCTION(CreatePipelineLayout);              // + | +
    VULKAN_FUNCTION(CreateDescriptorSetLayout);         // + | +
    VULKAN_FUNCTION(CreateShaderModule);                // + | +
    VULKAN_FUNCTION(CreateGraphicsPipelines);           // + | +
    VULKAN_FUNCTION(CreateComputePipelines);            // + | +
    VULKAN_FUNCTION(AllocateMemory);                    // + | +

    VULKAN_FUNCTION(DestroyBuffer);                     // - | +
    VULKAN_FUNCTION(DestroyImage);                      // - | +
    VULKAN_FUNCTION(DestroyBufferView);                 // - | +
    VULKAN_FUNCTION(DestroyImageView);                  // - | +
    VULKAN_FUNCTION(DestroySampler);                    // - | +
    VULKAN_FUNCTION(DestroyFramebuffer);                // - | +
    VULKAN_FUNCTION(DestroyQueryPool);                  // - | +
    VULKAN_FUNCTION(DestroyCommandPool);                // - | +
    VULKAN_FUNCTION(DestroySemaphore);                  // - | +
    VULKAN_FUNCTION(DestroyDescriptorPool);             // - | +
    VULKAN_FUNCTION(DestroyPipelineLayout);             // - | +
    VULKAN_FUNCTION(DestroyDescriptorSetLayout);        // - | +
    VULKAN_FUNCTION(DestroyShaderModule);               // - | +
    VULKAN_FUNCTION(DestroyPipeline);                   // - | +
    VULKAN_FUNCTION(FreeMemory);                        // - | +
    VULKAN_FUNCTION(FreeCommandBuffers);                // - | +
    VULKAN_FUNCTION(FreeDescriptorSets);                // - | - TODO: unused, add "FreeDescriptorSet"?

    VULKAN_FUNCTION(MapMemory);                         // - | + TODO: replace with 2 (VK_KHR_map_memory2 or VK 1.4)
    VULKAN_FUNCTION(FlushMappedMemoryRanges);           // + | +
    VULKAN_FUNCTION(QueueWaitIdle);                     // - | +
    VULKAN_FUNCTION(QueueSubmit2);                      // - | +
    VULKAN_FUNCTION(GetSemaphoreCounterValue);          // + | +
    VULKAN_FUNCTION(WaitSemaphores);                    // + | +
    VULKAN_FUNCTION(ResetCommandPool);                  // - | +
    VULKAN_FUNCTION(ResetDescriptorPool);               // - | +
    VULKAN_FUNCTION(AllocateCommandBuffers);            // - | +
    VULKAN_FUNCTION(AllocateDescriptorSets);            // - | +
    VULKAN_FUNCTION(UpdateDescriptorSets);              // + | +
    VULKAN_FUNCTION(BindBufferMemory2);                 // + | +
    VULKAN_FUNCTION(BindImageMemory2);                  // + | +
    VULKAN_FUNCTION(GetBufferMemoryRequirements2);      // + | +
    VULKAN_FUNCTION(GetImageMemoryRequirements2);       // + | +
    VULKAN_FUNCTION(GetDeviceBufferMemoryRequirements); // + | +
    VULKAN_FUNCTION(GetDeviceImageMemoryRequirements);  // + | +
    VULKAN_FUNCTION(ResetQueryPool);                    // + | +
    VULKAN_FUNCTION(GetBufferDeviceAddress);            // + | +

    VULKAN_FUNCTION(BeginCommandBuffer);                // - | +
    VULKAN_FUNCTION(CmdSetViewportWithCount);           // - | +
    VULKAN_FUNCTION(CmdSetScissorWithCount);            // - | +
    VULKAN_FUNCTION(CmdSetDepthBounds);                 // - | +
    VULKAN_FUNCTION(CmdSetStencilReference);            // - | +
    VULKAN_FUNCTION(CmdSetBlendConstants);              // - | +
    VULKAN_FUNCTION(CmdSetDepthBias);                   // - | + TODO: replace with 2 (VK_EXT_depth_bias_control)
    VULKAN_FUNCTION(CmdClearAttachments);               // - | +
    VULKAN_FUNCTION(CmdClearColorImage);                // - | +
    VULKAN_FUNCTION(CmdBindVertexBuffers2);             // - | +
    VULKAN_FUNCTION(CmdBindIndexBuffer);                // - | + TODO: remove after migration to v1.4
    VULKAN_FUNCTION(CmdBindPipeline);                   // - | +
    VULKAN_FUNCTION(CmdBindDescriptorSets);             // - | + TODO: replace with 2 (VK_KHR_maintenance6 or VK 1.4)
    VULKAN_FUNCTION(CmdPushConstants);                  // - | + TODO: replace with 2 (VK_KHR_maintenance6 or VK 1.4)
    VULKAN_FUNCTION(CmdDispatch);                       // - | +
    VULKAN_FUNCTION(CmdDispatchIndirect);               // - | +
    VULKAN_FUNCTION(CmdDraw);                           // - | +
    VULKAN_FUNCTION(CmdDrawIndexed);                    // - | +
    VULKAN_FUNCTION(CmdDrawIndirect);                   // - | +
    VULKAN_FUNCTION(CmdDrawIndexedIndirect);            // - | +
    VULKAN_FUNCTION(CmdDrawIndirectCount);              // - | +
    VULKAN_FUNCTION(CmdDrawIndexedIndirectCount);       // - | +
    VULKAN_FUNCTION(CmdCopyBuffer2);                    // - | +
    VULKAN_FUNCTION(CmdCopyImage2);                     // - | +
    VULKAN_FUNCTION(CmdResolveImage2);                  // - | +
    VULKAN_FUNCTION(CmdCopyBufferToImage2);             // - | +
    VULKAN_FUNCTION(CmdCopyImageToBuffer2);             // - | +
    VULKAN_FUNCTION(CmdPipelineBarrier2);               // - | +
    VULKAN_FUNCTION(CmdBeginQuery);                     // - | +
    VULKAN_FUNCTION(CmdEndQuery);                       // - | +
    VULKAN_FUNCTION(CmdWriteTimestamp2);                // - | +
    VULKAN_FUNCTION(CmdCopyQueryPoolResults);           // - | +
    VULKAN_FUNCTION(CmdResetQueryPool);                 // - | +
    VULKAN_FUNCTION(CmdFillBuffer);                     // - | +
    VULKAN_FUNCTION(CmdBeginRendering);                 // - | +
    VULKAN_FUNCTION(CmdEndRendering);                   // - | +
    VULKAN_FUNCTION(EndCommandBuffer);                  // - | +

    // VK_KHR_maintenance5
    VULKAN_FUNCTION(CmdBindIndexBuffer2KHR);

    // VK_KHR_push_descriptor
    VULKAN_FUNCTION(CmdPushDescriptorSetKHR);

    // VK_KHR_fragment_shading_rate
    VULKAN_FUNCTION(CmdSetFragmentShadingRateKHR);

    // VK_KHR_swapchain
    VULKAN_FUNCTION(AcquireNextImageKHR);
    VULKAN_FUNCTION(QueuePresentKHR);
    VULKAN_FUNCTION(CreateSwapchainKHR);
    VULKAN_FUNCTION(DestroySwapchainKHR);
    VULKAN_FUNCTION(GetSwapchainImagesKHR);

    // VK_KHR_present_wait
    VULKAN_FUNCTION(WaitForPresentKHR);

    // VK_KHR_acceleration_structure
    VULKAN_FUNCTION(CreateAccelerationStructureKHR);
    VULKAN_FUNCTION(DestroyAccelerationStructureKHR);
    VULKAN_FUNCTION(GetAccelerationStructureDeviceAddressKHR);
    VULKAN_FUNCTION(GetAccelerationStructureBuildSizesKHR);
    VULKAN_FUNCTION(CmdBuildAccelerationStructuresKHR);
    VULKAN_FUNCTION(CmdCopyAccelerationStructureKHR);
    VULKAN_FUNCTION(CmdWriteAccelerationStructuresPropertiesKHR);

    // VK_KHR_ray_tracing_pipeline
    VULKAN_FUNCTION(CreateRayTracingPipelinesKHR);
    VULKAN_FUNCTION(GetRayTracingShaderGroupHandlesKHR);
    VULKAN_FUNCTION(CmdTraceRaysKHR);
    VULKAN_FUNCTION(CmdTraceRaysIndirect2KHR);

    // VK_EXT_sample_locations
    VULKAN_FUNCTION(CmdSetSampleLocationsEXT);

    // VK_EXT_mesh_shader
    VULKAN_FUNCTION(CmdDrawMeshTasksEXT);
    VULKAN_FUNCTION(CmdDrawMeshTasksIndirectEXT);
    VULKAN_FUNCTION(CmdDrawMeshTasksIndirectCountEXT);

    // VK_NV_low_latency2
    VULKAN_FUNCTION(GetLatencyTimingsNV);
    VULKAN_FUNCTION(LatencySleepNV);
    VULKAN_FUNCTION(SetLatencyMarkerNV);
    VULKAN_FUNCTION(SetLatencySleepModeNV);
};

#undef VULKAN_FUNCTION
