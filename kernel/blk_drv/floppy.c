/*
* linux/kernel/floppy.c
*
* (C) 1991 Linus Torvalds
*/

/*
* 02.12.91 - Changed to static variables to indicate need for reset
* and recalibrate. This makes some things easier (output_byte reset
* checking etc), and means less interrupt jumping in case of errors,
* so the code is hopefully easier to understand.
*/
/*
* 02.12.91 - �޸ĳɾ�̬����������Ӧ��λ������У����������ʹ��ĳЩ����
* ��������Ϊ���㣨output_byte ��λ���ȣ���������ζ���ڳ���ʱ�ж���ת
* Ҫ��һЩ������ϣ�������ܸ����ױ���⡣
*/

/*
* This file is certainly a mess. I've tried my best to get it working,
* but I don't like programming floppies, and I have only one anyway.
* Urgel. I should check for more errors, and do more graceful error
* recovery. Seems there are problems with several drives. I've tried to
* correct them. No promises.
*/
/*
* ����ļ���Ȼ�Ƚϻ��ҡ����Ѿ���������ʹ���ܹ����������Ҳ�ϲ��������̣�
* ������Ҳֻ��һ�����������⣬��Ӧ��������Ĳ�������Լ���������Ĵ���
* ����ĳЩ�������������󻹴���һЩ���⡣���Ѿ������Ž��о����ˣ������ܱ�֤
* ��������ʧ��
*/
/* Ӣ��ע���еġ�Urgel����������ӦӢ�ĵ��ʡ�Ugh�����õ����������ں��л������֡�
2003��9�·�Linus��˵����������ʵĵط��ͱ�ʾ��صĴ����������Ƚϲ������
���������������
*/


/*
* As with hd.c, all routines within this file can (and will) be called
* by interrupts, so extreme caution is needed. A hardware interrupt
* handler may not sleep, or a kernel panic will happen. Thus I cannot
* call "floppy-on" directly, but have to set a special timer interrupt
* etc.
*
* Also, I'm not certain this works on more than 1 floppy. Bugs may
* abund.
*/
/*
* ��ͬhd.c �ļ�һ�������ļ��е������ӳ����ܹ����жϵ��ã�������Ҫ�ر�
* ��С�ġ�Ӳ���жϴ�������ǲ���˯�ߵģ������ں˾ͻ�ɵ��(����)?����˲���
* ֱ�ӵ���"floppy-on"����ֻ������һ�������ʱ���жϵȡ�
*
* ���⣬�Ҳ��ܱ�֤�ó������ڶ���1 ��������ϵͳ�Ϲ������п��ܴ��ڴ���
*/

#include <linux/sched.h>	/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 �����ݣ�	*/
							/* ����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/
#include <linux/fs.h>		/* �ļ�ϵͳͷ�ļ��������ļ���ṹ��file,buffer_head,m_inode �ȣ���	*/
#include <linux/kernel.h>	/* �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣	*/
#include <linux/fdreg.h>	/* ����ͷ�ļ����������̿�����������һЩ���塣	*/
#include <asm/system.h>		/* ϵͳͷ�ļ������������û��޸�������/�ж��ŵȵ�Ƕ��ʽ���ꡣ	*/
#include <asm/io.h>			/* io ͷ�ļ�������Ӳ���˿�����/���������䡣	*/
#include <asm/segment.h>	/* �β���ͷ�ļ����������йضμĴ���������Ƕ��ʽ��ຯ����	*/

/* �����������豸�ŷ��ų����������������У����豸�ű����ڰ���blk.h�ļ�֮ǰ�����塣
 * ��Ϊblk.h�ļ���Ҫ�õ�������ų���ֵ��ȷ��һЩ��������ط��ų����ͺꡣ
 */
#define MAJOR_NR 2			/* ���������豸����2��	*/
#include "blk.h"			/* ���豸ͷ�ļ��������������ݽṹ�����豸���ݽṹ�ͺ꺯������Ϣ��	*/

static int recalibrate = 0;	/* ��־��1��ʾ��Ҫ����У����ͷλ�ã���ͷ���������	*/
static int reset = 0;		/* ��־��1��ʾ��Ҫ���и�λ������	*/
static int seek = 0;		/* ��־��1��ʾ��Ҫִ��Ѱ��������	*/

/* ��ǰ��������Ĵ��� DOR (Digital Output Register)�������� kernel/sched.c��204 �С�
�ñ����������������е���Ҫ��־������ѡ�����������Ƶ��������������λ���̿�������
������/��ֹDMA���ж�������μ������б���DOR�Ĵ�����˵����
*/
extern unsigned char current_DOR;	/* ��ǰ��������Ĵ���(Digital Output Register)��	*/

/* �ֽ�ֱ���������Ƕ����꣩����ֵval�����port�˿ڡ�	*/
#define immoutb_p(val,port) \
__asm__("outb %0,%1\n\tjmp 1f\n1:\tjmp 1f\n1:"::"a" ((char) (val)),"i" (port))

/* �������궨�����ڼ����������豸�š�
 * ����x�Ǵ��豸�š����豸��=TYPE*4 + DRIVE�����㷽���μ��б��
 */

#define TYPE(x) ((x)>>2)	/* �������ͣ�2--1.2Mb��7--1.44Mb����	*/
#define DRIVE(x) ((x)&0x03)	/* ������ţ�0--3 ��ӦA--D����	*/
/*
* Note that MAX_ERRORS=8 doesn't imply that we retry every bad read
* max 8 times - some types of errors increase the errorcount by 2,
* so we might actually retry only 5-6 times before giving up.
*/
/*
* ע�⣬���涨��MAX_ERRORS=8 ������ʾ��ÿ�ζ����������8 �� - ��Щ����
* �Ĵ��󽫰ѳ������ֵ��2����������ʵ�����ڷ�������֮ǰֻ�賢��5-6 �鼴�ɡ�
*/
#define MAX_ERRORS 8
/*
* globals used by 'result()'
*/
/* �����Ǻ���'result()'ʹ�õ�ȫ�ֱ��� */
/* ��Щ״̬�ֽ��и�����λ�ĺ�����μ�include/linux/fdreg.h ͷ�ļ���	*/
#define MAX_REPLIES 7			/* FDC ��෵��7 �ֽڵĽ����Ϣ��	*/
static unsigned char reply_buffer[MAX_REPLIES];	/* ���FDC ���صĽ����Ϣ��	*/
#define ST0 (reply_buffer[0])	/* ���ؽ��״̬�ֽ�0��	*/
#define ST1 (reply_buffer[1])	/* ���ؽ��״̬�ֽ�1��	*/
#define ST2 (reply_buffer[2])	/* ���ؽ��״̬�ֽ�2��	*/
#define ST3 (reply_buffer[3])	/* ���ؽ��״̬�ֽ�3��	*/

