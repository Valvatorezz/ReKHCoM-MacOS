#ifndef G_STDLIB_H
#define G_STDLIB_H
#include <stddef.h>
void* malloc(size_t);
void* calloc(size_t, size_t);
void* realloc(void*, size_t);
void free(void*);
void exit(int) __attribute__((noreturn));
void abort(void) __attribute__((noreturn));
char* getenv(const char*);
int abs(int);
long labs(long);
int atoi(const char*);
long strtol(const char*, char**, int);
unsigned long strtoul(const char*, char**, int);
unsigned long long strtoull(const char*, char**, int);
double atof(const char*);
void qsort(void*, size_t, size_t, int (*)(const void*, const void*));
int rand(void);
void srand(unsigned);
#define RAND_MAX 0x7fffffff
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif
