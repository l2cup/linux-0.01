/*
 *	console.c
 *
 * This module implements the console io functions
 *	'void con_init(void)'
 *	'void con_write(struct tty_queue * queue)'
 * Hopefully this will be a rather complete VT102 implementation.
 *
 */

/*
 *  NOTE!!! We sometimes disable and enable interrupts for a short while
 * (to put a word in video IO), but this will work even for keyboard
 * interrupts. We know interrupts aren't enabled when getting a keyboard
 * interrupt, as we use trap-gates. Hopefully all is well.
 */

#include <linux/sched.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <asm/system.h>
#include <string.h>
#include <termios.h>
#include <linux/fs.h>
#include <sys/stat.h>

#define SCREEN_START 0xb8000
#define SCREEN_END   0xc0000
#define LINES 25
#define COLUMNS 80
#define NPAR 16

#define FM_START 57
#define FM_WIDTH 22
#define CL_SELECTED_COLOR 0x87

#define wchar(x,atb)__asm__("movb " #atb ",%%ah\n\t" \
		"movw %%ax,%1\n\t" \
		::"a" (x),"m" (*(short *)pos) \
		/*:"ax"*/);\

extern void keyboard_interrupt(void);

static unsigned long origin=SCREEN_START;
static unsigned long scr_end=SCREEN_START+LINES*COLUMNS*2;
static unsigned long pos;
static unsigned long x,y;
static unsigned long top=0,bottom=LINES;
static unsigned long lines=LINES,columns=COLUMNS;
static unsigned long state=0;
static unsigned long npar,par[NPAR];
static unsigned long ques=0;
static unsigned char attr=0x07;
static volatile unsigned char attr_dl = 0x02;
static volatile unsigned char sh_pound=0x02;
static char fm_flag = 0;

//Clib
static char clip_counter = 0;
static char clipboard[10][20];
static char cl_edit_flag = 0;

static char char_counter = 0;

//FM

static char fm_header[20];
//static char back_header[20];
static short back = 1;
static short inodes[10];
static char fm_names[10][15];
static unsigned char fm_colors[10];
static short root = 1;
static char fm_maximum;
static char fm_counter = 0;


/*
 * this is what the terminal answers to a ESC-Z or csi0c
 * query (= vt100 response).
 */
#define RESPONSE "\033[?1;2c"

static inline void gotoxy(unsigned int new_x,unsigned int new_y)
{
	if (new_x>=columns || new_y>=lines)
		return;
	x=new_x;
	y=new_y;
	pos=origin+((y*columns+x)<<1);
}

static inline void set_origin(void)
{
	cli();
	outb_p(12,0x3d4);
	outb_p(0xff&((origin-SCREEN_START)>>9),0x3d5);
	outb_p(13,0x3d4);
	outb_p(0xff&((origin-SCREEN_START)>>1),0x3d5);
	sti();
}

static void scrup(void)
{
	if (!top && bottom==lines) {
		origin += columns<<1;
		pos += columns<<1;
		scr_end += columns<<1;
		if (scr_end>SCREEN_END) {
			
			int d0,d1,d2,d3;
			__asm__ __volatile("cld\n\t"
				"rep\n\t"
				"movsl\n\t"
				"movl %[columns],%1\n\t"
				"rep\n\t"
				"stosw"
				:"=&a" (d0), "=&c" (d1), "=&D" (d2), "=&S" (d3)
				:"0" (0x0720),
				 "1" ((lines-1)*columns>>1),
				 "2" (SCREEN_START),
				 "3" (origin),
				 [columns] "r" (columns)
				:"memory");

			scr_end -= origin-SCREEN_START;
			pos -= origin-SCREEN_START;
			origin = SCREEN_START;
		} else {
			int d0,d1,d2;
			__asm__ __volatile("cld\n\t"
				"rep\n\t"
				"stosl"
				:"=&a" (d0), "=&c" (d1), "=&D" (d2) 
				:"0" (0x07200720),
				"1" (columns>>1),
				"2" (scr_end-(columns<<1))
				:"memory");
		}
		set_origin();
	} else {
		int d0,d1,d2,d3;
		__asm__ __volatile__("cld\n\t"
			"rep\n\t"
			"movsl\n\t"
			"movl %[columns],%%ecx\n\t"
			"rep\n\t"
			"stosw"
			:"=&a" (d0), "=&c" (d1), "=&D" (d2), "=&S" (d3)
			:"0" (0x0720),
			"1" ((bottom-top-1)*columns>>1),
			"2" (origin+(columns<<1)*top),
			"3" (origin+(columns<<1)*(top+1)),
			[columns] "r" (columns)
			:"memory");
	}
	if(fm_flag > 0) {
		clean_frame();
		draw_frame();
		if(fm_flag == 2)
			draw_clipboard();
		else if(fm_flag == 1)
			draw_fm(root);
	}
}

