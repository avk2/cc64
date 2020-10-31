/*
		Math function compiler version 4.0
		File fcodegen64.cpp : the compiler's back end
		Platrorms supported : x86 / Win32 and Win64
		Author: Kavinov A.V.
*/
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <limits.h>
#include "expr.h"

/* В 64-битном режиме стек должен быть выравнен по 16 байт */
constexpr int StackFrameSize= 80;	// Место под "внутренние" переменные
									// д.б. кратно 16 байтам
constexpr int8_t default_cw_offset = 0; // do not change
constexpr int8_t CW_save_offset = 2;
constexpr int8_t temp_word1_offset = 4;
constexpr int8_t temp_int1_offset = 6;
constexpr int8_t float_const_offset = 10;
constexpr int8_t int_const_offset = 16;
constexpr int8_t temp_double_offset = 24;
constexpr int8_t double_const_offset = 32;
constexpr int8_t arg1_offset = 40;
constexpr int8_t arg2_offset = 48;
constexpr int8_t arg3_offset = 56;
constexpr int8_t arg4_offset = 64;

#ifdef _WIN64
constexpr int BytesPerLocalVariable = 16;
#else
constexpr int BytesPerLocalVariable = 12;
#endif

#ifdef _WIN64
constexpr int BytesPerTempVariable = 16;
#else
constexpr int BytesPerTempVariable = 12;
#endif

void TCompilationState::start_code(size_t LocalVarCount)
{
		// Ф-ция записывает код, котоpым всегда
		// должна начинаться генеpиpуемая пpоцедуpа
		// Обязательно вызывается в самом начале
#ifdef _WIN64
		// адpес-паpаметp уже в rcx
		// если функция векторная, то адрес результата в rdx
		// не забудем про выравнивание по 16 байт
		// mov [rsp+8],rcx
	//PutBytes(0x48, 0x89, 0x4C, 0x24, 0x08);
    PutByte(0x57);  // push rdi
	PutByte(0x55);			// push   rbp
    PutBytes(0x48, 0x83, 0xEC, StackFrameSize + 32);	// sub  rsp,StackFrameSize+32
    PutBytes(0x48, 0x89, 0xE5);	// mov   rbp, rsp
    if (LocalVarCount > 0)
    {
        // sub rsp,LocalVarCount*BytesPerLocalVariable
        PutBytes(0x48, 0x81, 0xEC);
        PutDword(unsigned(LocalVarCount) * BytesPerLocalVariable);
    }
#else
	PutByte(0x55);			// push   ebp
		// Загpузим адpес-паpаметp в ecx
		// Запишем mov ecx,[esp+8]
	PutBytes(0x8B, 0x4C, 0x24, 8);
	/*
	if (VectorResult)
	{
		// Загpузим адpес результата в edx
		// Запишем mov edx,[esp+12]
		PutBytes(0x8B, 0x54, 0x24, 12);
	}*/
    PutBytes(0x83, 0xEC, StackFrameSize);	// sub  esp,StackFrameSize
    PutBytes(0x89, 0xE5);	// mov   ebp, esp
    if (LocalVarCount > 0)
    {
            // sub esp,LocalVarCount*BytesPerLocalVariable
        PutBytes(0x81, 0xEC);
        PutDword(LocalVarCount * BytesPerLocalVariable);
    }
#endif
		// Запишем fnstcw [r/ebp+CW_save_offset]
	PutBytes(0xD9, 0x7D, CW_save_offset);
        // Запишем mov word ptr [r/ebp+default_cw_offset],0x37f (default_cw_offset==0)
    PutBytes(0x66, 0xC7, 0x45, default_cw_offset);
    PutBytes(0x7F, 0x03);
        // Запишем fldcw [r/ebp+default_cw_offset] (default_cw_offset==0)
    PutBytes(0xD9, 0x6D, default_cw_offset);
}
void TCompilationState::end_code()
{
		// Ф-ция записывает код, котоpым всегда
		// должна заканчиваться генеpиpуемая пpоцедуpа
		// Запишем fldcw [r/ebp+CW_save_offset]
	PutBytes(0xD9,0x6D, CW_save_offset);
#ifdef _WIN64
    // переложим из FP0 в XMM0
    // fstp  qword ptr[rbp +temp_double_offset]
    PutBytes(0xDD, 0x5D, temp_double_offset);
    // movsd   xmm0, [rbp+temp_double_offset]
    PutBytes(0xF2, 0x0F, 0x10, 0x45, temp_double_offset);
    // add rbp,StackFrameSize+32
    PutBytes(0x48, 0x83, 0xC5, StackFrameSize+32);
#else
        // add ebp,StackFrameSize
    PutBytes(0x83, 0xC5, StackFrameSize);
#endif
		// leave
	PutByte(0xC9);
#ifdef _WIN64
    PutByte(0x5F);  // pop rdi
#endif
		// ret near
	PutByte(0xC3);
}

void TCompilationState::alloc_tmp_space(size_t TempVarCount)
{
#ifdef _WIN64
        // mov rdi,rsp
    PutBytes(0x48, 0x89, 0xE7);
    // sub rsp,TempVarCount*BytesPerTempVariable
    PutBytes(0x48,0x81, 0xEC);
    PutDword(unsigned(TempVarCount*BytesPerTempVariable));
#else
        // push e/rdi
    PutByte(0x57);
    // mov edi,esp
    PutBytes(0x89, 0xE7);
    // sub esp,TempVarCount*BytesPerTempVariable
    PutBytes(0x81, 0xEC);
    PutDword(TempVarCount*BytesPerTempVariable);
#endif
}
void TCompilationState::free_tmp_space()
{

#ifdef _WIN64
        // mov rsp,rdi
    PutBytes(0x48, 0x89, 0xFC);
#else
        // mov esp,edi
    PutBytes(0x89, 0xFC);
        // pop e/rdi
    PutByte(0x5F);
#endif
}

