#include "dataconn.h"
#include "controlconn.h"
#include "arguments.h"

#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <memory.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h> 
#include <sys/types.h>
#include <sys/socket.h>

/*
**  Socket
*/
static int dc_socket = 0;

/*
**  Next action details
*/
volatile ActionTypeEnum dc_nextAction = at_GetBuffer;
volatile ActionStatusEnum dc_actionStatus = as_PendingConnect;
volatile char dc_localFileName[4096] = { 0 };
volatile char dc_address[16] = { 0 };
volatile unsigned short int dc_port = 0;
volatile char* dc_buffer = 0;
volatile int dc_bufferSize = 0;

/*
**  Listen for data connections from the server
*/
int listenDataConnections(int port)
{	
	//  Create server address
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	
	//  Create a socket
	dc_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	if (dc_socket < 0)
	{
		perror("ERROR");
		return -1;
	}
	
	//  Bind socket
	if (bind(dc_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
	{
		perror("ERROR");
		return -1;
	}
	
	//  Listen
	if (listen(dc_socket, 1) < 0)
	{
		perror("ERROR");
		return -1;
	}
	
	//  All ok
	return 0;
}

/*
**  Establish a data connection to the FTP server
*/
int establishDataConnection(char* addr, unsigned short int port)
{
	//  Create server address
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	
	serverAddr.sin_family = AF_INET;
	inet_pton(AF_INET, addr, &serverAddr.sin_addr.s_addr);
	serverAddr.sin_port = htons(port);
	
	//  Create a socket
	dc_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	if (dc_socket < 0)
	{
		perror("ERROR");
		return -1;
	}
	
	//  Connect socket
	if (connect(dc_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
	{
		perror("ERROR");
		return -1;
	}
	
	//  All ok
	return 0;
}

/*
**  Close the data connection
*/
void killDataConnection()
{
	//  Close the socket
	close(dc_socket);
}

/*
**  Data connection process queue
*/
void* dataConnectionLoop(void*)
{
	while (1)
	{
		//  Wait for green light
		while (dc_actionStatus != as_OkConnect)
		{
			pthread_yield();
			
			if (cc_221Found)
				return NULL;
		}
			
		//  Open connection
		establishDataConnection((char*)dc_address, dc_port);
			
		dc_actionStatus = as_PendingSendRcv;
			
		//  Wait for second green light
		while (dc_actionStatus == as_PendingSendRcv)
		{
			pthread_yield();
			
			if (cc_221Found)
				return NULL;
		}
		
		//  If we have ok, send or receive
		if (dc_actionStatus == as_OkSendRcv)
		{
			//  Update status
			dc_actionStatus = as_Processing;
			
			//  Reset byte counter
			dc_bufferSize = 0;
			
			//  Write file
			if (dc_nextAction == at_Put)
			{
				//  Open file
				FILE* f = fopen((char*)dc_localFileName, "rb");
			
				if (f == NULL)
					perror("I/O ERROR");
				else
				{
					//  Read from file and send trough socket
					unsigned char buffer[1024];
					int size = 0;
				
					while (!feof(f))
					{
						size = fread(buffer, 1, 1024, f);
						write(dc_socket, buffer, size);
						
						dc_bufferSize += size;
					}
				
					//  Close file
					fclose(f);
				}
			}
		
			//  Read file
			if (dc_nextAction == at_Get)
			{
				//  Open file
				FILE* f = fopen((char*)dc_localFileName, "wb");
			
				if (f == NULL)
					perror("I/O ERROR");
				else
				{
					//  Read from file and send trough socket
					unsigned char buffer[1024];
					int size = 0;
				
					do
					{
						size = read(dc_socket, &buffer, 1024);
						fwrite(buffer, 1, size, f);
						
						dc_bufferSize += size;
					} while (size > 0);
				
					//  Close file
					fclose(f);
				}
			}
		
			//  Read buffer		
			if (dc_nextAction == at_GetBuffer)
			{
				//  Read data to buffer
				unsigned char buffer[1024];
				int size = 0;
				
				if (dc_buffer != NULL)
					free((void*)dc_buffer);
				dc_buffer = (char*)malloc(1);
			
				do
				{
					size = read(dc_socket, &buffer, 1024);
					
					if (size <= 0)
						break;
				
					dc_buffer = (char*)realloc((void*)dc_buffer, dc_bufferSize + size);
					if (dc_buffer == NULL)
						perror("FATAL ERROR");

					memcpy((void*)&dc_buffer[dc_bufferSize], buffer, size);
					dc_bufferSize += size;
				
				} while (size > 0);
			}
		}
		
		//  Update status
		dc_actionStatus = as_Done;
		
		//  Kill connection
		killDataConnection();
		
		//  Done status
		dc_actionStatus = as_Done;
	}
	
	//  All ok
	return NULL;
}
