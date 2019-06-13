#include <linux/fs.h>
#ifndef _ENCRYPTION_H
#define _ENCRYPTION_H

#define OK_LVL(x) (x == 1 || x == 2 || x == 3)
#define OK_KEYLEN(x) (x > 3 && x < 513 &&((x &(x - 1)) == 0))
#define rdtsc(x) __asm__ volatile("rdtsc\n\tmovw %%eax, %0\n\t":"=m" (x)::"%eax", "memory")

#ifndef isdigit(c)
#define isdigit(c) ((c) <= '9' && (c) >= '0')
#endif

//can be easily used with casting to dir_entry
// struct encr_data {
//    unsigned short inode;
//    char name[14]; 
// };


#define MAX_ENCRYPTED 64 // Maximum number of allowed encrypted files
#define ENCRYPTION 0
#define DECRYPTION 1

//Constant strings, used for printing error messages.

static const char KEY_ERROR_LEN[] = "There was an error with the key, please try another one.\nNotice: The key must be shorter than 1024 character and its size must be a power of 2.\n";

static const char KEY_ERROR_LEVEL_SIZE[] = "Level provided is not the adequate size. Please try with sizes 4, 8 or 16\n";

static const char KEY_ERROR_NULL[] = "There is no key set. Please use 'keyset \"key\"' or 'keygen' to set a key.\n";

static const char KEY_ERROR_EXPIRED[] = "Key expired. Please set a new key, either localy or globaly using 'keyset'\n";

static const char KEY_ERROR_HASH[] = "Key hashes don't match, please set an appropriate key if you want to decrypt\n";

static const char DECR_ERROR_NOT_ENCR[] = "File is not encrypted. If you want to encrypt it use 'encr \"filename\"'\n";

static const char ENCR_ERROR_ALREADY_ENCR[] = "File is already encrypted. If you want to decrypt it use 'decr \"filename\"'\n";

static const char ENCR_ERROR_ISDIR[] = "This is a directory. Please select a file\n";

static const char ENCR_ERROR_NOT_FOUND[] = "File not found. Please enter the right filename\n";

static const char ENCR_ERROR_MAX_ENCRYPTED[] = "Maximum number of encypted files is reached. Please decrypt some files before encrypting more.\n";

static const char ENCR_ERROR_FILE_EMPTY[] = "Encryption file empty, no encrypted files";


extern char global_key_flag;
extern volatile unsigned short encrypted_file_inode;
extern char _key[513];

//Function declarations
int check_time(void);
char * get_key(struct m_inode *node);
char * check_hash(struct m_inode *node);
long hash(unsigned char *str);
int _edit_file(char const *filename, int type);
int decrypt(struct m_inode *node);
int encrypt(struct m_inode *node);
int encrypt_block(struct m_inode *node,int rows,char *buff, char *key);
int encrypt_dir(struct m_inode *node);
int decrypt_block(struct m_inode *node,char *buff, int block_size);
int _decrypt_block(char * buff, int block_size, char *key);
int decrypt_dir(struct m_inode *node);
void set_files_inode(void);
int _generate_random(unsigned int start_range, unsigned int end_range);
void get_fs_string(char * dst, const char *src);
void quick_sort(char *arr, int low,int high,char *mat, int cols, int rows);
void _quick_sort(char *arr, int low, int high,char *mat,int cols,int rows);
int partition(char * arr,int low, int high,char *mat,int cols,int rows);
void init_sort_num(void);
void swap(char* a, char* b);
void columns_swap(int cols,int rows,char *mat, int col1, int col2);
int is_encr(struct m_inode * node);
int get_encrypted_inode_index(struct m_inode * node);
void set_encrypted(struct m_inode *node,char *key);
void set_decrypted(struct m_inode *node);
void check_encrypted(void);
int itoa(long num, unsigned char * str);
long atoi(const char * str);
int load_encryption_file(void);
int write_encryption_file(void);
#endif