#pragma warning(disable : 4996)

#include "xchg.h"
using namespace std;

static void get_time(char *str);
static void get_date(char *str);
static void get_datetime(char *str); 
static bool CheckColor(char ch);

FILE *logF = NULL;							// File to store log from port
char logFname[30];							// Log-file name (c-string)
static bool logWrite = false;				// Sign to write in log file

/**
* 	@brief: Create new file to log
*/
void xchg_init(char *fname)
{
	if (fname)
	{
		xchg_openlogfile(fname, "w+");
		fclose(logF);
		logWrite = true;
	}	
	else
	{ 
		logWrite = false;
		if (logF)
		{
			fclose(logF);
			logF = NULL;
		}
	}
}

/**
* 	@brief: Printf data via stdout and ofstrem if opened
*/
void xchg_print(uint8_t *buf, unsigned long len, TPRINT_TYPE type)
{
	static bool flStrStart = true;

	switch (type)
	{ 
	case _ascii:

		if (logWrite)
		{
			logF = fopen(logFname, "a");
			if (!logF) cout << "==  Error openning log file  ==\r\n";
		}
			
		for (unsigned int i = 0; i < len; i++)
		{
			if (flStrStart)
			{
				xchg_showtime(logWrite);
				flStrStart = false;
			}
			if (!CheckColor(buf[i])) printf("%c", buf[i]);
			
			if (logWrite) fprintf(logF, "%c", buf[i]);
									
			if (buf[i] == '\n') flStrStart = true;
		}

		if (logWrite) fclose(logF);

		break;

	case _hex:

		if (logWrite)
		{
			logF = fopen(logFname, "a");
			if (!logF) cout << "==  Error openning log file  ==\r\n";
		}

		for (unsigned int i = 0; i < len; i++)
		{
			if (flStrStart)
			{
				xchg_showtime(logWrite);
				flStrStart = false;
			}
			printf("0x%02X ", buf[i]);

			if (!(i % HEX_BYTES_ALIGNMENT)) printf("\n\t\t");

			if (logWrite)
			{
				if (!(i % HEX_BYTES_ALIGNMENT)) fprintf(logF, "\n\t ");
				fprintf(logF, "0x%02X ", buf[i]);
			}

			if (buf[i] == '\n' || buf[i] == '\r') 
			{ 
				printf("\n");
				flStrStart = true;
				if (logWrite) fprintf(logF, "\r\n");
			}
		}

		if (logWrite) fclose(logF);

		break;

	default:
		break;
	}	
}

/**
* 	@brief: Printf data via stdout and ofstrem if opened
*/
void xchg_showtime(bool FileWrite)
{
	char cur_time[20];

	get_time(cur_time);

	SetColor(Yellow, Black);
	cout << ">> " << cur_time << '\t';
	SetColor(White, Black);

	if (FileWrite) fprintf(logF, cur_time);
}

/**
* 	@brief: Open log file.
*/
bool xchg_openlogfile(char *fname, const char* param)
{
	bool res = false;
	char s_date[20];

	sprintf(logFname, "%s", fname);
	get_date(s_date);
	strcat(logFname, "_");
	strcat(logFname, s_date);
	strcat(logFname, ".log");
	// trunc - for rewriting current file data \ app - for writing to the end of current file

	logF = fopen(logFname, param);

	if (!logF)
	{
		if (param == "w" || param == "w+")
		{		
			SetColor(Yellow, Black);
			cout << "!# NEWCOM\t";
			SetColor(Red, Black);
			cout << "Unable to open log - file \'" << logFname << "\'\r\n";
			SetColor(White, Black);
		}
		res = true;
	}
	else if (param == "w" || param == "w+")
	{
		SetColor(Yellow, Black);
		cout << "!# NEWCOM\tLog-file \'" << logFname << "\' opened\r\n\n" ;
		SetColor(White, Black);	
	}

	return res;
}

/**
* @brief	Get system date and time and convert it to hour:min:sec format
* @note
* @param	txt	  -  color of text
bckgr -  color of background
* @retr
*/
void SetColor(TC_COLOR txt, TC_COLOR bckgr)
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hStdOut, (WORD)((bckgr << 4) | txt));
}

