/*
 *  linux/kernel/console.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *	console.c
 *
 * This module implements the console io functions
 *	'void con_init(void)'
 *	'void con_write(struct tty_queue * queue)'
 * Hopefully this will be a rather complete VT102 implementation.
 *
 * Beeping thanks to John T Kohl.
 */
/*
 * ��ģ��ʵ�ֿ���̨�����������
 * ��void con_init(void)��
 * 'void con_write(struct tty��queue ľ queue)��
 *ϣ������һ���ǳ�������VT102ʵ�֡�
 */
/*
 *  NOTE!!! We sometimes disable and enable interrupts for a short while
 * (to put a word in video IO), but this will work even for keyboard
 * interrupts. We know interrupts aren't enabled when getting a keyboard
 * interrupt, as we use trap-gates. Hopefully all is well.
 */
/*
* ע��!!! ������ʱ���ݵؽ�ֹ�������ж�(�ڽ�һ����(word)�ŵ���ƵIO)������ʹ
* ���ڼ����ж���Ҳ�ǿ��Թ����ġ���Ϊ����ʹ�������ţ���������֪���ڻ��һ��
* �����ж�ʱ�ж��ǲ�����ġ�ϣ��һ�о�������
*/
/*
 * Code to check for different video-cards mostly by Galen Hunt,
 * <g-hunt@ee.utah.edu>
 */
/*
* ��ⲻͬ��ʾ���Ĵ���������Galen Hunt ��д�ģ�
* <g-hunt@ee.utah.edu>
*/

#include <linux/sched.h>	/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 �����ݣ�	*/
							/* ����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/
#include <linux/tty.h>		/* tty ͷ�ļ����������й�tty_io������ͨ�ŷ���Ĳ�����������	*/
#include <asm/io.h>			/* io ͷ�ļ�������Ӳ���˿�����/���������䡣	*/
#include <asm/system.h>		/* ϵͳͷ�ļ������������û��޸�������/�ж��ŵȵ�Ƕ��ʽ���ꡣ	*/

/*
 * These are set up by the setup-routine at boot-time:
 */
/*
* ��Щ�������ӳ���setup ����������ϵͳʱ���õĲ�����
*/

/* �μ���boot/setup.s ��ע�ͣ���setup �����ȡ�������Ĳ�����	*/
#define ORIG_X (*(unsigned char *)0x90000)								/* ��ʼ����кš�	*/
#define ORIG_Y (*(unsigned char *)0x90001)								/* ��ʼ����кš�	*/
#define ORIG_VIDEO_PAGE (*(unsigned short *)0x90004)					/* ��ʾҳ�档	*/
#define ORIG_VIDEO_MODE ((*(unsigned short *)0x90006) & 0xff)			/* ��ʾģʽ��	*/
#define ORIG_VIDEO_COLS (((*(unsigned short *)0x90006) & 0xff00) >> 8)	/* �ַ�������	*/
#define ORIG_VIDEO_LINES (25)											/* ��ʾ������	*/
#define ORIG_VIDEO_EGA_AX (*(unsigned short *)0x90008)					/* [��]	*/
#define ORIG_VIDEO_EGA_BX (*(unsigned short *)0x9000a)					/* ��ʾ�ڴ��С��ɫ��ģʽ��	*/
#define ORIG_VIDEO_EGA_CX (*(unsigned short *)0x9000c)					/* ��ʾ�����Բ�����	*/

/* ������ʾ����ɫ/��ɫ��ʾģʽ���ͷ��ų�����	*/
#define VIDEO_TYPE_MDA		0x10	/* Monochrome Text Display		��ɫ�ı�		*/
#define VIDEO_TYPE_CGA		0x11	/* CGA Display 					CGA ��ʾ��	*/
#define VIDEO_TYPE_EGAM		0x20	/* EGA/VGA in Monochrome Mode	EGA/VGA ��ɫ	*/
#define VIDEO_TYPE_EGAC		0x21	/* EGA/VGA in Color Mode		EGA/VGA ��ɫ	*/

#define NPAR 16							/* ת���ַ�������������������	*/

extern void keyboard_interrupt (void);	/* �����жϴ������(keyboard.S)��	*/

/* ������Щ��̬�����Ǳ�������ʹ�õ�һЩȫ�ֱ�����	*/
static unsigned char video_type;		/* Type of display being used */	/* ʹ�õ���ʾ���� */
static unsigned long video_num_columns;	/* Number of text columns */		/* ��Ļ�ı����� */
static unsigned long video_size_row;	/* Bytes per row */					/* ÿ��ʹ�õ��ֽ��� */
static unsigned long video_num_lines;	/* Number of test lines */			/* ��Ļ�ı����� */
static unsigned char video_page;		/* Initial video page */			/* ��ʼ��ʾҳ�� */
static unsigned long video_mem_start;	/* Start of video RAM */			/* ��ʾ�ڴ���ʼ��ַ */
static unsigned long video_mem_end;		/* End of video RAM (sort of) */	/* ��ʾ�ڴ����(ĩ��)��ַ */
static unsigned short video_port_reg;	/* Video register select port */	/* ��ʾ���������Ĵ����˿� */
static unsigned short video_port_val;	/* Video register value port */		/* ��ʾ�������ݼĴ����˿� */
static unsigned short video_erase_char;	/* Char+Attrib to erase with */		/* �����ַ��������ַ�(0x0720) */

/* ������Щ����������Ļ������������origin��ʾ�ƶ������ⴰ�����Ͻ�ԭ���ڴ��ַ����	*/
static unsigned long origin;			/* Used for EGA/VGA fast scroll 	 scr_start��	*/
										/* ����EGA/VGA ���ٹ��� 	 ������ʼ�ڴ��ַ��	*/
static unsigned long scr_end;			/* Used for EGA/VGA fast scroll */
										/* ����EGA/VGA ���ٹ��� 	 ����ĩ���ڴ��ַ��	*/
static unsigned long pos;				/* ��ǰ����Ӧ����ʾ�ڴ�λ�á�	*/
static unsigned long x, y;				/* ��ǰ���λ�á�	*/
static unsigned long top, bottom;		/* ����ʱ�����кţ������кš�	*/
										/* state ���ڱ�������ESC ת������ʱ�ĵ�ǰ���衣npar,par[]���ڴ��ESC ���е��м䴦�������	*/
static unsigned long state = 0;			/* ANSI ת���ַ����д���״̬��	*/
static unsigned long npar, par[NPAR];	/* ANSI ת���ַ����в��������Ͳ������顣	*/
static unsigned long ques = 0;
static unsigned char attr = 0x07;		/* �ַ�����(�ڵװ���)��	*/

static void sysbeep (void);	/* ϵͳ����������	*/

/*
 * this is what the terminal answers to a ESC-Z or csi0c
 * query (= vt100 response).
 */
/*
* �������ն˻�ӦESC-Z ��csi0c �����Ӧ��(=vt100 ��Ӧ)��
*/
/* csi -�������������루Control Sequence Introducer����
 * II����ͨ�����Ͳ��������������0���豸���ԣ�DA���������У���ESC [c����ESC [0c�� ]
 * Ҫ���ն�Ӧ��һ���豸���Կ������У�ESC Z�����������ͬ�����ն�����������������Ӧ
 * �����������У�����ESC [?l;2c��]��ʾ�ն��Ǹ߼���Ƶ�նˡ�
*/
#define RESPONSE "\033[?1;2c"

