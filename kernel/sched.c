/*
 * linux/kernel/sched.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * 'sched.c' is the main kernel file. It contains scheduling primitives
 * (sleep_on, wakeup, schedule etc) as well as a number of simple system
 * call functions (type getpid(), which just extracts a field from
 * current-task
 */
/*
 * 'sched.c'����Ҫ���ں��ļ������а����йص��ȵĻ�������(sleep_on��wakeup��schedule ��)�Լ�
 * һЩ�򵥵�ϵͳ���ú���������getpid()�����ӵ�ǰ�����л�ȡһ���ֶΣ���
 */
#include <linux/sched.h>		/* ���ȳ���ͷ�ļ�������������ṹtask_struct����1 ����ʼ����	*/
								/* �����ݡ�����һЩ�Ժ����ʽ������й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������	*/
#include <linux/kernel.h>		/* �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣	*/
#include <linux/sys.h>			/* ϵͳ����ͷ�ļ�������72 ��ϵͳ����C �����������,��'sys_'��ͷ��	*/
#include <linux/fdreg.h>		/* ����ͷ�ļ����������̿�����������һЩ���塣	*/
#include <asm/system.h>			/* ϵͳͷ�ļ������������û��޸�������/�ж��ŵȵ�Ƕ��ʽ���ꡣ	*/
#include <asm/io.h>				/* io ͷ�ļ�������Ӳ���˿�����/���������䡣	*/
#include <asm/segment.h>		/* �β���ͷ�ļ����������йضμĴ���������Ƕ��ʽ��ຯ����	*/
#include <signal.h>				/* �ź�ͷ�ļ��������źŷ��ų�����sigaction �ṹ����������ԭ�͡�	*/

#define _S(nr) (1<<((nr)-1))	/* �ú�ȡ�ź�nr���ź�λͼ�ж�Ӧλ�Ķ�������ֵ���źű��1-32��������	*/
								/* ��5��λͼ��ֵ���� // 1<<(5-1) = 16 = 00010000b	*/

#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))	/* ����SIGKILL ��SIGSTOP �ź�������������	*/
													/* ��������(��10111111111011111111b)��	*/

/* ��ʾ�����nr �ں˵��Ժ�������ʾ�����nr�Ľ��̺š�����״̬���ں˶�ջ�����ֽ�������Լ����	*/
void show_task (int nr, struct task_struct *p)
{
	int i, j = 4096 - sizeof (struct task_struct);

	printk ("%d: pid=%d, state=%d, ", nr, p->pid, p->state);
	i = 0;
	while (i < j && !((char *) (p + 1))[i])	/* ���ָ���������ݽṹ�Ժ����0 ���ֽ�����	*/
		i++;
	printk ("%d (of %d) chars free in kernel stack\n\r", i, j);
}

/* ��ʾ�������������š����̺š�����״̬���ں˶�ջ�����ֽ�������Լ����	*/
/* NR��TASKS��ϵͳ�����ɵ�������(����)����(64��)��������include/kernel/sched.h��4�С�	*/
void
show_stat (void)
{
	int i;

	for (i = 0; i < NR_TASKS; i++)	/* NR_TASKS ��ϵͳ�����ɵ������̣�����������64 ������	*/
		if (task[i])				/* ������include/kernel/sched.h ��4 �С�	*/
			show_task (i, task[i]);
}

/* PC��8253��ʱоƬ������ʱ��Ƶ��ԼΪ1.193180MH^Linux�ں�ϣ����ʱ�������жϵ�Ƶ����
 * 100Hz��Ҳ��ÿ10ms����һ��ʱ���жϡ���������LATCH������8253оƬ�ĳ�ֵ���μ�407�С�	*/
#define LATCH (1193180/HZ)

extern void mem_use (void);			/* [��]û���κεط���������øú�����	*/
extern int timer_interrupt (void);	/* ʱ���жϴ������(kernel/system_call.s,176)��	*/
extern int system_call (void);		/* ϵͳ�����жϴ������(kernel/system_call.s,80)��	*/
union task_union
{									/* ������������(����ṹ��Ա��stack �ַ���������Ա)��	*/
	struct task_struct task;		/* ��Ϊһ���������ݽṹ�����ջ����ͬһ�ڴ�ҳ�У�����	*/
	char stack[PAGE_SIZE];			/* �Ӷ�ջ�μĴ���ss ���Ի�������ݶ�ѡ�����	*/
};

static union task_union init_task = { INIT_TASK, };	/* �����ʼ���������(sched.h ��)��	*/
/* �ӿ�����ʼ����ĵδ���ʱ��ֵȫ�ֱ�����10ms/�δ𣩡�ϵͳʱ���ж�ÿ����һ�μ�һ���δ�
 * ǰ����޶���volatile��Ӣ�Ľ������׸ı�ġ����ȶ�����˼������޶��ʵĺ������������
 * ָ�����������ݿ��ܻ����ڱ����������޸Ķ��仯��ͨ���ڳ���������һ������ʱ����������
 * �������������ͨ�üĴ����У�����ebx������߷���Ч�ʡ���CPU����ֵ�ŵ�ebx�к�һ��
 * �Ͳ����ٹ��ĸñ�����Ӧ�ڴ�λ���е����ݡ�����ʱ�������������ں˳����һ���жϹ��̣�
 * �޸����ڴ��иñ�����ֵ��ebx�е�ֵ��������֮���¡�Ϊ�˽����������ʹ�����volatile
 * �޶������ô��������øñ���ʱһ��Ҫ��ָ���ڴ�λ����ȡ����ֵ�����Ｔ��Ҫ��gcc��Ҫ��
 * jiffies�����Ż�����Ҳ��ҪŲ��λ�ã�������Ҫ���ڴ���ȡ��ֵ����Ϊʱ���жϴ������
 * �ȳ�����޸�����ֵ��
 */
