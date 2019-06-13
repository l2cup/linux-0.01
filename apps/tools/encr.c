#include <unistd.h>
#include <utils.h>

int main(char *args) { 

    if(get_argc(args) < 2) {
        write(1,"Nema dovoljno argumenata",25);
        _exit(-1);
    }
    
    encr(get_argv(args,1));
    _exit(0);

}