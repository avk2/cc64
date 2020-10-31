#pragma once
/*
        Math function compiler version 4.0
        File mathfc.h provides the external interface
        Platrorms supported : x86 / Win32 and Win64
        Author: Kavinov A.V. 
*/
#include <string>
#include <array>
#include <vector>

#ifndef INCLUDED_FROM_EXPR_H
namespace MathFC {
#endif // INCLUDED_FROM_EXPR_H

#ifdef _MSC_VER
typedef double(_cdecl *Function)(const double *);
#else
typedef double(*Function)(const double *) __attribute__((cdecl));
#endif

#ifdef BUILD_DLL
    #define DLL_EXPORT __declspec(dllexport)
#else
    #ifdef MATHFC_TESTING
        #define DLL_EXPORT
    #else
        #define DLL_EXPORT __declspec(dllimport)
    #endif
#endif

//#ifdef __cplusplus

constexpr int MaxErrorString = 256;

// Compiler options
constexpr unsigned CO_TRIGFUNCTIONSFROMCRT = 1;
constexpr unsigned CO_DISABLEEXTENDEDFUNCTIONS = 2;
constexpr unsigned CO_DONTOPTIMIZE = 16;

	// pointer to a library function which accepts 1 argument
#ifdef _MSC_VER
typedef double(_cdecl *UserFunction1Arg)(double);
typedef double(_cdecl *UserFunction2Arg)(double, double);
typedef double(_cdecl *UserFunction3Arg)(double,double,double);
typedef double(_cdecl *UserFunction4Arg)(double, double, double, double);
#else
typedef double(*UserFunction1Arg)(double) __attribute__((cdecl));
typedef double(*UserFunction2Arg)(double, double) __attribute__((cdecl));
typedef double(*UserFunction3Arg)(double, double, double) __attribute__((cdecl));
typedef double(*UserFunction4Arg)(double, double, double, double) __attribute__((cdecl));
#endif

typedef UserFunction1Arg UserFunction;

constexpr unsigned int CALLOPT_64BIT_SAVEx87 = 0x20000;


#pragma pack(push, 1)
struct UserFunctionTableEntry {
	const char* FunctionName = nullptr;
    int ArgumentNumber = 1;
    UserFunction Function;
    unsigned int CallOptions = 0;


     UserFunctionTableEntry(const char* Name,
        UserFunction1Arg f, unsigned int Options = 0)
        : FunctionName(Name),ArgumentNumber(1),
            Function(f),CallOptions(Options) {};

     UserFunctionTableEntry(const char* Name,
         UserFunction2Arg f, unsigned int Options = 0)
         : FunctionName(Name), ArgumentNumber(2),
         Function(reinterpret_cast<UserFunction>(f)), CallOptions(Options) {};

     UserFunctionTableEntry(const char* Name,
         UserFunction3Arg f, unsigned int Options = 0)
         : FunctionName(Name), ArgumentNumber(3),
         Function(reinterpret_cast<UserFunction>(f)), CallOptions(Options) {};

     UserFunctionTableEntry(const char* Name,
         UserFunction4Arg f, unsigned int Options = 0)
         : FunctionName(Name), ArgumentNumber(4),
         Function(reinterpret_cast<UserFunction>(f)), CallOptions(Options) {};

     UserFunctionTableEntry(const char* Name,
         std::nullptr_t null, unsigned int Options = 0)
         : FunctionName(Name), ArgumentNumber(0),
         Function((UserFunction)(null)), CallOptions(Options) {};
};

struct FunctionInfo {
	size_t ArgumentNum;
	size_t CodeSize;
	size_t Reserved1;
	size_t Reserved2;
};
#pragma pack(pop)

extern "C" DLL_EXPORT Function CreateFunction(const char* ExpressionString,
    const char* VariableNames[],
    char* ErrorStr = nullptr,UserFunctionTableEntry* UserFunctionTable = nullptr,
	unsigned CompilerOptions = 0, FunctionInfo* Info = nullptr);
extern "C" DLL_EXPORT void DeleteFunction(Function CompiledFunction);

class CallException : public std::runtime_error {
public:
    CallException(const char* text) : runtime_error(text) {};
};

class WrongArgumentNumberException : public CallException
{
public:
    WrongArgumentNumberException() : CallException("Wrong number of arguments") {};
};

class InvalidFunctionException : public CallException
{
public:
    InvalidFunctionException() : CallException("Invalid function call") {};
};


class MathFunction {
private:
    Function f = nullptr;
	size_t ArgNum = 0;
    std::string ErrorString;

