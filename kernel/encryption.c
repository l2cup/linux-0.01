#include <encryption.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <sys/stat.h>
#include <asm/segment.h>
#include <string.h>
#ifdef _ENCRYPTION_H


/* 
 * This is where the encryption is implemented, it uses a matrix transpose
 * algorithm, a fairly simple encryption. This is a POC. If you really need
 * encrypting files on an old linux system, please make yourself at home and
 * implement AES and change the system calls to use your encryption method.
 * 
 * l2cup 2019
 */


char _key[513];
char _terminator = '\0';
char global_key_flag = 0;
long keyset_time = 0;
int sort_num[512];

int encrypted[MAX_ENCRYPTED];
long encrypted_hashes[MAX_ENCRYPTED];
volatile unsigned short encrypted_file_inode = 0;
char encr_counter = 0;



int sys_init_encryption(void) {
    static file_loaded = 0;
    if(!file_loaded) {
        set_files_inode();
        load_encryption_file();
    }
    file_loaded = 1;
    return 0;
}


int sys_key(void) {
    if(_key[0] != 0)
        printk("Key: %s\n",_key);
    else
        printk("%s",KEY_ERROR_NULL);
    

    return 1;
}


int sys_keyclear(int global) {
    if(global) {
        global_key_flag = 0;
        keyset_time = 0;
        memset(_key,0,512);
    }
    else {
        current->keyset_time = 0;
        memset(current->encryption_key,0,512);
    }
    return 1;
}


int sys_keyset(const char *key,int global) {
    int len;
    char key_temp[513],c;

    get_fs_string(key_temp,key);
    len = strlen(key_temp);


    if(!OK_KEYLEN(len)) {
        printk("%s",KEY_ERROR_LEN);
        return -1;
    }

    if(global) {
        strncpy(_key,key_temp,len);
        keyset_time = CURRENT_TIME;
        global_key_flag = 1;
    }
    else {
        strncpy(current->encryption_key,key_temp,len);
        current->keyset_time = CURRENT_TIME;
    }
    printk("Key is set.\n");

    return 1;
}

int sys_encr(const char *filename) {

    return _edit_file(filename,ENCRYPTION);
}

int sys_decr(const char *filename) {

    return _edit_file(filename,DECRYPTION);

}

int sys_keygen(int level) {

    if(!OK_LVL(level)) {
        printk("%s",KEY_ERROR_LEVEL_SIZE);
        return -1;
    }
    
    level = 1 << ++level;
    memset(_key,0,513);
    int i;
    for(i = 0; i < level; i ++) {
        do _key[i] = (char) _generate_random(32,126);
            while(i!=0 && _key[i] == _key[i - 1]);
    }

    _key[i+1] = '\0';
    printk("Succesfully generated a key of size %d\nKey: %s\n",level, _key);
    global_key_flag = 1;
    return 1;

}

int check_time() {

    if(CURRENT_TIME - keyset_time >= 120) {
        memset(_key, 0, 512);
        global_key_flag = 0;
    }
    if(CURRENT_TIME - current->keyset_time > 45) 
        memset(current->encryption_key, 0, 512);
    return 1;
}

//Just checks both keys, and return the not null one, if both are null returns NULL
char * get_key(struct m_inode * node) {
    check_time();
    if(current->encryption_key[0] != 0)
        return current->encryption_key;
    else if(global_key_flag)
        return _key;
    else
        return NULL;
}

//Check hash return appropriate key, with local key having advantage.
char * check_hash(struct m_inode * node) {
    check_time();
    int index = get_encrypted_inode_index(node);
    if(index == -1)
        return NULL;
    
    if(current->encryption_key[0] != 0) {
        if(encrypted_hashes[index] == hash(current->encryption_key))
            return current->encryption_key; 
    }
    if(global_key_flag) {
        if(encrypted_hashes[index] == hash(_key))
        return _key;
    }

    return NULL;
}

long hash(unsigned char *str) {
    long hash = 0;

    while(*str != '\0' && *str != 0) {
        hash += *str;
        str++;
    }

    return hash;
}

 /*We go through this function because of error checking.
 *It will call the appropriate function, universal and shared code is here */
int _edit_file(char const *filename, int type) {

    //Error checking
    struct m_inode * node = namei(filename); 
    if(node == NULL) {
        iput(node);
        printk("%s",ENCR_ERROR_NOT_FOUND);
        return -1;
    }
    int is_encrypted = is_encr(node);

    if(type == ENCRYPTION && is_encrypted) {
        printk("%s",ENCR_ERROR_ALREADY_ENCR);
        return -1;
    }

    if(type == DECRYPTION && !is_encrypted) {
        printk("%s",DECR_ERROR_NOT_ENCR);
        return -1;
    }

    if(type == ENCRYPTION)
        return encrypt(node);
    else
        return decrypt(node);

}

