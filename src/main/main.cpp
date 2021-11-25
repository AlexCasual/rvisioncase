#include <app/app.hpp>

#ifdef _WIN32
#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#endif

void startup()
{
#ifdef _WIN32
	::HeapSetInformation(GetProcessHeap(), HeapCompatibilityInformation, NULL, 0);
	::SetConsoleOutputCP(CP_UTF8);
	::WSADATA wsaData;
	::WSAStartup(MAKEWORD(2, 2), &wsaData);
#ifndef _DEBUG	
	::SetErrorMode(SEM_NOGPFAULTERRORBOX);
#endif
#endif

#ifdef _MSC_VER
    _setmode(_fileno(stderr), _O_WTEXT);
#else
    std::setlocale(LC_ALL, "");
    std::locale::global(std::locale(""));
    std::cout.imbue(std::locale());
    std::wcerr.imbue(std::locale());
#endif
	
	gethostbyname("localhost");
}

int main (int argc, char** argv)
{

	startup();

	try
	{		
		//app
		rvision::app _app;

		return _app.run(argc, argv);
	}
	catch(const std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;

		return -1;
	}
	
	return 0;
}
