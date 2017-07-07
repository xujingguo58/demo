/*
* linux/kernel/exit.c
*
* (C) 1991 Linus Torvalds
*/

#include <errno.h>			/* �����ͷ�ļ�������ϵͳ�и��ֳ���š�(Linus ��minix ��������)	*/
#include <signal.h>			/* �ź�ͷ�ļ��������źŷ��ų������źŽṹ�Լ��źŲ�������ԭ�͡�	*/
#include <sys/wait.h>		/* �ȴ�����ͷ�ļ�������ϵͳ����wait()��waitpid()����س������š�	*/
#include <linux/sched.h>	/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 �����ݣ�	*/
							/* ����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/
#include <linux/kernel.h>	/* �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣	*/
#include <linux/tty.h>		/* tty ͷ�ļ����������й�tty_io������ͨ�ŷ���Ĳ�����������	*/
#include <asm/segment.h>	/* �β���ͷ�ļ����������йضμĴ���������Ƕ��ʽ��ຯ����	*/
int sys_pause (void);
int sys_close (int fd);

/* �ͷ�ָ������ռ�õ�����ۼ����������ݽṹռ�õ��ڴ�ҳ�档
����P���������ݽṹָ�롣�ú����ں����sys��killO��SyS��Waitpid()�����б����á�
ɨ������ָ�������task[]��Ѱ��ָ������������ҵ�����������ո�����ۣ�Ȼ���ͷ�
���������ݽṹ��ռ�õ��ڴ�ҳ�棬���ִ�е��Ⱥ������ڷ���ʱ�����˳����������������
����û���ҵ�ָ�������Ӧ������ں�panic��
*/
void release (struct task_struct *p)
{
	int i;

	if (!p)							/* ����������ݽṹָ����NULL����ʲôҲ�������˳���	*/
		return;
	for (i = 1; i < NR_TASKS; i++)	/* ɨ���������飬Ѱ��ָ������	*/
	if (task[i] == p)
	{
		task[i] = NULL;				/* �ÿո�������ͷ�����ڴ�ҳ��	*/
		free_page ((long) p);
		schedule ();				/* ���µ��ȡ�	*/
		return;
	}
	panic ("trying to release non-existent task");	/* ָ����������������������	*/
}

/* ��ָ������P�����ź�sig,Ȩ��Ϊpriv��
������sig -�ź�ֵ��p -ָ�������ָ�룻priv -ǿ�Ʒ����źŵı�־��������Ҫ���ǽ���
�û����Ի򼶱���ܷ����źŵ�Ȩ�����ú��������жϲ�������ȷ�ԣ�Ȼ���ж������Ƿ����㡣
����������ָ�����̷����ź�sig���˳������򷵻�δ��ɴ���š�
*/
static inline int
send_sig (long sig, struct task_struct *p, int priv)
{
/* ���źŲ���ȷ������ָ��Ϊ��������˳���	*/
	if (!p || sig < 1 || sig > 32)
		return -EINVAL;
/* ���ǿ�Ʒ��ͱ�־��λ�����ߵ�ǰ���̵���Ч�û���ʶ��(euid)����ָ�����̵�euid(Ҳ�����Լ�)��
���ߵ�ǰ�����ǳ����û����������p�����ź�sig�����ڽ���pλͼ����Ӹ��źţ���������˳���
����suser()����Ϊ(current-)euid==0}�������ж��Ƿ��ǳ����û���
*/
	if (priv || (current->euid == p->euid) || suser ())
		p->signal |= (1 << (sig - 1));
	else
		return -EPERM;
	return 0;
}

/* ��ֹ�Ự(session)��
* ���̻Ự�ĸ�����μ���1�����йؽ�����ͻỰ��˵����	*/
static void
kill_session (void)
{
	struct task_struct **p = NR_TASKS + task;	/* ָ��*p ����ָ������������ĩ�ˡ�	*/
/* ɨ������ָ�����飬�������е����񣨳�����0���⣩�������Ự��session���ڵ�ǰ���̵�
�Ự�ž��������͹ҶϽ����ź�SIGHUP��	*/
	while (--p > &FIRST_TASK)
	{
		if (*p && (*p)->session == current->session)
			(*p)->signal |= 1 << (SIGHUP - 1);	/* ���͹ҶϽ����źš�	*/
	}
}