/*
* This struct defines the different floppy types. Unlike minix
* linux doesn't have a "search for right type"-type, as the code
* for that is convoluted and weird. I've got enough problems with
* this driver as it is.
*
* The 'stretch' tells if the tracks need to be boubled for some
* types (ie 360kB diskette in 1.2MB drive etc). Others should
* be self-explanatory.
*/
/*
* ��������̽ṹ�����˲�ͬ���������͡���minix ��ͬ���ǣ�linux û��
* "������ȷ������"-���ͣ���Ϊ���䴦��Ĵ������˷ѽ��ҹֵֹġ�������
* �Ѿ��������������������ˡ�
*
* ��ĳЩ���͵����̣�������1.2MB �������е�360kB ���̵ȣ���'stretch'����
* ���ŵ��Ƿ���Ҫ���⴦����������Ӧ���������ġ�
*/
/* ���̲����У�
 * size ��С(������)��
 * sect ÿ�ŵ���������
 * head ��ͷ����
 * track �ŵ�����
 * stretch �Դŵ��Ƿ�Ҫ���⴦����־����
 * gap ������϶����(�ֽ���)��
 * rate ���ݴ������ʣ�
 * spec1 ��������4 λ�������ʣ�����λ��ͷж��ʱ�䣩��	*/

static struct floppy_struct
{
	unsigned int size, sect, head, track, stretch;
	unsigned char gap, rate, spec1;
}
floppy_type[] =
{
	{		0, 0,0, 0,0,0x00,0x00,0x00 },	/* no testing */
	{	720, 9,2,40,0,0x2A,0x02,0xDF },	/* 360kB PC diskettes */
	{ 2400,15,2,80,0,0x1B,0x00,0xDF },	/* 1.2 MB AT-diskettes */
	{	720, 9,2,40,1,0x2A,0x02,0xDF },	/* 360kB in 720kB drive */
	{ 1440, 9,2,80,0,0x2A,0x02,0xDF },	/* 3.5" 720kB diskette */
	{	720, 9,2,40,1,0x23,0x01,0xDF },	/* 360kB in 1.2MB drive */
	{ 1440, 9,2,80,0,0x23,0x01,0xDF },	/* 720kB in 1.2MB drive */
	{ 2880,18,2,80,0,0x1B,0x00,0xCF },	/* 1.44MB diskette */
};

/*
* Rate is 0 for 500kb/s, 2 for 300kbps, 1 for 250kbps
* Spec1 is 0xSH, where S is stepping rate (F=1ms, E=2ms, D=3ms etc),
* H is head unload time (1=16ms, 2=32ms, etc)
*
 * Spec2 is (HLD<<1 | ND), where HLD is head load time (1=2ms, 2=4 ms etc)
 * and ND is set means no DMA. Hardcoded to 6 (HLD=6ms, use DMA).
 */
/*
* ��������rate��0 ��ʾ500kb/s��1 ��ʾ300kbps��2 ��ʾ250kbps��
* ����spec1 ��0xSH������S �ǲ������ʣ�F-1 ���룬E-2ms��D=3ms �ȣ���
* H �Ǵ�ͷж��ʱ�䣨1=16ms��2=32ms �ȣ�
*
* spec2 �ǣ�HLD<<1 | ND��������HLD �Ǵ�ͷ����ʱ�䣨1=2ms��2=4ms �ȣ�
* ND ��λ��ʾ��ʹ��DMA��No DMA�����ڳ�����Ӳ�����6��HLD=6ms��ʹ��DMA����
*/
/* ע�⣬������ͷ����ʱ�����дHLD���д�ɱ�׼��HLT -- Head Load Time��
floppy��interrupt()��system��call.s�����������жϴ�����̱�š����ｫ�����̳�ʼ
������floppy��initO (��457��)ʹ������ʼ���ж���������������
extern void floppy interrupt (void);
����boot/head.s��132�д��������ʱ���̻����������������Ļ����������ڴ�1MB
����ĳ���ط�������Ҫ��DMA������������ʱ�������򴦡���Ϊ8237AоƬֻ����1MB��
ַ��Χ��Ѱַ��
*/
extern void floppy_interrupt (void);
extern char tmp_floppy_area[1024];

/*
* These are global variables, as that's the easiest way to give
* information to interrupts. They are the data used for the current
* request.
*/
/*
* ������һЩȫ�ֱ�������Ϊ���ǽ���Ϣ�����жϳ�����򵥵ķ�ʽ��������
* ���ڵ�ǰ��������ݡ�
*/
/* ��Щ��ν�ġ�ȫ�ֱ�������ָ�������жϴ�������е��õ�c����ʹ�õı�������Ȼ��Щ
 * c�������ڱ������ڡ�	*/
static int cur_spec1 = -1;
static int cur_rate = -1;
static struct floppy_struct *floppy = floppy_type;
static unsigned char current_drive = 0;
static unsigned char sector = 0;
static unsigned char head = 0;
static unsigned char track = 0;
static unsigned char seek_track = 0;
static unsigned char current_track = 255;
static unsigned char command = 0;
unsigned char selected = 0;
struct task_struct *wait_on_floppy_select = NULL;

