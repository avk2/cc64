/*
        Math function compiler version 4.0
        File expr.h : the compiler's internal include file
        Platrorms supported : x86 / Win32 and Win64
        Author: Kavinov A.V.
*/
#pragma once

#include <math.h>
#include <string>
#include <vector>
#include <cstddef>
#include <functional>
#include <map>
#include <set>


#ifdef _MSC_VER
#pragma warning( disable : 26451 )
#endif


#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2		1.57079632679489661923
#endif

inline std::string Int2Str(int num)
{
    char buf[32] = {};
    _snprintf_s(buf, 31, "%d", num);
    return std::string(buf);
}
inline std::string Double2Str(double num)
{
    char buf[32] = {};
    _snprintf_s(buf, 31, "%lg", num);
    return std::string(buf);
}

        // ������ �������
class EFException {
public:
	EFException() {};
    virtual std::string Message() = 0;
	virtual ~EFException() {};
};

class EFExtraRS : public EFException {
public:
    std::string Message() { return "Extra )"; };
};

class EFExtraLS : public EFException {
public:
    std::string Message() { return "Extra ("; };
};

class EFEmptySubexpression : public EFException {
public:
    std::string Message() { return "Empty subexpression"; };
};

class EFEmptyExpression : public EFException {
public:
    std::string Message() { return "Empty expression"; };
};

class EFInvalidNumber : public EFException {
public:
    std::string Message() { return "Invalid number"; };
};

class EFInvalidSymbol : public EFException {
    std::string sym;
public:
    EFInvalidSymbol(const std::string& s= "") : sym(s) {}
    std::string Message()
        { return (std::string("Invalid symbol \'")+sym+"\'"); };
};

class EFInvalidFunctionCall : public EFException {
    std::string func;
public:
    EFInvalidFunctionCall(const std::string& s= "") : func(s) {}
    std::string Message()
        { return (std::string("Invalid function \'")+func+"\' Call"); };
};

class EFInvalidPowerExpression : public EFException {
    std::string expr;
public:
    EFInvalidPowerExpression(const std::string& s = "") : expr(s) {}
    std::string Message()
    { return (std::string("Invalid power expression \'") + expr + "\'"); };
};

class EFFunctionNotFound : public EFException {
	std::string func;
	int mArgNum;
public:
	EFFunctionNotFound(const std::string& s,size_t ArgNum)
        : func(s),mArgNum(int(ArgNum)) {}
	std::string Message()
	{
		return (std::string("Function \'") + func + "\' with "+
          Int2Str(mArgNum)+" arguments not found");
	};
};

class EFInvalidArgumentNumber : public EFException {
	std::string func;
	int mPassedArgNum,mWantedArgNum;
public:
    EFInvalidArgumentNumber(const std::string& FunctionName,
        int PassedArgNum,int WantedArgNum)
        : func(FunctionName),mPassedArgNum(PassedArgNum),
        mWantedArgNum(WantedArgNum) {};
	std::string Message()
	{
		return (std::string("External function \'") + func +
          "\' called with " + Int2Str(mPassedArgNum) +
          " arguments ("+ Int2Str(mWantedArgNum) + " expected)");
	};
};
        // ������ ����������
class ECBufferExceeded : public EFException {
public:
    std::string Message() { return "Buffer exceeded"; };
};

class ECStackExceeded : public EFException {
public:
    std::string Message() { return "Stack exceeded"; };
};

class ECInternalError : public EFException {
public:
    std::string Message() { return "Internal error"; };
};

class ECUndefinedSymbol : public EFException {
	std::string SymbolName;
public:
	ECUndefinedSymbol(const std::string& BadSymbolName)
		: EFException(),SymbolName(BadSymbolName) {};
    std::string Message()
		{ return std::string("Undefined symbol '")+SymbolName+"'"; };
};

