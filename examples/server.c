#include <ec.h>
#include <stdio.h>
#include <stdlib.h>

ECServer server = { 0 };

void logErr(unsigned int code)
{
	unsigned int err = 0;
	const char *str = NULL;
	if((err = ECErrorCodeToString(code, &str)))
	{
		ECErrorCodeToString(err, &str);
		code = err;
	}
	printf("%s(Code: %d)\n", str, code);
}

void die(unsigned int code)
{
	logErr(code);
	exit(code);
}

void dataReceived(ECClient *client, char *ip, int port, int nsize, void *data)
{
	printf("Receieved message from client %s:%d\n\"%.*s\"\n", ip, port, nsize, data);
}

void connectionCreated(char *ip, int port)
{
	printf("Incoming connection from %s:%d\n", ip, port);

	unsigned int err = 0;
	if((err = ECServer_Send(&server, NULL, ip, -1, 5, "Pong")))
		logErr(err);
}

int main(int argc, char **argv)
{
	ecConfig config = { 0 };
	unsigned int err = 0;
	if((err = ECParseArgs(&config, argc, argv)))
		die(err);

	if((err = ECServer_Start(&server, &config, TCP, 16380, 5)))
		die(err);

	ECServer_OnConnectionCreate(connectionCreated);
	ECServer_OnDataReceive(dataReceived);

	getchar();
	if((err = ECServer_Shutdown(&server)))
		die(err);

	return 0;
}
