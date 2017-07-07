#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <sys/types.h>			/* ����ͷ�ļ��������˻�����ϵͳ�������͡�	*/

typedef int sig_atomic_t;		/* �����ź�ԭ�Ӳ������͡�	*/
typedef unsigned int sigset_t;	/* 32 bits */	/* �����źż����͡�	*/

#define _NSIG 32				/* �����ź����� -- 32 �֡�	*/
#define NSIG _NSIG				/* NSIG = _NSIG	*/

/* ������Щ��Linux0.11�ں��ж�����źš����а����� P0SIX.1Ҫ�������20���źš�	*/
#define SIGHUP		1			/* Hang Up		-- �ҶϿ����ն˻���̡�	*/
#define SIGINT		2			/* Interrupt		-- ���Լ��̵��жϡ�	*/
#define SIGQUIT		3			/* Quit			-- ���Լ��̵��˳���	*/
#define SIGILL		4			/* Illeagle		-- �Ƿ�ָ�	*/
#define SIGTRAP		5			/* Trap			-- ���ٶϵ㡣	*/
#define SIGABRT		6			/* Abort		-- �쳣������	*/
#define SIGIOT		6			/* IO Trap		-- ͬ�ϡ�	*/
#define SIGUNUSED	7			/* Unused		-- û��ʹ�á�	*/
#define SIGFPE		8			/* FPE			-- Э����������	*/
#define SIGKILL		9			/* Kill			-- ǿ�Ƚ�����ֹ��	*/
#define SIGUSR1		10			/* User1		-- �û��ź�1�����̿�ʹ�á�	*/
#define SIGSEGV		11			/* Segment Violation -- ��Ч�ڴ����á�	*/
#define SIGUSR2		12			/* User2		-- �û��ź�2�����̿�ʹ�á�	*/
#define SIGPIPE		13			/* Pipe			-- �ܵ�д�����޶��ߡ�	*/
#define SIGALRM		14			/* Alarm		-- ʵʱ��ʱ��������	*/
#define SIGTERM		15			/* Terminate	-- ������ֹ��	*/
#define SIGSTKFLT	16			/* Stack Fault	-- ջ����Э����������	*/
#define SIGCHLD		17			/* Child		-- �ӽ���ֹͣ����ֹ��	*/
#define SIGCONT		18			/* Continue		-- �ָ����̼���ִ�С�	*/
#define SIGSTOP		19			/* Stop			-- ֹͣ���̵�ִ�С�	*/
#define SIGTSTP		20			/* TTY Stop		-- tty ����ֹͣ���̣��ɺ��ԡ�	*/
#define SIGTTIN		21			/* TTY In		-- ��̨�����������롣	*/
#define SIGTTOU		22			/* TTY Out		-- ��̨�������������	*/

/* Ok, I haven't implemented sigactions, but trying to keep headers POSIX */
/* OK���һ�û��ʵ��sigactions �ı��ƣ�����ͷ�ļ�����ϣ������POSIX ��׼ */

/* ����ԭע���Ѿ���ʱ����Ϊ��0.11�ں����Ѿ�ʵ���� sigaction()��	*/
#define SA_NOCLDSTOP	1			/* ���ӽ��̴���ֹͣ״̬���Ͳ���SIGCHLD ����	*/
#define SA_NOMASK		0x40000000	/* ����ֹ��ָ�����źŴ������(�źž��)�����յ����źš�	*/
#define SA_ONESHOT		0x80000000	/* �źž��һ�������ù��ͻָ���Ĭ�ϴ�������	*/

/* ���²�������sigprocmask()-- �ı������źż�(������)����Щ�������Ըı�ú�������Ϊ��	*/
#define SIG_BLOCK	0	/* for blocking signals */			/* �������źż��м��ϸ������źż���	*/
#define SIG_UNBLOCK	1	/* for unblocking signals */		/* �������źż���ɾ��ָ�����źż���	*/
#define SIG_SETMASK	2	/* for setting the signal mask */	/* ���������źż����ź������룩��	*/

/* ���������������Ŷ���ʾָ���޷���ֵ�ĺ���ָ�룬�Ҷ���һ��int���Ͳ�����������ָ��
 * ֵ���߼��Ͻ�ʵ���ϲ����ܳ��ֵĺ�����ֵַ������Ϊ����signal�����ĵڶ�����������
 * �ڸ�֪�ںˣ����ں˴����źŻ���Զ��źŵĴ���ʹ�÷����μ�kernel/signal.c����
 * �� 94һ96 �С�	*/
