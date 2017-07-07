/*
* linux/fs/buffer.c
*
* (C) 1991 Linus Torvalds
*/

/*
* 'buffer.c' implements the buffer-cache functions. Race-conditions have
* been avoided by NEVER letting a interrupt change a buffer (except for the
* data, of course), but instead letting the caller do it. NOTE! As interrupts
* can wake up a caller, some cli-sti sequences are needed to check for
* sleep-on-calls. These should be extremely quick, though (I hope).
*/
/*
* ��buffer.c�� ����ʵ�ֻ��������ٻ��湦�ܡ� ͨ�������жϹ��̸ı仺������ �����õ�
* ������ִ�У������˾�����������Ȼ���ı��������⣩��	ע�⣡�����жϿ��Ի���һ����
* ���ߣ���˾���Ҫ�����ж�ָ�cli-sti�����������ȴ����÷��ء�* ����Ҫ�ǳ��ؿ�
* (��ϣ��������)��
*/
/*
* NOTE! There is one discordant note here: checking floppies for
* disk change. This is where it fits best, I think, as it should
* invalidate changed floppy-disk-caches.
*/
/*
*ע��!��һ������Ӧ����������:��������Ƿ�����������������Ƿ���
*�ó�����õĵط��ˣ���Ϊ����Ҫʹ���������̻���ʧЧ��
*/

#include <stdarg.h>		/* ��׼����ͷ�ļ����Ժ����ʽ������������б���Ҫ˵����-��
						 * ����(va_list)��������(va_start, va_arg ��va_end)��
						 * ����vsprintf��vprintf��vfprintf ������	*/

#include <linux/config.h>	/* �ں�����ͷ�ļ�������������Ժ�Ӳ�����ͣ�HD_TYPE����ѡ�	*/
#include <linux/sched.h>	/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 ����
							 * �ݣ�����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/

#include <linux/kernel.h>	/* �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣	*/
#include <asm/system.h>		/* ϵͳͷ�ļ������������û��޸�������/�ж��ŵȵ�Ƕ��ʽ���ꡣ	*/
#include <asm/io.h>			/* io ͷ�ļ�������Ӳ���˿�����/���������䡣	*/

/* ����end���ɱ���ʱ�����ӳ���Id���ɣ����ڱ����ں˴����ĩ�ˣ���ָ���ں�ģ��ĩ��
 * λ�ã��μ�ͼ12-15��Ҳ���Դӱ����ں�ʱ���ɵ�System.map�ļ��в����������������
 * �����ٻ�������ʼ���ں˴���ĩ��λ�á�
 * ��33���ϵ�buffer_wait�����ǵȴ����л�����˯�ߵ��������ͷָ�롣���뻺���ͷ
 * ���ṹ��b��waitָ������ò�ͬ������������һ����������������ϵͳȱ�����ÿ��л�
 * ���ʱ����ǰ����ͻᱻ��ӵ�buffer_wait˯�ߵȴ������С���b_wait����ר�Ź��ȴ�
 * ָ������飨��b_wait��Ӧ�Ļ���飩������ʹ�õĵȴ�����ͷָ�롣
*/
extern int end;				/* �����ӳ���ld ���ɵı�������ĩ�˵ı�����	*/
struct buffer_head *start_buffer = (struct buffer_head *) &end;
struct buffer_head *hash_table[NR_HASH];		/* NR_HASH = 307 �	*/
static struct buffer_head *free_list;			/* ���л��������ͷָ�롣	*/
static struct task_struct *buffer_wait = NULL;	/* �ȴ����л�����˯�ߵ�������С�	*/

/* ���涨��ϵͳ�������к��еĻ������������NR��BUFFERS��һ��������linux/fs.hͷ
 * �ļ���34�еĺ꣬��ֵ���Ǳ�����nr_buffers��������fs.h�ļ���166������Ϊȫ�ֱ�����
 * ��д����ͨ������һ�������ƣ�Linus������д������Ϊ�����������д�����������ر�ʾ
 * nr_buffers��һ�����ں˳�ʼ��֮���ٸı�ġ��������������ں���Ļ�������ʼ������
 * buffer_init()�б����ã��� 371 �У���
 */
int NR_BUFFERS = 0;

