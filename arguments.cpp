#include "arguments.h"

/*
**  Command line flags and parameter values
*/
int 	flag_use_encryption 			= 0;
int 	flag_display_help 				= 0;
int		flag_gui						= 0;
int 	flag_verbose 					= 0;

char 	param_host[1024] 				= "\0";
int 	param_port 						= 21;
char 	param_user[1024] 				= "anonymous";
char 	param_pass[1024] 				= "\0";
char 	param_dir[1024] 				= "\0";

/*
**  Command line option structure array
*/
static struct option param_options[] = {
	//  Flags
	{ "help", 			no_argument, 		&flag_display_help, 		1 },
	{ "encryption", 	no_argument, 		&flag_use_encryption, 		1 },
	{ "gui",			no_argument,		&flag_gui,					1 },
	{ "verbose", 		no_argument, 		&flag_verbose,		 		1 },
	
	//  Options
	{ "directory", 		required_argument,	NULL, 						'd' },
	{ "server",			required_argument, 	NULL, 						's' },
	{ "port",	 		required_argument, 	NULL, 						'p' },
	{ "user", 			required_argument, 	NULL, 						'u' },
	{ "password", 		required_argument, 	NULL, 						'w' },
	
	//  End of list NULL
	{ NULL, 			NULL, 				NULL, 						0 }
};

/*
**  Parse command line parameters
*/
int parseParameters(int argc, char* argv[])
{
	//  Option code and index
	int code, opt_index = 0;
	
	//  Parse
	while (1)
	{
		//  Get code
		code = getopt_long(argc, argv, "s:d:p:u:w:", param_options, &opt_index);
		
		//  Check for end
		if (code == -1)
			break;
			
		//  Switch for argument
		switch (code)
		{
			case 'd':
				if (optarg) strcpy(param_dir, optarg);
				break;
				
			case 's':
				if (optarg) strcpy(param_host, optarg);
				break;
				
			case 'p':
				if (optarg) param_port = atoi(optarg);
				break;
				
			case 'u':
				if (optarg) strcpy(param_user, optarg);
				break;
				
			case 'w':
				if (optarg) strcpy(param_pass, optarg);
				break;
		}
	}
	
	//  If help flag is set, don't display any more junk
	if (flag_display_help)
		return 0;
	
	//  Show what's extra
	if (optind < argc)
	{
		//  Show warning
		printf ("WARNING: Following parameters were not recognised: ");
		
		//  Show problems
		while (optind < argc)
			printf ("%s ", argv[optind++]);
		putchar ('\n');
		
		//  Exit
		return -1;
	}
	
	//  Check for mandatory fields
	if (strlen(param_host) <= 0)
	{
		printf("ERROR: Please specify host! \n");
		return -1;
	}
	
	//  Check for port correctness
	if ((param_port < 1) || (param_port > 65535))
	{
		printf("ERROR: Port has to be in the 1-65535 range \n");
		return -1;
	}
	
	//  All ok
	return 0;
}
