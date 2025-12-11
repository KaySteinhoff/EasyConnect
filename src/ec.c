#include <ec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __unix__
	#include <unistd.h>
	#include <sys/fcntl.h>
#endif

void zero(void *ptr, unsigned int size)
{
	unsigned char *p = ptr;
	for(int i = 0; i < size; ++i)
		p[i] = 0;
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
