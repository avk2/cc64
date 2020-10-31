/*
        Math function compiler version 4.0
        File expression.cpp : the compiler's front end
        Platrorms supported : x86 / Win32 and Win64
        Author: Kavinov A.V.
*/
#include <algorithm>
#include <cstring>

#ifdef _DEBUG
#include <iostream> // for debugging
#endif

#ifdef _MSC_VER
#include <charconv>
#else
#include <sstream>
#endif // _MSC_VER
#include "expr.h"

using namespace std;

//const bool DeleteLeftRecursion= false;
static const char spaces[]= " \t";
static const char specchars[]= "+-*/^()[]";
static const char linedelims[]= "\n\r;";

static bool IsSpecialChar(char c)
{
    return strchr(specchars,c)!=0;
}
static bool IsSpaceChar(char c)
{
    return strchr(spaces,c)!=0;
}


string::size_type FindFirstSpecialChar(const string& s,string::size_type pos= 0)
{
	return s.find_first_of(specchars,pos);
}

string::size_type FindFirstSignificantChar(const string& s,string::size_type pos= 0)
{
	return s.find_first_not_of(spaces,pos);
}
string::size_type FindLastSignificantChar(const string& s,string::size_type pos= string::npos)
{
	return s.find_last_not_of(spaces,pos);
}
string RemoveSpacesFromBeginAndEnd(const string& s)
{
	string::size_type start = FindFirstSignificantChar(s);
	if (start == string::npos) return "";
	string::size_type end = FindLastSignificantChar(s);
	return s.substr(start, end + 1 - start);
}
double StringToDouble(const string& str)
{
    const string s = RemoveSpacesFromBeginAndEnd(str);
    double res = 0;
#ifdef _MSC_VER
    const char* last = s.data() + s.size();
    from_chars_result fcres= from_chars(s.data(), last, res);
    if (fcres.ptr != last)
    {
        throw EFInvalidNumber();
    }
#else
    istringstream oss(s);
    oss >> res;
    if(!oss.eof()) throw EFInvalidNumber();
#endif // _MSC_VER
    if(isnan(res) || isinf(res)) throw EFInvalidNumber();
    return res;
}

vector<string> ParseSubexpression(const string& s,const char* delim)
{
	vector<string> subs;
	int br_count= 0 ;
	unsigned int cbegin= 0;
	for(unsigned int i= 0;i<s.length();i++)
	{
        if(s[i]=='(') br_count++;
        if(s[i]==')') br_count--;
        if(br_count<0) throw EFExtraRS();   // ')' больше, чем '('
        if(br_count) continue;  // Подвыражения в скобках пропускаем
            // Проверка на эксп. форму записи
        if((s[i]=='+' || s[i]=='-') && i>=2 &&
            (s[i-1]=='e' || s[i-1]=='E') && (isdigit(s[i-2]) || s[i-2]=='.'))
        {
                 // Похоже на экспоненциальную запись числа
            int j= i-2;
			bool flag = false;
			for (; j >= 0 && (isdigit(s[j]) || s[j] == '.'); j--)
			{
				if (j == 0) {
					flag = true; break;
				}
			}
            if(IsSpaceChar(s[j]) || IsSpecialChar(s[j]) || flag) // да
                continue;
			//else if isalpha(s[j])...

        }
		for(const char* p= delim;*p;p++)
        {
            if(s[i]==*p)
			{
				string substr= RemoveSpacesFromBeginAndEnd(string(s, cbegin, i - cbegin));
                subs.push_back(substr);
                cbegin= i;
                break;
			}
        }
	}
	subs.push_back(string(s,cbegin,s.length()-cbegin));
	if(br_count>0) throw EFExtraLS();
	return subs;
}
TExpression* CreateExpression(const std::string& strExpr)
{
        // Пытаемся сначала разбить выражение на слагаемые
    //int acount;
    //vector<char> delimstr;
	if(strExpr.empty()) throw EFEmptySubexpression();
	vector<string> subs;
		// Разбиваем на слагаемые
	subs= ParseSubexpression(strExpr,"+-");
        // Если не получилось больше одного слагаемого,
        // значит надо на множители раскладывать
    if(subs.size()>1)
	{
		if(subs.size()==2 && subs[0].length()==0 && subs[1].length()>=2 &&
			((subs[1])[0]=='-' || (subs[1])[0]=='+')
			&& isdigit((subs[1])[1]) && FindFirstSpecialChar(subs[1],1)==string::npos)
		{
				// отрицательная константа
			return new TConstant(StringToDouble(subs[1]));
		}
		else return new TSum(subs);
	}
	else
	{
		subs= ParseSubexpression(strExpr,"*/");
        if(subs.size()>1) return new TProduct(subs);
		else
        {
            subs= ParseSubexpression(strExpr, "^");
            if (subs.size() > 1)
            {
                    // степень
                if (subs.size() > 2)
                {       // степень с более чем 2 операндами не допускается
                    throw EFInvalidPowerExpression(strExpr);
                }
                subs[1] = RemoveSpacesFromBeginAndEnd(subs[1].substr(1));
                return new TFunctionCall("pow", subs);
            }
				// Что же это такое!
				// Не раскладывается ни на слагаемые, ни на множители и не степень!
            string::size_type fsc= FindFirstSignificantChar(strExpr);
            string::size_type lsc= FindLastSignificantChar(strExpr);
            string::size_type fspec= FindFirstSpecialChar(strExpr);


			if (fspec!=string::npos && strExpr[fspec] == '(' && strExpr[lsc] == ')')
			{	// <нечто1>(<нечто2>)
				string FunctionName = string(strExpr, fsc, fspec - fsc);
					// Проверим, не вызов ли это функции
					// Критерий проверки таков: <alnum>(...)
				string strArgs = string(strExpr, fspec + 1,
                    max(lsc - fspec - 1, string::size_type(0)));
                if(FunctionName.empty()) return CreateExpression(strArgs); // (...)
                TExpression* macro= CreateMacro(FunctionName,ParseSubexpression(strArgs, ","));
				return macro ? macro : new TFunctionCall(FunctionName, strArgs);
				//else throw EFInvalidFunctionCall(FunctionName);
			}
               // Если спецсимволов не найдено (?), то это простое выражение
            //if(fspec==string::npos)
			{
				if(isdigit(strExpr[fsc]))
                    return new TConstant(StringToDouble(strExpr));
				if(isalpha(strExpr[fsc]) || strExpr[fsc]=='$')
                    return new TVariable(string(strExpr,fsc,lsc-fsc+1));
				throw EFInvalidSymbol(strExpr);
			}/*
            else    // Сложное выражение
            {
				    // Первый спецсимвол может быть только '(',
			        // т.к. если бы это был знак ар. операции,
		            // выражение разделилось бы, а если бы ')',
                    // то выбросилось бы EFExtraRS
                    // Заметим, что здесь fsc<=fspec<strExpr.length();


            }*/
        }
	}
}