/**
* @brief	Get system time and convert it to hour:min:sec format
* @note
* @param	*str - pointer at c-string to store time
* @retr		None
*/
static void get_time(char *str)
{
	for (int i = 0; i<20; i++) str[i] = 0;

	time_t timer = time(NULL);

	struct tm *lctime = localtime(&timer);

	strftime(str, 20, "%H:%M:%S ", lctime);
}


/**
* @brief	Get system date and convert it to hour:min:sec format
* @note
* @param	*str - pointer at c-string to store date
* @retr		None
*/
static void get_date(char *str)
{
	for (int i = 0; i<20; i++) str[i] = 0;

	time_t timer = time(NULL);

	struct tm *lctime = localtime(&timer);

	strftime(str, 20, "%d_%m_%Y", lctime);
}

/**
* @brief	Get system date and time and convert it to hour:min:sec format
* @note
* @param	*str - pointer at c-string to store date+time
* @retr		None
*/
static void get_datetime(char *str)
{
	for (int i = 0; i<30; i++) str[i] = 0;

	time_t timer = time(NULL);

	struct tm *lctime = localtime(&timer);

	strftime(str, 30, "%d.%m.%Y_%H:%M:%S", lctime);
}

/**
* 	@brief: Parse incoming escape-code
*	@note:	Format of esc-code is '\[31m'
*	@param: ch - character from input stream
*	@retr:	true - if assemblying is in progress - no need to print esc-symbols
*/
static bool CheckColor(char ch)
{
	static char tmp[5] = { 0 };

	switch (ch)
	{
	case 0x1B: 
		tmp[0] = ch;
		return true;
		break;
	
	case '[': 
		if (tmp[0] == 0x1B) 
		{ 
			tmp[1] = ch;
			return true;
		}	
		break;

	case '3': 
		if (tmp[2] == '3')
		{
			tmp[3] = ch;
			return true;
		}
		else if (tmp[1] == '[')
		{ 
			tmp[2] = ch;
			return true;
		}
		break;

	case '1': 
		if (tmp[2] == '3')
		{
			tmp[3] = ch;
			return true;
		}
			  break;

	case '2': 
		if (tmp[2] == '3')
		{
			tmp[3] = ch;
			return true;
		}
		break;

	case '4': 
		if (tmp[2] == '3')
		{
			tmp[3] = ch;
			return true;
		}
		break;

	case '5': 
		if (tmp[2] == '3')
		{
			tmp[3] = ch;
			return true;
		}
		break;

	case '6': 
		if (tmp[2] == '3')
		{
			tmp[3] = ch;
			return true;
		}
		break;

	case '9': 
		if (tmp[1] == '[')
		{
			tmp[2] = ch;
			return true;
		}
		break;

	case '0': 
		if (tmp[2] == '9')
		{
			tmp[3] = ch;
			return true;
		}
		else if (tmp[1] == '[')
		{
			tmp[2] = ch;
			return true;
		}
		break;

	case 'm':
		switch (tmp[3])
		{
		case '1': SetColor(Red, Black);
				  memset(tmp, 0, sizeof(tmp));
			return true;

		case '2': SetColor(Green, Black);
			memset(tmp, 0, sizeof(tmp));
			return true;

		case '3': SetColor(Yellow, Black);
			memset(tmp, 0, sizeof(tmp));
			return true;

		case '4': SetColor(Blue, Black);
			memset(tmp, 0, sizeof(tmp));
			return true;

		case '5': SetColor(Magenta, Black);
			memset(tmp, 0, sizeof(tmp));
			return true;

		case '6': SetColor(Cyan, Black);
			memset(tmp, 0, sizeof(tmp));
			return true;

		case '0': SetColor(DarkGray, Black);
			memset(tmp, 0, sizeof(tmp));
			return true;

		default: 
			if (tmp[2] == '0')
			{
				SetColor(White, Black);	// reset color
				memset(tmp, 0, sizeof(tmp));
				return true;
			}
			break;
		}

		break;
		
	default: memset(tmp, 0, sizeof(tmp));
	}
	
	return false;
}