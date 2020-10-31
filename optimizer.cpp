/*
        Math function compiler version 4.0
        File optimizer.cpp 
        Platrorms supported : x86 / Win32 and Win64
        Author: Kavinov A.V.         
*/
#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>
#ifdef _DEBUG
#include <iostream>
#endif
#include <cstring>
#include "expr.h"
using namespace std;

void TSum::PrepareToOptimize()
{
	if(Subexpressions.empty()) return;

	for(size_t j= 0;j<Subexpressions.size();++j)
    {
        Term& p = Subexpressions[j];
        bool IsCurrentTermInverted = p.IsInverted;
        p.expr->PrepareToOptimize();
        start:
        if(TSum* s = dynamic_cast<TSum*>(p.expr);s)
        {       // раскрываем выражения вида x+(y+z) в одну сумму
            p = { s->Subexpressions[0].expr,
                s->Subexpressions[0].IsInverted!= IsCurrentTermInverted };
            s->Subexpressions[0].expr = nullptr;
            for(size_t i= 1;i<s->Subexpressions.size();i++)
            {
                Subexpressions.push_back({ s->Subexpressions[i].expr,
                    s->Subexpressions[i].IsInverted != IsCurrentTermInverted });
                s->Subexpressions[i].expr = nullptr;
            }
            delete s;
            j--;
            continue;
        }
        if(TFunctionCall* f= dynamic_cast<TFunctionCall*>(p.expr);
            f && f->GetName()=="$id")
        {   // $id это вспомогательная функция, которую можно удалить
            TExpression* a= f->Arguments[0];
            p.expr= a;
            f->Arguments[0]= nullptr;
            delete f;
            goto start;
        }
    }

    auto comp = [this](const Term& x, const Term& y)
        {  return CompareTerm(x, y)>0;      };
    sort(Subexpressions.begin(), Subexpressions.end(), comp);
    /*
        // сократим слагаемые вида x-x
    for (auto it = Subexpressions.begin(); it < Subexpressions.end() - 1; )
    {
        if (it->expr->SureEqual((it + 1)->expr) && it->IsInverted != (it + 1)->IsInverted)
        {
            delete it->expr;
            delete (it + 1)->expr;
            it->expr = new TConstant(0.);
            Subexpressions.erase(it + 1);
        }
        else ++it;
    }
    */
}
void TProduct::PrepareToOptimize()
{
    if (Subexpressions.empty()) return;

    for (size_t j = 0; j<Subexpressions.size(); ++j)
    {   // раскрываем выражения вида x*(y/z), x/(y*z) и т.п. в одно произведение
        Term& p = Subexpressions[j];
        bool IsCurrentTermInverted = p.IsInverted;
        p.expr->PrepareToOptimize();
        start:
        if(TProduct* s = dynamic_cast<TProduct*>(p.expr);s)
        {
            p = { s->Subexpressions[0].expr,
                s->Subexpressions[0].IsInverted != IsCurrentTermInverted };
            s->Subexpressions[0].expr = nullptr;
            for(size_t i= 1;i<s->Subexpressions.size();i++)
            {
                Subexpressions.push_back({ s->Subexpressions[i].expr,
                    s->Subexpressions[i].IsInverted != IsCurrentTermInverted });
                s->Subexpressions[i].expr = nullptr;
            }
            delete s;
            j--;
            continue;
        }
        if(TFunctionCall* f= dynamic_cast<TFunctionCall*>(p.expr);
            f && f->GetName()=="$id")
        {
            TExpression* a= f->Arguments[0];
            p.expr= a;
            f->Arguments[0]= nullptr;
            delete f;
            goto start;
        }
    }
    auto comp = [](const Term& x, const Term& y)
    {  return CompareTerm(x, y)>0;      };
    sort(Subexpressions.begin(), Subexpressions.end(), comp);
        // x*x => sqr(x)*1
    for (auto it= Subexpressions.begin(); it < Subexpressions.end() - 1; )
    {
        if (it->expr->SureEqual((it+1)->expr) && it->IsInverted == (it + 1)->IsInverted)
        {
                // произведение одинаковых множителей
            #ifdef _DEBUG
            cout << it->expr->GetText() + "^2" << endl;
            #endif

            TFunctionCall* sqr = new TFunctionCall("sqr", it->expr);
            it->expr = sqr;
            delete (it + 1)->expr;
            if (Subexpressions.size() == 2)
            {
                (it + 1)->expr = new TConstant(1.);
                break;
            }
            else
            {
                Subexpressions.erase(it + 1);
                it = Subexpressions.begin();
            }
        }
        else ++it;
    }
}
static set<uint8_t> GetBits(uint8_t n)
{
    set<uint8_t> ret;
    for (uint8_t i = 0; i < 8; i++)
    {
        if (n & (1 << i)) ret.insert(i);
    }
    return ret;
}
void TFunctionCall::ProcessPower()
{
        // На входе в эту функцию подразумевается, что Name=="pow"
        // и есть 2 аргумента
    TConstant* c = dynamic_cast<TConstant*>(Arguments[1]);
    if (c)      // первый тип возможной оптимизации
    {
        // Возведение в постоянную целую степень можно оптимизировать
        double s = c->GetDouble();
        if (s == 1.)
        {
            Arguments.pop_back();
            delete c;
            FuncName = "$id";
            return;
        }
        if (s == -1.)
        {
            Arguments.pop_back();
            delete c;
            FuncName = "$inv";
            return;
        }
        if (s == 0.5)
        {
            Arguments.pop_back();
            delete c;
            FuncName = "sqrt";
            return;
        }
        if (uint8_t p = (uint8_t)(s); p > 1 && p < 65 && double(p) == s)
        {
            Arguments.pop_back();
            delete c;
            if (p == 2)
            {
                FuncName = "sqr";
                return;
            }
                // p>2
            set<uint8_t> bits = GetBits(p);

            if (bits.size() == 1)   // полный квадрат
            {
                FuncName = "sqr";
                TFunctionCall* innerf = new TFunctionCall("sqr", Arguments[0]);
                for (uint8_t i = 0; i < (*bits.begin() - 2); i++)
                {
                    innerf = new TFunctionCall("sqr", innerf);
                }
                Arguments[0] = innerf;
            }
            else
            {
                    // не полный квадрат
                FuncName = "$id";
                TExpression* a = Arguments[0];
                TProduct* p = new TProduct{};
                Arguments[0] = p;
                for (uint8_t b : bits)
                {
                    if (b == 0) p->AddItem(a->CreateCopy(), false);
                    else
                    {
                        TFunctionCall* innerf = new TFunctionCall("sqr", a->CreateCopy());
                        for (uint8_t i = 0; i < b - 1; i++)
                        {
                            innerf = new TFunctionCall("sqr", innerf);
                        }
                        p->AddItem(innerf, false);
                    }

                }
                delete a;
            }
            return;
        }
    }
    /*
        // Здесь вопрос: стоит преобразовывать x^y в 2^(y*log2(x)) или нет
        // ответ: нет
    TExpression* x = Arguments[0];
    TExpression* y = Arguments[1];
    Arguments.pop_back();
    FuncName = "exp2";
    TProduct* p = new TProduct{};
    p->AddItem(y, false);
    TFunctionCall* f = new TFunctionCall("log2");
    f->Arguments.push_back(x);
    p->AddItem(f, false);
    Arguments[0] = p;
    */
}
void TFunctionCall::PrepareToOptimize()
{
    if (GetName() == "pow" && Arguments.size()==2) ProcessPower();
        // Синус и косинус переименуем так,
        // чтобы при сортировке они оказались рядом
    //if(GetName()=="sin") FuncName= "$1sin";
    //if(GetName()=="cos") FuncName= "$1cos";
    for(auto p : Arguments) p->PrepareToOptimize();
}

