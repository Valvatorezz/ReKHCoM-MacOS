#ifndef G_CTYPE_H
#define G_CTYPE_H
static inline int isspace(int c) { return c == ' ' || (c >= 9 && c <= 13); }
static inline int isdigit(int c) { return c >= '0' && c <= '9'; }
static inline int isalpha(int c) { return (c | 32) >= 'a' && (c | 32) <= 'z'; }
static inline int isalnum(int c) { return isalpha(c) || isdigit(c); }
static inline int isupper(int c) { return c >= 'A' && c <= 'Z'; }
static inline int islower(int c) { return c >= 'a' && c <= 'z'; }
static inline int toupper(int c) { return islower(c) ? c - 32 : c; }
static inline int tolower(int c) { return isupper(c) ? c + 32 : c; }
#endif
