#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <cstdio>
using namespace std;

#define USING_WINAPI

#define TIMEOUT				100
#define DEFAULT_PORT		"COM2"
#define DEFAULT_BAUDRATE	115200

#ifdef USING_WINAPI
  #include <windows.h>
#endif

class comport
{
public:
	comport();
	comport(const char* pn);
	~comport();
	char *portname;	// COM name (c-string)
	HANDLE init(unsigned int baudrate);
	HANDLE init();
	bool read_byte(unsigned char *byte);
	bool write_byte(unsigned char *byte);
	bool read(unsigned char *buf, unsigned short len);
	bool write(unsigned char *buf, unsigned short len);
	bool purge_rx();
	bool purge_tx();
	bool close();
private:
	HANDLE hPort;	// Port's handle
	DCB PortDCB;	// Connection settings structure
};

