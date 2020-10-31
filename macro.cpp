/*
        Файл macro.cpp компилятоpа математических функций
        Веpсия 4.0 для Win32 и Win64
        (c) Kavinov A.V. 1998-2002,2011,2019
*/
#include <algorithm>
#include <cstring>
#ifdef _DEBUG
#include <iostream> // for debugging
#endif
#include "expr.h"

using namespace std;

TExpression* CreateMacro(const string& FunctionName,
                            const vector<string>& args)
{
    size_t argnum= args.size();
    if(argnum==1 && (FunctionName=="cot" || FunctionName=="ctg"))
    {
        TFunctionCall* innerfunc =
            new TFunctionCall("tan",args[0]);
        return new TFunctionCall("$inv",innerfunc);
    }
    if(argnum==1 && (FunctionName=="sinh" || FunctionName=="sh"))
    {
        TFunctionCall* innerfunc =
            new TFunctionCall("exp",args[0]);
        return new TFunctionCall("$she",innerfunc);
    }
    if(argnum==1 && (FunctionName=="cosh" || FunctionName=="ch"))
    {
        TFunctionCall* innerfunc =
            new TFunctionCall("exp",args[0]);
        return new TFunctionCall("$che",innerfunc);
    }
    if(argnum==1 && (FunctionName=="tanh" || FunctionName=="th"))
    {
        TFunctionCall* innerfunc =
            new TFunctionCall("exp",args[0]);
        return new TFunctionCall("$the",innerfunc);
    }
    if(argnum==1 && (FunctionName=="arsinh"
        || FunctionName=="arsh" || FunctionName=="asinh"))
    {
        TExpression* x1= CreateExpression(args[0]);
        TSum* innersum= new TSum(),*outersum= new TSum();
        outersum->AddItem(x1,false);
        TExpression* x2= x1->CreateCopy();
        TFunctionCall* sq= new TFunctionCall("sqr",x2);
        innersum->AddItem(sq,false);
        innersum->AddItem(new TConstant(1.),false);
        TFunctionCall* sqroot= new TFunctionCall("sqrt",innersum);
        outersum->AddItem(sqroot,false);
        return new TFunctionCall("ln",outersum);
    }
    if(argnum==1 && (FunctionName=="arcosh"
        || FunctionName=="arch" || FunctionName=="acosh"))
    {
        TExpression* x1= CreateExpression(args[0]);
        TSum* innersum= new TSum(),*outersum= new TSum();
        outersum->AddItem(x1,false);
        TExpression* x2= x1->CreateCopy();
        TFunctionCall* sq= new TFunctionCall("sqr",x2);
        innersum->AddItem(sq,false);
        innersum->AddItem(new TConstant(1.),true); // отличие от asinh
        TFunctionCall* sqroot= new TFunctionCall("sqrt",innersum);
        outersum->AddItem(sqroot,false);
        return new TFunctionCall("ln",outersum);
    }
    if(argnum==1 && (FunctionName=="artanh"
        || FunctionName=="arth" || FunctionName=="atanh"))
    {
        TExpression* x1= CreateExpression(args[0]);
        TExpression* x2= x1->CreateCopy();

        TProduct* d= new TProduct();
        TSum* s1= new TSum(), *s2= new TSum();
        s1->AddItem(new TConstant(1.0),false);
        s1->AddItem(x1,false);
        s2->AddItem(new TConstant(1.0),false);
        s2->AddItem(x2,true);
        d->AddItem(s1,false);
        d->AddItem(s2,true);
        TFunctionCall* flog= new TFunctionCall("ln",d);

        TProduct* p= new TProduct();
        p->AddItem(new TConstant(0.5),false);
        p->AddItem(flog,false);
        return p;
    }

    if(FunctionName=="norm1")
    {
        TSum* sum= new TSum();
        for (const string& i : args)
        {
            if (i.length() == 0) throw EFEmptySubexpression();
            TExpression* e=
                CreateExpression(i[0] == ',' ? string(i, 1, string::npos) : i);
            sum->AddItem(new TFunctionCall("abs",e),false);
        }
        return sum;
    }
    if(FunctionName=="norm2")
    {
        TSum* sum= new TSum();
        for (const string& i : args)
        {
            if (i.length() == 0) throw EFEmptySubexpression();
            TExpression* e=
                CreateExpression(i[0] == ',' ? string(i, 1, string::npos) : i);
            sum->AddItem(new TFunctionCall("sqr",e),false);
        }
        return new TFunctionCall("sqrt",sum);
    }
    if(FunctionName=="norminf")
    {
        vector<TExpression*> max_args;
        for (const string& i : args)
        {
            if (i.length() == 0) throw EFEmptySubexpression();
            TExpression* e=
                CreateExpression(i[0] == ',' ? string(i, 1, string::npos) : i);
            TFunctionCall* a= new TFunctionCall("abs",e);
            max_args.push_back(a);
        }
        return new TFunctionCall("max",std::move(max_args));
    }
    return nullptr;
}
