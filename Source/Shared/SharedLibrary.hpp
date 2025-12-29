// © 2021 NVIDIA Corporation

#if defined(_WIN32)
#    include <windows.h>
#    undef LoadLibrary

Library* nri::LoadSharedLibrary(const char* path) {
    return (Library*)LoadLibraryA(path);
}

void* nri::GetSharedLibraryFunction(Library& library, const char* name) {
    return (void*)GetProcAddress((HMODULE)&library, name);
}

void nri::UnloadSharedLibrary(Library& library) {
    FreeLibrary((HMODULE)&library);
}
#else
#    include <dlfcn.h>

Library* nri::LoadSharedLibrary(const char* path) {
    return (Library*)dlopen(path, RTLD_NOW);
}

void* nri::GetSharedLibraryFunction(Library& library, const char* name) {
    return dlsym((void*)&library, name);
}

void nri::UnloadSharedLibrary(Library& library) {
    dlclose((void*)&library);
}
#endif
