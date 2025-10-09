// © 2021 NVIDIA Corporation

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wswitch"
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#    pragma GCC diagnostic ignored "-Wunused-variable"
#    pragma GCC diagnostic ignored "-Wsometimes-uninitialized"
#    pragma GCC diagnostic ignored "-Wunused-function"
#elif defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wswitch"
#    pragma clang diagnostic ignored "-Wunused-parameter"
#    pragma clang diagnostic ignored "-Wunused-variable"
#    pragma clang diagnostic ignored "-Wsometimes-uninitialized"
#    pragma clang diagnostic ignored "-Wunused-function"
#elif defined(_MSC_VER)
#    pragma warning(push)           // applicable to Clang in MSVC environment
#    pragma warning(disable : 4063) // case 'identifier' is not a valid value for switch of enum 'enumeration'
#    pragma warning(disable : 4100) // unreferenced formal parameter
#    pragma warning(disable : 4189) // local variable is initialized but not referenced
#    pragma warning(disable : 4505) // unreferenced function with internal linkage has been removed
#    pragma warning(disable : 4701) // potentially uninitialized local variable
#endif

#ifndef NDEBUG
#    define D3D12MA_DEBUG_LOG(format, ...) \
        do { \
            wprintf(format, __VA_ARGS__); \
            wprintf(L"\n"); \
        } while (false)
#endif

#include "D3D12MemAlloc.cpp"

#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(__clang__)
#    pragma clang diagnostic pop
#else
#    pragma warning(pop)
#endif
