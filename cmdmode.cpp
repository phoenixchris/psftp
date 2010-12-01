#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include "cmdmode.h"
#include "operations.h"
#include "controlconn.h"

/*
**  Defines
*/
#define ERR_MSG_WRONG_PARAM_COUNT 			"Wrong number of parameters!"
#define ERR_MSG_UNKNOWN_COMMAND 			"Unknown command!"

/*
**  strupr - Couldn't find the fucker so wrote it myself
*/
void strupr(char* str)
{
	while (*str) 
	{
		*str = toupper(*str);
		str++;
	}
}

/*
**  Help function
*/
void displayHelp(const char* topic)
{
	//  Generic help?
	if (topic == NULL)
	{
		printf(
			"Avaliable commands are: \n" \
			"  append \n" \
			"  cd \n" \
			"  get \n" \
			"  help \n" \
			"  ls \n" \
			"  mkdir \n" \
			"  mv \n" \
			"  put \n" \
			"  pwd \n" \
			"  rm \n" \
			"  rmdir \n" \
			"  quit \n" \
			"");
	}
	
	//  TODO: Complete topic help
}

/*
**  Parse a command string
*/
void parseCommandText(char* commandText)
{
	//  Trim left
	while (commandText[0] == ' ')
		commandText++;
	
	//  Trim right
	//  TODO: YES, I KNOW IT'S UGLY. SHUT UP ALREADY!
	while (1) 
	{
		if (strlen(commandText) == 0)
			return;
			
		if (commandText[strlen(commandText) - 1] == ' ')
			commandText[strlen(commandText) - 1] = 0;
		else
			break;
	}
	
	//  Tokenize
	int tokenCount = 0;
	char* tokens[1024] = { 0 };
	char* token = strtok(commandText, " \0");
	
	while (token != NULL)
	{
		tokens[tokenCount] = strdup(token);
		tokenCount++;
		
		token = strtok(NULL, " \0");
	}
	
	//  If no tokens, exit
	if (tokenCount == 0)
		return;
	
	//  Uppercase command (first token)
	strupr(tokens[0]);
	
	//  Check for known commands
	if (strcmp(tokens[0], "APPEND") == 0)						//  APPEND
	{
		if (tokenCount == 2 || tokenCount == 3)
			op_append(tokens[1], tokens[2]);
		else
			printf(ERR_MSG_WRONG_PARAM_COUNT "\n");
	}
	else if (strcmp(tokens[0], "CD") == 0)						//  CD
	{
		if (tokenCount == 2)
			op_cd(tokens[1]);
		else
			printf(ERR_MSG_WRONG_PARAM_COUNT "\n");
	}
	else if (strcmp(tokens[0], "GET") == 0)						//  GET
	{
		if (tokenCount == 2 || tokenCount == 3)
			op_get(tokens[1], tokens[2]);
		else
			printf(ERR_MSG_WRONG_PARAM_COUNT "\n");
	}
	else if (strcmp(tokens[0], "HELP") == 0)					//  HELP
	{
		if (tokenCount > 1)
			strupr(tokens[1]);
			
		displayHelp(tokens[1]);
	}
	else if (strcmp(tokens[0], "LS") == 0)						//  LS
	{
		if (tokenCount < 3)
			op_ls(tokens[1]);
		else
			printf(ERR_MSG_WRONG_PARAM_COUNT "\n");
	}
	else if (strcmp(tokens[0], "MKDIR") == 0)					//  MKDIR
	{
		if (tokenCount == 2)
			op_mkdir(tokens[1]);
		else
			printf(ERR_MSG_WRONG_PARAM_COUNT "\n");
	}
	else if (strcmp(tokens[0], "MV") == 0)						//  MV
	{
		if (tokenCount == 3)
			op_mv(tokens[1], tokens[2]);
		else
			printf(ERR_MSG_WRONG_PARAM_COUNT "\n");
	}
	else if (strcmp(tokens[0], "PUT") == 0)						//  PUT
	{
		if (tokenCount == 2 || tokenCount == 3)
			op_put(tokens[1], tokens[2]);
		else
			printf(ERR_MSG_WRONG_PARAM_COUNT "\n");
	}
	else if (strcmp(tokens[0], "PWD") == 0)						//  PWD
	{
		if (tokenCount == 1)
			op_pwd();
		else
			printf(ERR_MSG_WRONG_PARAM_COUNT "\n");
	}
	else if (strcmp(tokens[0], "RM") == 0)						//  RM
	{
		if (tokenCount == 2)
			op_rm(tokens[1]);
		else
			printf(ERR_MSG_WRONG_PARAM_COUNT "\n");
	}
	else if (strcmp(tokens[0], "RMDIR") == 0)					//  RMDIR
	{
		if (tokenCount == 2)
			op_rmdir(tokens[1]);
		else
			printf(ERR_MSG_WRONG_PARAM_COUNT "\n");
	}
	else if (strcmp(tokens[0], "QUIT") == 0)					//  QUIT
		op_quit();
	else
		printf(ERR_MSG_UNKNOWN_COMMAND "\n");
	
	//  Free memory
	for (int i=0; i<tokenCount; i++)
		free(tokens[i]);
}

/*
**  Command mode loop function
*/
void commandModeLoop()
{
	//  Variables
	char cmdText[1024] = "\0";
	
	//  Fire up loop
	while (!cc_221Found)
	{
		//  Read command from console
		printf("> ");
		fgets(cmdText, 1023, stdin);
		cmdText[strlen(cmdText)-1] = 0;
		
		//  Parse command
		parseCommandText(cmdText);
	}
}
