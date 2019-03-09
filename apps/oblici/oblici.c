#include <unistd.h>
#include "utils.h"
#include "oblici.h"


void print_menu() {
    printstr("Odaberite oblik:\n\r(1)Kvadrat\n\r(2)Trougao\n\r(3)Krug\n\r");
}

int menu() {
    int opt;
    char buf[10];
     while(1){
        print_menu();
        opt = read(1, buf, 10);
        buf[1] = '\0';
        if(opt > 2 || buf[0] < '0' || buf[0] > '3') {
            printstr("Odabrali ste pogresan oblik.\n\r");
            continue;
        }
        opt = atoi(buf) - 1;
        break;
    }
    return opt;
}

int main(int argc, char const *argv[])
{   
    char buf[11];
    void (*unos[]) (void) = {unos_kvadrat, unos_trougao, unos_krug};
    int  (*obim[]) (void) = {obim_kvadrat, obim_trougao, obim_krug};
    int  (*povrsina[]) (void) = {povrsina_kvadrat, povrsina_trougao, povrsina_krug};
    int opt = menu();

    unos[opt]();
    itoa(obim[opt](),buf,10);
    printstr("Obim:");
    printstr(buf);
    printstr("\n\r");
    itoa(povrsina[opt](),buf,10);
    printstr("Povrsina:");
    printstr(buf);

    _exit(0);
}
