/*
        Math function compiler version 4.0
        File libinterface.cpp : the DLL interface
        Platrorms supported : x86 / Win32 and Win64
        Author: Kavinov A.V.
*/
#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include <algorithm>
#include <memory>
#include <cmath>
#include <cstring>
#include <windows.h>
#include "expr.h"

constexpr int CodeBufferSize = 1024 * 64;

extern bool IsCorrectVariableName(const std::string& name);


extern "C" DLL_EXPORT Function CreateFunction(const char* ExpressionString,
    const char* VariableNames[],char* ErrorString,UserFunctionTableEntry* FunctionTable,
	unsigned CompilerOptions, FunctionInfo * Info)
{
    if (ErrorString) ErrorString[0] = 0;
    std::vector<std::string> vars;
    if (VariableNames)
    {
        for (unsigned int i = 0; VariableNames[i]; i++)
        {
            std::string Name = VariableNames[i];
            if (IsCorrectVariableName(Name))
                vars.emplace_back(std::move(Name));
            else
            {
                if (ErrorString)
                    strncpy(ErrorString, "Bad variable name", MaxErrorString);
                return nullptr;
            }
        }
    }

    std::vector<uint8_t> CodeBuffer(CodeBufferSize);
    size_t CompiledCodeSize = 0;
    try {
        TCompilationState cs(vars, CodeBuffer.data(), CompilerOptions,
            CodeBufferSize, FunctionTable);

        std::unique_ptr<Program> p{ new Program{ ExpressionString } };
        CompiledCodeSize = p->Compile(cs);
        if (Info)
        {
            memset(Info, 0, sizeof(FunctionInfo));
            Info->CodeSize = CompiledCodeSize;
            Info->ArgumentNum = vars.size();
        }
    }
    catch (EFException & ex)
    {
        if (ErrorString)
        {
            strncpy(ErrorString, ex.Message().c_str(), MaxErrorString);
            ErrorString[MaxErrorString - 1] = 0;
        }
        return nullptr;
    }
    void* buffer = VirtualAlloc(nullptr, CompiledCodeSize, MEM_COMMIT, PAGE_READWRITE);
    if (!buffer)
    {
        if (ErrorString) strncpy(ErrorString, "Not enough memory", MaxErrorString);
        return nullptr;
    }
    std::memcpy(buffer, CodeBuffer.data(), CompiledCodeSize);
    DWORD dummy;
    VirtualProtect(buffer, CompiledCodeSize, PAGE_EXECUTE_READ, &dummy);
    return reinterpret_cast<Function>(buffer);
}

extern "C" DLL_EXPORT void DeleteFunction(Function CompiledFunction)
{
    VirtualFree((void*)CompiledFunction, 0, MEM_RELEASE);
}