// -1 меньше, +1 больше, 0 равно

int TConstant::Compare(TExpression* e)
{
    if (GetTypeIndex() < e->GetTypeIndex()) return -1;
    if (GetTypeIndex() > e->GetTypeIndex()) return 1;
    TConstant* c = static_cast<TConstant*>(e);
    if (GetDouble() < c->GetDouble()) return 1;
    if (GetDouble() > c->GetDouble()) return -1;
    return 0;
}

int TVariable::Compare(TExpression* e)
{
    if (GetTypeIndex() < e->GetTypeIndex()) return -1;
    if (GetTypeIndex() > e->GetTypeIndex()) return 1;
    TVariable* v = static_cast<TVariable*>(e);
    return -strcmp(GetName().c_str(),v->GetName().c_str());
}

int TSum::CompareTerm(const TSum::Term& a, const TSum::Term& b)
{
    int c = a.expr->Compare(b.expr);
    if (c != 0) return c;
    if (a.IsInverted && !b.IsInverted) return 1;
    if (!a.IsInverted && b.IsInverted) return -1;
    return 0;
}

int TSum::Compare(TExpression* e)
{
    if (GetTypeIndex() < e->GetTypeIndex()) return -1;
    if (GetTypeIndex() > e->GetTypeIndex()) return 1;
    TSum* s = static_cast<TSum*>(e);
    if(Subexpressions.size()<s->Subexpressions.size()) return -1;
    if(Subexpressions.size()>s->Subexpressions.size()) return 1;

    for(size_t i=0;i<Subexpressions.size();i++)
    {
        int c = CompareTerm(Subexpressions[i],s->Subexpressions[i]);
        if(c!=0) return c;
    }
    return 0;
}
int TProduct::CompareTerm(const TProduct::Term& a, const TProduct::Term& b)
{
    int c = a.expr->Compare(b.expr);
    if (c != 0) return c;
    if (a.IsInverted && !b.IsInverted) return -1;
    if (!a.IsInverted && b.IsInverted) return 1;
    return 0;
}
int TProduct::Compare(TExpression* e)
{
    if (GetTypeIndex() < e->GetTypeIndex()) return -1;
    if (GetTypeIndex() > e->GetTypeIndex()) return 1;
    TProduct* s = static_cast<TProduct*>(e);
    if(Subexpressions.size()<s->Subexpressions.size()) return -1;
    if(Subexpressions.size()>s->Subexpressions.size()) return 1;

    for(size_t i=0;i<Subexpressions.size();i++)
    {
        int c = CompareTerm(Subexpressions[i],s->Subexpressions[i]);
        if(c!=0) return c;
    }
    return 0;
}
int TFunctionCall::Compare(TExpression* e)
{
    if (GetTypeIndex() < e->GetTypeIndex()) return -1;
    if (GetTypeIndex() > e->GetTypeIndex()) return 1;
        /* И там, и там функция
           В этом случае наше отношение порядка
           устроено так: сначала по числу аргументов,
           потом по первому аргументу,
           потом по имени функции, потом по оставшимся аргументам */
    TFunctionCall* s = static_cast<TFunctionCall*>(e);
        // по числу аргументов
    if (Arguments.size()<s->Arguments.size()) return -1;
    if (Arguments.size()>s->Arguments.size()) return 1;
        // по первому аргументу
    int c = Arguments[0]->Compare(s->Arguments[0]);
    if (c != 0) return c;
        // по имени функции
    c = -strcmp(GetName().c_str(), s->GetName().c_str());
    if (c != 0) return c;
        // по остальным аргументам
    for(size_t i=1;i<Arguments.size();i++)
    {
        c = Arguments[i]->Compare(s->Arguments[i]);
        if(c!=0) return c;
    }
    return 0;
}

