/*==============================================================================
>Brief:

>Note:			

>TODO:			1.When physical port closes, close threads and inprogram port
				2.Test data-sending 

>Author:        Mikhail Berezhanov
>Date:          30.01.2019
>Version:       0.2
==============================================================================*/
#pragma warning(disable : 4996)
#pragma warning(suppress: 26489)

#include <exception>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
using namespace std;

#include "comport.h"
#include "cmdargs.h"
#include "xchg.h"
#include "enumser.h"

//---------- Defines -----------------------------------------------------------
#define BUFSIZE				16383 

#define QUIT_SYMB			":q"
#define RESTART_SYMB		'r'
#define OPEN_FILE_SYMB		":f"
#define SEND_DATA_SYMB		's'
#define STOP_LOG_SYMB		":c"

#define ARG_PORT_NAME		"-n"
#define ARG_OUTPUT_TYPE		"-t"
#define ARG_PORT_BAUD		"-br"
#define ARG_CR				"-cr"
#define ARG_LF				"-lf"

//------------ Types -----------------------------------------------------------
typedef enum
{
	simple,
	close_port
}exit_param;							// Types of exit routine

//-------- Variables -----------------------------------------------------------
comport Serial;							// Port to send data to target
HANDLE hSerial;							// Serial port handle
HANDLE hRdThr;							// ReadSerial thread handle
HANDLE hWrThr;							// WriteSerial thread handle	
HANDLE chkConnThr;
OVERLAPPED ovrlapRd;					// For overlapping functions in Read thread
OVERLAPPED ovrlapWr;					// For overlapping functions in Write thread
uint8_t bufRd[BUFSIZE];					// Rx buffer for serial read
uint8_t bufWr[BUFSIZE];					// Tx buffer for serial write
DWORD WrSize = 0;						// Size of data to write into port in bytes
TPRINT_TYPE otype = _ascii;				// Type of output messages
unsigned int baudrate = 115200;			// Serial port default baudrate
bool is_CR = true;						// Add CR ('\r') after writing to port
bool is_LF = true;						// Add LF ('\n') after writing to port
CEnumerateSerial::CPortsArray ports;
CEnumerateSerial::CPortAndNamesArray portAndNames;
CEnumerateSerial::CNamesArray names;
ULONGLONG nStartTick = 0;

//------- Prototypes -----------------------------------------------------------
static void Main_routine();
static void Console_init(short x, short y);
static void Set_Cursor(int x, int y);
static COORD Get_Cursor();
static void Args_init(cmdargs& args);
static void StartMenu(void);
static bool Check_Connection();
static void Wait();
static inline void exit_routine(exit_param prm);

//-------- Functions ----------------------------------------------------------- 
void option(string s)	// dummy
{
	cout << "argument's param: " << s << '\n';
}

/**
* 	@brief: Thread to write to Serial port.
*/
DWORD WINAPI WriteSerialThread(LPVOID par)
{
	DWORD temp, signal;
	
	ovrlapWr.hEvent = CreateEvent(NULL, true, true, NULL);

	for (;;)
	{
		// Overlapped operation
		WriteFile(hSerial, bufWr, WrSize, &temp, &ovrlapWr);

		// Stop thread waiting for end of WriteFile 
		signal = WaitForSingleObject(ovrlapWr.hEvent, INFINITE);

		if ((signal == WAIT_OBJECT_0) && (GetOverlappedResult(hSerial, &ovrlapWr, &temp, true)))
		{
		}
		else
		{
			SetColor(Yellow, Black);
			cout << "!# NEWCOM\t";
			SetColor(Red, Black);
			cout << "Sending error\n";
			SetColor(White, Black);
		}

		// Thread will sleep
		SuspendThread(hWrThr);
	}
}