class ECBadVariableName : public EFException {
	std::string BadName;
public:
	ECBadVariableName(const std::string& BadVariableName)
		: EFException(),BadName(BadVariableName) {};
    std::string Message()
		{ return std::string("Bad variable name '")+BadName+"'"; };
};

class ECBadVariableUsage : public EFException {
    std::string BadName;
public:
    ECBadVariableUsage(const std::string& BadVariableName)
        : EFException(), BadName(BadVariableName) {};
    std::string Message()
    { return std::string("Undefined variable '") + BadName + "' usage"; };
};


class ECInvalidAssignment : public EFException {
	std::string text;
public:
	ECInvalidAssignment(const std::string& BadAssignment)
		: EFException(),text(BadAssignment) {};
    std::string Message()
		{ return std::string("Bad assignment '")+text+"'"; };
};
#define INCLUDED_FROM_EXPR_H
#include "mathfc.h"

// pointer to a function which accepts 1 argument
#ifdef _MSC_VER
typedef double(_cdecl *LibraryFunction1Arg)(double);
#else
typedef double(*LibraryFunction1Arg)(double) __attribute__((cdecl));
#endif

class TCompilationState {
    TCompilationState() = delete;
    TCompilationState(const TCompilationState&) = delete;
    TCompilationState& operator=(const TCompilationState&) = delete;
private:
	void PutByte(unsigned char code_byte)
		{ if(i>=maxcodesize) throw ECBufferExceeded(); fcode[i++]= code_byte; };
	void PutBytes(unsigned char code_byte1,unsigned char code_byte2)
		{ PutByte(code_byte1); PutByte(code_byte2); };
	void PutBytes(unsigned char code_byte1,unsigned char code_byte2,unsigned char code_byte3)
		{ PutByte(code_byte1); PutByte(code_byte2); PutByte(code_byte3); };
	void PutBytes(unsigned char code_byte1,unsigned char code_byte2,unsigned char code_byte3,unsigned char code_byte4)
		{ PutByte(code_byte1); PutByte(code_byte2); PutByte(code_byte3); PutByte(code_byte4); };
	void PutBytes(unsigned char code_byte1, unsigned char code_byte2, unsigned char code_byte3,
		unsigned char code_byte4, unsigned char code_byte5)
	{
		PutByte(code_byte1); PutByte(code_byte2); PutByte(code_byte3); PutByte(code_byte4); PutByte(code_byte5);
	};
	void PutBytes(unsigned char code_byte1, unsigned char code_byte2, unsigned char code_byte3,
		unsigned char code_byte4, unsigned char code_byte5, unsigned char code_byte6)
	{
		PutByte(code_byte1); PutByte(code_byte2); PutByte(code_byte3); PutByte(code_byte4);
		PutByte(code_byte5); PutByte(code_byte6);
	};
	void PutDword(unsigned int dw)
		{ CheckSize(4); *((unsigned int*)(fcode+i))= dw; i+= 4; };
	void PutQword(unsigned long long qw)
		{ CheckSize(8); *((unsigned long long*)(fcode + i)) = qw; i += 8; };

	double LastLoadedDouble = 0; bool IsDoubleLoaded = false;
	int LastLoadedInt = 0; bool IsIntLoaded = false;
	float LastLoadedFloat= 0; bool IsFloatLoaded = false;
    std::set<int> StoredLocalVariableIndices = {}; // ������� ����������� ��������� ����������
	std::vector<std::string>* LocalVariableNameTable = nullptr; // ������� ��� ���. �����-�
public:
    struct OptimData* pOptimData = nullptr; // ��� �����������
public:
	TCompilationState(const std::vector<std::string>& ArgumentNames,unsigned char* CodeBuffer,
		unsigned int Options,unsigned int MaxCodeSize,struct UserFunctionTableEntry* UserFunctionTable)
		:
		ArgNames(ArgumentNames),fcode(CodeBuffer),CompilerOptions(Options),
		maxcodesize(MaxCodeSize), FunctionTable(UserFunctionTable) {};
	const std::vector<std::string>& ArgNames; // ����� ���������� �������
	unsigned char *fcode = nullptr;	// ��������� �� ������ ����p� ����
	unsigned int i = 0;			// ������ ������������� ����� ����
	unsigned int CompilerOptions;	// �����
	unsigned int maxcodesize;	// ������������ p����p ����
	struct UserFunctionTableEntry* FunctionTable = nullptr;    // ������� ������� �������
	unsigned int StackRegsUsed = 0;		// ��p������� ��� �������� �� ������ ���p������p�
 	static const unsigned int StackSize= 8;

