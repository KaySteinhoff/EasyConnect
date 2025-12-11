#include <ecErrors.h>
#include <ecArgs.h>
#include <stdlib.h>

#define IPV4_HASH_S 12355667
#define IPV4_HASH_L 1555179381
#define IPV6_HASH_S 12964801
#define IPV6_HASH_L 1841777355
#define RECEIVE_BUFFER_SIZE_HASH_S 3340852545
#define RECEIVE_BUFFER_SIZE_HASH_L 4025428527

static unsigned int UDP_ReceiveBufferLength = 1024;
static unsigned char UDP_DefaultReceiveBuffer[1024] = { 0 };

static unsigned int hash(char *str)
{
    unsigned int value = 5381;
    for (; *str != 0; ++str)
        value ^= (value * *str) + (*str * *str);
    return value;
}

static int ECProcessArg(ecConfig *config, int argc, char **argv, int index, unsigned int value)
{
    config->ipv = 4;
    config->receiveBufferSize= 1024; // Default receive buffer size
    config->receiveBuffer = UDP_DefaultReceiveBuffer;

    switch(value)
    {
        case IPV4_HASH_L: // "--ipv4"
        case IPV4_HASH_S: // "-4"
            config->ipv = 4;
            break;
        case IPV6_HASH_L: // "--ipv6"
        case IPV6_HASH_S: // "-6"
            config->ipv = 6;
            break;
        case RECEIVE_BUFFER_SIZE_HASH_L: // "--receive-buffer-size"
        case RECEIVE_BUFFER_SIZE_HASH_S: // "-rbs"
            if(index + 1 >= argc)
                break;

            config->receiveBufferSize = (unsigned int)atoi(argv[index + 1]);
            if(config->receiveBufferSize <= UDP_ReceiveBufferLength)
                return index + 1;

            config->receiveBuffer = malloc(config->receiveBufferSize);
            if(!config->receiveBuffer)
                break;

            UDP_ReceiveBufferLength = config->receiveBufferSize;
            return index + 1;
        default:
            break;
    }
    return index;
}

unsigned int ECParseArgs(ecConfig *config, int argc, char **argv)
{
    if(!config || !argv)
        return EC_ERR_NULL_REFERENCE;

    if(argc < 1)
        return EC_ERR_INVALID_ARGUMENT;

    for(int i = 0; i < argc; ++i)
    {
        unsigned int value = hash(argv[i]);
        i = ECProcessArg(config, argc, argv, i, value);
    }

    return EC_ERR_SUCCESS;
}