long volatile jiffies = 0;			/* �ӿ�����ʼ����ĵδ���ʱ��ֵ��10ms/�δ𣩡�	*/
									
long startup_time = 0;				/* ����ʱ�䡣��1970:0:0:0 ��ʼ��ʱ��������	*/
struct task_struct *current = &(init_task.task);	/* ��ǰ����ָ�루��ʼ��Ϊ��ʼ���񣩡�	*/
struct task_struct *last_task_used_math = NULL;		/* ʹ�ù�Э�����������ָ�롣	*/
struct task_struct *task[NR_TASKS] = { &(init_task.task), };	/* ��������ָ�����顣	*/

/* �����û���ջ����1K�����4K�ֽڡ����ں˳�ʼ�����������б������ں�ջ����ʼ�����
 * �Ժ󽫱���������0���û�̬��ջ������������0֮ǰ�����ں�ջ���Ժ���������0��1����
 * ��̬ջ������ṹ�������ö�ջsS:eSp(���ݶ�ѡ�����ָ��)����head.s����23�С�
 * ss������Ϊ�ں����ݶ�ѡ�����0x10����ָ��espָ��user��stack�������һ����档����
 * ��ΪIntel CPUִ�ж�ջ����ʱ���ȵݼ���ջָ��spֵ��Ȼ����spָ�봦������ջ���ݡ�
 */
long user_stack[PAGE_SIZE >> 2];	/* ����ϵͳ��ջָ�룬4K��ָ��ָ�����һ�	*/
/* �ýṹ�������ö�ջss:esp�����ݶ�ѡ�����ָ�룩����head.s����23 �С�	*/
struct
{
	long *a;
	short b;
}
stack_start =
{
&user_stack[PAGE_SIZE >> 2], 0x10};

/*
 * 'math_state_restore()' saves the current math information in the
 * old math state array, and gets the new ones from the current task
 */
/*
 * ����ǰЭ���������ݱ��浽��Э������״̬�����У�������ǰ�����Э������
 * ���ݼ��ؽ�Э��������
 */
/* �����񱻵��Ƚ������Ժ󣬸ú������Ա���ԭ�����Э������״̬�������ģ����ָ��µ��Ƚ�����	*/
/* ��ǰ�����Э������ִ��״̬��	*/
void
math_state_restore ()
{
/* �������û���򷵻أ���һ��������ǵ�ǰ���񣩡������һ��������ָ�ձ�������ȥ������	*/
	if (last_task_used_math == current)	/* �������û���򷵻�(��һ��������ǵ�ǰ����)��	*/
/* �ڷ���Э����������֮ǰҪ�ȷ�WAITָ�����ϸ�����ʹ����Э���������򱣴���״̬��	*/
		return;							/* ������ָ��"��һ������"�Ǹձ�������ȥ������	*/
	__asm__ ("fwait");					/* �ڷ���Э����������֮ǰҪ�ȷ�WAIT ָ�	*/
	if (last_task_used_math)
		{								/* ����ϸ�����ʹ����Э���������򱣴���״̬��	*/
			__asm__ ("fnsave %0"::"m" (last_task_used_math->tss.i387));
		}
/* ���ڣ�last��task��used��mathָ��ǰ�����Ա���ǰ���񱻽�����ȥʱʹ�á���ʱ�����ǰ
�����ù�Э����������ָ���״̬������Ļ�˵���ǵ�һ��ʹ�ã����Ǿ���Э����������ʼ��
���������ʹ����Э��������־��	*/
	last_task_used_math = current;		/* ���ڣ�last_task_used_math ָ��ǰ����	*/
										/* �Ա���ǰ���񱻽�����ȥʱʹ�á�	*/
	if (current->used_math)
		{								/* �����ǰ�����ù�Э����������ָ���״̬��	*/
			__asm__ ("frstor %0"::"m" (current->tss.i387));
		}
	else
		{								/* ����Ļ�˵���ǵ�һ��ʹ�ã�	*/
			__asm__ ("fninit"::);		/* ���Ǿ���Э����������ʼ�����	*/
			current->used_math = 1;		/* ������ʹ����Э��������־��	*/
		}
}

/*
 * 'schedule()' is the scheduler function. This is GOOD CODE! There
 * probably won't be any reason to change this, as it should work well
 * in all circumstances (ie gives IO-bound processes good response etc).
 * The one thing you might take a look at is the signal-handler code here.
 *
 * NOTE!! Task 0 is the 'idle' task, which gets called when no other
 * tasks can run. It can not be killed, and it cannot sleep. The 'state'
 * information in task[0] is never used.
 */
