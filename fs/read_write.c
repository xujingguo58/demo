/*
* linux/fs/read_write.c
*
* (C) 1991 Linus Torvalds
*/

#include <sys/stat.h>		/* �ļ�״̬ͷ�ļ��������ļ����ļ�ϵͳ״̬�ṹstat{}�ͳ�����	*/
#include <errno.h>			/* �����ͷ�ļ�������ϵͳ�и��ֳ���š�(Linus ��minix ��������)��	*/
#include <sys/types.h>		/* ����ͷ�ļ��������˻�����ϵͳ�������͡�	*/
#include <linux/kernel.h>	/* �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣	*/
#include <linux/sched.h>	/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 �����ݣ�	*/
							/* ����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/
#include <asm/segment.h>	/* �β���ͷ�ļ����������йضμĴ���������Ƕ��ʽ��ຯ����	*/

/* �ַ��豸��д������fs/char��dev.c����95�С�	*/
extern int rw_char (int rw, int dev, char *buf, int count, off_t * pos);
/* ���ܵ�����������fs/pipe.c,��13�С�	*/
extern int read_pipe (struct m_inode *inode, char *buf, int count);
/* д�ܵ�����������fs/pipe.c����41�С�	*/
extern int write_pipe (struct m_inode *inode, char *buf, int count);
/* ���豸������������fs/block��dev.c����47�С�	*/
extern int block_read (int dev, off_t * pos, char *buf, int count);
/* ���豸д����������fs/block��dev.c����14�С�	*/
extern int block_write (int dev, off_t * pos, char *buf, int count);
/* ���ļ�����������fs/file��dev.c����17�С�	*/
extern int file_read (struct m_inode *inode, struct file *filp, char *buf, int count);
/* д�ļ�����������fs/file��dev.c����48�С�	*/
extern int file_write (struct m_inode *inode, struct file *filp, char *buf, int count);

/* �ض�λ�ļ���дָ��ϵͳ���á�
 * ����fd���ļ������offset���µ��ļ���дָ��ƫ��ֵ��origin��ƫ�Ƶ���ʼλ�ã���������ѡ��
 * SEEK��SET(0�����ļ���ʼ��)��SEEK��CURU���ӵ�ǰ��дλ�ã���SEEK��END(2�����ļ�β��)��	*/
int sys_lseek (unsigned int fd, off_t offset, int origin)
{
	struct file *file;
	int tmp;

/* �����жϺ����ṩ�Ĳ�����Ч�ԡ�����ļ����ֵ���ڳ��������ļ���NR��OPEN(20)��
 * ���߸þ�����ļ��ṹָ��Ϊ�գ����߶�Ӧ�ļ��ṹ��i�ڵ��ֶ�Ϊ�գ�����ָ���豸�ļ�
 * ָ���ǲ��ɶ�λ�ģ��򷵻س����벢�˳�������ļ���Ӧ��i�ڵ��ǹܵ��ڵ㣬�򷵻س���
 * ���˳�����Ϊ�ܵ�ͷβָ�벻�������ƶ���	*/
	if (fd >= NR_OPEN || !(file = current->filp[fd]) || !(file->f_inode)
			|| !IS_SEEKABLE (MAJOR (file->f_inode->i_dev)))
		return -EBADF;
	if (file->f_inode->i_pipe)
		return -ESPIPE;
/* Ȼ��������õĶ�λ��־���ֱ����¶�λ�ļ���дָ�롣	*/
	switch (origin)
		{
/* origin = SEEK��SET��Ҫ�����ļ���ʼ����Ϊԭ�������ļ���дָ�롣
 * ��ƫ��ֵС���㣬������ش����롣
 * ���������ļ���дָ�����offset��	*/
		case 0:
			if (offset < 0)
	return -EINVAL;
			file->f_pos = offset;
			break;
/* origin = SEEK����R��Ҫ�����ļ���ǰ��дָ�봦��Ϊԭ���ض�λ��дָ�롣
 * ����ļ���ǰָ�����ƫ��ֵС��0���򷵻س������˳���
 * �����ڵ�ǰ��дָ���ϼ���ƫ��ֵ��	*/
		case 1:
			if (file->f_pos + offset < 0)
	return -EINVAL;
			file->f_pos += offset;
			break;
/* origin = SEEK��END��Ҫ�����ļ�ĩβ��Ϊԭ���ض�λ��дָ�롣
 * ��ʱ���ļ���С����ƫ��ֵС�����򷵻س������˳���
 * �����ض�λ��дָ��Ϊ�ļ����ȼ���ƫ��ֵ��	*/
		case 2:
			if ((tmp = file->f_inode->i_size + offset) < 0)
	return -EINVAL;
			file->f_pos = tmp;
			break;
/* origin ���ó������س������˳���	*/
		default:
			return -EINVAL;
		}
	return file->f_pos;		/* ��󷵻��ض�λ����ļ���дָ��ֵ��	*/
}

