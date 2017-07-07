/*
* linux/fs/open.c
*
* (C) 1991 Linus Torvalds
*/

#include <string.h>			/* �ַ���ͷ�ļ�����Ҫ������һЩ�й��ַ���������Ƕ�뺯����	*/
#include <errno.h>			/* �����ͷ�ļ�������ϵͳ�и��ֳ���š�(Linus ��minix ��������)��	*/
#include <fcntl.h>			/* �ļ�����ͷ�ļ��������ļ������������Ĳ������Ƴ������ŵĶ��塣	*/
#include <sys/types.h>		/* ����ͷ�ļ��������˻�����ϵͳ�������͡�	*/
#include <utime.h>			/* �û�ʱ��ͷ�ļ��������˷��ʺ��޸�ʱ��ṹ�Լ�utime()ԭ�͡�	*/
#include <sys/stat.h>		/* �ļ�״̬ͷ�ļ��������ļ����ļ�ϵͳ״̬�ṹstat{}�ͳ�����	*/
#include <linux/sched.h>	/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 �����ݣ�	*/
							/* ����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/
#include <linux/tty.h>		/* tty ͷ�ļ����������й�tty_io������ͨ�ŷ���Ĳ�����������	*/
#include <linux/kernel.h>	/* �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣	*/
#include <asm/segment.h>	/* �β���ͷ�ļ����������йضμĴ���������Ƕ��ʽ��ຯ����	*/

/* ȡ�ļ�ϵͳ��Ϣ��
 * ����dev�Ǻ����Ѱ�װ�ļ�ϵͳ���豸�š�ubuf��һ��ustat�ṹ������ָ�룬���ڴ��
 * ϵͳ���ص��ļ�ϵͳ��Ϣ����ϵͳ�������ڷ����Ѱ�װ��mounted���ļ�ϵͳ��ͳ����Ϣ��
 * �ɹ�ʱ����0������ubufָ���ustate�ṹ�������ļ�ϵͳ�ܿ��п����Ϳ���i�ڵ�����
 * ustat �ṹ������ include/sys/types.h �С�	*/
int sys_ustat (int dev, struct ustat *ubuf)
{
	return -ENOSYS;							/* �����룺���ܻ�δʵ�֡�	*/
}

/* �����ļ����ʺ��޸�ʱ�䡣
 * ����filename ���ļ�����times �Ƿ��ʺ��޸�ʱ��ṹָ�롣
 * ��� times ָ�벻ΪNULL����ȡutimbuf �ṹ�е�ʱ����Ϣ�������ļ��ķ��ʺ��޸�ʱ�䡣
 * ��� times ָ����NULL����ȡϵͳ��ǰʱ��������ָ���ļ��ķ��ʺ��޸�ʱ����	*/
int sys_utime (char *filename, struct utimbuf *times)
{
	struct m_inode *inode;
	long actime, modtime;

/* �ļ���ʱ����Ϣ��������i�ڵ��ӡ�����������ȸ����ļ���ȡ�ö�Ӧi�ڵ㡣���û����
 * �����򷵻س����롣	*/
	if (!(inode = namei (filename)))
		return -ENOENT;
/* ����ṩ�ķ��ʺ��޸�ʱ��ṹָ��times��ΪNULL����ӽṹ�ж�ȡ�û����õ�ʱ��ֵ��
 * �������ϵͳ��ǰʱ���������ļ��ķ��ʺ��޸�ʱ�䡣	*/
	if (times)
		{
			actime = get_fs_long ((unsigned long *) &times->actime);
			modtime = get_fs_long ((unsigned long *) &times->modtime);
		}
	else
		actime = modtime = CURRENT_TIME;
/* Ȼ���޸�i�ڵ��еķ���ʱ���ֶκ��޸�ʱ���ֶΡ�������i�ڵ����޸ı�־���Żظ�i�ڵ㣬������0��	*/
	inode->i_atime = actime;
	inode->i_mtime = modtime;
	inode->i_dirt = 1;
	iput (inode);
	return 0;
}

