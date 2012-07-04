// TestMemLeakConsoleApp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MemLeakDetect.h"

#pragma warning(disable:4100)

CMemLeakDetect gMemLeakDetect;

//
class CLeakMem
{
	public:
		string mystring;
		int dataInt;

};

int _tmain(int argc, _TCHAR* argv[])
{
	int* p = new int;
	CLeakMem* pm;

	pm = new CLeakMem();
	delete pm;
	pm = new CLeakMem();
	delete pm;
	*p = 1;
	delete p;

	return 0;
}