/* ȡ��ѡ��������
�����������ָ��������nr��ǰ��û�б�ѡ��������ʾ������Ϣ��Ȼ��λ������ѡ����־
selected,�����ѵȴ�ѡ���������������������Ĵ���(DOR)�ĵ�2λ����ָ��ѡ�����
����0-3��Ӧ A-D����
*/
void
floppy_deselect (unsigned int nr)
{
	if (nr != (current_DOR & 3))
		printk ("floppy_deselect: drive not selected\n\r");
	selected = 0;							/* ��λ������ѡ����־��	*/
	wake_up (&wait_on_floppy_select);		/* ���ѵȴ�������	*/
}

/*
 * floppy-change is never called from an interrupt, so we can relax a bit
 * here, sleep etc. Note that floppy-on tries to set current_DOR to point
 * to the desired drive, but it will probably not survive the sleep if
 * several floppies are used at the same time: thus the loop.
*/
/*
* floppy-change()���Ǵ��жϳ����е��õģ������������ǿ�������һ�£�˯���ȡ�
* ע��floppy-on()�᳢������current_DOR ָ�������������������ͬʱʹ�ü���
* ����ʱ����˯�ߣ���˴�ʱֻ��ʹ��ѭ����ʽ��
*/
/* ���ָ�����������̸��������
����nr�������š�������̸������򷵻�1�����򷵻�0��
�ú�������ѡ������ָ��������nr��Ȼ��������̿���������������Ĵ���DIR��ֵ������
���������е������Ƿ񱻸��������ú����ɳ���fs/buffer.c�е�check_disk_change()��
�����ã���119�У���
*/
int
floppy_change (unsigned int nr)
{
/* ����Ҫ��������������ת�������ﵽ��������ת�١�����Ҫ����һ��ʱ�䡣���õķ�������
��kernel/sched.c�����̶�ʱ����do_floppy_timer()����һ������ʱ����floppy_on()
�����������ж���ʱ�Ƿ񵽣�mOn_timer[nr]==0?������û�е����õ�ǰ���̼���˯�ߵȴ���
����ʱ����do_floppy_timer)�ỽ�ѵ�ǰ���̡�
*/
repeat:
	floppy_on (nr);		/* �������ȴ�ָ������nr(kernel/sched.c���� 232 ��)�� 
/* ��������������ת��֮���������鿴һ�µ�ǰѡ��������ǲ��Ǻ�������ָ��������nr��
�����ǰѡ�����������ָ��������nr�������Ѿ�ѡ�����������������õ�ǰ��������
�жϵȴ�״̬���Եȴ�����������ȡ��ѡ�����μ�����floppy��deselectO�������ǰû
��ѡ������������������������ȡ��ѡ����ʹ��ǰ���񱻻���ʱ����ǰ������Ȼ����ָ��
������nr������ת��������ʼ������ѭ���ȴ���
*/
	while ((current_DOR & 3) != nr && selected)
		interruptible_sleep_on (&wait_on_floppy_select);
/* �������̿�������ѡ������ָ��������nr������ȡ��������Ĵ���DIR��ֵ����������
λ��λ7����λ�����ʾ�����Ѹ�������ʱ���ɹر���ﲢ����1�˳�������ر���ﷵ
��0�˳�����ʾ����û�б�������
*/
	if ((current_DOR & 3) != nr)
		goto repeat;
/* ȡ��������Ĵ���ֵ��������λ��λ7����λ�����ʾ�����Ѹ�������ʱ�ر���ﲢ�˳�����1��	*/
/* ����ر�����˳�����0��	*/
	if (inb (FD_DIR) & 0x80)
		{
			floppy_off (nr);
			return 1;
		}
	floppy_off (nr);
	return 0;
}

/* �����ڴ滺��飬��1024�ֽڡ�
���ڴ��ַfrom������1024�ֽ����ݵ���ַto����
*/
#define copy_buffer(from,to) \
 __asm__( "cld ; rep ; movsl" \
 :: "c" (BLOCK_SIZE/4), "S" ((long)(from)), "D" ((long)(to)))

//: "cx", "di", "si")

