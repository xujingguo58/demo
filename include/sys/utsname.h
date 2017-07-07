#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

#include <sys/types.h>	/* ����ͷ�ļ��������˻�����ϵͳ�������͡�	*/

struct utsname
{
  char sysname[9];		/* ��ǰ����ϵͳ�����ơ�	*/
  char nodename[9];		/* ��ʵ����ص������нڵ����ƣ��������ƣ���	*/
  char release[9];		/* ������ϵͳʵ�ֵĵ�ǰ���м���	*/
  char version[9];		/* ���η��еĲ���ϵͳ�汾����	*/
  char machine[9];		/* ϵͳ���е�Ӳ���������ơ�	*/
};

extern int uname (struct utsname *utsbuf);

#endif