#define SIG_DFL ((void (*)(int))0)	/* default signal handling */	/* Ĭ�ϵ��źŴ�������źž������	*/
#define SIG_IGN ((void (*)(int))1)	/* ignore signal */				/* �����źŵĴ������	*/

/* ������sigaction �����ݽṹ��
 * sa_handler �Ƕ�Ӧĳ�ź�ָ��Ҫ��ȡ���ж��������������SIG_DFL��������SIG_IGN ������
 * ���źţ�Ҳ������ָ������źź�����һ��ָ�롣
 * sa_mask �����˶��źŵ������룬���źų���ִ��ʱ����������Щ�źŵĴ���
 * sa_flags ָ���ı��źŴ�����̵��źż���������37-39 �е�λ��־����ġ�
 * sa_restorer �ָ�����ָ�룬�����ڱ���ԭ���صĹ���ָ�롣
 * ���⣬���𴥷��źŴ�����ź�Ҳ��������������ʹ����SA_NOMASK ��־��
 */
struct sigaction
{
  void (*sa_handler) (int);
  sigset_t sa_mask;
  int sa_flags;
  void (*sa_restorer) (void);
};

/* ����signal����������Ϊ�źš�sig��װһ�µ��źŴ�������źž��������sigaction()���ơ�
 * �ú�����������������ָ����Ҫ������ź�_sig;����һ���������޷���ֵ�ĺ���ָ��_func���ú�
 * ������ֵҲ�Ǿ���һ��int���������һ��(int)�����޷���ֵ�ĺ���ָ�룬 ���Ǵ�����źŵ�ԭ��
 * ������	*/
void (*signal (int _sig, void (*_func) (int))) (int);

/* �������������ڷ����źš�kilio�������κν��̻�����鷢���źš�raiseO������ǰ��
 * ���������źš������õȼ���kill(getpid()��sig)���μ�kernel/exit.c��60�С�
 */
int raise (int sig);				
int kill (pid_t pid, int sig);		/* ���������κν��������̷����κ��źš�	*/
/* �ڽ��̵�����ṹ�У�����һ���Ա���λ��ʾ��ǰ���̴������32λ�ź��ֶ�signal���⣬��
 * ��һ��ͬ���Ա���λ��ʾ���������ν��̵�ǰ�����źż��������źż������ֶ�blocked��Ҳ��
 * 32λ��ÿ�����ش���һ����Ӧ�������źš��޸Ľ��̵������źż�������������������ָ����
 * �źš�������������������ڲ������������źż�����Ȼ��ʵ�������ܼ򵥣������汾�ں���
 * ��δʵ�֡����� sigaddset() �� sigdelset() ���ڶ��źż��е��źŽ�������ɾ�޸ġ�
 * sigaddset()������ maskָ����źż�������ָ�����ź�signoosigdelset��֮������
 * sigemptyset()��sigfillset()���ڳ�ʼ�����������źż���ÿ��������ʹ���źż�ǰ����
 * ��Ҫʹ������������֮һ�������źż����г�ʼ����sigemptyset() ����������ε������źţ�
 * Ҳ����Ӧ���е��źš�sigfillset_+���źż������������źţ�Ҳ�����������źš���Ȼ
 * SIGINT��SIGSTOP�ǲ��ܱ����εġ�	*/




int sigaddset (sigset_t * mask, int signo);	/* ���źż�������źš�	*/
int sigdelset (sigset_t * mask, int signo);	/* ���źż���ȥ��ָ�����źš�	*/
int sigemptyset (sigset_t * mask);			/* ���źż������ָ���źż���	*/
int sigfillset (sigset_t * mask);			/* ���źż������������źš�	*/


int sigismember (sigset_t * mask, int signo);	/* 1 --is, 0 - not, -1 error */
					/* �ж�һ���ź��Ƿ����źż��еġ�    1 - �ǣ� 0 - ���ǣ�-1 - ����	*/

int sigpending (sigset_t * set);			/* ��set �е��źŽ��м�⣬���Ƿ��й�����źš�	*/

/* �ı�Ŀǰ�ı������źż����ź������룩��	*/
int sigprocmask (int how, sigset_t * set, sigset_t * oldset);

/* ��sigmask ��ʱ�滻���̵��ź�������,Ȼ����ͣ�ý���ֱ���յ�һ���źš�	*/
int sigsuspend (sigset_t * sigmask);

/* ���ڸı�������յ�ָ���ź�ʱ����ȡ���ж���	*/
int sigaction (int sig, struct sigaction *act, struct sigaction *oldact);

#endif /* _SIGNAL_H */
