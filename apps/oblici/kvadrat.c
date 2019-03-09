#include <unistd.h>
#include "oblici.h"
#include "utils.h"

typedef struct kvadrat {
    int a;
}Kvadrat;

static Kvadrat kvadrat;

void unos_kvadrat(void) {
    char buf[10];
    int len;
    printstr("Unesite stranicu a: ");
    len = read(1, buf, 10);
    buf[len] = '\0';
    kvadrat.a = atoi(buf);
    if(kvadrat.a <= 0) kvadrat.a = 1;
    
}


int obim_kvadrat(void){
    int o;
    __asm__ __volatile__ ("movl %2, %0\n\t"
                          "shll $2, %0\n\t"
                          : "=r" (o)
                          : "0" (o), "g" (kvadrat.a));
    
    return o;
}

int povrsina_kvadrat(void){ 
    int p;
    __asm__ __volatile__ ("movl %2, %0\n\t"
                          "imull %1, %0\n\t"
                          : "=r" (p)
                          : "0" (p), "g" (kvadrat.a));
    
    return p;
}




