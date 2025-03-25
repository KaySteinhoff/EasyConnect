#include <unistd.h>
#include <stdio.h>
#include <ec.h>

ECServer server = { 0 };

void ClientConnected(char *ip, int port)
{
	printf("Connected with %s on port %d\n", ip, port);
}

void DataReceived(ECClient *client, char *ip, int port, int nsize, void *data)
{
	printf("%s(Port: %d):'", ip, port);
	for(int i = 0; i < nsize; ++i)
		printf("%c", ((char*)data)[i]);
	puts("'");

	int err = 0;
	if((err = ECServer_Send(&server, NULL, NULL, client->clientfd, 5, "World")))
		printf("%s(Code: %d)\n", ECErrorCodeToString(err), err);
}

void ClientDisconnected(char *ip, int port)
{
	printf("%s(Port: %d) disconnected!\n", ip, port);
}

int main(int argc, char **argv)
{
	unsigned int err = 0;

	if((err = InitEC(argc, argv)))
	{
		printf("%s(Code:%d)\n", ECErrorCodeToString(err), err);
		return 1;
	}

	ECServer_OnConnectionCreate(ClientConnected);
	ECServer_OnDataReceive(DataReceived);
	ECServer_OnConnectionTerminated(ClientDisconnected);

	if((err = ECServer_Start(&server, TCP, 16380, 5)))
	{
		printf("%s(Code:%d)\n", ECErrorCodeToString(err), err);
		return 2;
	}
	getchar();
	return 0;
}