/* �ȴ�ָ��������������	*/
/* ���ָ���Ļ����bh�Ѿ��������ý��̲����жϵ�˯���ڸû����ĵȴ�����b_wait�С�
 * �ڻ�������ʱ����ȴ������ϵ����н��̽������ѡ���Ȼ���ڹر��жϣ�cli��֮��ȥ˯
 * �ߵģ���������������Ӱ����������������������Ӧ�жϡ���Ϊÿ�����̶����Լ���TSS��
 * �б����˱�־�Ĵ���EFLAGS��ֵ�������ڽ����л�ʱCPU�е�ǰEFLAGS��ֵҲ��֮�ı䡣
 * ʹ��sleep_on()����˯��״̬�Ľ�����Ҫ��wake_Up()��ȷ�ػ��ѡ�
*/
static inline void wait_on_buffer (struct buffer_head *bh)
{
	cli ();					/* ���жϡ�	*/
	while (bh->b_lock)		/* ����ѱ�����������̽���˯�ߣ��ȴ��������	*/
		sleep_on (&bh->b_wait);
	sti ();					/* ���жϡ�	*/
}

/* �豸����ͬ����
 * ͬ���豸���ڴ���ٻ��������ݡ����У�sync��inodes()������inode.c��59�С�
 */
int sys_sync (void)
{
	int i;
	struct buffer_head *bh;

/* ���ȵ���i�ڵ�ͬ�����������ڴ�i�ڵ���������޸Ĺ���i�ڵ�д����ٻ����С�Ȼ��
 * ɨ�����и��ٻ����������ѱ��޸ĵĻ�������д�����󣬽�����������д�����У�����
 * ���ٻ����е��������豸�е�ͬ����	*/
	sync_inodes ();				/* write out inodes into buffers ��i �ڵ�д����ٻ��� */
	bh = start_buffer;			/* bhָ�򻺳�����ʼ����	*/
	for (i = 0; i < NR_BUFFERS; i++, bh++)
	{
		wait_on_buffer (bh);	/* �ȴ�����������������������Ļ�����	*/
		if (bh->b_dirt)
		ll_rw_block (WRITE, bh);/* ����д�豸������	*/
	}
	return 0;
}

/* ��ָ���豸���и��ٻ����������豸�����ݵ�ͬ��������
 * �ú��������������ٻ����������л���顣����ָ���豸dev�Ļ���飬���������ѱ��޸�
 * ����д�����У�ͬ����������Ȼ����ڴ���i�ڵ������д����ٻ����С�֮���ٶ�ָ����
 * ��devִ��һ����������ͬ��д�̲�����
 */
int
sync_dev (int dev)
{
	int i;
	struct buffer_head *bh;

/* ���ȶԲ���ָ�����豸ִ������ͬ�����������豸�ϵ���������ٻ������е�����ͬ����
 * ������ɨ����ٻ����������л���飬��ָ���豸dev�Ļ���飬�ȼ�����Ƿ��ѱ�������
 * ���ѱ�������˯�ߵȴ��������Ȼ�����ж�һ�θû�����Ƿ���ָ���豸�Ļ���鲢��
 * ���޸Ĺ���b��dirt��־��λ�������ǾͶ���ִ��д�̲�������Ϊ������˯���ڼ�û���� 
 * �п����ѱ��ͷŻ��߱�Ų�����ã������ڼ���ִ��ǰ��Ҫ�ٴ��ж�һ�¸û�����Ƿ���
 * ָ���豸�Ļ���飬
 */
	bh = start_buffer;			/* bhָ�򻺳�����ʼ����	*/
	for (i = 0; i < NR_BUFFERS; i++, bh++)
	{
		if (bh->b_dev != dev)	/* �����豸dev�Ļ�����������	*/
			continue;
		wait_on_buffer (bh);	/* �ȴ�����������������������Ļ�����	*/
		if (bh->b_dev == dev && bh->b_dirt)
			ll_rw_block (WRITE, bh);
	}
/* �ٽ�i�ڵ�����д����ٻ��塣��i�ڵ��inode_table�е�inode�뻺���е���Ϣͬ����	*/
	sync_inodes ();	
/* Ȼ���ڸ��ٻ����е����ݸ���֮���ٰ��������豸�е�����ͬ���������������ͬ������
 * ��Ϊ������ں�ִ��Ч�ʡ���һ�黺����ͬ�������������ں�����ࡰ��顱��ɾ���ʹ��
 * i�ڵ��ͬ�������ܹ���Чִ�С����λ�����ͬ�����������Щ����i�ڵ�ͬ���������ֱ�
 * ��Ļ�������豸������ͬ����	*/
	bh = start_buffer;
	for (i = 0; i < NR_BUFFERS; i++, bh++)
	{
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer (bh);
			if (bh->b_dev == dev && bh->b_dirt)
				ll_rw_block (WRITE, bh);
		}
	return 0;
}

