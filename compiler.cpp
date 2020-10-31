/*
        Math function compiler version 4.0
        File compiler.cpp : the ompiler's middleend
        Platrorms supported : x86 / Win32 and Win64
        (c) Kavinov A.V. 1998-2002,2011,2019
*/
#include <algorithm>
#include <math.h>
#include <cstring>
#include "expr.h"

#ifdef _DEBUG
#include <iostream>
#endif

using namespace std;

static UserFunctionTableEntry ExtendedFunctionTable[] = {
    { "erf",    erf },
    { "erfc",   erfc },
    { "gamma",  tgamma },
    { "lgamma", lgamma },
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
    { "beta" ,  beta},
    { "zeta", riemann_zeta },
    { "Ei", expint},
    { "I", cyl_bessel_i },
    { "J", cyl_bessel_j },
    { "K", cyl_bessel_k },
    { "Y", cyl_neumann },
#endif
    { "",       nullptr } };


size_t Program::Compile(TCompilationState& cs)
{
    cs.SetLocalVariableTableName(&ResultNames);
    cs.start_code(GetLocalVariableCount());	// Код всегда начинается с "заголовка" - запишем его
    size_t i = 0;
    if (Lines.empty()) throw EFEmptyExpression{};
    while (i<Lines.size()-1)
    {
        Lines[i]->Compile(cs,false);
        cs.StoreLocalVariable(int(i++));
    }
    Lines[i]->Compile(cs,true);
    cs.end_code();	// Код всегда заканчивается "хвостом"
    if(cs.StackAvailable()!=cs.StackSize-1) throw ECInternalError();
	return cs.i;	// Возвращаем размер кода
}

unsigned TExpression::Compile(TCompilationState& cs,bool LastExpression)
{
    OptimData od{ cs.CompilerOptions,{} };
    if (!(cs.CompilerOptions & CO_DONTOPTIMIZE)) Optimize(this,od);
    cs.SetOptimData(&od);
    const int TempVarCount =
        (!od.IdenticalExpressions.empty()) ?
        int(od.IdenticalExpressions.size()) : 0;

	//if(WritePrologEpilog) cs.start_code();	// Код всегда начинается с "заголовка" - запишем его
    if(TempVarCount) cs.alloc_tmp_space(TempVarCount);
    LoadST0(cs, LastExpression);
    if(TempVarCount) cs.free_tmp_space();
    //if (WritePrologEpilog) cs.end_code();	// Код всегда заканчивается "хвостом"
    cs.SetOptimData(nullptr);
	if(cs.StackAvailable()!=cs.StackSize-1) throw ECInternalError();
	return cs.i;	// Возвращаем размер кода
}

void TExpression::LoadST0(TCompilationState& cs,bool LastExpression)
{
    int stored_index = -1000;
    if (!(cs.CompilerOptions & CO_DONTOPTIMIZE) && cs.pOptimData &&
       cs.pOptimData->Find(this,stored_index))
    {
        if(cs.pOptimData->IdenticalExpressions[stored_index].IsLoaded)
        {
            //cout<<"optim load: "<<GetText()<<endl;
            cs.load_temp_var(stored_index);
        }
        else
        {
            SubCompileLoadST(cs,false);
            cs.save_temp_var(stored_index);
            //cout<<"optim save: "<<GetText()<<endl;
            cs.pOptimData->IdenticalExpressions[stored_index].IsLoaded= true;
        }
    }
    else
    {
        SubCompileLoadST(cs,LastExpression);
    }
}
void TExpression::AddSubST0(TCompilationState& cs,bool Add)
{
    int stored_index = -1000;
    if (!(cs.CompilerOptions & CO_DONTOPTIMIZE) && cs.pOptimData &&
       cs.pOptimData->Find(this,stored_index))
    {
        bool StackFlag= cs.StackAvailable()<1;
        if(StackFlag) cs.move_st1_to_stack();
        if(cs.pOptimData->IdenticalExpressions[stored_index].IsLoaded)
        {
            cs.addsub_temp_var(stored_index,Add);
        }
        else
        {
            SubCompileLoadST(cs,false);
            cs.save_temp_var(stored_index);
            //cout<<"optim save: "<<GetText()<<endl;
            if(Add) cs.faddp_st1_st0();
            else cs.fsubp_st1_st0();
            cs.pOptimData->IdenticalExpressions[stored_index].IsLoaded= true;
        }
        if(StackFlag) cs.move_stack_top_to_st1();
        //cout<<"optim add: "<<GetText()<<endl;
    }
    else SubCompileAddSubST(cs,Add);
}
void TExpression::MulDivST0(TCompilationState& cs,bool Mul)
{
    int stored_index = -1000;
    if (!(cs.CompilerOptions & CO_DONTOPTIMIZE) && cs.pOptimData &&
       cs.pOptimData->Find(this,stored_index))
    {
        bool StackFlag= cs.StackAvailable()<1;
        if(StackFlag) cs.move_st1_to_stack();
        if(cs.pOptimData->IdenticalExpressions[stored_index].IsLoaded)
        {
            cs.muldiv_temp_var(stored_index,Mul);
        }
        else
        {
            SubCompileLoadST(cs,false);
            cs.save_temp_var(stored_index);
            //cout<<"optim save: "<<GetText()<<endl;
            if(Mul) cs.fmulp_st1_st0();
            else cs.fdivp_st1_st0();
            cs.pOptimData->IdenticalExpressions[stored_index].IsLoaded= true;
        }
        if(StackFlag) cs.move_stack_top_to_st1();
        //cout<<"optim add: "<<GetText()<<endl;
    }
    else SubCompileMulDivST(cs,Mul);
}

