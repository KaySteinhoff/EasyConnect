#include <ec.h>
#include <stdio.h>
#include <stdlib.h>

unsigned int received = 0;

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

void dataReceived(int nsize, void *data)
{
	printf("Received message from server:\n\"%.*s\"\n", nsize, data);
	received = 1;
}

int main(int argc, char **argv)
{
	ecConfig config = { 0 };
	unsigned int err = 0;
	if((err = ECParseArgs(&config, argc, argv)))
		die(err);

	ECClient client = { 0 };
	if((err = ECClient_Connect(&client, &config, TCP, "127.0.0.1", 16380)))
		die(err);

	ECClient_OnDataReceive(dataReceived);

	puts("Connected to server! Sending message...");
	if((err = ECClient_Send(&client, 5, "Ping")))
		die(err);

	puts("Send message! Awaiting server response...");
	while(!received) { }
	puts("Terminating connection...");

	if((err = ECClient_Disconnect(&client)))
		die(err);

	return 0;
}