/*
* XXX need to check permissions needed to send signals to process
* groups, etc. etc. kill() permissions semantics are tricky!
*/
/*
* Ϊ���������ȷ����źţ�XXX ��Ҫ�����ɡ�kill()����ɻ��Ʒǳ�����!
*/
/* ϵͳ����kiii()���������κν��̻�����鷢���κ��źţ�������ֻ��ɱ������?��
* ����pid�ǽ��̺ţ�Sig����Ҫ���͵��źš�
* ���pidֵ>0�����źű����͸����̺���pid�Ľ��̡�
* ���pid=0����ô�źžͻᱻ���͸���ǰ���̵Ľ������е����н��̡�
* ���Pid=-1�����ź�sig�ͻᷢ�͸�����һ�����̣���ʼ����init��������н��̡�
* ���pid < -1�����ź�sig�����͸�������-pid�����н��̡�
* ����ź�sigΪ0���򲻷����źţ����Ի���д����顣����ɹ��򷵻�0��
* �ú���ɨ�����������������pid��ֵ�����������Ľ��̷���ָ�����ź�sig����pid����0��
* ������ǰ�����ǽ������鳤�������Ҫ���������ڵĽ���ǿ�Ʒ����ź�sig��
*/
int
sys_kill (int pid, int sig)
{
	struct task_struct **p = NR_TASKS + task;
	int err, retval = 0;

	if (!pid)
		while (--p > &FIRST_TASK)
		{
			if (*p && (*p)->pgrp == current->pid)
				if (err = send_sig (sig, *p, 1))	/* ǿ�Ʒ����źš�	*/
					retval = err;
		}else if (pid > 0) 
			while (--p > &FIRST_TASK)
			{
				if (*p && (*p)->pid == pid)
					if (err = send_sig (sig, *p, 0))
						retval = err;
			}else if (pid == -1)
				while (--p > &FIRST_TASK)
					if (err = send_sig (sig, *p, 0))
						retval = err;
			else
				while (--p > &FIRST_TASK)
					if (*p && (*p)->pgrp == -pid)
						if (err = send_sig (sig, *p, 0))
							retval = err;
	return retval;
}

/* ֪ͨ������һ�����pid�����ź�SIGCHLD:Ĭ��������ӽ��̽�ֹͣ����ֹ��
���û���ҵ������̣����Լ��ͷš�������rosix.iҪ������������������ֹ�����ӽ���Ӧ��
����ʼ����1���ݡ�
*/
static void
tell_father (int pid)
{
	int i;

	if (pid)
/* ɨ����������Ѱ��ָ������pid�������䷢���ӽ��̽�ֹͣ����ֹ�ź�SIGCHLD��	*/
		for (i = 0; i < NR_TASKS; i++)
			{
	if (!task[i])
		continue;
	if (task[i]->pid != pid)
		continue;
	task[i]->signal |= (1 << (SIGCHLD - 1));
	return;
			}
/* if we don't find any fathers, we just release ourselves */
/* This is not really OK. Must change it to make father 1 */
/* ���û���ҵ������̣�����̾��Լ��ͷš������������ã�����ĳ��ɽ���1�䵱�丸���̡�	*/
	printk ("BAD BAD - no father found\n\r");
	release (current);	/* ���û���ҵ������̣����Լ��ͷš�	*/
}

