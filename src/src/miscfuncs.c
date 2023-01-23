#include "miscfuncs.h"

double _max(double x, double y)
{
	return x>y ? x : y;
}

double _min(double x, double y)
{
	return x<y ? x : y;
}

char* strncpy(char* destination, const char* source, size_t num)
{
    // return if no memory is allocated to the destination
    if (destination == NULL) {
        return NULL;
    }
 
    // take a pointer pointing to the beginning of the destination string
    char* ptr = destination;
 
    // copy first `num` characters of C-string pointed by source
    // into the array pointed by destination
    while (*source && num--)
    {
        *destination = *source;
        destination++;
        source++;
    }
 
    // null terminate destination string
    *destination = '\0';
 
    // the destination is returned by standard `strncpy()`
    return ptr;
}
