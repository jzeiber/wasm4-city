#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

double _max(double x, double y);
double _min(double x, double y);

char* strncpy(char* destination, const char* source, size_t num);

#ifdef __cplusplus
}
#endif