/* ���ã���ʼ��������DMAͨ����
���������ݶ�д������ʹ��DMA���еġ������ÿ�ν������ݴ���֮ǰ��Ҫ����DMAоƬ
��ר������������ͨ��2���й�DMA��̷�����μ������б�����Ϣ��
*/
static void
setup_DMA (void)
{
	long addr = (long) CURRENT->buffer;	/* ��ǰ��������������ڴ���λ�ã���ַ����	*/
/* ���ȼ��������Ļ���������λ�á���������������ڴ�1MB���ϵ�ĳ���ط�������Ҫ��
DMA������������ʱ��������tmp��floppy��area��������Ϊ8237AоƬֻ����1MB��ַ��
Χ��Ѱַ�������д���������Ҫ�����ݴ�������������Ƶ�����ʱ����
*/
	cli ();
/* ��������������ڴ�1M ���ϵĵط�����DMA ������������ʱ��������(tmp_floppy_area ����)	*/
/* (��Ϊ8237A оƬֻ����1M ��ַ��Χ��Ѱַ)�������д��������轫���ݸ��Ƶ�����ʱ����	*/
	if (addr >= 0x100000)
	{
		addr = (long) tmp_floppy_area;
		if (command == FD_WRITE)
			copy_buffer (CURRENT->buffer, tmp_floppy_area);
	}
/* ���������ǿ�ʼ����DMAͨ��2���ڿ�ʼ����֮ǰ��Ҫ�����θ�ͨ������ͨ�����μĴ���
�˿�ΪOxOA��λ0-1ָ��DMAͨ��(0��3)��λ2: 1��ʾ���Σ�0��ʾ��������Ȼ����
DMA�������˿� 12��11д�뷽ʽ�֣�������0x46��д������0x4A������д�봫��ʹ��
��������ַaddr����Ҫ������ֽ���0x3ff(0��1023)�����λ��DMAͨ��2�����Σ�
����DMA2����DREQ�źš�	*/
/* mask DMA 2	����DMA ͨ��2 */
/* ��ͨ�����μĴ����˿�Ϊ0x10��λ0-1 ָ��DMA ͨ��(0--3)��λ2��1 ��ʾ���Σ�0 ��ʾ��������	*/
	immoutb_p (4 | 2, 10);
/* output command byte. I don't know why, but everyone (minix, */
/* sanches & canton) output this twice, first to 12 then to 11 */
/* ��������ֽڡ����ǲ�֪��Ϊʲô������ÿ���ˣ�minix��*/
/* sanches ��canton����������Σ�������12 �ڣ�Ȼ����11 �� */
/* ����Ƕ���������DMA�������ġ�����Ⱥ󴥷������˿� 12�ͷ�ʽ�Ĵ����˿� 11д��
��ʽ�֣�����ʱ��0x46��д����0x4A����
���ڸ�ͨ���ĵ�ַ�ͼ����Ĵ�������16λ�ģ��������������ʱ����Ҫ��2�ν��в�����
һ�η��ʵ��ֽڣ���һ�η��ʸ��ֽڡ���ʵ����д�ĸ��ֽ������Ⱥ󴥷�����״̬������
��������Ϊ0ʱ������ʵ��ֽڣ����ֽڴ�����Ϊ1ʱ������ʸ��ֽڡ�ÿ����һ�Σ�
�ô�������״̬�ͱ仯һ�Ρ���д�˿� 12�Ϳ��Խ��������ó�0״̬���Ӷ���16λ�Ĵ�
�������ôӵ��ֽڿ�ʼ��
*/
	__asm__ ("outb %%al,$12\n\tjmp 1f\n1:\tjmp 1f\n1:\t"
		 "outb %%al,$11\n\tjmp 1f\n1:\tjmp 1f\n1:"::
		 "a" ((char) ((command == FD_READ) ? DMA_READ : DMA_WRITE)));
/* 8 low bits of addr	��ַ��0-7 λ */
/* ��DMA ͨ��2 д���/��ǰ��ַ�Ĵ������˿�4����	*/
	immoutb_p (addr, 4);
	addr >>= 8;
/* bits 8-15 of addr	��ַ��8-15 λ */
	immoutb_p (addr, 4);
	addr >>= 8;
/* bits 16-19 of addr	��ַ16-19 λ */
/* DMA ֻ������1M �ڴ�ռ���Ѱַ�����16-19 λ��ַ�����ҳ��Ĵ���(�˿�0x81)��	*/
	immoutb_p (addr, 0x81);
/* low 8 bits of count-1 (1024-1=0x3ff)	��������8 λ(1024-1=0x3ff) */
/* ��DMA ͨ��2 д���/��ǰ�ֽڼ�����ֵ���˿�5����	*/
	immoutb_p (0xff, 5);
/* high 8 bits of count-1	��������8 λ */
/* һ�ι�����1024 �ֽڣ�������������	*/
	immoutb_p (3, 5);
/* activate DMA 2	����DMA ͨ��2 ������ */
/* ��λ��DMA ͨ��2 �����Σ�����DMA2 ����DREQ �źš�	*/
	immoutb_p (0 | 2, 10);
	sti ();
}

/* ���������������һ���ֽ�����������
�������������һ���ֽ�֮ǰ����������Ҫ����׼����״̬���������ݴ��䷽���������
�ɴ�CPU��FDC����˺�����Ҫ���ȶ�ȡ������״̬��Ϣ������ʹ����ѭ����ѯ��ʽ����
���ʵ���ʱ��������������ø�λ��־reset��
*/
static void
output_byte (char byte)
{
	int counter;
	unsigned char status;

	if (reset)
		return;
/* ѭ����ȡ��״̬������FD_STATUS(0x3f4)��״̬�����״̬��STATUS_READY ����STATUS_DIR=0	*/
/* (CPU����>FDC)���������ݶ˿����ָ���ֽڡ�	*/
	for (counter = 0; counter < 10000; counter++)
		{
			status = inb_p (FD_STATUS) & (STATUS_READY | STATUS_DIR);
			if (status == STATUS_READY)
	{
		outb (byte, FD_DATA);
		return;
	}
		}
/* �����ѭ��1 ��ν��������ܷ��ͣ����ø�λ��־������ӡ������Ϣ��	*/
	reset = 1;
	printk ("Unable to send byte to FDC\n\r");
}

/* ��ȡFDCִ�еĽ����Ϣ��
�����Ϣ���7���ֽڣ����������reply��buffer ���С����ض���Ľ���ֽ�����������
ֵ=-1�����ʾ����������ʽ�����溯�����ơ�
*/
static int
result (void)
{
	int i = 0, counter, status;

/* ����λ��־����λ���������˳���ȥִ�к��������еĸ�λ����������ѭ����ȡ��״̬��
���� FD��STATUS (0x3f4)��״̬��
*/
	if (reset)
		return -1;
	for (counter = 0; counter < 10000; counter++)
	{
		status = inb_p (FD_STATUS) & (STATUS_DIR | STATUS_READY | STATUS_BUSY);
/* ���������״̬��READY����ʾ�Ѿ�û�����ݿ�ȡ���򷵻��Ѷ�ȡ���ֽ���i��	*/
		if (status == STATUS_READY)
			return i;
/* ���������״̬�Ƿ����־��λ��cro+FDc������׼���á�æ����ʾ�����ݿɶ�ȡ������
�ѿ������еĽ�����ݶ��뵽Ӧ���������С�����ȡMAX��REPLIES(7)���ֽڡ�
*/
		if (status == (STATUS_DIR | STATUS_READY | STATUS_BUSY))
		{
			if (i >= MAX_REPLIES)
				break;
			reply_buffer[i++] = inb_p (FD_DATA);
		}
	}
/* �����ѭ��1��ν���ʱ�����ܷ��ͣ����ø�λ��־������ӡ������Ϣ��	*/
	reset = 1;
	printk ("Getstatus times out\n\r");
	return -1;
}

