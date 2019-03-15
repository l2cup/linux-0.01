#include <unistd.h>
#include <fcntl.h>
#include "utils.h"
#include "scan.h"

int main(int argc, char const *argv[]) {
    char buf[64], output[64];
    int len, fd;
    
    load_config("scancodes.tbl", "ctrl.map");
    
    write(1,"Unesite naziv fajla, sa ekstenzijom:\n\r",39);
    len = read(0, buf, 64);
    buf[len - 1] = '\0';
    fd = open((const char *)buf, O_RDONLY);
    if(fd < 0){
        write(1,"Fajl nije nadjen, molimo vas unesite tacan naziv fajla sa ekstenzijom\n\r",72);
        _exit(0);
    }
    while(fgets(buf,fd) != 0) {
        len = process_scancode(atoi(buf),output);
        if(len > 0)
            write(1,output,len);
    }



    _exit(0);
    
}