/*
* XXX should we use the real or effective uid? BSD uses the real uid,
* so as to make this call useful to setuid programs.
*/
/*
* �ļ�����XXX�����Ǹ�����ʵ�û�id ������Ч�û�id��BSD ϵͳʹ������ʵ�û�id��
* ��ʹ�õ��ÿ��Թ�setuid ����ʹ�á�
* ��ע��POSIX ��׼����ʹ����ʵ�û�ID��
*/
/* ����ļ��ķ���Ȩ�ޡ�
 * ����filename���ļ�����mode�Ǽ��ķ������ԣ�����3����Ч����λ��ɣ�R_0K(ֵ4)��
 * W_0K(2)��X_0K(1)��F_OK(0)��ɣ��ֱ��ʾ����ļ��Ƿ�ɶ�����д����ִ�к��ļ���
 * ����ڡ������������Ļ����򷵻�0�����򷵻س����롣	*/

int sys_access (const char *filename, int mode)
{
	struct m_inode *inode;
	int res, i_mode;

/* �ļ��ķ���Ȩ����ϢҲͬ���������ļ���i�ڵ�ṹ�У��������Ҫ��ȡ�ö�Ӧ�ļ�����i
 * �ڵ㡣���ķ�������mode�ɵ�3λ��ɣ������Ҫ���ϰ˽���0007��������и߱���λ��
 * ����ļ�����Ӧ��i�ڵ㲻���ڣ��򷵻�û�����Ȩ�޳����롣��i�ڵ���ڣ���ȡi�ڵ�
 * ���ļ������룬���Żظ�i�ڵ㡣���⣬56������䡰iput(inode);��������60��֮��	*/
	mode &= 0007;
	if (!(inode = namei (filename)))
		return -EACCES;
	i_mode = res = inode->i_mode & 0777;
	iput (inode);
/* �����ǰ�����û��Ǹ��ļ�����������ȡ�ļ��������ԡ����������ǰ�����û�����ļ���
 * ��ͬ��һ�飬��ȡ�ļ������ԡ����򣬴�ʱres���3�����������˷��ʸ��ļ���������ԡ�
 * [[??����Ӧ res ?3 ??]	*/
	if (current->uid == inode->i_uid)
		res >>= 6;
	else if (current->gid == inode->i_gid)
		res >>= 6;
/* ��ʱres�����3�����Ǹ��ݵ�ǰ�����û����ļ��Ĺ�ϵѡ������ķ�������λ����������
 * ���ж���3���ء�����ļ����Ծ��в�������ѯ������λmode���������ɣ�����0��	*/
	if ((res & 0007 & mode) == mode)
		return 0;
/*
* XXX we are doing this test last because we really should be
* swapping the effective with the real user id (temporarily),
* and then calling suser() routine. If we do call the
* suser() routine, it needs to be called last.
*/
/*
* XXX ��������������Ĳ��ԣ���Ϊ����ʵ������Ҫ������Ч�û�id ��
* ��ʵ�û�id����ʱ�أ���Ȼ��ŵ���suser()������
* �������ȷʵҪ����
* suser()����������Ҫ���ű����á�
*/
/* �����ǰ�û�IDΪ0(�����û�)����������ִ��λ��0�����ļ����Ա��κ���ִ�С���
 * �����򷵻�0�����򷵻س����롣	*/
	if ((!current->uid) && (!(mode & 1) || (i_mode & 0111)))
		return 0;
	return -EACCES;
}

/* �ı䵱ǰ����Ŀ¼ϵͳ���ú�����	*/
/* ����filename ��Ŀ¼����	*/
/* �����ɹ��򷵻�0�����򷵻س����롣	*/
int
sys_chdir (const char *filename)
{
	struct m_inode *inode;

/* �ı䵱ǰ����Ŀ¼����Ҫ��ѽ�������ṹ�ĵ�ǰ����Ŀ¼�ֶ�ָ�����Ŀ¼����i�ڵ㡣
 * �����������ȡĿ¼����i�ڵ㡣���Ŀ¼����Ӧ��i�ڵ㲻���ڣ��򷵻س����롣�����
 * i�ڵ㲻��һ��Ŀ¼i�ڵ㣬��Żظ�i�ڵ㣬�����س����롣	*/
	if (!(inode = namei (filename)))
		return -ENOENT;					/* �����룺�ļ���Ŀ¼�����ڡ�	*/
	if (!S_ISDIR (inode->i_mode))
		{
			iput (inode);
			return -ENOTDIR;			/* �����룺����Ŀ¼����	*/
		}
/* Ȼ���ͷŵ�ǰ����ԭ����Ŀ¼i �ڵ㣬��ָ������õĹ���Ŀ¼i �ڵ㡣����0��	*/
	iput (current->pwd);
	current->pwd = inode;
	return (0);
}