static void scrdown(void)
{
	int d0,d1,d2,d3;
	__asm__ __volatile__("std\n\t"
		"rep\n\t"
		"movsl\n\t"
		"addl $2,%%edi\n\t"	/* %edi has been decremented by 4 */
		"movl %[columns],%%ecx\n\t"
		"rep\n\t"
		"stosw"
		:"=&a" (d0), "=&c" (d1), "=&D" (d2), "=&S" (d3)
		:"0" (0x0720),
		"1" ((bottom-top-1)*columns>>1),
		"2" (origin+(columns<<1)*bottom-4),
		"3" (origin+(columns<<1)*(bottom-1)-4),
		[columns] "r" (columns)
		:"memory");
}

static void lf(void)
{
	if (y+1<bottom) {
		y++;
		pos += columns<<1;
		return;
	}
	scrup();
}

static void ri(void)
{
	if (y>top) {
		y--;
		pos -= columns<<1;
		return;
	}
	scrdown();
}

static void cr(void)
{
	pos -= x<<1;
	x=0;
}

static void del(void)
{
	if (x) {
		pos -= 2;
		x--;
		*(unsigned short *)pos = 0x0720;
	}
}

static void csi_J(int par)
{
	long count;
	long start;

	switch (par) {
		case 0:	/* erase from cursor to end of display */
			count = (scr_end-pos)>>1;
			start = pos;
			break;
		case 1:	/* erase from start to cursor */
			count = (pos-origin)>>1;
			start = origin;
			break;
		case 2: /* erase whole display */
			count = columns*lines;
			start = origin;
			break;
		default:
			return;
	}
	int d0,d1,d2;
	__asm__ __volatile__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		:"=&c" (d0), "=&D" (d1), "=&a" (d2)
		:"0" (count),"1" (start),"2" (0x0720)
		:"memory");
}

static void csi_K(int par)
{
	long count;
	long start;

	switch (par) {
		case 0:	/* erase from cursor to end of line */
			if (x>=columns)
				return;
			count = columns-x;
			start = pos;
			break;
		case 1:	/* erase from start of line to cursor */
			start = pos - (x<<1);
			count = (x<columns)?x:columns;
			break;
		case 2: /* erase whole line */
			start = pos - (x<<1);
			count = columns;
			break;
		default:
			return;
	}
	int d0,d1,d2;
	__asm__ __volatile__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		:"=&c" (d0), "=&D" (d1), "=&a" (d2)
		:"0" (count),"1" (start),"2" (0x0720)
		:"memory");
}

void csi_m(void)
{
	int i;

	for (i=0;i<=npar;i++)
		switch (par[i]) {
			case 0:attr=0x07;break;
			case 1:attr=0x0f;break;
			case 4:attr=0x0f;break;
			case 7:attr=0x70;break;
			case 27:attr=0x07;break;
		}
}

static inline void set_cursor(void)
{
	cli();
	outb_p(14,0x3d4);
	outb_p(0xff&((pos-SCREEN_START)>>9),0x3d5);
	outb_p(15,0x3d4);
	outb_p(0xff&((pos-SCREEN_START)>>1),0x3d5);
	sti();
}

static void respond(struct tty_struct * tty)
{
	char * p = RESPONSE;

	cli();
	while (*p) {
		PUTCH(*p,tty->read_q);
		p++;
	}
	sti();
	copy_to_cooked(tty);
}

static void insert_char(void)
{
	int i=x;
	unsigned short tmp,old=0x0720;
	unsigned short * p = (unsigned short *) pos;

	while (i++<columns) {
		tmp=*p;
		*p=old;
		old=tmp;
		p++;
	}
}

static void insert_line(void)
{
	int oldtop,oldbottom;

	oldtop=top;
	oldbottom=bottom;
	top=y;
	bottom=lines;
	scrdown();
	top=oldtop;
	bottom=oldbottom;
}

