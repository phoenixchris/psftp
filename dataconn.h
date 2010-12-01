#pragma once

/*
**  Action type enum
*/
typedef enum {
	at_Put = 0,
	at_Get,
	at_GetBuffer
} ActionTypeEnum;

/*
**  Action progress enum
*/
typedef enum {
	as_PendingConnect = 0,
	as_OkConnect,
	as_PendingSendRcv,
	as_OkSendRcv,
	as_AbortSendRcv,
	as_Processing,
	as_Done
} ActionStatusEnum;

/*
**  Next action details
*/
extern volatile ActionTypeEnum dc_nextAction;
extern volatile ActionStatusEnum dc_actionStatus;
extern volatile char dc_localFileName[4096];
extern volatile char dc_address[16];
extern volatile unsigned short int dc_port;
extern volatile char* dc_buffer;
extern volatile int dc_bufferSize;

/*
**  Public functions
*/
void* dataConnectionLoop(void*);
