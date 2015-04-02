// libwav_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	Wave w(std::string("16bit_signed_pcm.wav"));
	std::cout << w.h->fileLength << std::endl;
	system("pause");
	return 0;
}