static void delete_char(void)
{
	int i;
	unsigned short * p = (unsigned short *) pos;

	if (x>=columns)
		return;
	i = x;
	while (++i < columns) {
		*p = *(p+1);
		p++;
	}
	*p=0x0720;
}

static void delete_line(void)
{
	int oldtop,oldbottom;

	oldtop=top;
	oldbottom=bottom;
	top=y;
	bottom=lines;
	scrup();
	top=oldtop;
	bottom=oldbottom;
}

static void csi_at(int nr)
{
	if (nr>columns)
		nr=columns;
	else if (!nr)
		nr=1;
	while (nr--)
		insert_char();
}

static void csi_L(int nr)
{
	if (nr>lines)
		nr=lines;
	else if (!nr)
		nr=1;
	while (nr--)
		insert_line();
}

static void csi_P(int nr)
{
	if (nr>columns)
		nr=columns;
	else if (!nr)
		nr=1;
	while (nr--)
		delete_char();
}

static void csi_M(int nr)
{
	if (nr>lines)
		nr=lines;
	else if (!nr)
		nr=1;
	while (nr--)
		delete_line();
}

static int saved_x=0;
static int saved_y=0;

static void save_cur(void)
{
	saved_x=x;
	saved_y=y;
}

static void restore_cur(void)
{
	x=saved_x;
	y=saved_y;
	pos=origin+((y*columns+x)<<1);
}
void con_write(struct tty_struct * tty)
{
	int nr;
	char c;

	nr = CHARS(tty->write_q);
	while (nr--) {
		GETCH(tty->write_q,c);
		switch(state) {
			case 0:
				if (c>31 && c<127) {
					if (x>=columns) {
						x -= columns;
						pos -= columns<<1;
						lf();
					}
					if(fm_flag == 2 && cl_edit_flag) {	
						int len = strlen(clipboard[clip_counter]);
						clipboard[clip_counter][len] = c;
						clipboard[clip_counter][len+1] = '\0';
						draw_line(clipboard[clip_counter],clip_counter+1, CL_SELECTED_COLOR);
						if(len % 2 == 0) {
							pos+= 2;
							x++;
						}
						char_counter++;
					}
					else {
						if(c == '#') {
							__asm__("movb sh_pound, %%ah\n\t"
									"movw %%ax, %1\n\t"
									::"a" (c),"m"(*(short *)pos));
						}
						else {
						__asm__("movb attr,%%ah\n\t"
							"movw %%ax,%1\n\t"
							::"a" (c),"m" (*(short *)pos)
							/*:"ax"*/);
						}
						pos += 2;
						x++;
					}
				} else if (c==27)
					state=1;
				else if (c==10 || c==11 || c==12)
					lf();
				else if (c==13)
					cr();
				else if (c==ERASE_CHAR(tty))
					 del();
				else if (c==8) {
					 if (x) {
					 	x--;
					 	pos -= 2;
					 }
						
				} else if (c==9) {
					c=8-(x&7);
					x += c;
					pos += c<<1;
					if (x>columns) {
						x -= columns;
						pos -= columns<<1;
						lf();
					}
					c=9;
				}
				break;
			case 1:
				state=0;
				if (c=='[')
					state=2;
				else if (c=='E')
					gotoxy(0,y+1);
				else if (c=='M')
					ri();
				else if (c=='D')
					lf();
				else if (c=='Z')
					respond(tty);
				else if (x=='7')
					save_cur();
				else if (x=='8')
					restore_cur();
				break;
			case 2:
				for(npar=0;npar<NPAR;npar++)
					par[npar]=0;
				npar=0;
				state=3;
				if ((ques=(c=='?')))
					break;
			case 3:
				if (c==';' && npar<NPAR-1) {
					npar++;
					break;
				} else if (c>='0' && c<='9') {
					par[npar]=10*par[npar]+c-'0';
					break;
				} else state=4;
			case 4:
				state=0;
				switch(c) {
					case 'G': case '`':
						if (par[0]) par[0]--;
						gotoxy(par[0],y);
						break;
					case 'A':
						if (!par[0]) par[0]++;
						gotoxy(x,y-par[0]);
						break;
					case 'B': case 'e':
						if (!par[0]) par[0]++;
						gotoxy(x,y+par[0]);
						break;
					case 'C': case 'a':
						if (!par[0]) par[0]++;
						gotoxy(x+par[0],y);
						break;
					case 'D':
						if (!par[0]) par[0]++;
						gotoxy(x-par[0],y);
						break;
					case 'E':
						if (!par[0]) par[0]++;
						gotoxy(0,y+par[0]);
						break;
					case 'F':
						if (!par[0]) par[0]++;
						gotoxy(0,y-par[0]);
						break;
					case 'd':
						if (par[0]) par[0]--;
						gotoxy(x,par[0]);
						break;
					case 'H': case 'f':
						if (par[0]) par[0]--;
						if (par[1]) par[1]--;
						gotoxy(par[1],par[0]);
						break;
					case 'J':
						csi_J(par[0]);
						break;
					case 'K':
						csi_K(par[0]);
						break;
					case 'L':
						csi_L(par[0]);
						break;
					case 'M':
						csi_M(par[0]);
						break;
					case 'P':
						csi_P(par[0]);
						break;
					case '@':
						csi_at(par[0]);
						break;
					case 'm':
						csi_m();
						break;
					case 'r':
						if (par[0]) par[0]--;
						if (!par[1]) par[1]=lines;
						if (par[0] < par[1] &&
						    par[1] <= lines) {
							top=par[0];
							bottom=par[1];
						}
						break;
					case 's':
						save_cur();
						break;
					case 'u':
						restore_cur();
						break;
				}
		}
	}
	set_cursor();
}

