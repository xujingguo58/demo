/*
 *  linux/lib/_exit.c
 *
 *  (C) 1991  Linus Torvalds
 */
#define __LIBRARY__		/* ����һ�����ų�����������˵����	*/
#include <unistd.h>		/* Linux ��׼ͷ�ļ��������˸��ַ��ų��������ͣ��������˸��ֺ�����	*/
						/* �綨����__LIBRARY__���򻹰���ϵͳ���úź���Ƕ���_syscall0()�ȡ�	*/

/* �ں�ʹ�õĳ���(�˳�)��ֹ������
 * ֱ�ӵ���ϵͳ�ж�int 0x80�����ܺš�NR��exit��
 * ������exit��code ���˳��롣
 * ������ǰ�Ĺؼ���volatile���ڸ��߱�����gcc�ú������᷵�ء���������gcc��������һ
 * Щ�Ĵ��룬����Ҫ����ʹ������ؼ��ֿ��Ա������ĳЩ��δ��ʼ�������ģ��پ�����Ϣ��
 * ��ͬ�� gcc �ĺ�������˵����void do_exit(int error��code) _attribute_ ((noreturn));
 */
volatile void _exit(int exit_code)
{
	/* %0 - eax(ϵͳ���ú�__NR_exit)��	*/
	/* %1 - ebx(�˳���exit_code)��	*/
	__asm__("int $0x80"::"a" (__NR_exit),"b" (exit_code));
}
