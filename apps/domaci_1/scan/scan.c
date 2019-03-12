#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "scan.h"

typedef struct mnemonic {
    char key;
    char text[63];
}Mnemonic;


static Mnemonic mn_arr[16];
static char sc_arr[2][47];

char sh_flag = 0;
char alt_flag = 0;
char ctrl_flag = 0;


void load_config(const char *scancodes_filename, const char *mnemonic_filename) {
    if(load_scancodes(scancodes_filename))
        _exit(0);
    if(load_mnemonics(mnemonic_filename))
        _exit(0);
}



int process_scancode(int scancode, char *buffer) {
    int result;

    return result;
}



int load_scancodes(const char *scancodes_filename) {
    int fd, i;
    char buf[128];
    if((fd = open(scancodes_filename, O_RDONLY)) < 0) {
       write(1,"Scancodes file can't be read, try with another file\n", 53);
       return fd;
    }

    for(i = 0; fgets(buf,fd) != 0; i++)
        strcpy(sc_arr[i], buf);
    
    return 1;
}

int load_mnemonics(const char *mnemonic_filename) {
    int fd, i;
    char buf[64], *tok, sep[2] = " ";
    
    if((fd = open(mnemonic_filename, O_RDONLY)) < 0) {
        write(1, "Mnemonic file can't be read, try with another file\n",52);
        return fd;
    }
    fgets(buf, fd);

    for(i = 0; fgets(buf,fd) != 0; i++) {
        tok = strtok(buf, sep);
        mn_arr[i].key = *tok;
        tok = strtok(NULL, "\n");
        strcpy(mn_arr[i].text, tok);
    }

    return 1;
}

//Implemented fgets here 
int fgets(char *buff, int fd){
    int i = 0;
    char c;
    do
	{
		if(!read(fd, &c, 1)) break;
		buff[i++] = c;	

	} while(c!= '\n' && c!= '\0' && c!= -1);
	buff[i] = '\0';
       
    return i;

}