/**
* 	@brief: Thread to read from Serial port when any byte comes.
*/
DWORD WINAPI ReadSerialThread(LPVOID par)
{
	COMSTAT comstat;					// Current port's status  
	DWORD btr, temp, mask, signal;
	memset(bufRd, '\0', sizeof(bufRd));
									
	ovrlapRd.hEvent = CreateEvent(NULL, true, true, NULL);

	// Enable specified event-detect(IRQ) for communication device
	SetCommMask(hSerial, EV_RXCHAR);	 
	
	cout << "Waiting data from port...\n\n";

	for(;;)	
	{
		// Stop thread (Waiting for an event to occur when any byte received via serial port)
		WaitCommEvent(hSerial, &mask, &ovrlapRd);	// Overlapped 
		signal = WaitForSingleObject(ovrlapRd.hEvent, INFINITE);	

		if (signal == WAIT_OBJECT_0)				        
		{
			// Check th result of WaitCommEvent and also update mask field
			if (GetOverlappedResult(hSerial, &ovrlapRd, &temp, true))  
			{ 
				if ((mask & EV_RXCHAR) != 0)				
				{
					// Filling up COMSTAT port's status structure
					ClearCommError(hSerial, &temp, &comstat);	
					// Get number of received bytes
					btr = comstat.cbInQue;                          	
					if (btr)                         			
					{			
						ReadFile(hSerial, bufRd, btr, &temp, &ovrlapRd);

						xchg_print(bufRd, btr, otype);

						memset(bufRd, '\0', sizeof(bufRd));	
					}
				}
			}
		}
	}
}

/**
* 	@brief: Thread to check if connection stil active
*/
DWORD WINAPI CheckConnThread(LPVOID par)
{
	while(Check_Connection())
	{
		Sleep(300);
	}

	SetColor(Yellow, Black);
	cout << "!# NEWCOM\t" << Serial.portname << " port disconnected!\n";
	SetColor(White, Black);

	return 1;
}

char input_arr[32][256];		// 32 strings , each max 256 chars
char *my_argv[32];	
int my_argc = 0;

static void input(string str, int *length)
{
	memset(my_argv, 0, sizeof(my_argv));
	memset(input_arr, 0, sizeof(input_arr));

	string delim(" ");
	size_t prev = 0;
	size_t next;
	size_t delta = delim.length();
	int cnt = 0;

	while ((next = str.find(delim, prev)) != string::npos) {

		string tmp = str.substr(prev, next - prev);

		memcpy(input_arr[cnt], tmp.c_str(), tmp.length());
		my_argv[cnt] = input_arr[cnt];

		prev = next + delta;
		cnt++;
	}

	string tmp = str.substr(prev);

	memcpy(input_arr[cnt], tmp.c_str(), tmp.length());
	my_argv[cnt] = input_arr[cnt];
	cnt++;

	*length = cnt;
}



int main(int argc, char* argv[])
{

	try
	{
		Console_init(84, 40);

		StartMenu();

		// Checking if COM port selected
		if (argc < 2)
		{ 
			cout << ("\nPlease choose serial port to open (COM#): ");
#if 0
			char cstr[6];
			memset(cstr, '\0', sizeof(cstr));
			cin >> cstr;
			Serial.portname = cstr;
#else
			string str;
			getline(cin, str);
			
			
			input(str, &my_argc);
#endif
		}
		else
		{ 
			Serial.portname = argv[1];		
		}

		cmdargs cmdArgs(my_argc, my_argv);
		Args_init(cmdArgs);
		cmdArgs.check();

		cout << "\nOutput type:\t";
		switch (otype)
		{
		case _hex:
			cout << "Hex\n";
			break;

		case _ascii:
			cout << "ASCII\n";
			break;

		default:
			cout << "Unknown\n";
			break;
		}

		hSerial = Serial.init(baudrate);
		Serial.purge_rx();

		chkConnThr = CreateThread(NULL, 0, CheckConnThread, NULL, 0, NULL);
		if (chkConnThr == NULL) throw runtime_error("Error: Couldn't create check connection thread\n");

		hRdThr = CreateThread(NULL, 0, ReadSerialThread, NULL, 0, NULL);
		if (hRdThr == NULL) throw runtime_error("Error: Couldn't create read thread\n");

		hWrThr = CreateThread(NULL, 0, WriteSerialThread, NULL, CREATE_SUSPENDED, NULL);
		if (hWrThr == NULL) throw runtime_error("Error: Couldn't create write thread\n");

		Main_routine();

		exit_routine(close_port);

		return 0;
	}

	catch (logic_error& e)
	{
		SetColor(Red, Black);
		cerr << "Logic " << e.what();
		SetColor(White, Black);
		exit_routine(simple);
		Wait();
		return 5;
	}

	catch (runtime_error& e)
	{
		SetColor(Red, Black);
		cerr << "Runtime " << e.what();
		SetColor(White, Black);
		exit_routine(close_port);
		Wait();
		return 1;
	}

	catch (...)
	{	
		SetColor(Red, Black);
		cerr << "Error: Unknown exception\n";
		SetColor(White, Black);
		exit_routine(close_port);
		Wait();
		return 2;
	}
}

