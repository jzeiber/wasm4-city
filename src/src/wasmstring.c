#include "wasmstring.h"

size_t strlen(const char *str)
{
    size_t len=0;
    size_t pos=0;
    while(str[pos++]!='\0')
    {
        len++;
    }
    return len;
}