void TConstant::SubCompileLoadST(TCompilationState& cs,bool LastExpression)
{
	cs.load_const_st0(GetDouble());
}
void TConstant::SubCompileAddSubST(TCompilationState& cs,bool Add)
{
	cs.add_sub_st0_const(GetDouble(),Add);
}
void TConstant::SubCompileMulDivST(TCompilationState& cs,bool Mul)
{
    if (GetDouble() == 1.) return;
	else cs.mul_div_st0_const(GetDouble(),Mul);
}


// TVariable

void TVariable::SubCompileLoadST(TCompilationState& cs,bool LastExpression)
{
    if (Name.empty()) throw ECInternalError();
	if(Name=="pi") cs.fldpi();
    else
    {
        if (Name[0] == '$') cs.load_local_var_st0(atoi(&Name[1])); // локальная переменная по номеру
        else
        {
                // Либо локальная переменная, либо аргумент
            int LocalIndex = cs.GetLocalVariableIndex(Name);
            if(LocalIndex >=0) cs.load_local_var_st0(LocalIndex);
			else
			{
				int ArgIndex = cs.ArgIndex(Name);
				if (ArgIndex >= 0) cs.load_var_st0(ArgIndex);
				else throw ECUndefinedSymbol(Name);
			}
        }
    }
}
void TVariable::SubCompileAddSubST(TCompilationState& cs,bool Add)
{
	if(Name=="pi")
	{
	    bool StackFlag= cs.StackAvailable()<1;
		if(StackFlag) cs.move_st1_to_stack();
		cs.fldpi();
		if(Add) cs.faddp_st1_st0();
		else cs.fsubp_st1_st0();
        if(StackFlag) cs.move_stack_top_to_st1();
	}
    else
    {
        if (Name[0] == '$') cs.add_sub_st0_local_var(atoi(&Name[1]), Add);
        else
        {
            // Либо локальная переменная, либо аргумент
            int LocalIndex = cs.GetLocalVariableIndex(Name);
            if (LocalIndex >= 0) cs.add_sub_st0_local_var(LocalIndex,Add);
			else
			{
				int ArgIndex = cs.ArgIndex(Name);
				if (ArgIndex >= 0) cs.add_sub_st0_var(cs.ArgIndex(Name), Add);
				else throw ECUndefinedSymbol(Name);
			}
        }
    }
}
void TVariable::SubCompileMulDivST(TCompilationState& cs,bool Mul)
{
	if(Name=="pi")
	{
	    bool StackFlag= cs.StackAvailable()<1;
		if(StackFlag) cs.move_st1_to_stack();
        cs.fldpi();
        if(Mul) cs.fmulp_st1_st0();
        else cs.fdivp_st1_st0();
        if(StackFlag) cs.move_stack_top_to_st1();

	}
    else
    {
        if (Name[0] == '$') cs.mul_div_st0_local_var(atoi(&Name[1]),Mul);
        else
        {
            // Либо локальная переменная, либо аргумент
            int LocalIndex = cs.GetLocalVariableIndex(Name);
            if (LocalIndex >= 0) cs.mul_div_st0_local_var(LocalIndex, Mul);
            else
			{
				int ArgIndex = cs.ArgIndex(Name);
				if (ArgIndex >= 0) cs.mul_div_st0_var(cs.ArgIndex(Name), Mul);
				else throw ECUndefinedSymbol(Name);
			}

        }
    }
}

