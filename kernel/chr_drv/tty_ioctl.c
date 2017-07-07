/*
* linux/kernel/chr_drv/tty_ioctl.c
*
* (C) 1991 Linus Torvalds
*/

#include <errno.h>			/* �����ͷ�ļ�������ϵͳ�и��ֳ���š�(Linus ��minix ��������)	��	*/
#include <termios.h>		/* �ն������������ͷ�ļ�����Ҫ��������첽ͨ�ſڵ��ն˽ӿڡ�	*/
#include <linux/sched.h>	/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 �����ݣ�	*/
							/* ����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/
#include <linux/kernel.h>	/* �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣	*/
#include <linux/tty.h>		/* tty ͷ�ļ����������й�tty_io������ͨ�ŷ���Ĳ�����������	*/
#include <asm/io.h>			/* io ͷ�ļ�������Ӳ���˿�����/���������䡣	*/
#include <asm/segment.h>	/* �β���ͷ�ļ����������йضμĴ���������Ƕ��ʽ��ຯ����	*/
#include <asm/system.h>		/* ϵͳͷ�ļ������������û��޸�������/�ж��ŵȵ�Ƕ��ʽ���ꡣ	*/

/* ���ǲ������������飨���Ϊ�������飩���������벨�������ӵĶ�Ӧ��ϵ�μ��б��˵����
 * ���粨������2400bpsʱ����Ӧ��������48 (0x30);9600bps��������12(0xlc)��
 */
static unsigned short quotient[] = {
	0, 2304, 1536, 1047, 857,
	768, 576, 384, 192, 96,
	64, 48, 24, 12, 6, 3
};

/* �޸Ĵ��䲨���ʡ�
 * ������tty -�ն˶�Ӧ��tty���ݽṹ��
 * �ڳ��������־DLAB��λ����£�ͨ���˿� 0x3f8��0x3f9��UART�ֱ�д�벨�������ӵ�
 * �ֽں͸��ֽڡ�д����ٸ�λDLABλ�����ڴ��� 2���������˿ڷֱ���0x2f8��0x2f9��
 */
static void
change_speed (struct tty_struct *tty)
{
	unsigned short port, quot;

/* �������ȼ�����ttyָ�����ն��Ƿ��Ǵ����նˣ����������˳������ڴ����ն˵�tty��
 * ��������������data�ֶδ���Ŵ��ж˿ڻ�ַ��0x3f8��0x2f8������һ�����̨�ն˵�tty��
 * ����read_q.data�ֶ�ֵΪ0��Ȼ����ն�termios�ṹ�Ŀ���ģʽ��־����ȡ������
 * �õĲ����������ţ����ݴ˴Ӳ�������������quotient[]��ȡ�ö�Ӧ�Ĳ���������ֵquot��
 * CBAUD�ǿ���ģʽ��־���в�����λ�����롣
*/
	if (!(port = tty->read_q.data))
		return;
	quot = quotient[tty->termios.c_cflag & CBAUD];
/* ���ŰѲ���������quotд�봮�ж˿ڶ�ӦUARTоƬ�Ĳ����������������С���д֮ǰ����
 * ��Ҫ����·���ƼĴ���LCR�ĳ���������ʱ���λDLAB (λ7)��1��Ȼ���16λ�Ĳ���
 * �����ӵ͸��ֽڷֱ�д��˿� 0x3f8��0x3f9 (�ֱ��Ӧ���������ӵ͡����ֽ�������)��
 * ����ٸ�λLCR��DLAB��־λ��
 */
	cli ();							/* ���жϡ�	*/
	outb_p (0x80, port + 3);		/* set DLAB	�������ó���������־DLAB��	*/
	outb_p (quot & 0xff, port);		/* LS of divisor	������ӵ��ֽڡ�	*/
	outb_p (quot >> 8, port + 1);	/* MS of divisor	������Ӹ��ֽڡ�	*/
	outb (0x03, port + 3);			/* reset DLAB		��λDLAB��	*/
	sti ();							/* ���жϡ�	*/
}