/*
 * 'schedule()'�ǵ��Ⱥ��������Ǹ��ܺõĴ��룡û���κ����ɶ��������޸ģ���Ϊ�����������е�
 * �����¹����������ܹ���IO-�߽紦��ܺõ���Ӧ�ȣ���ֻ��һ����ֵ�����⣬�Ǿ���������ź�
 * ������롣
 * ע�⣡������0 �Ǹ�����('idle')����ֻ�е�û�����������������ʱ�ŵ������������ܱ�ɱ
 * ����Ҳ����˯�ߡ�����0 �е�״̬��Ϣ'state'�Ǵ������õġ�
 */
void
schedule (void)
{
	int i, next, c;
	struct task_struct **p;				/* ����ṹָ���ָ�롣	*/
	/* check alarm, wake up any interruptible tasks that have got a signal */
	/* ���alarm�����̵ı�����ʱֵ���������κ��ѵõ��źŵĿ��ж����� */

	/* ���������������һ������ʼ���alarm��	*/
	for (p = &LAST_TASK; p > &FIRST_TASK; --p)
		if (*p)
		{
/* ������ù�����Ķ�ʱֵalarm�������Ѿ�����(alarm��jiffies���������ź�λͼ����SIGALRM
�źţ�����������SIGALARM�źš�Ȼ����alarm�����źŵ�Ĭ�ϲ�������ֹ���̡�jiffies
��ϵͳ�ӿ�����ʼ����ĵδ�����10ms/�δ𣩡�������sched.h��139�С�	*/
			if ((*p)->alarm && (*p)->alarm < jiffies)
			{
				(*p)->signal |= (1 << (SIGALRM - 1));
				(*p)->alarm = 0;
			}
	/* ����ź�λͼ�г����������ź��⻹�������źţ����������ڿ��ж�״̬����������Ϊ����״̬��	*/
	/* ����'~(_BLOCKABLE & (*p)->blocked)'���ں��Ա��������źţ���SIGKILL ��SIGSTOP ���ܱ�������	*/
	if (((*p)->signal & ~(_BLOCKABLE & (*p)->blocked)) &&
			(*p)->state == TASK_INTERRUPTIBLE)
		(*p)->state = TASK_RUNNING;		/*��Ϊ��������ִ�У�״̬��	*/
		}

	/* this is the scheduler proper: */
	/* �����ǵ��ȳ������Ҫ���� */

	while (1)
		{
			c = -1;
			next = 0;
			i = NR_TASKS;
			p = &task[NR_TASKS];
			/* ��δ���Ҳ�Ǵ�������������һ������ʼѭ�������������������������ۡ��Ƚ�ÿ������	*/
			/* ״̬�����counter����������ʱ��ĵݼ��δ������ֵ����һ��ֵ������ʱ�仹������next ��	*/
			/* ָ���ĸ�������š�	*/
			while (--i)
	{
		if (!*--p)
			continue;
		if ((*p)->state == TASK_RUNNING && (*p)->counter > c)
			c = (*p)->counter, next = i;
	}
/* ����Ƚϵó���counterֵ������0�Ľ��������ϵͳ��û��һ�������е�������ڣ���ʱc
��ȻΪ-1��neXt=0�������˳�124�п�ʼ��ѭ����ִ��141���ϵ������л�����������͸���
ÿ�����������Ȩֵ������ÿһ�������counterֵ��Ȼ��ص�125�����±Ƚϡ�counterֵ
�ļ��㷽ʽΪcounter = counter /2 + priority��ע�⣬���������̲����ǽ��̵�״̬��	*/
			if (c)
				break;
			for (p = &LAST_TASK; p > &FIRST_TASK; --p)
				if (*p)
					(*p)->counter = ((*p)->counter >> 1) + (*p)->priority;
		}
/* ������꣨������sched.h�У��ѵ�ǰ����ָ��currentָ�������Ϊnext�����񣬲��л�
�������������С���126��next����ʼ��Ϊ0�������ϵͳ��û���κ��������������ʱ��
��nextʼ��Ϊ0����˵��Ⱥ�������ϵͳ����ʱȥִ������0����ʱ����0��ִ��pause()
IIϵͳ���ã����ֻ���ñ�������	*/

	switch_to (next);					/* �л��������Ϊnext �����񣬲�����֮��	*/
}

/* pause()ϵͳ���á�ת����ǰ�����״̬Ϊ���жϵĵȴ�״̬�������µ��ȡ�
 * ��ϵͳ���ý����½��̽���˯��״̬��ֱ���յ�һ���źš����ź�������ֹ���̻���ʹ���̵���
 * һ���źŲ�������ֻ�е�������һ���źţ������źŲ����������أ�pause()�Ż᷵�ء�
 * ��ʱpause()����ֵӦ����-1������errno ����ΪEINTR�����ﻹû����ȫʵ�֣�ֱ��0.95 �棩��	*/
int
sys_pause (void)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule ();
	return 0;
}

