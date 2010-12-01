#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <memory.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "controlconn.h"
#include "dataconn.h"
#include "arguments.h"
#include "messages.h"

/*
**  Socket
*/
static int cc_socket = 0;

/*
**  Command queue
*/
volatile Command_t* cc_commandQueue[MAX_CC_QUEUE_SIZE] = { NULL };
volatile int cc_commandQueueSize = 0;
volatile int cc_currentCommand = 0;
volatile int cc_221Found = 0;

/*
**  Establish a control connection to the FTP server
*/
int establishControlConnection(const char* host, int port)
{
	//  Get server description
	if (flag_verbose)
		printf("Resolving host \"%s\" ... ", host);
	
	struct hostent* serverDesc = gethostbyname(host);
	
	if (serverDesc == NULL)
	{
		printf("\nERROR: Host \"%s\" not found! \n", host);
		return -1;
	}
	
	if (flag_verbose)
		printf("found %d.%d.%d.%d \n", 
			(unsigned char)serverDesc->h_addr[0], 
			(unsigned char)serverDesc->h_addr[1], 
			(unsigned char)serverDesc->h_addr[2], 
			(unsigned char)serverDesc->h_addr[3]);
	
	//  Create server address
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	
	serverAddr.sin_family = AF_INET;
	memcpy(&serverAddr.sin_addr.s_addr, serverDesc->h_addr, serverDesc->h_length);
	serverAddr.sin_port = htons(port);
	
	//  Create a socket
	cc_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	if (cc_socket < 0)
	{
		perror("ERROR");
		return -1;
	}
	
	//  Bind socket
	if (connect(cc_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
	{
		perror("ERROR");
		return -1;
	}
	
	//  All ok
	return 0;
}

/*
**  Close the control connection
*/
void killControlConnection()
{
	//  Close the socket
	close(cc_socket);
}

/*
**  Negotiate a data connection
*/
int setDataConnectionDetails()
{
	//  Send PASV command
	PasvResponse_t addr = sendPasvCommand();
	
	//  Prepare for outgoing connection
	strcpy((char*)dc_address, addr.addr);
	dc_port = addr.port;
	
	//  All ok
	return 0;
}

/*
**  Create a command structure
*/
Command_t* createCommand(const char* text)
{
	//  Create command
	Command_t* comm = (Command_t*)malloc(sizeof(Command_t));
	comm->text = strdup(text);
	comm->response.code = -1;
	comm->response.text = NULL;
	
	//  Return
	return comm;
}

/*
**  Free memory allocated to a command
*/
void killCommand(Command_t* comm)
{
	if (comm == NULL)
		return;
		
	if (comm->text != NULL)
		free(comm->text);
		
	if (comm->response.text != NULL)
		free(comm->response.text);
		
	free(comm);
}

/*
**  Add command to queue
*/
Command_t* queueCommand(Command_t* comm)
{
	//  Increase queue size
	cc_commandQueueSize++;
	
	//  Check for overflow
	while (cc_commandQueueSize > MAX_CC_QUEUE_SIZE)
	{
		//  Check if firs command was executed
		if (cc_currentCommand <= 0)
			return NULL;
		
		//  Delete first element
		free(cc_commandQueue[0]->text);
		free(cc_commandQueue[0]->response.text);
		free((void*)cc_commandQueue[0]);
		
		for (int i=0; i<MAX_CC_QUEUE_SIZE; i++)
			cc_commandQueue[i] = cc_commandQueue[i+1];
			
		//  Resize
		cc_commandQueueSize--;
		cc_currentCommand--;
	}
	
	//  Add new command
	cc_commandQueue[cc_commandQueueSize-1] = comm;
	
	//  All ok
	return comm;
}

/*
**  Send a FTP command trough the control connection
*/
int sendCommand(Command_t command)
{
	//  Add \15\12 at end of string
	int len = strlen(command.text);
	char* comm = (char*)malloc(len+3);
	strcpy(comm, command.text);
	
	comm[len] = '\15';
	comm[len+1] = '\12';
	comm[len+2] = '\0';
	
	//  Send trough socket
	write(cc_socket, comm, strlen(comm));
	
	//  Free memory
	free(comm);
	
	//  All ok
	return 0;
}

/*
**  Wait for response from the server
*/
Response_t receiveResponse()
{
	//  Wait for response
	char respText[10240];
	int size = read(cc_socket, respText, 10240);
	respText[size] = 0;
	
	//  Split data into lines and remember code
	int code = 0;
	char processedText[10240] = { 0 };
	char* line = strtok(respText, "\12");
	
	while (line != NULL)
	{
		//	Parse code
		char codeChars[4];
		strncpy(codeChars, line, 3);
		codeChars[3] = NULL;		
		int lineCode = atoi(codeChars);
		
		//  If line's code is valid, keep it
		if ((lineCode > 0) && (line[3] == ' '))
			code = lineCode;
			
		//  Append to text
		strcat(processedText, line + (lineCode != 0 ? 3 : 0));
		
		//  Remove trailing \15\12
		if (processedText[strlen(processedText)-1] == '\12')
			processedText[strlen(processedText)-1] = '\0';
			
		if (processedText[strlen(processedText)-1] == '\15')
			processedText[strlen(processedText)-1] = '\0';
			
		//  Add newline
		strcat(processedText, "\n");
		
		//  Next line
		line = strtok(NULL, "\12");
	}
	
	//  Check for 221 (closing connection message)
	if (code == 221)
		cc_221Found = 1;
	
	//  Return response
	Response_t resp;
	resp.text = strdup(processedText);
	resp.code = code;
	
	return resp;
}

/*
**  Send PASV command and parse IP/Port
*/
PasvResponse_t sendPasvCommand()
{
	//  Create and send command
	Command_t* comm = queueCommand(createCommand("PASV"));
	waitForCommand(comm);
	
	printResponse(comm->response);
	
	//  Parse address and port
	unsigned int a1 = 0, a2 = 0, a3 = 0, a4 = 0, p1 = 0, p2 = 0;
	char* fd = comm->response.text;
	
	while ((*fd < '0' || *fd > '9') && *fd != 0)
		fd++;
		
	if (*fd != 0)
		sscanf(fd, "%d,%d,%d,%d,%d,%d", &a1, &a2, &a3, &a4, &p1, &p2);
	
	//  Return
	PasvResponse_t resp;
	sprintf(resp.addr, "%d.%d.%d.%d", a1, a2, a3, a4);
	resp.port = (p1 << 8) + p2;
	
	return resp;
}

/*
**  Send command to negotiate data connection type
*/
int sendTypeCommand(DataConnTypeEnum type)
{
	//  Create and send command
	Command_t* comm = queueCommand(createCommand(type == dt_Binary ? "TYPE I" : "TYPE A"));
	waitForCommand(comm);
	printResponse(comm->response);
	
	//  Interpret result
	if (comm->response.code != 200)
		return -1;
	else
		return 0;
}

/*
**  Enter a loop and wait for command to finish
*/
void waitForCommand(Command_t* comm)
{
	while (comm->response.text == NULL && comm->response.code < 0)
		pthread_yield();
}

/*
**  Enter a loop and wait for all commands to finish
*/
void waitAllCommands()
{
	while (cc_currentCommand < cc_commandQueueSize)
		pthread_yield();
}

/*
**  Main loop for the command queue thread
*/
void* commandQueueLoop(void* param)
{
	//  Run loop
	while (!cc_221Found)
	{
		//  Execute command if any
		while (cc_commandQueueSize > cc_currentCommand)
		{
			//  Get pointer to message
			volatile Command_t* command = cc_commandQueue[cc_currentCommand];			
			
			//  Send the message string
			sendCommand(*command);
			
			//  Wait for response
			Response_t resp = receiveResponse();
			command->response.text = resp.text;
			command->response.code = resp.code;
			
			//  Advance
			cc_currentCommand++;
		}
		
		//  Yeild to other threads
		pthread_yield();
	}
	
	//  All ok
	return NULL;
}
