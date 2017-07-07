/*
* linux/fs/ioctl.c
*
* (C) 1991 Linus Torvalds
*/

#include <string.h>			/* �ַ���ͷ�ļ�����Ҫ������һЩ�й��ַ���������Ƕ�뺯����	*/
#include <errno.h>			/* �����ͷ�ļ�������ϵͳ�и��ֳ���š�(Linus ��minix ��������)��	*/
#include <sys/stat.h>		/* �ļ�״̬ͷ�ļ��������ļ����ļ�ϵͳ״̬�ṹstat{}�ͳ�����	*/

#include <linux/sched.h>	/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 �����ݣ�	*/
							/* ����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/

extern int tty_ioctl (int dev, int cmd, int arg);	/* �ն�ioctl(chr_drv/tty_ioctl.c, 115)��	*/

/* ���������������(ioctl)����ָ�롣	*/
typedef int (*ioctl_ptr) (int dev, int cmd, int arg);

/* ȡϵͳ���豸�����ĺꡣ	*/
#define NRDEVS ((sizeof (ioctl_table))/(sizeof (ioctl_ptr)))

/* ioctl ��������ָ���	*/
static ioctl_ptr ioctl_table[] = {
	NULL,		/* nodev */
	NULL,		/* /dev/mem */
	NULL,		/* /dev/fd */
	NULL,		/* /dev/hd */
	tty_ioctl,	/* /dev/ttyx */
	tty_ioctl,	/* /dev/tty */
	NULL,		/* /dev/lp */
	NULL
};				/* named pipes */


/* ϵͳ���ú���-����������ƺ�����
 * �ú��������жϲ����������ļ��������Ƿ���Ч��Ȼ����ݶ�Ӧi�ڵ����ļ������ж��ļ�
 * ���ͣ������ݾ����ļ����͵�����صĴ�������
 * ������fd -�ļ���������cmd -�����룻arg -������
 * ���أ��ɹ��򷵻�0�����򷵻س����롣
	*/
int
sys_ioctl (unsigned int fd, unsigned int cmd, unsigned long arg)
{
	struct file *filp;
	int dev, mode;

/* �����жϸ������ļ�����������Ч�ԡ�����ļ������������ɴ򿪵��ļ��������߶�Ӧ����
 * �����ļ��ṹָ��Ϊ�գ��򷵻س������˳���	*/
	if (fd >= NR_OPEN || !(filp = current->filp[fd]))
		return -EBADF;
/* �����ȡ��Ӧ�ļ������ݴ��ж��ļ������͡�������ļ��Ȳ����ַ��豸�ļ���Ҳ��
 * �ǿ��豸�ļ����򷵻س������˳��������ַ�����豸�ļ�������ļ���i�ڵ���ȡ�豸�š�
 * ����豸�Ŵ���ϵͳ���е��豸�����򷵻س���š�	*/
	mode = filp->f_inode->i_mode;
	if (!S_ISCHR (mode) && !S_ISBLK (mode))
		return -EINVAL;
	dev = filp->f_inode->i_zone[0];
	if (MAJOR (dev) >= NRDEVS)
		return -ENODEV;
/* Ȼ�����10���Ʊ�ioctl_table��ö�Ӧ�豸��ioctl����ָ�룬�����øú������������
 * ����ioctl����ָ�����û�ж�Ӧ�������򷵻س����롣	*/
	if (!ioctl_table[MAJOR (dev)])
		return -ENOTTY;
	return ioctl_table[MAJOR (dev)] (dev, cmd, arg);
}
