#pragma warning(disable : 4996)

#include "comport.h"
using namespace std;
/**
* 	@brief: default construct
*	@param:
*	@return:
*/
comport::comport()
{
	portname = (char*)DEFAULT_PORT;
}

/**
* 	@brief:
*	@param:
*	@return:
*/
comport::comport(const char* pn)
{
	portname = (char*)pn;
}

/**
* 	@brief: default destruct
*	@param:
*	@return:
*/
comport::~comport()
{
}


/**
* 	@brief:
*	@param:
*	@return:
*/
HANDLE comport::init(unsigned int baudrate)
{
	ostringstream oss;
	
	COMMTIMEOUTS CommTimeouts = { 0xffffffff, 0, TIMEOUT, 0, TIMEOUT };
	char str[32] = { 0 };
	WCHAR wstr[32] = { 0 };

	sprintf(str, "\\\\.\\%s", portname);
	mbstowcs((WCHAR*)wstr, str, 32);
	hPort = CreateFileW(wstr, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, /*0*/FILE_FLAG_OVERLAPPED, NULL);

	/* Cheking for valid operation result */
	if (hPort == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			oss << "Error: Serial Port '" << portname << "' doesn't exist.\n";
			throw runtime_error(oss.str());
			oss.str("");
		}
		else
		{
			oss << "Error: Port \'" << portname << "\' is busy(in use) now\n";
			throw runtime_error(oss.str());
			oss.str("");
		}
	}

	cout << "Opened port:\t" << portname << '\n';

	if (!baudrate) baudrate = DEFAULT_BAUDRATE;

	/* Initialisation of DCB structure */
	memset((void *)&PortDCB, 0, sizeof(DCB));
	PortDCB.DCBlength = sizeof(DCB);
	if (!GetCommState(hPort, &PortDCB))
	{
		CloseHandle(hPort);
		throw runtime_error("Error: Getting state error\n");
	}

	PortDCB.BaudRate = baudrate;				// Current baud 
	PortDCB.ByteSize = 8;                       // Number of bits/byte, 4-8 
	PortDCB.Parity = NOPARITY;                  // 0-4=no,odd,even,mark,space 
	PortDCB.StopBits = ONESTOPBIT;              // 0,1,2 = 1, 1.5, 2
	PortDCB.fBinary = TRUE;                     // Binary mode; no EOF check 
	PortDCB.fParity = TRUE;                     // Enable parity checking 
	PortDCB.fOutxCtsFlow = FALSE;               // No CTS output flow control 
	PortDCB.fOutxDsrFlow = FALSE;               // No DSR output flow control 
	PortDCB.fDtrControl = DTR_CONTROL_ENABLE;   // DTR flow control type 
	PortDCB.fDsrSensitivity = FALSE;            // DSR sensitivity 
	PortDCB.fTXContinueOnXoff = TRUE;           // XOFF continues Tx
	PortDCB.fOutX = FALSE;                      // No XON/XOFF out flow control 
	PortDCB.fInX = FALSE;                       // No XON/XOFF in flow control 
	PortDCB.fErrorChar = FALSE;                 // Disable error replacement 
	PortDCB.fNull = FALSE;                      // Disable null stripping 
	PortDCB.fRtsControl = RTS_CONTROL_ENABLE;   // RTS flow control
	PortDCB.fAbortOnError = FALSE;              // Do not abort reads/writes on error

	/* Set serial communication parameters */
	if (!SetCommState(hPort, &PortDCB))
	{
		oss << "Error: Couldn't set communication parameters. Err Code: " << hex << GetLastError() << '\n';
		CloseHandle(hPort);
		oss.str("");
		throw runtime_error(oss.str());
	}

	/* Output message about current port setting */
	printf("Baudrate:\t%d\nDataSize:\t%d\n", PortDCB.BaudRate, PortDCB.ByteSize);
	cout << "Stopbits:\t";
	switch (PortDCB.StopBits)
	{
	case ONESTOPBIT: cout << "1\n";
		break;

	case ONE5STOPBITS: cout << "1.5\n";
		break;

	case TWOSTOPBITS: cout << "2\n";
		break;

	default: cout << "UNKNOWN\n";
		break;
	}
	cout << "Parity:\t\t";
	switch (PortDCB.Parity)
	{
	case NOPARITY: cout << "No parity\n";
		break;

	case ODDPARITY: cout << "Odd\n";
		break;

	case EVENPARITY: cout << "Even\n";
		break;

	case MARKPARITY: cout << "Mark\n";
		break;

	case SPACEPARITY: cout << "Space\n";
		break;

	default: cout << "UNKNOWN\n";
		break;
	}

	/* Set serial communication timeouts */
	if (!SetCommTimeouts(hPort, &CommTimeouts))
	{
		oss << "Error: Couldn't set communication timeouts. Err Code: " << hex << GetLastError() << '\n';
		CloseHandle(hPort);
		oss.str("");
		throw runtime_error(oss.str());
	}

	return hPort;
}

