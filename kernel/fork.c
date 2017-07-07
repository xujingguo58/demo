/*
 *  linux/kernel/fork.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  'fork.c' contains the help-routines for the 'fork' system call
 * (see also system_call.s), and some misc functions ('verify_area').
 * Fork is rather simple, once you get the hang of it, but the memory
 * management can be a bitch. See 'mm/mm.c': 'copy_page_tables()'
 */
/*
* 'fork.c'�к���ϵͳ����'fork'�ĸ����ӳ��򣨲μ�system_call.s�����Լ�һЩ��������
* ('verify_area')��һ�����˽���fork���ͻᷢ�����Ƿǳ��򵥵ģ����ڴ����ȴ��Щ�Ѷȡ�
* �μ�'mm/mm.c'�е�'copy_page_tables()'��
*/
#include <errno.h>					/* �����ͷ�ļ�������ϵͳ�и��ֳ���š�(Linus ��minix ��������)��	*/

#include <linux/sched.h>			/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 �����ݣ�	*/
									/* ����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/
#include <linux/kernel.h>			/* �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣	*/
#include <asm/segment.h>			/* �β���ͷ�ļ����������йضμĴ���������Ƕ��ʽ��ຯ����	*/
#include <asm/system.h>				/* ϵͳͷ�ļ������������û��޸�������/�ж��ŵȵ�Ƕ��ʽ���ꡣ	*/

/* дҳ����֤����ҳ�治��д������ҳ�档������mm/memory.c��261�п�ʼ��	*/
extern void write_verify(unsigned long address);

long last_pid=0;	/* ���½��̺ţ���ֵ���� get_empty_process()���ɡ�	*/

/* ���̿ռ�����дǰ��֤������
����80386 CPU,��ִ����Ȩ��0����ʱ��������û��ռ��е�ҳ���Ƿ���ҳ�����ģ����
��ִ���ں˴���ʱ�û��ռ�������ҳ�汣����־�������ã�дʱ���ƻ���Ҳ��ʧȥ�����á�
verify��areaO���������ڴ�Ŀ�ġ�������80486�������CPU������ƼĴ���CR0����һ��
д������־WP (λ16)���ں˿���ͨ�����øñ�־����ֹ��Ȩ��0�Ĵ������û��ռ�ֻ��
ҳ��ִ��д���ݣ����򽫵��·���д�����쳣���Ӷ�486����CPU����ͨ�����øñ�־����
����������Ŀ�ġ�
�ú����Ե�ǰ�����߼���ַ��addr��addr + size��һ�η�Χ��ҳΪ��λִ��д����ǰ
�ļ����������ڼ���ж�����ҳ��Ϊ��λ���в�������˳���������Ҫ�ҳ�addr����ҳ
�濪ʼ��ַstart��Ȼ��start���Ͻ������ݶλ�ַ��ʹ���start�任��CPU 4G���Կ�
���еĵ�ַ�����ѭ������write��verifyO��ָ����С���ڴ�ռ����дǰ��֤����ҳ��
��ֻ���ģ���ִ�й������͸���ҳ�������дʱ���ƣ���
*/

