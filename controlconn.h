#pragma once

/*
**  Defines
*/
#define MAX_CC_QUEUE_SIZE 512

/*
**  Response structure
*/
typedef struct {
	char* text;
	int code;
} Response_t;

/*
**  Command structure
*/
typedef struct {
	char* text;
	Response_t response;
} Command_t;

/*
**  PASV command response
*/
typedef struct {
	char addr[16];
	unsigned short int port;
} PasvResponse_t;

/*
**  Data connection type enum
*/
typedef enum {
	dt_Binary = 0,
	dt_Text
} DataConnTypeEnum;

/*
**  Command queue
*/
extern volatile Command_t* cc_commandQueue[MAX_CC_QUEUE_SIZE];
extern volatile int cc_commandQueueSize;
extern volatile int cc_currentCommand;
extern volatile int cc_221Found;

/*
**  Public functions
*/
int establishControlConnection(const char* host, int port);
void killControlConnection();
int setDataConnectionDetails();

Command_t* createCommand(const char* text);
void killCommand(Command_t* comm);
Command_t* queueCommand(Command_t* comm);

int sendCommand(Command_t command);
Response_t receiveResponse();

PasvResponse_t sendPasvCommand();
int sendTypeCommand(DataConnTypeEnum type);

void waitForCommand(Command_t* comm);
void waitAllCommands();

void* commandQueueLoop(void*);
