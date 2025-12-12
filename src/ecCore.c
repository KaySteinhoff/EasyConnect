#include <ecErrors.h>
#include <ecCore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

unsigned int ipv42str(struct in_addr addr, char *ip)
{
	if(!ip)
		return EC_ERR_NULL_REFERENCE;

	snprintf(ip, 16, "%03d.%03d.%03d.%03d", addr.s_addr & 0xff, (addr.s_addr >> 8) & 0xff, (addr.s_addr >> 16) & 0xff, (addr.s_addr >> 24) & 0xff);
	return EC_ERR_SUCCESS;
}

unsigned int ipv62str(struct in6_addr addr, char *ip)
{
	if(!ip)
		return EC_ERR_NULL_REFERENCE;

	snprintf(ip, 40,
		"%02x%02x:"
		"%02x%02x:"
		"%02x%02x:"
		"%02x%02x:"
		"%02x%02x:"
		"%02x%02x:"
		"%02x%02x:"
		"%02x%02x"
	, addr.s6_addr[0] , addr.s6_addr[1]
	, addr.s6_addr[2] , addr.s6_addr[3]
	, addr.s6_addr[4] , addr.s6_addr[5]
	, addr.s6_addr[6] , addr.s6_addr[7]
	, addr.s6_addr[8] , addr.s6_addr[9]
	, addr.s6_addr[10], addr.s6_addr[11]
	, addr.s6_addr[12], addr.s6_addr[13]
	, addr.s6_addr[14], addr.s6_addr[15]);

	return EC_ERR_SUCCESS;
}

unsigned int str2ipv4(char *ip, struct in_addr *addr)
{
	if(!ip || !addr)
		return EC_ERR_NULL_REFERENCE;

	addr->s_addr = 0;
	char *startp = ip, *endp = NULL;
	int i = 0;
	do
	{
		long o = strtol(startp, &endp, 10);
		if(o == 0 && startp == endp)
			return EC_ERR_INVALID_ARGUMENT;

		addr->s_addr |= o << (8 * i);
		startp = endp + 1;
	} while(*endp == '.' && ++i < 4);

	return EC_ERR_SUCCESS;
}

unsigned int str2ipv6(char *ip, struct in6_addr *addr)
{
	if(!ip || !addr)
		return EC_ERR_NULL_REFERENCE;

	char *startp = ip, *endp = NULL;
	int i = 0;
	uint16_t *in6_addr = (uint16_t*)addr->s6_addr;
	do
	{
		long o = strtol(startp, &endp, 16);
		if (o == 0 && endp == startp)
			return EC_ERR_INVALID_ARGUMENT;

		in6_addr[i++] = o;
		startp = endp + 1;
	} while(*endp == ':' && i < 8);

	return EC_ERR_SUCCESS;
}