/* �ı��Ŀ¼ϵͳ���ú�����
 * ��ָ����Ŀ¼�����ó�Ϊ��ǰ���̵ĸ�Ŀ¼��/����
 * ��������ɹ��򷵻�0�����򷵻س����롣	*/
int sys_chroot (const char *filename)
{
	struct m_inode *inode;

/* �õ������ڸı䵱ǰ��������ṹ�еĸ�Ŀ¼�ֶ�root������ָ���������Ŀ¼����i�ڵ㡣
//���Ŀ¼����Ӧ��i�ڵ㲻���ڣ��򷵻س����롣�����i�ڵ㲻��Ŀ¼i�ڵ㣬��Żظ� // i�ڵ㣬Ҳ���س����롣	*/
	if (!(inode = namei (filename)))
		return -ENOENT;
/* �����i �ڵ㲻��Ŀ¼��i �ڵ㣬���ͷŸýڵ㣬���س����롣	*/
	if (!S_ISDIR (inode->i_mode))
		{
			iput (inode);
			return -ENOTDIR;
		}
/* �ͷŵ�ǰ���̵ĸ�Ŀ¼i �ڵ㣬������Ϊ����ָ��Ŀ¼����i �ڵ㣬����0��	*/
	iput (current->root);
	current->root = inode;
	return (0);
}

/* �޸��ļ�����ϵͳ���á�
 * ����filename���ļ�����mode���µ��ļ����ԡ�
 * �������ɹ��򷵻�0�����򷵻س����롣	*/
int sys_chmod (const char *filename, int mode)
{
	struct m_inode *inode;

/* �õ���Ϊָ���ļ������µķ�������mode���ļ��ķ����������ļ�����Ӧ��i�ڵ��У����
 * ��������ȡ�ļ�����Ӧ��i�ڵ㡣���i�ڵ㲻���ڣ��򷵻س����루�ļ���Ŀ¼�����ڣ���
 * �����ǰ���̵���Ч�û�id���ļ�i�ڵ���û�id��ͬ������Ҳ���ǳ����û�����Żظ�
 * �ļ�i�ڵ㣬���س����루û�з���Ȩ�ޣ���	*/
	if (!(inode = namei (filename)))
		return -ENOENT;
	if ((current->euid != inode->i_uid) && !suser ())
		{
			iput (inode);
			return -EACCES;
		}
/* ������������i �ڵ���ļ����ԣ����ø�i �ڵ����޸ı�־���ͷŸ�i �ڵ㣬����0��	*/
	inode->i_mode = (mode & 07777) | (inode->i_mode & ~07777);
	inode->i_dirt = 1;
	iput (inode);
	return 0;
}

/* �޸��ļ�����ϵͳ���á�
 * ����filename���ļ�����uid���û���ʶ��(�û�ID)��gid����ID��
 * �������ɹ��򷵻�0�����򷵻س����롣	*/
int sys_chown (const char *filename, int uid, int gid)
{
	struct m_inode *inode;

/* �õ������������ļ�i�ڵ��е��û�����ID���������Ҫȡ�ø����ļ�����i�ڵ㡣�����
 * ������i�ڵ㲻���ڣ��򷵻س����루�ļ���Ŀ¼�����ڣ��������ǰ���̲��ǳ����û���
 * ��Żظ�i�ڵ㣬�����س����루û�з���Ȩ�ޣ���	*/
	if (!(inode = namei (filename)))
		return -ENOENT;
	if (!suser ())
	{
		iput (inode);
		return -EACCES;
	}
/* �������Ǿ��ò����ṩ��ֵ�������ļ�i�ڵ���û�ID����ID������i�ڵ��Ѿ��޸ı�־��
 * �Żظ�i�ڵ㣬����0��	*/
	inode->i_uid = uid;
	inode->i_gid = gid;
	inode->i_dirt = 1;
	iput (inode);
	return 0;
}