/* �ѵ�ǰ������Ϊ�����жϵĵȴ�״̬������˯�߶���ͷָ��ָ��ǰ����
 * ֻ����ȷ�ػ���ʱ�Ż᷵�ء��ú����ṩ�˽������жϴ������֮���ͬ�����ơ�
 * ��������P�ǵȴ��������ͷָ�롣ָ���Ǻ���һ��������ַ�ı������������Pʹ����ָ���
 * ָ����ʽ��**P����������ΪC��������ֻ�ܴ�ֵ��û��ֱ�ӵķ�ʽ�ñ����ú����ı���øú���
 * �����б�����ֵ������ָ�롯*P��ָ���Ŀ�꣨����������ṹ����ı䣬���Ϊ�����޸ĵ��ø�
 * ����������ԭ������ָ�������ֵ������Ҫ����ָ�롯*P����ָ�룬����**P�����μ�ͼ8-6��pָ��
 * ��ʹ�������
	*/
void
sleep_on (struct task_struct **p)
{
	struct task_struct *tmp;

	/* ��ָ����Ч�����˳�����ָ����ָ�Ķ��������NULL����ָ�뱾��Ӧ��Ϊ0�������⣬���
	��ǰ����������0������������Ϊ����0�����в������Լ���״̬�������ں˴��������0��
	Ϊ˯��״̬�������塣	*/
	if (!p)
		return;
	if (current == &(init_task.task))		/* �����ǰ����������0��������(impossible!)��	*/
		panic ("task[0] trying to sleep");
/* ��tmpָ���Ѿ��ڵȴ������ϵ�����(����еĻ�)������inode->i��wait�����ҽ�˯�߶���ͷ
�ĵȴ�ָ��ָ��ǰ���������Ͱѵ�ǰ������뵽�� *P�ĵȴ������С�Ȼ�󽫵�ǰ������
Ϊ�����жϵĵȴ�״̬����ִ�����µ��ȡ�	*/
	tmp = *p;								/* ��tmp ָ���Ѿ��ڵȴ������ϵ�����(����еĻ�)��	*/
	*p = current;							/* ��˯�߶���ͷ�ĵȴ�ָ��ָ��ǰ����	*/
	current->state = TASK_UNINTERRUPTIBLE;	/* ����ǰ������Ϊ�����жϵĵȴ�״̬��	*/
	schedule ();							/* ���µ��ȡ�	*/
	/* ֻ�е�����ȴ����񱻻���ʱ�����ȳ�����ַ��ص������ʾ�������ѱ���ȷ�ػ��ѣ���
	��̬������Ȼ��Ҷ��ڵȴ�ͬ������Դ����ô����Դ����ʱ�����б�Ҫ�������еȴ�����Դ
	�Ľ��̡��ú���Ƕ�׵��ã�Ҳ��Ƕ�׻������еȴ�����Դ�Ľ��̡�����Ƕ�׵�����ָ��һ��
	���̵����� sleep_on()��ͻ��ڸú����б��л���������Ȩ��ת�Ƶ����������С���ʱ����
	����Ҳ��Ҫʹ��ͬһ��Դ����ôҲ��ʹ��ͬһ���ȴ�����ͷָ����Ϊ��������sleep_on()��
	��������Ҳ�ᡰ���롱�ú��������᷵�ء�ֻ�е��ں�ĳ�������Զ���ͷָ����Ϊ����wake_up
	�˸ö��У���ô��ϵͳ�л�ȥִ��ͷָ����ָ�Ľ���Aʱ���ý��̲Ż����ִ��163�У���
	���к�һ������B��λ����״̬�����ѣ��������ֵ�B����ִ��ʱ����Ҳ�ſ��ܼ���ִ��
	163�С��������滹�еȴ��Ľ���C����ô��Ҳ���C���ѵȡ�������163��ǰ��Ӧ�����
	һ����䣺*P = tmp;��183���ϵĽ��͡�
	*/
	if (tmp)								/* �������ڵȴ���������Ҳ������Ϊ����״̬�����ѣ���	*/
		tmp->state = 0;
}

/* ����ǰ������Ϊ���жϵĵȴ�״̬��������*p ָ���ĵȴ������С��μ��б���sleep_on()��˵����	*/
void
interruptible_sleep_on (struct task_struct **p)
{
	struct task_struct *tmp;
/* ��ָ����Ч�����˳�����ָ����ָ�Ķ��������NULL����ָ�뱾����Ϊ0���������ǰ����������0����������impossible!����	*/
	if (!p)
		return;
	if (current == &(init_task.task))
		panic ("task[0] trying to sleep");
/* ��tmpָ���Ѿ��ڵȴ������ϵ�����(����еĻ�)������inode->i��wait�����ҽ�˯�߶���ͷ
�ĵȴ�ָ��ָ��ǰ���������Ͱѵ�ǰ������뵽��  P�ĵȴ������С�Ȼ�󽫵�ǰ������
Ϊ���жϵĵȴ�״̬����ִ�����µ��ȡ�	*/
	tmp = *p;
	*p = current;
repeat:current->state = TASK_INTERRUPTIBLE;
	schedule ();
/* ֻ�е�����ȴ����񱻻���ʱ��������ֻ᷵�ص������ʾ�����ѱ���ȷ�ػ��Ѳ�ִ�С�
����ȴ������л��еȴ����񣬲��Ҷ���ͷָ����ָ��������ǵ�ǰ����ʱ���򽫸õȴ�
������Ϊ�����еľ���״̬��������ִ�е��ȳ��򡣵�ָ��*P��ָ��Ĳ��ǵ�ǰ����ʱ����
ʾ�ڵ�ǰ���񱻷�����к������µ����񱻲���ȴ�����ǰ������������Ȼ������ǣ���
���Լ���Ȼ�ȴ����ȴ���Щ����������е����񱻻���ִ��ʱ�����ѱ���������ȥִ����
�µ��ȡ�	*/
	if (*p && *p != current)
		{
			(**p).state = 0;
			goto repeat;
		}
	/* ����һ���������Ӧ����*p = tmp���ö���ͷָ��ָ������ȴ����񣬷����ڵ�ǰ����֮ǰ����	*/
	/* �ȴ����е��������Ĩ���ˡ���Ȼ��ͬʱҲ��ɾ��192���ϵ���䡣�μ�ͼ4.3��	*/
	*p = NULL;
	if (tmp)
		tmp->state = 0;
}