TFunctionCall::TFunctionCall(const string& FunctionName,const string& ArgList)
	: TFunctionCall(FunctionName, ParseSubexpression(ArgList, ","))
{
}
TFunctionCall::TFunctionCall(const string& FunctionName,const vector<string>& args)
    : Arguments(), FuncName(FunctionName)
{
    for (const string& i : args)
    {
        if (i.length() == 0) throw EFEmptySubexpression();
        Arguments.push_back(CreateExpression(i[0] == ',' ? string(i, 1, string::npos) : i));
    }
}
TFunctionCall::TFunctionCall(const string& FunctionName, TExpression* arg)
    : Arguments(), FuncName(FunctionName)
{
    Arguments.push_back(arg);
}

TExpression* TFunctionCall::CreateCopy()
{
    TFunctionCall* ret = new TFunctionCall{ GetName() };
    for (TExpression* e : Arguments)
    {
        ret->Arguments.push_back(e->CreateCopy());
    }
    return ret;
}
TFunctionCall::~TFunctionCall()
{
	Clear();
}
void TFunctionCall::Clear()
{
	for_each(Arguments.begin(),Arguments.end(),[](TExpression* p) { delete p; });
	Arguments.clear();
}
void TFunctionCall::EnumSubexpressions(std::function<bool(TExpression*)> f)
{
	for (TExpression* ep : Arguments)
	{
        if(f(ep)) ep->EnumSubexpressions(f);
	}
}
void TProduct::AddItem(TExpression* e,bool IsInv)
{
    Subexpressions.push_back({ e,IsInv });
}

TProduct::TProduct(std::vector<std::string> subs)
    : Subexpressions()
{
	if(subs.size()<=1) throw ECInternalError();
	for(auto i= subs.begin();i!=subs.end();++i)
	{
		string s= *i;
		if(s.empty()) throw EFEmptySubexpression();
		int first= (i==subs.begin()) ? 0 : 1;
        if(FindFirstSpecialChar(s)<string::npos)
        {
                // Это не простое слагаемое
            TExpression* newe= CreateExpression(string(s,first));
            AddItem(newe,s[0]=='/');
            continue;
        }
                // Это простое слагаемое - число или переменная
        if(isdigit(s[first]))
        {
                // Число
            //TConstant* newc= new TConstant(atof(s.c_str()+first));
            string snum= s.substr(first);
            TConstant* newc= new TConstant(StringToDouble(s));
            AddItem(newc,s[0]=='/');
            continue;
        }
        if(isalpha(s[first]) || s[first]=='$')
        {
                // Переменная
            TVariable* newv= new TVariable(string(s,first));
            AddItem(newv,s[0]=='/');
            continue;
        }
                // Непонятно что
        throw EFInvalidSymbol(s);
	}
    /*
	if(DeleteLeftRecursion && Subexpressions[0].expr->IsSimple())
	{
		for(unsigned int i= 1;i<Subexpressions.size();++i)
		{
			if(!Subexpressions[i].expr->IsSimple())
			{
				swap(Subexpressions[i],Subexpressions[0]);
				break;
			}
		}
	}
    */
}
TExpression* TProduct::CreateCopy()
{
    TProduct* ret = new TProduct{};
    for (Term& t : Subexpressions)
    {
        ret->AddItem(t.expr->CreateCopy(), t.IsInverted);
    }
    return ret;
}
TProduct::~TProduct()
{
	Clear();
}
void TProduct::Clear()
{
    for (Term& t : Subexpressions)
    {
        delete t.expr;
    }
	Subexpressions.clear();
}