/* ʹָ���豸�ڸ��ٻ������е�������Ч��	*/
/* ɨ����ٻ����е����л���飬����ָ���豸�Ļ���������λ����Ч(����)��־�����޸ı�־��	*/
void inline
invalidate_buffers (int dev)
{
	int i;
	struct buffer_head *bh;

	bh = start_buffer;
	for (i = 0; i < NR_BUFFERS; i++, bh++)
	{
		if (bh->b_dev != dev)	/* �������ָ���豸�Ļ���飬��	*/
			continue;			/* ����ɨ����һ�顣	*/

		wait_on_buffer (bh);	/* �ȴ��û���������������ѱ���������	*/

/* ���ڽ���ִ�й�˯�ߵȴ���������Ҫ���ж�һ�»������Ƿ���ָ���豸�ġ�	*/

		if (bh->b_dev == dev)
			bh->b_uptodate = bh->b_dirt = 0;
	}
}

/*
* This routine checks whether a floppy has been changed, and
* invalidates all buffer-cache-entries in that case. This
* is a relatively slow routine, so we have to try to minimize using
* it. Thus it is called only upon a 'mount' or 'open'. This
* is the best way of combining speed and utility, I think.
* People changing diskettes in the middle of an operation deserve
* to loose :-)
*
* NOTE! Although currently this is only for floppies, the idea is
* that any additional removable block-device will use this routine,
* and that mount/open needn't know that floppies/whatever are
* special.
*/
/*
* ���ӳ�����һ�������Ƿ��Ѿ�������������Ѿ�������ʹ���ٻ������������
* ��Ӧ�����л�������Ч��
* ���ӳ��������˵��������������Ҫ������ʹ������	
* ���Խ���ִ��'mount'��'open'ʱ�ŵ�������
* �������ǽ��ٶȺ�ʵ�������ϵ�
* ��÷�����
* ���ڲ������̵��и������̣��ᵼ�����ݵĶ�ʧ�����Ǿ�����ȡ?��
*
* ע�⣡����Ŀǰ���ӳ�����������̣��Ժ��κο��ƶ����ʵĿ��豸����ʹ�ø�
* ����mount/open �����ǲ���Ҫ֪���Ƿ������̻�����ʲô������ʵġ�
*/

/* �������Ƿ����������Ѹ�����ʹ��Ӧ���ٻ�������Ч��	*/
void
check_disk_change (int dev)
{
	int i;

/* ���ȼ��һ���ǲ��������豸����Ϊ���ڽ�֧�����̿��ƶ����ʡ�����������˳���Ȼ��
 * ���������Ƿ��Ѹ��������û�����˳���floppy��change()��blk��drv/floppy.c��139�С�	*/

	if (MAJOR (dev) != 2)
		return;
/* �����Ѿ������������ͷŶ�Ӧ�豸��i�ڵ�λͼ���߼���λͼ��ռ�ĸ��ٻ���������ʹ��
 * �豸��i�ڵ�����ݿ���Ϣ��ռ��ĸ��ٻ������Ч��	*/

	if (!floppy_change (dev & 0x03))
		return;
/* �����Ѿ������������ͷŶ�Ӧ�豸��i �ڵ�λͼ���߼���λͼ��ռ�ĸ��ٻ�������
 * ��ʹ���豸��* i �ڵ�����ݿ���Ϣ��ռ�ĸ��ٻ�������Ч��	*/

	for (i = 0; i < NR_SUPER; i++)
		if (super_block[i].s_dev == dev)
			put_super (super_block[i].s_dev);
	invalidate_inodes (dev);
	invalidate_buffers (dev);
}

/* �������д�����hash (ɢ��)���������hash����ļ���ꡣ
 * hash�����Ҫ�����Ǽ��ٲ��ұȽ�Ԫ�������ѵ�ʱ�䡣ͨ����Ԫ�صĴ洢λ����ؼ���֮��
 * ����һ����Ӧ��ϵ��hash�����������ǾͿ���ֱ��ͨ�������������̲�ѯ��ָ����Ԫ�ء���
 * ��hash������ָ��������Ҫ�Ǿ���ȷ��ɢ�е��κ�������ĸ��ʻ�����ȡ����������ķ���
 * �ж��֣�����Linux0.11��Ҫ�����˹ؼ��ֳ�������������Ϊ����Ѱ�ҵĻ����������������
 * ���豸��dev�ͻ�����block�������Ƶ�hash�����϶���Ҫ�����������ؼ�ֵ����������
 * �ؼ��ֵ�������ֻ�Ǽ���ؼ�ֵ��һ�ַ������ٶԹؼ�ֵ����MOD����Ϳ��Ա�֤��������
 * ��õ���ֵ�����ں��������Χ�ڡ�
 */
#define _hashfn(dev,block) (((unsigned)(dev^block))%NR_HASH)
#define hash(dev,block) hash_table[_hashfn(dev,block)]

