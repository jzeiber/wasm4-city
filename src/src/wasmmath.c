#include "wasmmath.h"
#include "miscfuncs.h"

#include <stdint.h>

//#define USE_BUILTIN_SIN     1
//#define USE_BUILTIN_COS     1
#define USE_BUILTIN_TRUNC   1
#define USE_BUILTIN_FMOD    1
#define USE_BUILTIN_CEIL    1
#define USE_BUILTIN_FLOOR   1
#define USE_BUILTIN_SQRT    1

// https://stackoverflow.com/questions/11930594/calculate-atan2-without-std-functions-or-c99#
// Approximates atan2(y, x) normalized to the [0,4) range
// with a maximum error of 0.1620 degrees

float normalized_atan2( float y, float x )
{
    static const uint32_t sign_mask = 0x80000000;
    static const float b = 0.596227f;

    // Extract the sign bits
    //uint32_t ux_s  = sign_mask & (uint32_t &)x;
    //uint32_t uy_s  = sign_mask & (uint32_t &)y;
    uint32_t ux_s  = sign_mask & (*(uint32_t *)&x);
    uint32_t uy_s  = sign_mask & (*(uint32_t *)&y);

    // Determine the quadrant offset
    float q = (float)( ( ~ux_s & uy_s ) >> 29 | ux_s >> 30 ); 

    // Calculate the arctangent in the first quadrant
	float val = ( b * x * y );
	if( val < 0.0 )
	{
		val = 0.0 - val;
	}
    float bxy_a = val;
    float num = bxy_a + y * y;
    float atan_1q =  num / ( x * x + bxy_a + num );

    // Translate it to the proper quadrant
    uint32_t uatan_2q = (ux_s ^ uy_s) | (*(uint32_t *)&atan_1q);
    return q + (*(float *)&uatan_2q);
}


/*
	quick and dirty sin / cos 
	https://stackoverflow.com/questions/2284860/how-does-c-compute-sin-and-other-math-functions
*/

double _pow(double a, double b) {
    double c = 1;
    for (int i=0; i<b; i++)
        c *= a;
    return c;
}

double _fact(double x) {
    double ret = 1;
    for (int i=1; i<=x; i++) 
        ret *= i;
    return ret;
}

double _sin(double x) {
    #ifdef USE_BUILTIN_SIN
        return __builtin_sin(x);
    #else
    double y = x;
    double s = -1;
    for (int i=3; i<=100; i+=2) {
        y+=s*(_pow(x,i)/_fact(i));
        s *= -1;
    }  
    return y;
    #endif
}
double _cos(double x) {
    #ifdef USE_BUILTIN_COS
        return __builtin_cos(x);
    #else
    double y = 1;
    double s = -1;
    for (int i=2; i<=100; i+=2) {
        y+=s*(_pow(x,i)/_fact(i));
        s *= -1;
    }  
    return y;
    #endif
}
double _tan(double x) {
     return (_sin(x)/_cos(x));
}
/* end QAD sin / cos */

double deg2rad(const double deg)
{
	return deg*M_PI/180.0;
}

double rad2deg(const double rad)
{
	return rad*180.0/M_PI;
}

double _atan2(const double y, const double x)
{
	return deg2rad(normalized_atan2(y,x)*90.0);
}

/*
double _sqrt(double x)
{
  // Max and min are used to take into account numbers less than 1
  double lo = _min(1, x), hi = _max(1, x), mid;

  // Update the bounds to be off the target by a factor of 10
  while(100 * lo * lo < x) lo *= 10;
  while(100 * hi * hi > x) hi *= 0.1;

  for(int i = 0 ; i < 100 ; i++){
      mid = (lo+hi)/2;
      if(mid*mid == x) return mid;
      if(mid*mid > x) hi = mid;
      else lo = mid;
  }
  return mid;
}
*/

double _sqrt(double x)
{
    #ifdef USE_BUILTIN_SQRT
        return __builtin_sqrt(x);
    #else
	if(x==0 || x==1)
	{
		return x;
	}
	double guess=x/2.0;
	for(int i=0; i<10; i++)
	{
		guess-=(((guess*guess)-x) / (2.0*guess));
	}
	return guess;
    #endif
}

double _dabs(double x)
{
	return (x < 0 ? -x : x);
}

int64_t _abs(int64_t x)
{
    return (x < 0 ? -x : x);
}

double _floor(double x)
{
    #ifdef USE_BUILTIN_FLOOR
        return __builtin_floor(x);
    #else
    int64_t n=(int64_t)x;
    double d=(double)n;
    if(d==x || x>=0)
    {
        return d;
    }
    else
    {
        return d-1;
    }
    #endif
}

double _ceil(double x)
{
    #ifdef USE_BUILTIN_CEIL
        return __builtin_ceil(x);
    #else
    int64_t n=(int64_t)x;
    double d=(double)n;
    if(d==x || x<=0)
    {
        return d;
    }
    else
    {
        return d+1;
    }
    #endif
}

double _trunc(double x)
{
    #ifdef USE_BUILTIN_TRUNC
        return __builtin_trunc(x);
    #else
    if(x==0)
    {
        return 0;
    }
    else if(x>0)
    {
        return _floor(x);
    }
    else
    {
        return _ceil(x);
    }
    #endif
}

double _fmod(double val, double mod)
{
    #ifdef USE_BUILTIN_FMOD
        return __builtin_fmod(val,mod);
    #else
    return val-_trunc(val/mod)*mod;
    #endif
}