/* �򿪣��򴴽����ļ�ϵͳ���á�
 * ����filename���ļ�����flag�Ǵ��ļ���־������ȡֵ��0_RD0NLY (ֻ��)��0_WR0NLY
 * (ֻд)��0_RDWR (��д)���Լ�0_CREAT (����)��0_EXCL (�������ļ����벻����)��
 * 0_APPEND (���ļ�β�������)������һЩ��־����ϡ���������ô�����һ�����ļ�����
 * mode������ָ���ļ���������ԡ���Щ������S_IRWXU (�ļ��������ж���д��ִ��Ȩ��)��
 * S_IRUSR (�û����ж��ļ�Ȩ��)��S_IRWXG(���Ա���ж���д��ִ��Ȩ��)�ȵȡ�������
 * �������ļ�����Щ����ֻӦ���ڽ������ļ��ķ��ʣ�������ֻ���ļ��Ĵ򿪵���Ҳ������һ
 * ���ɶ�д���ļ������������ò����ɹ����򷵻��ļ����(�ļ�������)�����򷵻س����롣
 * �μ� sys/stat.h��fcntl.h��	*/
int sys_open (const char *filename, int flag, int mode)
{
	struct m_inode *inode;
	struct file *f;
	int i, fd;

/* ���ȶԲ������д������û����õ��ļ�ģʽ�ͽ���ģʽ���������룬������ɵ��ļ�ģʽ��
 * Ϊ��Ϊ���ļ�����һ���ļ��������Ҫ�������̽ṹ���ļ��ṹָ�����飬�Բ���һ����
 * ����������������fd���Ǿ��ֵ�����Ѿ�û�п�����򷵻س����루������Ч����	*/
	mode &= 0777 & ~current->umask;
	for (fd = 0; fd < NR_OPEN; fd++)
		if (!current->filp[fd])					/* �ҵ������	*/
			break;
	if (fd >= NR_OPEN)
		return -EINVAL;
/* Ȼ���������õ�ǰ���̵�ִ��ʱ�ر��ļ����(close_on_exec)λͼ����λ��Ӧ�ı���λ��
 * close_on_exec��һ�����������ļ������λͼ��־��ÿ������λ����һ�����ŵ��ļ���
 * ����������ȷ���ڵ���ϵͳ����execveOʱ��Ҫ�رյ��ļ������������ʹ��fork()����
 * ������һ���ӽ���ʱ��ͨ�����ڸ��ӽ����е���execveO��������ִ����һ���³��򡣴�ʱ
 * �ӽ����п�ʼִ���³�����һ���ļ������close_on_exec�еĶ�Ӧ����λ����λ����ô
 * ��ִ��execveOʱ�ö�Ӧ�ļ���������رգ�������ļ������ʼ�մ��ڴ�״̬������
 * һ���ļ�ʱ��Ĭ��������ļ�������ӽ�����Ҳ���ڴ�״̬���������Ҫ��λ��Ӧ����λ��	*/
	current->close_on_exec &= ~(1 << fd);
/* Ȼ��Ϊ���ļ����ļ�����Ѱ��һ�����нṹ�������fָ���ļ������鿪ʼ����������
 * ���ļ��ṹ����ü���Ϊ0��������Ѿ�û�п����ļ���ṹ��򷵻س����롣
 * ���⣬�� 151 ���ϵ�ָ�븳ֵ ��0+file_table ����ͬ�ڡ�file_table����
 * ��&file_table[0]������������д���ܸ�������һЩ��	*/
	f = 0 + file_table;
	for (i = 0; i < NR_FILE; i++, f++)
		if (!f->f_count)
			break;
	if (i >= NR_FILE)
		return -EINVAL;
/* ��ʱ�����ý��̶�Ӧ�ļ����fd���ļ��ṹָ��ָ�����������ļ��ṹ�������ļ����ü���
 * ����1��Ȼ����ú���open_nameiOִ�д򿪲�����������ֵС��0����˵�����������ͷ�
 * �����뵽���ļ��ṹ�����س�����i�����ļ��򿪲����ɹ�����inode���Ѵ��ļ���i�ڵ�
 * ָ�롣	*/
	(current->filp[fd] = f)->f_count++;
	if ((i = open_namei (filename, flag, mode, &inode)) < 0)
		{
			current->filp[fd] = NULL;
			f->f_count = 0;
			return i;
		}
/* �����Ѵ��ļ���i�ڵ�������ֶΣ����ǿ���֪���ļ��ľ������͡����ڲ�ͬ���͵��ļ���
 * ������Ҫ��һЩ�ر�Ĵ�������򿪵����ַ��豸�ļ�����ô���������豸����4���ַ���
 * ��(����/dev/ttyO)�������ǰ�����ǽ��������첢�ҵ�ǰ���̵�tty�ֶ�С��0(û���ն�)��
 * �����õ�ǰ���̵�tty��Ϊ��i�ڵ�����豸�ţ������õ�ǰ����tty��Ӧ��tty����ĸ���
 * ����ŵ��ڵ�ǰ���̵Ľ�����š���ʾΪ�ý����飨�Ự�ڣ���������նˡ��������豸����
 * 5���ַ��ļ���/dev/tty��������ǰ����û��tty����˵���������ǷŻ�i�ڵ�����뵽��
 * �ļ��ṹ�����س����루����ɣ���
 * (ע����δ����������)��*/
		
/* ttys are somewhat special (ttyxx major==4, tty major==5)	*/
/* ttys ��Щ���⣨ttyxx ����==4��tty ����==5��	*/

	if (S_ISCHR (inode->i_mode))
		if (MAJOR (inode->i_zone[0]) == 4)
		{
			if (current->leader && current->tty < 0)
			{
				current->tty = MINOR (inode->i_zone[0]);
				tty_table[current->tty].pgrp = current->pgrp;
			}
		}
		else if (MAJOR (inode->i_zone[0]) == 5)
			if (current->tty < 0)
			{
				iput (inode);
				current->filp[fd] = NULL;
				f->f_count = 0;
				return -EPERM;
			}
/* Likewise with block-devices: check for floppy_change */
/* ͬ�����ڿ��豸�ļ�����Ҫ�����Ƭ�Ƿ񱻸���*/
/* ����򿪵��ǿ��豸�ļ���������Ƭ�Ƿ��������������������Ҫ�ø��ٻ������и��豸
 * �����л����ʧЧ��	*/
	if (S_ISBLK (inode->i_mode))
		check_disk_change (inode->i_zone[0]);
/* �������ǳ�ʼ�����ļ����ļ��ṹ�������ļ��ṹ���Ժͱ�־���þ�����ü���Ϊ1����
 * ����i�ڵ��ֶ�Ϊ���ļ���i�ڵ㣬��ʼ���ļ���дָ��Ϊ0����󷵻��ļ�����š�	*/
	f->f_mode = inode->i_mode;
	f->f_flags = flag;
	f->f_count = 1;
	f->f_inode = inode;
	f->f_pos = 0;
	return (fd);
}