/* ��hash ���кͿ��л������������ָ���Ļ���顣	*/

static inline void
remove_from_queues (struct buffer_head *bh)
{
/* remove from hash-queue */
/* ��hash �������Ƴ������ */
	if (bh->b_next)
		bh->b_next->b_prev = bh->b_prev;
	if (bh->b_prev)
		bh->b_prev->b_next = bh->b_next;
/* ����û������Ǹö��е�ͷһ���飬����hash ��Ķ�Ӧ��ָ�򱾶����е���һ����������	*/
	if (hash (bh->b_dev, bh->b_blocknr) == bh)
		hash (bh->b_dev, bh->b_blocknr) = bh->b_next;
/* remove from free list */
/* �ӿ��л����������Ƴ������ */
	if (!(bh->b_prev_free) || !(bh->b_next_free))
		panic ("Free block list corrupted");
	bh->b_prev_free->b_next_free = bh->b_next_free;
	bh->b_next_free->b_prev_free = bh->b_prev_free;
/* �����������ͷָ�򱾻�������������ָ����һ��������	*/
	if (free_list == bh)
		free_list = bh->b_next_free;
}

/* ��ָ�������������������β������hash �����С�	*/
static inline void insert_into_queues (struct buffer_head *bh)
{
/* put at end of free list */
/* ���ڿ�������ĩβ�� */
	bh->b_next_free = free_list;
	bh->b_prev_free = free_list->b_prev_free;
	free_list->b_prev_free->b_next_free = bh;
	free_list->b_prev_free = bh;
/* put the buffer in new hash-queue if it has a device */
/* ����û�����Ӧһ���豸�����������hash������*/
/* ��ע�⵱hash��ĳ���1�β�����ʱ��hash()����ֵ�϶�ΪNULL����˴�ʱ��161����
 * �õ���bh����>b_next�϶���NULL�����Ե�163����Ӧ����bh����>b_next��ΪNULLʱ���ܸ�
 * b��prev��bhֵ������163��ǰӦ�������жϡ�if (bh����>b_next]�����ô���0.96���
 * �ű�������
 */
	bh->b_prev = NULL;
	bh->b_next = NULL;
	if (!bh->b_dev)
		return;
	bh->b_next = hash (bh->b_dev, bh->b_blocknr);
	hash (bh->b_dev, bh->b_blocknr) = bh;
	bh->b_next->b_prev = bh;			/* �˾�ǰӦ��� ��if (bh-)b��next)�� �жϡ�	*/
}

/* ����hash���ڸ��ٻ�����Ѱ�Ҹ����豸��ָ����ŵĻ������顣
 * ����ҵ��򷵻ػ��������ָ�룬���򷵻�NULL��
 */
static struct buffer_head * find_buffer (int dev, int block)
{
	struct buffer_head *tmp;

/* ����hash��Ѱ��ָ���豸�źͿ�ŵĻ���顣	*/
	for (tmp = hash (dev, block); tmp != NULL; tmp = tmp->b_next)
		if (tmp->b_dev == dev && tmp->b_blocknr == block)
			return tmp;
	return NULL;
}

/*
* Why like this, I hear you say... The reason is race-conditions.
* As we don't lock buffers (unless we are readint them, that is),
* something might happen to it while we sleep (ie a read-error
* will force it bad). This shouldn't really happen currently, but
* the code is ready.
*/
/*
* ����Ϊʲô���������ӵģ�����������... ԭ���Ǿ���������	
* ��������û�ж�
* �����������������������ڶ�ȡ�����е����ݣ�����ô�����ǣ����̣�˯��ʱ
* ���������ܻᷢ��һЩ���⣨����һ�������󽫵��¸û�����������
* Ŀǰ�������ʵ�����ǲ��ᷢ���ģ�������Ĵ����Ѿ�׼�����ˡ�
*/

struct buffer_head *
get_hash_table (int dev, int block)
{
	struct buffer_head *bh;

	for (;;)
		{
/* �ڸ��ٻ�����Ѱ�Ҹ����豸��ָ����Ļ����������û���ҵ��򷵻�NULL���˳���	*/

			if (!(bh = find_buffer (dev, block)))
				return NULL;
/* �Ըû�����������ü��������Ȼ�������������ѱ������������ھ�����˯��״̬��
 * ����б�Ҫ����֤�û�������ȷ�ԣ������ػ����ͷָ�롣	*/
			bh->b_count++;
			wait_on_buffer (bh);
			if (bh->b_dev == dev && bh->b_blocknr == block)
	return bh;
/* ����û������������豸�Ż�����˯��ʱ�����˸ı䣬�������������ü���������Ѱ�ҡ�	*/

			bh->b_count--;
		}
}