/* ���̶�д����������
�ú����������̶�д���������ȷ����Ҫ��ȡ�Ľ�һ���ж��������ǰ��������������
�������ڹ涨�����������MAX��ERR0RS(8��)�����ٶԵ�ǰ����������һ���Ĳ���
���ԡ������/д��������Ѿ�����MAX��ERR0RS/2������Ҫ����������λ������������
��λ��־reset������������������������ֵ��һ�룬��ֻ������У��һ�´�ͷλ�ã�
������������У����־recalibrate�������ĸ�λ������У��������ں����ĳ����н��С�
*/
static void
bad_flp_intr (void)
{
/* ���Ȱѵ�ǰ��������������1�������ǰ���������������������������������ȡ
��ѡ����ǰ�����������������������������û�б����£���
*/
	CURRENT->errors++;		/* ��ǰ��������������1��	*/
/* �����ǰ���������������������������������ȡ��ѡ����ǰ������������������������£���	*/
	if (CURRENT->errors > MAX_ERRORS)
		{
			floppy_deselect (current_drive);
			end_request (0);
		}
/* �����ǰ�������������������������������һ�룬���ø�λ��־������������и�λ������	*/
/* Ȼ�����ԡ���������������У��һ�£����ԡ�	*/
	if (CURRENT->errors > MAX_ERRORS / 2)
		reset = 1;
	else
		recalibrate = 1;
}

 /*
		* Ok, this interrupt is called after a DMA read/write has succeeded,
		* so we check the results, and copy any buffers.
	*/
/*
* OK��������жϴ���������DMA ��/д�ɹ�����õģ��������ǾͿ��Լ��ִ�н����
* �����ƻ������е����ݡ�
*/
/* ���̶�д�����ɹ��жϵ��ú�������
�ú��������������������������������жϴ�������б����á��������ȶ�ȡ�������״
̬��Ϣ���ݴ��жϲ����Ƿ�������Ⲣ����Ӧ���������/д�����ɹ�����ô��������
�Ƕ����������仺�������ڴ�1MB����λ�ã�����Ҫ�����ݴ�������ʱ���������Ƶ�����
��Ļ�������
*/
static void rw_interrupt (void)
{
/* ��ȡFDCִ�еĽ����Ϣ��������ؽ���ֽ���������7������״̬�ֽ�0��1��2�д���
�����־����ô����д��������ʾ������Ϣ���ͷŵ�ǰ����������������ǰ���������
��ִ�г����������Ȼ�����ִ���������������������״̬�ĺ���μ�fdreg.h�ļ���
( 0xf8 = ST0��INTR | STO��SE | STO��ECE | ST0��NR )
( Oxbf = ST1��E0C |ST1��CRC | ST1��0R | ST1��ND | ST1��WP | ST1��MAM��Ӧ���� 0xb7)
( 0x73 = ST2��CM | ST2��CRC |ST2��WC |ST2��BC |ST2��MAM )
 */
	if (result () != 7 || (ST0 & 0xf8) || (ST1 & 0xbf) || (ST2 & 0x73))
		{
			if (ST1 & 0x02)
	{			/* 0x02 = ST1_WP - Write Protected��	*/
		printk ("Drive %d is write protected\n\r", current_drive);
		floppy_deselect (current_drive);
		end_request (0);
	}
			else
	bad_flp_intr ();
			do_fd_request ();
			return;
		}
/* �����ǰ������Ļ�����λ��1M ��ַ���ϣ���˵���˴����̶����������ݻ�������ʱ�������ڣ�	*/
/* ��Ҫ���Ƶ�������Ļ������У���ΪDMA ֻ����1M ��ַ��ΧѰַ����	*/
	if (command == FD_READ && (unsigned long) (CURRENT->buffer) >= 0x100000)
		copy_buffer (tmp_floppy_area, CURRENT->buffer);
/* �ͷŵ�ǰ����(ȡ��ѡ��)��ִ�е�ǰ����������������ѵȴ���������Ľ��У�����
�ȴ�����������Ľ��̣����еĻ������������豸������������ɾ����������ټ���ִ
���������������������
*/
	floppy_deselect (current_drive);
	end_request (1);
	do_fd_request ();
}

/* ����DMAͨ��2�������̿������������Ͳ��������1�ֽ�����+ 0~7�ֽڲ�������
��reset��־û����λ����ô�ڸú����˳��������̿�����ִ������Ӧ��/д������ͻ�
����һ�������ж����󣬲���ʼִ�������жϴ������
*/
inline void
setup_rw_floppy (void)
{
	setup_DMA ();					/* ��ʼ������DMA ͨ����	*/
	do_floppy = rw_interrupt;		/* �������жϵ��ú���ָ�롣	*/
	output_byte (command);			/* ���������ֽڡ�	*/
	output_byte (head << 2 | current_drive);	/* ���Ͳ�������ͷ��+�������ţ���	*/
	output_byte (track);			/* ���Ͳ������ŵ��ţ���	*/
	output_byte (head);				/* ���Ͳ�������ͷ�ţ���	*/
	output_byte (sector);			/* ���Ͳ�������ʼ�����ţ���	*/
	output_byte (2);				/* sector size = 512 ���Ͳ���(�ֽ���(N=2)512 �ֽ�)��	*/
	output_byte (floppy->sect);		/* ���Ͳ�����ÿ�ŵ�����������	*/
	output_byte (floppy->gap);		/* ���Ͳ���������������ȣ���	*/
	output_byte (0xFF);				/* sector size (0xff when n!=0 ?) */
									/* ���Ͳ�������N=0 ʱ������������ֽڳ��ȣ����������á�	*/
/* �������κ�һ��output_byte()��������������ø�λ��־reset����ʱ��������ȥִ��
do��fd��request()�еĸ�λ������롣
*/
	if (reset)
		do_fd_request ();
}

 /*
		* This is the routine called after every seek (or recalibrate) interrupt
		* from the floppy controller. Note that the "unexpected interrupt" routine
		* also does a recalibrate, but doesn't come here.
	*/
/*
* ���ӳ�������ÿ�����̿�����Ѱ����������У�����жϺ󱻵��õġ�ע��
* "unexpected interrupt"(�����ж�)�ӳ���Ҳ��ִ������У�������������ڴ˵ء�
*/
/* Ѱ�������жϵ��ú�����
 * ���ȷ��ͼ���ж�״̬������״̬��ϢST0 �ʹ�ͷ���ڴŵ���Ϣ����������ִ�д������
 * ��⴦���ȡ���������̲���������������״̬��Ϣ���õ�ǰ�ŵ�������Ȼ����ú���
 * setup_rw_floppy()����DMA ��������̶�д����Ͳ�����
 */
