#include <stdio.h>
#include <unistd.h>
#include <ec.h>

void DataReceive(int nsize, void *data)
{
	for(int i = 0; i < nsize; ++i)
		printf("%c", ((char*)data)[i]);
	puts("");
}

void ServerDisconnected(void)
{
	puts("Connection was terminated by the server side.");
}

int main(int argc, char **argv)
{
	ECClient client = { 0 };
	unsigned int err = 0;

	if((err = InitEC(argc, argv)))
	{
		printf("%s(Code: %d)\n", ECErrorCodeToString(err), err);
		return 1;
	}

	ECClient_OnDataReceive(DataReceive);
	ECClient_OnConnectionTerminated(ServerDisconnected);

	if((err = ECClient_Connect(&client, UDP, "127.0.0.1", 16380)))
	{
		printf("%s(Code: %d)\n", ECErrorCodeToString(err), err);
		return 2;
	}

	if((err = ECClient_Send(&client, 5, "Hello")))
	{
		printf("%s(Code: %d)\n", ECErrorCodeToString(err), err);
		return 3;
	}

	getchar();

	if((err = ECClient_Disconnect(&client)))
	{
		printf("%s(Code: %d)\n", ECErrorCodeToString(err), err);
		return 4;
	}
	return 0;
}