/* NOTE! gotoxy thinks x==video_num_columns is ok */
/* ע�⣡gotoxy ������Ϊx==video_num_columns��������ȷ�� */
/* ���ٹ�굱ǰλ�á�
 * ������new_x - ��������кţ�new_y - ��������кš�
 * ���µ�ǰ���λ�ñ���x,y��������pos ָ��������ʾ�ڴ��еĶ�Ӧλ�á�
 */
static inline void gotoxy(unsigned int new_x,unsigned int new_y)
{
/* ���ȼ���������Ч�ԡ���������Ĺ���кų�����ʾ�����������߹���кŲ�������ʾ��
 * ������������˳�������͸��µ�ǰ���������¹��λ�ö�Ӧ����ʾ�ڴ���λ��pos��
 */
	if (new_x > video_num_columns || new_y >= video_num_lines)
		return;
	x=new_x;
	y=new_y;
	pos=origin + y*video_size_row + (x<<1);	/* 1 ���� 2 ���ֽڱ�ʾ������ x<<1��	*/
}

/* ���ù�����ʼ��ʾ�ڴ��ַ��	*/
static inline void set_origin(void)
{
/* ��������ʾ�Ĵ���ѡ��˿�video_port_reg���12����ѡ����ʾ�������ݼĴ���rl2��Ȼ��
д�������ʼ��ַ���ֽڡ������ƶ�9λ����ʾ�����ƶ�8λ�ٳ���2 (��Ļ��1���ַ���2
�ֽڱ�ʾ)����ѡ����ʾ�������ݼĴ���rl3��Ȼ��д�������ʼ��ַ���ֽڡ������ƶ�1λ
��ʾ����2��ͬ��������Ļ��1���ַ���2�ֽڱ�ʾ�����ֵ�������Ĭ����ʾ�ڴ���ʼλ��
video_mem_start�����ģ�������ڲ�ɫģʽ��viedo_mem_start =�����ڴ��ַ0xb8000��
*/
	cli();
	outb_p(12, video_port_reg);		/* ѡ�����ݼĴ���rl2�����������ʼλ�ø��ֽڡ�	*/
	outb_p(0xff&((origin-video_mem_start)>>9), video_port_val);
	outb_p(13, video_port_reg);		/* ѡ�����ݼĴ���rl3�����������ʼλ�õ��ֽڡ�	*/
	outb_p(0xff&((origin-video_mem_start)>>1), video_port_val);
	sti();
}

/* ���Ͼ�һ�С�
����Ļ�������������������ƶ�һ�У��������򶥳��ֵ���������ӿո��ַ��������������
������2�л�2�����ϡ��μ������б��˵����
*/
static void scrup(void)
{
/* �����ж���ʾ�����͡�����EGA/VGA�������ǿ���ָ�������з�Χ�����򣩽��й���������
��MDA��ɫ��ʾ��ֻ�ܽ������������������ú�����EGA��MDA��ʾ���ͽ��зֱ������
��ʾ������EGA���򻹷�Ϊ���������ƶ��������ڴ����ƶ����������ȴ�����ʾ����EGA/VGA 
��ʾ���͵������
*/
	if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM)
	{
/* ����ƶ���ʼ��top=0���ƶ������bottom = video_num_lines = 25�����ʾ������������
�ƶ������ǰ�������Ļ�������ϽǶ�Ӧ����ʼ�ڴ�λ��origin����Ϊ������һ�ж�Ӧ���ڴ�
λ�ã�ͬʱҲ���ٵ�����ǰ����Ӧ���ڴ�λ���Լ���Ļĩ��ĩ���ַ�ָ��scr_end��λ�á�
��������Ļ���������ڴ���ʼλ��ֵoriginд����ʾ�������С�	*/
		if (!top && bottom == video_num_lines)
		{
			origin += video_size_row;
			pos += video_size_row;
			scr_end += video_size_row;
/* �����Ļ����ĩ������Ӧ����ʾ�ڴ�ָ��scr��end������ʵ����ʾ�ڴ�ĩ�ˣ�����Ļ����
 * ����һ�����������ж�Ӧ���ڴ������ƶ�����ʾ�ڴ����ʼλ��video_mem_start��������
 * �������������ƶ����ֵ�����������ո��ַ���Ȼ�������Ļ�ڴ������ƶ�������������
 * ��������Ļ��Ӧ�ڴ����ʼָ�롢���λ��ָ�����Ļĩ�˶�Ӧ�ڴ�ָ��scr_end��
 * ���Ƕ����������Ƚ�����Ļ�ַ�����-1���ж�Ӧ���ڴ������ƶ�����ʾ�ڴ���ʼλ��
 * 	video_mem_start����Ȼ���������ڴ�λ�ô����һ�пո񣨲������ַ����ݡ�
 * 	%0 -eax(�����ַ�+����)��
 *	%1 -ecx((��Ļ�ַ�����-1)����Ӧ���ַ���/2���Գ����ƶ�)��
 * 	%2 -edi(��ʾ�ڴ���ʼλ��video_mem_start);
 *	%3 -esi(��Ļ�����ڴ���ʼλ��origin)��
 * �ƶ�����[edi]����>[esi]���ƶ�ecx�����֡�
*/
			if (scr_end > video_mem_end)
			{
				__asm__("cld\n\t"
					"rep\n\t"
					"movsl\n\t"
					"movl _video_num_columns,%1\n\t"
					"rep\n\t"
					"stosw"
					::"a" (video_erase_char),
					"c" ((video_num_lines-1)*video_num_columns>>1),
					"D" (video_mem_start),
					"S" (origin)
					:) ;
					/* �巽��λ��	*/
					/* �ظ�����������ǰ��Ļ�ڴ�����	*/
					/* �ƶ�����ʾ�ڴ���ʼ����	*/
					/* ecx=1 ���ַ�����	*/
					/* ������������ո��ַ���	*/

				scr_end -= origin-video_mem_start;
				pos -= origin-video_mem_start;
				origin = video_mem_start;
			} else {
/* ������������Ļĩ�˶�Ӧ���ڴ�ָ��scr_end û�г�����ʾ�ڴ��ĩ��video_mem_end����ֻ����	*/
/* ��������������ַ�(�ո��ַ�)��	*/
/* %0 - eax(�����ַ�+����)��%1 - ecx(��ʾ���ַ�����)��%2 - edi(��Ļ��Ӧ�ڴ����һ�п�ʼ��)��	*/
				__asm__("cld\n\t"
					"rep\n\t"
					"stosw"
					::"a" (video_erase_char),
					"c" (video_num_columns),
					"D" (scr_end-video_size_row)
					:) ;
					/* �巽��λ��	*/
					/* �ظ����������³�������	*/
					/* ��������ַ�(�ո��ַ�)��	*/
			}


/* Ȼ�������Ļ���������ڴ���ʼλ��ֵoriginд����ʾ�������С�	*/
			set_origin();
/* �����ʾ���������ƶ�������ʾ��ָ����top��ʼ��bottom�����е������������ƶ�1�У�
 * ָ����top��ɾ������ʱֱ�ӽ���Ļ��ָ����top����Ļĩ�������ж�Ӧ����ʾ�ڴ�������
 * ���ƶ�1�У������������³��ֵ�������������ַ���
 *	%0 - eax(�����ַ�+����)��
 *	%1 - ecx(top����1�п�ʼ��bottom������Ӧ���ڴ泤����)��
 *	%2 - edi(top���������ڴ�λ��)��
 *	%3 - esi(top+l���������ڴ�λ��)��
*/
		} else {
			__asm__("cld\n\t"
				"rep\n\t"
				"movsl\n\t"
				"movl _video_num_columns,%%ecx\n\t"
				"rep\n\t"
				"stosw"
				::"a" (video_erase_char),
				"c" ((bottom-top-1)*video_num_columns>>1),
				"D" (origin+video_size_row*top),
				"S" (origin+video_size_row*(top+1))
				:) ;
				/* �巽��λ��	*/
				/* ѭ����������top+1 ��bottom ��	*/
				/* ����Ӧ���ڴ���Ƶ�top �п�ʼ����	*/
				/* ecx = 1 ���ַ�����	*/
				/* ����������������ַ���	*/
		}
	}