/*����*Pָ�������*P������ȴ�����ͷָ�롣�����µȴ������ǲ����ڵȴ�����ͷָ��
���ģ���˻��ѵ���������ȴ����е�����	*/
void
wake_up (struct task_struct **p)
{
	if (p && *p)
		{
			(**p).state = 0;		/* ��Ϊ�����������У�״̬��	*/
			*p = NULL;
		}
}

/*
 * OK, here are some floppy things that shouldn't be in the kernel
 * proper. They are here because the floppy needs a timer, and this
 * was the easiest way of doing it.
 */
/*
 * ���ˣ������￪ʼ��һЩ�й����̵��ӳ��򣬱���Ӧ�÷����ں˵���Ҫ�����еġ������Ƿ�������
 * ����Ϊ������Ҫһ��ʱ�ӣ����������������İ취��
 */
/* �����201 - 262�д������ڴ���������ʱ�����Ķ���δ���֮ǰ���ȿ�һ�¿��豸һ��
 * ���й�������������floppy.c)�����˵�������ߵ��Ķ����̿��豸��������ʱ����
 * ����δ��롣����ʱ�䵥λ��1���δ�=1/100�롣
 * ���������ŵȴ������������������ת�ٵĽ���ָ�롣��������0-3�ֱ��Ӧ����A-D��
*/

static struct task_struct *wait_motor[4] = { NULL, NULL, NULL, NULL };
/* ��������ֱ��Ÿ����������������Ҫ�ĵδ���Ϊ50���δ�(0.5�룩	*/
static int mon_timer[4] = { 0, 0, 0, 0 };
/* ��������ֱ��Ÿ����������ͣת֮ǰ��ά�ֵ�ʱ�䡣�������趨Ϊ10000���δ�(100�룩��	*/
static int moff_timer[4] = { 0, 0, 0, 0 };
/* ��Ӧ�����������е�ǰ��������Ĵ������üĴ���ÿλ�Ķ������£�
 * λ7-4:�ֱ����������D-A����������1-������0-�رա�
 * λ3 :1-����DMA���ж�����0 -��ֹDMA���ж�����
 * λ2 :1-�������̿�������0 -��λ���̿�������
 * λ1-0: 00 - 11������ѡ����Ƶ�����A-D��
*/
unsigned char current_DOR = 0x0C;		/* ��������Ĵ���(��ֵ������DMA �������жϡ�����FDC)��	*/
/* ָ������������������ת״̬����ȴ�ʱ�䡣
 * ����nr ��������(0��3)������ֵΪ�δ�����
 * �ֲ�����selected��ѡ��������־��blk��drv/floppy.c��122��?mask����ѡ������Ӧ��
 * ��������Ĵ���������������λ��mask��4λ�Ǹ�������������־��
 */
int
ticks_to_floppy_on (unsigned int nr)
{
	extern unsigned char selected;		/* ��ǰѡ�е����̺�(kernel/blk_drv/floppy.c,122)��	*/
	unsigned char mask = 0x10 << nr;	/* ��ѡ������Ӧ��������Ĵ���������������λ��	*/
/* ϵͳ�����4������������Ԥ�����ú�ָ������nrͣת֮ǰ��Ҫ������ʱ�䣨100�룩��Ȼ��
ȡ��ǰDOR�Ĵ���ֵ����ʱ����mask�У�����ָ�����������������־��λ��	*/
	if (nr > 3)
		panic ("floppy_on: nr>3");		/* ���4 ��������	*/
	moff_timer[nr] = 10000;				/* 100 s = very big :-)	ͣתά��ʱ�䡣 */
	cli ();								/* use floppy_off to turn it off */
	mask |= current_DOR;
	/* ������ǵ�ǰ�����������ȸ�λ����������ѡ��λ��Ȼ���ö�Ӧ����ѡ��λ��	*/
	if (!selected)
		{
			mask &= 0xFC;
			mask |= nr;
		}
	/* �����������Ĵ����ĵ�ǰֵ��Ҫ���ֵ��ͬ������FDC��������˿������ֵ(mask)������
	���Ҫ����������ﻹû��������������Ӧ���������������ʱ��ֵ��HZ/2 = 0.5���50��
	�δ𣩡����Ѿ���������������������ʱΪ2���δ�����������do_floppy_timerO���ȵ�
	�����жϵ�Ҫ��ִ�б��ζ�ʱ�����Ҫ�󼴿ɡ��˺���µ�ǰ��������Ĵ���current_DOR��	*/
	if (mask != current_DOR)
		{
			outb (mask, FD_DOR);
			if ((mask ^ current_DOR) & 0xf0)
	mon_timer[nr] = HZ / 2;
			else if (mon_timer[nr] < 2)
	mon_timer[nr] = 2;
			current_DOR = mask;
		}
	sti ();
	return mon_timer[nr];
}