/* ���ļ�ϵͳ���ú�����	*/
/* ����fd ���ļ������buf �ǻ�������count �������ֽ�����	*/
int
sys_read (unsigned int fd, char *buf, int count)
{
	struct file *file;
	struct m_inode *inode;

/* �������ȶԲ�����Ч�Խ����жϡ�����ļ����ֵ���ڳ��������ļ���NR��OPEN������
 * ��Ҫ��ȡ���ֽڼ���ֵС��0�����߸þ�����ļ��ṹָ��Ϊ�գ��򷵻س����벢�˳�����
 * ���ȡ���ֽ���count����0���򷵻�0�˳���	*/
	if (fd >= NR_OPEN || count < 0 || !(file = current->filp[fd]))
		return -EINVAL;
	if (!count)
		return 0;
/* Ȼ����֤������ݵĻ������ڴ����ơ���ȡ�ļ���i�ڵ㡣���ڸ��ݸ�i�ڵ�����ԣ��ֱ�
 * ������Ӧ�Ķ��������������ǹܵ��ļ��������Ƕ��ܵ��ļ�ģʽ������ж��ܵ�����������
 * ���򷵻ض�ȡ���ֽ��������򷵻س����룬�˳���������ַ����ļ�������ж��ַ��豸��
 * ���������ض�ȡ���ַ���������ǿ��豸�ļ�����ִ�п��豸�������������ض�ȡ���ֽ�����	*/
	verify_area (buf, count);
	inode = file->f_inode;
	if (inode->i_pipe)
		return (file->f_mode & 1) ? read_pipe (inode, buf, count) : -EIO;
	if (S_ISCHR (inode->i_mode))
		return rw_char (READ, inode->i_zone[0], buf, count, &file->f_pos);
	if (S_ISBLK (inode->i_mode))
		return block_read (inode->i_zone[0], &file->f_pos, buf, count);
/* �����Ŀ¼�ļ������ǳ����ļ�����������֤��ȡ�ֽ�^"unt���е���(����
 * ȡ�ֽ��������ļ���ǰ��дָ��ֵ�����ļ����ȣ����������ö�ȡ�ֽ���Ϊ�ļ�����-��ǰ
 * ��дָ��ֵ������ȡ������0���򷵻�0�˳�)��Ȼ��ִ���ļ������������ض�ȡ���ֽ���
 * ���˳���	*/
	if (S_ISDIR (inode->i_mode) || S_ISREG (inode->i_mode))
		{
			if (count + file->f_pos > inode->i_size)
	count = inode->i_size - file->f_pos;
			if (count <= 0)
	return 0;
			return file_read (inode, file, buf, count);
		}
/* ִ�е����˵�������޷��ж��ļ������ԡ����ӡ�ڵ��ļ����ԣ������س������˳���	*/
	printk ("(Read)inode->i_mode=%06o\n\r", inode->i_mode);
	return -EINVAL;
}

/* д�ļ�ϵͳ���á�
 * ����fd���ļ������buf���û���������count����д�ֽ�����	*/
int sys_write (unsigned int fd, char *buf, int count)
{
	struct file *file;
	struct m_inode *inode;

/* ͬ���أ����������жϺ�����������Ч�ԡ���������ļ����ֵ���ڳ��������ļ���
 * NR��0PEN��������Ҫд����ֽڼ���С��0�����߸þ�����ļ��ṹָ��Ϊ�գ��򷵻س���
 * �벢�˳���������ȡ���ֽ���count����0���򷵻�0�˳���	*/
	if (fd >= NR_OPEN || count < 0 || !(file = current->filp[fd]))
		return -EINVAL;
	if (!count)
		return 0;
/* Ȼ����֤������ݵĻ������ڴ����ơ���ȡ�ļ���i�ڵ㡣���ڸ��ݸ�i�ڵ�����ԣ��ֱ�
 * ������Ӧ�Ķ��������������ǹܵ��ļ���������д�ܵ��ļ�ģʽ�������д�ܵ�����������
 * ���򷵻�д����ֽ��������򷵻س������˳���������ַ��豸�ļ��������д�ַ��豸��
 * ��������д����ַ����˳�������ǿ��豸�ļ�������п��豸д������������д����ֽ�
 * ���˳������ǳ����ļ�����ִ���ļ�д������������д����ֽ������˳���	*/
	inode = file->f_inode;
	if (inode->i_pipe)
		return (file->f_mode & 2) ? write_pipe (inode, buf, count) : -EIO;
	if (S_ISCHR (inode->i_mode))
		return rw_char (WRITE, inode->i_zone[0], buf, count, &file->f_pos);
	if (S_ISBLK (inode->i_mode))
		return block_write (inode->i_zone[0], &file->f_pos, buf, count);
	if (S_ISREG (inode->i_mode))
		return file_write (inode, file, buf, count);
/* ִ�е����˵�������޷��ж��ļ�����'�����ӡ�ļ����ԣ������س������˳���	*/
	printk ("(Write)inode->i_mode=%06o\n\r", inode->i_mode);
	return -EINVAL;
}
