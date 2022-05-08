#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <time.h>
#include <fstream>
using namespace std;

#include <Windows.h>

#define HEX_BYTES_ALIGNMENT		8

// Formating of stdout output
typedef enum
{
	_hex,
	_ascii
}TPRINT_TYPE;

// Console colors
typedef enum 
{
	Black = 0,
	Blue = 1,
	Green = 2,
	Cyan = 3,
	Red = 4,
	Magenta = 5,
	Brown = 6,
	LightGray = 7,
	DarkGray = 8,
	LightBlue = 9,
	LightGreen = 10,
	LightCyan = 11,
	LightRed = 12,
	LightMagenta = 13,
	Yellow = 14,
	White = 15
}TC_COLOR;

/**
* 	@brief: Create new file to log
*	@param: *fname - pointer at string to create log file with such name. 
					 if NULL no file will be created and written
*/
void xchg_init(char *fname);

/**
* 	@brief: Print data via stdout and ofstrem if opened
*	@param	*buf - pointer at buffer to print
*			len	 - size of buffer to print
*			type - type of output 
*	@return	None
*/
void xchg_print(uint8_t *buf, unsigned long len, TPRINT_TYPE type);

/**
* 	@brief: Print data via stdout and ofstrem if opened
*	@param	FileWrite - flag to write in log file 
*	@return	None
*/
void xchg_showtime(bool FileWrite);

/**
* 	@brief: Print data via stdout and ofstrem if opened
*	@param	**fp - pointer at c-FILE
*			*fname - how to name output file
*	@return	true - success
*/
bool xchg_openlogfile(char *fname, const char* param);

/**
* @brief	Get system date and time and convert it to hour:min:sec format
* @note
* @param	txt	  -  color of text
bckgr -  color of background
* @retr
*/
void SetColor(TC_COLOR txt, TC_COLOR bckgr);