static void
seek_interrupt (void)
{
/* ���ȷ��ͼ���ж�״̬����Ի�ȡѰ������ִ�еĽ��������������������ؽ����
Ϣ�������ֽڣ�ST0�ʹ�ͷ��ǰ�ŵ��š�Ȼ���ȡFDCִ�еĽ����Ϣ��������ؽ����
����������2������ST0��ΪѰ�����������ߴ�ͷ���ڴŵ���ST1)�������趨�ŵ�����˵
�������˴�������ִ�м������������Ȼ�����ִ�������������ִ�и�λ����
*/
/* sense drive status */ /* ���������״̬ */
	output_byte (FD_SENSEI);
	if (result () != 2 || (ST0 & 0xF8) != 0x20 || ST1 != seek_track)
		{
			bad_flp_intr ();
			do_fd_request ();
			return;
		}
/* ��Ѱ�������ɹ��������ִ�е�ǰ����������̲������������̿�������������Ͳ�����	*/
	current_track = ST1;		/* ���õ�ǰ�ŵ���	*/
	setup_rw_floppy ();			/* ����DMA ��������̲�������Ͳ�����	*/
}

 /*
		* This routine is called when everything should be correctly set up
		* for the transfer (ie floppy motor is on and the correct floppy is
		* selected).
	*/
/*
* �ú������ڴ��������������Ϣ����ȷ���úú󱻵��õģ�Ҳ����������ѿ���
* ������ѡ������ȷ�����̣���������
*/
/* ��д���ݴ��亯����	*/
static void
transfer (void)
{
/* ���ȼ�鵱ǰ�����������Ƿ����ָ���������Ĳ����������Ǿͷ���������������������
����Ӧ����������1:��4λ�������ʣ�����λ��ͷж��ʱ�䣻����2:��ͷ����ʱ�䣩��
Ȼ���жϵ�ǰ���ݴ��������Ƿ���ָ����������һ�£������Ǿͷ���ָ������������ֵ��
���ݴ������ʿ��ƼĴ���(FD��DCR)��
*/
	if (cur_spec1 != floppy->spec1)		/* ��⵱ǰ������	*/
	{
		cur_spec1 = floppy->spec1;
		output_byte (FD_SPECIFY);		/* �������ô��̲������	*/
		output_byte (cur_spec1);		/* hut etc	���Ͳ�����	*/
		output_byte (6);				/* Head load time =6ms, DMA */
	}
/* �жϵ�ǰ���ݴ��������Ƿ���ָ����������һ�£������Ǿͷ���ָ������������ֵ�����ݴ���	*/
/* ���ʿ��ƼĴ���(FD_DCR)��	*/
	if (cur_rate != floppy->rate)		/* ��⵱ǰ���ʡ�	*/
		outb_p (cur_rate = floppy->rate, FD_DCR);
/* �������κ�һ��output_byteO����ִ�г�����λ��־reset�ͻᱻ��λ���������
������Ҫ���һ��reset��־����reset��ı���λ�ˣ�������ȥִ��do_fd_request()
�еĸ�λ������롣
*/
	if (reset)
	{
		do_fd_request ();
		return;
	}
/* �����ʱ��Ѱ����־Ϊ�㣨����ҪѰ������������DMA�������̿�����������Ӧ��������
�Ͳ����󷵻ء������ִ��Ѱ���������������������жϴ�����ú���ΪѰ���жϺ�����
�����ʼ�ŵ��Ų����������ʹ�ͷѰ������Ͳ�������ʹ�õĲ������ǵ�112-121����
���õ�ȫ�ֱ���ֵ�������ʼ�ŵ���seek��trackΪ0����ִ������У�������ô�ͷ����λ��
*/
	if (!seek)
	{
		setup_rw_floppy ();						/* ������������顣	*/
		return;
	}
	do_floppy = seek_interrupt;					/* Ѱ���жϵ��ú�����	*/
	if (seek_track)								/* ��ʼ�ŵ��š�	*/
	{
		output_byte (FD_SEEK);					/* ���ʹ�ͷѰ�����	*/
		output_byte (head << 2 | current_drive);/*���Ͳ�������ͷ��+��ǰ�����š�	*/
		output_byte (seek_track);				/* ���Ͳ������ŵ��š�	*/
	}
	else
	{
		output_byte (FD_RECALIBRATE);			/* ��������У�����	*/
		output_byte (head << 2 | current_drive);/*���Ͳ�������ͷ��+��ǰ�����š�	*/
	}
/* ͬ���أ��������κ�һ��output��byte()����ִ�г�����λ��־reset�ͻᱻ��λ��
��reset��ı���λ�ˣ�������ȥִ��do��fd��request()�еĸ�λ������롣
*/
	if (reset)
		do_fd_request ();
}

 /*
		* Special case - used after a unexpected interrupt (or reset)
	*/
/*
* �������-���������жϣ���λ�������
*/
/* ��������У���жϵ��ú�����
 * ���ȷ��ͼ���ж�״̬����޲�������������ؽ�������������ø�λ��־����������
 * У����־���㡣Ȼ���ٴ�ִ�������������������Ӧ������
 */
static void
recal_interrupt (void)
{
	output_byte (FD_SENSEI);					/* ���ͼ���ж�״̬���	*/
	if (result () != 2 || (ST0 & 0xE0) == 0x60)	/* ������ؽ���ֽ���������2 ������	*/
		reset = 1;								/* �쳣���������ø�λ��־��	*/
	else										/* ����λ����У����־��	*/
		recalibrate = 0;
	do_fd_request ();							/* ִ�����������	*/
}

/* ���������ж������жϵ��ú�����
 * ���ȷ��ͼ���ж�״̬����޲�������������ؽ�������������ø�λ��־������������
 * У����־��
 */
