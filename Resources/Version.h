// © 2021 NVIDIA Corporation

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define VERSION_MAJOR                   1
#define VERSION_MINOR                   164
#define VERSION_BUILD                   0
#define VERSION_REVISION                0

#define VERSION_STRING STR(VERSION_MAJOR.VERSION_MINOR.VERSION_BUILD.VERSION_REVISION)