    MathFunction() = delete;
    MathFunction(const MathFunction& mf) = delete;
    const MathFunction& operator=(const MathFunction& mf) = delete;
	void Initialize(const char* fstr, const char* varnames[],
		UserFunctionTableEntry* UserFunctionTable, unsigned int Options)
	{
		FunctionInfo info;
		char buffer[MaxErrorString];
		f = CreateFunction(fstr, varnames, buffer, UserFunctionTable, Options, &info);
		if (!f) ErrorString = buffer;
		else ArgNum = info.ArgumentNum;
	}
public:
    MathFunction(const char* fstr, const char* ArgumentNames[],
        UserFunctionTableEntry* UserFunctionTable = nullptr,
        unsigned int Options = 0)
	{
		Initialize(fstr, ArgumentNames, UserFunctionTable, Options);
	};
	MathFunction(const std::string& str, const char* ArgumentNames[],
		UserFunctionTableEntry* UserFunctionTable = nullptr,
		unsigned int Options = 0)
		: MathFunction(str.c_str(), ArgumentNames, UserFunctionTable, Options) {};
	MathFunction(const std::string& str, const std::string& ArgName,	// один аргумент
		UserFunctionTableEntry* UserFunctionTable = nullptr,
		unsigned int Options = 0)
	{
		const char* argnames[] = { ArgName.c_str(),nullptr };
		Initialize(str.c_str(), argnames, UserFunctionTable, Options);
	};
	MathFunction(const std::string& str,const std::vector<std::string>& ArgumentNames,
		UserFunctionTableEntry* UserFunctionTable = nullptr,
		unsigned int Options = 0)
	{
		std::vector<const char*> argnames(ArgumentNames.size()+1,nullptr);
		for (size_t i = 0; i < ArgumentNames.size(); i++)
			argnames[i] = ArgumentNames[i].c_str();
		Initialize(str.c_str(), argnames.data(), UserFunctionTable, Options);
	};

    MathFunction(MathFunction&& mf) noexcept
    {
        f = mf.f; mf.f = nullptr;
        ArgNum = mf.ArgNum;
        if(!mf.ErrorString.empty()) ErrorString = std::move(mf.ErrorString);
    }
    void operator=(MathFunction&& mf) noexcept
    {
        f = mf.f; mf.f = nullptr;
        ArgNum = mf.ArgNum;
        if (!mf.ErrorString.empty()) ErrorString = std::move(mf.ErrorString);
    }
    ~MathFunction() noexcept { DeleteFunction(f); };
        // функции вызова
    template<size_t n> double operator()(const double (&args)[n])
    {
        if (!f) throw InvalidFunctionException{};
        if (n != ArgNum) throw WrongArgumentNumberException{};
        return f(args);
    }
	double operator()(const std::vector<double>& args)
	{
        if (!f) throw InvalidFunctionException{};
        if (args.size() != ArgNum) throw WrongArgumentNumberException{};
		return f(args.data());
	}
    double operator()(double a)
    {
        if (!f) throw InvalidFunctionException{};
        if (ArgNum != 1) throw WrongArgumentNumberException{};
        return f(&a);
    }
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
        // C++17 only
    template<class ...Ts> double operator()(double a,double b,Ts... args)
    {
        std::array<double,2+sizeof...(Ts)> ar{a,b,args...};
        return operator()(ar.data(),ar.size());
    }
#endif
    double operator()(const double* args,size_t argnum)
	{
        if (!f)
            throw InvalidFunctionException{};
		if (argnum != ArgNum)
            throw WrongArgumentNumberException{};
		return f(args);
	}
    operator bool() const noexcept { return f != 0; };
    std::string GetErrorString() const { return ErrorString; }
};

#ifndef INCLUDED_FROM_EXPR_H
}
#endif // INCLUDED_FROM_EXPR_H
/*
#else
#define MATHFC_MAX_ERROR_STRING 256
// Compiler options
#define CO_TRIGFUNCTIONSFROMCRT  1
#define CO_DISABLEEXTENDEDFUNCTIONS  2
#define CO_DONTOPTIMIZE 16

extern "C" DLL_EXPORT Function CreateFunction(const char* ExpressionString,
    const char* VariableNames[],
    char* ErrorStr,UserFunctionTableEntry* UserFunctionTable,
    unsigned CompilerOptions, FunctionInfo* Info);
extern "C" DLL_EXPORT void DeleteFunction(Function CompiledFunction);
#endif // __cplusplus
*/