/* ˢ��tty ������С�	*/
/* ������gueue - ָ���Ļ������ָ�롣	*/
/* �����е�ͷָ�����βָ�룬�Ӷ��ﵽ��ջ�����(���ַ�)��Ŀ�ġ�	*/
static void
flush (struct tty_queue *queue)
{
	cli ();
	queue->head = queue->tail;
	sti ();
}

/* �ȴ��ַ����ͳ�ȥ��	*/
static void
wait_until_sent (struct tty_struct *tty)
{
/* do nothing - not implemented	ʲô��û�� - ��δʵ�� */
}

/* ����BREAK ���Ʒ���	*/
static void
send_break (struct tty_struct *tty)
{
/* do nothing - not implemented	ʲô��û�� - ��δʵ�� */
}

/* ȡ�ն�termios �ṹ��Ϣ��	*/
/* ������tty - ָ���ն˵�tty �ṹָ�룻termios - �û�������termios �ṹ������ָ�롣	*/
/* ����0 ��	*/
static int
get_termios (struct tty_struct *tty, struct termios *termios)
{
	int i;

/* ������֤�û�������ָ����ָ�ڴ��������Ƿ��㹻���粻��������ڴ档Ȼ����ָ���ն�
 * ��termios�ṹ��Ϣ���û��������С���󷵻�0��*/
	verify_area (termios, sizeof (*termios));
	for (i = 0; i < (sizeof (*termios)); i++)
		put_fs_byte (((char *) &tty->termios)[i], i + (char *) termios);
	return 0;
}

/* �����ն�termios �ṹ��Ϣ��	*/
/* ������tty - ָ���ն˵�tty �ṹָ�룻termios - �û�������termios �ṹָ�롣	*/
/* ����0 ��	*/
static int
set_termios (struct tty_struct *tty, struct termios *termios)
{
	int i;

/* ���Ȱ��û���������termios�ṹ��Ϣ���Ƶ�ָ���ն�tty�ṹ��termios�ṹ�С���Ϊ��
 * ���п������޸����ն˴��пڴ��䲨���ʣ����������ٸ���termios�ṹ�еĿ���ģʽ��־
 * c_cflag�еĲ�������Ϣ�޸Ĵ���UARTоƬ�ڵĴ��䲨���ʡ���󷵻�0��
 */
	for (i = 0; i < (sizeof (*termios)); i++)
		((char *) &tty->termios)[i] = get_fs_byte (i + (char *) termios);
	change_speed (tty);
	return 0;
}

/* ��ȡtermio �ṹ�е���Ϣ��
 * ������tty -ָ���ն˵�tty�ṹָ�룻termio -����termio�ṹ��Ϣ���û���������
 * ����0��	*/
static int
get_termio (struct tty_struct *tty, struct termio *termio)
{
	int i;
	struct termio tmp_termio;

/* ������֤�û��Ļ�����ָ����ָ�ڴ��������Ƿ��㹻���粻��������ڴ档Ȼ��termios
 * �ṹ����Ϣ���Ƶ���ʱtermio�ṹ�С��������ṹ������ͬ�������롢��������ƺͱ���
 * ��־���������Ͳ�ͬ��ǰ�ߵ���long�������ߵ���short������ȸ��Ƶ���ʱtermio�ṹ
 * ��Ŀ����Ϊ�˽�����������ת����
*/
	verify_area (termio, sizeof (*termio));
/* ��termios �ṹ����Ϣ���Ƶ�termio �ṹ�С�Ŀ����Ϊ������ģʽ��־�������ͽ���ת����Ҳ��	*/
/* ��termios �ĳ���������ת��Ϊtermio �Ķ��������͡�	*/
	tmp_termio.c_iflag = tty->termios.c_iflag;
	tmp_termio.c_oflag = tty->termios.c_oflag;
	tmp_termio.c_cflag = tty->termios.c_cflag;
	tmp_termio.c_lflag = tty->termios.c_lflag;
/* ���ֽṹ��c_line ��c_cc[]�ֶ�����ȫ��ͬ�ġ�	*/
	tmp_termio.c_line = tty->termios.c_line;
	for (i = 0; i < NCC; i++)
		tmp_termio.c_cc[i] = tty->termios.c_cc[i];
/* /������ֽڵذ���ʱtermio�ṹ�е���Ϣ���Ƶ��û�termio�ṹ�������С�������0��	*/
	for (i = 0; i < (sizeof (*termio)); i++)
		put_fs_byte (((char *) &tmp_termio)[i], i + (char *) termio);
	return 0;
}

