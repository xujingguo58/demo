/*
* linux/fs/exec.c
*
* (C) 1991 Linus Torvalds
*/

/*
* #!-checking implemented by tytso.
*/
/*
* #!��ʼ�ĳ����ⲿ������tytso ʵ�ֵġ�	*/



/*
* Demand-loading implemented 01.12.91 - no need to read anything but
* the header into memory. The inode of the executable is put into
* "current->executable", and page faults do the actual loading. Clean.
*
* Once more I can proudly say that linux stood up to being changed: it
* was less than 2 hours work to get demand-loading completely implemented.
*/
/*
* ����ʱ��������1991.12.1 ʵ�ֵ� - ֻ�轫ִ���ļ�ͷ���ֶ����ڴ������
* ������ִ���ļ������ؽ��ڴ档
* ִ���ļ���i �ڵ㱻���ڵ�ǰ���̵Ŀ�ִ���ֶ���
* ("current->executable")����ҳ�쳣�����ִ���ļ���ʵ�ʼ��ز����Լ���������
*
* �ҿ�����һ���Ժ���˵��linux �������޸ģ�ֻ���˲���2 Сʱ�Ĺ���ʱ�����ȫ
* ʵ����������ش���
*/

#include <errno.h>			/* �����ͷ�ļ�������ϵͳ�и��ֳ���š�(Linus ��minix ��������)��	*/
#include <string.h>			/* �ַ���ͷ�ļ�����Ҫ������һЩ�й��ַ���������Ƕ�뺯����	*/
#include <sys/stat.h>		/* �ļ�״̬ͷ�ļ��������ļ����ļ�ϵͳ״̬�ṹstat{}�ͳ�����	*/
#include <a.out.h>			/* a.out ͷ�ļ���	������a.out ִ���ļ���ʽ��һЩ�ꡣ	*/
#include <linux/fs.h>		/* �ļ�ϵͳͷ�ļ��������ļ���ṹ��file,buffer_head,m_inode �ȣ���	*/
#include <linux/sched.h>	/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 �����ݣ�	*/
							/* ����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/
#include <linux/kernel.h>	/* �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣	*/
#include <linux/mm.h>		/* �ڴ����ͷ�ļ�������ҳ���С�����һЩҳ���ͷź���ԭ�͡�	*/
#include <asm/segment.h>	/* �β���ͷ�ļ����������йضμĴ���������Ƕ��ʽ��ຯ����	*/

extern int sys_exit (int exit_code);	/* �����˳�ϵͳ���á�	*/
extern int sys_close (int fd);			/* �ļ��ر�ϵͳ���á�	*/

/*
* MAX_ARG_PAGES defines the number of pages allocated for arguments
* and envelope for the new program. 32 should suffice, this gives
* a maximum env+arg of 128kB !
*/
/*
* MAX_ARG_PAGES �������³������������ͻ�������ʹ�õ��ڴ����ҳ����
* 32 ҳ�ڴ�Ӧ���㹻�ˣ���ʹ�û����Ͳ���(env+arg)�ռ���ܺϴﵽ128kB!
*/
#define MAX_ARG_PAGES 32

int sys_uselib()
{
	return -ENOSYS;
}

/*
 * create_tables() parses the env- and arg-strings in new user
 * memory and creates the pointer tables from them, and puts their
 * addresses on the "stack", returning the new stack pointer value.
 */
/*
 * create_tables()���������û��ڴ��н������������Ͳ����ַ������ɴ�
 * ����ָ����������ǵĵ�ַ�ŵ�"��ջ"�ϣ�Ȼ�󷵻���ջ��ָ��ֵ��
 */

/* �����û���ջ�д��������Ͳ�������ָ���	
 * ������p - �����ݶ�Ϊ���Ĳ����ͻ�����Ϣƫ��ָ�룻
 * argc - ����������envc -������������
 * ���أ���ջָ�롣	*/
static unsigned long * create_tables (char *p, int argc, int envc)
{
	unsigned long *argv, *envp;
	unsigned long *sp;

/* ջָ������4�ֽڣ�1�ڣ�Ϊ�߽����Ѱַ�ģ������������spΪ4��������ֵ����ʱsp
 * λ�ڲ����������ĩ�ˡ�Ȼ�������Ȱ�sp���£��͵�ַ�����ƶ�����ջ�пճ���������
 * ָ��ռ�õĿռ䣬���û�������ָ��envpָ��ô�����ճ���һ��λ�������������һ
 * ��NULLֵ������ָ���l��Sp������ָ�����ֽ�ֵ��4�ֽڣ����ٰ�sp�����ƶ����ճ�
 * �����в���ָ��ռ�õĿռ䣬����argvָ��ָ��ô���ͬ������մ���һ��λ�����ڴ��
 * һ��NULLֵ����ʱspָ�����ָ������ʼ�������ǽ�����������ָ��envp�������в�
 * ����ָ���Լ������в�������ֵ�ֱ�ѹ��ջ�С�	*/
	sp = (unsigned long *) (0xfffffffc & (unsigned long) p);
	sp -= envc + 1;
	envp = sp;
	sp -= argc + 1;
	argv = sp;
	put_fs_long ((unsigned long) envp, --sp);
	put_fs_long ((unsigned long) argv, --sp);
	put_fs_long ((unsigned long) argc, --sp);
/* �ٽ������и�����ָ��ͻ���������ָ��ֱ����ǰ��ճ�������Ӧ�ط������ֱ����һ
 * ��NULLָ�롣	*/
	while (argc-- > 0)
	{
		put_fs_long ((unsigned long) p, argv++);
		while (get_fs_byte (p++)) /* nothing */ ;	/* p ָ��ǰ��4 �ֽڡ�	*/
	}
	put_fs_long (0, argv);
/* ������������ָ�����ǰ��ճ�������Ӧ�ط���������һ��NULL ָ�롣	*/
	while (envc-- > 0)
	{
		put_fs_long ((unsigned long) p, envp++);
		while (get_fs_byte (p++)) /* nothing */ ;
	}
	put_fs_long (0, envp);
	return sp;					/* ���ع���ĵ�ǰ�¶�ջָ�롣	*/
}