/* �����˳���������������137�д���ϵͳ���ô�����sys_exit()�б����á�
�ú������ѵ�ǰ������ΪTASK_ZOMBIE״̬��Ȼ��ȥִ�е��Ⱥ���scheduleO�����ٷ��ء�
����code���˳�״̬�룬���Ϊ�����롣
*/
int
do_exit (long code)		/* code �Ǵ����롣	*/
{
	int i;

/* �����ͷŵ�ǰ���̴���κ����ݶ���ռ���ڴ�ҳ������free��page��tablesO�ĵ�1������
(get��baseO����ֵ)ָ����CPU���Ե�ַ�ռ�����ʼ����ַ����2����get��limit()����ֵ��
˵�����ͷŵ��ֽڳ���ֵ��get��base()���е�current-}ldt[l]�������̴������������λ��
(current->ldt[2]�������̴������������λ��)��get��limit()�е�0x0f�ǽ��̴���ε�
ѡ�����0x17�ǽ������ݶε�ѡ�����������ȡ�λ���ַʱʹ�øöε�������������ַ��Ϊ
������ȡ�γ���ʱʹ�øöε�ѡ�����Ϊ������free��page��tables()����λ��mm/memory.c
�ļ��� 105 �У�get��base()�� get��limit()��λ�� include/linux/sched.h ͷ�ļ��� 213 �д���
*/
	free_page_tables (get_base (current->ldt[1]), get_limit (0x0f));
	free_page_tables (get_base (current->ldt[2]), get_limit (0x17));
/* �����ǰ�������ӽ��̣��ͽ��ӽ��̵�father ��Ϊ1(�丸���̸�Ϊ����1)��������ӽ����Ѿ�	*/
/* ���ڽ���(ZOMBIE)״̬���������1 �����ӽ�����ֹ�ź�SIGCHLD��	*/
	for (i = 0; i < NR_TASKS; i++)
		if (task[i] && task[i]->father == current->pid)
			{
	task[i]->father = 1;
	if (task[i]->state == TASK_ZOMBIE)
/* assumption task[1] is always init */	/* ������� task[l]�϶��ǽ��� init 	*/
		(void) send_sig (SIGCHLD, task[1], 1);
			}
/* �رյ�ǰ���̴��ŵ������ļ���	*/
	for (i = 0; i < NR_OPEN; i++)
		if (current->filp[i])
			sys_close (i);
/* �Ե�ǰ���̹���Ŀ¼pwd����Ŀ¼root �Լ����г����i �ڵ����ͬ��������
�Żظ���i�ڵ㲢�ֱ��ÿգ��ͷţ���
*/
	iput (current->pwd);
	current->pwd = NULL;
	iput (current->root);
	current->root = NULL;
	iput (current->executable);
	current->executable = NULL;
/* �����ǰ��������ͷ(leader)���̲������п��Ƶ��նˣ����ͷŸ��նˡ�	*/
	if (current->leader && current->tty >= 0)
		tty_table[current->tty].pgrp = 0;
/* �����ǰ�����ϴ�ʹ�ù�Э����������last_task_used_math �ÿա�	*/
	if (last_task_used_math == current)
		last_task_used_math = NULL;
/* �����ǰ������leader ���̣�����ֹ������ؽ��̡�	*/
	if (current->leader)
		kill_session ();
/* �ѵ�ǰ������Ϊ����״̬��������ǰ�����Ѿ��ͷ�����Դ�������潫�ɸ����̶�ȡ���˳��롣	*/
	current->state = TASK_ZOMBIE;
	current->exit_code = code;
/* ֪ͨ�����̣�Ҳ���򸸽��̷����ź�SIGCHLD -- �ӽ��̽�ֹͣ����ֹ��	*/
	tell_father (current->father);
	schedule ();	/* ���µ��Ƚ������У����ø����̴����������������ƺ����ˡ�
����return��������ȥ��������Ϣ����Ϊ������������أ��������ں�����ǰ�ӹؼ���
volatile,�Ϳ��Ը���gcc���������������᷵�ص������������������gcc��������һ
Щ�Ĵ��룬���ҿ��Բ�����д����return���Ҳ��������پ�����Ϣ��
*/
	return (-1);	/* just to suppress warnings */	/* ������ȥ��������Ϣ	*/
}

