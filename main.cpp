/*
        Файл main.cpp компилятоpа математических функций
        Веpсия 4.0 для Win32 и Win64
        (c) Kavinov A.V. 1998-2002,2011,2019
*/
#include "main.h"

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

