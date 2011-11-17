#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "arguments.h"
#include "controlconn.h"
#include "dataconn.h"
#include "cmdmode.h"

/*
**  Thread handles
*/
pthread_t controlThread, dataThread;

/*
**  Display program info
*/
void displayInfo()
{
	printf(
		"psFTP Client v0.1 \n" \
		"Written by Vasile \"Rimio\" Vilvoiu \n\n" \
		);
}

/*
**  Display command line help
*/
void displayHelp()
{
	printf(
		"Usage: psftp -s <host> [-option <value>] [--flag] \n" \
		"Options: \n"
		"  -d <dir>         Go to default directory \n" \
		"  -s <host>        Host IP address or URL \n" \
		"  -p <port>        Port to connect to (default 21) \n" \
		"  -u <username>    Login username (default anonymous) \n" \
		"  -w <password>    Login password (default NULL) \n" \
		"Flags: \n"
		"  --encryption     Use TLS encryption \n" \
		"  --gui            GUI mode with parallel data connection \n" \
		"  --help           Display this help screen \n" \
		"  --passive        Use passive mode \n" \
		"  --telnet         Telnet mode with parallel data connection \n" \
		"  --verbose        Display detailed information \n" \
		"\n");
		
}

/*
**  Initialise FTP client
*/
int initialise()
{
	//  Create a control connection
	if (flag_verbose) 
		printf("Establishing control connection ... \n");
	
	if (establishControlConnection(param_host, param_port))
		return -1;
		
	if (flag_verbose)
		printf("Control connection OK! \n");
	
	//  Create threads for connections
	pthread_create(&controlThread, NULL, commandQueueLoop, NULL);
	pthread_create(&dataThread, NULL, dataConnectionLoop, NULL);
	
	//  Display done message
	if (flag_verbose)
		printf("You're live! Have fun :) \n\n");
		
	//  Display welcome message
	Response_t response = receiveResponse();
	printf("%s \n", response.text);
	
	if (response.code != 220)
	{
		printf("ERROR: Connection refused with code %d ! \n", response.code);
		return -1;
	}
	
	//  All ok
	return 0;
}

/*
**  Set initial conditions specified in the command line
*/
void setInitialConditions()
{		
	//  String buffer and response
	char buffer[1024] = { 0 };
	Command_t* command;
	Response_t response;
		
	//  Send username
	sprintf(buffer, "USER %s", param_user);
	command = createCommand(buffer);
	
	sendCommand(*command);
	response = receiveResponse();
	
	killCommand(command);
	printf("[%d] %s", response.code, response.text);
	
	if (response.code != 230 && response.code != 331 && response.code != 332)
		printf("WARNING: Login failed for user \"%s\" ! \n", param_user);
	else
	{	
		//  Send password
		if (strcmp(param_user, "anonymous") == 0)
			sprintf(buffer, "PASS psFTPclient");
		else
			sprintf(buffer, "PASS %s", param_pass);
			
		command = createCommand(buffer);
		sendCommand(*command);
		response = receiveResponse();
		
		killCommand(command);
		printf("[%d] %s", response.code, response.text);
		
		if (response.code != 230 && response.code != 202 && response.code != 332)
			printf("WARNING: Login failed for user/pass combo! \n");	
	}
	
	//  Change directory
	if (param_dir[0] != 0)
	{
		sprintf(buffer, "CWD %s", param_dir);
		command = createCommand(buffer);
		
		sendCommand(*command);
		response = receiveResponse();
		
		killCommand(command);
		printf("[%d] %s", response.code, response.text);
		
		if (response.code != 200 && response.code != 250)
			printf("WARNING: Default directory change was unsuccessful! \n");
	}
}

/*
**  Uninitialise FTP client
*/
void kill()
{
	//  Kill connections
	killControlConnection();
}

/*
**  Application entry point
*/
int main(int argc, char **argv)
{
	//  Display info
	displayInfo();
	
	//  Parse parameters
	int parseError = parseParameters(argc, argv);
	if (parseError)
		return 0;
		
	//  Display help if required
	if (flag_display_help)
	{
		displayHelp();
		return 0;
	}
	
	//  Initialise
	if (initialise())
		return 0;
		
	//  Set initial conditions
	setInitialConditions();
		
	//  Enter appropriate loop
	//if (flag_gui)
	//	guiModeLoop()
	//else
		commandModeLoop();
	
	//  Wait for data thread to finish
	if (flag_verbose)
		printf("Waiting for data thread to finish ... \n");
		
	pthread_join(dataThread, NULL);
	
	if (flag_verbose)
		printf("Data thread dead! \n");
		
	//  Uninitialise
	kill();
		
	//  All ok, program exit
	return 0;
}

