// ConsoleCalc.cpp: test program
//
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <windows.h>
#include "mathfc.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2		1.57079632679489661923
#endif

using namespace std;


double disp(double x)
{
	cout << x << endl;
	return 0;
}
double disp2(double x,double y)
{
	cout << "("<<x<<","<<y<<")" << endl;
	return 0;
}
double disp3(double x, double y,double z)
{
	cout << "(" << x << "," << y <<","<<z<< ")" << endl;
	return 0;
}

double disp4(double x, double y, double z,double u)
{
	cout << "(" << x << "," << y << "," << z <<","<<u<< ")" << endl;
	return 0;
}


MathFC::UserFunctionTableEntry MyFunctionTable[] = {
{ "disp",   disp },
{ "disp2",  disp2 },
{ "disp3",  disp3 },
{ "disp4",  disp4 },
{ "",       nullptr } };


unsigned MostSignBit(unsigned long long n)
{
	unsigned diffbit = 0;
	for (unsigned p = 1; p <=64; p++)
	{
		if (n & 1) diffbit = p;
		n = n >> 1;
	}
	return diffbit;
}

union nd {
	unsigned long long n;
	double d;
};

const unsigned difflevel = 4;

const bool DisplayAll = false, DisplayBits = false;

string TestExpr(const char* expr,double* args,double result)
{
	ostringstream os;
	os.setf(ios::scientific, ios::floatfield);
	os.precision(16);

	bool ErrorDetected = false;
    unsigned CompilerOptions = 0;//CO_DISABLEEXTENDEDFUNCTIONS;//CO_DONTOPTIMIZE;

	const char* varnames[] = { "x","y","z",0 };
	MathFC::MathFunction f(expr, varnames, MyFunctionTable, CompilerOptions);
	if (!f)
	{
		os << string("!!!!!!!!!!!! ")<< f.GetErrorString() <<endl;
		ErrorDetected= true;
	}
	else
	{
		double ourres = f(args,3);
		if (ourres == result)
		{
			if (DisplayAll)
			{
				os << expr << " : ";
				os << "Ok ";
			}
		}
		else
		{
			os << expr << " : ";
			union nd ourh, ch; ourh.d = ourres; ch.d = result;
			unsigned diffbits = MostSignBit(max(ourh.n, ch.n) - min(ourh.n, ch.n));
			if (diffbits >= difflevel)
			{
			    ErrorDetected = true;
				os << endl << "\tours: " << ourres << " hex: " << hex << ourh.n << dec << endl;
				os << "\tc++ : " << result << " hex: " << hex << ch.n << dec << endl;
				//os<< "\tdiff: " << ourres - result;
				//os << endl << "\tours: " << ourres << endl;
				//os << "\tc++ : " << result << endl;
				os << "\tdiff: " << ourres - result << " bits: " << diffbits << endl;
			}
			else
			{
				os << "Ok (" << diffbits << ") ";
			}
		}
	}
	return (DisplayAll || DisplayBits || ErrorDetected) ? os.str() : "";
}

double sign(double x)
{
	return x > 0 ? 1 : (x == 0 ? 0 : -1);
}

double heavi(double x)
{
	return x >= 0 ? 1 : 0;
}
double cot(double x)
{
	return 1/tan(x);
}
double acot(double x)
{
	return M_PI_2-atan(x);
}


double test(double a)
{
	return sin(a);
}
double mod(double a,double b)
{
	return fmod(a,b);
}
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
double zeta(double x)
{
    return riemann_zeta(x);
}
double Ei(double x)
{
    return expint(x);
}
double I(double k, double x)
{
	return cyl_bessel_i(k, x);
}
double J(double k, double x)
{
	return cyl_bessel_j(k, x);
}
double K(double k, double x)
{
	return cyl_bessel_k(k, x);
}
double Y(double k, double x)
{
	return cyl_neumann(k, x);
}
#endif

#define TEST1(expr) cout<<"\""#expr"\"" << " "<<expr<<endl
#define TEST2(expr) cout<<"\""#expr"\""<<" : "<<TestExpr(#expr,args,expr)<<endl
#define TEST(expr) cout<<TestExpr(#expr,args,expr)
#define TESTC(expr,result) cout<<TestExpr(expr,args,result)
#define TESTE(expr) cout<<TestExpr(expr,args,0)