void verify_area(void * addr,int size)
{
	unsigned long start;

/* ���Ƚ���ʼ��ַstart����Ϊ������ҳ����߽翪ʼλ�ã�ͬʱ��Ӧ�ص�����֤�����С��
�¾��е�start & Oxfff�������ָ����ʼλ��addr (Ҳ��start)������ҳ���е�ƫ��
ֵ��ԭ��֤��Χsize�������ƫ��ֵ����չ����addr����ҳ����ʼλ�ÿ�ʼ�ķ�Χֵ��
�����30����Ҳ��Ҫ����֤��ʼλ��start������ҳ��߽�ֵ���μ�ǰ���ͼ���ڴ���
֤��Χ�ĵ�������
*/
	start = (unsigned long) addr;
/* ����ʼ��ַstart ����Ϊ������ҳ����߽翪ʼλ�ã�ͬʱ��Ӧ�ص�����֤�����С��	*/
/* ��ʱstart �ǵ�ǰ���̿ռ��е����Ե�ַ��	*/
	size += start & 0xfff;
	start &= 0xfffff000;				/* ��ʱstart�ǵ�ǰ���̿ռ��е��߼���ַ��	*/
/* �����start���Ͻ������ݶ������Ե�ַ�ռ��е���ʼ��ַ�����ϵͳ�������Կռ��еĵ�
ַλ�á�����Linux0.11�ںˣ������ݶκʹ���������Ե�ַ�ռ��еĻ�ַ���޳�����ͬ��
Ȼ��ѭ������дҳ����֤����ҳ�治��д������ҳ�档��mm/memory.c��261�У�
*/
	start += get_base(current->ldt[2]);	/* ��ʱstart ���ϵͳ�������Կռ��еĵ�ַλ�á�	*/
	while (size>0)
	{
		size -= 4096;
/* дҳ����֤����ҳ�治��д������ҳ�档��mm/memory.c��261 �У�	*/
		write_verify(start);
		start += 4096;
	}
}

/* �����ڴ�ҳ��
����nr��������ţ�p�����������ݽṹָ�롣�ú���Ϊ�����������Ե�ַ�ռ������ô���
�κ����ݶλ�ַ���޳���������ҳ������Linuxϵͳ������дʱ���ƣ�copy on write��
��������������Ϊ�½��������Լ���ҳĿ¼�����ҳ�����û��ʵ��Ϊ�½��̷�������
�ڴ�ҳ�档��ʱ�½������丸���̹��������ڴ�ҳ�档�����ɹ�����0�����򷵻س���š�
*/
int copy_mem(int nr,struct task_struct * p)
{
	unsigned long old_data_base,new_data_base,data_limit;
	unsigned long old_code_base,new_code_base,code_limit;

/* ����ȡ��ǰ���ֲ̾����������д���������������ݶ����������еĶ��޳����ֽ�������
0x0f�Ǵ����ѡ�����0x17�����ݶ�ѡ�����Ȼ��ȡ��ǰ���̴���κ����ݶ������Ե�ַ
�ռ��еĻ���ַ������Linux0.11�ں˻���֧�ִ�������ݶη�������������������Ҫ
������κ����ݶλ�ַ���޳��Ƿ񶼷ֱ���ͬ�������ں���ʾ������Ϣ����ֹͣ���С�
get_limit()�� get_base()������ include/linux/sched.h �� 226 �д���
*/
	code_limit=get_limit(0x0f);						/* ȡ�ֲ����������д�������������ж��޳���	*/
	data_limit=get_limit(0x17);						/* ȡ�ֲ��������������ݶ����������ж��޳���	*/
	old_code_base = get_base(current->ldt[1]);		/* ȡԭ����λ�ַ��	*/
	old_data_base = get_base(current->ldt[2]);		/* ȡԭ���ݶλ�ַ��	*/
	if (old_data_base != old_code_base)
		panic("We don't support separate I&D");
	if (data_limit < code_limit)					/* ������ݶγ��� < ����γ���Ҳ���ԡ�	*/
		panic("Bad data_limit");
/* Ȼ�����ô����е��½��������Ե�ַ�ռ��еĻ���ַ���ڣ�64MB *������ţ������ø�ֵ
�����½��ֲ̾����������ж��������еĻ���ַ�����������½��̵�ҳĿ¼�����ҳ���
�����Ƶ�ǰ���̣������̣���ҳĿ¼�����ҳ�����ʱ�ӽ��̹������̵��ڴ�ҳ�档
���������copy��page��tablesO����0�������ʾ�������ͷŸ������ҳ���
*/
	new_data_base = new_code_base = nr * 0x4000000;	/* �»�ַ=�����*64Mb(�����С)��	*/
	p->start_code = new_code_base;
	set_base(p->ldt[1],new_code_base);				/* ���ô�����������л�ַ��	*/
	set_base(p->ldt[2],new_data_base);				/* �������ݶ��������л�ַ��	*/
	if (copy_page_tables(old_data_base,new_data_base,data_limit))
	{												/* ���ƴ�������ݶΡ�	*/
		free_page_tables(new_data_base,data_limit);	/* ����������ͷ�������ڴ档	*/
		return -ENOMEM;
	}
	return 0;
}