/*
* This only works as the 386 is low-byt-first
*/
/*
* �����termio ���ú�������386 ���ֽ���ǰ�ķ�ʽ�¿��á�
*/
/* �����ն�termio�ṹ��Ϣ��
 * ������tty -ָ���ն˵�tty�ṹָ�룻termio -�û���������termio�ṹ��
 * ���û�������termio����Ϣ���Ƶ��ն˵�termios�ṹ�С�����0��
 */
static int set_termio (struct tty_struct *tty, struct termio *termio)
{
	int i;
	struct termio tmp_termio;

/* ���ȸ����û���������termio�ṹ��Ϣ����ʱtermio�ṹ�С�Ȼ���ٽ�termio�ṹ����Ϣ
 * ���Ƶ�tty��termios�ṹ�С���������Ŀ����Ϊ�˶�����ģʽ��־�������ͽ���ת������ 
 * ��termio�Ķ���������ת����termios�ĳ��������͡������ֽṹ��c��line��c��cc[]�ֶ�
 * ����ȫ��ͬ�ġ�
 */
	for (i = 0; i < (sizeof (*termio)); i++)
		((char *) &tmp_termio)[i] = get_fs_byte (i + (char *) termio);
/* �ٽ�termio �ṹ����Ϣ���Ƶ�tty ��termios �ṹ�С�Ŀ����Ϊ������ģʽ��־�������ͽ���ת����	*/
/* Ҳ����termio �Ķ���������ת����termios �ĳ��������͡�	*/
	*(unsigned short *) &tty->termios.c_iflag = tmp_termio.c_iflag;
	*(unsigned short *) &tty->termios.c_oflag = tmp_termio.c_oflag;
	*(unsigned short *) &tty->termios.c_cflag = tmp_termio.c_cflag;
	*(unsigned short *) &tty->termios.c_lflag = tmp_termio.c_lflag;
/* ���ֽṹ��c_line ��c_cc[]�ֶ�����ȫ��ͬ�ġ�	*/
	tty->termios.c_line = tmp_termio.c_line;
	for (i = 0; i < NCC; i++)
		tty->termios.c_cc[i] = tmp_termio.c_cc[i];
/* �����Ϊ�û��п������޸����ն˴��пڴ��䲨���ʣ����������ٸ���termios�ṹ�еĿ���
 * ģʽ��־c_cflag�еĲ�������Ϣ�޸Ĵ���UARTоƬ�ڵĴ��䲨���ʣ�������0��
 */
	change_speed (tty);
	return 0;
}