const double pi = M_PI;
		// unit test
void testrun()
{
	const double x = 2., y = -1., z = 4.;
	double args[] = {x,y,z};

    // тесты на локальные переменные
    TESTC("u=-y;v=2;w=x+y+v;u*100+v*10+w", 123);
    TESTC("x=-pi;y=3;cos(x*y)", -1);
    TESTC("r=sqrt(x*x + y * y + 4); cos(r*pi)+r", cos(sqrt(x*x + y * y + 4)*M_PI)+ sqrt(x*x + y * y + 4));
    TESTC("u=1;v=2;w=x+y+v;u*100+v*10+w", 123);

    TESTC("1;2;$0+$1",3);
    TESTC("-1;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;$0+$1+$2+$3+$4+$5+$6+$7+$8+$9+$10+$11+$12+$13+$14+$15", -1+ 1+ 2+ 3+ 4+ 5+ 6+ 7+ 8+ 9+ 10+ 11+ 12+ 13+14+ 15);
    TESTC("sqrt(x*x+y*y+4);cos($0*pi)", cos(sqrt(x*x + y * y + 4)*M_PI));
    TESTC("sqrt(x*x+y*y+4);2*x;cos($0*pi)+$1", cos(sqrt(x*x + y * y + 4)*M_PI)+2*x);

    // тесты на оптимизацию
    TEST((z-y) * sin(x - sqrt(2))*(- sqrt(2)+x) + pow(x - sqrt(2), exp(y+x)) + sin(x - sqrt(2))+exp(x+y)-exp(y-1));
    TEST(y * z*(x - 1.414213562) + pow(x - 1.414213562, y) + sin(x - 1.414213562));
        // тесты на сообщения об ошибках
/*
    cout << "---------errrors-----------errrors-----------" << endl;
    TESTE("x^-y");
    TESTE("+x+-y");
	TESTE("sin(y,z)");
	TESTE("erf(y,z)");
    TESTE("1;2;$57");
    TESTE("1;2;$(57)");
    TESTE("z=z+1;z");
    TESTE("pow(2,3,3)");
    TESTE("2^3^3");
    TESTE("sin(1.57)f");
    TESTE("sin (1.57)");
    cout << "---------------------------------------------" << endl;
*/        // тесты на внешние функции
	TEST(erf(x));
	TEST(erf(erf(x) + erf(y) + erf(z)));
	TEST( erf(erf(x)+erf(z)) + erf(erf(y)+erf(x)) + erf(erf(y)+erf(x)) );
    TEST(erf(1.2)+1);
	TEST(erfc(x));
	TEST(beta(2, 3));
	TEST(zeta(-1.5));
	TEST(zeta(1));
	TEST(Ei(3));
	TEST(I(1.7,3));
	TEST(J(1.7, 3));
	TEST(K(1.7, 3));
	TEST(Y(1.7, 3));
	TEST(disp(y));
	TEST(disp2(z,z+1));
	TEST(disp3(x,y,z));
		// тесты на степень
	TEST(pow(z, -0.5));
	TEST(pow(z, 0.5));
	TEST(pow(z, 1.5));
    TEST(pow(z, -1.5));
    TESTC("x^y", pow(x, y));
    TESTC(" ( x ^ y ) ^ z ", pow(pow(x, y),z));
    TEST(pow(z, 1)); TEST(pow(z, 2)); TEST(pow(z, 3)); TEST(pow(z, 4)); TEST(pow(z, 5)); TEST(pow(z, 6));
    TEST(pow(z, 7)); TEST(pow(z, 8)); TEST(pow(z, 9)); TEST(pow(z, 10)); TEST(pow(z, 11)); TEST(pow(z, 12));
    TEST(pow(z, 13)); TEST(pow(z, 14)); TEST(pow(z, 15)); TEST(pow(z, 16));
    TEST(pow(z, 3.1) + pow(x, 1.51) + pow(z, 3.1) + pow(x, y + 1.51));
	TEST(x * y * (x * (x * z * (x * x * x))));
	TEST(x * x * x * x);
	TESTC("x^9+x^8+x^6-x^4+x^2+x", 822);
	TESTC("x^(-1)+x^(-2)+x^(-4)+x^0+x^1", 3.8125);
	TESTC("x^(-1)+x^(-2)+x^(-4)+x^2.78+x^1+x^2", pow(x, -1) + pow(x, -2) + pow(x, -4) + pow(x, 2.78) + x + x * x);
	TEST(pow(x, 1.2) + pow(x, 2.3) + pow(x, -1.4));
	TESTC("x^(-4)", 0.0625);
	TESTC("x^255", pow(x, 255));
	TESTC("x^(-128)", pow(x, -128));
	TEST(pow(x, 12));

        // тесты на хитрые функции
	TEST(z*(4-cos(sin(x)+sin(sin(cos(0.125*pi)+(sin(sin(sin(3.4-y)+cos(x+2))+cos(sin(x)+sin(cos(y)+sin(sin(z)+cos(sin(z)-sin(cos(sin(x)+cos(y))+cos(sin(z)+sin(cos(sin(y)+cos(x))+sin(z))-sin(4+y)+sin(cos(x-y)+sin(sin(z-3)+cos(cos(sin(cos(x)-sin(cos(4)-sin(y-z)))-cos(z))+sin(3-y)))+cos(z-4))+y)+cos(z+2))-x))+cos(sin(x)+cos(3+z)))+sin(sin(x+z)+cos(z-y)))+sin(cos(z)+sin(x)))+cos(x))+sin(x+(sin(sin(x-y)+cos(sin(cos(z+x))+cos(sin(y)+z))+sin(sin(sin(x+y)+cos(sin(sin(z+3)+cos(y-4))+cos(sin(x)-cos(sin(3-z)+cos(y-x)))))+cos(y)))+cos(sin(cos(sin(cos(sin(cos(x-z)+sin(y+1))+sin(sin(y)-sin(sin(x)+cos(y+z))+cos(sin(x)+sin(cos(y)+sin(cos(x+y)+cos(sin(sin(cos(z-x)-sin(y+x))+cos(y+x))+cos(x-z)))+cos(sin(sin(cos(x+z)+sin(y+z))-cos(sin(sin(x-y)+cos(y+z))+cos(sin(y+z)+cos(x-z)))-sin(y+z))+sin(y-z)))+cos(sin(z)+cos(x))))+cos(sin(cos(sin(cos(sin(cos(sin(x)+cos(z))-sin(sin(y)-sin(z)))+cos(y+z))-sin(x+z))+cos(sin(y-x)+cos(x+z)))+sin(z-y))))+sin(cos(sin(x)+cos(x))+sin(sin(cos(z)+sin(y))+cos(sin(x)+cos(y))))))+sin(sin(cos(x)+sin(y))+cos(z)))+cos(sin(z)+cos(y)))))+cos(z+sin(x)))))));
	//TEST(z*(4 - cos(tan(x) + tan(tan(cos(0.125*pi) + (atan(tan(atan(3.4 - y) + cos(x + 2)) + cos(tan(x) + tan(cos(y) + tan(log(abs(z)) + cos(log(abs(z)) - tan(cos(tan(x) + cos(y)) + cos(log(abs(z)) + tan(cos(tan(y) + cos(x)) + log(abs(z))) - tan(4 + y) + tan(cos(x - y) + tan(tan(z - 3) + cos(cos(tan(cos(x) - tan(cos(4) - tan(y - z))) - cos(z)) + tan(3 - y))) + cos(z - 4)) + y) + cos(z + 2)) - x)) + cos(tan(x) + cos(3 + z))) + tan(tan(x + z) + cos(z - y))) + tan(cos(z) + atan(x))) + cos(x)) + tan(x + (tan(tan(x - y) + cos(tan(cos(z + x)) + cos(tan(y) + z)) + tan(atan(tan(x + y) + cos(tan(tan(z + 3) + cos(y - 4)) + cos(tan(x) - cos(tan(3 - z) + cos(y - x))))) + cos(y))) + cos(tan(cos(tan(cos(tan(cos(x - z) + tan(y + 1)) + tan(tan(y) - tan(tan(x) + cos(y + z)) + cos(tan(x) + tan(cos(y) + tan(cos(x + y) + cos(tan(tan(cos(z - x) - tan(y + x)) + cos(y + x)) + cos(x - z))) + cos(tan(tan(cos(x + z) + tan(y + z)) - cos(tan(tan(x - y) + cos(y + z)) + cos(tan(y + z) + cos(x - z))) - tan(y + z)) + tan(y - z))) + cos(log(abs(z)) + cos(x)))) + cos(tan(cos(tan(cos(tan(cos(tan(x) + cos(z)) - tan(tan(y) - log(abs(z)))) + cos(y + z)) - tan(x + z)) + cos(tan(y - x) + cos(x + z))) + tan(z - y)))) + tan(cos(tan(x) + cos(x)) + tan(tan(cos(z) + tan(y)) + cos(tan(x) + cos(y)))))) + tan(tan(cos(x) + tan(y)) + cos(z))) + cos(log(abs(z)) + cos(y))))) + cos(z + tan(x)))))));
    TEST((x*(z-y)-y/(-x+z)/x/x/y*x*z*y/(-x/y/z))-((x+y)*z-x*(-y-z-(-x-y-z+1))));
    TEST(0.125*x + 0.5*y + 3 * z + 8.2*x + 1.4*y + 4 * z);
	TEST(1+2+x-y+z*z-(-y));
	TEST(x*(x+y*(x+z*(y-x*(z-y*(x-2*(z-x*(y+3*(x-z*(x+2*(y-z)))))))))));
	TEST(x + x * sin(x) + cos(x) + sin(x + cos(x) + sin(y)));
	TEST(sin(x - y) - exp(x *  y - z));
	TEST(0.001 + 1000000 + 1e+14 + 1e-30);
	TEST(cos(0.1*x) + sin(0.03*z));
    TEST(cos(x*sin(x)-cos(z)+x)+log2(z+x)+cosh(x)+tan(x/y)+cot(tanh(z))-exp2(sin(x)+cos(x))+sinh(x+y));

		// тесты на разные триг. функции
	TEST(cos(-10000*y+z*z*z*z));
	TEST(cot(x*(y+z))+atan(pi/3)+acot(x+y+z)+asin(x/3)+acos(y/3));
	TEST(asin(1/(x*x+1)+acos(1/(x*x+z*z)))+acot(x));
	TEST(atan(x/y)+cos(x)+sin(0.0078125*z)+tan(x));
	TEST(tan(x/y));
	TEST(cot(pi / 4 + 2 * x + y));
		// тесты на ошибку Intel в триг.функциях
    /*
	TEST(sin(pi - 1e-3*x));
	TEST(cos(pi / 2 - 1e-3*x));
	TEST(tan(pi / 2 - 1e-3*x));
	TEST(cot(pi / 2 - 1e-3*x));
    */

		// тесты на экспоненты и логарифмы
	TEST(pow(x+2.1, 3)+exp(-3+y)+log(x));
	TEST(log2(x));
	TEST(exp2(x));
	TEST(exp(x));
	TEST(log(x));
		// тесты на гиперболические функции
	TEST(tanh(x) + cosh(z) + sinh(y));
		// тесты на минимум/максимум
    TEST(min(x, z)); TEST(min(1, -1)); TEST(max(1, -1));
	TEST(max(x, z));
    TESTC("min(-1, 0, 1)",-1);
    TESTC("max(-1, 0, 1)",1);
	TEST(min(x,y)+max(y,z));


        // тесты на округление
	TEST(round(y*pi));
	TEST(ceil(y*pi));
	TEST(floor(y*pi));
	TEST(int(y*pi));
	TEST(ceil(x*pi)+floor(y*pi)+round(z*pi));
	TEST(floor(x*pi) + round(y*pi) + ceil(z*pi));
	TEST(round(x*pi) + ceil(y*pi) + floor(z*pi));

    /* здесь и должна быть ошибка - intel округляет числа вида 2.5 к четному,
	round из с++ - от нуля */
	//TEST(round(2.5));
	//TEST(round(-2.5));
	TEST(round(3.5)); TEST(round(3.1));
	TEST(round(-3.5)); TEST(round(-3.1));
	TEST(ceil(2.5)); TEST(ceil(2.1));
	TEST(ceil(-2.5)); TEST(ceil(-2.1));
	TEST(floor(2.5)); TEST(floor(2.1));
	TEST(floor(-2.5)); TEST(floor(-2.1));
	TEST(int(2.5)); TEST(int(2.1));
	TEST(int(-2.5)); TEST(int(-2.1));
	// Тесты на остаток от деления
	TEST(mod(x, y));
	TEST(mod(y, z));
	TEST(mod(z, x));
	// тесты на знак и функцию Хэвисайда
    TEST(sign(x)); TEST(sign(1)); TEST(sign(0)); TEST(sign(-1));
	TEST(sign(y + cos(x)) + max(sign(y), sign(z)) - sign(x*y) + sign(x) + sign(y));
	TEST(sign(y));
	TEST(sign(2 * x - z));
	TEST(heavi(y));
	TEST(heavi(2*x-z));
	TEST(heavi(0)); TEST(heavi(-1)); TEST(heavi(1));

        // тесты на ошибки выполнения
	//TEST(cot(0));
	//TEST(sqrt(y));
	//TEST(1 / (2+y*x));
		// разные тесты
    TEST(2-5 + 1 - 9 + 3 + 5 + 4);
    TEST(2 * 5 * 1 *(- 9) * 3 * 5 * 4);

    TEST((x - 1)*(y - x)*(x - 1)*sin(x - 1)*cos(x*y - 1+sin(x))*sin(x)+x*y);
    TEST((x+y)*(y+z)*(y+x)*(z+z*x*y));
    TEST(x+y+z+sin(x)+cos(x)+cos(y)+x*y+z*y-z*x+pi+1e-4+5-3-1e-5);
    TEST((x+y+z+1)*(x + y + z + 1)*(x + y + z + 2));
    TEST(x*y*z*2+x*z*2*y);
    TEST(x*x+sin(x*x+y*y)+abs(sin(x*x + y * y)));
    TEST(x*x-x*x);
    TEST(x*x -y*y+z*z);
    TEST(x*x/y*y/z/z);

	TEST(cbrt(x) + cbrt(z));
	TEST(cbrt(y) + cbrt(z));
    TEST(acot(x)+asin(-x/3)+acos(-x/3));
    TEST(log(x*z+3)+exp(y)+exp(-x)+exp(z));
    TEST(exp(x)+cosh(x)+sinh(x)+tanh(x));
    TEST(exp(x)+sin(exp(x)));
    TEST(sin(x)+exp(sin(x)));
    TEST(sinh(z));
    TEST(cosh(y));
    TEST(tanh(z));
    TEST(tan(x)+cot(x));
    TEST(y*cot(x));
	TEST(x + (y + z) + sin(x + z + y));
	TEST(exp(x) + tan(tanh(x)) + cot(tanh(x)) + sqrt(sinh(x)));
	TEST(exp2(3 * log2(2)));
	TEST(cos(pi) - cos(pi));
	TEST(sin(3) - sin(3));
	TEST(y + sin(x) - sin(x) - y);
	TEST(1 - (x - 3));
	TEST(x / (3. / 4.));
	TEST(x + (3 - z));
	TEST(x / (3. * 4.));
		// Тесты на внутренние функции
    TESTC("$she(z)",(z-1/z)/2);
    TESTC("$che(z)",(z+1/z)/2);
    TESTC("$the(z)",(z*z-1)/(z*z+1));
    TESTC("$inv(z)",1/z);
    TESTC("x+$id(y+z)",5);
		//
	TESTC("u=sin(1);u-sin(1)",0);
    TEST(sin(x+y)+cos(x+y)+abs(cos(x+y))-sqrt(sin(x+y))+log(cos(x+y)));
    TEST(acot(-10));
		//
    TESTC("norm2(-3)",3);
    TESTC("norm2(3,4,5,6)",sqrt(3*3+4*4+5*5+6*6));
    TESTC("norm1(-3,4,-2,1)",10);
    TESTC("norminf(-3,-4,2,1)",4);

    TEST(asinh(x));
    TEST(asinh(x)+asinh(y)+asinh(z));
    TEST(acosh(x));
    TEST(acosh(x)+acosh(y+4)+acosh(z));
    TEST(atanh(1/z));
    TEST(atanh(1/x)+atanh(y)+atanh(x/z));

    TESTC("c= x;x=5;x+c",7);
	TESTC("x= x+1;x", 3);
	TESTC("1+ 2 ",3);
}

