#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "operations.h"
#include "arguments.h"
#include "messages.h"
#include "controlconn.h"
#include "dataconn.h"

/*
**  Append to file
*/
int op_append(const char* file, const char* local)
{
	//  All ok
	return 0;
}

/*
**  Directory change function
*/
int op_cd(const char* dir)
{
	char cmdText[4100] = { 0 };
	sprintf(cmdText, "CWD %s", dir);
	
	Command_t* cmd = queueCommand(createCommand(cmdText));
	waitForCommand(cmd);
	printResponse(cmd->response);
	
	if (cmd->response.code == 200 || cmd->response.code == 250)
		return 0;
	else
		return -1;
}

/*
**  Download file
*/
int op_get(const char* file, const char* local)
{
	//  Send TYPE commands
	Command_t* cmd = queueCommand(createCommand("TYPE I"));
	waitForCommand(cmd);
	printResponse(cmd->response);
	
	if (cmd->response.code != 200)
		return -1;
	
	//  Check local file
	local = (local ? local : file);
	
	FILE* f = fopen(local, "wb");
	if (f == NULL)
	{
		perror("I/O ERROR");
		return -1;
	}
	else
		fclose(f);
		
	//  Prepare data connection
	if (setDataConnectionDetails())
		return -1;
		
	//  Prepare file
	strcpy((char*)dc_localFileName, local);
	dc_nextAction = at_Get;
	
	//  Establish data connection
	dc_actionStatus = as_OkConnect;
	
	while (dc_actionStatus == as_OkConnect)
		pthread_yield();
		
	if (dc_actionStatus != as_PendingSendRcv)
		return -1;
	
	//  Send RETR command
	char cmdText[4100] = { 0 };
	sprintf(cmdText, "RETR %s", file);
	
	cmd = queueCommand(createCommand(cmdText));
	waitForCommand(cmd);
	printResponse(cmd->response);
	
	//  Download file
	//  Some servers send 150, then data, then the 226
	//  Some send 226, then data
	if (cmd->response.code != 150 && cmd->response.code != 226)
	{
		dc_actionStatus = as_AbortSendRcv;
		return -1;
	}
	else
		dc_actionStatus = as_OkSendRcv;
	
	//  Wait for download to finish	
	while (dc_actionStatus != as_Done)
	{
		printf("\15Downloaded %d kB ...", dc_bufferSize/1024);
		pthread_yield();
	}
	
	//  Get second response if necesarry
	if (cmd->response.code == 150)
	{	
		Response_t resp = receiveResponse();
		printResponse(resp);
	}
	
	//  All ok
	return 0;
}

/*
**  List files/folders
*/
int op_ls(const char* dir)
{
	//  Send TYPE commands
	Command_t* cmd = queueCommand(createCommand("TYPE I"));
	waitForCommand(cmd);
	printResponse(cmd->response);
	
	if (cmd->response.code != 200)
		return -1;
		
	//  Prepare data connection
	if (setDataConnectionDetails())
		return -1;
		
	//  Establish data connection
	dc_nextAction = at_GetBuffer;
	dc_actionStatus = as_OkConnect;
	
	while (dc_actionStatus == as_OkConnect)
		pthread_yield();
		
	if (dc_actionStatus != as_PendingSendRcv)
		return -1;
	
	//  Send LIST command
	char cmdText[4100] = { 0 };
	if (dir != NULL)
		sprintf(cmdText, "LIST %s", dir);
	else
		sprintf(cmdText, "LIST");
		
	cmd = queueCommand(createCommand(cmdText));
	waitForCommand(cmd);
	printResponse(cmd->response);
	
	//  Download list
	if (cmd->response.code != 150 && cmd->response.code != 226)
	{
		dc_actionStatus = as_AbortSendRcv;
		return -1;
	}
	else
		dc_actionStatus = as_OkSendRcv;
	
	while (dc_actionStatus != as_Done)
		pthread_yield();
	
	//  Print results
	printf("%s \n", strndup((char*)dc_buffer, dc_bufferSize));
	
	//  Get second response if needed
	if (cmd->response.code == 150)
	{
		Response_t resp = receiveResponse();
		printResponse(resp);
	}
	
	//  All ok
	return 0;
}