void TCompilationState::save_temp_var(int index)
{
    bool StackFlag = StackRegsUsed + 1 > StackSize;
    if(StackFlag) // мало места в стеке
    {
            // fstp tbyte ptr [edi-(index+1)*BytesPerTempVariable]
        PutBytes(0xDB, 0xBF);
        PutDword(-(index + 1)*BytesPerTempVariable);
        StackRegsUsed--;
        load_temp_var(index);
    }
    else
    {       // место есть
        fld_st0();
            // fstp tbyte ptr [edi-(index+1)*BytesPerTempVariable]
        PutBytes(0xDB, 0xBF);
        PutDword(-(index + 1)*BytesPerTempVariable);
        StackRegsUsed--;
    }
}
void TCompilationState::load_temp_var(int index)
{
   	if(++StackRegsUsed>StackSize) throw ECStackExceeded();
            // fld tbyte ptr [edi-(index+1)*BytesPerTempVariable]
    PutBytes(0xDB,0xAF);
    PutDword(-(index+1)*BytesPerTempVariable);
}
void TCompilationState::addsub_temp_var(int index,bool Add)
{
    if (StackRegsUsed>=StackSize) throw ECStackExceeded();
    load_temp_var(index);
    if (Add) faddp_st1_st0();
    else fsubp_st1_st0();
}
void TCompilationState::muldiv_temp_var(int index,bool Mul)
{
    if (StackRegsUsed>=StackSize) throw ECStackExceeded();
    load_temp_var(index);
    if (Mul) fmulp_st1_st0();
    else fdivp_st1_st0();
}

/*
void TCompilationState::fsave_to_stack()
{
#ifdef _WIN64
		// sub rsp,112
	PutBytes(0x48, 0x83, 0xEC, 0x70);
#else
		// sub esp,108
	PutBytes(0x83, 0xEC, 0x6C);
#endif
		// fnsave [e/rsp]
	PutBytes(0xDD, 0x34, 0x24);
}
//	PutBytes();
void TCompilationState::frstor_from_stack()
{
		// frstor [e/rsp]
	PutBytes(0xDD, 0x24, 0x24);
#ifdef _WIN64
		// add rsp,112
	PutBytes(0x48, 0x83, 0xC4, 0x70);
#else
		// add esp,108
	PutBytes(0x83, 0xC4, 0x6C);
#endif
}
*/

void TCompilationState::_move_st0_to_stack_64bit()
{
		// form x87 stack to x86 stack
#ifdef _WIN64
		// sub rsp,16
	PutBytes(0x48,0x83,0xEC,0x08);
#else
		// sub esp,8
	PutBytes(0x83, 0xEC, 0x08);
#endif
		// fstp qword ptr [rsp]
	PutBytes(0xDD,0x1C,0x24);
	StackRegsUsed--;
}

void TCompilationState::_move_stack_top_to_st0_64bit()
{
		// form x86 stack to x87 stack
		// fld qword ptr [r/esp]
	PutBytes(0xDD,0x04,0x24);
#ifdef _WIN64
		// add rsp,16
	PutBytes(0x48,0x83,0xC4,0x08);
#else
		// add esp,8
	PutBytes(0x83, 0xC4, 0x08);
#endif
	StackRegsUsed++;
}

constexpr int8_t arg_offset[4] = { arg1_offset , arg2_offset , arg3_offset , arg4_offset };

void TCompilationState::movsd_xmmX_offset_bp(int8_t X, int8_t storenum)
{
	PutBytes(0xF2, 0x0F, 0x10);
	switch (X)
	{
	case 0: PutByte(0x45); break;
	case 1: PutByte(0x4d); break;
	case 2: PutByte(0x55); break;
	case 3: PutByte(0x5d); break;
	default: throw ECInternalError();
	}
	PutByte(arg_offset[storenum]);
}
void TCompilationState::fstp_to_arg_store(int storenum)
{
	if (storenum < 0 || storenum >= 4) throw ECInternalError();
		// fstp qword ptr [ebp+arg_offset[storenum]]
	PutBytes(0xDD, 0x5D, arg_offset[storenum]);
	StackRegsUsed--;
}
void TCompilationState::push_from_arg_store(int storenum)
{
	if (storenum < 0 || storenum >= 4) throw ECInternalError();
#ifdef _WIN64
		// push qword ptr [rbp + arg_offset[storenum]]
	PutBytes(0xFF, 0x75, arg_offset[storenum]);
#else
		// push dword ptr [ebp+arg_offset[storenum]+4]
	PutBytes(0xFF, 0x75, arg_offset[storenum] + 4);
		// push dword ptr [ebp+arg_offset[storenum]]
	PutBytes(0xFF, 0x75, arg_offset[storenum]);
#endif
}

