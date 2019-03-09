#include "utils.h"

//Reverses a string
void strrvs(unsigned char *str) {
    char c;
    int i, j;
    for(i =0, j = strlen(str) - 1; i < j; i++, j--) {
        c = str[i];
        str[i] = str[j];
        str[j] = c;
    }
}

//Integer to string conversion
int itoa(int num, unsigned char *str,int base) {
    int i = 0, 
        val = num, 
        sign;

    if((sign = val) < 0)
        val = -val;
    
    do {
        str[i++] = (val % base) + '0';
    } while((val/=base) > 0);

    if(sign < 0)
        str[i++] = '-';

    str[i] = '\0';
    strrvs(str);
    return i;
}

//Converts a string to an int
int atoi(const char *str) {
    int r = 0, 
        i = 0;
    do {
        r = r * 10 + str[i++] - '0';
    } while(isdigit(str[i]));

    return r;
}