	void CheckSize(unsigned int sz) // ������ �� ��� sz ����?
		{ if(i+sz>maxcodesize) throw ECBufferExceeded(); };
	unsigned int StackAvailable() { return StackSize-StackRegsUsed; };

    void SetOptimData(struct OptimData* od) { pOptimData = od; };

	void fchs();					// ���������� � ����p ����p����� fchs
	void load_const_st0(double);	// ������� ���������� � ����p ���,
									// ���p������� ��������� � ST(0)
	void load_const_bp_double(double cd);
	void load_const_bp_float(float cf);
	void load_const_bp_int32(int n);

	void load_var_st0(int index);	// ���������� ���, ������� ���������
									// ������� ���������� ����� index � ST(0)
	void add_sub_st0_var(int index,bool add);	// ���������� ���,
									// ������������/���������� ������� ���������� �� ST(0)
	void mul_div_st0_var(int index,bool mul);	// ���������� ���,
                                                // ����������/������� ST(0) �� ������� ���������� ����� index

    void load_local_var_st0(int index); // ���������� ���, ������� ���������
            									// ��������� ���������� ����� index � ST(0)
    void add_sub_st0_local_var(int index, bool add);	// ���������� ���,
                                                // ������������/���������� ��������� ���������� �� ST(0)
    void mul_div_st0_local_var(int index, bool mul);	// ���������� ���,
                                                // ����������/������� ST(0) �� ��������� ���������� ����� index
    void add_sub_st0_const(double cnst,bool add); // ST(0) ����/����� ���������
	void mul_div_st0_const(double cnst,bool mul); // ���������� ���,
									// ���������� ST(0) �� ���������
	void faddp_st1_st0(); 		// ��� ��������p���
	void fsubp_st1_st0();		// ��� ��������p���
	void fsubrp_st1_st0();		// ��� ��������p���
	void fmulp_st1_st0(); 		// ��� ��������p���
	void fdivp_st1_st0(); 		// ��� ��������p���
	void fdivrp_st1_st();
	void fxch_st1();
	void fldpi();
	void fld_st0();
	void fld1();

	void start_code(size_t LocalVarCount = 0);			// ��������� ���
	void end_code();			// �������� ���

    void move_st0_to_stack() // ����������� st(0) �� ����� FPU � �������
    {
        _move_st0_to_stack_80bit();
        // _move_st0_to_stack_64bit();
    }
    void move_stack_top_to_st0() // ��������
    {
        _move_stack_top_to_st0_80bit();
        //else _move_stack_top_to_st0_64bit();
    }

	void _move_st0_to_stack_64bit(); // ����������� st(0) �� ����� FPU � �������
	void _move_stack_top_to_st0_64bit(); // ��������
	void _move_st0_to_stack_80bit(); // ����������� st(0) �� ����� FPU � �������, � �������������, ���� ����
	void _move_stack_top_to_st0_80bit(); // ��������
        // ��������� ���� ������� ����������� 1 ����� � ����� FPU / ���������� ��� �������
    void move_st1_to_stack() // ����������� st(1) �� ����� FPU � �������, � �������������, ���� ����
        { fxch_st1(); move_st0_to_stack();  }
	void move_stack_top_to_st1() // ��������
        { move_stack_top_to_st0(); fxch_st1(); }
	void movsd_xmmX_offset_bp(int8_t X, int8_t storenum);
	//void fsave_to_stack();
	//void frstor_from_stack();