int maint(int argc, char* argv[])
{		// run unit tests
	testrun();
	cout << endl;
    system("pause");
    return 0;
}

int main(int argc, char* argv[])
{	// command line test
	string command;
	//double values[]= {1.,2.,3.};
	double values1[]= {1,2,3};
	do {
		cout<<setprecision(16);
		cin>>command;
		if(command=="exit") break;
		if(command=="test") command= "";
		MathFC::MathFunction f(command, { "x","y","z" });
		if(f)
		{
			cout << "value: " << f(values1) << endl;
		}
		else
		{
			cout<<f.GetErrorString()<<endl;
		}
	} while(1);
	return 0;
}

double g(double* values)
{
	double& x= values[0]; //double& y= values[1]; double& z= values[2];
	//return y*z*(x-1)+sqrt(y*y+z*z)-5*(z-y);
	//return y * z*(x - 1) + sin(z*M_PI) ;
	//return (x < z) ? (std::min)(x, y) : (std::max)(x, y);
	//return (std::min)(x, y)+ (std::max)(y, z);
    //return y * z*(x - sqrt(2)) + pow(x - sqrt(2),  y)+sin(x - sqrt(2));
    //return exp(x) + sin(cosh(x)) + cos(tanh(x))+sqrt(sinh(x));
    //return pow(x, 1.2) + pow(x, 2.3) + pow(x, -1.4)+log2(x);
    //return pow(x, 1.2) + pow(y, 2.3) + pow(x, -1.4);
    //return x * y + z;
	return erf(x);
}