void TCompilationState::_move_st0_to_stack_80bit()
{
		// form x87 stack to x86 stack, 80bit
#ifdef _WIN64
        // 16-bytes align
		// sub rsp,16
	PutBytes(0x48,0x83,0xEC,0x10);
#else
		// sub esp,12
	PutBytes(0x83, 0xEC, 0x0C);
#endif
		// fstp tbyte ptr [r/esp]
	PutBytes(0xDB, 0x3C, 0x24);
	StackRegsUsed--;
}
void TCompilationState::_move_stack_top_to_st0_80bit()
{
		// form x86 stack to x87 stack
		// fld tbyte ptr [r/esp]
	PutBytes(0xDB, 0x2C, 0x24);
#ifdef _WIN64
		// add rsp,16
	PutBytes(0x48,0x83,0xC4,0x10);
#else
		// add esp,12
	PutBytes(0x83, 0xC4, 0x0C);
#endif
	StackRegsUsed++;
}
//PutBytes();
void TCompilationState::CallLibraryFunction(LibraryFunction1Arg f,int ArgNum,
    bool Savex87Stack,bool LastExpression)
{
	if (ArgNum <= 0 || ArgNum>4) throw ECInternalError();
	/*if (VectorResult)
	{
		PutByte(0x50); // push r/eax (для выравнивания)
		PutByte(0x52); // push r/edx
	}*/
#ifdef _WIN64
    PutByte(0x51); // push rcx
	for (int8_t i = 0; i < ArgNum - 1; i++) movsd_xmmX_offset_bp(i,i);
	fstp_to_arg_store(ArgNum - 1);
	movsd_xmmX_offset_bp(ArgNum - 1, ArgNum - 1);
    if (Savex87Stack)
    {
            // sub rsp,112
        PutBytes(0x48, 0x83, 0xEC, 0x70);
            // fnsave [rsp]
        PutBytes(0xDD, 0x34, 0x24);
    }
		// sub rsp,32
	PutBytes(0x48, 0x83, 0xEC, 0x20);
		// mov rax,f
	PutBytes(0x48, 0xB8);
	PutQword(reinterpret_cast<unsigned long long>(f));
		// call rax
	PutBytes(0xFF, 0xD0);
		// add rsp,32
	PutBytes(0x48, 0x83, 0xC4, 0x20);
    if (Savex87Stack)
    {
            // frstor [rsp]
        PutBytes(0xDD, 0x24, 0x24);
            // add rsp,112
        PutBytes(0x48, 0x83, 0xC4, 0x70);
    }
		// movsd qword ptr [rbp+temp_double_offset],xmm0
	PutBytes(0xF2, 0x0F, 0x11, 0x45, temp_double_offset);
		// fld qword ptr [rbp+temp_double_offset]
	PutBytes(0xDD, 0x45, temp_double_offset);
	PutByte(0x59); // pop rcx
	StackRegsUsed++;
#else
	if(!LastExpression) PutByte(0x51); // push ecx
	int RegsToSave = StackRegsUsed-1;
	if (RegsToSave > 0)
	{
			// В стеке сопроцессора есть ещё что-то, кроме аргумента
			// Тогда надо его сохранить
			// fxch st(RegsToSave)
		PutBytes(0xD9, 0xC8+ RegsToSave);
		for (int i = 0; i < RegsToSave; i++) _move_st0_to_stack_80bit();
	}
		// последний аргумент лежит в стеке сопроцессора
		// переложим его куда следует
		// sub esp,8
	PutBytes(0x83, 0xEC,  8);
		// fstp qword ptr [esp]
	PutBytes(0xDD, 0x5C, 0x24, 0);
	StackRegsUsed--;
		// кладём в стек остальные аргументы
	for (int i = ArgNum-2; i >=0 ; i--)	push_from_arg_store(i);
		// Запишем fldcw [rbp+CW_save_offset]
	PutBytes(0xD9, 0x6D, CW_save_offset);
		// mov eax,f
	PutByte(0xB8);
	PutDword(reinterpret_cast<unsigned int>(f));
		// call eax
	PutBytes(0xFF, 0xD0);

	// Запишем fldcw [rbp+default_cw_offset]
	if(!LastExpression) PutBytes(0xD9, 0x6D, default_cw_offset);

		// add esp,ArgNum*8
	if(!LastExpression || RegsToSave > 0) PutBytes(0x83, 0xC4, ArgNum * 8);
	if (RegsToSave > 0)
	{
		for (int i = 0; i < RegsToSave; i++) _move_stack_top_to_st0_80bit();
		// fxch st(RegsToSave)
		PutBytes(0xD9, 0xC8 + RegsToSave);
	}
	if(!LastExpression) PutByte(0x59); // pop ecx
	StackRegsUsed++;
#endif
	/*if (VectorResult)
	{
		PutByte(0x5A); // pop r/edx
		PutByte(0x58); // pop r/eax
	}*/
}
void TCompilationState::CallUserFunction(UserFunctionTableEntry* pufs,bool LastExpression)
{
    const int ArgNum = pufs->ArgumentNumber;
    CallLibraryFunction(pufs->Function, ArgNum,
        pufs->CallOptions & CALLOPT_64BIT_SAVEx87,LastExpression);
}
void TCompilationState::fxch_st1()
{
		// fxch st(1)
	PutBytes(0xD9,0xC9);
}
void TCompilationState::fchs()
{
		// Записывает в буфеp инстpукцию fchs
	PutBytes(0xD9,0xE0);
}
void TCompilationState::load_const_bp_double(double cd)
{
		// Функция записывает в буфеp код,
		// загpужающий константу в qword ptr [rbp+double_const_offset]
		// Если в [rbp+double_const_offset] уже есть эта константа,
		// то ничего не делаем
	if(IsDoubleLoaded && LastLoadedDouble==cd) return;
		// Запишем mov dword ptr [e/rbp+double_const_offset],...
	PutBytes(0xC7,0x45,double_const_offset);
		// Загрузка числа, первая часть
	uint32_t *dwp= (uint32_t*)&cd;
	PutDword(*dwp);
		// Запишем mov uint32_t ptr [rbp+double_const_offset+4],...
	PutBytes(0xC7,0x45, double_const_offset+4);
		// Загрузка числа, вторая часть
	dwp= ((uint32_t*)&cd)+1;
	PutDword(*dwp);
	LastLoadedDouble= cd;
	IsDoubleLoaded= true;
}
void TCompilationState::load_const_bp_float(float cf)
{
		// Функция записывает в буфеp код,
		// загpужающий константу в dword ptr [rbp-22]
		// Если в [rbp-22] уже есть эта константа,
		// то ничего не делаем
	if(IsFloatLoaded && LastLoadedFloat==cf) return;
		// Запишем mov dword ptr [rbp+float_const_offset],...
	PutBytes(0xC7,0x45,float_const_offset);
		// Загрузка числа
	uint32_t *dwp= (uint32_t*)&cf;
	PutDword(*dwp);
	LastLoadedFloat= cf;
	IsFloatLoaded= true;
}
void TCompilationState::load_const_bp_int32(int n)
{
		// Функция записывает в буфеp код,
		// загpужающий целую константу в uint32_t ptr [e/rbp+int_const_offset]
    if (IsIntLoaded && LastLoadedInt == n) return;
		// Запишем mov dword ptr [e/rbp+int_const_offset],...
	PutBytes(0xC7,0x45, int_const_offset);
		// Загрузка числа, вторая часть
	uint32_t* dwp= ((uint32_t*)&n);
	PutDword(*dwp);
    LastLoadedInt= n;
    IsIntLoaded= true;
}