/*
* Ok, this is getblk, and it isn't very clear, again to hinder
* race-conditions. Most of the code is seldom used, (ie repeating),
* so it should be much more efficient than it looks.
*
* The algoritm is changed: hopefully better, and an elusive bug removed.
*/
/*
* OK��������getblk �������ú������߼������Ǻ�������ͬ��Ҳ����ΪҪ����
* �����������⡣
* ���д󲿷ִ�������õ���(�����ظ��������)�������Ӧ��
* �ȿ���ȥ��������Ч�öࡣ
* �㷨�Ѿ����˸ı䣺ϣ���ܸ��ã�����һ��������ĥ�Ĵ����Ѿ�ȥ����
*/

/* ����궨������ͬʱ�жϻ��������޸ı�־��������־�����Ҷ����޸ı�־��Ȩ��Ҫ��������־��	*/
#define BADNESS(bh) (((bh)->b_dirt<<1)+(bh)->b_lock)

/* ȡ���ٻ�����ָ���Ļ���顣
 * ���ָ�����豸�źͿ�ţ��Ļ������Ƿ��Ѿ��ڸ��ٻ����С����ָ�����Ѿ��ڸ��ٻ����У�
 * �򷵻ض�Ӧ������ͷָ���˳���������ڣ�����Ҫ�ڸ��ٻ���������һ����Ӧ�豸�źͿ�ŵ�
 * ���������Ӧ������ͷָ�롣	*/
struct buffer_head * getblk (int dev, int block)
{
	struct buffer_head *tmp, *bh;

repeat:
/* ����hash �����ָ�����Ѿ��ڸ��ٻ����У��򷵻ض�Ӧ������ͷָ�룬�˳���	*/

	if (bh = get_hash_table (dev, block))
		return bh;
/* ɨ��������ݿ�����Ѱ�ҿ��л�������	*/
/* ������tmp ָ���������ĵ�һ�����л�����ͷ��	*/

	tmp = free_list;
	do
		{
/* ����û���������ʹ�ã����ü���������0���������ɨ����һ�	*/

			if (tmp->b_count)
	continue;
/* �������ͷָ��bhΪ�գ�����tmp��ָ����ͷ�ı�־(�޸ġ�����)Ȩ��С��bhͷ��־��Ȩ
 * �أ�����bhָ��tmp�����ͷ�������tmp�����ͷ����������û���޸�Ҳû��������
 * ־��λ����˵����Ϊָ���豸�ϵĿ�ȡ�ö�Ӧ�ĸ��ٻ���飬���˳�ѭ�����������Ǿͼ���
 * ִ�б�ѭ���������ܷ��ҵ�һ��BADNESS0��С�Ļ���졣
 */

			if (!bh || BADNESS (tmp) < BADNESS (bh))
	{
		bh = tmp;
		if (!BADNESS (tmp))
			break;
	}
/* and repeat until we find something good	�ظ�����ֱ���ҵ��ʺϵĻ����� */
		}
	while ((tmp = tmp->b_next_free) != free_list);
/* ���ѭ����鷢�����л���鶼���ڱ�ʹ�ã����л�����ͷ�����ü�����>0���У���˯��
 * �ȴ��п��л������á����п��л�������ʱ�����̻ᱻ��ȷ�ػ��ѡ�Ȼ�����Ǿ���ת��
 * ������ʼ�����²��ҿ��л���顣	*/
	if (!bh)
		{
			sleep_on (&buffer_wait);
			goto repeat;			/* ��ת�� 210 �С�*/
		}
/* ִ�е����˵�������Ѿ��ҵ���һ���Ƚ��ʺϵĿ��л�����ˡ������ȵȴ��û���������
 * (����ѱ������Ļ�)�����������˯�߽׶θû������ֱ���������ʹ�õĻ���ֻ���ظ�����
 * Ѱ�ҹ��̡�	*/
	wait_on_buffer (bh);
	if (bh->b_count)				/* �ֱ�ռ���ˣ�	*/
		goto repeat;				/* ��ת�� 210 �С�*/
/* ����û������ѱ��޸ģ�������д�̣����ٴεȴ�������������ͬ���أ����û������ֱ�
 * ��������ʹ�õĻ���ֻ�����ظ�����Ѱ�ҹ��̡�	*/
	while (bh->b_dirt)
		{
			sync_dev (bh->b_dev);
			wait_on_buffer (bh);
			if (bh->b_count)		/* �ֱ�ռ���ˣ�	*/
	goto repeat;					/* ��ת�� 210 �С�*/
		}
/* NOTE!! While we slept waiting for this block, somebody else might */
/* already have added "this" block to the cache. check it */
/* ע�⣡��������Ϊ�˵ȴ��û�����˯��ʱ���������̿����Ѿ����û���� 
 * ��������ٻ����У�����Ҫ�Դ˽��м�顣
 */

/* �ڸ��ٻ���hash ���м��ָ���豸�Ϳ�Ļ������Ƿ��Ѿ��������ȥ��
 * ����ǵĻ������ٴ��ظ��������̡�	*/