/**
* 	@brief: Wait for ch from istream to see console
*/
static void Wait()
{
	char ch;
	cout << "Enter any key to continue..\n";
	cin >> ch;
}

/**
* 	@brief: Main program's process
*/
static void Main_routine()
{
	string str;
	char ch;
	COORD pos; 
	

	do
	{ 
		//pos = Get_Cursor();

		getline(cin, str);
		
		//Set_Cursor(pos.X, pos.Y + 1);

		if (str == OPEN_FILE_SYMB) xchg_init(Serial.portname);
		else if (str == STOP_LOG_SYMB)
		{
			xchg_init(NULL);
			SetColor(Yellow, Black);
			cout << "!# NEWCOM\tLog file has been closed\n\n";
			SetColor(White, Black);
		}
		else
		{
			Serial.purge_tx();
			//memset(bufWr, 0, sizeof(bufWr));
			memcpy(bufWr, str.c_str(), str.size());
			WrSize = str.size();

			if (is_CR)
			{
				bufWr[WrSize] = '\r';
				WrSize++;
			}
			if (is_LF)
			{
				bufWr[WrSize] = '\n';
				WrSize++;
			}

			ResumeThread(hWrThr);
		}

	} while (strcmp(str.c_str(), QUIT_SYMB));
}

/**
* 	@brief: Size of console window configurations.
*/
static void Console_init(short x, short y)
{
	setlocale(LC_ALL, "Russian");

	HANDLE out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD crd = { x, y };
	SMALL_RECT src = { 0, 0, crd.X, crd.Y }; // -1 for removing scroll
	SetConsoleWindowInfo(out_handle, true, &src);
	SetConsoleScreenBufferSize(out_handle, crd);
}

/**
* 	@brief: 
*/
static void Set_Cursor(int x, int y)
{
	COORD position = { x,y }; 
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(hConsole, position);
}

/**
* 	@brief:
*/
static COORD Get_Cursor()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO cbsi;
	if (GetConsoleScreenBufferInfo(hConsole, &cbsi))
	{
		return cbsi.dwCursorPosition;
	}
	else
	{
		// The function failed. Call GetLastError() for details.
		COORD invalid = { 0, 0 };
		return invalid;
	}
}

/**
* 	@brief: Print program menu.
*/
static void StartMenu(void)
{
	SetColor(Cyan, Black);
	cout << "       ___      ___ ________ ___         ___ _________ _________ __      ___\n";
	cout << "      /   \\    /  /    ____/   /        /  /   ______/  ____   /   \\    /   \\\n";
	cout << "     /     \\  /  /    /___    / ___    /  /   /        /   /  /     \\  /     \\\n";
	cout << "    /  /\\   \\/  /    ____/   / /   |  /  /   /        /   /  /       \\/       \\\n";
	cout << "   /  /  \\     /    /___    /_/ /| |_/  /   /______  /___/  /    /\\      /\\    \\\n";
	cout << "  /__/    \\___/________/_______/ |_____/__________/________/____/  \\____/  \\____\\\n";
	SetColor(White, Black);
	cout << "\n==============================  Available arguments =============================\n";
	cout << "#\t\t\t\t\t\t\t\t\t\t#\n";
	cout << "# " << ARG_PORT_NAME << "  <portname>\tto set port name \'com#\'\t\t\t\t\t#\n";
	cout << "# " << ARG_OUTPUT_TYPE << "  <type>\t\tto set \'ASCII\' or \'Hex\' output data type\t\t#\n";
	cout << "# " << ARG_PORT_BAUD << " <baudrate>\tto set port \'baudrate\'\t\t\t\t\t#\n";
	cout << "# " << ARG_CR << " <CR>\t\tto remove '\\r' from every transmition\t\t\t#\n";
	cout << "# " << ARG_LF << " <LF>\t\tto remove '\\n' from every transmition\t\t\t#\n";
	cout << "#\t\t\t\t\t\t\t\t\t\t#";
	cout << "\n==============================  Available commands  =============================\n";
	cout << "#\t\t\t\t\t\t\t\t\t\t#\n";
	cout << "# Enter\t\'" << OPEN_FILE_SYMB << "\'\t\tto start log file\t\t\t\t\t#\n";
	cout << "# Enter\t\'" << STOP_LOG_SYMB << "\'\t\tto close log file\t\t\t\t\t#\n";
	cout << "# Enter\t\'" << QUIT_SYMB << "\'\t\tto exit\t\t\t\t\t\t\t#\n";
	cout << "#\t\t\t\t\t\t\t\t\t\t#";
	cout << "\n==============================  Available Ports  ================================\n";
	cout << "#\t\t\t\t\t\t\t\t\t\t#\n";
	if (CEnumerateSerial::UsingSetupAPI2(portAndNames))
	{
		for (const auto& port : portAndNames)
		{ 
			if (port.first < 100)
				_tprintf(_T("# COM%u\t\t\t<%s>\n"), port.first, port.second.c_str());
			else 
				_tprintf(_T("# COM%u\t\t<%s>\n"), port.first, port.second.c_str());
		}
	}
	else _tprintf(_T("CEnumerateSerial::UsingSetupAPI2 failed, Error:%u\n"), GetLastError());
	cout << "#\t\t\t\t\t\t\t\t\t\t#";
	cout << "\n=================================================================================\n";
}