void TCompilationState::load_const_st0(double cd)
{
		// Функция записывает в буфеp код,
		// загpужающий константу в ST(0)
	if(++StackRegsUsed>StackSize) throw ECStackExceeded();
	if(cd==0)
	{
			// Если эта константа-ноль,
			// То запишем fldz
		PutBytes(0xD9,0xEE);
		return;
	}
	if(cd==1. || cd==-1.)
	{
			// fld1
		PutBytes(0xD9,0xE8);
		if(cd<0) fchs();
		return;
	}
	if(cd==2. || cd==-2.)
	{

			// fld1
		PutBytes(0xD9,0xE8);
			// fadd st,st
		PutBytes(0xDC,0xC0);
		if(cd<0) fchs();
		return;
	}

		// Какая-то дpугая константа
		// загpузим ее чеpез стек
	if(double(int(cd))==cd)
	{
		load_const_bp_int32(int(cd));
			// fild dword ptr [e/rbp+int_const_offset]
		PutBytes(0xDB,0x45, int_const_offset);
		return;
	}
	if(double(float(cd))==cd)
	{
		load_const_bp_float(float(cd));
			// fld dword ptr [e/rbp+float_const_offset]
		PutBytes(0xD9,0x45, float_const_offset);
	}
	else
	{
		load_const_bp_double(cd);
			// Запишем fld qword ptr [rbp+double_const_offset]
		PutBytes(0xDD,0x45, double_const_offset);
	}
}
void TCompilationState::add_sub_st0_const(double cnst,bool add)
{
		// Генерируем код, прибавляющий/вычитающий из ST(0) константу
	if(cnst==0.) return;
	if(StackRegsUsed+1<StackSize) // есть ещё место в стеке
	{
		if(::fabs(cnst)==1. || ::fabs(cnst)==2.)
		{
				// оптимизация сложения с 1,-1,2,-2
			//if(cnst<0 && add) {	cnst= -cnst; add= !add;	}
			load_const_st0(cnst);
			if(add) faddp_st1_st0();
			else fsubp_st1_st0();
			return;
		}
	}
		// места в стеке нет или смысла в оптимизации нет
	if(double(int(cnst))==cnst)
	{
			// Если константа целая, то можно всё короче сделать
		load_const_bp_int32(int(cnst));
			// fiadd dword ptr [e/rbp+int_const_offset]/fisub dword ptr [e/rbp+int_const_offset]
		if(add) PutBytes(0xDA,0x45, int_const_offset);
		else PutBytes(0xDA,0x65, int_const_offset);
		return;
	}
	if(double(float(cnst))==cnst)
	{
		load_const_bp_float(float(cnst));
			// fadd uint32_t ptr [e/rbp+float_const_offset]/fsub uint32_t ptr [e/rbp+float_const_offset]
		if(add) PutBytes(0xD8,0x45, float_const_offset);
		else PutBytes(0xD8,0x65, float_const_offset);
	}
	else
	{
		load_const_bp_double(cnst);
			// Запишем начало fadd/fsub qword ptr [rbp+double_const_offset]
		PutByte(0xDC);
		if(add) PutByte(0x45); 	// fadd
		else PutByte(0x65);		// fsub
			// Запишем конец fadd/fsub qword ptr [rbp+double_const_offset]
		PutByte(double_const_offset);
	}
}
void TCompilationState::mul_div_st0_const(double cnst,bool mul)
{
		// Генерируем код, умножающий/делящий ST(0) на константу
		// Некоторые простые оптимизации
		// Умножение на ноль оптимизировать нельзя!
	if(cnst==1.) return;
	if(cnst==-1.) {	fchs();	return;	}
	if(::fabs(cnst)==2. && mul)
	{
					// fadd st,st
		PutBytes(0xDC,0xC0);
		if(cnst<0) fchs();
		return;
	}

	if(double(int(cnst))==cnst)
	{
			// Если константа целая, то можно всё короче сделать
		load_const_bp_int32(int(cnst));
			// fimul dword ptr [e/rbp+int_const_offset]/fidiv dword ptr [e/rbp+int_const_offset]
		if(mul) PutBytes(0xDA,0x4D, int_const_offset);
		else PutBytes(0xDA,0x75, int_const_offset);
		return;
	}
	if(double(float(cnst))==cnst)
	{
		load_const_bp_float(float(cnst));
			// fmul uint32_t ptr [e/rbp+float_const_offset]/fdiv uint32_t ptr [e/rbp+float_const_offset]
		if(mul) PutBytes(0xD8,0x4D, float_const_offset);
		else PutBytes(0xD8,0x75, float_const_offset);
	}
	else
	{
		load_const_bp_double(cnst);
			// Запишем fmul/fdiv qword ptr [rbp+double_const_offset]
		if(mul) PutBytes(0xDC,0x4D, double_const_offset);
		else PutBytes(0xDC,0x75, double_const_offset);
	}
}
void TCompilationState::fldpi()
{
	if(++StackRegsUsed>StackSize) throw ECStackExceeded();
	PutBytes(0xD9,0xEB);
}
void TCompilationState::load_var_st0(int index)
{
		// Генерируем код, который загружает
		// внешнюю переменную номер index в ST(0)
	if(++StackRegsUsed>StackSize) throw ECStackExceeded();
   if(!index)
   {
   		// Первая переменная
			// Запишем fld qword ptr [r/ecx]
		PutBytes(0xDD,0x01);
   }
   else
	{
			// Запишем fld qword ptr [r/ecx+8*index]
		PutBytes(0xDD,0x81);
		PutDword(8*index);
	}
}
void TCompilationState::add_sub_st0_var(int index,bool add)
{
		// Генерируем код,
		// прибавляющий/вычитающий из ST(0) внешнюю переменную
   if(!index)
   {
	  		// Первая переменная
		PutByte(0xDC);
		if(add) 	// Запишем fadd qword ptr [r/ecx]
		{
			PutByte(0x01);
		}
		else		// Запишем fsub qword ptr [r/ecx]
		{
			PutByte(0x21);
		}
   }
   else
   {
		PutByte(0xDC);
		if(add) 	// Запишем fadd qword ptr [r/ecx+8*index]
		{
			PutByte(0x81);
		}
		else		// Запишем fsub qword ptr [r/ecx+8*index]
		{
			PutByte(0xA1);
		}
		PutDword(8*index);
   }
}
void TCompilationState::mul_div_st0_var(int index,bool mul)
{
		// Генерируем код,
		// умножающий ST(0) на внешнюю переменную
   if(!index)
   {
	  		// Первая переменная
		PutByte(0xDC);
		if(mul) 	// Запишем fmul qword ptr [r/ecx]
		{
			PutByte(0x09);
		}
		else		// Запишем fdiv qword ptr [r/ecx]
		{
			PutByte(0x31);
		}
   }
   else
   {
		PutByte(0xDC);
		if(mul) 	// Запишем fmul qword ptr [r/ecx+8*index]
		{
			PutByte(0x89);
		}
		else		// Запишем fdiv qword ptr [r/ecx+8*index]
		{
			PutByte(0xB1);
		}
		PutDword(8*index);
   }
}
/*
void TCompilationState::StoreResult(int index)
{
		// fstp qword ptr [e/rdx+index*8]
	PutBytes(0xDD, 0x9A);
	PutDword(8*index);
	StackRegsUsed--;
}
*/
void TCompilationState::StoreLocalVariable(int index)
{
        // fstp tbyte ptr [e/rbp-(index+1)*BytesPerLocalVariable]
    PutBytes(0xDB, 0xBD);
    PutDword(unsigned(-(index + 1)*BytesPerLocalVariable));
    StoredLocalVariableIndices.insert(index);
    StackRegsUsed--;
}