		if (find_buffer (dev, block))
		goto repeat;
/* OK, FINALLY we know that this buffer is the only one of it's kind, */
/* and that it's unused (b_count=0), unlocked (b_lock=0), and clean */
/* OK����������֪���û�������ָ��������Ψһһ�飬���һ�û�б�ʹ��
 * (b_count=0)��δ������(b_lock=0)�������Ǹɾ��ģ�δ���޸ĵģ�*/
/* ����������ռ�ô˻������������ü���Ϊ1����λ�޸ı�־����Ч(����)��־��	*/
	bh->b_count = 1;
	bh->b_dirt = 0;
	bh->b_uptodate = 0;
/* ��hash���кͿ��п��������Ƴ��û�����ͷ���øû���������ָ���豸�����ϵ�ָ���顣
 * Ȼ����ݴ��µ��豸�źͿ�����²�����������hash������λ�ô��������շ��ػ���
 * ͷָ�롣	*/
	remove_from_queues (bh);
	bh->b_dev = dev;
	bh->b_blocknr = block;
/* Ȼ����ݴ��µ��豸�źͿ�����²�����������hash ������λ�ô���	
 * �����շ��ػ���ͷָ�롣	*/
	insert_into_queues (bh);
	return bh;
}

/* �ͷ�ָ������顣
 * �ȴ��û���������Ȼ�����ü����ݼ�1������ȷ�ػ��ѵȴ����л����Ľ��̡�
 */
void
brelse (struct buffer_head *buf)
{
	if (!buf)			/* �������ͷָ����Ч�򷵻ء�	*/

		return;
	wait_on_buffer (buf);
	if (!(buf->b_count--))
		panic ("Trying to free free buffer");
	wake_up (&buffer_wait);
}

/*
* bread() reads a specified block and returns the buffer that contains
* it. It returns NULL if the block was unreadable.
*/
/*
 * ���豸�϶�ȡָ�������ݿ鲢���غ������ݵĻ�������
 * ���ָ���Ŀ鲻����* �򷵻�NULL��	*/

/* ���豸�϶�ȡ���ݿ顣
 * �ú�������ָ�����豸��dev�����ݿ��block�������ڸ��ٻ�����������һ�黺��顣
 * ����û�������Ѿ���������Ч�����ݾ�ֱ�ӷ��ظû����ָ�룬����ʹ��豸�ж�ȡ
 * ָ�������ݿ鵽�û�����в����ػ����ָ�롣
 */
struct buffer_head * bread (int dev, int block)
{
	struct buffer_head *bh;

/* �ڸ��ٻ�����������һ�黺��顣�������ֵ��NULL�����ʾ�ں˳���ͣ����Ȼ������
 * �ж������Ƿ����п������ݡ�����û��������������Ч�ģ��Ѹ��µģ�����ֱ��ʹ�ã�
 * �򷵻ء�	*/
	if (!(bh = getblk (dev, block)))
		panic ("bread: getblk returned NULL\n");
	if (bh->b_uptodate)
		return bh;
/* �������Ǿ͵��õײ���豸��д11��^blockO�������������豸������Ȼ��ȴ�ָ��
 * ���ݿ鱻���룬���ȴ���������������˯������֮������û������Ѹ��£����ܷ��ػ���
 * ��ͷָ�룬�˳�������������豸����ʧ�ܣ������ͷŸû�����������NULL���˳���	*/
	ll_rw_block (READ, bh);
	wait_on_buffer (bh);
	if (bh->b_uptodate)
		return bh;
	brelse (bh);
	return NULL;
}

/* �����ڴ�顣	*/
/* ��from ��ַ����һ�����ݵ�to λ�á�	*/

#define COPYBLK(from,to) \
__asm__( "cld\n\t" \
"rep\n\t" \
"movsl\n\t" \
:: "c" (BLOCK_SIZE/4), "S" (from), "D" (to))
//: "cx", "di", "si")

/*
* bread_page reads four buffers into memory at the desired address. It's
* a function of its own, as there is some speed to be got by reading them
* all at the same time, not waiting for one to be read, and then another
* etc.
*/
/*
* bread_page һ�ζ��ĸ���������ݶ����ڴ�ָ���ĵ�ַ������һ�������ĺ�����
* ��Ϊͬʱ��ȡ�Ŀ���Ի���ٶ��ϵĺô������õ��Ŷ�һ�飬�ٶ�һ���ˡ�	
*/

