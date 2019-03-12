#include <unistd.h>
#include "utils.h"
#include "scan.h"

int main(int argc, char const *argv[]) {
    
    load_config("scancodes.tbl", "ctrl.map");

    _exit(0);
    
}