void
unexpected_floppy_interrupt (void)
{
	output_byte (FD_SENSEI);					/* ���ͼ���ж�״̬���	*/
	if (result () != 2 || (ST0 & 0xE0) == 0x60)	/* ������ؽ���ֽ���������2 ������	*/
		reset = 1;								/* �쳣���������ø�λ��־��	*/
	else										/* ����������У����־��	*/
		recalibrate = 1;
}

/* ��������У����������	*/
/* �����̿�����FDC��������У������Ͳ���������λ����У����־�������̿�����ִ����
����У������ͻ����������������ж��е���recal��interruptO������	*/
static void
recalibrate_floppy (void)
{
	recalibrate = 0;							/* ��λ����У����־��	*/
	current_track = 0;							/* ��ǰ�ŵ��Ź��㡣	*/
	do_floppy = recal_interrupt;				/* �������жϵ��ú���ָ��ָ������У�����õ�C������	*/
	output_byte (FD_RECALIBRATE);				/* �����������У����	*/
	output_byte (head << 2 | current_drive);	/* ���Ͳ���������ͷ�żӣ���ǰ�������š�	*/
/* �������κ�һ��out_byte()����ִ�г�����λ��־reset�ͻᱻ��λ���������
������Ҫ���һ��reset��־����reset��ı���λ�ˣ�������ȥִ��do_fd_request()
�еĸ�λ������롣
*/

	if (reset)									/* �������(��λ��־����λ)�����ִ����������	*/
		do_fd_request ();
}

/* ���̿�����FDC��λ�жϵ��ú�����
�ú�������������������˸�λ��������������������жϴ�������б����á�
���ȷ��ͼ���ж�״̬����޲�������Ȼ��������صĽ���ֽڡ����ŷ����趨����
�����������ز���������ٴε������������do_fd_requestOȥִ������У��
������������ִ��output_byteO��������ʱ��λ��־�ֻᱻ��λ�����Ҳ�����ٴ�ȥ
ִ�и�λ����
*/
static void
reset_interrupt (void)
{
	output_byte (FD_SENSEI);			/* ���ͼ���ж�״̬���	*/
	(void) result ();					/* ��ȡ����ִ�н���ֽڡ�	*/
	output_byte (FD_SPECIFY);			/* �����趨�����������	*/
	output_byte (cur_spec1);			/* hut etc	���Ͳ�����	*/
	output_byte (6);					/* Head load time =6ms, DMA */
	do_fd_request ();					/* ����ִ����������	*/
}

 /*
		* reset is done by pulling bit 2 of DOR low for a while.
	*/
/* FDC ��λ��ͨ������������Ĵ���(DOR)λ2 ��0 һ���ʵ�ֵ� */
/* ��λ���̿�������
 * �ú����������ò����ͱ�־���Ѹ�λ��־��0��Ȼ�����������cur_specl��cur_rate
 * ��Ϊ��Ч����Ϊ��λ��������������������Ҫ�������á�����������Ҫ����У����־��
 * ������FDCִ�и�λ�����������������ж��е��õ�C����reset��interruptO�����
 * ��DOR�Ĵ���λ2��0������Զ�����ִ�и�λ��������ǰ��������Ĵ���DOR��λ2����
 * ��/��λ����λ��
 */
static void
reset_floppy (void)
{
	int i;

	reset = 0;							/* ��λ��־��0��	*/
	cur_spec1 = -1;						/* ʹ��Ч��	*/
	cur_rate = -1;
	recalibrate = 1;					/* ����У����־��λ��	*/
	printk ("Reset-floppy called\n\r");	/* ��ʾִ�����̸�λ������Ϣ��	*/
	cli ();								/* ���жϡ�	*/
	do_floppy = reset_interrupt;		/* �����������жϴ�������е��õĺ�����	*/
	outb_p (current_DOR & ~0x04, FD_DOR);	/* �����̿�����FDC ִ�и�λ������	*/
	for (i = 0; i < 100; i++)			/* �ղ������ӳ١�	*/
		__asm__ ("nop");
	outb (current_DOR, FD_DOR);			/* ���������̿�������	*/
	sti ();								/* ���жϡ�	*/
}

/* ����������ʱ�жϵ��ú�����
 * ��ִ��һ��������Ҫ��Ĳ���֮ǰ��Ϊ�˵ȴ�ָ�����������ת�������������Ĺ���ת�٣�
 do_fd_requestO����Ϊ׼���õĵ�ǰ�����������һ����ʱ��ʱ�������������Ǹö�ʱ��
 ����ʱ���õĺ����������ȼ����������Ĵ���(D0R)��ʹ��ѡ��ǰָ������������Ȼ��
 ����ִ�����̶�д���亯��transfer()��
 */
static void
floppy_on_interrupt (void)
{
/* We cannot do a floppy-select, as that might sleep. We just force it */
/* ���ǲ�����������ѡ�����������Ϊ���������ܻ��������˯�ߡ�����ֻ����ʹ���Լ�ѡ�� */
/* �����ǰ������������������Ĵ���DOR�еĲ�ͬ������Ҫ��������DORΪ��ǰ��������
 * ������������Ĵ��������ǰDOR�Ժ�ʹ�ö�ʱ���ӳ�2���δ�ʱ�䣬��������õ�ִ
 * �С�Ȼ��������̶�д���亯��transfer()������ǰ��������DOR�е��������ô�Ϳ���
 * ֱ�ӵ������̶�д���亯����
*/

	selected = 1;					/* ����ѡ��ǰ��������־��	*/
/* �����ǰ������������������Ĵ���DOR �еĲ�ͬ������������DOR Ϊ��ǰ������current_drive��	*/
/* ��ʱ�ӳ�2 ���δ�ʱ�䣬Ȼ��������̶�д���亯��transfer()������ֱ�ӵ������̶�д���亯����	*/
	if (current_drive != (current_DOR & 3))
	{
		current_DOR &= 0xFC;
		current_DOR |= current_drive;
		outb (current_DOR, FD_DOR);	/* ����������Ĵ��������ǰDOR��	*/
		add_timer (2, &transfer);	/* ��Ӷ�ʱ����ִ�д��亯����	*/
	}
	else
		transfer ();				/* ִ�����̶�д���亯����	*/
}