    void alloc_tmp_space(size_t TempVarCount);
    void free_tmp_space();
    void save_temp_var(int index);
    void load_temp_var(int index);
    void addsub_temp_var(int index,bool Add);
    void muldiv_temp_var(int index, bool Mul);

	void fstp_to_arg_store(int storenum);
	void push_from_arg_store(int storenum);

    void SetLocalVariableTableName(std::vector<std::string>* LocalVarTableName)
        { LocalVariableNameTable = LocalVarTableName; };
    int GetLocalVariableIndex(const std::string& name);
    void StoreLocalVariable(int index);
    int ArgIndex(const std::string& Name);
	//void SaveToUserVariable(int Index);

	//void StoreResult(int index);    // Saves the result of vector function

    UserFunctionTableEntry* FindUserFunction(const char* FuncName);
        // Savex87State has an effect for 64 bit only - for 32 bit it is always saved
	void CallLibraryFunction(LibraryFunction1Arg f,int ArgNum = 1,bool Savex87State = false,
            bool LastExpression = false);
    void CallUserFunction(UserFunctionTableEntry*,bool LastExpression = false);
	void fexp();
	void fexp2();
	void fpow();
	void fln();
	void flog2();
	void ftan();
	void fcot();
	void finv(); // ��� �����������
	void fatan();
	void factg();
	void fasin();
	void facos();
	void fabs();
	void fsin();
	void fcos();
	void fsqrt();
    void fcbrt();
	void fsign();
	void fsquare();
	void fmin();
	void fmax();
	void fround();
	void fceil();
	void ffloor();
	void fint();
	void fmod();
	void fsinh_from_exp();
    void fcosh_from_exp();
    void ftanh_from_exp();
	void heaviside();
};

class TExpression {
private:
    TExpression& operator=(const TExpression&) = delete;
	TExpression(const TExpression&) = delete;
protected:
	TExpression() {};
private:
        // ��� ������� �������
	virtual void SubCompileLoadST(TCompilationState& cs,bool LastExpression) = 0;
	virtual void SubCompileAddSubST(TCompilationState& cs,bool Add) = 0;
	virtual void SubCompileMulDivST(TCompilationState& cs,bool Mul) = 0;
public:
    void LoadST0(TCompilationState& cs,bool LastExpression);
    void AddSubST0(TCompilationState& cs,bool Add);
    void MulDivST0(TCompilationState& cs,bool Mul);
public:
    virtual TExpression* CreateCopy() = 0;
    virtual ~TExpression() {};
    bool IsSimple() { return GetTypeIndex() < 3; }; // ��������� ��� ���������� ? ��� ���?
    virtual bool IsEasyToCalculate() = 0; // ����� ������ ���������, ��� �������?
	virtual void EnumSubexpressions(std::function<bool(TExpression*)> f) = 0;
	virtual bool IsConstantValue(double val) { return false; };
	virtual bool SureEqual(TExpression* e) { return false; }; // surely equal if true
    virtual int GetTypeIndex() = 0;     // ��� ��������� �������
    virtual int Compare(TExpression* e) = 0;
    virtual void PrepareToOptimize() {};
	//unsigned Compile(TCompilationState& cs, bool WritePrologEpilog);
	unsigned Compile(TCompilationState& cs,bool LastExpression);
#ifdef _DEBUG
    virtual std::string GetText() = 0;
#endif
};
class TConstant : public TExpression {
private:
	double Value;
	const TConstant& operator=(const TConstant&) = delete;
    TConstant(const TConstant& c) = delete;
public:
	void SubCompileLoadST(TCompilationState& cs,bool LastExpression) override;
	void SubCompileAddSubST(TCompilationState& cs,bool Add) override;
	void SubCompileMulDivST(TCompilationState& cs,bool Mul) override;

public:
	TConstant(double dValue) : Value(dValue) {};
    TExpression* CreateCopy() override { return new TConstant(Value); };
    void EnumSubexpressions(std::function<bool(TExpression*)> f) override {};
    double GetDouble() { return Value; };
	bool IsConstantValue(double val) override { return val==Value; };
	bool SureEqual(TExpression* e) override;
    int GetTypeIndex() override { return 1; };
    bool IsEasyToCalculate() override { return true; };
    int Compare(TExpression* e) override;
#ifdef _DEBUG
    std::string GetText() override { return Double2Str(Value); };
#endif
};
class TVariable : public TExpression {
private:
	std::string Name;
	const TVariable& operator=(const TVariable&) = delete;
    TVariable(const TVariable& v) = delete;
public:
	void SubCompileLoadST(TCompilationState& cs,bool LastExpression) override;
	void SubCompileAddSubST(TCompilationState& cs,bool Add) override;
	void SubCompileMulDivST(TCompilationState& cs,bool Mul) override;
public:
	TVariable(const std::string& sName) : Name(sName) {};
    TExpression* CreateCopy() override { return new TVariable(Name); };
 	void EnumSubexpressions(std::function<bool(TExpression*)> f) override {};
    std::string GetName() const { return Name; };
    bool SureEqual(TExpression* e) override;
    int GetTypeIndex() override { return 2; };
    bool IsEasyToCalculate() override { return true; };
    int Compare(TExpression* e) override;
#ifdef _DEBUG
    std::string GetText() override { return Name; };
#endif
};

