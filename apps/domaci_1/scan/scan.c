#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "scan.h"

static char alt_count = 0;
static char alt_val = 0;

static char mn_arr[128][64];
//Scan codes array, used to access character for equivalent scancode
//[0][scancode] contains lowercase - no shift values for scancode
//[1][scancode] contains uppercase - shift values for scancode
static char sc_arr[2][128];

// Shift flag - used to access sc_arr[sh_flag][scancode]
static char sh_flag = 0;
static char alt_flag = 0;
static char ctrl_flag = 0;


void load_config(const char *scancodes_filename, const char *mnemonic_filename) {
    if(!load_scancodes(scancodes_filename))
        _exit(0);
    if(!load_mnemonics(mnemonic_filename))
        _exit(0);
    
    return;
}




int process_scancode(int scancode, char *buffer) {
    //Result will contain the current buffer length used for writing the buffer
    int result = 0;
    
    __asm__ __volatile__("cmpw $400, %2\n\t"
                         "je EOF\n\t"
                         "cmpl $302, %2\n\t"
                         "je ALT_UP\n\t"
                         "cmpl $301, %2\n\t"
                         "je CTRL_UP\n\t"
                         "cmpl $300, %2\n\t"
                         "je SHIFT_UP\n\t"
                         "cmpl $202, %2\n\t"
                         "je ALT_D\n\t"
                         "cmpl $201,%2\n\t"
                         "je CTRL_D\n\t"
                         "cmpl $200, %2\n\t"
                         "jg NOT_CONTROL\n\t"
                         "je SHIFT_D\n\t"
                         "NOT_CONTROL:\n\t"
                         "cmpb $1, (alt_flag)\n\t"
                         "je ALT\n\t"
                         "xorl %%ebx, %%ebx\n\t"
                         "xorl %%edx, %%edx\n\t"
                         "movb (sh_flag), %%dl\n\t"
                         "shlb $7, %%dl\n\t"
                         "addl %2, %%edx\n\t"
                         "movb sc_arr(,%%edx,1), %%bl\n\t"
                         "movl %0, %%edx\n\t"
                         "cmpb $1, (ctrl_flag)\n\t"
                         "je CTRL\n\t"
                         "movb %%bl, (%%edx)\n\t"
                         "movb $1, %1\n\t"
                         "jmp END\n\t"
                         "CTRL:\n\t"
                         "cld\n\t"
                         "shll $6, %%ebx\n\t"
                         "xorl %%ecx, %%ecx\n\t"
                         "movw $64, %%cx\n\t"
                         "movb %%cl, %1\n\t"
                         "lea mn_arr(,%%ebx), %%esi\n\t"
                         "lea (%%edx), %%edi\n\t"
                         "rep movsb\n\t"
                         "movb $64, %1\n\t"
                         "jmp END\n\t"
                         "ALT:\n\t"
                         "xorl %%ecx, %%ecx\n\t"
                         "movl %2, %%ecx\n\t"
                         "xorl %%eax, %%eax\n\t"
                         "cmpb $4, (alt_count)\n\t"
                         "je END\n\t"
                         "incb (alt_count)\n\t"
                         "cmpl $1, (alt_val)\n\t"
                         "je ALT_100\n\t"
                         "movb $10, %%al\n\t"
                         "jmp ALT_CONTINUE\n\t"
                         "ALT_100:\n\t"
                         "movb $100, %%al\n\t"
                         "ALT_CONTINUE:\n\t"
                         "mulb (alt_val)\n\t"
                         "jo ALT_OF\n\t"
                         "addb %%al, %%cl\n\t"
                         "movb %%cl, (alt_val)\n\t"
                         "jmp END\n\t"
                         "CTRL_D:\n\t"
                         "movb $1, (ctrl_flag)\n\t"
                         "jmp END\n\t"
                         "SHIFT_D:\n\t"
                         "movb $1,(sh_flag)\n\t"
                         "jmp END\n\t"
                         "ALT_D:\n\t"
                         "movb $1, (alt_flag)\n\t"
                         "jmp END\n\t"
                         "SHIFT_UP:\n\t"
                         "movb $0,(sh_flag)\n\t"
                         "jmp END\n\t"
                         "ALT_UP:\n\t"
                         "xorl %%edx, %%edx\n\t"
                         "movb $0, (alt_count)\n\t"
                         "movb $0, (alt_flag)\n\t"
                         "movb (alt_val), %%dl\n\t"
                         "movb $0, (alt_val)\n\t"
                         "movl %0, %%ebx\n\t"
                         "movb %%dl, (%%ebx)\n\t"
                         "movb $1, %1\n\t"
                         "jmp END\n\t"
                         "CTRL_UP:\n\t"
                         "movb $0, (ctrl_flag)\n\t"
                         "jmp END\n\t"
                         "ALT_OF:\n\t"
                         //Overflow on ascii value will set 7 as it does not 
                         //actually trigger the bell character in the terminal
                         //It will be refined later with an error message. 
                         "movb $7, (alt_val)\n\t" 
                         "jmp END\n\t"
                         "EOF:\n\t"
                         "movl %0, %%edx\n\t"
                         "movb $0, (%%edx)\n\t"
                         "END:\n\t"
                         : "=m" (buffer), "=m" (result)
                         : "m" (scancode) 
                         :"memory", "%edx","%ebx", "%eax", "%esi", "%edi");
                          
    
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

    
    write(1,"Succesfuly read scancodes.\n",28);
    return 1;
}

int load_mnemonics(const char *mnemonic_filename) {
    int fd, i;
    char buf[64], *tok, sep[2] = " ", val;
    
    if((fd = open(mnemonic_filename, O_RDONLY)) < 0) {
        write(1, "Mnemonic file can't be read, try with another file\n",52);
        return fd;
    }
    fgets(buf, fd);

    for(i = 0; fgets(buf,fd) != 0; i++) {
        tok = strtok(buf, sep);
        val = *tok;
        tok = strtok(NULL, "\n");
        strcpy(mn_arr[val], tok);
    }
    write(1,"Succesfuly read mnemonics.\n",28);
    return 1;
}

//Implemented fgets here so we can skip including utils.h  
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