/*
 *  void con_init(void);
 *
 * This routine initalizes console interrupts, and does nothing
 * else. If you want the screen to clear, call tty_write with
 * the appropriate escape-sequece.
 */
void con_init(void)
{
	register unsigned char a;

	gotoxy(*(unsigned char *)(0x90000+510),*(unsigned char *)(0x90000+511));
	set_trap_gate(0x21,&keyboard_interrupt);
	outb_p(inb_p(0x21)&0xfd,0x21);
	a=inb_p(0x61);
	outb_p(a|0x80,0x61);
	outb(a,0x61);
}

/*
 * This part contains code for the file manager.
 * Someday i may put it in a seperate file for now it will stay here
 * 
 */

//This draws the frame for the file manager
void clean_frame() {
	int i, j;
	save_cur();

	for(i = 0; i < 12; i ++) { 
		for (j = FM_START; j < COLUMNS; j ++) {	
				gotoxy(j,i);
				wchar(' ',attr);
		}
	}
	restore_cur();	
}

void gotoclixy() {
	int border = ((FM_WIDTH - strlen(clipboard[clip_counter]) + 1) / 2);
	gotoxy(FM_START + border + strlen(clipboard[clip_counter]), //x
										 clip_counter + 1); //y
	set_cursor();
}

void backspace(void) {

		int len = strlen(clipboard[clip_counter]);

		if(len != 0) {
			clipboard[clip_counter][len - 1] = '\0';
			if(len % 2 == 1){
				x--;
				pos-=2;
			}
			set_cursor();
			draw_line(clipboard[clip_counter],clip_counter + 1,CL_SELECTED_COLOR);
		}
	
}

void toggle_ef(void) {
	if(cl_edit_flag == 0)
		efu();
	else
		efd();	
}


static unsigned long x_con, y_con;

void efu(void) {
	if(fm_flag != 2)
		return;
	x_con = x;
	y_con = y;
	cl_edit_flag = 1;
	draw_line(clipboard[clip_counter],clip_counter + 1, CL_SELECTED_COLOR);
	gotoclixy();
}

void efd(void) {
	if(fm_flag != 2)
		return;
	draw_line(clipboard[clip_counter],clip_counter + 1, CL_SELECTED_COLOR);
	cl_edit_flag = 0;
	gotoxy(x_con + char_counter,y_con);
	set_cursor();
	clear_buffer();
}

void clear_buffer(void) {

	for(char_counter; char_counter > 0 ;char_counter --) 
		PUTCH(127, tty_table[0].read_q);
		
	copy_to_cooked(&tty_table[0]);
}