bool TConstant::SureEqual(TExpression* e)
{
    TConstant* c = dynamic_cast<TConstant*>(e);
    if (c && (Value == c->GetDouble())) return true;
    else return false;
}

bool TVariable::SureEqual(TExpression* e)
{
    TVariable* v = dynamic_cast<TVariable*>(e);
    if (v && (Name == v->GetName())) return true;
    else return false;
}
bool TSum::SureEqual(TExpression* e)
{
    TSum* p = dynamic_cast<TSum*>(e);
    if (p)
    {
        size_t termnum = Subexpressions.size();
        if (p->Subexpressions.size() != termnum) return false;
        for (size_t i = 0; i < termnum; i++)
        {
            if (!EqualTerm(Subexpressions[i], p->Subexpressions[i])) return false;
        }
        return true;
    }
    else return false;
}
bool TProduct::SureEqual(TExpression* e)
{
    TProduct* p = dynamic_cast<TProduct*>(e);
    if (p)
    {
        size_t termnum = Subexpressions.size();
        if (p->Subexpressions.size() != termnum) return false;
        for (size_t i = 0; i < termnum; i++)
        {
            if (!EqualTerm(Subexpressions[i], p->Subexpressions[i])) return false;
        }
        return true;
    }
    else return false;
}
bool TFunctionCall::SureEqual(TExpression* e)
{
    TFunctionCall* p = dynamic_cast<TFunctionCall*>(e);
    if (p)
    {
        if (FuncName != p->GetName()) return false;
        if (Arguments.size() != p->Arguments.size()) return false;
        for (size_t i = 0; i < Arguments.size(); i++)
        {
            if (!Arguments[i]->SureEqual(p->Arguments[i])) return false;
        }
        return true;
    }
    else return false;
}