/* �����ļ�ϵͳ���ú�����	*/
/* ����pathname ��·������mode �������sys_open()������ͬ��	*/
/* �ɹ��򷵻��ļ���������򷵻س����롣	*/
int
sys_creat (const char *pathname, int mode)
{
	return sys_open (pathname, O_CREAT | O_TRUNC, mode);
}

/* �ر��ļ�ϵͳ���ú�����	*/
/* ����fd ���ļ������	*/
/* �ɹ��򷵻�0�����򷵻س����롣	*/
int
sys_close (unsigned int fd)
{
	struct file *filp;

/* ���ȼ�������Ч�ԡ����������ļ����ֵ���ڳ���ͬʱ�ܴ򿪵��ļ���NR��OPEN���򷵻�
 * �����루������Ч����Ȼ��λ���̵�ִ��ʱ�ر��ļ����λͼ��Ӧλ�������ļ������Ӧ
 * ���ļ��ṹָ����NULL���򷵻س����롣	*/
	if (fd >= NR_OPEN)
		return -EINVAL;
	current->close_on_exec &= ~(1 << fd);
	if (!(filp = current->filp[fd]))
		return -EINVAL;
/* �����ø��ļ�������ļ��ṹָ��ΪNULL�����ڹر��ļ�֮ǰ����Ӧ�ļ��ṹ�еľ������
 * �����Ѿ�Ϊ0����˵���ں˳���ͣ�������򽫶�Ӧ�ļ��ṹ�����ü�����1����ʱ�������
 * ��Ϊ0����˵����������������ʹ�ø��ļ������Ƿ���0 (�ɹ�)��������ü����ѵ���0��
 * ˵�����ļ��Ѿ�û�н������ã����ļ��ṹ�ѱ�Ϊ���С����ͷŸ��ļ�i�ڵ㣬����0��	*/
	current->filp[fd] = NULL;
	if (filp->f_count == 0)
		panic ("Close: file count is 0");
	if (--filp->f_count)
		return (0);
	iput (filp->f_inode);
	return (0);
}