/*
* count() counts the number of arguments/envelopes
*/
/*
* count()�������������в���/���������ĸ�����
*/
/* �������������
 * ������argv -����ָ�����飬���һ��ָ������NULL��
 * ͳ�Ʋ���ָ��������ָ��ĸ��������ں�����������ָ���ָ������ã���μ�����
 * kernel/sched.c�е�151��ǰ��ע�͡�
 * ���أ�����������	*/
static int count (char **argv)
{
	int i = 0;
	char **tmp;
	if (tmp = argv)
		while (get_fs_long ((unsigned long *) (tmp++)))
			i++;
	return i;
}

/*
* 'copy_string()' copies argument/envelope strings from user
* memory to free pages in kernel mem. These are in a format ready
* to be put directly into the top of new user memory.
*
* Modified by TYT, 11/24/91 to add the from_kmem argument, which specifies
* whether the string and the string array are from user or kernel segments:
*
* from_kmem argv * argv **
* 0 user space user space
* 1 kernel space user space
* 2 kernel space kernel space
*
* We do this by playing games with the fs segment register. Since it
* it is expensive to load a segment register, we try to avoid calling
* set_fs() unless we absolutely have to.
*/
/*
* 'copy_string()'�������û��ڴ�ռ俽�������ͻ����ַ������ں˿���ҳ���ڴ��С�
* ��Щ�Ѿ���ֱ�ӷŵ����û��ڴ��еĸ�ʽ��
*
* ��TYT(Tytso)��1991.12.24 ���޸ģ�������from_kmem �������ò���ָ�����ַ�����
* �ַ��������������û��λ����ں˶Ρ�
*
* from_kmem argv * argv **
* 0 �û��ռ� �û��ռ�
* 1 �ں˿ռ� �û��ռ�
* 2 �ں˿ռ� �ں˿ռ�
*
* ������ͨ�������fs �μĴ����������ġ�
* ���ڼ���һ���μĴ�������̫������
* ���Ǿ����������set_fs()������ʵ�ڱ�Ҫ��
*/
/* ����ָ�������Ĳ����ַ����������ͻ����ռ��С�
 * ������argc -����ӵĲ���������argv -����ָ�����飻page -�����ͻ����ռ�ҳ��ָ��
 * ���顣P -������ռ���ƫ��ָ�룬ʼ��ָ���Ѹ��ƴ���ͷ����from_kmem -�ַ�����Դ��־��
 * ��do_execveO�����У�p��ʼ��Ϊָ�������128kB���ռ�����һ�����ִ��������ַ���
 * ���Զ�ջ������ʽ���������и��ƴ�ŵġ����Pָ������Ÿ�����Ϣ�����Ӷ��𽥼�С��
 * ��ʼ��ָ������ַ�����ͷ�����ַ�����Դ��־from_kmemӦ����TYTΪ�˸�execveO����
 * ִ�нű��ļ��Ĺ��ܶ��¼ӵĲ�������û�����нű��ļ��Ĺ���ʱ�����в����ַ���������
 * �����ݿռ��С�
 * ���أ������ͻ����ռ䵱ǰͷ��ָ�롣�������򷵻�0��	*/