void Optimize(TExpression* e,OptimData& od)
{
#ifdef _DEBUG
    string tt= e->GetText();
    cout << "=>"<<tt << " : "<<endl;
#endif
        // предварительно отсортируем всё, что можно
    e->PrepareToOptimize();
#ifdef _DEBUG
    if(tt!=e->GetText()) cout << "<="<<e->GetText() << endl;
#endif
    /* обходим граф и записываем указатели на все
       нетривиальные выражения в массив */
    vector<TExpression*> Es;
    Es.reserve(100);
    auto f = [&Es](TExpression* e)
    {
        if(!(e->IsEasyToCalculate())) Es.push_back(e);
        return true;
    };
    e->EnumSubexpressions(f);
    if (Es.empty()) return;
        // Отсортируем в соответствии с введённым отношением поряддка
        // тогда одинаковые выражения окажутся рядом
    sort(Es.begin(), Es.end(), [](TExpression* a, TExpression* b)
        { return a->Compare(b) < 0; });
        // Здесь многое не учтено
        // Чётные/нечётные функции, например
    TExpression* oldp = nullptr;
    bool IdFound = false;
    for (TExpression* p : Es)
    {
        if (oldp && p->SureEqual(oldp))
        {
            if (!IdFound)
            {
                od.IdenticalExpressions.emplace_back();
                od.IdenticalExpressions.back().ExprSet.insert(oldp);
                od.IdenticalExpressions.back().ExprSet.insert(p);
                IdFound = true;
            }
            else od.IdenticalExpressions.back().ExprSet.insert(p);
        }
        else IdFound = false;
        oldp = p;
    }
        /* Некоторые из найденных одинаковых выражений
            являются частью других одинаковых выражений
            и сохранять их не надо. Найдём их и удалим из списка
        */
    vector<bool> Used(od.IdenticalExpressions.size(), false);
    auto d = [&od,&Used](TExpression* ex) -> bool
    {
        int si;
        if (od.Find(ex, si))
        {
            Used[si] = true;
            return false;
        }
        else return true;
    };
    e->EnumSubexpressions(d);
    for (size_t i= 0;i<od.IdenticalExpressions.size();i++)
    {
        if (!Used[i])  od.IdenticalExpressions[i].ExprSet.clear();
    }
    od.IdenticalExpressions.erase(
        remove_if(od.IdenticalExpressions.begin(), od.IdenticalExpressions.end(),
            [](const OptimItem& s) { return s.ExprSet.empty();  }
            ), od.IdenticalExpressions.end());
#ifdef _DEBUG
    cout << endl << e->GetText() << ":" << endl;
    for (auto& v : od.IdenticalExpressions) //IdentArgs
    {
        for (TExpression* p : v.ExprSet) cout << "   " << p->GetText() << " ";
        cout << endl;
    }
    cout << endl;
#endif
}

bool TSum::IsEasyToCalculate()
{
    if (Subexpressions.size() > 3) return false;
    if (Subexpressions.size() > 2 && !Subexpressions[2].expr->IsEasyToCalculate()) return false;
    if (Subexpressions.size() > 1 && !Subexpressions[1].expr->IsEasyToCalculate()) return false;
    return Subexpressions[0].expr->IsEasyToCalculate();
}
bool TProduct::IsEasyToCalculate()
{
    if (Subexpressions.size() > 3) return false;
    if (Subexpressions.size() > 2 && !Subexpressions[2].expr->IsEasyToCalculate()) return false;
    if (Subexpressions.size() > 1 && !Subexpressions[1].expr->IsEasyToCalculate()) return false;
    return Subexpressions[0].expr->IsEasyToCalculate();
}
bool TFunctionCall::IsEasyToCalculate()
{
        // все функции "тяжёлые", кроме модуля и квадрата
    if (GetName() == "pow")
    {
        TConstant* p = dynamic_cast<TConstant*>(Arguments[1]);
        if (p && p->GetDouble() == 2.) return true;
        else return false;
    }
    if (GetName() != "abs" && GetName() != "square" && GetName() != "sqr") return false;
    return Arguments[0]->IsSimple();
}

bool OptimData::Find(TExpression* e,int& stored_index)
{
    for(size_t i= 0;i<IdenticalExpressions.size();i++)
    {
        if(auto it= IdenticalExpressions[i].ExprSet.find(e);
            it!=IdenticalExpressions[i].ExprSet.end())
        {
            stored_index= int(i);
            return true;
        }
    }
    return false;
}
