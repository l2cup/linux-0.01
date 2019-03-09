#include <string.h>
#include <unistd.h>
#include "utils.h"
#define MAX_LEN 100



int main(void) {

    char buf[MAX_LEN];
    int num1 = 0, num2 = 0, i, len;
    
    len = read(0, buf, MAX_LEN);
    buf[len] = '\0';
    for(i = 0;i<=len; i++) {
        if(isdigit(buf[i])) {
            num1 = num1 * 10 + (buf[i] - '0');
        }
        else if (buf[i] == ' ' || buf[i] == '\0'){  
            num2 += num1;
            num1 =0;
        }
    }
    write(1, buf, len);
    write(1,"\n",2);
    itoa(num2, buf,10);
    write(1, buf,strlen(buf)); 
    _exit(0);
}