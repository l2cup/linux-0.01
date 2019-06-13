#include <unistd.h>
#include <utils.h>

int main(char *args) { 

    if(get_argc(args) > 1) {
        write(1,"Previse argumenata",19);
        _exit(-1);
    }
    char buff[513];
    echo_off();
    int num = read(0,buff,512);
    buff[num - 1] = '\0';
    echo_on();
    keyset(buff,1);
    _exit(0);

}