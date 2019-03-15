#ifndef _SCAN_H
#define _SCAN_H


#ifndef FGETS
int fgets(char *buff, int fd);
#endif

void load_config(const char *scancodes_filename, const char *mnemonic_filename);
int process_scancode(int scancode, char *buffer);

int load_scancodes(const char *scancodes_filename);
int load_mnemonics(const char *mnemonic_filename);

#endif
