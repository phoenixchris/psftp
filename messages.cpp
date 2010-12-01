#include "messages.h"
#include "arguments.h"

/*
**  Standard print of command response
*/
void printResponse(Response_t response)
{
	if (!flag_gui)
		printf("[%d] %s", response.code, response.text);
}
