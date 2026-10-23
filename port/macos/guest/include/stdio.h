#ifndef G_STDIO_H
#define G_STDIO_H
#include <stddef.h>
#include <stdarg.h>
typedef struct GFile FILE;
extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
FILE* fopen(const char*, const char*);
int fclose(FILE*);
size_t fread(void*, size_t, size_t, FILE*);
size_t fwrite(const void*, size_t, size_t, FILE*);
int fseek(FILE*, long, int);
long ftell(FILE*);
void rewind(FILE*);
int fflush(FILE*);
char* fgets(char*, int, FILE*);
int fputs(const char*, FILE*);
int fputc(int, FILE*);
int fgetc(FILE*);
int ungetc(int, FILE*);
int feof(FILE*);
int ferror(FILE*);
int fprintf(FILE*, const char*, ...) __attribute__((format(printf, 2, 3)));
int vfprintf(FILE*, const char*, va_list);
int printf(const char*, ...) __attribute__((format(printf, 1, 2)));
int puts(const char*);
int snprintf(char*, size_t, const char*, ...) __attribute__((format(printf, 3, 4)));
int vsnprintf(char*, size_t, const char*, va_list);
int sprintf(char*, const char*, ...);
int sscanf(const char*, const char*, ...);
FILE* popen(const char*, const char*);
int pclose(FILE*);
int remove(const char*);
int rename(const char*, const char*);
#endif