/* ���豸��һ��ҳ�棨4������飩�����ݵ�ָ���ڴ��ַ����
 * ����address�Ǳ���ҳ�����ݵĵ�ַ��dev��ָ�����豸�ţ�b[4]�Ǻ���4���豸���ݿ��
 * �����顣�ú���������mm/memory.c�ļ���do��no��page()�����У���386�У���
*/
void
bread_page (unsigned long address, int dev, int b[4])
{
	struct buffer_head *bh[4];
	int i;

/* �ú���ѭ��ִ��4�Σ����ݷ�������b ���е�4����Ŵ��豸dev�ж�ȡһҳ���ݷŵ�ָ��
 * �ڴ�λ��address�������ڲ���b[i]��������Ч��ţ��������ȴӸ��ٻ�����ȡָ���豸
 * �Ϳ�ŵĻ���顣����������������Ч��δ���£���������豸������豸�϶�ȡ��Ӧ��
 * �ݿ顣����b[i]��Ч�Ŀ������ȥ�����ˡ���˱�������ʵ���Ը���ָ����b���еĿ��
 * �����ȡ1һ4�����ݿ顣	*/
	for (i = 0; i < 4; i++)
		if (b[i])				/* �������Ч��	*/
			{
	if (bh[i] = getblk (dev, b[i]))
		if (!bh[i]->b_uptodate)
			ll_rw_block (READ, bh[i]);
			}
		else
			bh[i] = NULL;
/* ���4��������ϵ�����˳���Ƶ�ָ����ַ�����ڽ��и��ƣ�ʹ�ã������֮ǰ����
 * ��Ҫ˯�ߵȴ��������������������Ļ��������⣬��Ϊ����˯�߹��ˣ��������ǻ���Ҫ
 * �ڸ���֮ǰ�ټ��һ�»�����е������Ƿ�����Ч�ġ�����������ǻ���Ҫ�ͷŻ���顣	*/
	for (i = 0; i < 4; i++, address += BLOCK_SIZE)
		if (bh[i])
			{
	wait_on_buffer (bh[i]);		/* �ȴ�����������(����ѱ������Ļ�)��	*/
	if (bh[i]->b_uptodate)		/* ����û�������������Ч�Ļ������ơ�	*/
		COPYBLK ((unsigned long) bh[i]->b_data, address);
	brelse (bh[i]);				/* �ͷŸû�������	*/
			}
}

/*
* Ok, breada can be used as bread, but additionally to mark other
* blocks for reading as well. End the argument list with a negative
* number.
*/
/*
* OK��breada ������bread һ��ʹ�ã���������Ԥ��һЩ�顣
* �ú��������б�
* ��Ҫʹ��һ�����������������б�Ľ�����
*/

/* ��ָ���豸��ȡָ����һЩ�顣
 * �������������ɱ䣬��һϵ��ָ���Ŀ�š��ɹ�ʱ���ص�1��Ļ����ͷָ�룬���򷵻�NULL��
 */
struct buffer_head * breada (int dev, int first, ...)
{
	va_list args;
	struct buffer_head *bh, *tmp;

/* ����ȡ�ɱ�������е�1����������ţ������ŴӸ��ٻ�������ȡָ���豸�Ϳ�ŵĻ���
 * �顣����û����������Ч�����±�־δ��λ�����򷢳����豸���ݿ�����	*/
	va_start (args, first);
	if (!(bh = getblk (dev, first)))
		panic ("bread: getblk returned NULL\n");
	if (!bh->b_uptodate)
		ll_rw_block (READ, bh);
/* Ȼ��˳��ȡ�ɱ������������Hi����ţ�����������ͬ�������������á�ע�⣬336����
 * ��һ��bug�����е�bhӦ����tmp�����bugֱ����0.96����ں˴����вű�����������
 * ��Ϊ������Ԥ���������ݿ飬ֻ��������ٻ����������������Ͼ�ʹ�ã����Ե�337����
 * ����Ҫ�������ü����ݼ��ͷŵ��ÿ飨��ΪgetblkO�������������ü���ֵ����	*/
	while ((first = va_arg (args, int)) >= 0)
		{
			tmp = getblk (dev, first);
			if (tmp)
	{
		if (!tmp->b_uptodate)
			ll_rw_block (READA, bh);		/* bh Ӧ���� tmp��	*/
		tmp->b_count--;						/* ��ʱ�ͷŵ���Ԥ���顣	*/
	}
		}
/* ��ʱ�ɱ�����������в���������ϡ����ǵȴ���1������������������ѱ����������ڵ�
 * ���˳�֮�������������������Ȼ��Ч���򷵻ػ�����ͷָ���˳��������ͷŸû���������
 * NULL���˳���	*/
	va_end (args);
	wait_on_buffer (bh);
	if (bh->b_uptodate)
		return bh;
	brelse (bh);
	return (NULL);
}

