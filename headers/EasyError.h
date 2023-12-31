#ifndef EASY_ERROR
#define EASY_ERROR

struct ErrorLog
{
	char* log;
	int length;
	int size;
};

#define ERR_CLIENT_SOCK 0
#define ERR_FAULTY_DATA 1
#define ERR_SERVER_SOCK 2
#define ERR_NO_LISTEN 3
#define ERR_NO_OPEN_SOCK 4
#define ERR_POLL 5
#define ERR_PACKAGE_SEND 6
#define ERR_CONNECT 7
#define ERR_NO_SERVER 8
#define ERR_CANT_SEND 9
#define ERR_NO_PROTOCOL 10

int AppendToLog(int errorCode);
char* GetError(void);
void SetErrorCallback(void (*func)(char*));

#endif