int decrypt(struct m_inode *node) {

    struct buffer_head *bh;
    char *key = check_hash(node);
    if(key == NULL) {
        printk("%s",KEY_ERROR_HASH);
        return -1;
    }
    int zones = node->i_size % BLOCK_SIZE ? 
                node->i_size/BLOCK_SIZE + 1 : node->i_size/BLOCK_SIZE;
    
    int size, i,j;
    char transpose_matrix[BLOCK_SIZE];

    for(i = 0, j = 0; i < zones && j < node->i_size;i ++) {

        size = (zones > 1) && i != (zones - 1) 
            ? BLOCK_SIZE : (node->i_size - i * BLOCK_SIZE);
        
        bh = bread(node->i_dev,bmap(node,i));
        for(j = 0; j < BLOCK_SIZE && j < node->i_size - i * BLOCK_SIZE; j ++)
            transpose_matrix[j] = bh->b_data[j];

        _decrypt_block(transpose_matrix,size,key);

       for(j = 0;j < BLOCK_SIZE && j < node->i_size - i * BLOCK_SIZE; j ++)
            bh->b_data[j] = transpose_matrix[j];
        bh->b_dirt = 1;
        brelse(bh);
    }

    if(S_ISDIR(node->i_mode))
        decrypt_dir(node);

    set_decrypted(node);
    
}

int encrypt(struct m_inode *node) {


    if(S_ISDIR(node->i_mode))
        encrypt_dir(node);

    struct buffer_head *bh;
    char * key = get_key(node);
    if(key == NULL) {
        printk("%s",KEY_ERROR_EXPIRED);
        return -1;
    }
    int columns = strlen(key);

    int zones = node->i_size % BLOCK_SIZE ? 
                node->i_size/BLOCK_SIZE + 1 : node->i_size/BLOCK_SIZE;
    
    int rows,i,j;
    char transpose_matrix[BLOCK_SIZE];

    for(i = 0, j = 0; i < zones && j < node->i_size;i ++) {
            
        rows = (zones > 1) && i!= (zones - 1) ? BLOCK_SIZE/columns :
        ((node->i_size - i * BLOCK_SIZE) / columns) + 1;
        

        bh = bread(node->i_dev,bmap(node,i));
        for(j = 0; j < BLOCK_SIZE && j < node->i_size - i * BLOCK_SIZE; j ++) {
                transpose_matrix[j] = bh->b_data[j];
        }

        encrypt_block(node,rows,transpose_matrix,key);

        for(j = 0;j < BLOCK_SIZE && j < node->i_size - i * BLOCK_SIZE; j ++) {
            bh->b_data[j] = transpose_matrix[j];
        }
        bh->b_dirt = 1;
        brelse(bh); 
    } 
    set_encrypted(node,key);
}

int encrypt_block(struct m_inode *node,int rows,char *buff, char *key) {

    char sorted_key[strlen(key)];
    strcpy(sorted_key,key);
    quick_sort(sorted_key, 0, strlen(key) - 1, buff, strlen(key), rows);

}


int encrypt_dir(struct m_inode *node) {

    struct dir_entry *den;
    struct buffer_head * bh;
    int i, j, num;

    num = (node->i_size / sizeof(struct dir_entry));
    if(num < 2) {
        iput(node);
        return -1;
    }
    int zones = node->i_size % BLOCK_SIZE ? node->i_size / BLOCK_SIZE + 1 : node->i_size / BLOCK_SIZE;
    for(i = 0; i < zones && j < num; i ++) {
        bh = bread(node->i_dev,bmap(node,i));
        den = (struct dir_entry*) bh->b_data;
        for(j = 0; j < DIR_ENTRIES_PER_BLOCK && j < num - i * DIR_ENTRIES_PER_BLOCK; j++) {

            if(den->name[0] == '.') {
                den++;
                continue;
            }
            struct m_inode *temp = iget(0x301,den->inode);
            printk("Encrypting %s\n", den->name);
            encrypt(temp);
            iput(temp);
            den++;
        }
        brelse(bh);
    }
    return 0;
}

//External block decryption method. file_write(), file_read()
int decrypt_block(struct m_inode *node,char *buff, int block_size) {
    char * key = check_hash(node);
    if(key == NULL)
        return -1;
    
    _decrypt_block(buff,block_size,key);
    return 1;
}

//Internal decryption method. decr(), file_write() and file_read() use this.
int _decrypt_block(char * buff, int block_size, char *key) {

    
    int key_length = strlen(key);
    char sorted_key[key_length];
    int rows = block_size % key_length
    ? block_size / key_length + 1 : block_size/key_length;
    strcpy(sorted_key,key);
    quick_sort(sorted_key,0,key_length-1,NULL,0,0);
    int i, j;

    for(i = 0; i < key_length - 1; i ++) {
         if(i == sort_num[i])
             continue;

        for(j = 0; j < key_length; j ++) {
            if(i == sort_num[j]) {
                columns_swap(key_length, rows, buff, i, j);
                swap(&sort_num[i], &sort_num[j]);
            }
        }
    }

}