// TSum
void TSum::SubCompileLoadST(TCompilationState& cs,bool LastExpression)
{
    if(Subexpressions.empty()) throw ECInternalError();
    auto i= Subexpressions.begin();
        // Самое пеpвое выpажение отдельно компилиpуем
	TConstant* pc= dynamic_cast<TConstant*>(i->expr);
	if(pc && i->IsInverted)
	{
		cs.load_const_st0(-pc->GetDouble());	//  Маленькая оптимизация
	}
	else
	{
		i->expr->LoadST0(cs,false);
		if(i->IsInverted) cs.fchs();
	}

        // Первое выражение загрузили, дальше пошли прибавлять и вычитать
	for(++i;i!=Subexpressions.end();++i)
    {
		i->expr->AddSubST0(cs,!(i->IsInverted));
    }
}
void TSum::SubCompileAddSubST(TCompilationState& cs,bool Add)
{
    if(Subexpressions.empty()) throw ECInternalError();
    auto i= Subexpressions.begin();
	for(;i!=Subexpressions.end();++i)
    {
		i->expr->AddSubST0(cs,(!i->IsInverted)==Add);
    }
}
void TSum::SubCompileMulDivST(TCompilationState& cs,bool Mul)
{
    if(Subexpressions.empty()) throw ECInternalError();
    bool StackFlag= cs.StackAvailable()<1;
    if(StackFlag) cs.move_st1_to_stack();
		LoadST0(cs,false);
		if(Mul) cs.fmulp_st1_st0();
		else cs.fdivp_st1_st0();
    if(StackFlag) cs.move_stack_top_to_st1();
}

#ifdef _DEBUG
std::string TSum::GetText()
{
    std::string ret="(";
    auto i = Subexpressions.begin();
    for (; i != Subexpressions.end(); ++i)
    {
        ret += (i->IsInverted) ? "-" : (i == Subexpressions.begin() ? "" : "+");
        ret += i->expr->GetText();
    }
    return ret+")";
}
#endif

// TProduct
void TProduct::SubCompileLoadST(TCompilationState& cs,bool LastExpression)
{
    if(Subexpressions.empty()) throw ECInternalError();
    auto i= Subexpressions.begin();
        // Самое пеpвое выpажение отдельно компилиpуем
	if(i->IsInverted)
	{
		cs.load_const_st0(1.);
		i->expr->MulDivST0(cs,false);
	}
	else i->expr->LoadST0(cs,false);

        // Первое выражение загрузили, дальше пошли умножать и делить
	for(++i;i!=Subexpressions.end();++i)
    {
        i->expr->MulDivST0(cs,!(i->IsInverted));
    }
}
void TProduct::SubCompileMulDivST(TCompilationState& cs,bool Mul)
{
    if(Subexpressions.empty()) throw ECInternalError();
    auto i= Subexpressions.begin();
	for(;i!=Subexpressions.end();++i)
    {
        i->expr->MulDivST0(cs,(!i->IsInverted)==Mul);
    }
}
void TProduct::SubCompileAddSubST(TCompilationState& cs,bool Add)
{
    if(Subexpressions.empty()) throw ECInternalError();
    bool StackFlag= cs.StackAvailable()<1;
    if(StackFlag) cs.move_st1_to_stack();
		LoadST0(cs,false);
		if(Add) cs.faddp_st1_st0();
		else cs.fsubp_st1_st0();
    if(StackFlag) cs.move_stack_top_to_st1();
}

