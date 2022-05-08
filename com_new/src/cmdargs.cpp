#include "cmdargs.h"

using namespace std;

/**
* 	@brief: Default construct
*/
cmdargs::cmdargs()
{
	argc = 1;
	argv = NULL;
}

/**
* 	@brief: Normal construct
*/
cmdargs::cmdargs(int c, char **v)
{
	argc = c;
	argv = v;
}


/**
* 	@brief: Destruct
*/
cmdargs::~cmdargs()
{
}

/**
* 	@brief: Init cmd line argumets table
*/
void cmdargs::insert(string arg_name, int *func_addr)
{
	argsTable.insert(make_pair(arg_name, func_addr));
}


/**
* 	@brief: Parse cmd line argumets
*/
void cmdargs::check()
{
	TFunc srvFunc;
	int argv_itr = 1;

	for (int i = 0; i < argc; i += argv_itr)
	{
		string arg_param = DEFAULT_ARG_PARAM;	// Default agrument's param is empty string
		string arg_name = argv[i];				// Argument's name in the argTable

		if (argv[i][0] == ARG_START_SYMB)					// Is it simple-arg or arg-with-param
		{
			argv_itr = 1;

			if (i < (argc-1))
			{ 
				if (argv[i+1][0] != ARG_START_SYMB)
				{
					// Considering any string after arg starting without '-' as param
					arg_param = argv[i+1];
					argv_itr = 2;				// to skip param while parsing cmdline
				}
			}
		}

		// Searching fot current arg in the argTable
		if (argsTable.find(arg_name) != argsTable.end())
		{
			// Call function from argsTable that is linked with current arg
			srvFunc = (TFunc)argsTable[arg_name];
			srvFunc(arg_param);			
		}
	}	
}