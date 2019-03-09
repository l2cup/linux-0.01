#include <unistd.h>
#include "utils.h"
#include "oblici.h"

typedef struct krug {
    int r;
}Krug;

static Krug krug;


void unos_krug(void) {
    char buf[11];
    int len;
    printstr("Unesite poluprecnik kruga: ");
    len = read(1, buf, 11);
    buf[len] = '\0';
    krug.r = atoi(buf);
    if(krug.r <=0 ) krug.r = 1;
}


int obim_krug(void) {
    int o;
    __asm__ __volatile__ ("movl %2, %0\n\t"
                          "imull $6, %0\n\t"
                          : "=r" (o)
                          : "0" (o), "g" (krug.r));

    return o;
}

int povrsina_krug(void) {
    int p;
    __asm__ __volatile__ ("movl %2, %0\n\t"
                          "imul %1, %0\n\t"
                          "imul $3, %0\n\t"
                          : "=r" (p)
                          : "0" (p) , "g" (krug.r));

    return p;
}