/*
 *  Ok, this is the main fork-routine. It copies the system process
 * information (task[nr]) and sets up the necessary registers. It
 * also copies the data segment in it's entirety.
 */
/*
* OK����������Ҫ��fork �ӳ���������ϵͳ������Ϣ(task[n])�������ñ�Ҫ�ļĴ�����
* ���������ظ������ݶΡ�
*/
/* ���ƽ��̡�
�ú����Ĳ����ǽ���ϵͳ�����жϴ�����̣�system��call.s����ʼ��ֱ�����ñ�ϵͳ���ô���
���̣�system��call.s��208�У��͵��ñ�����ǰʱ��system��call.s��217�У���ѹ��ջ��
���Ĵ�����ֵ����Щ��system��call.s��������ѹ��ջ��ֵ��������������
��CPUִ���ж�ָ��ѹ����û�ջ��ַss��esp����־�Ĵ���eflags�ͷ��ص�ַcs��eip;
�ڵ�83��88���ڸս���system��callʱѹ��ջ�ĶμĴ���ds��es��fs��edx��ecx��ebx;
�۵�94�е���sys��call��table��sys��fork����ʱѹ��ջ�ķ��ص�ַ���ò���none��ʾ����
�ܵ� 212--216 ���ڵ��� copy��process()֮ǰѹ��ջ�� gs��esi��edi��ebp �� eax(nr)ֵ��
���в���nr�ǵ���find��empty��process()���������������š�
*/