/* �ȴ�ָ������������������һ��ʱ�䣬Ȼ�󷵻ء�
����ָ���������������������ת���������ʱ��Ȼ��˯�ߵȴ����ڶ�ʱ�жϹ����л�һֱ
�ݼ��ж������趨����ʱֵ������ʱ���ڣ��ͻỽ������ĵȴ����̡�	*/
void
floppy_on (unsigned int nr)
{
	cli ();								/* ���жϡ�	*/
/* ������������ʱ��û������һֱ�ѵ�ǰ������Ϊ�����ж�˯��״̬������ȴ�������еĶ����С�	*/
	while (ticks_to_floppy_on (nr))
		sleep_on (nr + wait_motor);
	sti ();								/* ���жϡ�	*/
}

/* �ùر���Ӧ�������ͣת��ʱ����3 �룩��	*/
/* ����ʹ�øú�����ȷ�ر�ָ����������������￪��100��֮��Ҳ�ᱻ�رա�	*/
void
floppy_off (unsigned int nr)
{
	moff_timer[nr] = 3 * HZ;
}

/* ���̶�ʱ�����ӳ��򡣸������������ʱֵ�����ر�ͣת��ʱֵ�����ӳ�������ʱ�Ӷ�ʱ	*/
/* �ж��б����ã����ÿһ���δ�(10ms)������һ�Σ�������￪����ͣת��ʱ����ֵ�����ĳ	*/
/* һ�����ͣת��ʱ��������������Ĵ����������λ��λ��	*/
void
do_floppy_timer (void)
{
	int i;
	unsigned char mask = 0x10;

	for (i = 0; i < 4; i++, mask <<= 1)
	{
		if (!(mask & current_DOR))			/* �������DOR ָ���������������	*/
			continue;
		if (mon_timer[i])
		{
			if (!--mon_timer[i])
				wake_up (i + wait_motor);	/* ������������ʱ�����ѽ��̡�	*/
		}
		else if (!moff_timer[i])
		{									/* ������ͣת��ʱ����	*/
			current_DOR &= ~mask;			/* ��λ��Ӧ�������λ����	*/
			outb (current_DOR, FD_DOR);		/* ������������Ĵ�����	*/
		}
		else
			moff_timer[i]--;				/* �������ͣת��ʱ�ݼ���	*/
	}
}

/* �����ǹ��ڶ�ʱ���Ĵ��롣������64����ʱ����	*/
#define TIME_REQUESTS 64
/* ��ʱ������ṹ�Ͷ�ʱ�����顣�ö�ʱ������ר���ڹ������ر�����������ﶨʱ����������
���Ͷ�ʱ�������ִ�Linuxϵͳ�еĶ�̬��ʱ����Dynamic Timer���������ں�ʹ�á�	*/
static struct timer_list
{
	long jiffies;							/* ��ʱ�δ�����	*/
	void (*fn) ();							/* ��ʱ�������	*/
	struct timer_list *next;				/* ��һ����ʱ����	*/
}
timer_list[TIME_REQUESTS], *next_timer = NULL;

/* ��Ӷ�ʱ�����������Ϊָ���Ķ�ʱֵ(�δ���)����Ӧ�Ĵ������ָ�롣
������������floppy.c�����øú���ִ��������ر�������ʱ������
����jiffies -��10����Ƶĵδ�����*fn()-��ʱʱ�䵽ʱִ�еĺ�����
*/
void
add_timer (long jiffies, void (*fn) (void))
{
	struct timer_list *p;

	/* �����ʱ�������ָ��Ϊ�գ����˳���	*/
	if (!fn)
		return;
	cli ();
	/* �����ʱֵ<=0�������̵����䴦����򡣲��Ҹö�ʱ�������������С�	*/
	if (jiffies <= 0)
		(fn) ();
	else
		{
			/* �Ӷ�ʱ�������У���һ�������	*/
			for (p = timer_list; p < timer_list + TIME_REQUESTS; p++)
	if (!p->fn)
		break;
			/* ����Ѿ������˶�ʱ�����飬��ϵͳ����?��������ʱ�����ݽṹ������Ӧ��Ϣ������������ͷ��	*/
			if (p >= timer_list + TIME_REQUESTS)
	panic ("No more time requests free");
			/* ��ʱ�����ݽṹ������Ӧ��Ϣ������������ͷ	*/
			p->fn = fn;
			p->jiffies = jiffies;
			p->next = next_timer;
			next_timer = p;
/* �������ʱֵ��С��������������ʱ��ȥ����ǰ����Ҫ�ĵδ����������ڴ���ʱ��ʱ
ֻҪ�鿴����ͷ�ĵ�һ��Ķ�ʱ�Ƿ��ڼ��ɡ�[[��γ������û�п�����ȫ�������
����Ķ�ʱ��ֵС��ԭ��ͷһ����ʱ��ֵʱ������������ѭ���У�����ʱ����Ӧ�ý�����
������һ����ʱ��ֵ��ȥ�µĵ�1���Ķ�ʱֵ���������1����ʱֵ<=��2�������2��
��ʱֵ�۳���1����ֵ���ɣ������������ѭ���н��д���]]	*/
			while (p->next && p->next->jiffies < p->jiffies)
	{
		p->jiffies -= p->next->jiffies;
		fn = p->fn;
		p->fn = p->next->fn;
		p->next->fn = fn;
		jiffies = p->jiffies;
		p->jiffies = p->next->jiffies;
		p->next->jiffies = jiffies;
		p = p->next;
	}
		}
	sti ();
}

