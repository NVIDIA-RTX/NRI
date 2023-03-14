/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma region [  Core  ]

static void NRI_CALL SetCommandQueueDebugName(CommandQueue& commandQueue, const char* name)
{
    ((CommandQueueVal*)&commandQueue)->SetDebugName(name);
}

static void NRI_CALL QueueSubmit(CommandQueue& commandQueue, const QueueSubmitDesc& queueSubmitDesc)
{
    ((CommandQueueVal*)&commandQueue)->Submit(queueSubmitDesc);
}

void FillFunctionTableCommandQueueVal(CoreInterface& coreInterface)
{
    coreInterface.SetCommandQueueDebugName = ::SetCommandQueueDebugName;
    coreInterface.QueueSubmit = ::QueueSubmit;
}

#pragma endregion

#pragma region [  Helper  ]

static Result NRI_CALL ChangeResourceStatesVal(CommandQueue& commandQueue, const TransitionBarrierDesc& transitionBarriers)
{
    return ((CommandQueueVal&)commandQueue).ChangeResourceStates(transitionBarriers);
}

static nri::Result NRI_CALL UploadDataVal(CommandQueue& commandQueue, const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum,
    const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum)
{
    return ((CommandQueueVal&)commandQueue).UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

static nri::Result NRI_CALL WaitForIdleVal(CommandQueue& commandQueue)
{
    return ((CommandQueueVal&)commandQueue).WaitForIdle();
}

void FillFunctionTableCommandQueueVal(HelperInterface& helperInterface)
{
    helperInterface.ChangeResourceStates = ::ChangeResourceStatesVal;
    helperInterface.UploadData = ::UploadDataVal;
    helperInterface.WaitForIdle = ::WaitForIdleVal;
}

#pragma endregion