int TCompilationState::ArgIndex(const std::string& Name)
{
    std::vector<std::string>::const_iterator it = std::find(ArgNames.begin(), ArgNames.end(), Name);
    if (it == ArgNames.end()) return -1;
    return (int)distance(ArgNames.begin(), it);
}


int TCompilationState::GetLocalVariableIndex(const std::string& name)
{
    if (name.empty() || !LocalVariableNameTable) return -1;
    auto it = std::find(LocalVariableNameTable->begin(),
		LocalVariableNameTable->end(),name);

	if (it != LocalVariableNameTable->end())
	{
		int index = (int)std::distance(LocalVariableNameTable->begin(), it);
		//if (StoredLocalVariableIndices.contains(index)) return index;
        if (StoredLocalVariableIndices.find(index)!=
            StoredLocalVariableIndices.end()) return index;
		else return -1;
	}
	else return -1;
}
/*
void TCompilationState::SaveToUserVariable(int index)
{
		// moves st(0) to the user variable
	if (!index)
	{
		// Первая переменная
			// Запишем fstp qword ptr [r/ecx]
		PutBytes(0xDD, 0x19);
	}
	else
	{
		// Запишем fstp qword ptr [r/ecx+8*index]
		PutBytes(0xDD, 0x99);
		PutDword(8 * index);
	}
	StackRegsUsed--;
}
*/
void TCompilationState::load_local_var_st0(int index)
{
    // Генерируем код, который загружает
    // локальную переменную номер index в ST(0)
    if (++StackRegsUsed>StackSize) throw ECStackExceeded();
        // Записали ли мы раньше эту переменную?
    if (StoredLocalVariableIndices.find(index) == StoredLocalVariableIndices.end())
        throw ECBadVariableUsage(std::string("local variable index= ")+Int2Str(index));
            // Запишем fld tbyte ptr [e/rbp-(index+1)*BytesPerLocalVariable]
    PutBytes(0xDB, 0xAD);
    PutDword(unsigned(-(index + 1)*BytesPerLocalVariable));
}
void TCompilationState::add_sub_st0_local_var(int index, bool add)
{
                // Генерируем код,
                // прибавляющий/вычитающий локальную переменную из ST(0)
    bool StackFlag = StackRegsUsed + 1 > StackSize;
    if (StackFlag) move_st1_to_stack();
    // Записали ли мы раньше эту переменную?
    if (StoredLocalVariableIndices.find(index) == StoredLocalVariableIndices.end())
        throw ECBadVariableUsage(std::string("local variable index= ") + Int2Str(index));
    load_local_var_st0(index);
    if (add) faddp_st1_st0();
    else fsubp_st1_st0();

    if (StackFlag) move_stack_top_to_st1();
}
void TCompilationState::mul_div_st0_local_var(int index, bool mul)
{
                // Генерируем код,
                // умножающий/делящий ST(0) на локальную переменную номер index
    bool StackFlag = StackRegsUsed + 1 > StackSize;
    if (StackFlag) move_st1_to_stack();
    // Записали ли мы раньше эту переменную?
    if (StoredLocalVariableIndices.find(index) == StoredLocalVariableIndices.end())
        throw ECBadVariableUsage(std::string("local variable index= ") + Int2Str(index));
    load_local_var_st0(index);
    if (mul) fmulp_st1_st0();
    else fdivp_st1_st0();

    if (StackFlag) move_stack_top_to_st1();
}
void TCompilationState::fld1()
{
		// Запишем fld1
	PutBytes(0xD9,0xE8);
	StackRegsUsed++;
}
void TCompilationState::fld_st0()
{
		// Запишем fld st(0)
	PutBytes(0xD9,0xC0);
	StackRegsUsed++;
}
void TCompilationState::faddp_st1_st0()
{
		// Запишем faddp ST(1),ST
	PutBytes(0xDE,0xC1);
	StackRegsUsed--;
}
void TCompilationState::fsubp_st1_st0()
{
		// Запишем fsubp ST(1),ST
	PutBytes(0xDE,0xE9);
	StackRegsUsed--;
}
void TCompilationState::fsubrp_st1_st0()
{
		// Запишем fsubrp ST(1),ST
	PutBytes(0xDE,0xE1);
	StackRegsUsed--;
}
void TCompilationState::fmulp_st1_st0()
{
		// Запишем fmulp ST(1),ST
	PutBytes(0xDE,0xC9);
	StackRegsUsed--;
}
void TCompilationState::fdivp_st1_st0()
{
		// Запишем fdivp ST(1),ST
	PutBytes(0xDE,0xF9);
	StackRegsUsed--;
}
void TCompilationState::fsqrt()
{
	// Запишем fsqrt
	PutBytes(0xD9, 0xFA);
}
void TCompilationState::fcbrt()
{
	// Кубический корень с учётом знака
#ifdef _WIN64
	bool StackFlag = StackRegsUsed + 1 > StackSize;
	if (StackFlag)	// нужно 1 место в стеке
	{
		move_st1_to_stack();
	}
	// Запишем fldz
	PutBytes(0xD9, 0xEE);
		// Запишем fucomip st,st(1)
	PutBytes(0xDF, 0xE9);
		// jz $+47
	PutBytes(0x74, 0x2d);
	// Запишем cmc
	PutByte(0xF5);
	if (StackFlag) move_stack_top_to_st1();
#else
	// Запишем ftst
	PutBytes(0xD9, 0xE4);
	// Запишем fstsw ax
	PutBytes(0x9B, 0xDF, 0xE0);
	// Запишем sahf
	PutByte(0x9E);  // C0->CF - сохраним знак
	// jz $+46
	PutBytes(0x74, 0x2c);
#endif
	fabs();	// уберём знак
	flog2();
	mul_div_st0_const(3., false);
	fexp2();
		// восстановим знак
	// jc $+4 (в конец)
	PutBytes(0x73, 0x02);
	fchs();
}
void TCompilationState::fsquare()
{
	// Запишем fmul st,st
	PutBytes(0xDC, 0xC8);
}
void TCompilationState::fabs()
{
	// Запишем fabs
	PutBytes(0xD9, 0xE1);
}
// Тригонометрия начало
void TCompilationState::fsin()
{
	if (CompilerOptions & CO_TRIGFUNCTIONSFROMCRT)
	{
		CallLibraryFunction(sin);
	}
	else
	{
		// Запишем fsin
		PutBytes(0xD9, 0xFE);
	}
}
void TCompilationState::fcos()
{
	if (CompilerOptions & CO_TRIGFUNCTIONSFROMCRT)
	{
		CallLibraryFunction(cos);
	}
	else
	{
		// Запишем fcos
		PutBytes(0xD9,0xFF);
	}
}
void TCompilationState::ftan()
{
	if (CompilerOptions & CO_TRIGFUNCTIONSFROMCRT)
	{
		CallLibraryFunction(tan);
	}
	else
	{
        bool StackFlag = StackRegsUsed + 1 > StackSize;
        if (StackFlag) move_st1_to_stack();
		    //  fptan
		PutBytes(0xD9, 0xF2);
			// Запишем fstp st0
		PutBytes(0xDD, 0xD8);
        if (StackFlag) move_stack_top_to_st1();
	}
}
static double cot(double x) noexcept
{
	return 1/tan(x);
}
void TCompilationState::finv()
{
    bool StackFlag = StackRegsUsed + 1 > StackSize;
    if (StackFlag) move_st1_to_stack();
		// Запишем fld1
	PutBytes(0xD9, 0xE8);
		// Запишем fdivrp st(1),st
    PutBytes(0xDE, 0xF1);
    if (StackFlag) move_stack_top_to_st1();
}
void TCompilationState::fcot()
{
	if (CompilerOptions & CO_TRIGFUNCTIONSFROMCRT)
	{
		CallLibraryFunction(cot);
	}
	else
	{
        bool StackFlag = StackRegsUsed + 1 > StackSize;
        if (StackFlag) move_st1_to_stack();
			// Запишем fptan
		PutBytes(0xD9, 0xF2);
			// Запишем fdivrp st(1),st
		PutBytes(0xDE, 0xF1);
        if (StackFlag) move_stack_top_to_st1();
	}
}
void TCompilationState::fatan()
{
	if (CompilerOptions & CO_TRIGFUNCTIONSFROMCRT)
	{
		CallLibraryFunction(atan);
	}
	else
	{
        bool StackFlag = StackRegsUsed + 1 > StackSize;
        if (StackFlag) move_st1_to_stack();
			// Запишем fld1
		PutBytes(0xD9, 0xE8);
			// Запишем fpatan
		PutBytes(0xD9, 0xF3);
        if (StackFlag) move_stack_top_to_st1();
	}
}
void TCompilationState::factg()
{
	fatan();
    bool StackFlag = StackRegsUsed + 2 > StackSize;
    if (StackFlag)
    {
        move_st1_to_stack();
        move_st1_to_stack();
    }
	fldpi();
	load_const_st0(2.0);
	fdivp_st1_st0();
	fsubrp_st1_st0();
    if (StackFlag)
    {
        move_stack_top_to_st1();
        move_stack_top_to_st1();
    }
}
void TCompilationState::fasin()
{
	if (CompilerOptions & CO_TRIGFUNCTIONSFROMCRT)
	{
		CallLibraryFunction(asin);
	}
	else
	{
		bool StackFlag = StackRegsUsed + 1 > StackSize;

		if (StackFlag) move_st1_to_stack();
		// Запишем fld st
		PutBytes(0xD9, 0xC0);
		StackRegsUsed++;
		// Запишем fmul st,st
		PutBytes(0xDC, 0xC8);
		if (StackRegsUsed + 1 > StackSize)
		{		// мало места в стеке
				// Записываем инстpукцию fchs
			fchs();
			//
			add_sub_st0_const(1., true);
		}
		else
		{		// места полно
				// Запишем fld1
			PutBytes(0xD9, 0xE8);
			// fsubrp st(1),st
			PutBytes(0xDE, 0xE1);
		}
		fsqrt();
		// Запишем fpatan
		PutBytes(0xD9, 0xF3);
		StackRegsUsed--;
		if (StackFlag) move_stack_top_to_st1();
	}
}
void TCompilationState::facos()
{
	if (CompilerOptions & CO_TRIGFUNCTIONSFROMCRT)
	{
		CallLibraryFunction(acos);
	}
	else
	{
        bool StackFlag = StackRegsUsed + 2 > StackSize;
        if (StackFlag)
        {
            move_st1_to_stack();
            move_st1_to_stack();
        }

		fasin();

		fldpi();
		load_const_st0(2.0);
		fdivp_st1_st0();
		fsubrp_st1_st0();
        if (StackFlag)
        {
            move_stack_top_to_st1();
            move_stack_top_to_st1();
        }
	}
}
// Тригонометрия конец

