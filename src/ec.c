#include <ec.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <pthread.h>
#include <string.h>

char *errorStrings[] = {
	"Success",
	"NW uninitialized",
	"Null reference",
	"Invalid argument",
	"Invalid operation",
	"Network exception",
	"Index out or range"
};

#define ERR_SUCCESS 0
#define ERR_UNINITIALIZED 1
#define ERR_NULL_REFERENCE 2
#define ERR_INVALID_ARGUMENT 3
#define ERR_INVALID_OPERATION 4
#define ERR_NETWORK 5
#define ERR_INDEX_OUT_OF_RANGE 6

unsigned int ecInitialized = 0;

unsigned int UDP_ReceiveBufferLength = 1024;
unsigned char UDP_DefaultReceiveBuffer[1024] = { 0 };
void *UDP_ReceiveBuffer = UDP_DefaultReceiveBuffer;

unsigned int SetReceiveBuffer(void *ptr, unsigned int size)
{
	if(!ptr)
		return ERR_NULL_REFERENCE;
	UDP_ReceiveBuffer = ptr;
	UDP_ReceiveBufferLength = size;

	return ERR_SUCCESS;
}

void zero(void *ptr, unsigned int size)
{
	unsigned char *p = ptr;
	for(int i = 0; i < size; ++i)
		p[i] = 0;
}

char *ECErrorCodeToString(unsigned int errorCode)
{
	if(errorCode >= sizeof(errorStrings)/sizeof(errorStrings[0]))
		return errorStrings[ERR_INDEX_OUT_OF_RANGE];
	return errorStrings[errorCode];
}

void ipToStr(in_addr_t ip, char *str)
{
	snprintf(str, 16, "%03d.%03d.%03d.%03d", (ip>>24)&0xff, (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff);
}

unsigned int strToIP(in_addr_t *addr, char *ip)
{
	if(!ip || !addr)
		return ERR_NULL_REFERENCE;
	char *endp = NULL;
	long first = strtol(ip, &endp, 10);
	if(ip == endp || first > 255 || first < 0 || endp[0] != '.')
		return ERR_INVALID_ARGUMENT;

	long second = strtol(++endp, &endp, 10);
	if(ip == endp || second > 255 || second < 0 || endp[0] != '.')
		return ERR_INVALID_ARGUMENT;

	long third = strtol(++endp, &endp, 10);
	if(ip == endp || third > 255 || third < 0 || endp[0] != '.')
		return ERR_INVALID_ARGUMENT;

	long fourth = strtol(++endp, &endp, 10);
	if(ip == endp || fourth > 255 || fourth < 0)
		return ERR_INVALID_ARGUMENT;

	*addr = (in_addr_t)((first<<24) | (second<<16) | (third<<8) | fourth);
	return ERR_SUCCESS;
}

void SetFdToNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

unsigned int InitEC(int argc, char **argv)
{
	ecInitialized = 1;
	return ERR_SUCCESS;
}