/* ���̶�д�����������
�ú�����������������������Ҫ�ĺ�������Ҫ�����ǣ��ٴ����и�λ��־������У����־��
λ������������������е��豸�ż���ȡ��������ָ�������Ĳ����飻�������ںӶ�ʱ����
�����̶�/д������
*/

void
do_fd_request (void)
{
	unsigned int block;
/* ���ȼ���Ƿ��и�λ��־������У����־��λ�������򱾺�����ִ����ر�־�Ĵ�����
��ͷ��ء������λ��־����λ����ִ�����̸�λ���������ء��������У����־����λ��
��ִ����������У�����������ء�
*/
	seek = 0;			/* ��Ѱ����־��	*/
	if (reset)			/* �����λ��־����λ����ִ�����̸�λ�����������ء�	*/
	{
		reset_floppy ();
		return;
	}
	if (recalibrate)	/* �������У����־����λ����ִ����������У�������������ء�	*/
	{
		recalibrate_floppy ();
		return;
	}
/* ���������������ܴ����￪ʼ����������blk.h�ļ��е�INIT��REQUEST��������������
�Ϸ��ԣ������û�����������˳����μ�blk.h��127����Ȼ�������������е��豸��ȡ����
����ָ�������Ĳ����顣��������齫�����������������̲���ʹ�õ�ȫ�ֱ��������飨��
��112 - 122�У����������豸���е��������ͣ�MIN0R(CURRENT->dev)>>2��������������
������floppy��type ��������ֵ��ȡ��ָ�������Ĳ����顣
*/
	INIT_REQUEST;
	floppy = (MINOR (CURRENT->dev) >> 2) + floppy_type;
/* ���濪ʼ����112��122���ϵ�ȫ�ֱ���ֵ�������ǰ��������current��drive����������
��ָ�����������ţ����ñ�־seek����ʾ��ִ�ж�/д����֮ǰ��Ҫ����������ִ��Ѱ����
��Ȼ��ѵ�ǰ������������Ϊ��������ָ�����������š�
*/
	if (current_drive != CURRENT_DEV)	/* CURRENT_DEV ����������ָ���������š�	*/
		seek = 1;
	current_drive = CURRENT_DEV;
/* ���ö�д��ʼ����block����Ϊÿ�ζ�д���Կ�Ϊ��λ��1��Ϊ2����������������ʼ����
��Ҫ����ȴ�����������С2������������˵����������������Ч�������ô�����������
ȥִ����һ�������
*/
	block = CURRENT->sector;	/* ȡ��ǰ��������������ʼ�����š���>block��	*/
	if (block + 2 > floppy->size)
	{							/* ���block+2 ���ڴ���������������	*/
		end_request (0);		/* �����������������	*/
		goto repeat;
	}
/* �����Ӧ�ڴŵ��ϵ������š���ͷ�š��ŵ��š���Ѱ�ŵ��ţ�������������ͬ��ʽ���̣���	*/
	sector = block % floppy->sect;			/* ��ʼ������ÿ�ŵ�������ȡģ���ôŵ��������š�	*/
	block /= floppy->sect;					/* ��ʼ������ÿ�ŵ�������ȡ��������ʼ�ŵ�����	*/
	head = block % floppy->head;			/* ��ʼ�ŵ����Դ�ͷ��ȡģ���ò����Ĵ�ͷ�š�	*/
	track = block / floppy->head;			/* ��ʼ�ŵ����Դ�ͷ��ȡ�����ò����Ĵŵ��š�	*/
	seek_track = track << floppy->stretch;	/* ��Ӧ���������������ͽ��е�������Ѱ���š�	*/
/* �ٿ����Ƿ���Ҫ����Ѱ�����������Ѱ�����뵱ǰ��ͷ���ڴŵ��Ų�ͬ������Ҫ����
 * Ѱ����������������ҪѰ����־seek�������������ִ�е���������command��
 */
	if (seek_track != current_track)
		seek = 1;
	sector++;								/* ������ʵ�����������Ǵ�1 ����	*/
	if (CURRENT->cmd == READ)				/* ������������Ƕ��������������̶������롣	*/
		command = FD_READ;
	else if (CURRENT->cmd == WRITE)			/* �������������д��������������д�����롣	*/
		command = FD_WRITE;
	else
		panic ("do_fd_request: unknown command");
/* ���������ú�TTfl22����"����ȫ�ֱ���ֵ֮�����ǿ��Կ�ʼִ������������ˡ��ò�
�����ö�ʱ������������ΪΪ���ܶ��������ж�д��������Ҫ����������������ﲢ�ﵽ��
����ת�ٶȡ�������Ҫһ����ʱ�䡣�����������ticks��to��floppy��on()������������ʱ
ʱ�䣬Ȼ��ʹ�ø���ʱ�趨һ����ʱ������ʱ�䵽ʱ�͵��ú���floppy��on��interrupt()��
*/
	add_timer (ticks_to_floppy_on (current_drive), &floppy_on_interrupt);
}

/* ����ϵͳ��ʼ����
 * �������̿��豸������Ĵ�����do��fd��requestO�������������ж��ţ�int 0x26����Ӧ
 * Ӳ���ж������ź�IRQ6����Ȼ��ȡ���Ը��ж��źŵ����Σ����������̿�����FDC������
 * �������źš��ж���������IDT�����������������ú�set��trap��gateO������ͷ�ļ�
 * include/asm/system.h �С�
 */
void
floppy_init (void)
{
/* ���������ж�����������floppy��interrupt(kernel/system��call.s��252��)�����жϴ�
 * ����̡��жϺ�Ϊint 0x26 (38)����Ӧ8259AоƬ�ж������ź�IRQ6��
 */
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;	/* = do_fd_request()��	*/
	set_trap_gate (0x26, &floppy_interrupt);/*���������ж��� int 0x26(38)��	*/
	outb (inb_p (0x21) & ~0x40, 0x21);		/* ��λ���̵��ж���������λ������	*/
											/* ���̿����������ж������źš�	*/
}