// экспонета и т.п. начало
void TCompilationState::flog2()
{
        // Вычисляем  логаpифм
    bool StackFlag = StackRegsUsed + 1 > StackSize;
	if (StackFlag) move_st1_to_stack();
		// Запишем fld1
	PutBytes(0xD9, 0xE8);
		// Запишем fxch
    PutBytes(0xD9, 0xC9);
		// Запишем fyl2x
	PutBytes(0xD9, 0xF1);
	if (StackFlag) move_stack_top_to_st1();
}

void TCompilationState::fln()
{
		// Вычисляем нат. логаpифм по ф-ле ln(a)= log2(a)/log2(e)
	bool StackFlag = StackRegsUsed + 1 > StackSize;
	if (StackFlag) move_st1_to_stack();
		// Запишем FLDLN2
    PutBytes(0xD9, 0xED);
		// Запишем fxch
    PutBytes(0xD9, 0xC9);
		// Запишем fyl2x
	PutBytes(0xD9, 0xF1);
	if (StackFlag) move_stack_top_to_st1();
}
//
void TCompilationState::fexp2()
{
	bool StackFlag = StackRegsUsed + 2 > StackSize;
	if (StackFlag)  // здесь возможна оптимизация
	{
		move_st1_to_stack();
		move_st1_to_stack();
	}
		// Разделим на целое и дpобное
		// Создадим втоpой экземпляp показателя 2х
		// Запишем fld st(0)
	PutBytes(0xD9, 0xC0);
		// 2-й экземпляp округлим
		// Запишем frndint
	PutBytes(0xD9, 0xFC);
		// Получили целую часть
		// А в 1-й экземпляp запишем дpобную часть
		// Запишем fsub st(1),st(0)
	PutBytes(0xDC, 0xE9);
		// Вычислим 2^(ц.ч.)
		// Запишем fld1
	PutBytes(0xD9, 0xE8);
		// Запишем fscale
	PutBytes(0xD9, 0xFD);
		// Запишем fstp st(1)
	PutBytes(0xDD, 0xD9);
		// Запишем fxch st(1)
	PutBytes(0xD9, 0xC9);
		// Запишем f2xm1
	PutBytes(0xD9, 0xF0);
		// Запишем fld1
	PutBytes(0xD9, 0xE8);
		// Запишем faddp st(1),st
	PutBytes(0xDE, 0xC1);
		// Запишем fmulp st(1)
	PutBytes(0xDE, 0xC9);
	if (StackFlag)
	{
		move_stack_top_to_st1();
		move_stack_top_to_st1();
	}
}
void TCompilationState::fexp()
{
		// Вычисляем экспоненту по ф-ле exp(a)= 2^(a*log2e)
	bool StackFlag = StackRegsUsed + 2 > StackSize;
	if (StackFlag)  // для fexp2 всё равно надо 2 регистра
	{
		move_st1_to_stack();
		move_st1_to_stack();
	}
		// Запишем fldl2e
    PutBytes(0xD9, 0xEA);
        // Запишем fmulp ST(1),ST
    PutBytes(0xDE, 0xC9);
    fexp2();
    if (StackFlag)
	{
		move_stack_top_to_st1();
		move_stack_top_to_st1();
	}
}
void TCompilationState::fpow()
{
        // Запишем fyl2x
    PutBytes(0xD9, 0xF1);
    StackRegsUsed--;
    fexp2();
}
// экспонета и т.п. конец

