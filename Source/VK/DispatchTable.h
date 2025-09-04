// © 2021 NVIDIA Corporation

#pragma once

#define VK_FUNC(name) PFN_vk##name name

struct DispatchTable {
    //==========================================================================
    // Instance
    //==========================================================================
    VK_FUNC(GetInstanceProcAddr);
    VK_FUNC(CreateInstance);
    VK_FUNC(EnumerateInstanceExtensionProperties);
    VK_FUNC(EnumerateInstanceLayerProperties);

    VK_FUNC(GetDeviceProcAddr);
    VK_FUNC(DestroyInstance);
    VK_FUNC(DestroyDevice);
    VK_FUNC(GetPhysicalDeviceMemoryProperties2);
    VK_FUNC(GetDeviceGroupPeerMemoryFeatures);
    VK_FUNC(GetPhysicalDeviceFormatProperties2);
    VK_FUNC(GetPhysicalDeviceImageFormatProperties2);
    VK_FUNC(CreateDevice); // may return "VK_ERROR_DEVICE_LOST"
    VK_FUNC(GetDeviceQueue2);
    VK_FUNC(EnumeratePhysicalDeviceGroups);
    VK_FUNC(GetPhysicalDeviceProperties2);
    VK_FUNC(GetPhysicalDeviceFeatures2);
    VK_FUNC(GetPhysicalDeviceQueueFamilyProperties2);
    VK_FUNC(EnumerateDeviceExtensionProperties);

    // VK_KHR_get_surface_capabilities2
    VK_FUNC(GetPhysicalDeviceSurfaceFormats2KHR);
    VK_FUNC(GetPhysicalDeviceSurfaceCapabilities2KHR);

    // VK_KHR_surface
    VK_FUNC(GetPhysicalDeviceSurfaceSupportKHR);
    VK_FUNC(GetPhysicalDeviceSurfacePresentModesKHR);
    VK_FUNC(DestroySurfaceKHR);

#ifdef VK_USE_PLATFORM_WIN32_KHR
    VK_FUNC(CreateWin32SurfaceKHR);
    VK_FUNC(GetMemoryWin32HandlePropertiesKHR);
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    VK_FUNC(CreateXlibSurfaceKHR);
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    VK_FUNC(CreateWaylandSurfaceKHR);
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
    VK_FUNC(CreateMetalSurfaceEXT);
#endif

    // VK_EXT_debug_utils
    VK_FUNC(SetDebugUtilsObjectNameEXT);
    VK_FUNC(CmdBeginDebugUtilsLabelEXT);
    VK_FUNC(CmdEndDebugUtilsLabelEXT);
    VK_FUNC(CmdInsertDebugUtilsLabelEXT);
    VK_FUNC(QueueBeginDebugUtilsLabelEXT);
    VK_FUNC(QueueEndDebugUtilsLabelEXT);
    VK_FUNC(QueueInsertDebugUtilsLabelEXT);