#ifdef _DEBUG
std::string TProduct::GetText()
{
    std::string ret="(";
    auto i = Subexpressions.begin();
    for (; i != Subexpressions.end(); ++i)
    {
        ret += (i->IsInverted) ? "/" : (i== Subexpressions.begin() ? "" : "*");
        ret += i->expr->GetText();
    }
    return ret+")";
}
#endif
/*
// TFunctionCall
inline bool IsInteger(double d)
{
	return double((int)d)==d;
}
*/
UserFunctionTableEntry* TCompilationState::FindUserFunction(const char* FuncName)
{
	if (!FuncName || *FuncName == 0) return nullptr;
        // Сначала ищем в пользовательских функциях
    if(FunctionTable)
    {
        for (UserFunctionTableEntry* uftep = FunctionTable;
            uftep->FunctionName && *(uftep->FunctionName);uftep++)
        {
            if (!strcmp(FuncName, uftep->FunctionName)) return uftep;
        }
    }
    if(!(CompilerOptions & CO_DISABLEEXTENDEDFUNCTIONS))
    {
        for (UserFunctionTableEntry* uftep = ExtendedFunctionTable;
            uftep->FunctionName && *(uftep->FunctionName);uftep++)
        {
            if (!strcmp(FuncName, uftep->FunctionName)) return uftep;
        }
    }
	return nullptr;
}
void TFunctionCall::SubCompileLoadST(TCompilationState& cs,bool LastExpression)
{
		// когда вызываются функции xxx::SubCompileLoadLoadST, предполагается, что  1 регистр стека есть
		// - для аргумента в нашем случае. Об этом заботится вызывающая функция
		// Если функции (например, atan) нужны ещё регистры и их нет, она себе их освободит сама
    size_t argnum = Arguments.size();
		// проверим сначала, нет ли пользовательской функции с таким именем
	if (UserFunctionTableEntry* ufs = cs.FindUserFunction(FuncName.c_str()); ufs)
	{
		if(Arguments.empty()) throw EFInvalidFunctionCall(FuncName);
		if(argnum>4u || int(argnum)!=ufs->ArgumentNumber)
            throw EFInvalidArgumentNumber(FuncName,int(argnum),ufs->ArgumentNumber);
		if (argnum == 1)
		{
				// один аргумент - всё просто
			Arguments[0]->LoadST0(cs,false);
			cs.CallUserFunction(ufs,LastExpression);
			return;
		}
			// несколько аргументов. всё сложнее
			// последний аргумент должен быть в стеке сопроцессора,
			// предыдущие сохранены в стековом фрейме
		for (int8_t i = 0; size_t(i) < argnum - 1; i++)
		{
			Arguments[i]->LoadST0(cs,false);
			cs.fstp_to_arg_store(i);
		}
		Arguments[argnum-1]->LoadST0(cs,false);
			// Дальше CallUserFunction знает, что делать
        cs.CallUserFunction(ufs,LastExpression);
		return;

	}
		// Встроенная функция
	if(argnum==1)
	{
		Arguments[0]->LoadST0(cs,false);

		if(FuncName=="abs") { cs.fabs(); return; }
		if(FuncName=="sign") { cs.fsign(); return; }
		if(FuncName=="round") { cs.fround(); return; }
		if(FuncName=="ceil") { cs.fceil(); return; }
		if(FuncName=="floor") { cs.ffloor(); return; }
		if(FuncName=="int") { cs.fint(); return; }
		if(FuncName=="sqrt") { cs.fsqrt(); return; }
        if(FuncName=="cbrt") { cs.fcbrt(); return; }
		if(FuncName=="square" || FuncName == "sqr") { cs.fsquare(); return; }
		if(FuncName=="sin"  || FuncName ==  "$1sin") { cs.fsin(); return; }
		if(FuncName=="cos" || FuncName == "$1cos")	{ cs.fcos(); return; }
		if(FuncName=="tan" || FuncName=="tg") { cs.ftan(); return; }
		//if(FuncName=="cot" || FuncName=="ctg") { cs.fcot(); return; }
		if(FuncName=="asin" || FuncName=="arcsin") { cs.fasin(); return; }
		if(FuncName=="acos" || FuncName=="arccos") { cs.facos(); return; }
		if(FuncName=="atan" || FuncName=="arctg") { cs.fatan(); return; }
		if(FuncName=="acot" || FuncName=="arcctg")  { cs.factg(); return; }
		if(FuncName=="log2") { cs.flog2(); return; }
		if(FuncName=="log" || FuncName=="ln") { cs.fln(); return; }
		if(FuncName=="exp2") { cs.fexp2(); return; }
		if(FuncName=="exp") { cs.fexp(); return; }
		if (FuncName == "heaviside" || FuncName == "heavi") { cs.heaviside(); return; }

            // внутренние функции
        if(FuncName=="$id") { return; }
        if(FuncName=="$inv") { cs.finv(); return; }
        if(FuncName=="$she") { cs.fsinh_from_exp(); return; }
        if(FuncName=="$che") { cs.fcosh_from_exp(); return; }
        if(FuncName=="$the") { cs.ftanh_from_exp(); return; }
	}
		// Для функций двух аргументов нужно иначе контролировать стек сопроцессора
		// чтобы хватило места для обоих аргументов
	if(argnum==2 && FuncName=="pow")
	{
		bool StackFlag= cs.StackAvailable()<2;
		if(StackFlag) cs.move_st0_to_stack();
		Arguments[1]->LoadST0(cs,false);
		Arguments[0]->LoadST0(cs,false);
		cs.fpow();
		if(StackFlag) {	cs.move_stack_top_to_st0(); cs.fxch_st1();	}
		return;
	}
	if(argnum==2 && FuncName=="mod")
	{
		bool StackFlag= cs.StackAvailable()<2;
		if(StackFlag) cs.move_st0_to_stack();
		Arguments[1]->LoadST0(cs,false);
		Arguments[0]->LoadST0(cs,false);
		cs.fmod();
		if(StackFlag) {	cs.move_stack_top_to_st0(); cs.fxch_st1();	}
		return;
	}
	if(argnum>=2 && FuncName=="min")
	{
		bool StackFlag= cs.StackAvailable()<2;
		if(StackFlag) cs.move_st0_to_stack();
		Arguments[0]->LoadST0(cs,false);
		for(unsigned int i= 1;i<argnum;i++)
		{
			Arguments[i]->LoadST0(cs,false);
			cs.fmin();
		}
		if(StackFlag) {	cs.move_stack_top_to_st0(); cs.fxch_st1();	}
		return;
	}
	if(argnum>=2 && FuncName=="max")
	{
		bool StackFlag= cs.StackAvailable()<2;
		if(StackFlag) cs.move_st0_to_stack();
		Arguments[0]->LoadST0(cs,false);
		for(unsigned int i= 1;i<argnum;i++)
		{
			Arguments[i]->LoadST0(cs,false);
			cs.fmax();
		}
		if(StackFlag) {	cs.move_stack_top_to_st0(); cs.fxch_st1();	}
		return;
	}
    throw EFFunctionNotFound(FuncName,argnum);
}
void TFunctionCall::SubCompileAddSubST(TCompilationState& cs,bool Add)
{
    bool StackFlag = cs.StackAvailable() <1;
    if(StackFlag) cs.move_st1_to_stack();
		LoadST0(cs,false);
		if(Add) cs.faddp_st1_st0();
		else cs.fsubp_st1_st0();
    if(StackFlag) cs.move_stack_top_to_st1();
}
void TFunctionCall::SubCompileMulDivST(TCompilationState& cs,bool Mul)
{
    bool StackFlag= cs.StackAvailable()<1;
    if(StackFlag) cs.move_st1_to_stack();
		LoadST0(cs,false);
		if(Mul) cs.fmulp_st1_st0();
		else cs.fdivp_st1_st0();
    if(StackFlag) cs.move_stack_top_to_st1();
}

#ifdef _DEBUG
std::string TFunctionCall::GetText()
{
    std::string ret = FuncName;
    ret += "(";
    ret += Arguments[0]->GetText();
    for (size_t i= 1; i <Arguments.size(); ++i)
    {
        ret += ",";
        ret += Arguments[i]->GetText();
    }
    ret += ")";
    return ret;
}
#endif