// гиперболические функции начало
void TCompilationState::fsinh_from_exp()
{
    bool StackFlag = StackRegsUsed + 1 > StackSize;
    if (StackFlag) move_st1_to_stack();

    fld1();
    PutBytes(0xD8, 0xF1);	// fdiv st,st(1)
    fsubp_st1_st0();
    load_const_st0(2.);
    fdivp_st1_st0();
    if (StackFlag) move_stack_top_to_st1();
}
void TCompilationState::fcosh_from_exp()
{
    bool StackFlag = StackRegsUsed + 1 > StackSize;
    if (StackFlag) move_st1_to_stack();

    fld1();
    PutBytes(0xD8, 0xF1);	// fdiv st,st(1)
    faddp_st1_st0();
    load_const_st0(2.);
    fdivp_st1_st0();

    if (StackFlag) move_stack_top_to_st1();
}
void TCompilationState::ftanh_from_exp()
{
    bool StackFlag = StackRegsUsed + 2 > StackSize;
    if (StackFlag)
    {		// здесь возможна оптимизация
        move_st1_to_stack();
        move_st1_to_stack();
    }

    fsquare();
    fld_st0();
    fld1();
    PutBytes(0xDE, 0xE9);   // fsubp st1,st
    StackRegsUsed--;
    fld1();
    PutBytes( 0xDE, 0xC2 );   // faddp st2,st
    StackRegsUsed--;
    PutBytes(0xDE, 0xF1);	// fdivrp st(1),st
    StackRegsUsed--;

    if (StackFlag)
    {
        move_stack_top_to_st1();
        move_stack_top_to_st1();
    }
}

// гиперболические функции конец

void TCompilationState::fdivrp_st1_st()
{
	PutBytes(0xDE,0xF1); // fdivrp st(1),st
	StackRegsUsed--;
}

void TCompilationState::fmod()
{
    // Запишем fprem
    PutBytes(0xD9, 0xF8);
    fxch_st1();
    // Запишем fstp st
    PutBytes(0xDD, 0xD8);
    StackRegsUsed--;
}

