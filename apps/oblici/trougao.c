#include <unistd.h>
#include <string.h>
#include "oblici.h"
#include "utils.h"

typedef struct tr_vars {
   int a, //Stranica 1
       b, //Stranica 2
       c, //Stranica 3
       h; //Visina
}tr_vars;

typedef union trougao {
    int tr_arr[4];
    tr_vars vars;
}Trougao;

static Trougao trougao;


void unos_trougao(void) {
    //47 is maximum 4 int size in chararacters + 3 spaces;
    char buf[47], *tok, sep[2] = " ";
    int len, i;
    printstr("Unesite stranice a, b i c i visinu h razdvojene razmakom.\n\r'a' 'b' 'c' 'h'\n\r");
    len = read(1, buf, 10);
    buf[len] = '\0';
    for(i = 0, tok = strtok(buf,sep); 
        i < 4; //i < 4 instead of tok so we can stop if user inputs >4 values.
        i++, tok = strtok(NULL, sep)) {
            printstr(tok);
            //We use character comparison instead of return value comparison to
            //reduce necesarry function calls
            trougao.tr_arr[i] = (*tok != '0') ? atoi(tok) : 1;
        }
}

int obim_trougao(void) {
    int o = 0;
    __asm__ __volatile__ ("addl %2, %0\n\t"
                          "addl %3, %0\n\t"
                          "addl %4, %0\n\t"
                          : "=r" (o)
                          : "0" (o), "g" (trougao.vars.a), "g" (trougao.vars.b),  "g"(trougao.vars.c));
    return o;
}


int povrsina_trougao(void) {
    int p = 0;
    __asm__ __volatile__ ("movl %2, %0\n\t"
                          "imull %3, %0\n\t"
                          "sarl $1, %0\n\t" 
                          : "=r" (p)
                          : "0" (p), "g" (trougao.vars.h), "g" (trougao.vars.a));
    
    return p;
}