int copy_process(int nr,long ebp,long edi,long esi,long gs,long none,
		long ebx,long ecx,long edx,
		long fs,long es,long ds,
		long eip,long cs,long eflags,long esp,long ss)
{
	struct task_struct *p;
	int i;
	struct file *f;

/* ����Ϊ���������ݽṹ�����ڴ档����ڴ��������򷵻س����벢�˳���Ȼ��������
�ṹָ��������������nr���С�����nrΪ����ţ���ǰ��find��empty��process()���ء�
���Űѵ�ǰ��������ṹ���ݸ��Ƶ������뵽���ڴ�ҳ��P��ʼ����
*/
	p = (struct task_struct *) get_free_page();		/* Ϊ���������ݽṹ�����ڴ档	*/
	if (!p)											/* ����ڴ��������򷵻س����벢�˳���	*/
		return -EAGAIN;
	task[nr] = p;									/* ��������ṹָ��������������С�	*/
													/* ����nr Ϊ����ţ���ǰ��find_empty_process()���ء�	*/
	*p = *current;									/* NOTE! this doesn't copy the supervisor stack */
													/* ע�⣡���������Ḵ�Ƴ����û��Ķ�ջ ��ֻ���Ƶ�ǰ�������ݣ���	*/
/* ���Ը������Ľ��̽ṹ���ݽ���һЩ�޸ģ���Ϊ�½��̵�����ṹ���Ƚ��½��̵�״̬
��Ϊ�����жϵȴ�״̬���Է�ֹ�ں˵�����ִ�С�Ȼ�������½��̵Ľ��̺�pid�͸�����
��father������ʼ����������ʱ��Ƭֵ������priorityֵ��һ��Ϊ15��������������
��λ�½��̵��ź�λͼ��������ʱֵ���Ự��session���쵼��־leader�����̼�����
�������ں˺��û�̬����ʱ��ͳ��ֵ�������ý��̿�ʼ���е�ϵͳʱ��start��time��
*/
	p->state = TASK_UNINTERRUPTIBLE;
	p->pid = last_pid;						/* �½��̺š���ǰ�����find_empty_process()�õ���	*/
	p->father = current->pid;				/* ���ø����̺š�	*/
	p->counter = p->priority;				/* ����ʱ��Ƭֵ��	*/
	p->signal = 0;							/* �ź�λͼ��0��	*/
	p->alarm = 0;							/* ������ʱֵ���δ�������	*/
	p->leader = 0;							/* process leadership doesn't inherit */
											/* ���̵��쵼Ȩ�ǲ��ܼ̳е� */
	p->utime = p->stime = 0;				/* ��ʼ���û�̬ʱ��ͺ���̬ʱ�䡣	*/
	p->cutime = p->cstime = 0;				/* ��ʼ���ӽ����û�̬�ͺ���̬ʱ�䡣	*/
	p->start_time = jiffies;				/* ���̿�ʼ����ʱ�䣨��ǰʱ��δ�������	*/
/* ���޸�����״̬��TSS���ݣ��μ��б��˵����������ϵͳ������ṹp������ 1ҳ��
�ڴ棬���ԣ�PAGE��SIZE + (long) p����espO����ָ���ҳ���ˡ�ss0:esp0��������
���ں�ִ̬��ʱ��ջ�����⣬�ڵ�3���������Ѿ�֪����ÿ��������GDT���ж�������
����������һ���������TSS������������һ���������LDT���������������111��
�����ǰ�GDT�б�����LDT����������ѡ��������ڱ������TSS���С���CPUִ��
�л�����ʱ�����Զ���TSS�а�LDT����������ѡ������ص�ldtr�Ĵ����С�
*/
	p->tss.back_link = 0;					/* ������������״̬��TSS ��������ݣ��μ��б��˵������	*/
	p->tss.esp0 = PAGE_SIZE + (long) p;		/* �����ں�̬ջָ�롣��ջָ�루�����Ǹ�����ṹp ����	*/
											/* ��1 ҳ���ڴ棬���Դ�ʱesp0 ����ָ���ҳ���ˣ���	*/
	p->tss.ss0 = 0x10;						/* �ں�̬ջ�Ķ�ѡ��������ں����ݶ���ͬ����	*/
	p->tss.eip = eip;						/* ָ�����ָ�롣	*/
	p->tss.eflags = eflags;					/* ��־�Ĵ�����	*/
	p->tss.eax = 0;							/* ���ǵ�fork()����ʱ�½��̻᷵��0��ԭ�����ڡ�	*/
	p->tss.ecx = ecx;
	p->tss.edx = edx;
	p->tss.ebx = ebx;
	p->tss.esp = esp;
	p->tss.ebp = ebp;
	p->tss.esi = esi;
	p->tss.edi = edi;
	p->tss.es = es & 0xffff;			/* �μĴ�����16 λ��Ч��	*/
	p->tss.cs = cs & 0xffff;
	p->tss.ss = ss & 0xffff;
	p->tss.ds = ds & 0xffff;
	p->tss.fs = fs & 0xffff;
	p->tss.gs = gs & 0xffff;
	p->tss.ldt = _LDT(nr);				/* ��������nr �ľֲ���������ѡ�����LDT ����������GDT �У���	*/
	p->tss.trace_bitmap = 0x80000000;	/* (�� 16 λ��Ч)��	*/

/* �����ǰ����ʹ����Э���������ͱ����������ġ����ָ��cits����������ƼĴ���CR0
�е������ѽ�����TS����־��ÿ�����������л���CPU�������øñ�־���ñ�־���ڹ���
��ѧЭ������������ñ�־��λ����ôÿ��ESCָ��ᱻ�����쳣7�������Э����
�����ڱ�־MPҲͬʱ��λ�Ļ�����ôWAITָ��Ҳ�Ჶ����ˣ���������л�������һ
��ESCָ�ʼִ��֮����Э�������е����ݾͿ�����Ҫ��ִ���µ�ESCָ��֮ǰ����
���������������ᱣ��Э�����������ݲ���λTS��־��ָ��fnsave���ڰ�Э������
������״̬���浽Ŀ�Ĳ�����ָ�����ڴ������У�tss.i387����
*/
	if (last_task_used_math == current)
		__asm__("clts ; fnsave %0"::"m" (p->tss.i387));
/* ���������ƽ���ҳ���������Ե�ַ�ռ����������������κ����ݶ��������еĻ�ַ
���޳���������ҳ�������������ֵ����0������λ������������Ӧ��ͷ�Ϊ
��������������������ṹ���ڴ�ҳ��
*/
	if (copy_mem(nr,p))
	{									/* ���ز�Ϊ0 ��ʾ����	*/
		task[nr] = NULL;
		free_page((long) p);
		return -EAGAIN;
	}
/* ��������������ļ��Ǵ򿪵ģ��򽫶�Ӧ�ļ��Ĵ򿪴�����1����Ϊ���ﴴ�����ӽ���
���븸���̹�����Щ�򿪵��ļ�������ǰ���̣������̣���pwd��root��executable
���ô�������1��������ͬ���ĵ����ӽ���Ҳ��������Щi�ڵ㡣
*/
	for (i=0; i<NR_OPEN;i++)
		if (f=p->filp[i])
			f->f_count++;
/* ����ǰ���̣������̣���pwd, root ��executable ���ô�������1��	*/
	if (current->pwd)
		current->pwd->i_count++;
	if (current->root)
		current->root->i_count++;
	if (current->executable)
		current->executable->i_count++;
/* �����GDT��������������TSS�κ�LDT����������������ε��޳��������ó�104��
�ڡ�set��tss��desc()��set��ldt��desc()�Ķ���μ� include/asm/system.h ��
��52��66�к��롣��gdt+(nr?l)+FIRST��TSS��ENTRY��������nr��TSS����������ȫ��
���еĵ�ַ����Ϊÿ������ռ��GDT����2������ʽ��Ҫ��������nr?l������
����Ȼ����½������óɾ���̬�������������л�ʱ������Ĵ���tr��CPU�Զ����ء�
��󷵻��½��̺š�
*/
	set_tss_desc(gdt+(nr<<1)+FIRST_TSS_ENTRY,&(p->tss));
	set_ldt_desc(gdt+(nr<<1)+FIRST_LDT_ENTRY,&(p->ldt));
	p->state = TASK_RUNNING;		/* do this last, just in case */
									/* ����ٽ����������óɿ�����״̬���Է���һ */
	return last_pid;				/* �����½��̺ţ���������ǲ�ͬ�ģ���	*/
}

/* Ϊ�½���ȡ�ò��ظ��Ľ��̺�last_pid�������������������е������(������index)��	*/
int find_empty_process(void)
{
	int i;
/* ���Ȼ�ȡ�µĽ��̺š����last��pid��1�󳬳����̺ŵ�������ʾ��Χ�������´�1��ʼ
ʹ��pid�š�Ȼ�����������������������õ�pid���Ƿ��Ѿ����κ�����ʹ�á��������
��ת��������ʼ�����»��һ��pid�š�����������������Ϊ������Ѱ��һ���������
������š�last��pid��һ��ȫ�ֱ��������÷��ء������ʱ����������64�����Ѿ���ȫ
��ռ�ã��򷵻س����롣
*/
	repeat:
		if ((++last_pid)<0) last_pid=1;
		for(i=0 ; i<NR_TASKS ; i++)
			if (task[i] && task[i]->pid == last_pid) goto repeat;
	for(i=1 ; i<NR_TASKS ; i++)		/* ����0 �ų����⡣	*/
		if (!task[i])
			return i;
	return -EAGAIN;
}