/* ��������ʼ��������
 * ����buffer��end�ǻ������ڴ�ĩ�ˡ����ھ���16MB�ڴ��ϵͳ��������ĩ�˱�����Ϊ4MB��
 * ������8MB�ڴ��ϵͳ��������ĩ�˱�����Ϊ2MB���ú����ӻ�������ʼλ��start_buffer
 * ���ͻ�����ĩ��buffer��end���ֱ�ͬʱ���ã���ʼ���������ͷ�ṹ�Ͷ�Ӧ�����ݿ顣ֱ��
 * �������������ڴ汻������ϡ��μ������б�ǰ���ʾ��ͼ��	*/
void buffer_init (long buffer_end)
{
	struct buffer_head *h = start_buffer;
	void *b;
	int i;

/* ���ȸ��ݲ����ṩ�Ļ������߶�λ��ȷ��ʵ�ʻ������߶�λ��b������������߶˵���1Mb��
 * ����Ϊ��640KB - 1MB����ʾ�ڴ��BIOSռ�ã�����ʵ�ʿ��û������ڴ�߶�λ��Ӧ����
 * 640KB�����򻺳����ڴ�߶�һ������1MB��	*/
	if (buffer_end == 1 << 20)
		b = (void *) (640 * 1024);
	else
		b = (void *) buffer_end;
/* ��δ������ڳ�ʼ�����������������л����ѭ����������ȡϵͳ�л������Ŀ��������
 * �����Ǵӻ������߶˿�ʼ����1KB��С�Ļ���飬���ͬʱ�ڻ������Ͷ˽��������û����
 * �Ľṹbuffer��head��������Щbuffer��head���˫������
 * h��ָ�򻺳�ͷ�ṹ��ָ�룬��h+1��ָ���ڴ��ַ��������һ������ͷ��ַ��Ҳ����˵��
 * ָ��h����ͷ��ĩ���⡣Ϊ�˱�֤���㹻���ȵ��ڴ����洢һ������ͷ�ṹ����Ҫb��ָ��
 * ���ڴ���ַ>=h����ͷ��ĩ�ˣ���Ҫ��>=h+1��	*/
	while ((b -= BLOCK_SIZE) >= ((void *) (h + 1)))
		{
			h->b_dev = 0;				/* ʹ�øû��������豸�š�	*/
			h->b_dirt = 0;				/* ���־��Ҳ���������޸ı�־��	*/
			h->b_count = 0;				/* �û��������ü�����	*/
			h->b_lock = 0;				/* ������������־��	*/
			h->b_uptodate = 0;			/* ���������±�־�����������Ч��־����	*/
			h->b_wait = NULL;			/* ָ��ȴ��û����������Ľ��̡�	*/
			h->b_next = NULL;			/* ָ�������ͬhash ֵ����һ������ͷ��	*/
			h->b_prev = NULL;			/* ָ�������ͬhash ֵ��ǰһ������ͷ��	*/
			h->b_data = (char *) b;		/* ָ���Ӧ���������ݿ飨1024 �ֽڣ���	*/
			h->b_prev_free = h - 1;		/* ָ��������ǰһ�	*/
			h->b_next_free = h + 1;		/* ָ����������һ�	*/
			h++;						/* h ָ����һ�»���ͷλ�á�	*/
			NR_BUFFERS++;				/* �����������ۼӡ�	*/
			if (b == (void *) 0x100000)	/* �����ַb �ݼ�������1MB��������384KB��	*/
		b = (void *) 0xA0000;			/* ��b ָ���ַ0xA0000(640KB)����	*/
		}
	h--;								/* ��h ָ�����һ����Ч����ͷ��	*/

	free_list = start_buffer;			/* �ÿ�������ͷָ��ͷһ��������ͷ��	*/
	free_list->b_prev_free = h;			/* ����ͷ��b_prev_free ָ��ǰһ������һ���	*/
	h->b_next_free = free_list;			/* h ����һ��ָ��ָ���һ��γ�һ��������	*/

/* ����ʼ��hash ����ϣ��ɢ�б����ñ������е�ָ��ΪNULL��	*/
	for (i = 0; i < NR_HASH; i++)
		hash_table[i] = NULL;
}
