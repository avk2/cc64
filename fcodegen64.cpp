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

/* � 64-������ ������ ���� ������ ���� �������� �� 16 ���� */
constexpr int StackFrameSize= 80;	// ����� ��� "����������" ����������
									// �.�. ������ 16 ������
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
		// �-��� ���������� ���, ����p�� ������
		// ������ ���������� ����p�p����� �p�����p�
		// ����������� ���������� � ����� ������
#ifdef _WIN64
		// ��p��-��p����p ��� � rcx
		// ���� ������� ���������, �� ����� ���������� � rdx
		// �� ������� ��� ������������ �� 16 ����
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
		// ���p���� ��p��-��p����p � ecx
		// ������� mov ecx,[esp+8]
	PutBytes(0x8B, 0x4C, 0x24, 8);
	/*
	if (VectorResult)
	{
		// ���p���� ��p�� ���������� � edx
		// ������� mov edx,[esp+12]
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
		// ������� fnstcw [r/ebp+CW_save_offset]
	PutBytes(0xD9, 0x7D, CW_save_offset);
        // ������� mov word ptr [r/ebp+default_cw_offset],0x37f (default_cw_offset==0)
    PutBytes(0x66, 0xC7, 0x45, default_cw_offset);
    PutBytes(0x7F, 0x03);
        // ������� fldcw [r/ebp+default_cw_offset] (default_cw_offset==0)
    PutBytes(0xD9, 0x6D, default_cw_offset);
}
void TCompilationState::end_code()
{
		// �-��� ���������� ���, ����p�� ������
		// ������ ������������� ����p�p����� �p�����p�
		// ������� fldcw [r/ebp+CW_save_offset]
	PutBytes(0xD9,0x6D, CW_save_offset);
#ifdef _WIN64
    // ��������� �� FP0 � XMM0
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
    if(StackFlag) // ���� ����� � �����
    {
            // fstp tbyte ptr [edi-(index+1)*BytesPerTempVariable]
        PutBytes(0xDB, 0xBF);
        PutDword(-(index + 1)*BytesPerTempVariable);
        StackRegsUsed--;
        load_temp_var(index);
    }
    else
    {       // ����� ����
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
		PutByte(0x50); // push r/eax (��� ������������)
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
			// � ����� ������������ ���� ��� ���-��, ����� ���������
			// ����� ���� ��� ���������
			// fxch st(RegsToSave)
		PutBytes(0xD9, 0xC8+ RegsToSave);
		for (int i = 0; i < RegsToSave; i++) _move_st0_to_stack_80bit();
	}
		// ��������� �������� ����� � ����� ������������
		// ��������� ��� ���� �������
		// sub esp,8
	PutBytes(0x83, 0xEC,  8);
		// fstp qword ptr [esp]
	PutBytes(0xDD, 0x5C, 0x24, 0);
	StackRegsUsed--;
		// ����� � ���� ��������� ���������
	for (int i = ArgNum-2; i >=0 ; i--)	push_from_arg_store(i);
		// ������� fldcw [rbp+CW_save_offset]
	PutBytes(0xD9, 0x6D, CW_save_offset);
		// mov eax,f
	PutByte(0xB8);
	PutDword(reinterpret_cast<unsigned int>(f));
		// call eax
	PutBytes(0xFF, 0xD0);

	// ������� fldcw [rbp+default_cw_offset]
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
		// ���������� � ����p ����p����� fchs
	PutBytes(0xD9,0xE0);
}
void TCompilationState::load_const_bp_double(double cd)
{
		// ������� ���������� � ����p ���,
		// ���p������� ��������� � qword ptr [rbp+double_const_offset]
		// ���� � [rbp+double_const_offset] ��� ���� ��� ���������,
		// �� ������ �� ������
	if(IsDoubleLoaded && LastLoadedDouble==cd) return;
		// ������� mov dword ptr [e/rbp+double_const_offset],...
	PutBytes(0xC7,0x45,double_const_offset);
		// �������� �����, ������ �����
	uint32_t *dwp= (uint32_t*)&cd;
	PutDword(*dwp);
		// ������� mov uint32_t ptr [rbp+double_const_offset+4],...
	PutBytes(0xC7,0x45, double_const_offset+4);
		// �������� �����, ������ �����
	dwp= ((uint32_t*)&cd)+1;
	PutDword(*dwp);
	LastLoadedDouble= cd;
	IsDoubleLoaded= true;
}
void TCompilationState::load_const_bp_float(float cf)
{
		// ������� ���������� � ����p ���,
		// ���p������� ��������� � dword ptr [rbp-22]
		// ���� � [rbp-22] ��� ���� ��� ���������,
		// �� ������ �� ������
	if(IsFloatLoaded && LastLoadedFloat==cf) return;
		// ������� mov dword ptr [rbp+float_const_offset],...
	PutBytes(0xC7,0x45,float_const_offset);
		// �������� �����
	uint32_t *dwp= (uint32_t*)&cf;
	PutDword(*dwp);
	LastLoadedFloat= cf;
	IsFloatLoaded= true;
}
void TCompilationState::load_const_bp_int32(int n)
{
		// ������� ���������� � ����p ���,
		// ���p������� ����� ��������� � uint32_t ptr [e/rbp+int_const_offset]
    if (IsIntLoaded && LastLoadedInt == n) return;
		// ������� mov dword ptr [e/rbp+int_const_offset],...
	PutBytes(0xC7,0x45, int_const_offset);
		// �������� �����, ������ �����
	uint32_t* dwp= ((uint32_t*)&n);
	PutDword(*dwp);
    LastLoadedInt= n;
    IsIntLoaded= true;
}

void TCompilationState::load_const_st0(double cd)
{
		// ������� ���������� � ����p ���,
		// ���p������� ��������� � ST(0)
	if(++StackRegsUsed>StackSize) throw ECStackExceeded();
	if(cd==0)
	{
			// ���� ��� ���������-����,
			// �� ������� fldz
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

		// �����-�� �p���� ���������
		// ���p���� �� ��p�� ����
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
			// ������� fld qword ptr [rbp+double_const_offset]
		PutBytes(0xDD,0x45, double_const_offset);
	}
}
void TCompilationState::add_sub_st0_const(double cnst,bool add)
{
		// ���������� ���, ������������/���������� �� ST(0) ���������
	if(cnst==0.) return;
	if(StackRegsUsed+1<StackSize) // ���� ��� ����� � �����
	{
		if(::fabs(cnst)==1. || ::fabs(cnst)==2.)
		{
				// ����������� �������� � 1,-1,2,-2
			//if(cnst<0 && add) {	cnst= -cnst; add= !add;	}
			load_const_st0(cnst);
			if(add) faddp_st1_st0();
			else fsubp_st1_st0();
			return;
		}
	}
		// ����� � ����� ��� ��� ������ � ����������� ���
	if(double(int(cnst))==cnst)
	{
			// ���� ��������� �����, �� ����� �� ������ �������
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
			// ������� ������ fadd/fsub qword ptr [rbp+double_const_offset]
		PutByte(0xDC);
		if(add) PutByte(0x45); 	// fadd
		else PutByte(0x65);		// fsub
			// ������� ����� fadd/fsub qword ptr [rbp+double_const_offset]
		PutByte(double_const_offset);
	}
}
void TCompilationState::mul_div_st0_const(double cnst,bool mul)
{
		// ���������� ���, ����������/������� ST(0) �� ���������
		// ��������� ������� �����������
		// ��������� �� ���� �������������� ������!
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
			// ���� ��������� �����, �� ����� �� ������ �������
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
			// ������� fmul/fdiv qword ptr [rbp+double_const_offset]
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
		// ���������� ���, ������� ���������
		// ������� ���������� ����� index � ST(0)
	if(++StackRegsUsed>StackSize) throw ECStackExceeded();
   if(!index)
   {
   		// ������ ����������
			// ������� fld qword ptr [r/ecx]
		PutBytes(0xDD,0x01);
   }
   else
	{
			// ������� fld qword ptr [r/ecx+8*index]
		PutBytes(0xDD,0x81);
		PutDword(8*index);
	}
}
void TCompilationState::add_sub_st0_var(int index,bool add)
{
		// ���������� ���,
		// ������������/���������� �� ST(0) ������� ����������
   if(!index)
   {
	  		// ������ ����������
		PutByte(0xDC);
		if(add) 	// ������� fadd qword ptr [r/ecx]
		{
			PutByte(0x01);
		}
		else		// ������� fsub qword ptr [r/ecx]
		{
			PutByte(0x21);
		}
   }
   else
   {
		PutByte(0xDC);
		if(add) 	// ������� fadd qword ptr [r/ecx+8*index]
		{
			PutByte(0x81);
		}
		else		// ������� fsub qword ptr [r/ecx+8*index]
		{
			PutByte(0xA1);
		}
		PutDword(8*index);
   }
}
void TCompilationState::mul_div_st0_var(int index,bool mul)
{
		// ���������� ���,
		// ���������� ST(0) �� ������� ����������
   if(!index)
   {
	  		// ������ ����������
		PutByte(0xDC);
		if(mul) 	// ������� fmul qword ptr [r/ecx]
		{
			PutByte(0x09);
		}
		else		// ������� fdiv qword ptr [r/ecx]
		{
			PutByte(0x31);
		}
   }
   else
   {
		PutByte(0xDC);
		if(mul) 	// ������� fmul qword ptr [r/ecx+8*index]
		{
			PutByte(0x89);
		}
		else		// ������� fdiv qword ptr [r/ecx+8*index]
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
		// ������ ����������
			// ������� fstp qword ptr [r/ecx]
		PutBytes(0xDD, 0x19);
	}
	else
	{
		// ������� fstp qword ptr [r/ecx+8*index]
		PutBytes(0xDD, 0x99);
		PutDword(8 * index);
	}
	StackRegsUsed--;
}
*/
void TCompilationState::load_local_var_st0(int index)
{
    // ���������� ���, ������� ���������
    // ��������� ���������� ����� index � ST(0)
    if (++StackRegsUsed>StackSize) throw ECStackExceeded();
        // �������� �� �� ������ ��� ����������?
    if (StoredLocalVariableIndices.find(index) == StoredLocalVariableIndices.end())
        throw ECBadVariableUsage(std::string("local variable index= ")+Int2Str(index));
            // ������� fld tbyte ptr [e/rbp-(index+1)*BytesPerLocalVariable]
    PutBytes(0xDB, 0xAD);
    PutDword(unsigned(-(index + 1)*BytesPerLocalVariable));
}
void TCompilationState::add_sub_st0_local_var(int index, bool add)
{
                // ���������� ���,
                // ������������/���������� ��������� ���������� �� ST(0)
    bool StackFlag = StackRegsUsed + 1 > StackSize;
    if (StackFlag) move_st1_to_stack();
    // �������� �� �� ������ ��� ����������?
    if (StoredLocalVariableIndices.find(index) == StoredLocalVariableIndices.end())
        throw ECBadVariableUsage(std::string("local variable index= ") + Int2Str(index));
    load_local_var_st0(index);
    if (add) faddp_st1_st0();
    else fsubp_st1_st0();

    if (StackFlag) move_stack_top_to_st1();
}
void TCompilationState::mul_div_st0_local_var(int index, bool mul)
{
                // ���������� ���,
                // ����������/������� ST(0) �� ��������� ���������� ����� index
    bool StackFlag = StackRegsUsed + 1 > StackSize;
    if (StackFlag) move_st1_to_stack();
    // �������� �� �� ������ ��� ����������?
    if (StoredLocalVariableIndices.find(index) == StoredLocalVariableIndices.end())
        throw ECBadVariableUsage(std::string("local variable index= ") + Int2Str(index));
    load_local_var_st0(index);
    if (mul) fmulp_st1_st0();
    else fdivp_st1_st0();

    if (StackFlag) move_stack_top_to_st1();
}
void TCompilationState::fld1()
{
		// ������� fld1
	PutBytes(0xD9,0xE8);
	StackRegsUsed++;
}
void TCompilationState::fld_st0()
{
		// ������� fld st(0)
	PutBytes(0xD9,0xC0);
	StackRegsUsed++;
}
void TCompilationState::faddp_st1_st0()
{
		// ������� faddp ST(1),ST
	PutBytes(0xDE,0xC1);
	StackRegsUsed--;
}
void TCompilationState::fsubp_st1_st0()
{
		// ������� fsubp ST(1),ST
	PutBytes(0xDE,0xE9);
	StackRegsUsed--;
}
void TCompilationState::fsubrp_st1_st0()
{
		// ������� fsubrp ST(1),ST
	PutBytes(0xDE,0xE1);
	StackRegsUsed--;
}
void TCompilationState::fmulp_st1_st0()
{
		// ������� fmulp ST(1),ST
	PutBytes(0xDE,0xC9);
	StackRegsUsed--;
}
void TCompilationState::fdivp_st1_st0()
{
		// ������� fdivp ST(1),ST
	PutBytes(0xDE,0xF9);
	StackRegsUsed--;
}
void TCompilationState::fsqrt()
{
	// ������� fsqrt
	PutBytes(0xD9, 0xFA);
}
void TCompilationState::fcbrt()
{
	// ���������� ������ � ������ �����
#ifdef _WIN64
	bool StackFlag = StackRegsUsed + 1 > StackSize;
	if (StackFlag)	// ����� 1 ����� � �����
	{
		move_st1_to_stack();
	}
	// ������� fldz
	PutBytes(0xD9, 0xEE);
		// ������� fucomip st,st(1)
	PutBytes(0xDF, 0xE9);
		// jz $+47
	PutBytes(0x74, 0x2d);
	// ������� cmc
	PutByte(0xF5);
	if (StackFlag) move_stack_top_to_st1();
#else
	// ������� ftst
	PutBytes(0xD9, 0xE4);
	// ������� fstsw ax
	PutBytes(0x9B, 0xDF, 0xE0);
	// ������� sahf
	PutByte(0x9E);  // C0->CF - �������� ����
	// jz $+46
	PutBytes(0x74, 0x2c);
#endif
	fabs();	// ����� ����
	flog2();
	mul_div_st0_const(3., false);
	fexp2();
		// ����������� ����
	// jc $+4 (� �����)
	PutBytes(0x73, 0x02);
	fchs();
}
void TCompilationState::fsquare()
{
	// ������� fmul st,st
	PutBytes(0xDC, 0xC8);
}
void TCompilationState::fabs()
{
	// ������� fabs
	PutBytes(0xD9, 0xE1);
}
// ������������� ������
void TCompilationState::fsin()
{
	if (CompilerOptions & CO_TRIGFUNCTIONSFROMCRT)
	{
		CallLibraryFunction(sin);
	}
	else
	{
		// ������� fsin
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
		// ������� fcos
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
			// ������� fstp st0
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
		// ������� fld1
	PutBytes(0xD9, 0xE8);
		// ������� fdivrp st(1),st
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
			// ������� fptan
		PutBytes(0xD9, 0xF2);
			// ������� fdivrp st(1),st
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
			// ������� fld1
		PutBytes(0xD9, 0xE8);
			// ������� fpatan
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
		// ������� fld st
		PutBytes(0xD9, 0xC0);
		StackRegsUsed++;
		// ������� fmul st,st
		PutBytes(0xDC, 0xC8);
		if (StackRegsUsed + 1 > StackSize)
		{		// ���� ����� � �����
				// ���������� ����p����� fchs
			fchs();
			//
			add_sub_st0_const(1., true);
		}
		else
		{		// ����� �����
				// ������� fld1
			PutBytes(0xD9, 0xE8);
			// fsubrp st(1),st
			PutBytes(0xDE, 0xE1);
		}
		fsqrt();
		// ������� fpatan
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
// ������������� �����

// ��������� � �.�. ������
void TCompilationState::flog2()
{
        // ���������  ����p���
    bool StackFlag = StackRegsUsed + 1 > StackSize;
	if (StackFlag) move_st1_to_stack();
		// ������� fld1
	PutBytes(0xD9, 0xE8);
		// ������� fxch
    PutBytes(0xD9, 0xC9);
		// ������� fyl2x
	PutBytes(0xD9, 0xF1);
	if (StackFlag) move_stack_top_to_st1();
}

void TCompilationState::fln()
{
		// ��������� ���. ����p��� �� �-�� ln(a)= log2(a)/log2(e)
	bool StackFlag = StackRegsUsed + 1 > StackSize;
	if (StackFlag) move_st1_to_stack();
		// ������� FLDLN2
    PutBytes(0xD9, 0xED);
		// ������� fxch
    PutBytes(0xD9, 0xC9);
		// ������� fyl2x
	PutBytes(0xD9, 0xF1);
	if (StackFlag) move_stack_top_to_st1();
}
//
void TCompilationState::fexp2()
{
	bool StackFlag = StackRegsUsed + 2 > StackSize;
	if (StackFlag)  // ����� �������� �����������
	{
		move_st1_to_stack();
		move_st1_to_stack();
	}
		// �������� �� ����� � �p�����
		// �������� ���p�� ��������p ���������� 2�
		// ������� fld st(0)
	PutBytes(0xD9, 0xC0);
		// 2-� ��������p ��������
		// ������� frndint
	PutBytes(0xD9, 0xFC);
		// �������� ����� �����
		// � � 1-� ��������p ������� �p����� �����
		// ������� fsub st(1),st(0)
	PutBytes(0xDC, 0xE9);
		// �������� 2^(�.�.)
		// ������� fld1
	PutBytes(0xD9, 0xE8);
		// ������� fscale
	PutBytes(0xD9, 0xFD);
		// ������� fstp st(1)
	PutBytes(0xDD, 0xD9);
		// ������� fxch st(1)
	PutBytes(0xD9, 0xC9);
		// ������� f2xm1
	PutBytes(0xD9, 0xF0);
		// ������� fld1
	PutBytes(0xD9, 0xE8);
		// ������� faddp st(1),st
	PutBytes(0xDE, 0xC1);
		// ������� fmulp st(1)
	PutBytes(0xDE, 0xC9);
	if (StackFlag)
	{
		move_stack_top_to_st1();
		move_stack_top_to_st1();
	}
}
void TCompilationState::fexp()
{
		// ��������� ���������� �� �-�� exp(a)= 2^(a*log2e)
	bool StackFlag = StackRegsUsed + 2 > StackSize;
	if (StackFlag)  // ��� fexp2 �� ����� ���� 2 ��������
	{
		move_st1_to_stack();
		move_st1_to_stack();
	}
		// ������� fldl2e
    PutBytes(0xD9, 0xEA);
        // ������� fmulp ST(1),ST
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
        // ������� fyl2x
    PutBytes(0xD9, 0xF1);
    StackRegsUsed--;
    fexp2();
}
// ��������� � �.�. �����

// ��������������� ������� ������
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
    {		// ����� �������� �����������
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

// ��������������� ������� �����

void TCompilationState::fdivrp_st1_st()
{
	PutBytes(0xDE,0xF1); // fdivrp st(1),st
	StackRegsUsed--;
}

void TCompilationState::fmod()
{
    // ������� fprem
    PutBytes(0xD9, 0xF8);
    fxch_st1();
    // ������� fstp st
    PutBytes(0xDD, 0xD8);
    StackRegsUsed--;
}

void TCompilationState::heaviside()
{ // fxam
	bool StackFlag = StackRegsUsed + 1>StackSize;
	if (StackFlag)	// ����� 1 ����� � �����
	{
		move_st1_to_stack();
	}
#ifdef _WIN64
	// ������� fldz
	PutBytes(0xD9, 0xEE);
	fxch_st1();
	// ������� fucomip st,st(1)
	PutBytes(0xDF, 0xE9);
#else
    // ������� ftst
    PutBytes(0xD9, 0xE4);
    // ������� fstsw ax
    PutBytes(0x9B, 0xDF, 0xE0);
    // ������� fstp st
    PutBytes(0xDD, 0xD8);
    // ������� fldz
    PutBytes(0xD9, 0xEE);
    // ������� sahf
    PutByte(0x9E);  // C0->CF
#endif
    // jc $+6 (� �����)
    PutBytes(0x72, 0x04);
    // ������� fstp st
    PutBytes(0xDD, 0xD8);
    // ������� fld1
    PutBytes(0xD9, 0xE8);
    if (StackFlag) move_stack_top_to_st1();
}

void TCompilationState::fsign()
{
		// ��������� ���� ��p������ (-1,0,1)
#ifdef _WIN64
	bool StackFlag = StackRegsUsed + 1>StackSize;
	if (StackFlag)	// ����� 1 ����� � �����
	{
		move_st1_to_stack();
	}
		// ������� fldz
	PutBytes(0xD9, 0xEE);
	fxch_st1();
		// ������� fucomip st,st(1)
	PutBytes(0xDF, 0xE9);
		// jz $+10 � �����
	PutBytes(0x74, 0x08);
		// ������� fcomp
	PutBytes(0xD8, 0xD9);
		// ������� fld1
	PutBytes(0xD9, 0xE8);
		// jnc $+4
	PutBytes(0x73, 0x02);
		// ������� fchs
	PutBytes(0xD9, 0xE0);

	if (StackFlag) move_stack_top_to_st1();
#else
    // ������� ftst
    PutBytes(0xD9, 0xE4);
    // ������� fstsw ax
    PutBytes(0x9B, 0xDF, 0xE0);
    // ������� sahf
    PutByte(0x9E);
    // ������� jp $+16
    PutBytes(0x7A, 0x0E);
    // ������� fcomp
    PutBytes(0xD8, 0xD9);
    // ������� jnz $+6
    PutBytes(0x75, 0x04);
    // C3 == 1, ������ �p������ = 0
    // ������� fldz
    PutBytes(0xD9, 0xEE);
    // ������� jmp $+8
    PutBytes(0xEB, 0x06);
    // C3 == 0, ������ �p������ != 0
    // ������� fld1
    PutBytes(0xD9, 0xE8);
    // �p���p�� C0
    // ������� jnc $+4
    PutBytes(0x73, 0x02);
    // ������� fchs
    PutBytes(0xD9, 0xE0);
#endif
}
void TCompilationState::fmin()
{
#ifdef _WIN64
    // ������� fucomi st(1)
    PutBytes(0xDB, 0xE9);
#else
    // ������� fcom st(1)
    PutBytes(0xD8, 0xD1);
    // ������� fstsw ax
    PutBytes(0x9B, 0xDF, 0xE0);
    // ������� sahf
    PutByte(0x9E);
#endif
		// �p���p�� C0
		// ������� jnc $+4
	PutBytes(0x73,0x02);
	fxch_st1();
		// ������� fstp st
	PutBytes(0xDD,0xD8);
	StackRegsUsed--;
}
void TCompilationState::fmax()
{
#ifdef _WIN64
    // ������� fucomi st(1)
    PutBytes(0xDB, 0xE9);
#else
    // ������� fcom st(1)
    PutBytes(0xD8, 0xD1);
    // ������� fstsw ax
    PutBytes(0x9B, 0xDF, 0xE0);
    // ������� sahf
    PutByte(0x9E);
#endif
    // �p���p�� CF
    // ������� jc $+4
    PutBytes(0x72, 0x02);
	fxch_st1();
		// ������� fstp st
	PutBytes(0xDD, 0xD8);
	StackRegsUsed--;
}
void TCompilationState::fround()
{
		// ������� frndint
	PutBytes(0xD9,0xFC);
}
void TCompilationState::fceil()
{
        // mov word ptr [e/rbp+temp_word1_offset],0x0B7F
    PutBytes(0x66, 0xC7, 0x45,temp_word1_offset);
    PutBytes(0x7F, 0x0B);
		// ������� fldcw [rbp+temp_word1_offset]
	PutBytes(0xD9,0x6D, temp_word1_offset);
		// ������� frndint
	PutBytes(0xD9,0xFC);
		// ������� fldcw [rbp+default_cw_offset]
	PutBytes(0xD9,0x6D,default_cw_offset);
}
void TCompilationState::ffloor()
{
        // mov word ptr [e/rbp+temp_word1_offset],0x077F
    PutBytes(0x66, 0xC7, 0x45, temp_word1_offset);
    PutBytes(0x7F, 0x07);
		// ������� fldcw [rbp+temp_word1_offset]
	PutBytes(0xD9, 0x6D, temp_word1_offset);
		// ������� frndint
	PutBytes(0xD9,0xFC);
		// ������� fldcw [rbp+default_cw_offset]
	PutBytes(0xD9, 0x6D, default_cw_offset);
}
void TCompilationState::fint()
{
        // mov word ptr [e/rbp+temp_word1_offset],0x0F7F
    PutBytes(0x66, 0xC7, 0x45, temp_word1_offset);
    PutBytes(0x7F, 0x0F);
		// ������� fldcw [rbp+temp_word1_offset]
	PutBytes(0xD9, 0x6D, temp_word1_offset);
		// ������� frndint
	PutBytes(0xD9,0xFC);
		// ������� fldcw [rbp+default_cw_offset]
	PutBytes(0xD9, 0x6D, default_cw_offset);
}
