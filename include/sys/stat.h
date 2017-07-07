#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include <sys/types.h>

struct stat
{
  dev_t st_dev;			/* �����ļ����豸�š�	*/
  ino_t st_ino;			/* �ļ�i �ڵ�š�	*/
  umode_t st_mode;		/* �ļ����ԣ������棩��	*/
  nlink_t st_nlink;		/* ָ���ļ�����������	*/
  uid_t st_uid;			/* �ļ����û�(��ʶ)�š�	*/
  gid_t st_gid;			/* �ļ�����š�	*/
  dev_t st_rdev;		/* �豸��(����ļ���������ַ��ļ�����ļ�)��	*/
  off_t st_size;		/* �ļ���С���ֽ�����������ļ��ǳ����ļ�����	*/
  time_t st_atime;		/* �ϴΣ���󣩷���ʱ�䡣	*/
  time_t st_mtime;		/* ����޸�ʱ�䡣	*/
  time_t st_ctime;		/* ���ڵ��޸�ʱ�䡣	*/
};

/* ����st_modeֵ�ķ������ƣ���Щֵ���ð˽��Ʊ�ʾ���μ���9���ļ�ϵͳ��ͼ9-5(i�ڵ�
 * �����ֶ�����)��Ϊ���ڼ��䣬��Щ�������ƾ�ΪһЩӢ�ĵ��ʵ�����ĸ����д��϶��ɡ���
 * ������S_IFMT��ÿ����ĸ�ֱ������State��Inode��File��Mask��Type;������S��IFREG
 * ����State��Inode��File��REGular������д��ĸ����ϣ�����S_IRWXU��State��Inode��
 * Read��Write��eXecute��User����ϡ����������ƿ��Դ����ơ�	*/
/* �ļ����ͣ�	*/
#define S_IFMT 00170000		/* �ļ����ͣ�8 ���Ʊ�ʾ����	*/
#define S_IFLNK	0120000
#define S_IFREG 0100000		/* �����ļ���	*/
#define S_IFBLK 0060000		/* �����⣨�豸���ļ��������dev/fd0��	*/
#define S_IFDIR 0040000		/* Ŀ¼�ļ���	*/
#define S_IFCHR 0020000		/* �ַ��豸�ļ���	*/
#define S_IFIFO 0010000		/* FIFO �����ļ���	*/

/* �ļ�����λ��	*/
/* S_ISUID���ڲ����ļ���set_user_ID��־�Ƿ���λ�����ñ�־��λ����ִ�и��ļ�ʱ�����̵�
 * ��Ч�û�ID��������Ϊ���ļ��������û�ID��S_ISGID���������ID������ͬ����	*/
#define S_ISUID 0004000		/* ִ��ʱ�����û�ID��set-user-ID����	*/
#define S_ISGID 0002000		/* ִ��ʱ������ID��	*/
#define S_ISVTX 0001000		/* ����Ŀ¼������ɾ����־��	*/

#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)	/* �����Ƿ񳣹��ļ���	*/
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)	/* �Ƿ�Ŀ¼�ļ���	*/
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)	/* �Ƿ��ַ��豸�ļ���	*/
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)	/* �Ƿ���豸�ļ���	*/
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)	/* �Ƿ�FIFO �����ļ���	*/

/* �ļ�����Ȩ�ޣ�	*/
#define S_IRWXU 00700		/* �������Զ���д��ִ��/���������������ĸ����User����	*/
#define S_IRUSR 00400		/* ��������ɣ����3����ĸ����User����	*/
#define S_IWUSR 00200		/* ����д��ɡ�	*/
#define S_IXUSR 00100		/* ����ִ��/������ɡ�	*/

#define S_IRWXG 00070		/* ���Ա���Զ���д��ִ��/���������������ĸ����Group����	*/
#define S_IRGRP 00040		/* ���Ա����ɣ����3����ĸ����Group����	*/
#define S_IWGRP 00020		/* ���Աд��ɡ�	*/
#define S_IXGRP 00010		/* ���Աִ��/������ɡ�	*/

#define S_IRWXO 00007		/* �����˶���д��ִ��/������ɣ����������ĸ0����Other����	*/
#define S_IROTH 00004		/* �����˶���ɣ����3����ĸ����Other����	*/
#define S_IWOTH 00002		/* ������д��ɡ�	*/
#define S_IXOTH 00001		/* ������ִ��/������ɡ�	*/

extern int chmod (const char *_path, mode_t mode);		/* �޸��ļ����ԡ�	*/
extern int fstat (int fildes, struct stat *stat_buf);	/* ȡָ���ļ�������ļ�״̬��Ϣ��	*/
extern int mkdir (const char *_path, mode_t mode);		/* ����Ŀ¼��	*/
extern int mkfifo (const char *_path, mode_t mode);		/* �����ܵ��ļ���	*/
extern int stat (const char *filename, struct stat *stat_buf);	/* ȡָ���ļ������ļ�״̬��Ϣ��	*/
extern mode_t umask (mode_t mask);						/* �������������롣	*/

#endif