/**
* 	@brief:
*	@param:
*	@return:
*/
HANDLE comport::init()
{
	ostringstream oss;

	COMMTIMEOUTS CommTimeouts = { 0xffffffff, 0, TIMEOUT, 0, TIMEOUT };
	char str[32] = { 0 };
	WCHAR wstr[32] = { 0 };

	sprintf(str, "\\\\.\\%s", portname);
	mbstowcs((WCHAR*)wstr, str, 32);
	hPort = CreateFileW(wstr, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, /*0*/FILE_FLAG_OVERLAPPED, NULL);

	/* Cheking for valid operation result */
	if (hPort == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			oss << "Error: Serial Port '" << portname << "' doesn't exist.\n";
			throw runtime_error(oss.str());
			oss.str("");
		}
		else
		{
			oss << "Error: Port \'" << portname << "\' is busy(in use) now\n";
			throw runtime_error(oss.str());
			oss.str("");
		}
	}

	cout << "Opened port:\t" << portname << '\n';

	/* Initialisation of DCB structure */
	memset((void *)&PortDCB, 0, sizeof(DCB));
	PortDCB.DCBlength = sizeof(DCB);
	if (!GetCommState(hPort, &PortDCB))
	{
		CloseHandle(hPort);
		throw runtime_error("Error: Getting state error\n");
	}

	PortDCB.BaudRate = DEFAULT_BAUDRATE;		// Current baud 
	PortDCB.ByteSize = 8;                       // Number of bits/byte, 4-8 
	PortDCB.Parity = NOPARITY;                  // 0-4=no,odd,even,mark,space 
	PortDCB.StopBits = ONESTOPBIT;              // 0,1,2 = 1, 1.5, 2
	PortDCB.fBinary = TRUE;                     // Binary mode; no EOF check 
	PortDCB.fParity = TRUE;                     // Enable parity checking 
	PortDCB.fOutxCtsFlow = FALSE;               // No CTS output flow control 
	PortDCB.fOutxDsrFlow = FALSE;               // No DSR output flow control 
	PortDCB.fDtrControl = DTR_CONTROL_ENABLE;   // DTR flow control type 
	PortDCB.fDsrSensitivity = FALSE;            // DSR sensitivity 
	PortDCB.fTXContinueOnXoff = TRUE;           // XOFF continues Tx
	PortDCB.fOutX = FALSE;                      // No XON/XOFF out flow control 
	PortDCB.fInX = FALSE;                       // No XON/XOFF in flow control 
	PortDCB.fErrorChar = FALSE;                 // Disable error replacement 
	PortDCB.fNull = FALSE;                      // Disable null stripping 
	PortDCB.fRtsControl = RTS_CONTROL_ENABLE;   // RTS flow control
	PortDCB.fAbortOnError = FALSE;              // Do not abort reads/writes on error

	/* Set serial communication parameters */
	if (!SetCommState(hPort, &PortDCB))
	{
		oss << "Error: Couldn't set communication parameters. Err Code: " << hex << GetLastError() << '\n';
		CloseHandle(hPort);
		oss.str("");
		throw runtime_error(oss.str());
	}

	/* Output message about current port setting */
	printf("Baudrate:\t%d\nDataSize:\t%d\n", PortDCB.BaudRate, PortDCB.ByteSize);
	cout << "Stopbits:\t";
	switch (PortDCB.StopBits)
	{
	case ONESTOPBIT: cout << "1\n";
		break;

	case ONE5STOPBITS: cout << "1.5\n";
		break;

	case TWOSTOPBITS: cout << "2\n";
		break;

	default: cout << "UNKNOWN\n";
		break;
	}
	cout << "Parity:\t\t";
	switch (PortDCB.Parity)
	{
	case NOPARITY: cout << "No parity\n";
		break;

	case ODDPARITY: cout << "Odd\n";
		break;

	case EVENPARITY: cout << "Even\n";
		break;

	case MARKPARITY: cout << "Mark\n";
		break;

	case SPACEPARITY: cout << "Space\n";
		break;

	default: cout << "UNKNOWN\n";
		break;
	}

	/* Set serial communication timeouts */
	if (!SetCommTimeouts(hPort, &CommTimeouts))
	{
		oss << "Error: Couldn't set communication timeouts. Err Code: " << hex << GetLastError() << '\n';
		CloseHandle(hPort);
		oss.str("");
		throw runtime_error(oss.str());
	}

	return hPort;
}

/**
* 	@brief:
*	@param:
*	@return: 0 - FAILURE, Nonzero - SUCCESS
*/
bool comport::read_byte(unsigned char *byte)
{
	DWORD iSize;
	return ReadFile(hPort, byte, 1, &iSize, 0);
}

/**
* 	@brief:
*	@param:
*	@return: 0 - FAILURE, Nonzero - SUCCESS
*/
bool comport::write_byte(unsigned char *byte)
{
	DWORD iSize;
	return WriteFile(hPort, byte, 1, &iSize, 0);
}

/**
* 	@brief:
*	@param:
*	@return: 0 - FAILURE, Nonzero - SUCCESS
*/
bool comport::read(unsigned char *buf, unsigned short len)
{
	DWORD iSize;
	return ReadFile(hPort, buf, len, &iSize, 0);
}

/**
* 	@brief:
*	@param:
*	@return: 0 - FAILURE, Nonzero - SUCCESS
*/
bool comport::write(unsigned char *buf, unsigned short len)
{
	DWORD iSize;
	return WriteFile(hPort, buf, len, &iSize, 0);
}

/**
* 	@brief:
*	@param:
*	@return: 0 - FAILURE, Nonzero - SUCCESS
*/
bool comport::close()
{
	bool res = CloseHandle(hPort);
	hPort = 0;
	return res;
}

/**
* 	@brief:
*	@param:
*	@return: 0 - FAILURE, Nonzero - SUCCESS
*/
bool comport::purge_rx()
{
	return PurgeComm(hPort, PURGE_RXCLEAR);
}

/**
* 	@brief:
*	@param:
*	@return: 0 - FAILURE, Nonzero - SUCCESS
*/
bool comport::purge_tx()
{
	return PurgeComm(hPort, PURGE_TXCLEAR);
}