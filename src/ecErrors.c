#include <ecErrors.h>

unsigned int EC_ERR_SUCCESS = 0;
unsigned int EC_ERR_UNINITIALIZED = 1;
unsigned int EC_ERR_NULL_REFERENCE = 2;
unsigned int EC_ERR_INVALID_ARGUMENT = 3;
unsigned int EC_ERR_INVALID_OPERATION = 4;
unsigned int EC_ERR_NETWORK = 5;
unsigned int EC_ERR_INDEX_OUT_OF_RANGE = 6;

static const char *errorStrings[] = {
    "Success",
    "Easy Connect not initialized",
    "NULL reference",
    "Invalid argument",
    "Invalid operation",
    "Network error occurred",
    "Index out of range",
};

unsigned int ECErrorCodeToString(unsigned int errorCode, const char **ptr)
{
    if(!ptr)
        return EC_ERR_NULL_REFERENCE;
    if(errorCode >= sizeof(errorStrings)/sizeof(errorStrings[0]))
        return EC_ERR_INDEX_OUT_OF_RANGE;

    *ptr = errorStrings[errorCode];
    return EC_ERR_SUCCESS;
}