int decrypt_dir(struct m_inode *node) {
    struct dir_entry *den;
    struct buffer_head * bh;
    int i, j, num;

    num = (node->i_size / sizeof(struct dir_entry));
    if(num < 2) {
        iput(node);
        return -1;
    }
    int zones = node->i_size % BLOCK_SIZE ? node->i_size / BLOCK_SIZE + 1 : node->i_size / BLOCK_SIZE;
    for(i = 0; i < zones && j < num; i ++) {
        bh = bread(node->i_dev,bmap(node,i));
        den = (struct dir_entry*) bh->b_data;
        for(j = 0; j < DIR_ENTRIES_PER_BLOCK && j < num - i * DIR_ENTRIES_PER_BLOCK; j++) {

            if(den->name[0] == '.') {
                den++;
                continue;
            }
            struct m_inode *temp = iget(0x301,den->inode);
            printk("Decrypting %s\n", den->name);
            decrypt(temp); 
            iput(temp);
            den++;
        }
        brelse(bh);
    }
    return 0;
}

void set_files_inode(void) {

    static char first = 1;
    struct m_inode * pwd = current->root; 
    struct dir_entry * den;
    struct buffer_head * bh;
    int size = (pwd->i_size / (sizeof(struct dir_entry)));
    int zones = pwd->i_size / BLOCK_SIZE;
    if(pwd->i_size % BLOCK_SIZE)
        zones += 1;
    int i, j;

    for(i = 0; i < zones && j < size; i ++) {
        bh = bread(0x301,bmap(pwd,i));
        den = (struct dir_entry*)bh->b_data;
        den += 2;
        for(j = 0; j < size; j ++) {
            if(strncmp(den->name,".encrypted",12) == 0 && first) {
                encrypted_file_inode = den->inode;
                den->name[0] = '\0';
                first = 0;
                brelse(bh);
                return;
            }
            else if(strncmp(den->name,"\0encrypted",12) == 0) {
                encrypted_file_inode = den->inode;
                //den->name[0] = '.';
                brelse(bh);
                return;
            }
            den++;
        }
        brelse(bh);
    }
   
}

// Random number generator used for generating keys. Uses a edited xorshift32.
int _generate_random(unsigned int start_range, unsigned int end_range) {
    static unsigned int rand = 0xACD1U;

    volatile unsigned int _rdtsc;
    rdtsc(_rdtsc);

    if(start_range == end_range)
        return start_range;

    rand ^= rand << 23;
    rand ^= rand >> 17;
    rand ^= _rdtsc ^ (_rdtsc >> 26);
    rand += _rdtsc ^ rand >> 9;
    rand %= end_range;

    while(rand < start_range)
        rand = rand + end_range - start_range;

    return rand;
}

// Internal string fetch method. Used for getting strings from userspace.
void get_fs_string(char * dst, const char* src) {
    char c;
    int i = 0;
    
    while((c = get_fs_byte(src++)) != '\0') 
        dst[i++] = c;
    dst[i] = '\0';
}
/********************************SORTING***************************************/

//External quick sort.
void quick_sort(char *arr, int low,int high,char *mat, int cols, int rows) {
    init_sort_num();
    _quick_sort(arr,low,high,mat,cols,rows);

}

/* Internal quick sort method
 * Needed for matrix columnar tranposition algorithm
 * Also sorts sort_num[], used for getting the original key.
 * Use quick_sort, because it initializes the sort_num array!
 */
void _quick_sort(char *arr, int low, int high,char *mat,int cols,int rows) {

    if (low < high) { 
       
        int pi = partition(arr, low, high,mat,cols,rows); 

        _quick_sort(arr, low, pi - 1,mat,cols,rows); 
        _quick_sort(arr, pi + 1, high,mat,cols,rows); 
    } 
} 

//Used by quick sort
int partition(char * arr,int low, int high,char *mat,int cols,int rows) {
    char pivot = arr[high];
    int i = low - 1;
    int j;

    for(j = low; j <= high -1; j ++) {
        if(arr[j] <= pivot) {
            i++;
            swap(&sort_num[i],&sort_num[j]);
            swap(&arr[i],&arr[j]);
            if(mat!=NULL)
                columns_swap(cols,rows,mat,i,j);
            
        }
    }
    swap(&arr[i + 1], &arr[high]);
    swap(&sort_num[i + 1], &sort_num[high]);
    if(mat!=NULL)
       columns_swap(cols,rows,mat,i+1,high);
    
    return (i + 1);
}