    //==========================================================================
    // Device                                    Thread safety | Accounted
    //==========================================================================
    VK_FUNC(CreateBuffer);                                // + | +
    VK_FUNC(CreateImage);                                 // + | +
    VK_FUNC(CreateBufferView);                            // + | +
    VK_FUNC(CreateImageView);                             // + | +
    VK_FUNC(CreateSampler);                               // + | +
    VK_FUNC(CreateQueryPool);                             // + | +
    VK_FUNC(CreateCommandPool);                           // + | +
    VK_FUNC(CreateSemaphore);                             // + | +
    VK_FUNC(CreateDescriptorPool);                        // + | +
    VK_FUNC(CreatePipelineLayout);                        // + | +
    VK_FUNC(CreateDescriptorSetLayout);                   // + | +
    VK_FUNC(CreateShaderModule);                          // + | +
    VK_FUNC(CreateGraphicsPipelines);                     // + | +
    VK_FUNC(CreateComputePipelines);                      // + | +
    VK_FUNC(AllocateMemory);                              // + | +
    VK_FUNC(DestroyBuffer);                               // - | +
    VK_FUNC(DestroyImage);                                // - | +
    VK_FUNC(DestroyBufferView);                           // - | +
    VK_FUNC(DestroyImageView);                            // - | +
    VK_FUNC(DestroySampler);                              // - | +
    VK_FUNC(DestroyFramebuffer);                          // - | +
    VK_FUNC(DestroyQueryPool);                            // - | +
    VK_FUNC(DestroyCommandPool);                          // - | +
    VK_FUNC(DestroySemaphore);                            // - | +
    VK_FUNC(DestroyDescriptorPool);                       // - | +
    VK_FUNC(DestroyPipelineLayout);                       // - | +
    VK_FUNC(DestroyDescriptorSetLayout);                  // - | +
    VK_FUNC(DestroyShaderModule);                         // - | +
    VK_FUNC(DestroyPipeline);                             // - | +
    VK_FUNC(FreeMemory);                                  // - | +
    VK_FUNC(FreeCommandBuffers);                          // - | +
    VK_FUNC(MapMemory);                                   // - | +
    VK_FUNC(FlushMappedMemoryRanges);                     // + | +
    VK_FUNC(QueueWaitIdle);                               // - | + may return "VK_ERROR_DEVICE_LOST"
    VK_FUNC(QueueSubmit2);                                // - | + may return "VK_ERROR_DEVICE_LOST"
    VK_FUNC(GetSemaphoreCounterValue);                    // + | + TODO: may return "VK_ERROR_DEVICE_LOST"
    VK_FUNC(WaitSemaphores);                              // + | + TODO: may return "VK_ERROR_DEVICE_LOST"
    VK_FUNC(ResetCommandPool);                            // - | +
    VK_FUNC(ResetDescriptorPool);                         // - | +
    VK_FUNC(AllocateCommandBuffers);                      // - | +
    VK_FUNC(AllocateDescriptorSets);                      // - | +
    VK_FUNC(UpdateDescriptorSets);                        // + | +
    VK_FUNC(BindBufferMemory2);                           // + | +
    VK_FUNC(BindImageMemory2);                            // + | +
    VK_FUNC(GetBufferMemoryRequirements2);                // + | +
    VK_FUNC(GetImageMemoryRequirements2);                 // + | +
    VK_FUNC(ResetQueryPool);                              // + | +
    VK_FUNC(GetBufferDeviceAddress);                      // + | +
    VK_FUNC(BeginCommandBuffer);                          // - | +
    VK_FUNC(CmdSetViewportWithCount);                     // - | +
    VK_FUNC(CmdSetScissorWithCount);                      // - | +
    VK_FUNC(CmdSetDepthBounds);                           // - | +
    VK_FUNC(CmdSetStencilReference);                      // - | +
    VK_FUNC(CmdSetBlendConstants);                        // - | +
    VK_FUNC(CmdSetDepthBias);                             // - | + TODO: "VK_EXT_depth_bias_control" offers "2" but MoltenVK doesn't support it yet
    VK_FUNC(CmdClearAttachments);                         // - | +
    VK_FUNC(CmdClearColorImage);                          // - | +
    VK_FUNC(CmdBindVertexBuffers2);                       // - | +
    VK_FUNC(CmdBindIndexBuffer);                          // - | +
    VK_FUNC(CmdBindPipeline);                             // - | +
    VK_FUNC(CmdBindDescriptorSets);                       // - | +
    VK_FUNC(CmdPushConstants);                            // - | +
    VK_FUNC(CmdDispatch);                                 // - | +
    VK_FUNC(CmdDispatchIndirect);                         // - | +
    VK_FUNC(CmdDraw);                                     // - | +
    VK_FUNC(CmdDrawIndexed);                              // - | +
    VK_FUNC(CmdDrawIndirect);                             // - | +
    VK_FUNC(CmdDrawIndexedIndirect);                      // - | +
    VK_FUNC(CmdDrawIndirectCount);                        // - | +
    VK_FUNC(CmdDrawIndexedIndirectCount);                 // - | +
    VK_FUNC(CmdCopyBuffer2);                              // - | +
    VK_FUNC(CmdCopyImage2);                               // - | +
    VK_FUNC(CmdResolveImage2);                            // - | +
    VK_FUNC(CmdCopyBufferToImage2);                       // - | +
    VK_FUNC(CmdCopyImageToBuffer2);                       // - | +
    VK_FUNC(CmdPipelineBarrier2);                         // - | +
    VK_FUNC(CmdBeginQuery);                               // - | +
    VK_FUNC(CmdEndQuery);                                 // - | +
    VK_FUNC(CmdWriteTimestamp2);                          // - | +
    VK_FUNC(CmdCopyQueryPoolResults);                     // - | +
    VK_FUNC(CmdResetQueryPool);                           // - | +
    VK_FUNC(CmdFillBuffer);                               // - | +
    VK_FUNC(CmdBeginRendering);                           // - | +
    VK_FUNC(CmdEndRendering);                             // - | +
    VK_FUNC(CmdPushDescriptorSet);                        // - | +
    VK_FUNC(EndCommandBuffer);                            // - | +
                                                          // VK_KHR_maintenance4
    VK_FUNC(GetDeviceBufferMemoryRequirements);           // + | +
    VK_FUNC(GetDeviceImageMemoryRequirements);            // + | +
                                                          // VK_KHR_maintenance5
    VK_FUNC(CmdBindIndexBuffer2);                         // - | +
                                                          // VK_KHR_maintenance6
    VK_FUNC(CmdBindDescriptorSets2);                      // - | +
    VK_FUNC(CmdPushConstants2);                           // - | +
                                                          // VK_KHR_fragment_shading_rate
    VK_FUNC(CmdSetFragmentShadingRateKHR);                // - | +
                                                          // VK_KHR_swapchain
    VK_FUNC(AcquireNextImage2KHR);                        // - | ? may return "VK_ERROR_DEVICE_LOST"
    VK_FUNC(QueuePresentKHR);                             // - | ? may return "VK_ERROR_DEVICE_LOST"
    VK_FUNC(CreateSwapchainKHR);                          // + | ? may return "VK_ERROR_DEVICE_LOST"
    VK_FUNC(DestroySwapchainKHR);                         // - | +
    VK_FUNC(GetSwapchainImagesKHR);                       // + | +
                                                          // VK_KHR_present_wait
    VK_FUNC(WaitForPresentKHR);                           // - | ? may return "VK_ERROR_DEVICE_LOST"
                                                          // VK_KHR_acceleration_structure
    VK_FUNC(CreateAccelerationStructureKHR);              // + | +
    VK_FUNC(DestroyAccelerationStructureKHR);             // - | +
    VK_FUNC(GetAccelerationStructureDeviceAddressKHR);    // + | +
    VK_FUNC(GetAccelerationStructureBuildSizesKHR);       // + | +
    VK_FUNC(CmdBuildAccelerationStructuresKHR);           // - | +
    VK_FUNC(CmdCopyAccelerationStructureKHR);             // - | +
    VK_FUNC(CmdWriteAccelerationStructuresPropertiesKHR); // - | +
                                                          // VK_KHR_ray_tracing_pipeline
    VK_FUNC(CreateRayTracingPipelinesKHR);                // + | +
    VK_FUNC(GetRayTracingShaderGroupHandlesKHR);          // + | +
    VK_FUNC(CmdTraceRaysKHR);                             // - | +
    VK_FUNC(CmdTraceRaysIndirect2KHR);                    // - | +
                                                          // VK_EXT_opacity_micromap
    VK_FUNC(CreateMicromapEXT);                           // + | +
    VK_FUNC(DestroyMicromapEXT);                          // - | +
    VK_FUNC(GetMicromapBuildSizesEXT);                    // + | +
    VK_FUNC(CmdBuildMicromapsEXT);                        // - | +
    VK_FUNC(CmdCopyMicromapEXT);                          // - | +
    VK_FUNC(CmdWriteMicromapsPropertiesEXT);              // - | +
                                                          // VK_EXT_sample_locations
    VK_FUNC(CmdSetSampleLocationsEXT);                    // - | +
                                                          // VK_EXT_mesh_shader
    VK_FUNC(CmdDrawMeshTasksEXT);                         // - | +
    VK_FUNC(CmdDrawMeshTasksIndirectEXT);                 // - | +
    VK_FUNC(CmdDrawMeshTasksIndirectCountEXT);            // - | +
                                                          // VK_NV_low_latency2
    VK_FUNC(GetLatencyTimingsNV);                         // + | +
    VK_FUNC(LatencySleepNV);                              // + | +
    VK_FUNC(SetLatencyMarkerNV);                          // + | +
    VK_FUNC(SetLatencySleepModeNV);                       // + | +
};

#undef VK_FUNC