class TSum : public TExpression {
	const TSum& operator=(const TSum&) = delete;
	TSum(const TSum& v) = delete;
	friend TExpression* CreateMacro(const std::string& FunctionName,
                const std::vector<std::string>& args);
private:
    struct Term {
        TExpression* expr = nullptr;
        bool IsInverted = false;
    };
    bool EqualTerm(const Term& a, const Term& b)
    {
        if (a.IsInverted == b.IsInverted && a.expr->SureEqual(b.expr)) return true;
        return false;
    }
	std::vector<Term> Subexpressions;
	void AddItem(TExpression* Term,bool IsSubstracted);
	void Clear();
public:
    static int CompareTerm(const Term& a, const Term& b);

	void SubCompileLoadST(TCompilationState& cs,bool LastExpression) override;
	void SubCompileAddSubST(TCompilationState& cs,bool Add) override;
	void SubCompileMulDivST(TCompilationState& cs,bool Mul) override;
public:
	TSum(std::vector<std::string> Subexpressions);
    TSum() {};
    ~TSum();
    TExpression* CreateCopy() override;
	void EnumSubexpressions(std::function<bool(TExpression*)> f) override;
	bool SureEqual(TExpression* e) override;
    int GetTypeIndex() override { return 3; };
    bool IsEasyToCalculate() override;
    int Compare(TExpression* e) override;
    void PrepareToOptimize() override;
#ifdef _DEBUG
    std::string GetText() override;
#endif
};
class TProduct : public TExpression {
	const TProduct& operator=(const TProduct&) = delete;
	TProduct(const TProduct& v) = delete;
private:
    struct Term {
        TExpression* expr = nullptr;
        bool IsInverted = false;
    };
    static int CompareTerm(const Term& a, const Term& b);
    bool EqualTerm(const Term& a, const Term& b)
    {
        if (a.IsInverted == b.IsInverted && a.expr->SureEqual(b.expr)) return true;
        return false;
    }
	std::vector<Term> Subexpressions;
	void Clear();
public:
    void AddItem(TExpression* Term, bool IsInverted);
    void SubCompileLoadST(TCompilationState& cs,bool LastExpression) override;
	void SubCompileAddSubST(TCompilationState& cs,bool Add) override;
	void SubCompileMulDivST(TCompilationState& cs,bool Mul) override;
public:
    TProduct() {};
	TProduct(std::vector<std::string> Subexpressions);
	~TProduct();
    TExpression* CreateCopy() override;
	void EnumSubexpressions(std::function<bool(TExpression*)> f) override;
	bool SureEqual(TExpression* e) override;
    int GetTypeIndex() override { return 4; };
    bool IsEasyToCalculate() override;
    int Compare(TExpression* e) override;
    void PrepareToOptimize() override;
#ifdef _DEBUG
    std::string GetText() override;
#endif
};
class TFunctionCall : public TExpression {
    const TFunctionCall& operator=(const TFunctionCall&) = delete;
    TFunctionCall(const TFunctionCall& v) = delete;
    friend void TSum::PrepareToOptimize();
    friend void TProduct::PrepareToOptimize();
    friend void Optimize(TExpression* e,OptimData& od);
    friend TExpression* CreateMacro(const std::string& FunctionName,
        const std::vector<std::string>& args);
    void ProcessPower(); // ��� �����������
private:
    std::vector<TExpression*> Arguments;
    std::string FuncName;
    void Clear();
    //void SubCompileLoadST_withouthints(TCompilationState&);
    TFunctionCall(const std::string& FunctionName) : FuncName(FunctionName) {};
    TFunctionCall(const std::string& FunctionName, TExpression* arg); // ��� �����������  � ��������
    TFunctionCall(const std::string& FunctionName,
            std::vector<TExpression*>&& args)
        : Arguments(std::move(args)), FuncName(FunctionName) {};
public:
    void SubCompileLoadST(TCompilationState& cs,bool LastExpression) override;
    void SubCompileAddSubST(TCompilationState& cs, bool Add) override;
    void SubCompileMulDivST(TCompilationState& cs, bool Mul) override;
public:
    TFunctionCall(const std::string& FunctionName, const std::string& ArgList);
    TFunctionCall(const std::string& FunctionName, const std::vector<std::string>& args);
    ~TFunctionCall();
    TExpression* CreateCopy() override;
    std::string GetName() { return FuncName; };
    void EnumSubexpressions(std::function<bool(TExpression*)> f) override;
    bool SureEqual(TExpression* e) override;
    int GetTypeIndex() override { return 5; };
    bool IsEasyToCalculate() override;
    int Compare(TExpression* e) override;
    void PrepareToOptimize();
#ifdef _DEBUG
    std::string GetText() override;
#endif
};