/* ʱ���ж�C�������������system��call.s�еġ�timer��interrupt(176��)�����á�
����cpl�ǵ�ǰ��Ȩ��0��3����ʱ���жϷ���ʱ����ִ�еĴ���ѡ����е���Ȩ����
cpl=0ʱ��ʾ�жϷ���ʱ����ִ���ں˴��룻cpl=3ʱ��ʾ�жϷ���ʱ����ִ���û����롣
����һ����������ִ��ʱ��Ƭ����ʱ������������л�����ִ��һ����ʱ���¹�����	*/
void
do_timer (long cpl)
{
	extern int beepcount;					/* ����������ʱ��δ���(kernel/chr_drv/console.c,697)	*/
	extern void sysbeepstop (void);			/* �ر�������(kernel/chr_drv/console.c,691)	*/

	/* ���������������������رշ�����(��0x61 �ڷ��������λλ0 ��1��λ0 ����8253	*/
	/* ������2 �Ĺ�����λ1 ����������)��	*/
	if (beepcount)
		if (!--beepcount)
			sysbeepstop ();

/* �����ǰ��Ȩ��(cpl)Ϊ0(��ߣ���ʾ���ں˳����ڹ���)�����ں˴�������ʱ��stime
������[Linus���ں˳���ͳ��Ϊ�����û���supervisor���ĳ��򣬼�system��call.s��193��
�ϵ�Ӣ��ע��]�����cpl > 0�����ʾ��һ���û������ڹ���������utime��
*/
	if (cpl)
		current->utime++;
	else
		current->stime++;

	/* ����ж�ʱ�����ڣ��������1����ʱ����ֵ��1������ѵ���0���������Ӧ�Ĵ������
	 * �����ô������ָ����Ϊ�ա�Ȼ��ȥ�����ʱ����next��timer�Ƕ�ʱ�������ͷָ�롣	*/
	if (next_timer)
		{									/* next_timer �Ƕ�ʱ�������ͷָ��(��270 ��)��	*/
			next_timer->jiffies--;
			while (next_timer && next_timer->jiffies <= 0)
	{
		void (*fn) (void);					/* ���������һ������ָ�붨�壡����	*/

		fn = next_timer->fn;
		next_timer->fn = NULL;
		next_timer = next_timer->next;
		(fn) ();							/* ���ô�������	*/
	}
		}
	/* �����ǰ���̿�����FDC ����������Ĵ������������λ����λ�ģ���ִ�����̶�ʱ����(245 ��)��	*/
	if (current_DOR & 0xf0)
		do_floppy_timer ();
/* �����������ʱ�仹û�꣬���˳��������õ�ǰ�������м���ֵΪ0������������ʱ���ж�ʱ����
�ں˴����������򷵻أ��������ִ�е��Ⱥ�����	*/
	if ((--current->counter) > 0)
		return;								/* �����������ʱ�仹û�꣬���˳���	*/
	current->counter = 0;
	if (!cpl)
		return;								/* ���ڳ����û����򣬲�����counter ֵ���е��ȡ�	*/
	schedule ();
}

/* ϵͳ���ù���-���ñ�����ʱʱ��ֵ(��)��
�������seconds����0���������¶�ʱֵ��������ԭ��ʱʱ�̻�ʣ��ļ��ʱ�䡣���򷵻�0��
�������ݽṹ�б�����ʱֵalarm�ĵ�λ��ϵͳ�δ�1�δ�Ϊ10���룩������ϵͳ������
���ö�ʱ����ʱϵͳ�δ�ֵjiffies��ת���ɵδ�λ�Ķ�ʱֵ֮�ͣ�����jiffies + HZ*��ʱ
��ֵ����������������������Ϊ��λ�Ķ�ʱֵ����˱���������Ҫ�����ǽ������ֵ�λ��ת����
���г���HZ = 100�����ں�ϵͳ����Ƶ�ʡ�������include/sched.h��5���ϡ�
����seconds���µĶ�ʱʱ��ֵ����λ���롣
*/
int
sys_alarm (long seconds)
{
	int old = current->alarm;

	if (old)
		old = (old - jiffies) / HZ;
	current->alarm = (seconds > 0) ? (jiffies + HZ * seconds) : 0;
	return (old);
}

/* ȡ��ǰ���̺�pid��	*/
int
sys_getpid (void)
{
	return current->pid;
}

/* ȡ�����̺�ppid��	*/
int
sys_getppid (void)
{
	return current->father;
}

/* ȡ�û���uid��	*/
int
sys_getuid (void)
{
	return current->uid;
}

/* ȡeuid��	*/
int
sys_geteuid (void)
{
	return current->euid;
}

/* ȡ���gid��	*/
int
sys_getgid (void)
{
	return current->gid;
}