void draw_clipboard(void) {
	int i;
	draw_header("[ Clipboard ]");
	for(i = 0; i < 10; i ++) {
		if(i == clip_counter){
			cl_edit_flag = 1;
			draw_line(clipboard[clip_counter],clip_counter + 1,CL_SELECTED_COLOR);
			cl_edit_flag = 0;
		}
		else {
			draw_line(clipboard[i],i + 1,CL_SELECTED_COLOR);
		}
		
	}
		
}



void draw_line(char * buf, unsigned long y, unsigned char text_color) {
	attr_dl = text_color;
	if(y <= 0 || y > 10)
		return;
	save_cur();
	int i, j, border, saved_x, saved_y;
	border = ((FM_WIDTH - strlen(buf) + 1) / 2) + 1;
	saved_x = x;
	saved_y = y;
	for(i = FM_START + 1, j = 0;i < COLUMNS - 1; i++) {
		if(i - FM_START + 1 == border + j) {
			gotoxy(i, y);
			if((fm_flag == 1 || cl_edit_flag == 1)) {
				wchar(buf[j],attr_dl);
			}
			else {
				wchar(buf[j],attr);
			}
			if(j < strlen(buf) - 1)
				j++;
		}
		else if(fm_flag == 2 && cl_edit_flag == 1){
			gotoxy(i, y);
			wchar(' ',attr_dl);
		}
		else {
			gotoxy(i, y);
			wchar(' ', attr);
		}
	}
	restore_cur();
}

void arr_up(void) {
	if(fm_flag == 0)
		return;
	else if(fm_flag == 1) {
		draw_line(fm_names[fm_counter],fm_counter + 1,fm_colors[fm_counter]);
		fm_counter--;
		if(fm_counter < 0)
			fm_counter = fm_maximum - 1;
		
		draw_line(fm_names[fm_counter],fm_counter + 1, fm_colors[fm_counter] +0x80);
	}
	else {
		draw_line(clipboard[clip_counter],clip_counter + 1, CL_SELECTED_COLOR);
		clip_counter--;
		if(clip_counter < 0)
			clip_counter = 9;
		
		cl_edit_flag = 1;
		draw_line(clipboard[clip_counter],clip_counter + 1, CL_SELECTED_COLOR);
		cl_edit_flag = 0;
	}
}

void space_pressed() {
	int i;
	if(fm_flag == 0)
		return;
	else if(fm_flag == 1){
		for(i = 0; i < strlen(fm_header); i ++ ) {
			PUTCH(fm_header[i],tty_table[0].read_q);
		}
		//PUTCH('/',tty_table[0].read_q);
		for(i = 0; i < strlen(fm_names[fm_counter]); i ++) {
			PUTCH(fm_names[fm_counter][i],tty_table[0].read_q);
		}
		copy_to_cooked(&tty_table[0]);
	}
	else {
		for(i = 0; i < strlen(clipboard[clip_counter]); i++) {
			PUTCH(clipboard[clip_counter][i],tty_table[0].read_q);
		}
		copy_to_cooked(&tty_table[0]);
	}
}


void arr_down(void) {
	if(fm_flag == 0)
		return;
	else if(fm_flag == 1) {
		draw_line(fm_names[fm_counter],fm_counter + 1,fm_colors[fm_counter]);
		fm_counter = (fm_counter + 1) % fm_maximum;
		if(fm_counter < 0)
			fm_counter = fm_maximum;
		
		draw_line(fm_names[fm_counter],fm_counter + 1, fm_colors[fm_counter] +0x80);
	}
	else {
		draw_line(clipboard[clip_counter],clip_counter + 1, CL_SELECTED_COLOR);
		clip_counter = (clip_counter + 1) % 10;
		cl_edit_flag = 1;
		draw_line(clipboard[clip_counter],clip_counter + 1,CL_SELECTED_COLOR);
		cl_edit_flag = 0;
	}
}



void draw_header(char * buf){
	int i, j, 
		border = (FM_WIDTH - strlen(buf) + 1) / 2;
	save_cur();
	for(i = FM_START, j = 0; j < strlen(buf); i++) {
		if(i - FM_START == border + j) {
			gotoxy(i, 0);
			wchar(buf[j],attr);
			j++;
		}
	}
	restore_cur();	
}


void draw_frame(void) {	
	int i, j;
	save_cur();
	for(i = 0; i < 12; i ++) { 
		for (j = FM_START; j < COLUMNS; j ++) {
			if(i == 0 || i == 11) {
				gotoxy(j,i);
				wchar('#',attr);
			}
			else{
				if(j == FM_START || j == COLUMNS - 1) {
					gotoxy(j,i);
					wchar('#',attr);
				}
			}
		}
	}
	restore_cur();
}