/* �����ʾ���Ͳ���EGA(��MDA)����ִ�������ƶ���������ΪMDA ��ʾ���ƿ����Զ�����������ʾ��Χ
 * �������Ҳ�����Զ�����ָ�룬�������ﲻ����Ļ���ݶ�Ӧ�ڴ泬����ʾ�ڴ�����������������
 * ������EGA �������ƶ������ȫһ����
 */
	else		/* Not EGA/VGA */
	{
		__asm__("cld\n\t"
			"rep\n\t"
			"movsl\n\t"
			"movl _video_num_columns,%%ecx\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" ((bottom-top-1)*video_num_columns>>1),
			"D" (origin+video_size_row*top),
			"S" (origin+video_size_row*(top+1))
			:) ;
	}
}

/* ���¾�һ�С�
 * ����Ļ�������������ƶ�һ�У���Ӧ��Ļ�����������������ƶ�1�С������ƶ���ʼ�е��Ϸ�
 * ����һ���С��μ������б��˵������������scrup()���ƣ�ֻ��Ϊ�����ƶ���ʾ�ڴ���
 * ��ʱ����������ݸ��ǵ����⣬���Ʋ�������������еģ����ȴ���Ļ������2�е����һ��
 * �ַ���ʼ���Ƶ����һ�У��ٽ�������3�и��Ƶ�������2�еȵȡ���Ϊ��ʱ��EGA/VGA��ʾ
 * ���ͺ�MDA���͵Ĵ��������ȫһ�������Ըú���ʵ����û�б�Ҫд������ͬ�Ĵ��롣������
 * if��else�����еĲ�����ȫһ����
 */
static void scrdown(void)
{
/* ������Ҳ���������ʾ���ͣ�EGA/VGA��MDA���ֱ���в����������ʾ������EGA����ִ��
 * ���в�����������if��else�еĲ�����ȫһ�����Ժ���ں˾ͺ϶�Ϊһ�ˡ���	*/
	if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM)
	{
/* %0-eax(�����ַ�+����)��%1-ecx(top �п�ʼ����Ļĩ��-1 �е���������Ӧ���ڴ泤����)��	*/
/* %2-edi(��Ļ���½����һ������λ��)��%3-esi(��Ļ������2 �����һ������λ��)��	*/
/* �ƶ�����[esi]����>[edi]���ƶ�ecx �����֡�	*/
		__asm__("std\n\t"
			"rep\n\t"
			"movsl\n\t"
			"addl $2,%%edi\n\t"	/* %edi has been decremented by 4 */
			"movl _video_num_columns,%%ecx\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" ((bottom-top-1)*video_num_columns>>1),
			"D" (origin+video_size_row*bottom-4),
			"S" (origin+video_size_row*(bottom-1)-4)
			:) ;
			/* �÷���λ��	*/
			/* �ظ������������ƶ���top �е�bottom-1 ��	*/
			/* ��Ӧ���ڴ����ݡ�	*/
			/* %edi has been decremented by 4 */
			/* %edi �Ѿ���4����ΪҲ�Ƿ���������ַ� */
			/* ��ecx=1 ���ַ�����	*/
			/* �������ַ������Ϸ������С�	*/
	}

/* �������EGA ��ʾ���ͣ���ִ�����²�����Ŀǰ��������ȫһ������	*/
	else		/* Not EGA/VGA */
	{
		__asm__("std\n\t"
			"rep\n\t"
			"movsl\n\t"
			"addl $2,%%edi\n\t"	/* %edi has been decremented by 4 */
			"movl _video_num_columns,%%ecx\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" ((bottom-top-1)*video_num_columns>>1),
			"D" (origin+video_size_row*bottom-4),
			"S" (origin+video_size_row*(bottom-1)-4)
			:) ;
	}
}

/* ���λ������һ�У�If - line feed���У���
������û�д������һ���ϣ���ֱ���޸Ĺ�굱ǰ�б���y++������������Ӧ��ʾ�ڴ�
λ��pos (����һ���ַ�����Ӧ���ڴ泤��)���������Ҫ����Ļ������������һ�С�
��������lf(line feed����)��ָ��������ַ�LF��
*/
static void lf (void)
{
	if (y + 1 < bottom)
	{
		y++;
		pos += video_size_row;		/* ������Ļһ��ռ���ڴ���ֽ�����	*/
		return;
	}
	scrup ();						/* ����Ļ������������һ�С�	*/
}

/* �����ͬ������һ�С�
�����겻����Ļ��һ���ϣ���ֱ���޸Ĺ�굱ǰ�б���y��������������Ӧ��ʾ�ڴ�λ��
pos,��ȥ��Ļ��һ���ַ�����Ӧ���ڴ泤���ֽ�����������Ҫ����Ļ������������һ�С�
��������ri(reVerse index��������)��ָ�����ַ�RI��ת�����С�ESC M����
*/
static void ri (void)
{
	if (y > top)
	{
		y--;
		pos -= video_size_row;		/* ��ȥ��Ļһ��ռ���ڴ���ֽ�����	*/
		return;
	}
	scrdown ();						/* ����Ļ������������һ�С�	*/	
}

/* ���ص���1�У�0�У���
��������Ӧ�ڴ�λ��pos����������к�*2����0�е���������ж�Ӧ���ڴ��ֽڳ��ȡ�
��������cr(carriage return�س�)ָ������Ŀ����ַ��ǻس��ַ���
*/
static void cr (void)
{
	pos -= x << 1;					/* ��ȥ0�е���괦ռ�õ��ڴ��ֽ�����	*/
	x = 0;
}

/* �������ǰһ�ַ����ÿո��������del - deleteɾ������
������û�д���0�У��򽫹���Ӧ�ڴ�λ��pos����2�ֽڣ���Ӧ��Ļ��һ���ַ�����
Ȼ�󽫵�ǰ��������ֵ��1�������������λ�ô��ַ�������
*/
static void
del (void)
{
	if (x)
	{
		pos -= 2;
		x--;
		*(unsigned short *) pos = video_erase_char;
	}
}

/* ɾ����Ļ������λ����صĲ��֡�
 * ANSI�������У���ESC [ Ps J����Ps =
 *	0 -ɾ����괦����Ļ�׶ˣ�
 *	1 -ɾ����Ļ��ʼ����괦��
 *	2 -����ɾ������
 * ����������ָ���Ŀ������о������ֵ��ִ������λ����ص�ɾ�������������ڲ����ַ�����
 * ʱ���λ�ò��䡣��������csi��J (CSI - Control Sequence Introducer����������
 * ��������)ָ���Կ������С�CSI Ps J�����д���
 * ������par -��Ӧ�������������Ps��ֵ��
 */

