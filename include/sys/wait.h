#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include <sys/types.h>

#define _LOW(v) ( (v) & 0377)				/* ȡ���ֽڣ�8 ���Ʊ�ʾ����	*/
#define _HIGH(v) ( ((v) >> 8) & 0377)		/* ȡ���ֽڡ�	*/

/* options for waitpid, WUNTRACED not supported */
/* waitpid��ѡ�����WUNTRACEDδ��֧��	*/
/* [ע����ʵ0.11�ں��Ѿ�֧��WUNTRACEDѡ���������ע��Ӧ������ǰ�ں˰汾���������ġ�]
 * ���³��������Ǻ��� waitpid(pid_t pid, long *stat_addr��int options)�� options ʹ�õ�ѡ�
 */
#define WNOHANG		1							/* ���û��״̬Ҳ��Ҫ���𣬲����̷��ء�	*/
#define WUNTRACED	2							/* ����ִֹͣ�е��ӽ���״̬��	*/

#define WIFEXITED(s)	(!((s)&0xFF)			/* ����ӽ��������˳�����Ϊ�档	*/
#define WIFSTOPPED(s)	(((s)&0xFF)==0x7F)		/* ����ӽ�����ֹͣ�ţ���Ϊtrue��	*/
#define WEXITSTATUS(s)	(((s)>>8)&0xFF)			/* �����˳�״̬��	*/
#define WTERMSIG(s)		((s)&0x7F)				/* ���ص��½�����ֹ���ź�ֵ���ź�������	*/
#define WSTOPSIG(s)		(((s)>>8)&0xFF)			/* ���ص��½���ֹͣ���ź�ֵ��	*/
#define WIFSIGNALED(s)	(((unsigned int)(s)-1 & 0xFFFF) < 0xFF)	/* �������δ��׽���ź�
																 * �������ӽ����˳���Ϊ�档	*/

/* wait()��waitpitO����������̻�ȡ�����ӽ���֮һ��״̬��Ϣ������ѡ�������ȡ�Ѿ���ֹ��
 * ֹͣ���ӽ���״̬��Ϣ������������������������ӽ��̵�״̬��Ϣ���򱨸��˳���ǲ�ָ���ġ�
 * wait()������ǰ���̣�ֱ�����ӽ���֮һ�˳�����ֹ���������յ�Ҫ����ֹ�ý��̵��źţ�
 * ��������Ҫ����һ���źž�����źŴ�����򣩡�
 * waitpidO����ǰ���̣�ֱ��pidָ�����ӽ����˳�����ֹ�������յ�Ҫ����ֹ�ý��̵��źţ�
 * ��������Ҫ����һ���źž�����źŴ�����򣩡�
 * ���pid=-l��options=0,��waitpidO��������wait()����һ������������Ϊ����pid��options
 * �����Ĳ�ͬ����ͬ�����μ�kernel/exit.c��142��
 * ����pid�ǽ��̺ţ�*stat_loc�Ǳ���״̬��Ϣλ�õ�ָ�룻options�ǵȴ�ѡ�����10,11�С�
 */
pid_t wait (int *stat_loc);
pid_t waitpid (pid_t pid, int *stat_loc, int options);

#endif
