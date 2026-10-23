#ifndef G_MATH_H
#define G_MATH_H
#define M_PI 3.14159265358979323846
double sin(double);
double cos(double);
double tan(double);
double atan2(double, double);
double pow(double, double);
double exp(double);
double log(double);
double fmod(double, double);
float sinf(float);
float cosf(float);
float powf(float, float);
#define sqrt __builtin_sqrt
#define sqrtf __builtin_sqrtf
#define fabs __builtin_fabs
#define fabsf __builtin_fabsf
#define floor __builtin_floor
#define floorf __builtin_floorf
#define ceil __builtin_ceil
#define ceilf __builtin_ceilf
#define round __builtin_round
#define roundf __builtin_roundf
long lround(double);
long lroundf(float);
#endif