static void
csi_J (int par)
{
	long count __asm__ ("cx");			/* ��Ϊ�Ĵ���������	*/
	long start __asm__ ("di");

/* ���ȸ�����������ֱ�������Ҫɾ�����ַ�����ɾ����ʼ����ʾ�ڴ�λ�á�	*/
	switch (par)
	{
		case 0:			/* erase from cursor to end of display	������굽��Ļ�׶� */
			count = (scr_end - pos) >> 1;
			start = pos;
			break;
		case 1:			/* erase from start to cursor 	 		ɾ������Ļ��ʼ����괦���ַ� */
			count = (pos - origin) >> 1;	
			start = origin;
			break;
		case 2:			/* erase whole display 	 				ɾ��������Ļ�ϵ��ַ� */
			count = video_num_columns * video_num_lines;
			start = origin;
			break;
		default:
			return;
	}
/* Ȼ��ʹ�ò����ַ���дɾ���ַ��ĵط���
 * %0 - ecx(Ҫɾ�����ַ���count)��%1 - edi(ɾ��������ʼ��ַ)��%2 - eax������Ĳ����ַ�����
 */
__asm__ ("cld\n\t" "rep\n\t" "stosw\n\t");
}

/* ɾ��һ��������λ����صĲ��֡�
 *  ANSIת���ַ����У���ESC [ Ps K����Ps = 0ɾ������β��1�ӿ�ʼɾ����2���ж�ɾ������
 * ���������ݲ���������������еĲ��ֻ������ַ���������������Ļ�������ַ�����Ӱ����
 * ���ַ����������ַ����������ڲ����ַ�����ʱ���λ�ò��䡣
 * ������par -��Ӧ�������������Ps��ֵ��
 */
static void
csi_K (int par)
{
	long count __asm__ ("cx");	/* ���üĴ���������	*/
	long start __asm__ ("di");

/* ���ȸ�����������ֱ�������Ҫɾ�����ַ�����ɾ����ʼ����ʾ�ڴ�λ�á�	*/
	switch (par)
	{
		case 0:			/* erase from cursor to end of line 	 ɾ����굽��β�ַ� */
			if (x >= video_num_columns)
				return;
			count = video_num_columns - x;
			start = pos;
			break;
		case 1:			/* erase from start of line to cursor 	 ɾ�����п�ʼ����괦 */
			start = pos - (x << 1);
			count = (x < video_num_columns) ? x : video_num_columns;
			break;
		case 2:			/* erase whole line 	 				�������ַ�ȫɾ�� */
			start = pos - (x << 1);
			count = video_num_columns;
			break;
		default:
			return;
	}
/* Ȼ��ʹ�ò����ַ���дɾ���ַ��ĵط���
 * %0 - ecx(Ҫɾ�����ַ���count)��%1 - edi(ɾ��������ʼ��ַ)��%2 - eax������Ĳ����ַ�����
 */
__asm__ ("cld\n\t" "rep\n\t" "stosw\n\t");
}

/* ������ʾ�ַ����ԡ�
ANSIת�����У���ESC [Psm^Ps =0Ĭ�����ԣ�1�Ӵ֣�4���»��ߣ�7���ԣ�27���ԡ�
�ÿ������и��ݲ��������ַ���ʾ���ԡ��Ժ����з��͵��ն˵��ַ�����ʹ������ָ������
�ԣ�ֱ���ٴ�ִ�б������������������ַ���ʾ�����ԡ����ڵ�ɫ�Ͳ�ɫ��ʾ�������õ���
����������ģ���������˼򻯴���
 */
void csi_m (void)
{
	int i;

	for (i = 0; i <= npar; i++)
		switch (par[i])
		{
			case 0:
				attr = 0x07;
				break;
			case 1:
				attr = 0x0f;
				break;
			case 4:
				attr = 0x0f;
				break;
			case 7:
				attr = 0x70;
				break;
			case 27:
				attr = 0x07;
				break;
		}
}

/* ����������ʾ��ꡣ������ʾ�ڴ����Ӧλ��pos��������ʾ������������ʾλ�á�	*/
static inline void
set_cursor (void)
{
	cli ();
/* ����ʹ�������Ĵ����˿�ѡ����ʾ�������ݼĴ���rl4(��굱ǰ��ʾλ�ø��ֽ�)��Ȼ��
 * д���굱ǰλ�ø��ֽڣ������ƶ�9λ��ʾ���ֽ��Ƶ����ֽ��ٳ���2�����������Ĭ��
 * ��ʾ�ڴ�����ġ���ʹ�������Ĵ���ѡ��H5��������굱ǰλ�õ��ֽ�д�����С�
 */
	outb_p (14, video_port_reg);			/* ѡ�����ݼĴ���rl4��	*/
	outb_p (0xff & ((pos - video_mem_start) >> 9), video_port_val);
/* ��ʹ�������Ĵ���ѡ��r15��������굱ǰλ�õ��ֽ�д�����С�	*/
	outb_p (15, video_port_reg);			/* ѡ�����ݼĴ���rl5��	*/
	outb_p (0xff & ((pos - video_mem_start) >> 1), video_port_val);
	sti ();
}

/* ���Ͷ�VT100����Ӧ���С�
��Ϊ��Ӧ���������ն������������豸���ԣ�DA��������ͨ�����Ͳ��������������0��DA
�������У���ESC [ 0c����ESC Z��]Ҫ���ն˷���һ���豸���ԣ�DA���������У��ն���
��85���϶����Ӧ�����У�����ESC [?l;2c��]����Ӧ���������У������и����������ն�
�Ǿ��и߼���Ƶ���ܵ�VT100�����նˡ���������ǽ�Ӧ�����з������������У���ʹ��
 copy��to��cooked()�����������븨�������С�
*/
static void
respond (struct tty_struct *tty)
{
	char *p = RESPONSE;

	cli ();						/* ���жϡ�	*/
	while (*p)
	{							/* ���ַ����з���д���С�	*/
		PUTCH (*p, tty->read_q);/* ���ַ����롣include/linux/tty.h��33 �С�	*/
		p++;
	}
	sti ();						/* ���жϡ�	*/
	copy_to_cooked (tty);		/* ת���ɹ淶ģʽ(���븨��������)��tty��io.c��145 �С�	*/
}

/* �ڹ�괦����һ�ո��ַ���
 * �ѹ�꿪ʼ���������ַ�����һ�񣬲��������ַ������ڹ�����ڴ���
 * ��һ���϶����ַ��Ļ����������һ���ַ����������
 */
static void
insert_char (void)
{
	int i = x;
	unsigned short tmp, old = video_erase_char;	/* �����ַ��������ԣ���	*/
	unsigned short *p = (unsigned short *) pos;	/* ����Ӧ�ڴ�λ�á�	*/

	while (i++ < video_num_columns)
	{
		tmp = *p;
		*p = old;
		old = tmp;
		p++;
	}
}

/* �ڹ�괦����һ��	*/
/* ����Ļ���ڴӹ�������е����ڵ׵��������¾�һ�С���꽫�����µĿ����ϡ�	*/
static void
insert_line (void)
{
	int oldtop, oldbottom;

	oldtop = top;					/* ����ԭtop��bottom ֵ��	*/
	oldbottom = bottom;
	top = y;						/* ������Ļ����ʼ�С�	*/
	bottom = video_num_lines;		/* ������Ļ������С�	*/
	scrdown ();						/* �ӹ�꿪ʼ������Ļ�������¹���һ�С�	*/
	top = oldtop;					/* �ָ�ԭtop��bottom ֵ��	*/
	bottom = oldbottom;
}

