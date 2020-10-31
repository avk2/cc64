/*
        Math function compiler version 4.0
        File main.cpp 
*/
#include "main.h"

// Nothing interesting here

extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // attach to process
            // return FALSE to fail DLL load
            break;

        case DLL_PROCESS_DETACH:
            // detach from process
            break;

        case DLL_THREAD_ATTACH:
            // attach to thread
            break;

        case DLL_THREAD_DETACH:
            // detach from thread
            break;
    }
    return TRUE; // succesful
}

#ifdef __GNUC__
extern "C" char* __cxa_demangle(const char* mangled_name, char* buf, size_t* n, int* status)
{
    if (status) *status = -1;
    return nullptr;
}
#endif

