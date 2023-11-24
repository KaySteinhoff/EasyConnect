#include "../headers/EasyConnect.h"

char *EasyConnectErrorMessages[8] = 
{
	"00001: Failed to create client socket\n",
	"00002: Faulty data received\n",
	"00003: Failed to create server socket\n",
	"00004: Failed to listen to open client sockets\n",
	"00005: Failed to find open client socket\n",
	"00006: Error polling events\n",
	"00007: Error sending package\n"
	"00008: Error connecting to server\n"
	"00009: Server unavailable(IP not found)\n"
};

struct ErrorLog EasyConnectErrorLog;
void (*EasyErrorCallback)(char*);

int AppendToLog(int errorCode)
{
	int length = strlen(EasyConnectErrorMessages[errorCode]);

	if(EasyConnectErrorLog.length == 0)
		EasyConnectErrorLog.log = (char*)malloc(length);

	while(length+EasyConnectErrorLog.length > EasyConnectErrorLog.size)
	{
		EasyConnectErrorLog.size *= 2;
		if(!(EasyConnectErrorLog.log = (char*)realloc(EasyConnectErrorLog.log, EasyConnectErrorLog.size)))
			return 0;
	}
		
	memcpy(EasyConnectErrorLog.log+EasyConnectErrorLog.length, EasyConnectErrorMessages[errorCode], length);
	EasyConnectErrorLog.length += length;

	if(EasyErrorCallback != 0)
		EasyErrorCallback(EasyConnectErrorMessages[errorCode]);

	return 1;
}

char* GetError(void)
{
	if(EasyConnectErrorLog.length == 0)
		return "No errors\n";

	char* result = (char*)malloc(EasyConnectErrorLog.length+1);
	memcpy(result, EasyConnectErrorLog.log, EasyConnectErrorLog.length);
	result[EasyConnectErrorLog.length] = '\0';
	
	return result;
}

void SetErrorCallback(void (*func)(char*))
{
	EasyErrorCallback = func;
}