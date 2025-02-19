// © 2021 NVIDIA Corporation

#ifndef _WIN32
#    include <csignal> // raise
#    include <cstdarg> // va_start, va_end
#endif

#include "SharedExternal.h"

#include "HelperDataUpload.h"
#include "HelperDeviceMemoryAllocator.h"
#include "HelperWaitIdle.h"
#include "Streamer.h"
#include "Upscaler.h"

using namespace nri;

#include "HelperDataUpload.hpp"
#include "HelperDeviceMemoryAllocator.hpp"
#include "HelperWaitIdle.hpp"
#include "Streamer.hpp"
#include "Upscaler.hpp"

#include "SharedExternal.hpp"
#include "SharedLibrary.hpp"
