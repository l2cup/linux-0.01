#include <errno.h>
#include <fcntl.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <encryption.h>
#include <string.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

int file_read(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	int left,chars,nr;
	struct buffer_head * bh;

	if ((left=count)<=0)
		return 0;
	while (left) {
		if ((nr = bmap(inode,(filp->f_pos)/BLOCK_SIZE))) {
			if (!(bh=bread(inode->i_dev,nr)))
				break;
		} else
			bh = NULL;
		nr = filp->f_pos % BLOCK_SIZE;
		chars = MIN( BLOCK_SIZE-nr , left );
		filp->f_pos += chars;
		left -= chars;
		if (bh) {
			if(global_key_flag && is_encr(inode)) {
				char holder[chars];
				strncpy(holder, nr + bh->b_data, chars);
				if(decrypt_block(inode,holder,chars) < 0) {
					printk("%s", KEY_ERROR_HASH);
					return -EINVAL;
				}
				char *p = holder;
				while(chars-->0)
					put_fs_byte(*(p++), buf++);
			}
			else {
				char * p = nr + bh->b_data;
				while (chars-->0)
					put_fs_byte(*(p++),buf++);
			}
			brelse(bh);
		} else {
			while (chars-->0)
				put_fs_byte(0,buf++);
		}
	}
	inode->i_atime = CURRENT_TIME;
	return (count-left)?(count-left):-ERROR;
}

int file_write(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	
	off_t pos;
	int block,c,encr_size;
	struct buffer_head * bh;
	char * p,encr_flag = 0;
	int i=0;

/*
 * ok, append may not work when many processes are writing at the same time
 * but so what. That way leads to madness anyway.
 */	
	if(global_key_flag && is_encr(inode)) 
		encr_flag = 1;
	if (filp->f_flags & O_APPEND)
		pos = inode->i_size;
	else
		pos = filp->f_pos;
	while (i<count) {
		if (!(block = create_block(inode,pos/BLOCK_SIZE)))
			break;
		if (!(bh=bread(inode->i_dev,block)))
			break;
		c = pos % BLOCK_SIZE;
		encr_size = pos % BLOCK_SIZE ? pos % BLOCK_SIZE : BLOCK_SIZE;
		printk("Encr size 1 %d\n",encr_size);
		char encr_buf[encr_size];
		// if(encr_flag) {	
		// 	strncpy(encr_buf,bh->b_data,encr_size);
		// 	_decrypt(encr_buf,encr_size,1);
		// }

		p = encr_flag ? encr_buf + c : c + bh->b_data;
		printk("encr flag \n",encr_flag);
		bh->b_dirt = 1;
		c = BLOCK_SIZE-c;
		if (c > count-i) c = count-i;
		pos += c;
		if (pos > inode->i_size) {
			inode->i_size = pos;
			inode->i_dirt = 1;
		}
		i += c;
		encr_size += c;
		while (c-->0)
			*(p++) = get_fs_byte(buf++);

		printk("encr size 2 %d\n", encr_size);
		// if(encr_flag) {
		// 	int rows = encr_size % strlen(_key) ? (encr_size / strlen(_key)) + 1 : encr_size / strlen(_key);

		// 	char sorted[strlen(_key)];
		// 	strncpy(sorted,_key,strlen(_key));
		// 	quick_sort(sorted,0,strlen(_key) - 1, encr_buf, strlen(_key), rows);

		// 	int z = 0;
		// 	for(z; z < encr_size; z ++) {
		// 		bh->b_data[z] = encr_buf[z];
		// 	}

		// }
		
		brelse(bh);
	}
	inode->i_mtime = CURRENT_TIME;
	if (!(filp->f_flags & O_APPEND)) {
		filp->f_pos = pos;
		inode->i_ctime = CURRENT_TIME;
	}
	return (i?i:-1);
}