static unsigned long copy_strings (int argc, char **argv, unsigned long *page,
				unsigned long p, int from_kmem)
{
	char *tmp, *pag;
	int len, offset = 0;
	unsigned long old_fs, new_fs;

/* ����ȡ��ǰ�μĴ���ds (ָ���ں����ݶ�)��fsֵ���ֱ𱣴浽����new_fs��old_fs�С�
 * ����ַ������ַ������飨ָ�룩�����ں˿ռ䣬������fs�μĴ���ָ���ں����ݶΡ�	*/
	if (!p)
		return 0;			/* bullet-proofing	ƫ��ָ����֤ */
	new_fs = get_ds ();
	old_fs = get_fs ();
	if (from_kmem == 2)		/* ��������ָ�����ں˿ռ�������fsָ���ں˿ռ䡣	*/
		set_fs (new_fs);
/* Ȼ��ѭ��������������������һ����������ʼ���ƣ����Ƶ�ָ��ƫ�Ƶ�ַ������ѭ���У�	*/
	while (argc-- > 0)
	{
/* ����ȡ��Ҫ���Ƶĵ�ǰ�ַ���ָ�롣����ַ������û��ռ���ַ������飨�ַ���ָ�룩��
 * �ں˿ռ䣬������fs�μĴ���ָ���ں����ݶΣ�ds���������ں����ݿռ���ȡ���ַ���ָ��
 * tmp֮������ָ̻�fs�μĴ���ԭֵ��fs��ָ���û��ռ䣩���������޸�fsֵ��ֱ�Ӵ�
 * �û��ռ�ȡ�ַ���ָ�뵽tmp��	*/
		if (from_kmem == 1)		/* ����ָ�����ں˿ռ䣬��fsָ���ں˿ռ䡣	*/
			set_fs (new_fs);
		if (!(tmp = (char *) get_fs_long (((unsigned long *) argv) + argc)))
			panic ("argc is wrong");
		if (from_kmem == 1)		/* ����ָ�����ں˿ռ䣬��fsָ���û��ռ䡣	*/
			set_fs (old_fs);
/* Ȼ����û��ռ�ȡ���ַ�����������ò����ַ�������len���˺�tmpָ����ַ���ĩ�ˡ�
 * ������ַ������ȳ�����ʱ�����ͻ����ռ��л�ʣ��Ŀ��г��ȣ���ռ䲻���ˡ����ǻָ�
 * fs�μĴ���ֵ��������ı�Ļ���������0��������Ϊ�����ͻ����ռ�����128KB������ͨ
 * �������ܷ������������	*/
		len = 0;				/* remember zero-padding */
		do
		{						/* ����֪��������NULL �ֽڽ�β�� */
			len++;
		}
		while (get_fs_byte (tmp++));
			if (p - len < 0)
			{						/* this shouldn't happen - 128kB */
				set_fs (old_fs);	/* ���ᷢ��-��Ϊ��128kB �Ŀռ� */
				return 0;
			}
/* ����������������ַ��ذ��ַ������Ƶ������ͻ����ռ�ĩ�˴�����ѭ�������ַ������ַ�
 * �����У���������Ҫ�жϲ����ͻ����ռ�����Ӧλ�ô��Ƿ��Ѿ����ڴ�ҳ�档�����û�о�
 * ��Ϊ������1ҳ�ڴ�ҳ�档ƫ����offset������Ϊ��һ��ҳ���еĵ�ǰָ��ƫ��ֵ����Ϊ
 * �տ�ʼִ�б�����ʱ��ƫ�Ʊ���offset����ʼ��Ϊ0������(offset-1 < 0)�϶�������ʹ��
 * offset���±�����Ϊ��ǰpָ����ҳ�淶Χ�ڵ�ƫ��ֵ��	*/
		while (len)
		{
			--p;	--tmp;	--len;
			if (--offset < 0)
			{
				offset = p % PAGE_SIZE;
/* ����ַ������ַ������鶼���ں˿ռ��У���ôΪ�˴��ں����ݿռ临���ַ������ݣ�����
 * 143���ϻ��fs����Ϊָ���ں����ݶΡ�������137��������������ָ�fs�μĴ���ԭֵ��	*/
				if (from_kmem == 2)				/* ���ַ������ں˿ռ䡣	*/
					set_fs (old_fs);
/* �����ǰƫ��ֵP���ڵĴ��ռ�ҳ��ָ��������pagelp/PAGE��SIZE} ==0����ʾ��ʱpָ��
 * �����Ŀռ��ڴ�ҳ�滹�����ڣ���������һ�����ڴ�ҳ��������ҳ��ָ������ָ�����飬ͬ
 * ʱҲʹҳ��ָ��pagָ�����ҳ�档�����벻������ҳ���򷵻�0��	*/
				if (!(pag = (char *) page[p/PAGE_SIZE]) && !(pag = (char *) (page[p/PAGE_SIZE] = (unsigned long *) get_free_page()))) 
				//if (!(pag = (char *) page[p / PAGE_SIZE]) &&
				//		!(pag = (char *) page[p / PAGE_SIZE] =
				//		(unsigned long *) get_free_page ()))
					return 0;
/* ����ַ������ַ������������ں˿ռ䣬������fs �μĴ���ָ���ں����ݶΣ�ds����	*/
				if (from_kmem == 2)
					set_fs (new_fs);
			}
/* Ȼ���fs���и����ַ�����1�ֽڵ������ͻ����ռ��ڴ�ҳ��pag��offset����	*/

			*(pag + offset) = get_fs_byte (tmp);
		}
	}
/* ����ַ������ַ����������ں˿ռ䣬��ָ�fs�μĴ���ԭֵ����󣬷��ز����ͻ�����
 * �����Ѹ��Ʋ�����ͷ��ƫ��ֵ��	*/
	if (from_kmem == 2)
		set_fs (old_fs);
	return p;
}

/* �޸�����ľֲ������������ݡ�
 * �޸ľֲ���������LDT���������Ķλ�ַ�Ͷ��޳������������ͻ����ռ�ҳ����������ݶ�ĩ�ˡ�
 * ������text_size -ִ���ļ�ͷ����a_text�ֶθ����Ĵ���γ���ֵ��
 * page -�����ͻ����ռ�ҳ��ָ�����顣
 * ���أ����ݶ��޳�ֵ��64MB����	*/
static unsigned long change_ldt (unsigned long text_size, unsigned long *page)
{
	unsigned long code_limit, data_limit, code_base, data_base;
	int i;

/* ���ȸ���ִ���ļ�ͷ�����볤���ֶ�a_extֵ��������ҳ�泤��Ϊ�߽�Ĵ�����޳�����
 * �������ݶγ���Ϊ64MB��Ȼ��ȡ��ǰ���ֲ̾��������������������д���λ�ַ������
 * �λ�ַ�����ݶλ�ַ��ͬ����ʹ����Щ��ֵ�������þֲ����д���κ����ݶ��������еĻ�
 * ַ�Ͷ��޳���������ע�⣬���ڱ����ص��³���Ĵ�������ݶλ�ַ��ԭ�������ͬ�����
 * û�б�Ҫ���ظ�ȥ�������ǣ�����164��166�����������öλ�ַ�������࣬��ʡ�ԡ�	*/
	code_limit = text_size + PAGE_SIZE - 1;
	code_limit &= 0xFFFFF000;
	data_limit = 0x4000000;
	code_base = get_base (current->ldt[1]);
	data_base = code_base;
	set_base (current->ldt[1], code_base);
	set_limit (current->ldt[1], code_limit);
	set_base (current->ldt[2], data_base);
	set_limit (current->ldt[2], data_limit);
/* make sure fs points to the NEW data segment */
/* Ҫȷ��fs �μĴ�����ָ���µ����ݶ� */
/* fs�μĴ����з���ֲ������ݶ���������ѡ���(0x17)����Ĭ�������fs��ָ���������ݶΡ�	*/
	__asm__ ("pushl $0x17\n\tpop %%fs"::);
/* Ȼ�󽫲����ͻ����ռ��Ѵ�����ݵ�ҳ�棨�����MAX��ARG��PAGESҳ��128kB���ŵ����ݶ�
 * ĩ�ˡ������Ǵӽ��̿ռ�ĩ������һҳһҳ�طš�����put��pageO���ڰ�����ҳ��ӳ�䵽��
 * ���߼��ռ��С���mm/memory.c����197�С�	*/
	data_base += data_limit;
	for (i = MAX_ARG_PAGES - 1; i >= 0; i--)
	{
		data_base -= PAGE_SIZE;
		if (page[i])		/* �����ҳ����ڣ��ͷ��ø�ҳ�档	*/
			put_page (page[i], data_base);	

	}
	return data_limit;		/* ��󷵻����ݶ��޳�(64MB)��	*/

}