/* ȡegid��	*/
int
sys_getegid (void)
{
	return current->egid;
}

/* ϵͳ���ù��� -- ���Ͷ�CPU ��ʹ������Ȩ�����˻����𣿣���	*/
/* Ӧ������increment ����0������Ļ�,��ʹ����Ȩ���󣡣�	*/
int
sys_nice (long increment)
{
	if (current->priority - increment > 0)
		current->priority -= increment;
	return 0;
}

/* ���ȳ���ĳ�ʼ���ӳ���	*/
void
sched_init (void)
{
	int i;
	struct desc_struct *p;	/* ��������ṹָ�롣	*/
/* Linuxϵͳ����֮�����ں˲����졣�ں˴���ᱻ�����޸ġ�Linus���Լ��������޸�����Щ
�ؼ��Ե����ݽṹ�������rosix��׼�Ĳ����ݡ����������������ж���䲢�ޱ�Ҫ������
��Ϊ�������Լ��Լ������޸��ں˴�����ˡ�	*/
	if (sizeof (struct sigaction) != 16)	/* sigaction �Ǵ���й��ź�״̬�Ľṹ��	*/
		panic ("Struct sigaction MUST be 16 bytes");
/* ��ȫ���������������ó�ʼ��������0��������״̬���������;ֲ����ݱ���������
FIRST��TSS��ENTRY �� FIRST��LDT��ENTRY ��ֵ�ֱ��� 4 �� 5�������� include/linux/sched.h
�У�gdt��һ�������������飨include/linux/head.h ����ʵ���϶�Ӧ����head.s��
��234���ϵ�ȫ�����������ַ����gdt�������gdt + FIRST��TSS��ENTRY��Ϊ
gdt[FIRST_TSS_ENTRY](��Ϊgdt[4])��Ҳ��gdt�����4��ĵ�ַ���μ�
include/asm/system.h���� 65 �п�ʼ��
*/
	set_tss_desc (gdt + FIRST_TSS_ENTRY, &(init_task.task.tss));
	set_ldt_desc (gdt + FIRST_LDT_ENTRY, &(init_task.task.ldt));
	/* ��������������������ע��i=1 ��ʼ�����Գ�ʼ��������������ڣ�����������ṹ�������ļ� include/linux/head.h �С�	*/
	
	p = gdt + 2 + FIRST_TSS_ENTRY;
	for (i = 1; i < NR_TASKS; i++)
		{
			task[i] = NULL;
			p->a = p->b = 0;
			p++;
			p->a = p->b = 0;
			p++;
		}
	/* Clear NT, so that we won't have troubles with that later on */
	/* �����־�Ĵ����е�λNT�������Ժ�Ͳ������鷳 */
	/* NT ��־���ڿ��Ƴ���ĵݹ����(Nested Task)����NT ��λʱ����ô��ǰ�ж�����ִ��	*/
	/* iret ָ��ʱ�ͻ����������л���NT ָ��TSS �е�back_link �ֶ��Ƿ���Ч��	*/
	__asm__ ("pushfl ; andl $0xffffbfff,(%esp) ; popfl");	/* ��λNT ��־��	*/
/* ������0��TSS��ѡ������ص�����Ĵ���tr�����ֲ����������ѡ������ص��ֲ�����
����Ĵ���Idtr�С�ע�⣡���ǽ�GDT����ӦLDT��������ѡ������ص�Idtr��ֻ��ȷ��
��һ�Σ��Ժ�������LDT�ļ��أ���CPU����TSS�е�LDT���Զ����ء�	*/
	ltr (0);						/* ������0 ��TSS ���ص�����Ĵ���tr��	*/
	lldt (0);						/* ���ֲ�����������ص��ֲ���������Ĵ�����	*/
	
/* ע�⣡���ǽ�GDT ����ӦLDT ��������ѡ������ص�ldtr��ֻ��ȷ������һ�Σ��Ժ�������	*/
/* LDT �ļ��أ���CPU ����TSS �е�LDT ���Զ����ء�	*/

/* ����������ڳ�ʼ��8253��ʱ����ͨ��0��ѡ������ʽ3�������Ƽ�����ʽ��ͨ��0��
������Ž����жϿ�����оƬ��IRQ0�ϣ���ÿ10���뷢��һ��IRQ0����LATCH�ǳ�ʼ
��ʱ����ֵ��	*/
	outb_p (0x36, 0x43);			/* binary, mode 3, LSB/MSB, ch 0 */
	outb_p (LATCH & 0xff, 0x40);	/* LSB	��ʱֵ���ֽڡ�	*/
	outb (LATCH >> 8, 0x40);		/* MSB	��ʱֵ���ֽڡ�	*/
/* ����ʱ���жϴ��������������ʱ���ж��ţ����޸��жϿ����������룬����ʱ���жϡ�
Ȼ������ϵͳ�����ж��š������������ж���������IDT���������ĺ궨�����ļ�
include/asm/system.h�е�33��39�д������ߵ�����μ�system.h�ļ���ʼ����˵����
*/
	set_intr_gate (0x20, &timer_interrupt);
	/* �޸��жϿ����������룬����ʱ���жϡ�	*/
	outb (inb_p (0x21) & ~0x01, 0x21);
	/* ����ϵͳ�����ж��š�	*/
	set_system_gate (0x80, &system_call);
}