void TCompilationState::heaviside()
{ // fxam
	bool StackFlag = StackRegsUsed + 1>StackSize;
	if (StackFlag)	// нужно 1 место в стеке
	{
		move_st1_to_stack();
	}
#ifdef _WIN64
	// Запишем fldz
	PutBytes(0xD9, 0xEE);
	fxch_st1();
	// Запишем fucomip st,st(1)
	PutBytes(0xDF, 0xE9);
#else
    // Запишем ftst
    PutBytes(0xD9, 0xE4);
    // Запишем fstsw ax
    PutBytes(0x9B, 0xDF, 0xE0);
    // Запишем fstp st
    PutBytes(0xDD, 0xD8);
    // Запишем fldz
    PutBytes(0xD9, 0xEE);
    // Запишем sahf
    PutByte(0x9E);  // C0->CF
#endif
    // jc $+6 (в конец)
    PutBytes(0x72, 0x04);
    // Запишем fstp st
    PutBytes(0xDD, 0xD8);
    // Запишем fld1
    PutBytes(0xD9, 0xE8);
    if (StackFlag) move_stack_top_to_st1();
}

void TCompilationState::fsign()
{
		// Вычисляем знак выpажения (-1,0,1)
#ifdef _WIN64
	bool StackFlag = StackRegsUsed + 1>StackSize;
	if (StackFlag)	// нужно 1 место в стеке
	{
		move_st1_to_stack();
	}
		// Запишем fldz
	PutBytes(0xD9, 0xEE);
	fxch_st1();
		// Запишем fucomip st,st(1)
	PutBytes(0xDF, 0xE9);
		// jz $+10 в конец
	PutBytes(0x74, 0x08);
		// Запишем fcomp
	PutBytes(0xD8, 0xD9);
		// Запишем fld1
	PutBytes(0xD9, 0xE8);
		// jnc $+4
	PutBytes(0x73, 0x02);
		// Запишем fchs
	PutBytes(0xD9, 0xE0);

	if (StackFlag) move_stack_top_to_st1();
#else
    // Запишем ftst
    PutBytes(0xD9, 0xE4);
    // Запишем fstsw ax
    PutBytes(0x9B, 0xDF, 0xE0);
    // Запишем sahf
    PutByte(0x9E);
    // Запишем jp $+16
    PutBytes(0x7A, 0x0E);
    // Запишем fcomp
    PutBytes(0xD8, 0xD9);
    // Запишем jnz $+6
    PutBytes(0x75, 0x04);
    // C3 == 1, значит аpгумент = 0
    // Запишем fldz
    PutBytes(0xD9, 0xEE);
    // Запишем jmp $+8
    PutBytes(0xEB, 0x06);
    // C3 == 0, значит аpгумент != 0
    // Запишем fld1
    PutBytes(0xD9, 0xE8);
    // Пpовеpим C0
    // Запишем jnc $+4
    PutBytes(0x73, 0x02);
    // Запишем fchs
    PutBytes(0xD9, 0xE0);
#endif
}
void TCompilationState::fmin()
{
#ifdef _WIN64
    // Запишем fucomi st(1)
    PutBytes(0xDB, 0xE9);
#else
    // Запишем fcom st(1)
    PutBytes(0xD8, 0xD1);
    // Запишем fstsw ax
    PutBytes(0x9B, 0xDF, 0xE0);
    // Запишем sahf
    PutByte(0x9E);
#endif
		// Пpовеpим C0
		// Запишем jnc $+4
	PutBytes(0x73,0x02);
	fxch_st1();
		// Запишем fstp st
	PutBytes(0xDD,0xD8);
	StackRegsUsed--;
}
void TCompilationState::fmax()
{
#ifdef _WIN64
    // Запишем fucomi st(1)
    PutBytes(0xDB, 0xE9);
#else
    // Запишем fcom st(1)
    PutBytes(0xD8, 0xD1);
    // Запишем fstsw ax
    PutBytes(0x9B, 0xDF, 0xE0);
    // Запишем sahf
    PutByte(0x9E);
#endif
    // Пpовеpим CF
    // Запишем jc $+4
    PutBytes(0x72, 0x02);
	fxch_st1();
		// Запишем fstp st
	PutBytes(0xDD, 0xD8);
	StackRegsUsed--;
}
void TCompilationState::fround()
{
		// Запишем frndint
	PutBytes(0xD9,0xFC);
}
void TCompilationState::fceil()
{
        // mov word ptr [e/rbp+temp_word1_offset],0x0B7F
    PutBytes(0x66, 0xC7, 0x45,temp_word1_offset);
    PutBytes(0x7F, 0x0B);
		// Запишем fldcw [rbp+temp_word1_offset]
	PutBytes(0xD9,0x6D, temp_word1_offset);
		// Запишем frndint
	PutBytes(0xD9,0xFC);
		// Запишем fldcw [rbp+default_cw_offset]
	PutBytes(0xD9,0x6D,default_cw_offset);
}
void TCompilationState::ffloor()
{
        // mov word ptr [e/rbp+temp_word1_offset],0x077F
    PutBytes(0x66, 0xC7, 0x45, temp_word1_offset);
    PutBytes(0x7F, 0x07);
		// Запишем fldcw [rbp+temp_word1_offset]
	PutBytes(0xD9, 0x6D, temp_word1_offset);
		// Запишем frndint
	PutBytes(0xD9,0xFC);
		// Запишем fldcw [rbp+default_cw_offset]
	PutBytes(0xD9, 0x6D, default_cw_offset);
}
void TCompilationState::fint()
{
        // mov word ptr [e/rbp+temp_word1_offset],0x0F7F
    PutBytes(0x66, 0xC7, 0x45, temp_word1_offset);
    PutBytes(0x7F, 0x0F);
		// Запишем fldcw [rbp+temp_word1_offset]
	PutBytes(0xD9, 0x6D, temp_word1_offset);
		// Запишем frndint
	PutBytes(0xD9,0xFC);
		// Запишем fldcw [rbp+default_cw_offset]
	PutBytes(0xD9, 0x6D, default_cw_offset);
}