/* ϵͳ����exit()����ֹ���̡�
����error��code���û������ṩ���˳�״̬��Ϣ��ֻ�е��ֽ���Ч����error��code����8
������wait()��waitpidO������Ҫ�󡣵��ֽ��н���������wait()��״̬��Ϣ�����磬
������̴�����ͣ״̬��TASK��STOPPED������ô����ֽھ͵���0x7f���μ�sys/wait.h
�ļ���13��18�С�wait()��waitpidO������Щ��Ϳ���ȡ���ӽ��̵��˳�״̬�����
������ֹ��ԭ���źţ���
*/
int
sys_exit (int error_code)
{
	return do_exit ((error_code & 0xff) << 8);
}

/* ϵͳ����waitpidO������ǰ���̣�ֱ��pidָ�����ӽ����˳�����ֹ�������յ�Ҫ����ֹ
�ý��̵��źţ���������Ҫ����һ���źž�����źŴ�����򣩡����pid��ָ���ӽ�������
�˳����ѳ���ν�Ľ������̣����򱾵��ý����̷��ء��ӽ���ʹ�õ�������Դ���ͷš�
���pid > 0����ʾ�ȴ����̺ŵ���pid���ӽ��̡�
���pid = 0����ʾ�ȴ�������ŵ��ڵ�ǰ������ŵ��κ��ӽ��̡�
���pid < -1����ʾ�ȴ�������ŵ���pid����ֵ���κ��ӽ��̡�
���pid = -1����ʾ�ȴ��κ��ӽ��̡�
��options = WUNTRACED����ʾ����ӽ�����ֹͣ�ģ�Ҳ���Ϸ��أ�������٣���
��options = WN0HANG����ʾ���û���ӽ����˳�����ֹ�����Ϸ��ء�
�������״ָ̬��stat��addr��Ϊ�գ���ͽ�״̬��Ϣ���浽���
����pid�ǽ��̺ţ�*stat��addr�Ǳ���״̬��Ϣλ�õ�ָ�룻options��waitpidѡ�
*/
int
sys_waitpid (pid_t pid, unsigned long *stat_addr, int options)
{
	int flag, code;		/* flag��־���ں����ʾ��ѡ�����ӽ��̴��ھ�����˯��̬��	*/
	struct task_struct **p;

	verify_area (stat_addr, 4);
repeat:
	flag = 0;
	/* ����������ĩ�˿�ʼɨ��������������������������Լ��ǵ�ǰ���̵��ӽ����	*/
	for (p = &LAST_TASK; p > &FIRST_TASK; --p)
	{									/* ����������ĩ�˿�ʼɨ����������	*/
		if (!*p || *p == current)			/* ��������ͱ������	*/
			continue;
		if ((*p)->father != current->pid)	/* ������ǵ�ǰ���̵��ӽ�����������	*/
			continue;
/* ��ʱɨ��ѡ�񵽵Ľ���P�϶��ǵ�ǰ���̵��ӽ��̡�
����ȴ����ӽ��̺�pid>0�����뱻ɨ���ӽ���P��pid����ȣ�˵�����ǵ�ǰ�����������
���̣����������ý��̣�����ɨ����һ�����̡�
*/
		if (pid > 0)
		{									/* ���ָ����pid>0����ɨ��Ľ���pid	*/
			if ((*p)->pid != pid)				/* ��֮���ȣ���������	*/
				continue;
/* �������ָ���ȴ����̵�pid=0����ʾ���ڵȴ�������ŵ��ڵ�ǰ������ŵ��κ��ӽ��̡�
�����ʱ��ɨ�����P�Ľ�������뵱ǰ���̵���Ų��ȣ���������
*/
		}
		else if (!pid)
		{									/* ���ָ����pid=0����ɨ��Ľ������	*/
			if ((*p)->pgrp != current->pgrp)	/* �뵱ǰ���̵���Ų��ȣ���������	*/
				continue;
/* �������ָ����Pid<-1����ʾ���ڵȴ�������ŵ���pid����ֵ���κ��ӽ��̡������ʱ
��ɨ�����P�������pid�ľ���ֵ���ȣ���������
*/
		}
		else if (pid != -1)
		{									/* ���ָ����pid<-1����ɨ��Ľ�����	*/
			if ((*p)->pgrp != -pid)			/* �������ֵ���ȣ���������	*/
				continue;
		}
/* ���ǰ3����pid���ж϶������ϣ����ʾ��ǰ�������ڵȴ����κ��ӽ��̣�Ҳ��pid =-1
���������ʱ��ѡ�񵽵Ľ���p����������̺ŵ���ָ��pid�������ǵ�ǰ�������е��κ�
�ӽ��̣������ǽ��̺ŵ���ָ��pid����ֵ���ӽ��̣��������κ��ӽ��̣���ʱָ����pid
����-1������������������ӽ���p������״̬������
*/
		switch ((*p)->state)
		{
/* ����ӽ���P���ڽ���״̬�������Ȱ������û�̬���ں�̬���е�ʱ��ֱ��ۼƵ���ǰ����
(������)�У�Ȼ��ȡ���ӽ��̵�pid���˳��룬���ͷŸ��ӽ��̡���󷵻��ӽ��̵��˳����pid��
*/
			case TASK_STOPPED:
				if (!(options & WUNTRACED))
					continue;
				put_fs_long (0x7f, stat_addr);	/* ��״̬��ϢΪ0x7f��	*/
				return (*p)->pid;					/* �˳��������ӽ��̵Ľ��̺š�	*/
			case TASK_ZOMBIE:
				current->cutime += (*p)->utime;	/* ���µ�ǰ���̵��ӽ����û�	*/
				current->cstime += (*p)->stime;	/* ̬�ͺ���̬����ʱ�䡣	*/
				flag = (*p)->pid;				/* ��ʱ�����ӽ���pid��	*/
				code = (*p)->exit_code;			/* ȡ�ӽ��̵��˳��롣	*/
				release (*p);					/* �ͷŸ��ӽ��̡�	*/
				put_fs_long (code, stat_addr);	/* ��״̬��ϢΪ�˳���ֵ��	*/
				return flag;					/* �˳��������ӽ��̵�pid.	*/
/* �������ӽ���p��״̬�Ȳ���ֹͣҲ���ǽ�������ô����flag=l����ʾ�ҵ���һ������
Ҫ����ӽ��̣���������������̬��˯��̬��
*/
			default:
				flag = 1;		/* ����ӽ��̲���ֹͣ����״̬����flag=1��	*/
				continue;
		}
	}
/* ���������������ɨ����������flag����λ��˵���з��ϵȴ�Ҫ����ӽ��̲�û�д�
���˳�����״̬�������ʱ������WN0HANGѡ���ʾ��û���ӽ��̴����˳�����ֹ̬��
���̷��أ��������̷���0���˳�������ѵ�ǰ������Ϊ���жϵȴ�״̬������ִ�е��ȡ�
���ֿ�ʼִ�б�����ʱ�����������û���յ���SIGCHLD������źţ������ظ�����
���򣬷��س����롮�жϵ�ϵͳ���á����˳���������������û�����Ӧ���ټ������ñ�
�����ȴ��ӽ��̡�
*/

	if (flag)
	{									/* ����ӽ���û�д����˳�����״̬��	*/
		if (options & WNOHANG)			/* ����options = WNOHANG�������̷��ء�	*/
			return 0;
		current->state = TASK_INTERRUPTIBLE;	/* �õ�ǰ����Ϊ���жϵȴ�״̬��	*/
		schedule ();					/* ���µ��ȡ�	*/
		if (!(current->signal &= ~(1 << (SIGCHLD - 1))))	/* �ֿ�ʼִ�б�����ʱ��	*/
			goto repeat;				/* �������û���յ���SIGCHLD ���źţ������ظ�����	*/
		else
			return -EINTR;				/* �˳������س����롣	*/
	}
/* ��û���ҵ�����Ҫ����ӽ��̣��򷵻س����루�ӽ��̲����ڣ���	*/
	return -ECHILD;
}