static void Set_ComPort(string param)
{
	static char cstr[6];
	memset(cstr, '\0', sizeof(cstr));
	memcpy(cstr, param.c_str(), 6);
	Serial.portname = cstr;
}

/**
* 	@brief: 
*/
static void Change_BaudRate(string param)
{
	if (param != "") baudrate = stoi(param);
}

/**
* 	@brief: 
*/
static void Change_OutputType(string param)
{
	if (param != "")
	{
		char* pr = _strupr((char*)param.c_str());
		
		if (!strcmp(pr, "HEX")) otype = _hex;

		else if (!strcmp(pr, "ASCII")) otype = _ascii;

		else otype = _ascii;	// as default	
	}
}

/**
* 	@brief:
*/
static void Change_CR(string param)
{
	is_CR = false;
}

/**
* 	@brief:
*/
static void Change_LF(string param)
{
	is_LF = false;
}

/**
* 	@brief: All supported cmdline arguments.
*	@note:	Add new ones with links to service-functions here.
*/
static void Args_init(cmdargs& args)
{
	args.insert(ARG_PORT_NAME, (int*)&Set_ComPort);
	args.insert(ARG_OUTPUT_TYPE, (int*)&Change_OutputType);
	args.insert(ARG_PORT_BAUD, (int*)&Change_BaudRate);
	args.insert(ARG_CR, (int*)&Change_CR);
	args.insert(ARG_LF, (int*)&Change_LF);
}

static bool Check_Connection()
{

#ifndef NO_CENUMERATESERIAL_USING_REGISTRY

	bool flC = false;

	if (CEnumerateSerial::UsingRegistry(names))
	{
		for (const auto& name : names)
		{
			if (!strcmp(name.c_str(), _strupr(Serial.portname))) flC = true;
		}
		//	_tprintf(_T("%s\n"), name.c_str());
		
	}
	else _tprintf(_T("CEnumerateSerial::UsingRegistry failed, Error:%u\n"), GetLastError());

	return flC;

#endif //#ifndef NO_CENUMERATESERIAL_USING_REGISTRY

}

/**
* 	@brief: Program-Exit routine.
*/
static inline void exit_routine(exit_param prm)
{

	if (prm == close_port)
	{
		Serial.close();
	}

	if (hRdThr)
	{
		// Close thread and destroy event handle
		TerminateThread(hRdThr, 0);
		CloseHandle(ovrlapRd.hEvent);
		CloseHandle(hRdThr);
	}

	if (hWrThr)
	{
		TerminateThread(hWrThr, 0);
		CloseHandle(ovrlapWr.hEvent);
		CloseHandle(hWrThr);
	}

	xchg_init(NULL);

}