/*
* 'do_execve()' executes a new program.
*/
/*
* 'do_execve()'����ִ��һ���³���
*/
/* execve()ϵͳ�жϵ��ú��������ز�ִ���ӽ��̣��������򣩡�
* �ú�����ϵͳ�жϵ��ã�int   0x80�����ܺ�__NR_execYH ���õĺ����������Ĳ����ǽ���ϵͳ
* ���ô�����̺�ֱ��������ϵͳ���ô�����̣�system_call.c �� 200 �У��͵����ֺ���֮ǰ
* ��system_call.c���� 203 �У���ѹ��ջ�е�ֵ����Щֵ������
* �� �� 8b--88 ����ѵ� edx��ecx �� edx �Ĵ���ֵ���ֱ��Ӧ**envp��**argv ��*filename��
* �� �� 94 �е��� sys_call_table �� sys_execYH ������ָ�룩ʱѹ��ջ�ĺ������ص�ַ��tmp����
* �� �� 202 ���ڵ����ֺ��� do_execve ǰ��ջ��ָ��ջ�е���ϵͳ�жϵĳ������ָ�� eip��
* ������
* eip  -  ����ϵͳ�жϵĳ������ָ�룬�μ�  kernel/system_call.c ����ʼ���ֵ�˵����
* tmp - ϵͳ�ж����ڵ���_sys_execve ʱ�ķ��ص�ַ�����ã�
* filename - ��ִ�г����ļ���ָ�룻
* argv - �����в���ָ�������ָ�룻
* envp - ��������ָ�������ָ�롣
* ���أ�������óɹ����򲻷��أ��������ó���ţ�������-1��	*/
int do_execve (unsigned long *eip, long tmp, char *filename,
		 char **argv, char **envp)
{
	struct m_inode *inode;				/* �ڴ���I �ڵ�ָ��ṹ������	*/
	struct buffer_head *bh;				/* ���ٻ����ͷָ�롣	*/
	struct exec ex;						/* ִ���ļ�ͷ�����ݽṹ������	*/
	unsigned long page[MAX_ARG_PAGES];	/* �����ͻ����ַ����ռ��ҳ��ָ�����顣	*/
	int i, argc, envc;
	int e_uid, e_gid;					/* ��Ч�û�id ����Ч��id��	*/
	int retval;							/* ����ֵ��	*/
	int sh_bang = 0;					/* �����Ƿ���Ҫִ�нű�������롣	*/
	unsigned long p = PAGE_SIZE * MAX_ARG_PAGES - 4;	/* p ָ������ͻ����ռ����󲿡�

/* ����ʽ����ִ���ļ������л���֮ǰ���������ȸ�Щ���¡��ں�׼���� 128KB(32��ҳ��)
 * �ռ�����Ż�ִ���ļ��������в����ͻ����ַ��������а�P��ʼ���ó�λ��128KB�ռ��
 * ���1�����ִ����ڳ�ʼ�����ͻ����ռ�Ĳ��������У�P������ָ����128KB�ռ��еĵ�
 * ǰλ�á�
 * ���⣬����eip[l]�ǵ��ñ���ϵͳ���õ�ԭ�û��������μĴ���CSֵ�����еĶ�ѡ���
 * ��Ȼ�����ǵ�ǰ����Ĵ����ѡ�����0x000f���������Ǹ�ֵ����ôCSֻ�ܻ����ں˴���
 * �ε�ѡ���0x0008�������Ǿ��Բ�����ģ���Ϊ�ں˴����ǳ�פ�ڴ�����ܱ��滻���ġ�
 * ����������eip[l]��ֵȷ���Ƿ�������������Ȼ���ٳ�ʼ��128KB�Ĳ����ͻ�������
 * �䣬�������ֽ����㣬��ȡ��ִ���ļ���i�ڵ㡣�ٸ��ݺ��������ֱ����������в�����
 * �����ַ����ĸ���argc��envc�����⣬ִ���ļ������ǳ����ļ���	*/
	if ((0xffff & eip[1]) != 0x000f)
		panic ("execve called from supervisor mode");
	for (i = 0; i < MAX_ARG_PAGES; i++)	/* clear page-table */
		page[i] = 0;
	if (!(inode = namei (filename)))	/* get executables inode */
		return -ENOENT;
	argc = count (argv);				/* �����в���������	*/
	envc = count (envp);				/* �����ַ�������������	*/

/* ִ���ļ������ǳ����ļ��������ǳ����ļ����ó������룬��ת��exec_error2(��347 ��)��	*/
restart_interp:
	if (!S_ISREG (inode->i_mode))
	{									/* must be regular file */
		retval = -EACCES;
		goto exec_error2;		/* �����ǳ����ļ����ó����룬��ת��347�С�	*/
	}
/* �����鵱ǰ�����Ƿ���Ȩ����ָ����ִ���ļ���������ִ���ļ�i�ڵ��е����ԣ�������
 * �����Ƿ���Ȩִ�������ڰ�ִ���ļ�i�ڵ�������ֶ�ֵȡ��i�к��������Ȳ鿴������
 * �Ƿ������� ������-�û�-ID����set_user_id����־�͡�����-��-ID����set_group_id����
 * ־����������־��Ҫ����һ���û��ܹ�ִ����Ȩ�û����糬���û�root���ĳ�������ı�
 * ����ĳ���passwd�ȡ����set_user_id��־��λ�������ִ�н��̵���Ч�û�ID(euid:
 * �����ó�ִ���ļ����û�ID���������óɵ�ǰ���̵�euid�����ִ���ļ�set_group_id��
 * ��λ�Ļ�����ִ�н��̵���Ч��ID(egid)������Ϊִ���ļ�����ID���������óɵ�ǰ����
 * ��egid��������ʱ���������жϳ�����ֵ�����ڱ���e_uid��e_gid�С�	*/
	i = inode->i_mode;					/* ȡ�ļ������ֶ�ֵ��	*/
	e_uid = (i & S_ISUID) ? inode->i_uid : current->euid;
	e_gid = (i & S_ISGID) ? inode->i_gid : current->egid;
/* ���ڸ��ݽ��̵�euid��egid��ִ���ļ��ķ������Խ��бȽϡ����ִ���ļ��������н���
 * ���û������ܰ��ļ�����ֵi����6λ����ʱ�����3λ���ļ������ķ���Ȩ�ޱ�־�������
 * �����ִ���ļ��뵱ǰ���̵��û�����ͬ�飬��ʹ����ֵ���3λ��ִ���ļ����û��ķ���
 * Ȩ�ޱ�־�������ʱ���������3λ���������û����ʸ�ִ���ļ���Ȩ�ޡ�
 * Ȼ�����Ǹ���������i�����3����ֵ���жϵ�ǰ�����Ƿ���Ȩ���������ִ���ļ������
 * ѡ������Ӧ�û�û�����и��ļ���Ȩ����λ0��ִ��Ȩ�ޣ������������û�Ҳû���κ�Ȩ��
 * ���ߵ�ǰ�����û����ǳ����û����������ǰ����û��Ȩ���������ִ���ļ��������ò���
 * ִ�г����룬����ת��exec��error2��ȥ���˳�����*/
	if (current->euid == inode->i_uid)
		i >>= 6;
	else if (current->egid == inode->i_gid)
		i >>= 3;
	if (!(i & 1) && !((inode->i_mode & 0111) && suser ()))
	{
		retval = -ENOEXEC;
		goto exec_error2;
	}
/* ����ִ�е����˵����ǰ����������ָ��ִ���ļ���Ȩ�ޡ���˴����￪ʼ������Ҫȡ��
 * ִ���ļ�ͷ�����ݲ��������е���Ϣ�������������л���������������һ��shell������ִ
 * �нű��������ȶ�ȡִ���ļ���1�����ݵ����ٻ�����С������ƻ�������ݵ�ex�С�
 * ���ִ���ļ���ʼ�������ֽ����ַ���#!������˵��ִ���ļ���һ���ű��ı��ļ����������
 * �нű��ļ������Ǿ���Ҫִ�нű��ļ��Ľ��ͳ�������shell���򣩡�ͨ���ű��ļ��ĵ�
 * һ���ı�Ϊ��#! /bin/bash������ָ�������нű��ļ���Ҫ�Ľ��ͳ������з����Ǵӽű�
 * �ļ���1�У����ַ���#!������ȡ�����еĽ��ͳ�����������Ĳ��������еĻ�����Ȼ����
 * Щ�����ͽű��ļ����Ž�ִ���ļ�����ʱ�ǽ��ͳ��򣩵������в����ռ��С�����֮ǰ����
 * ��Ȼ��Ҫ�ȰѺ���ָ����ԭ�������в����ͻ����ַ����ŵ�128KB�ռ��У������ｨ������
 * �������в�����ŵ�����ǰ��λ�ô�����Ϊ��������ã���������ں�ִ�нű��ļ��Ľ���
 * ����������������úý��ͳ���Ľű��ļ����Ȳ�����ȡ�����ͳ����i�ڵ㲢��ת��
 * 204��ȥִ�н��ͳ�������������Ҫ��ת��ִ�й��Ĵ���204��ȥ�����������ȷ�ϲ���
 * ���˽ű��ļ�֮����Ҫ����һ����ֹ�ٴ�ִ������Ľű���������־sh��bang���ں����
 * �����иñ�־Ҳ������ʾ�����Ѿ����ú�ִ���ļ��������в�������Ҫ�ظ����á�	*/
	if (!(bh = bread (inode->i_dev, inode->i_zone[0])))
	{
		retval = -EACCES;
		goto exec_error2;
	}
	ex = *((struct exec *) bh->b_data);	/* read exec-header	��ȡִ��ͷ���� */
	if ((bh->b_data[0] == '#') && (bh->b_data[1] == '!') && (!sh_bang))
	{
		/*
		* This section does the #! interpretation.
		* Sorta complicated, but hopefully it will work. -TYT
		*/
		/*
		* �ⲿ�ִ����'#!'�Ľ��ͣ���Щ���ӣ���ϣ���ܹ����� -TYT
		*/

		char buf[1023], *cp, *interp, *i_name, *i_arg;
		unsigned long old_fs;

/* �����￪ʼ�����Ǵӽű��ļ�����ȡ���ͳ�����������������ѽ��ͳ����������ͳ���Ĳ���
 * �ͽű��ļ�����Ϸ��뻷���������С����ȸ��ƽű��ļ�ͷ1���ַ���#!��������ַ�����buf
 * �У����к��нű����ͳ�����������/bin/sh)��Ҳ���ܻ��������ͳ���ļ���������Ȼ���
 * buf�е����ݽ��д���ɾ����ʼ�Ŀո��Ʊ����	*/
		strncpy (buf, bh->b_data + 2, 1022);

		brelse (bh);				/* �ͷŻ���鲢�Żؽű��ļ�i�ڵ㡣	*/
		iput (inode);
		buf[1022] = '\0';
		if (cp = strchr (buf, '\n'))
		{
			*cp = '\0';				/* ��1�����з����ɶ�LL��ȥ���ո��Ʊ����	*/
			for (cp = buf; (*cp == ' ') || (*cp == '\t'); cp++);
		}
		if (!cp || *cp == '\0')
		{							/* ������û���������ݣ������	*/
			retval = -ENOEXEC;		/* No interpreter name found */
			goto exec_error1;		/* û���ҵ��ű����ͳ�����	*/
		}
/* ��ʱ���ǵõ��˿�ͷ�ǽű����ͳ�������һ�����ݣ��ַ�����������������С�����ȡ��һ��
 * �ַ�������Ӧ���ǽ��ͳ���������ʱi��nameָ������ơ������ͳ����������ַ���������Ӧ
 * ���ǽ��ͳ���Ĳ�������������i��argָ��ô���	*/
		interp = i_name = cp;
		i_arg = 0;
		for (; *cp && (*cp != ' ') && (*cp != '\t'); cp++)
		{
			if (*cp == '/')
				i_name = cp + 1;
		}
		if (*cp)
		{
			*cp++ = '\0';			/* ���ͳ�����β���NULL�ַ�	*/
			i_arg = cp;				/* i_argָ����ͳ������	*/
		}
/*
* OK, we've parsed out the interpreter name and
* (optional) argument.
*/
/*
* OK�������Ѿ����������ͳ�����ļ����Լ�(��ѡ��)������
*/
/* ��������Ҫ��������������Ľ��ͳ�����i��name�������i��arg�ͽű��ļ�����Ϊ���ͳ�
 * ��Ĳ����Ž������Ͳ������С���������������Ҫ�Ѻ����ṩ��ԭ��һЩ�����ͻ����ַ���
 * �ȷŽ�ȥ��Ȼ���ٷ�������������ġ�������������в�����˵�����ԭ���Ĳ�����"-argl
 * -arg2�������ͳ������ǡ�bash����������ǡ�-iargl -iarg2�����ű��ļ�������ԭ����ִ����
 * ��������"example.sh"����ô�ڷ�������Ĳ���֮���µ�������������������
 * ��bash -iargl -iarg2 example.sh -argl -arg2����
 * �������ǰ�sh��bang��־���ϣ�Ȼ��Ѻ��������ṩ��ԭ�в����ͻ����ַ������뵽�ռ��С�
 * �����ַ����Ͳ��������ֱ���envc��argc-1�����ٸ��Ƶ�һ��ԭ�в�����ԭ����ִ���ļ�
 * ����������Ľű��ļ�����[[??���Կ�����ʵ�������ǲ���Ҫȥ���д���ű��ļ���������
 * ����ȫ���Ը���argc������������ԭ��ִ���ļ����������ڵĽű��ļ���������Ϊ��λ��ͬ
 * һ��λ����]]��ע�⣡����ָ��P���Ÿ�����Ϣ���Ӷ�����С��ַ�����ƶ������������
 * ���ƴ�����ִ����󣬻�����������Ϣ��λ�ڳ��������в�������Ϣ����Ϸ�������Pָ��
 * ����ĵ�1����������copy��strings()���һ��������0��ָ�������ַ������û��ռ䡣	*/

		if (sh_bang++ == 0)
		{
			p = copy_strings (envc, envp, page, p, 0);
			p = copy_strings (--argc, argv + 1, page, p, 0);
		}
		/*
		* Splice in (1) the interpreter's name for argv[0]
		* (2) (optional) argument to interpreter
		* (3) filename of shell script
		*
		* This is done in reverse order, because of how the
		* user environment and arguments are stored.
		*/
		/*
		* ƴ�� (1) argv[0]�зŽ��ͳ��������
		* (2) (��ѡ��)���ͳ���Ĳ���
		* (3) �ű����������
		*
		* ������������д���ģ��������û������Ͳ����Ĵ�ŷ�ʽ��ɵġ�
		*/
/* �������������ƽű��ļ��������ͳ���Ĳ����ͽ��ͳ����ļ����������ͻ����ռ��С�
 * ���������ó����룬��ת��exec_errorl�����⣬���ڱ����������ṩ�Ľű��ļ���
 * filename���û��ռ䣬�����︳��copy_strings()�Ľű��ļ���ָ�����ں˿ռ䣬���
 * ��������ַ������������һ���������ַ�����Դ��־����Ҫ�����ó�1�����ַ�����
 * �ں˿ռ䣬��copy_strings()�����һ������Ҫ���ó�2��������ĵ�276��279�С�	*/
		p = copy_strings (1, &filename, page, p, 1);
		argc++;
		if (i_arg)
		{								/* ���ƽ��ͳ���Ķ��������	*/
			p = copy_strings (1, &i_arg, page, p, 2);
			argc++;
		}
		p = copy_strings (1, &i_name, page, p, 2);
		argc++;
		if (!p)
		{
			retval = -ENOMEM;
			goto exec_error1;
		}
		/*
		* OK, now restart the process with the interpreter's inode.
		*/
		/*
		* OK������ʹ�ý��ͳ����i �ڵ��������̡�
		*/
/* �������ȡ�ý��ͳ����i�ڵ�ָ�룬Ȼ����ת��204��ȥִ�н��ͳ���Ϊ�˻�ý��ͳ�
 * ���i�ڵ㣬������Ҫʹ��namei()���������Ǹú�����ʹ�õĲ������ļ������Ǵ��û���
 * �ݿռ�õ��ģ����ӶμĴ���fs��ָ�ռ���ȡ�á�����ڵ���namei()����֮ǰ������Ҫ
 * ����ʱ��fsָ���ں����ݿռ䣬���ú����ܴ��ں˿ռ�õ����ͳ�����������namei()
 * ���غ�ָ�fs��Ĭ�����á����������������ʱ����ԭfs�μĴ�����ԭָ���û����ݶΣ�
 * ��ֵ���������ó�ָ���ں����ݶΣ�Ȼ��ȡ���ͳ����i�ڵ㡣֮���ٻָ�fs��ԭֵ����
 * ��ת��restart��interp(204��)�����´����µ�ִ���ļ�һ�ű��ļ��Ľ��ͳ���	*/
		old_fs = get_fs ();
		set_fs (get_ds ());
		if (!(inode = namei (interp)))
		{							/* get executables inode */
			set_fs (old_fs);		/* ȡ�ý��ͳ����i�ڵ�		*/
			retval = -ENOENT;
			goto exec_error1;
		}
		set_fs (old_fs);
		goto restart_interp;		/* 204 ��	*/
	}
/* ��ʱ������е�ִ���ļ�ͷ�ṹ�����Ѿ����Ƶ��� ex�С��������ͷŸû���飬����ʼ��
 * ex�е�ִ��ͷ��Ϣ�����жϴ�������Linux0.11�ں���˵������֧��ZMAGICִ���ļ���
 * ʽ������ִ���ļ����붼���߼���ַ0��ʼִ�У���˲�֧�ֺ��д���������ض�λ��Ϣ��
 * ִ���ļ�����Ȼ�����ִ���ļ�ʵ��̫�����ִ���ļ���ȱ��ȫ����ô����Ҳ������������
 * ��˶��������������ִ�г������ִ���ļ���������ҳ��ִ���ļ���ZMAGIC�������ߴ�
 * ��������ض�λ���ֳ��Ȳ�����0������(�����+���ݶ�+��)���ȳ���50MB������ִ���ļ�
 * ����С�ڣ������+���ݶ�+���ű���+ִ��ͷ���֣����ȵ��ܺ͡�	*/
	brelse (bh);
	if (N_MAGIC (ex) != ZMAGIC || ex.a_trsize || ex.a_drsize ||
			ex.a_text + ex.a_data + ex.a_bss > 0x3000000 ||
			inode->i_size < ex.a_text + ex.a_data + ex.a_syms + N_TXTOFF (ex))
	{
		retval = -ENOEXEC;
		goto exec_error2;
	}
/* ���⣬���ִ���ļ��д��뿪ʼ��û��λ��1��ҳ�棨1024�ֽڣ��߽紦����Ҳ����ִ�С�
 * ��Ϊ����ҳ��Demand paging������Ҫ�����ִ���ļ�����ʱ��ҳ��Ϊ��λ�����Ҫ��ִ��
 * �ļ�ӳ���д�������ݶ���ҳ��߽紦��ʼ��	*/
	if (N_TXTOFF (ex) != BLOCK_SIZE)
	{
		printk ("%s: N_TXTOFF != BLOCK_SIZE. See a.out.h.", filename);
		retval = -ENOEXEC;
		goto exec_error2;
	}
/* ���sh_bang��־û�����ã�����ָ�������������в����ͻ����ַ����������ͻ����ռ�
 * �С���sh_bang��־�Ѿ����ã�������ǽ����нű����ͳ��򣬴�ʱ��������ҳ���Ѿ����ƣ�
 * �����ٸ��ơ�ͬ������sh_bangû����λ����Ҫ���ƵĻ�����ô��ʱָ��p ���Ÿ�����Ϣ��
 * �Ӷ�����С��ַ�����ƶ���������������ƴ�����ִ����󣬻�����������Ϣ��λ�ڳ���
 * ��������Ϣ����Ϸ�������Pָ�����ĵ�1������������ʵ�ϣ�p��128KB�����ͻ�����
 * ���е�ƫ��ֵ��������p=0�����ʾ��������������ռ�ҳ���Ѿ���ռ�������ɲ����ˡ�	*/
	if (!sh_bang)
	{
		p = copy_strings (envc, envp, page, p, 0);
		p = copy_strings (argc, argv, page, p, 0);
		if (!p)
		{
			retval = -ENOMEM;
			goto exec_error2;
		}
	}
/* OK, This is the point of no return */
/* OK�����濪ʼ��û�з��صĵط��� */
/* ǰ��������Ժ��������ṩ����Ϣ����Ҫ����ִ���ļ��������в����ͻ����ռ���������ã���
 * ��û��Ϊִ���ļ�����ʲôʵ���ԵĹ���������û������Ϊִ���ļ���ʼ����������ṹ��Ϣ��
 * ����ҳ��ȹ������������Ǿ�������Щ����������ִ���ļ�ֱ��ʹ�õ�ǰ���̵ġ����ǡ�������ǰ
 * ���̽��������ִ���ļ��Ľ��̣����������Ҫ�����ͷŵ�ǰ����ռ�õ�ĳЩϵͳ��Դ��������
 * ��ָ�����Ѵ��ļ���ռ�õ�ҳ����ڴ�ҳ��ȡ�Ȼ�����ִ���ļ�ͷ�ṹ��Ϣ�޸ĵ�ǰ����ʹ
 * �õľֲ���������LDT�������������ݣ��������ô���κ����ݶ����������޳���������ǰ�洦
 * ��õ���e_uid��e_gid����Ϣ�����ý�������ṹ����ص��ֶΡ�����ִ�б���ϵͳ���ó�
 * ��ķ��ص�ַeip[]ָ��ִ���ļ��д������ʼλ�ô�����������ϵͳ�����˳����غ�ͻ�ȥ��
 * ����ִ���ļ��Ĵ����ˡ�ע�⣬��Ȼ��ʱ��ִ���ļ���������ݻ�û�д��ļ��м��ص��ڴ��У�
 * ��������ͻ������Ѿ���copy_strings()��ʹ��get_free_page()�����������ڴ�ҳ����
 * �����ݣ�����change_ldt()������ʹ��put_page() �ŵ��˽����߼��ռ��ĩ�˴������⣬
 * �� create_tables()��Ҳ���������û�ջ�ϴ�Ų����ͻ���ָ��������ȱҳ�쳣���Ӷ���
 * ��������Ҳ��ʹ�Ϊ�û�ջ�ռ�ӳ�������ڴ�ҳ��
 * 
 * �����������ȷŻؽ���ԭִ�г����i�ڵ㣬�����ý���executable�ֶ�ָ����ִ���ļ���i
 * �ڵ㡣Ȼ��λԭ���̵������źŴ�������[[������SIG��IGN������븴λ�������322��
 * 323��֮��Ӧ�����1�� if ��䣺if(current->sa[i].sa��handler != SIG��IGN)]]
 * �ٸ����趨��ִ��ʱ�ر��ļ���� ��close_on_exec��λͼ��־���ر�ָ���Ĵ��ļ�����
 * ��λ�ñ�־��	*/
	if (current->executable)
		iput (current->executable);
	current->executable = inode;
	for (i = 0; i < 32; i++)
		current->sigaction[i].sa_handler = NULL;
	for (i = 0; i < NR_OPEN; i++)
		if ((current->close_on_exec >> i) & 1)
			sys_close (i);
	current->close_on_exec = 0;
/* Ȼ����ݵ�ǰ����ָ���Ļ���ַ���޳����ͷ�ԭ������Ĵ���κ����ݶ�����Ӧ���ڴ�ҳ��
 * ָ���������ڴ�ҳ�漰ҳ������ʱ��ִ���ļ���û��ռ�����ڴ����κ�ҳ�棬����ڴ�
 * ��������������ִ���ļ�����ʱ�ͻ�����ȱҳ�쳣�жϣ���ʱ�ڴ������򼴻�ִ��ȱҳ��
 * ���Ϊ��ִ���ļ������ڴ�ҳ����������ҳ������Ұ����ִ���ļ�ҳ������ڴ��С�
 * ������ϴ�����ʹ����Э��������ָ����ǵ�ǰ���̣������ÿգ�����λʹ����Э������
 * �ı�־��	*/
	free_page_tables (get_base (current->ldt[1]), get_limit (0x0f));
	free_page_tables (get_base (current->ldt[2]), get_limit (0x17));
	if (last_task_used_math == current)
		last_task_used_math = NULL;
	current->used_math = 0;
/* Ȼ�����Ǹ�����ִ���ļ�ͷ�ṹ�еĴ��볤���ֶ�a��text��ֵ�޸ľֲ�������������ַ��
 * ���޳�������128KB�Ĳ����ͻ����ռ�ҳ����������ݶ�ĩ�ˡ�ִ���������֮��p��ʱ
 * ���ĳ������ݶ���ʼ��Ϊԭ���ƫ��ֵ������ָ������ͻ����ռ����ݿ�ʼ��������ת����
 * Ϊջָ��ֵ��Ȼ������ڲ�����create_tablesO��ջ�ռ��д��������Ͳ�������ָ���
 * �������mainO��Ϊ����ʹ�ã������ظ�ջָ�롣	*/

	p += change_ldt (ex.a_text, page) - MAX_ARG_PAGES * PAGE_SIZE;
	p = (unsigned long) create_tables ((char *) p, argc, envc);
/* �������޸Ľ��̸��ֶ�ֵΪ��ִ���ļ�����Ϣ��
 * �����������ṹ����β�ֶ� end��code �� ִ���ļ��Ĵ���γ���a��text;
 * ������β�ֶ� end��data �� ִ���ļ��Ĵ���γ��ȼ����ݶγ��ȣ�a��data + a��text��;
 * ������̶ѽ�β�ֶ� brk �� a��text + a��data + a��bss��
 * brk����ָ�����̵�ǰ���ݶΣ�����δ��ʼ�����ݲ��֣�ĩ��λ�á�Ȼ�����ý���ջ��ʼ��
 * ��Ϊջָ������ҳ�棬���������ý��̵���Ч�û�id����Ч��id��	*/
	current->brk = ex.a_bss +
		(current->end_data = ex.a_data + (current->end_code = ex.a_text));
	current->start_stack = p & 0xfffff000;
	current->euid = e_uid;
	current->egid = e_gid;
/* ���ִ���ļ���������ݳ��ȵ�ĩ�˲���ҳ��߽��ϣ������󲻵�1ҳ���ȵ��ڴ�ռ��
 * ʼ��Ϊ�㡣[[ʵ��������ʹ�õ���ZMAGIC��ʽ��ִ���ļ�����˴���κ����ݶγ��Ⱦ���
 * ҳ������������ȣ����343�в���ִ�У�����i&0xfff�� = 0����δ�����Linux�ں���ǰ
 * �汾�Ĳ�����]]	*/
	i = ex.a_text + ex.a_data;
	while (i & 0xfff)
		put_fs_byte (0, (char *) (i++));
/* ���ԭ����ϵͳ�жϵĳ����ڶ�ջ�ϵĴ���ָ���滻Ϊָ����ִ�г������ڵ㣬����ջ
 * ָ���滻Ϊ��ִ���ļ���ջָ�롣�˺󷵻�ָ�������Щջ���ݲ�ʹ��CPUȥִ����ִ��
 * �ļ�����˲��᷵�ص�ԭ����ϵͳ�жϵĳ�����ȥ�ˡ�	*/
	eip[0] = ex.a_entry;	/* eip, magic happens :-)	eip��ħ���������� */
	eip[3] = p;				/* stack pointer			esp����ջָ�� */
	return 0;
exec_error2:
	iput (inode);				/* �Ż� i �ڵ㡣	*/
exec_error1:
	for (i = 0; i < MAX_ARG_PAGES; i++)
		free_page (page[i]);	/* �ͷŴ�Ų����ͻ��������ڴ�ҳ�档	*/
	return (retval);			/* ���س����롣	*/
}
