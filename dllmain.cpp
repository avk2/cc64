#include <windows.h>

#include "mathfc.h"

#include "expr.h"


using namespace std;

const int MaxErrorString= 255;
const int InitCodeSize= 128;
static char ErrorString[MaxErrorString+1];

extern "C" DLL_EXPORT Function CreateFunction(const char* ExpressionString,const char* VariableNames,int* outSize)
{
	ErrorString[0]= 0;
	char* CodeBuffer;

	vector<string> vars;
	unsigned int cbegin= 0,len= strlen(VariableNames);
	for(unsigned int i= 0;i<len;i++)
	{
        if(VariableNames[i]=='/')
		{
			vars.push_back(string(VariableNames,cbegin,i-cbegin));
			cbegin= i+1;
		}
	}
	vars.push_back(string(VariableNames,cbegin,len-cbegin));


	//vector<string> vars= ParseSubexpression(VariableNames,"/");
	for(int CodeBufferSize= InitCodeSize;CodeBufferSize<1024*64;CodeBufferSize*=2)
	{
		CodeBuffer= new char[CodeBufferSize];
		try {
			TExpression* e= CreateExpression(ExpressionString);
			int size= e->Compile(CodeBuffer,CodeBufferSize,vars,0);
			if(outSize) *outSize= size;
			delete e;
		}
		catch(ECBufferExceeded&)
		{
			delete[] CodeBuffer;
			continue;
		}
		catch(EFException& ex)
		{
			strncpy(ErrorString,ex.Message().c_str(),MaxErrorString);
			break;
		}
		return (Function)((void*)CodeBuffer);
	}
	return 0;
}

extern "C" DLL_EXPORT char* GetLastFunctionCompilerError()
{
	return ErrorString;
}
extern "C" DLL_EXPORT void DeleteFunction(Function CompiledFunction)
{
	delete (char*)CompiledFunction;
}

extern "C" DLL_EXPORT Function CopyFunction(Function SomeFunction,int CodeSize)
{
    char* NewFunction= new char[CodeSize];
    memcpy(NewFunction,(char*)SomeFunction,CodeSize);
    return (Function)NewFunction;
}

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