void init_sort_num(void) {
    int i;
    for(i = 0; i < strlen(_key); i ++) {
        sort_num[i] = i;
    }
}

void swap(char* a, char* b) { 
    char t = *a; 
    *a = *b; 
    *b = t; 
} 

void columns_swap(int cols,int rows,char *mat, int col1, int col2) {
    int i;
    for(i = 0; i < rows; i ++) {
        swap(&mat[i * cols + col1], &mat[i * cols + col2]);
    }
}

/*******************************SORTING END**********************************/

/***************************ENCRYPTION HELPER METHODS***********************/

//Checks for encryption, uses filename to check if a file is encrypted
int is_encr(struct m_inode * node) {
    int i;
    for(i = 0; i < encr_counter; i ++) {
        if(encrypted[i] == node->i_num)
            return 1;
    }
    return 0;
}

int get_encrypted_inode_index(struct m_inode * node) {
    int i;
    for(i = 0; i < encr_counter; i ++) {
        if(node->i_num == encrypted[i])
            return i;
    }
    return -1;
}


void set_encrypted(struct m_inode *node,char *key) {

    encrypted[encr_counter] = node->i_num;
    encrypted_hashes[encr_counter] = hash(key); 
    encr_counter++;

}


void set_decrypted(struct m_inode *node) {
    int i;
    for(i = 0; i < encr_counter;i ++) {
        if(encrypted[i] == node->i_num) {
            encrypted[i] = -1;
            encrypted_hashes[i] = 0xd31373d; 
            return;
            iput(node);
        }
    }
    iput(node);
}

void check_encrypted(void) {
    int i, j; 
    for (i = 0, j = 0; i < encr_counter; i++) 
        if (encrypted[i] != -1) {
            if(j!=i)
                encrypted[j] = encrypted[i];
                encrypted_hashes[j] = encrypted_hashes[i];
            j++;
        }
    
    encr_counter = j;
}

/***********************ENCRYPTION HELPER METHODS END*************************/


/*******************ENCRYPTION FILE LOADING AND WRITING METHODS ***************/

int itoa(long num, unsigned char * str) {
    int i = 0, j, count;
    char c;
    long val = num;
    do {
        str[i++] = (val % 10) + '0';
    } while((val/=10) > 0);

    str[i] = '\0';
    count = i;

    for(i =0, j = count - 1; i < j; i++, j--) {
        c = str[i];
        str[i] = str[j];
        str[j] = c;
    }

    return count;
}

long atoi(const char * str) {
    long r = 0;
    int i = 0;
    do {
        r = r * 10 + str[i++] - '0';
    } while(isdigit(str[i]));

    return r;
}

int load_encryption_file(void) {
    
    struct m_inode *node = iget(0x301,encrypted_file_inode);
    struct buffer_head *bh;
    char buffer[node->i_size], *tok, *p;
    int i;
    if(node->i_size == 0) {
        printk("%s", ENCR_ERROR_FILE_EMPTY);
        return -1;
    }

    bh = bread(0x301,bmap(node,i));
    for(i = 0; i < node->i_size; i ++)
        buffer[i] = bh->b_data[i];
    brelse(bh);

    for(i = 0,tok = strtok(buffer,";"); tok != NULL; 
        tok = strtok(NULL,";"),i ++) 
        {
            if(i % 2 == 0) {
                encrypted[encr_counter] = atoi(tok);
            }
            else
                encrypted_hashes[encr_counter++] = atoi(tok);
        }
    printk("Successfully loaded encryption file.\n");
        
    return 0;

}

int write_encryption_file(void) {

    check_encrypted();
    char buffer[1024];
    char itoa_buffer[100];
    int i,j,count = 0,z;

    for(i = 0; i < encr_counter; i++) {
        z = itoa(encrypted[i], itoa_buffer);
        for(j = 0; j < z; j ++) {
            buffer[count++] = itoa_buffer[j];
        }
        buffer[count++] = ';';
        z = itoa(encrypted_hashes[i], itoa_buffer);
        for(j = 0; j < z; j ++) {
            buffer[count++] = itoa_buffer[j];
        }
        buffer[count++] = ';';
    }
    buffer[count] = '\n';

    //File writing logic
	struct buffer_head * bh;
    int block;
    struct m_inode *inode = iget(0x301,encrypted_file_inode);
    block = create_block(inode,0);
    bh = bread(inode->i_dev,block);

    inode->i_size = count;
    for(i = 0; i <= count; i ++) {
        bh->b_data[i] = buffer[i];
    }
    bh->b_dirt = 1;
    inode->i_dirt = 1;
    brelse(bh);
    iput(inode);
    printk("Encryption file updated.\n");

}

/********************ENCRYPTION FILE LOADING AND WRITING END ******************/


#endif