/* ɾ��һ���ַ���
 * ɾ����괦��һ���ַ�������ұߵ������ַ�����һ��
*/
static void delete_char (void)
{
	int i;
	unsigned short *p = (unsigned short *) pos;

/* ������ĵ�ǰ��λ��x������Ļ�����У��򷵻ء�����ӹ����һ���ַ���ʼ����ĩ����
 * �ַ�����һ��Ȼ�������һ���ַ�����������ַ���	*/
	if (x >= video_num_columns)
		return;
/* �ӹ����һ���ַ���ʼ����ĩ�����ַ�����һ��	*/
	i = x;
	while (++i < video_num_columns)
	{
		*p = *(p + 1);
		p++;
	}
/* ���һ���ַ�����������ַ�(�ո��ַ�)��	*/
	*p = video_erase_char;
}

/* ɾ����������С�	*/
/* ɾ����������У����ӹ�������п�ʼ��Ļ�����Ͼ�һ�С�	*/
static void
delete_line (void)
{
	int oldtop, oldbottom;

/* ���ȱ�����Ļ����ʼ��top�������bottomֵ��Ȼ��ӹ������������Ļ�������Ϲ���
 * һ�С����ָ���Ļ����ʼ��top�������bottom��ԭ��ֵ��	*/
	oldtop = top;					/* ����ԭtop��bottom ֵ��	*/
	oldbottom = bottom;
	top = y;						/* ������Ļ����ʼ�С�	*/
	bottom = video_num_lines;		/* ������Ļ������С�	*/
	scrup ();						/* �ӹ�꿪ʼ������Ļ�������Ϲ���һ�С�	*/
	top = oldtop;					/* �ָ�ԭtop��bottom ֵ��	*/
	bottom = oldbottom;
}

/* �ڹ�괦����nr���ַ���
 * ANSIת���ַ����У���ESC [Pn@�����ڵ�ǰ��괦����1�������ո��ַ���Pn�ǲ������
 * ������Ĭ����1����꽫��Ȼ���ڵ�1������Ŀո��ַ������ڹ�����ұ߽���ַ������ơ�
 * �����ұ߽���ַ�������ʧ��
 * ����nr =ת���ַ������еĲ���n��
 */
static void csi_at (unsigned int nr)
{
/* ���������ַ�������һ���ַ��������Ϊһ���ַ������������ַ���nrΪ0�������1��
 * �ַ���Ȼ��ѭ������ָ�����ո��ַ���	*/
	if (nr > video_num_columns)
		nr = video_num_columns;
	else if (!nr)
		nr = 1;
/* ѭ������ָ�����ַ�����	*/
	while (nr--)
		insert_char ();
}

/* �ڹ��λ�ô�����nr�С�
 * ANSIת���ַ����У���ESC [PnL�����ÿ��������ڹ�괦����1�л���п��С�������ɺ��
 * ��λ�ò��䡣�����б�����ʱ��������¹��������ڵ��������ƶ�����������ʾҳ���оͶ�ʧ��
 * ����nr =ת���ַ������еĲ���Pn��
 */
static void csi_L (unsigned int nr)
{
/* ������������������Ļ������������Ϊ��Ļ��ʾ����������������nrΪ0�������1�С�
 * Ȼ��ѭ������ָ������nr�Ŀ��С�	*/
	if (nr > video_num_lines)
		nr = video_num_lines;
	else if (!nr)
		nr = 1;

	while (nr--)				/* ѭ������ָ������nr��	*/
		insert_line ();
}

/* ɾ����괦��nr���ַ���
 * ANSIת�����У���ESC [PnP�����ÿ������дӹ�괦ɾ��Pn���ַ�����һ���ַ���ɾ��ʱ��
 * ����������ַ������ơ�������ұ߽紦����һ�����ַ���������Ӧ�������һ�������ַ�
 * ��ͬ�����������˼򻯴�����ʹ���ַ���Ĭ�����ԣ��ڵװ��ֿո�0x0720�������ÿ��ַ���
 * ����nr =ת���ַ������еĲ���Pn��
 */
static void csi_P (unsigned int nr)
{
/* ���ɾ�����ַ�������һ���ַ��������Ϊһ���ַ�������ɾ���ַ���nrΪ0����ɾ��1��
 * �ַ���Ȼ��ѭ��ɾ����괦ָ���ַ���nr��	*/
	if (nr > video_num_columns)
		nr = video_num_columns;
	else if (!nr)
		nr = 1;

	while (nr--)			/* ѭ��ɾ��ָ���ַ���nr��	*/
		delete_char ();
}

/* ɾ����괦��nr�С�
ANSIת�����У���ESC [PnM�����ÿ��������ڹ��������ڣ��ӹ�������п�ʼɾ��1�л��
�С����б�ɾ��ʱ�����������ڵı�ɾ�����µ��л������ƶ������һ�����������1���С�
��Pn������ʾҳ��ʣ�������������н�ɾ����Щʣ���У����Թ��������ⲻ�����á�
����nr =ת���ַ������еĲ���Pn��
*/
static void
csi_M (unsigned int nr)
{
/* ���ɾ��������������Ļ������������Ϊ��Ļ��ʾ����������ɾ��������nrΪ0����ɾ��
 * 1�С�Ȼ��ѭ��ɾ��ָ������nr��*/
	if (nr > video_num_lines)
		nr = video_num_lines;
	else if (!nr)
		nr = 1;

	while (nr--)			/* ѭ��ɾ��ָ������nr��	*/
		delete_line ();
}

static int saved_x = 0;		/* ����Ĺ���кš�	*/
static int saved_y = 0;		/* ����Ĺ���кš�	*/

/* ���浱ǰ���λ�á�	*/
static void
save_cur (void)
{
	saved_x = x;
	saved_y = y;
}

/* �ָ�����Ĺ��λ�á�	*/
static void restore_cur(void)
{
	gotoxy(saved_x, saved_y);
}

/* ����̨д������
 * ���ն˶�Ӧ��ttyд���������ȡ�ַ������ÿ���ַ����з��������ǿ����ַ���ת������
 * ���У�����й�궨λ���ַ�ɾ���ȵĿ��ƴ���������ͨ�ַ���ֱ���ڹ�괦��ʾ��
 * ����tty�ǵ�ǰ����̨ʹ�õ�tty�ṹָ�롣
 */