/*
**  Create directory
*/
int op_mkdir(const char* dir)
{
	char cmdText[4100] = { 0 };
	sprintf(cmdText, "MKD %s", dir);
	
	Command_t* cmd = queueCommand(createCommand(cmdText));
	waitForCommand(cmd);
	printResponse(cmd->response);
	
	if (cmd->response.code == 250 || cmd->response.code == 257)
		return 0;
	else
		return -1;
}

/*
**  Move file or directory
*/
int op_mv(const char* src, const char* dest)
{
	char cmdText[4100] = { 0 };
	sprintf(cmdText, "RNFR %s", src);
	
	Command_t* cmd = queueCommand(createCommand(cmdText));
	waitForCommand(cmd);
	printResponse(cmd->response);
	
	if (cmd->response.code == 350)
	{
		sprintf(cmdText, "RNTO %s", dest);
		
		Command_t* cmd = queueCommand(createCommand(cmdText));
		waitForCommand(cmd);
		printResponse(cmd->response);
		
		if (cmd->response.code == 250)
			return 0;
		else
			return -1;
	}
	else
		return -1;
}

/*
**  Upload file
*/
int op_put(const char* file, const char* local)
{
	//  Send TYPE commands
	Command_t* cmd = queueCommand(createCommand("TYPE I"));
	waitForCommand(cmd);
	printResponse(cmd->response);
	
	if (cmd->response.code != 200)
		return -1;
	
	//  Check local file
	local = (local ? local : file);
	
	FILE* f = fopen(local, "rb");
	if (f == NULL)
	{
		perror("I/O ERROR");
		return -1;
	}
	else
		fclose(f);
		
	//  Prepare connection
	if (setDataConnectionDetails())
		return -1;
		
	//  Prepare file
	strcpy((char*)dc_localFileName, local);
	dc_nextAction = at_Put;
	
	//  Establish connection
	dc_actionStatus = as_OkConnect;
	
	while (dc_actionStatus == as_OkConnect)
		pthread_yield();
		
	if (dc_actionStatus != as_PendingSendRcv)
		return -1;
	
	//  Send STOR command
	char cmdText[4100] = { 0 };
	sprintf(cmdText, "STOR %s", file);
	
	cmd = queueCommand(createCommand(cmdText));
	waitForCommand(cmd);
	printResponse(cmd->response);
	
	//  Upload file
	if (cmd->response.code != 150 && cmd->response.code != 226)
	{
		dc_actionStatus = as_AbortSendRcv;
		return -1;
	}
	else
		dc_actionStatus = as_OkSendRcv;
	
	//  Wait for upload to finish	
	while (dc_actionStatus != as_Done)
		pthread_yield();
	
	//  Get second response if necesarry
	if (cmd->response.code == 150)
	{	
		Response_t resp = receiveResponse();
		printResponse(resp);
	}
	
	//  All ok
	return 0;
}

/*
**  Show current folder
*/
int op_pwd()
{
	Command_t* cmd = queueCommand(createCommand("PWD"));
	waitForCommand(cmd);
	printResponse(cmd->response);
	
	if (cmd->response.code == 257)
		return 0;
	else
		return -1;
}

/*
**  Remove file
*/
int op_rm(const char* file)
{
	char cmdText[4100] = { 0 };
	sprintf(cmdText, "DELE %s", file);
	
	Command_t* cmd = queueCommand(createCommand(cmdText));
	waitForCommand(cmd);
	printResponse(cmd->response);
	
	if (cmd->response.code == 250)
		return 0;
	else
		return -1;
}

/*
**  Remove directory
*/
int op_rmdir(const char* dir)
{
	char cmdText[4100] = { 0 };
	sprintf(cmdText, "RMD %s", dir);
	
	Command_t* cmd = queueCommand(createCommand(cmdText));
	waitForCommand(cmd);
	printResponse(cmd->response);
	
	if (cmd->response.code == 250)
		return 0;
	else
		return -1;
}

/*
**  Quit function
*/
int op_quit()
{
	//  Post quit command
	queueCommand(createCommand("QUIT"));
	waitAllCommands();
	
	return 0;
}