int mainb(int argc, char* argv[])
{ // benchmark
	MathFC::FunctionInfo info;
	cout<<CLOCKS_PER_SEC<<endl;
	volatile double vvalues[]= {2.,3.,4.};
	//double values[] = { 1.,3.,4. };
	//TFunction f= CreateFunction("y*z*(x-1)+sqrt(pow(y,2)+pow(z,2))-5*(z-y)","x/y/z",&size);
	//Function f = CreateFunction("y * z*(x - sqrt(2)) + pow(x - sqrt(2),  y)+sin(x - sqrt(2))", "x/y/z",0,0, &size);
	//Function f = CreateFunction("min(x,y)+max(y,z)", "x/y/z",0,0, &size);
   // Function f = CreateFunction("exp(x) + tan(tanh(x)) + cot(tanh(x))+sqrt(sinh(x))", "x/y/z", 0, 0, &size);
    //Function f = CreateFunction("pow(x,1.2)+pow(x,2.3)+pow(x,-1.4)+log2(x)", "x/y/z", 0, 0, &size);
    //Function f = CreateFunction("pow(x,1.2)+pow(y,2.3)+pow(x,-1.4)", "x/y/z", 0, 0, &size);
    //Function f = CreateFunction("x*y+z", "x/y/z", 0, 0, &size);

	const char* varnames[] = { "x","y","z",0 };
	MathFC::Function f = MathFC::CreateFunction("erf(x)", varnames, 0, MyFunctionTable, 0, &info);
    if(f)
	{
		clock_t t0= clock(); double s= 0;
		for (unsigned int i = 0; i < 100000000; i++)
		{
			double values[] = { vvalues[0],vvalues[1],vvalues[2] };
			s += f(values);
		}
		clock_t t1= clock();
		cout<<"our: size: "<<info.CodeSize<<"   value: "<<s<<"   time:"<<(double)(t1-t0)/CLOCKS_PER_SEC<<endl;

		t0= clock(); s= 0;
		for (unsigned int i = 0; i < 100000000; i++)
		{
			double values[] = { vvalues[0],vvalues[1],vvalues[2] };
			s += g(values);
		}
		t1= clock();
		cout<<"ms:  size: "<< info.CodeSize <<"   value: "<<s<<"   time:"<<(double)(t1-t0)/CLOCKS_PER_SEC<<endl;
	}
	string s;
	getline(cin,s);
	return 0;
}