void con_write(struct tty_struct * tty)
{
	int nr;
	char c;

/* ����ȡ��д��������������ַ���nr��Ȼ����Զ����е�ÿ���ַ����д����ڴ���ÿ����
 * ����ѭ�������У����ȴ�д������ȡһ�ַ�c������ǰ�洦���ַ������õ�״̬state�ֲ�
 * ����д���״̬֮���ת����ϵΪ��
 * state =	0:��ʼ״̬������ԭ��״̬4;����ԭ��״̬1�����ַ����ǡ�[����
 * 			1:ԭ��״̬0�������ַ���ת���ַ�ESC(0xlb = 033 = 27)�������ָ�״̬0��
 * 			2:ԭ��״̬1�������ַ��ǡ�[����
 * 			3:ԭ��״̬2������ԭ��״̬3�������ַ��ǡ����������֡�
 * 			4:ԭ��״̬3�������ַ����ǡ����������֣������ָ�״̬0��
 */
	nr = CHARS(tty->write_q);
	while (nr--)
	{
		GETCH(tty->write_q,c);
		switch(state)
		{
			case 0:
/* �����д������ȡ�����ַ�����ͨ��ʾ�ַ����룬��ֱ�Ӵӵ�ǰӳ���ַ�����ȡ����Ӧ����ʾ 
�ַ������ŵ���ǰ�����������ʾ�ڴ�λ�ô�����ֱ����ʾ���ַ���Ȼ��ѹ��λ������һ��
�ַ�λ�á�����أ�����ַ����ǿ����ַ�Ҳ������չ�ַ�����(31<c<127)����ô������ǰ��
�괦����ĩ�˻�ĩ�����⣬�򽫹���Ƶ�����ͷ�С����������λ�ö�Ӧ���ڴ�ָ��pos��Ȼ
���ַ�Cд����ʾ�ڴ���pos���������������1�У�ͬʱҲ��pos��Ӧ���ƶ�2���ֽڡ�
*/
	/* ����ַ����ǿ����ַ�(c>31)������Ҳ������չ�ַ�(c<127)����:	*/
				if (c>31 && c<127)				/* ����ͨ��ʾ�ַ���	*/
				{
	/* ����ǰ��괦����ĩ�˻�ĩ�����⣬�򽫹���Ƶ�����ͷ�С����������λ�ö�Ӧ���ڴ�ָ��pos��	*/
					if (x>=video_num_columns) {	/* Ҫ���У�	*/
						x -= video_num_columns;
						pos -= video_size_row;
						lf();
					}
	/* ���ַ�c д����ʾ�ڴ���pos ���������������1 �У�ͬʱҲ��pos ��Ӧ���ƶ�2 ���ֽڡ�	*/
					__asm__("movb _attr,%%ah\n\t"	/* д�ַ���	*/
						"movw %%ax,%1\n\t"
						::"a" (c),"m" (*(short *)pos)
						:) ;
					pos += 2;
					x++;
	/* ����ַ�c ��ת���ַ�ESC����ת��״̬state ��1��	*/
				} else if (c==27)				/*  ESC -ת������ַ���	*/
					state=1;
	/* ����ַ�c �ǻ��з�(10)�����Ǵ�ֱ�Ʊ��VT(11)�������ǻ�ҳ��FF(12)�����ƶ���굽��һ�С�	*/
				else if (c==10 || c==11 || c==12)
					lf();
	/* ����ַ�c �ǻس���CR(13)���򽫹���ƶ���ͷ��(0 ��)��	*/
				else if (c==13)					/* CR -�س���	*/
					cr();
	/* ����ַ�c ��DEL(127)���򽫹���ұ�һ�ַ�����(�ÿո��ַ����)����������Ƶ�������λ�á�	*/
				else if (c==ERASE_CHAR(tty))
					del();
	/* ����ַ�c ��BS(backspace,8)���򽫹������1 �񣬲���Ӧ��������Ӧ�ڴ�λ��ָ��pos��	*/
				else if (c==8)					/* BS-���ˡ�	*/
				{
					if (x)
					{
						x--;
						pos -= 2;
					}
	/* ����ַ�c ��ˮƽ�Ʊ��TAB(9)���򽫹���Ƶ�8 �ı������ϡ�����ʱ�������������Ļ���������	*/
	/* �򽫹���Ƶ���һ���ϡ�	*/
				} else if (c==9) {				/* HT-ˮƽ�Ʊ�	*/
					c=8-(x&7);
					x += c;
					pos += c<<1;
					if (x>video_num_columns) {
						x -= video_num_columns;
						pos -= video_size_row;
						lf();
					}
					c=9;
				} else if (c==7)	/* ����ַ�c�������BEL(7)������÷�����������������������	*/
					sysbeep();
				break;
			case 1:
				state=0;
				if (c=='[')			/* ����ַ�c ��'['����״̬state ת��2��	*/
					state=2;
				else if (c=='E')	/* ����ַ�c ��'E'�������Ƶ���һ�п�ʼ��(0 ��)��	*/
					gotoxy(0,y+1);
				else if (c=='M')	/* ����ַ�c ��'M'����������һ�С�	*/
					ri();
				else if (c=='D')	/* ����ַ�c ��'D'����������һ�С�	*/
					lf();
				else if (c=='Z')	/* ����ַ�c ��'Z'�������ն�Ӧ���ַ����С�	*/
					respond(tty);
				else if (x=='7')	/* ����ַ�c ��'7'���򱣴浱ǰ���λ�á�ע���������д��Ӧ����(c=='7')��	*/
					save_cur();
				else if (x=='8')	/* ����ַ�c ��'8'����ָ���ԭ����Ĺ��λ�á�ע���������д��Ӧ����(c=='8')��	*/
					restore_cur();
				break;
/* �����״̬1(��ת���ַ�ESC)ʱ�յ��ַ���[�����������һ����������������CSI������ת������״̬2������	*/
			case 2:
/* ���ȶ�ESCת���ַ����б������������par[]���㣬��������nparָ���������
 * ����״̬Ϊ3������ʱ�ַ����ǡ���������ֱ��ת��״̬3ȥ��������ʱ�ַ��ǡ�������
 * ˵������������ն��豸˽�����У��������һ�������ַ�������ȥ����һ�ַ�����
 * ��״̬3������봦������ֱ�ӽ���״̬3��������	*/
				for(npar=0;npar<NPAR;npar++)
					par[npar]=0;
				npar=0;
				state=3;
				if (ques=(c=='?'))
					break;
/* ״̬3���ڰ�ת���ַ������е������ַ�ת������ֵ������par[]�����С����ԭ��״̬2��
 * ����ԭ������״̬3����ԭ�ַ��ǡ����������֣�����״̬3������ʱ������ַ�c�Ƿֺ�
 * ����������������parδ����������ֵ��1��׼��������һ���ַ���	*/
			case 3:
				if (c==';' && npar<NPAR-1) {
					npar++;
					break;
/* ��������ַ�c�������ַ���0��-��9�����򽫸��ַ�ת������ֵ����npar�������������
 *10����������׼��������һ���ַ��������ֱ��ת��״̬4��
 */
				} else if (c>='0' && c<='9') {
					par[npar]=10*par[npar]+c-'0';
					break;
				} else state=4;
/* ״̬4�Ǵ���ת���ַ����е����һ����1^�漸��״̬�Ĵ��������Ѿ������ת���ַ�
 * ���е�ǰ�����֣����ڸ��ݲ����ַ��������һ���ַ��������ִ����صĲ��������ԭ
 * ״̬��״̬3�������ַ����ǡ����������֣���ת��״̬4�������ȸ�λ״̬state=0��
 */
			case 4:
				state=0;
				switch(c)
				{
					/* ����ַ�c ��'G'��'`'����par[]�е�һ�����������кš����кŲ�Ϊ�㣬�򽫹������һ��	*/
					case 'G': case '`':
						if (par[0]) par[0]--;
						gotoxy(par[0],y);
						break;
					/* ����ַ�c ��'A'�����һ���������������Ƶ�������������Ϊ0 ������һ�С�	*/
					case 'A':
						if (!par[0]) par[0]++;
						gotoxy(x,y-par[0]);
						break;
					/* ����ַ�c ��'B'��'e'�����һ���������������Ƶ�������������Ϊ0 ������һ�С�	*/
					case 'B': case 'e':
						if (!par[0]) par[0]++;
						gotoxy(x,y+par[0]);
						break;
					/* ����ַ�c ��'C'��'a'�����һ���������������Ƶĸ�����������Ϊ0 ������һ��	*/
					case 'C': case 'a':
						if (!par[0]) par[0]++;
						gotoxy(x+par[0],y);
						break;
					/* ����ַ�c ��'D'�����һ���������������Ƶĸ�����������Ϊ0 ������һ��	*/
					case 'D':
						if (!par[0]) par[0]++;
						gotoxy(x-par[0],y);
						break;
					/* ����ַ�c ��'E'�����һ�����������������ƶ������������ص�0 �С�������Ϊ0 ������һ�С�	*/
					case 'E':
						if (!par[0]) par[0]++;
						gotoxy(0,y+par[0]);
						break;
					/* ����ַ�c ��'F'�����һ�����������������ƶ������������ص�0 �С�������Ϊ0 ������һ�С�	*/
					case 'F':
						if (!par[0]) par[0]++;
						gotoxy(0,y-par[0]);
						break;
					/* ����ַ�c ��'d'�����һ�����������������ڵ��к�(��0 ����)��	*/
					case 'd':
						if (par[0]) par[0]--;
						gotoxy(x,par[0]);
						break;
					/* ����ַ�c ��'H'��'f'�����һ�������������Ƶ����кţ��ڶ��������������Ƶ����кš�	*/
					case 'H': case 'f':
						if (par[0]) par[0]--;
						if (par[1]) par[1]--;
						gotoxy(par[1],par[0]);
						break;
					/* ����ַ�c ��'J'�����һ�����������Թ������λ�������ķ�ʽ��	*/
					/* ANSI ת�����У�'ESC [sJ'(s = 0 ɾ����굽��Ļ�׶ˣ�1 ɾ����Ļ��ʼ����괦��2 ����ɾ��)��	*/
					case 'J':
						csi_J(par[0]);
						break;
					/* ����ַ�c ��'K'�����һ�����������Թ������λ�ö������ַ�����ɾ������ķ�ʽ��	*/
					/* ANSI ת���ַ����У�'ESC [sK'(s = 0 ɾ������β��1 �ӿ�ʼɾ����2 ���ж�ɾ��)��	*/
					case 'K':
						csi_K(par[0]);
						break;
					/* ����ַ�c ��'L'����ʾ�ڹ��λ�ô�����n ��(ANSI ת���ַ�����'ESC [nL')��	*/
					case 'L':
						csi_L(par[0]);
						break;
					/* ����ַ�c ��'M'����ʾ�ڹ��λ�ô�ɾ��n ��(ANSI ת���ַ�����'ESC [nM')��	*/
					case 'M':
						csi_M(par[0]);
						break;
					/* ����ַ�c ��'P'����ʾ�ڹ��λ�ô�ɾ��n ���ַ�(ANSI ת���ַ�����'ESC [nP')��	*/
					case 'P':
						csi_P(par[0]);
						break;
					/* ����ַ�c ��'@'����ʾ�ڹ��λ�ô�����n ���ַ�(ANSI ת���ַ�����'ESC [n@')��	*/
					case '@':
						csi_at(par[0]);
						break;
					/* ����ַ�c ��'m'����ʾ�ı��괦�ַ�����ʾ���ԣ�����Ӵ֡����»��ߡ���˸�����Եȡ�	*/
					/* ANSI ת���ַ����У�'ESC [nm'��n = 0 ������ʾ��1 �Ӵ֣�4 ���»��ߣ�7 ���ԣ�27 ������ʾ��	*/
					case 'm':
						csi_m();
						break;
					/* ����ַ�c ��'r'�����ʾ�������������ù�������ʼ�кź���ֹ�кš�	*/
					case 'r':
						if (par[0]) par[0]--;
						if (!par[1]) par[1] = video_num_lines;
						if (par[0] < par[1] &&
						    par[1] <= video_num_lines) {
							top=par[0];
							bottom=par[1];
						}
						break;
					/* ����ַ�c ��'s'�����ʾ���浱ǰ�������λ�á�	*/
					case 's':
						save_cur();
						break;
					/* ����ַ�c ��'u'�����ʾ�ָ���굽ԭ�����λ�ô���	*/
					case 'u':
						restore_cur();
						break;
				}
		}
	}	
	set_cursor();		/* �������������õĹ��λ�ã�����ʾ���������͹����ʾλ�á�	*/
}