class Program {
    std::vector<TExpression*> Lines;
	std::vector<std::string> ResultNames;
public:
    Program() {};
    Program(const Program&) = delete;
    Program operator=(const Program&) = delete;
    Program(Program&& p) noexcept
	{
		std::swap(Lines,p.Lines);
		std::swap(ResultNames, p.ResultNames);
	};
    Program(const std::string& text);
    ~Program() { for(TExpression* p : Lines) delete p; };

    void Add(const std::string& destname,TExpression *expr)
        {
			ResultNames.push_back(destname);
            Lines.push_back(expr);
        }

    size_t Compile(TCompilationState& cs);
	size_t GetLocalVariableCount()
        { return Lines.empty() ? 0 : Lines.size()-1; };
    //size_t GetOutputSize() { return Lines.size(); };
	//{ return Lines.empty() ? 0 : Lines.size()-1; };
};

TExpression* CreateMacro(const std::string& FunctionName,
                            const std::vector<std::string>& args);

TExpression* CreateExpression(const std::string& str);

struct OptimItem {
    std::set<TExpression*> ExprSet;
    bool IsLoaded = false;
};

struct OptimData {
    unsigned CompilerOptions;
    std::vector<OptimItem> IdenticalExpressions;
    bool Find(TExpression*,int&);
};

void Optimize(TExpression* e,OptimData& optdata);