void TProduct::EnumSubexpressions(std::function<bool(TExpression*)> f)
{
    for (Term& t : Subexpressions)
    {
        if(f(t.expr)) t.expr->EnumSubexpressions(f);
    }
}



void TSum::AddItem(TExpression* e,bool IsInv)
{
    Subexpressions.push_back({ e,IsInv });
}

TSum::TSum(std::vector<std::string> subs)
    : Subexpressions()
{
	if(subs.size()==1) throw ECInternalError();
	if(subs[0].length()==0) subs.erase(subs.begin());
	for(auto i= subs.begin();i!=subs.end();++i)
	{
		string s= *i;
		if(s.empty()) throw EFEmptySubexpression();
		int first= ((i==subs.begin()) && (s[0]!='-') && (s[0] != '+')) ? 0 : 1;
        if(FindFirstSpecialChar(s)<string::npos)
        {
                // Это не простое слагаемое
            TExpression* newe= CreateExpression(string(s,first));
            AddItem(newe,s[0]=='-');
            continue;
        }
                // Это простое слагаемое - число или переменная
        if(isdigit(s[first]))
        {
                // Число
            string snum= s.substr(first);
            TConstant* newc= new TConstant(StringToDouble(s));
            AddItem(newc,s[0]=='-');
            continue;
        }
        if(isalpha(s[first]) || s[first]=='$')
        {
                // Переменная
            TVariable* newv= new TVariable(string(s,first));
            AddItem(newv,s[0]=='-');
            continue;
        }
                // Непонятно что
        throw EFInvalidSymbol(s);
	}
    /*
	if(DeleteLeftRecursion && Subexpressions[0].expr->IsSimple())
	{
		for(unsigned int i= 1;i<Subexpressions.size();++i)
		{
			if(!Subexpressions[i].expr->IsSimple())
			{
				swap(Subexpressions[i],Subexpressions[0]);
				break;
			}
		}
	}
    */
}
TSum::~TSum()
{
	Clear();
}
void TSum::Clear()
{
    for (Term& t : Subexpressions)
    {
        delete t.expr;
    }
    Subexpressions.clear();
}
TExpression* TSum::CreateCopy()
{
    TSum* ret= new TSum{};
    for (Term& t : Subexpressions)
    {
        ret->AddItem(t.expr->CreateCopy(),t.IsInverted);
    }
    return ret;
}
void TSum::EnumSubexpressions(std::function<bool(TExpression*)>  f)
{
    for (Term& t : Subexpressions)
    {
        if(f(t.expr)) t.expr->EnumSubexpressions(f);
    }
}

static vector<string> SplitText(const std::string& text,const char* delim)
{
    vector<string> ret;
        // разбивает текст на части без учёта скобок и т.п.
    size_t pos= 0,dpos = 0;
    do
	{
	    dpos = text.find_first_of(delim,pos);
	    if(dpos<string::npos) ret.push_back(text.substr(pos,dpos-pos));
	    else ret.push_back(text.substr(pos));
	    pos= dpos+1;
	} while(dpos<string::npos);
	return ret;
}

bool IsCorrectVariableName(const std::string& name)
{
    if(name.empty()) return false;
    if(!isalpha(name[0]) && name[0]!='_') return false;
    for(size_t i= 1;i<name.size();i++)
        if(!isalnum(name[i]) && name[i]!='_') return false;
    return true;
}

Program::Program(const std::string& text)
{
    vector<string> textlines = SplitText(text,linedelims);
    for(const string& line : textlines)
    {
        if(FindFirstSignificantChar(line)==string::npos) continue;
        vector<string> lineparts = SplitText(line,"=");
        string varname, expression;
        switch(lineparts.size())
        {
        case 0:
            continue;
        case 1:
            expression = RemoveSpacesFromBeginAndEnd(lineparts[0]);
            break;
        case 2:
            varname = RemoveSpacesFromBeginAndEnd(lineparts[0]);
            if(!IsCorrectVariableName(varname)) throw ECBadVariableName(varname);
            expression = RemoveSpacesFromBeginAndEnd(lineparts[1]);
            break;
        default:
            throw ECInvalidAssignment(line);
        }
        Add(varname,CreateExpression(expression));
        //cout<<varname<<":"<<expression<<endl;
    }
}