/*
 *  void con_init(void);
 *
 * This routine initalizes console interrupts, and does nothing
 * else. If you want the screen to clear, call tty_write with
 * the appropriate escape-sequece.
 *
 * Reads the information preserved by setup.s to determine the current display
 * type and sets everything accordingly.
 */
/*
* void con_init(void);
* ����ӳ����ʼ������̨�жϣ�����ʲô�������������������Ļ�ɾ��Ļ�����ʹ��
* �ʵ���ת���ַ����е���tty_write()������
*
* ��ȡsetup.s ���򱣴����Ϣ������ȷ����ǰ��ʾ�����ͣ���������������ز�����
*/
/* ����̨��ʼ��������init/main.c�б����á�
 * �ú������ȸ���setup.s����ȡ�õ�ϵͳӲ��������ʼ�����ü���������ר�õľ�̬ȫ�ֱ�
 * ����Ȼ�������ʾ��ģʽ����ɫ���ǲ�ɫ��ʾ������ʾ�����ͣ�EGA/VGA����CGA���ֱ���
 * ����ʾ�ڴ���ʼλ���Լ���ʾ�����Ĵ�������ʾ��ֵ�Ĵ����˿ںš�������ü����ж�����
 * ����������λ�Լ����жϵ�����λ����������̿�ʼ������
 * ��619�ж���һ���Ĵ�������a���ñ�������������һ���Ĵ����У��Ա��ڸ�Ч���ʺͲ�����
 * ����ָ����ŵļĴ�������eax�������д�ɡ�register unsigned char a asm(��ax��)������
*/
void con_init(void)
{
	register unsigned char a;
	char *display_desc = "????";
	char *display_ptr;

/* ���ȸ���setup.s����ȡ�õ�ϵͳӲ�����������������39��47�У���ʼ������������ר��
 * �ľ�̬ȫ�ֱ�����	*/
	video_num_columns = ORIG_VIDEO_COLS;		/* ��ʾ����ʾ�ַ�������	*/
	video_size_row = video_num_columns * 2;		/* ÿ����ʹ���ֽ�����	*/
	video_num_lines = ORIG_VIDEO_LINES;			/* ��ʾ����ʾ�ַ�������	*/
	video_page = ORIG_VIDEO_PAGE;				/* ��ǰ��ʾҳ�档	*/
	video_erase_char = 0x0720;					/* �����ַ�(0x20 ��ʾ�ַ��� 0x07 ������)��	*/
	
/* Ȼ�������ʾģʽ�ǵ�ɫ���ǲ�ɫ�ֱ�������ʹ�õ���ʾ�ڴ���ʼλ���Լ���ʾ�Ĵ�������
 * �˿ںź���ʾ�Ĵ������ݶ˿ںš����ԭʼ��ʾģʽ����7�����ʾ�ǵ�ɫ��ʾ����	*/
	if (ORIG_VIDEO_MODE == 7)			/* Is this a monochrome display? */
	{
		video_mem_start = 0xb0000;			/* ���õ���ӳ���ڴ���ʼ��ַ��	*/
		video_port_reg = 0x3b4;				/* ���õ��������Ĵ����˿ڡ�	*/
		video_port_val = 0x3b5;				/* ���õ������ݼĴ����˿ڡ�	*/
		
/* �������Ǹ���BIOS�ж�int 0x10����0x12��õ���ʾģʽ��Ϣ���ж���ʾ���ǵ�ɫ��ʾ��
 * ���ǲ�ɫ��ʾ������ʹ�������жϹ������õ���BX�Ĵ�������ֵ������0x10����˵����EGA
 * ������˳�ʼ��ʾ����ΪEGA��ɫ����ȻEGA�����н϶���ʾ�ڴ棬���ڵ�ɫ��ʽ�����ֻ
 * �����õ�ַ��Χ��0xb0000��0xb8000֮�����ʾ�ڴ档Ȼ������ʾ�������ַ���Ϊ��EGAm����
 * ������ϵͳ��ʼ���ڼ���ʾ�������ַ�������ʾ����Ļ�����Ͻǡ�
 * ע�⣬����ʹ���� bx�ڵ����ж�int 0x10ǰ���Ƿ񱻸ı�ķ������жϿ������͡���BL��
 * �жϵ��ú�ֵ���ı䣬��ʾ��ʾ��֧��Ah=12h���ܵ��ã���EGA����Ƴ�����VGA�����͵�
 * ��ʾ�������жϵ��÷���ֵδ�䣬��ʾ��ʾ����֧��������ܣ���˵����һ�㵥ɫ��ʾ����
 */
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10)
		{
			video_type = VIDEO_TYPE_EGAM;			/* ������ʾ����(EGA ��ɫ)��	*/
			video_mem_end = 0xb8000;				/* ������ʾ�ڴ�ĩ�˵�ַ��	*/
			display_desc = "EGAm";					/* ������ʾ�����ַ�����	*/
		}
/* ���BX �Ĵ�����ֵ����0x10����˵���ǵ�ɫ��ʾ��MDA����������Ӧ������	*/
		else
		{
			video_type = VIDEO_TYPE_MDA;			/* ������ʾ����(MDA ��ɫ)��	*/
			video_mem_end = 0xb2000;				/* ������ʾ�ڴ�ĩ�˵�ַ��	*/
			display_desc = "*MDA";					/* ������ʾ�����ַ�����	*/
		}
	}
