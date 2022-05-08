#pragma once

#include <string>
#include <iostream>
#include <map>
using namespace std;

#define DEFAULT_ARG_PARAM		""		// Empty string as default argument's param
#define ARG_START_SYMB			'-'		// What to consider as an argument

typedef void(*TFunc)(string param);		// Function-to-call type

class cmdargs
{
public:
	int argc;							// Number of arguments in cmdline
	char **argv;						// Pointer at ch from cmdline 
	cmdargs();
	cmdargs(int c, char **v);
	~cmdargs();
	void check();
	void insert(string arg_name, int *func_addr);
private:
	map <string, int*> argsTable;		// Associated array with pairs(cmdline argument - function-to-call address)
};

