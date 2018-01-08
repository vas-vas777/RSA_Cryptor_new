#include <stdlib.h>
#include <tchar.h>
#include <stdio.h>
#include <exception>
#include "LONG_INT.h"

void DsplStatus(TCHAR *StatusStr);

int i, l, ErrCnt= 0;

LI_T a, b, c, tmp;
LI_UNIT_T Val;
LI_UNIT_T Param;
LI_UNIT_T Rslt;

void Test1()
{
	DsplStatus(_T("Test 1. addition (+)"));
				
	Val=	 0xFF;
	Param= 0x01;
	ErrCnt= 0;		
	for (i= 0; i < l; i++)
	{
		li_set_ui(a, Val);
		li_add_ui(c, a, Param);
		if (c->Units[0] != (Val+ Param)) 
			ErrCnt++;
		Val= Val << 8;
		Param= Param << 8;
	}
	if (ErrCnt == 0)
		DsplStatus(_T("     LONG_INT+ unsigned int: passed"));
	else
		DsplStatus(_T("     LONG_INT+ unsigned int: Err"));
	if (c->Units[1] == 1)	
		DsplStatus(_T("     LONG_INT+ unsigned int, carry: passed"));
	else
		DsplStatus(_T("     LONG_INT+ unsigned int, carry: Err"));
	Val=	 0xFF;
	Param= 0x01;
	ErrCnt= 0;
	for (i= 0; i < l; i++)
	{
		li_set_ui(a, Val);
		li_set_ui(b, Param);
		li_add(c, a, b);
		if (c->Units[0] != (Val+ Param)) 
			ErrCnt++;
		Val= Val << 8;
		Param= Param << 8;
	}
	if (ErrCnt == 0)
		DsplStatus(_T("     LONG_INT+ LONG_INT: passed"));
	else
		DsplStatus(_T("     LONG_INT+ LONG_INT: Err"));
	if (c->Units[1] == 1)	
		DsplStatus(_T("     LONG_INT+ LONG_INT, carry: passed"));
	else
		DsplStatus(_T("     LONG_INT+ LONG_INT int, carry: Err"));
		
	li_set(tmp, c);	
	DsplStatus(_T(""));
}

void Test2()
{
	DsplStatus(_T("Test 2. substraction (-)"));
	
	Val = 0xFFFFFFFF;
	Param = 0xFF100000;
	ErrCnt = 0;
	
	li_set(a, tmp);
	li_sub_ui(c, a, 1);
	if (c->Units[0]  == Val)
		DsplStatus(_T("     LONG_INT- unsigned int, carry: passed"));
	else
		DsplStatus(_T("     LONG_INT- unsigned int, carry: Err"));

	for (i= 0; i < l; i++)
	{
		li_set_ui(a, Val);
		li_sub_ui(c, a, Param);
		Rslt= Val- Param;
		if (c->Units[0] != Rslt) 
			ErrCnt++;
		Val= Val >> 8;
		Param= Param >> 8;
	}
	if (ErrCnt == 0)
		DsplStatus(_T("     LONG_INT- unsigned int: passed"));
	else
		DsplStatus(_T("     LONG_INT- unsigned int: Err"));
		
	Val=	 0xFFFFFFFF;
	Param= 0xFF100000;
	ErrCnt= 0;
	li_set(a, tmp);
	li_set_ui(b, 1);
	li_sub(c, a, b);
	if (c->Units[0]  == Val)
		DsplStatus(_T("     LONG_INT- LONG_INT int, carry: passed"));
	else
		DsplStatus(_T("     LONG_INT- LONG_INT int, carry: Err"));
		
	for (i= 0; i < l; i++)
	{
		li_set_ui(a, Val);
		li_set_ui(b, Param);
		li_sub(c, a, b);
		if (c->Units[0] != (Val- Param)) 
		{
			if (c->RealLen !=  0)
				ErrCnt++;
		}
		Val= Val >> 8;
		Param= Param >> 8;
	}
	if (ErrCnt == 0)
		DsplStatus(_T("     LONG_INT- LONG_INT: passed"));
	else
		DsplStatus(_T("     LONG_INT- LONG_INT: Err"));
		
	DsplStatus(_T(""));
}

void Test3()
{
	DsplStatus(_T("Test 3. multiplication (*)"));
	
	Val=	 0xFFFFFFFF;
	Param= 0x02;
	ErrCnt= 0;	

	li_set_ui(a, Val);
	li_set_ui(b, Param);
	li_mul_ui(c, a, 0);
	if (c->RealLen ==  0)
		DsplStatus(_T("     LONG_INT* 0: passed"));
	else
		DsplStatus(_T("     LONG_INT* 0: Err"));
	li_set_ui(a, Val);
	li_mul_ui(c, a, Param);
	if ((c->Units[0] == (Val << 1)) && (c->Units[1] == 1))
		DsplStatus(_T("     LONG_INT* unsigned int: passed"));
	else
		DsplStatus(_T("     LONG_INT* unsigned int: Err"));
	li_set_ui(a, Val);
	li_set_ui(b, 0);
	li_mul(c, a, b);
	if (c->RealLen ==  0)
		DsplStatus(_T("     LONG_INT* LONG_INT= 0: passed"));
	else
		DsplStatus(_T("     LONG_INT* LONG_INT= 0: Err"));
	li_set_ui(a, Val);
	li_set_ui(b, Param);
	li_mul(c, a, b);
	if ((c->Units[0] == (Val << 1)) && (c->Units[1] == 1))
		DsplStatus(_T("     LONG_INT* LONG_INT: passed"));
	else
		DsplStatus(_T("     LONG_INT* LONG_INT: Err"));

	li_set(tmp, c);
	
	DsplStatus(_T(""));
}

void Test4()
{
	DsplStatus(_T("Test4. division (/)"));
	
	li_set(a, tmp);
	li_set_ui(b, Param);
	li_cdiv_q(c, a, b);
	if ((c->Units[0] == Val) && (c->Units[1] == 0))
		DsplStatus(_T("     LONG_INT/ LONG_INT: passed"));
	else
		DsplStatus(_T("     LONG_INT. LONG_INTt: Err"));
try
{
	ErrCnt= 1;

	li_set(a, tmp);
	li_set_ui(b, 0);
	li_cdiv_q(c, a, b);
}
catch (std::exception e)
{
	DsplStatus(_T("     LONG_INT / 0: passed"));
	ErrCnt= 0;
}
	if (ErrCnt > 0)
		DsplStatus(_T("     LONG_INT / 0: Err"));
		
	DsplStatus(_T(""));
}

void Test_Init()
{
	li_init(a);
	li_init(b);
	li_init(c);

	l= BYTES_PER_LONG_INT_UNIT;
}

void Test_Clear()
{
	li_clear(a);	
	li_clear(b);	
	li_clear(c);	
	li_clear(tmp);	
}

void Test()
{
	Test_Init();
	
	Test1();
	Test2();
	Test3();
	Test4();
	
	Test_Clear();
//	DsplStatus(_T("Press Enter to evit"));
//	getchar();
}