/* �����ʾģʽ��Ϊ7����Ϊ��ɫģʽ����ʱ���õ���ʾ�ڴ���ʼ��ַΪ0xb800����ʾ���������Ĵ�	*/
/* ���˿ڵ�ַΪ0x3d4�����ݼĴ����˿ڵ�ַΪ0x3d5��	*/
	else								/* If not, it is color. */
	{
		video_mem_start = 0xb8000;			/* ��ʾ�ڴ���ʼ��ַ��	*/
		video_port_reg = 0x3d4;				/* ���ò�ɫ��ʾ�����Ĵ����˿ڡ�	*/
		video_port_val = 0x3d5;				/* ���ò�ɫ��ʾ���ݼĴ����˿ڡ�	*/
/* ���ж���ʾ��������BX������0x10����˵����EGA/VGA��ʾ������ʱʵ�������ǿ���ʹ
 * ��32KB��ʾ�ڴ棨0xb8000 �� 0xc0000�������ó���ֻʹ��������16KB��ʾ�ڴ档
 */
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10)
		{
			video_type = VIDEO_TYPE_EGAC;			/* ������ʾ����(EGA ��ɫ)��	*/
			video_mem_end = 0xbc000;				/* ������ʾ�ڴ�ĩ�˵�ַ��	*/
			display_desc = "EGAc";					/* ������ʾ�����ַ�����	*/
		}
/* ���BX�Ĵ�����ֵ����0x10����˵����CGA��ʾ����ֻʹ��8KB��ʾ�ڴ档	*/
		else
		{
			video_type = VIDEO_TYPE_CGA;			/* ������ʾ����(CGA)��	*/
			video_mem_end = 0xba000;				/* ������ʾ�ڴ�ĩ�˵�ַ��	*/
			display_desc = "*CGA";					/* ������ʾ�����ַ�����	*/
		}
	}

	/* Let the user known what kind of display driver we are using */
	/* ���û�֪����������ʹ����һ����ʾ�������� */

/* Ȼ����������Ļ�����Ͻ���ʾ�����ַ��������õķ�����ֱ�ӽ��ַ���д����ʾ�ڴ����Ӧ
 * λ�ô������Ƚ���ʾָ��display��ptrָ����Ļ��1���Ҷ˲�4���ַ�����ÿ���ַ���2��
 * �ֽڣ���˼�8����Ȼ��ѭ�������ַ������ַ�������ÿ����1���ַ����տ�1�������ֽڡ�
 */
	display_ptr = ((char *) video_mem_start) + video_size_row - 8;
	while (*display_desc)
	{
		*display_ptr++ = *display_desc++;	/* �����ַ���	*/
		display_ptr++;						/* �տ������ֽ�λ�á�	*/
	}
	
	/* Initialize the variables used for scrolling (mostly EGA/VGA)	*/
	/* ��ʼ�����ڹ����ı���(��Ҫ����EGA/VGA) */

	origin = video_mem_start;					/* ������ʼ��ʾ�ڴ��ַ��	*/
	scr_end = video_mem_start + video_num_lines * video_size_row;	/* ���������ڴ��ַ��	*/
	top = 0;									/* ��кš�	*/
	bottom = video_num_lines;					/* ����кš�	*/

/* ����ʼ����ǰ�������λ�ú͹���Ӧ���ڴ�λ��pos�����������ü����ж�0x21������
 * ��������&keyboard_interrupt�Ǽ����жϴ�����̵�ַ��ȡ��8259A�жԼ����жϵ����Σ�
 * ������Ӧ���̷�����IRQ1�����źš����λ���̿�������������̿�ʼ����������
*/
	gotoxy (ORIG_X, ORIG_Y);					/* ��ʼ�����λ��x,y �Ͷ�Ӧ���ڴ�λ��pos��	*/
	set_trap_gate (0x21, &keyboard_interrupt);	/* ���ü����ж������š�	*/
	outb_p (inb_p (0x21) & 0xfd, 0x21);			/* ȡ��8259A �жԼ����жϵ����Σ�����IRQ1��	*/
	a = inb_p (0x61);							/* �ӳٶ�ȡ���̶˿�0x61(8255A �˿�PB)��	*/
	outb_p (a | 0x80, 0x61);					/* ���ý�ֹ���̹���(λ7 ��λ)��	*/
	outb (a, 0x61);								/* ��������̹��������Ը�λ���̲�����	*/
}
/* from bsd-net-2: */

/* ֹͣ������	*/
/* ��λ8255A PB�˿ڵ�λ1��λ0���μ�kernel/sched.c�����Ķ�ʱ�����˵����	*/
void sysbeepstop(void)
{
	/* disable counter 2 */
	outb(inb_p(0x61)&0xFC, 0x61);
}

int beepcount = 0;

/* ��ͨ������
 * 8255AоƬPB�˿ڵ�λ1�����������Ŀ����źţ�λ0����8253��ʱ��2�����źţ��ö�ʱ
 * �������������������������Ϊ������������Ƶ�ʡ����Ҫʹ��������������Ҫ���������ȿ�
 * ��PB�˿ڣ�0x61��λ1��λ0(��λ)��Ȼ�����ö�ʱ��2ͨ������һ���Ķ�ʱƵ�ʼ��ɡ�
 * �μ�boot/setup.s�����8259AоƬ��̷�����kernel/sched.c�����Ķ�ʱ�����˵����
 */
static void sysbeep(void)
{
	/* enable counter 2 */		/* ������ʱ��2 */
	outb_p (inb_p (0x61) | 3, 0x61);
	/* set command for counter 2, 2 byte write 	 �����ö�ʱ��2 ���� */
	outb_p (0xB6, 0x43);		/* ��ʱ��оƬ�����ּĴ����˿ڡ�	*/
	/* send 0x637 for 750 HZ */	/* 	����Ƶ��Ϊ750HZ������Ͷ�ʱֵ0x637 */
	outb_p (0x37, 0x42);		/* ͨ��2���ݶ˿ڷֱ��ͼ����ߵ��ֽڡ�	*/
	outb (0x06, 0x42);
	/* 1/8 second */			/* ����ʱ��Ϊ1/8 �� */
	beepcount = HZ / 8;
}
