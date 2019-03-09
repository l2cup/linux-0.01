#ifndef isdigit(c)
#define isdigit(c) ((c) <= '9' && (c) >= '0')
#endif

#define SIG_INT_STR_SIZE 11
 
#ifdef _UNISTD_H
#define printstr(x) write(1, x, strlen(x))
#define printerr(x) write(2, x, strlen(x))
#endif


void strrvs(unsigned char *str);
int itoa(int num, unsigned char *str, int base);
int atoi(const char *str);