/* tty�ն��豸����������ƺ�����
 * ������dev -�豸�ţ�cmd - ioctl���arg -��������ָ�롣
 * �ú������ȸ��ݲ����������豸���ҳ���Ӧ�ն˵�tty�ṹ��Ȼ����ݿ�������cmd�ֱ���д���
*/
int tty_ioctl (int dev, int cmd, int arg)
{
	struct tty_struct *tty;
/* ���ȸ����豸��ȡ��tty���豸�ţ��Ӷ�ȡ���ն˵�tty�ṹ�������豸����5(�����ն�)��
 * ����̵�tty�ֶμ���tty���豸�š���ʱ������̵�tty���豸���Ǹ����������ý���û��
 * �����նˣ������ܷ�����ioctl���ã�������ʾ������Ϣ��ͣ����������豸�Ų���5����4��
 * ���ǾͿ��Դ��豸����ȡ�����豸�š����豸�ſ�����0 (����̨�ն�)��1 (���� 1�ն�)��
 * 2(���� 2�ն�)��
 */
	if (MAJOR (dev) == 5)
	{
		dev = current->tty;
		if (dev < 0)
			panic ("tty_ioctl: dev<0");
/* ����ֱ�Ӵ��豸����ȡ�����豸�š�	*/
		}
	else
		dev = MINOR (dev);		/* ���豸�ſ�����0(����̨�ն�)��1(����1 �ն�)��2(����2 �ն�)��	*/

/* Ȼ��������豸�ű����ǿ�ȡ�ö�Ӧ�ն˵�tty�ṹ��������ttyָ���Ӧ���豸
 * �ŵ�tty�ṹ��Ȼ���ٸ��ݲ����ṩ��ioctl����cmd���зֱ���	*/

	tty = dev + tty_table;		/* ��tty ָ���Ӧ���豸�ŵ�tty �ṹ��	*/

	switch (cmd)				/* ����tty ��ioctl ������зֱ���	*/
		{
/* ȡ��Ӧ�ն�termios�ṹ��Ϣ����ʱ����arg���û�������ָ�롣	*/
		case TCGETS:
			return get_termios (tty, (struct termios *) arg);
/* ������termios�ṹ��Ϣ֮ǰ����Ҫ�ȵȴ�����������������ݴ�����ϣ�����ˢ�£���գ�
 * ������С��ٽ���ִ�����������ն�termios�ṹ�Ĳ�����	*/
		case TCSETSF:
			flush (&tty->read_q);	/* fallthroug	���ż���ִ��	*/
/* �������ն�termios ����Ϣ֮ǰ����Ҫ�ȵȴ�����������������ݴ�����(�ľ�)��	�����޸Ĳ���	*/
/* ��Ӱ����������������Ҫʹ��������ʽ��	*/
		case TCSETSW:
			wait_until_sent (tty);	/* fallthrough	���ż���ִ��	*/
/* ������Ӧ�ն�termios�ṹ��Ϣ����ʱ����arg�Ǳ���termios�ṹ���û�������ָ�롣	*/
		case TCSETS:
			return set_termios (tty, (struct termios *) arg);
/* ȡ��Ӧ�ն�terrnio�ṹ�е���Ϣ����ʱ����arg���û�������ָ�롣	*/
		case TCGETA:
			return get_termio (tty, (struct termio *) arg);
/* ������termio�ṹ��Ϣ֮ǰ����Ҫ�ȵȴ�����������������ݴ�����ϣ�����ˢ�£���գ�
 * ������С��ٽ���ִ�����������ն�termio�ṹ�Ĳ�����	*/
		case TCSETAF:
			flush (&tty->read_q);	/* fallthrough */
/* �������ն�termios����^ǰ����Ҫ�ȵȴ�����������������ݴ����꣨�ľ����������޸�
 * ������Ӱ����������������Ҫʹ��������ʽ��	*/
		case TCSETAW:
			wait_until_sent (tty);	/* fallthrough	����ִ�� */
/* ������Ӧ�ն�termio�ṹ��Ϣ����ʱ����arg�Ǳ���termio�ṹ���û�������ָ�롣	*/
		case TCSETA:
			return set_termio (tty, (struct termio *) arg);
/* �������argֵ��0����ȴ�������д�����ϣ��գ���������һ��break��	*/
		case TCSBRK:
			if (!arg)
			{
				wait_until_sent (tty);
				send_break (tty);
			}
			return 0;
/* ��ʼ/ֹͣ���ơ�	
 * �������ֵ��0������������
 * �����1����ָ�����������
 * �����2����������룻
 * �����3�������¿�����������롣
 */
		case TCXONC:

			return -EINVAL;		/* not implemented	δʵ�� */
		case TCFLSH:
/* ˢ����д�������û�з��͡������յ���û�ж������ݡ��������arg��0����ˢ��(���)
 * ������У������1����ˢ��������У������2����ˢ�������������С�
 */

			if (arg == 0)
				flush (&tty->read_q);
			else if (arg == 1)
				flush (&tty->write_q);
			else if (arg == 2)
			{
				flush (&tty->read_q);
				flush (&tty->write_q);
			}
			else
				return -EINVAL;
			return 0;
		case TIOCEXCL:			/* �����ն˴�����·ר��ģʽ��	*/
			return -EINVAL;		/* not implemented	δʵ�� */
		case TIOCNXCL:			/* ��λ�ն˴�����·ר��ģʽ��	*/
			return -EINVAL;		/* not implemented	δʵ�� */
		case TIOCSCTTY:			/* ����tty Ϊ�����նˡ�(TIOCNOTTY - ��ֹtty Ϊ�����ն�)��	*/
			return -EINVAL;		/* set controlling term NI	���ÿ����ն�NI */
		case TIOCGPGRP:		/* NI - Not Implemented��	*/
/* ��ȡ�ն��豸������š�������֤�û����������ȣ�Ȼ����tty��pgrp�ֶε��û���������
 * ��ʱ�����tg���û�������ָ�롣	*/
			verify_area ((void *) arg, 4);
			put_fs_long (tty->pgrp, (unsigned long *) arg);
			return 0;
		case TIOCSPGRP:
/* �����ն��豸�Ľ������pgrp����ʱ�����tg���û���������pgrp��ָ�롣	*/
			tty->pgrp = get_fs_long ((unsigned long *) arg);
			return 0;
		case TIOCOUTQ:
/* ������������л�δ�ͳ����ַ�����������֤�û����������ȣ�Ȼ���ƶ������ַ������û���
 * ��ʱ�����tg���û�������ָ�롣	*/
			verify_area ((void *) arg, 4);
			put_fs_long (CHARS (tty->write_q), (unsigned long *) arg);
			return 0;
		case TIOCINQ:
/* ������������л�δ��ȡ���ַ�����������֤�û����������ȣ�Ȼ���ƶ������ַ������û���
 * ��ʱ�����tg���û�������ָ�롣	*/
			verify_area ((void *) arg, 4);
			put_fs_long (CHARS (tty->secondary), (unsigned long *) arg);
			return 0;
		case TIOCSTI:
/* ģ���ն����롣��������һ��ָ���ַ���ָ����Ϊ����������װ���ַ������ն��ϼ���ġ�
 * �û������ڸÿ����ն��Ͼ��г����û�Ȩ�޻���ж����Ȩ�ޡ� */

			return -EINVAL;		/* not implemented	δʵ�� */
		case TIOCGWINSZ:		/* ��ȡ�ն��豸���ڴ�С��Ϣ���μ�termios.h �е�winsize �ṹ����	*/
			return -EINVAL;		/* not implemented	δʵ�� */
		case TIOCSWINSZ:		/* �����ն��豸���ڴ�С��Ϣ���μ�winsize �ṹ����	*/
			return -EINVAL;		/* not implemented	δʵ�� */
		case TIOCMGET:			/* ����modem ״̬�������ߵĵ�ǰ״̬����λ��־�����μ�termios.h ��185-196 �У���	*/
			return -EINVAL;		/* not implemented	δʵ�� */
		case TIOCMBIS:			/* ���õ���modem ״̬�������ߵ�״̬(true ��false)��	*/
			return -EINVAL;		/* not implemented	δʵ�� */
		case TIOCMBIC:			/* ��λ����modem ״̬�������ߵ�״̬��	*/
			return -EINVAL;		/* not implemented	δʵ�� */
		case TIOCMSET:			/* ����modem ״̬���ߵ�״̬�����ĳһ����λ��λ����modem ��Ӧ��״̬���߽���Ϊ��Ч��	*/
			return -EINVAL;		/* not implemented	δʵ�� */
		case TIOCGSOFTCAR:		/* ��ȡ����ز�����־(1 - ������0 - �ر�)��	*/
			return -EINVAL;		/* not implemented	δʵ�� */
		case TIOCSSOFTCAR:		/* ��������ز�����־(1 - ������0 - �ر�)��	*/
			return -EINVAL;		/* not implemented	δʵ�� */
		default:
			return -EINVAL;
		}
}