void _go_right(short new_root) {
	short temp = root;
	root = new_root;
	back = root;
}

void _go_left(void) {
	short temp = back;
	root = back;
	struct m_inode *node = iget(0x301,root);
	struct dir_entry * den;
	struct buffer_head * buff_head = bread(0x301,node->i_zone[0]);
	den = (struct dir_entry *) buff_head->b_data;
	den++;
	back = den->inode;
	iput(node);
}


void arr_left(void) {
	int i;
	for(i = strlen(fm_header) - 1; i > 0; i--) {
		if(fm_header[i] == '\\') {
			fm_header[i] = '\0';
			return;
		}
		fm_header[i] = '\0';
	}
	clean_frame();
	draw_frame();
	draw_header(fm_header);
	draw_fm(back);
}

void arr_right(void) {
	if(fm_colors[fm_counter] != 0x0B)
		return;

	//struct m_inode *next = iget(0x301,inodes[fm_counter]);
	strcat(fm_header,fm_names[fm_counter]);
	strcat(fm_header,"/");
	clean_frame();
	draw_frame();
	draw_fm(inodes[fm_counter]);



}

void draw_fm(short _root) {

	draw_header(fm_header);
	struct m_inode * node = iget(0x301,_root);
	int num, i, j;
	struct dir_entry * den;
	struct buffer_head *buff_head;

	num = (node->i_size / (sizeof(struct dir_entry)));
	if(!(num - 2)) {
		iput(node);
		return;
	}

	fm_maximum = 0;
	fm_counter = 0;	
	buff_head = bread(0x301,node->i_zone[0]);
	den = (struct dir_entry *) buff_head->b_data;
	
	den++;
	if(!strcmp(den->name,".."))
		back = den->inode;
	root = _root;
	
	for(i = 0; i < 10 && i < num - 1; i ++) {

		if(den->name[0] == '.') {
			den++;
			continue;
		}

		struct m_inode *temp_node = iget(0x301,den->inode);


		set_fm_color(temp_node->i_mode);
		
		for(j = 0; j < 14; j++) {
			if(den->name[j] > 32 && den->name[j] < 127)
				continue;
			else {
				den->name[j] = '\0';
				break;
			}
			
		}

		memset(&fm_names[fm_counter][0],0,sizeof(fm_names[fm_counter]));
		strcpy(fm_names[fm_counter],den->name);
		
		inodes[fm_counter] = den->inode;

		draw_line(fm_names[fm_counter], fm_counter + 1, fm_colors[fm_counter]);

		den++;	
		fm_counter ++;
		fm_maximum ++;
		iput(temp_node);
	}

	iput(node);
	brelse(buff_head);
	fm_counter = 0;
	draw_line(fm_names[fm_counter],fm_counter +1, fm_colors[fm_counter] + 0x80);

}

void set_fm_color(unsigned short i_mode) {

	if(S_ISDIR(i_mode)) {
		fm_colors[fm_counter] = 0x0B;
	}
	else if(S_ISCHR(i_mode)) {
		fm_colors[fm_counter] = 0x06;
	}
	else if(S_IXUSR & i_mode) {
		fm_colors[fm_counter] = 0x0A;
	}
	else if(S_ISREG(i_mode)) {
		fm_colors[fm_counter] = 0x07;
	}
	
}

void fm_toggle(void) {
	fm_flag = (fm_flag + 1) % 3;
	clean_frame();
	if(fm_flag == 0)
		root = 1;

	if(fm_flag != 0)
		draw_frame();
	if(fm_flag == 1) {
		if(root == 1) {
		fm_header[0] = '/';
		fm_header[1] = '\0';
		}
		draw_fm(root);
	}
	else if(fm_flag == 2)
		draw_clipboard();
}



int sys_clear(void) {
	
	int i;
	for(i = 0; i < LINES; i++)
	{
		scrup();
	}
	gotoxy(0,0);
	return 0;
}

int sys_echo_off(void) {
	tty_table[0].termios.c_lflag &= ~ECHO;
}

int sys_echo_on(void) {
	tty_table[0].termios.c_lflag |= ECHO;
}