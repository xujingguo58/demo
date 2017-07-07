/*
* linux/fs/inode.c
*
* (C) 1991 Linus Torvalds
*/

#include <string.h>			/* �ַ���ͷ�ļ�����Ҫ������һЩ�й��ַ���������Ƕ�뺯����	*/
#include <sys/stat.h>		/* �ļ�״̬ͷ�ļ��������ļ����ļ�ϵͳ״̬�ṹstat{}�ͳ�����	*/
#include <linux/sched.h>	/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 �����ݣ�	*/
							/* ����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/
#include <linux/kernel.h>	/* �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣	*/
#include <linux/mm.h>		/* �ڴ����ͷ�ļ�������ҳ���С�����һЩҳ���ͷź���ԭ�͡�	*/
#include <asm/system.h>		/* ϵͳͷ�ļ������������û��޸�������/�ж��ŵȵ�Ƕ��ʽ���ꡣ	*/

struct m_inode inode_table[NR_INODE] = { {0,}, };	/* �ڴ���i�ڵ��NR_INODE=32���	*/

static void read_inode (struct m_inode *inode);		/* ��ָ��i�ڵ�ŵ�i�ڵ���Ϣ��294�С�	*/
static void write_inode (struct m_inode *inode);	/* дi�ڵ���Ϣ�����ٻ����У�314�С�	*/

/* �ȴ�ָ����i �ڵ���á�	*/
/* ���i�ڵ��ѱ��������򽫵�ǰ������Ϊ�����жϵĵȴ�״̬������ӵ���i�ڵ�ĵȴ���
 * ��i��wait�С�ֱ����i�ڵ��������ȷ�ػ��ѱ�����	*/
static inline void wait_on_inode (struct m_inode *inode)
{
	cli ();
	while (inode->i_lock)
		sleep_on (&inode->i_wait);
	sti ();
}

/* ��ָ����i�ڵ�����������ָ����i�ڵ㣩��
 * ���i�ڵ��ѱ��������򽫵�ǰ������Ϊ�����жϵĵȴ�״̬������ӵ���i�ڵ�ĵȴ���
 * ��i_wait�С�ֱ����i�ڵ��������ȷ�ػ��ѱ�����Ȼ�����������	*/
static inline void lock_inode (struct m_inode *inode)
{
	cli ();
	while (inode->i_lock)
		sleep_on (&inode->i_wait);
	inode->i_lock = 1;			/* ��������־��	*/
	sti ();
}

/* ��ָ����i�ڵ������
 * ��λi�ڵ��������־������ȷ�ػ��ѵȴ��ڴ�i�ڵ�ȴ�����i��wait�ϵ����н��̡�	*/
static inline void unlock_inode (struct m_inode *inode)
{
	inode->i_lock = 0;
	wake_up (&inode->i_wait);		/* kernel/sched.c���� 188 �С�	*/
}

/* �ͷ��豸dev���ڴ�i�ڵ���е�����i�ڵ㡣
 * ɨ���ڴ��е�i�ڵ�����飬�����ָ���豸ʹ�õ�i�ڵ���ͷ�֮��	*/
void invalidate_inodes (int dev)
{
	int i;
	struct m_inode *inode;

/* ������ָ��ָ���ڴ�i�ڵ���������Ȼ��ɨ��i�ڵ��ָ�������е�����i�ڵ㡣���
 * ����ÿ��i�ڵ㣬�ȵȴ���i�ڵ�������ã���Ŀǰ���������Ļ��������ж��Ƿ�����ָ��
 * �豸��i�ڵ㡣�����ָ���豸��i�ڵ㣬�򿴿����Ƿ񻹱�ʹ���ţ��������ü����Ƿ�
 * Ϊ0����������ʾ������Ϣ��Ȼ���ͷ�֮������i�ڵ���豸���ֶ�i��dev��0����48����
 * ��ָ�븳ֵ��0+inode��table����ͬ�ڡ�inode_table������&inode_table[0]����
 * ��������д II���ܸ�����һЩ��	*/
	inode = 0 + inode_table;		/* ��ָ������ָ��i �ڵ��ָ���������	*/
	for (i = 0; i < NR_INODE; i++, inode++)
	{								/* ɨ��i �ڵ��ָ�������е�����i �ڵ㡣	*/
		wait_on_inode (inode);		/* �ȴ���i �ڵ���ã���������	*/
		if (inode->i_dev == dev)
		{							/* �����ָ���豸��i �ڵ㣬��	*/
			if (inode->i_count)		/* �������������Ϊ0������ʾ�����棻	*/
				printk ("inode in use on removed disk\n\r");
			inode->i_dev = inode->i_dirt = 0;	/* �ͷŸ�i �ڵ�(���豸��Ϊ0 ��)��	*/
		}
	}
}

/* ͬ������i �ڵ㡣	*/
/* ���ڴ�i�ڵ��������i�ڵ����豸��i�ڵ���ͬ��������	*/
void sync_inodes (void)
{
	int i;
	struct m_inode *inode;

/* �������ڴ�i�ڵ����͵�ָ��ָ��i�ڵ�����Ȼ��ɨ������i�ڵ���еĽڵ㡣���
 * ����ÿ��i�ڵ㣬�ȵȴ���i�ڵ�������ã���Ŀǰ���������Ļ�����Ȼ���жϸ�i�ڵ�
 * �Ƿ��ѱ��޸Ĳ��Ҳ��ǹܵ��ڵ㡣������������򽫸�i�ڵ�д����ٻ������С�������
 * �������buffer.c�����ʵ�ʱ��������д�����С�	*/
	inode = 0 + inode_table;		/* ��ָ������ָ��i �ڵ��ָ���������	*/
	for (i = 0; i < NR_INODE; i++, inode++)
	{								/* ɨ��i �ڵ��ָ�����顣	*/
		wait_on_inode (inode);		/* �ȴ���i �ڵ���ã���������	*/
		if (inode->i_dirt && !inode->i_pipe)	/* �����i �ڵ����޸��Ҳ��ǹܵ��ڵ㣬	*/
		write_inode (inode);		/* ��д�̣�ʵ����д�뻺�����У���	*/
	}
}

/* �ļ����ݿ�ӳ�䵽�̿�Ĵ����������blockλͼ��������bmap - block map��
 * ������inode -�ļ���i�ڵ�ָ�룻block -�ļ��е����ݿ�ţ�create -�������־��
 * �ú�����ָ�����ļ����ݿ�block��Ӧ���豸���߼����ϣ��������߼���š����������־
 * ��λ�������豸�϶�Ӧ�߼��鲻����ʱ�������´��̿飬�����ļ����ݿ�block��Ӧ���豸
 * �ϵ��߼���ţ��̿�ţ���	*/
static int _bmap (struct m_inode *inode, int block, int create)
{
	struct buffer_head *bh;
	int i;

/* �����жϲ����ļ����ݿ��block����Ч�ԡ�������С��0����ͣ���������Ŵ���ֱ��
 * ���� + ��ӿ��� + ���μ�ӿ����������ļ�ϵͳ��ʾ��Χ����ͣ����	*/
	if (block < 0)
		panic ("_bmap: block<0");
	if (block >= 7 + 512 + 512 * 512)
		panic ("_bmap: block>big");
/* Ȼ������ļ���ŵĴ�Сֵ���Ƿ������˴�����־�ֱ���д�������ÿ��С��7����ʹ
 * ��ֱ�ӿ��ʾ�����������־��λ������i�ڵ��ж�Ӧ�ÿ���߼��飨���Σ��ֶ�Ϊ0����
 * ����Ӧ�豸����һ���̿飨�߼��飩�����ҽ������߼���ţ��̿�ţ������߼����ֶ��С�
 * Ȼ������i�ڵ�ı�ʱ�䣬��i�ڵ����޸ı�־����󷵻��߼���š�����new��block()
 * ������bitmap.c�����е�75�п�ʼ����	*/
	if (block < 7)
	{
		if (create && !inode->i_zone[block])
			if (inode->i_zone[block] = new_block (inode->i_dev))
			{
				inode->i_ctime = CURRENT_TIME;
				inode->i_dirt = 1;
			}
		return inode->i_zone[block];
	}
/* ����ÿ��>=7����С��7+512����˵��ʹ�õ���һ�μ�ӿ顣�����һ�μ�ӿ���д���
 * ����Ǵ��������Ҹ�i�ڵ��ж�Ӧ��ӿ��ֶ�i_Zone[7]��0�������ļ����״�ʹ�ü�ӿ飬
 * ��������һ���̿����ڴ�ż�ӿ���Ϣ��������ʵ�ʴ��̿�������ӿ��ֶ��С�Ȼ����
 * ��i�ڵ����޸ı�־���޸�ʱ�䡣�������ʱ������̿�ʧ�ܣ����ʱi�ڵ��ӿ��ֶ�
 * i_zone[7]Ϊ0���򷵻�0�����߲��Ǵ�������i_zone[7]ԭ����Ϊ0������i�ڵ���û�м�
 * �ӿ飬����ӳ����̿�ʧ�ܣ�����0�˳���	*/
	block -= 7;
	if (block < 512)
	{
		if (create && !inode->i_zone[7])
			if (inode->i_zone[7] = new_block (inode->i_dev))
			{
				inode->i_dirt = 1;
				inode->i_ctime = CURRENT_TIME;
			}
			if (!inode->i_zone[7])
		return 0;
/* ���ڶ�ȡ�豸�ϸ�i�ڵ��һ�μ�ӿ顣��ȡ�ü�ӿ��ϵ�block���е��߼���ţ��̿�
 * �ţ�i��ÿһ��ռ2���ֽڡ�����Ǵ������Ҽ�ӿ�ĵ�block���е��߼����Ϊ0�Ļ���
 * ������һ���̿飬���ü�ӿ��еĵ�block����ڸ����߼����š�Ȼ����λ��ӿ����
 * �޸ı�־��������Ǵ�������i������Ҫӳ�䣨Ѱ�ң����߼���š�	*/
		if (!(bh = bread (inode->i_dev, inode->i_zone[7])))
			return 0;
		i = ((unsigned short *) (bh->b_data))[block];
		if (create && !i)
			if (i = new_block (inode->i_dev))
			{
				((unsigned short *) (bh->b_data))[block] = i;
				bh->b_dirt = 1;
			}
/* ����ͷŸü�ӿ�ռ�õĻ���飬�����ش������������ԭ�еĶ�Ӧblock���߼����š�	*/
		brelse (bh);
		return i;
	}
/* ���������е��ˣ���������ݿ����ڶ��μ�ӿ顣�䴦�������һ�μ�ӿ����ơ������Ƕ�
 * ���μ�ӿ�Ĵ������Ƚ�block�ټ�ȥ��ӿ������ɵĿ�����512����Ȼ������Ƿ�����
 * �˴�����־���д�����Ѱ�Ҵ���������´�������i�ڵ�Ķ��μ�ӿ��ֶ�Ϊ0��������
 * ��һ���̿����ڴ�Ŷ��μ�ӿ��һ������Ϣ��������ʵ�ʴ��̿��������μ�ӿ��ֶ�
 * �С�֮����i�ڵ����޸ı��ƺ��޸�ʱ�䡣ͬ���أ��������ʱ������̿�ʧ�ܣ����
 * ʱi�ڵ���μ�ӿ��ֶ�i_zone[8]Ϊ0���򷵻�0�����߲��Ǵ�������i_zone[8]ԭ����
 * Ϊ0������i�ڵ���û�м�ӿ飬����ӳ����̿�ʧ�ܣ�����0�˳���	*/
	block -= 512;
	if (create && !inode->i_zone[8])
		if (inode->i_zone[8] = new_block (inode->i_dev))
		{
			inode->i_dirt = 1;
			inode->i_ctime = CURRENT_TIME;
		}
	if (!inode->i_zone[8])
		return 0;
/* ���ڶ�ȡ�豸�ϸ�i�ڵ�Ķ��μ�ӿ顣��ȡ�ö��μ�ӿ��һ�����ϵڣ�block/512��
 * ���е��߼����i������Ǵ������Ҷ��μ�ӿ��һ�����ϵڣ�block/512�����е��߼�
 * ���Ϊ0�Ļ�����������һ���̿飨�߼��飩��Ϊ���μ�ӿ�Ķ�����i�����ö��μ�ӿ�
 * ��һ�����еڣ�block/512������ڸö�����Ŀ��i��Ȼ����λ���μ�ӿ��һ������
 * �޸ı�־�����ͷŶ��μ�ӿ��һ���顣������Ǵ�������i������Ҫӳ�䣨Ѱ�ң�����
 * ����š�	*/
	if (!(bh = bread (inode->i_dev, inode->i_zone[8])))
		return 0;
	i = ((unsigned short *) bh->b_data)[block >> 9];
	if (create && !i)
		if (i = new_block (inode->i_dev))
			{
				((unsigned short *) (bh->b_data))[block >> 9] = i;
				bh->b_dirt = 1;
			}
	brelse (bh);
/* ������μ�ӿ�Ķ�������Ϊ0����ʾ������̿�ʧ�ܻ���ԭ����Ӧ��ž�Ϊ0����
 * ��0�˳�������ʹ��豸�϶�ȡ���μ�ӿ�Ķ����飬��ȡ�ö������ϵ�block���е���
 * ����ţ�����511��Ϊ���޶�blockֵ������511����	*/
	if (!i)
		return 0;
	if (!(bh = bread (inode->i_dev, i)))
		return 0;
	i = ((unsigned short *) bh->b_data)[block & 511];
/* ����Ǵ������Ҷ�����ĵ�block�����߼����Ϊ0�Ļ���������һ���̿飨�߼��飩��
 * ��Ϊ���մ��������Ϣ�Ŀ顣���ö������еĵ�block����ڸ����߼�����(i)��Ȼ��
 * ��λ����������޸ı�־��	*/
	if (create && !i)
		if (i = new_block (inode->i_dev))
		{
			((unsigned short *) (bh->b_data))[block & 511] = i;
			bh->b_dirt = 1;
		}
/* ����ͷŸö��μ�ӿ�Ķ����飬���ش�����������Ķ�Ӧblock ���߼���Ŀ�š�	*/
	brelse (bh);
	return i;
}

/* ȡ�ļ����ݿ�block���豸�϶�Ӧ���߼���š�
 * ������inode -�ļ����ڴ�i�ڵ�ָ�룻block -�ļ��е����ݿ�š�
 * �������ɹ��򷵻ض�Ӧ���߼���ţ����򷵻�0��	*/
int bmap (struct m_inode *inode, int block)
{
	return _bmap (inode, block, 0);
}

/* ȡ�ļ����ݿ�block���豸�϶�Ӧ���߼���š�
 * �����Ӧ���߼��鲻���ھʹ���һ�顣�����豸�϶�Ӧ���Ѵ��ڻ��½����߼���š�
 * ������inode -�ļ����ڴ�i�ڵ�ָ�룻block -�ļ��е����ݿ�š�
 * �������ɹ��򷵻ض�Ӧ���߼���ţ����򷵻�0��	*/
int create_block (struct m_inode *inode, int block)
{
	return _bmap (inode, block, 1);
}

/* �Żأ����ã�һ��i�ڵ㣨��д���豸����
 * �ú�����Ҫ���ڰ�i�ڵ����ü���ֵ�ݼ�1���������ǹܵ�i�ڵ㣬���ѵȴ��Ľ��̡�
 * ���ǿ��豸�ļ�i�ڵ���ˢ���豸��������i�ڵ�����Ӽ���Ϊ0�����ͷŸ�i�ڵ�ռ��
 * �����д����߼��飬���ͷŸ�i�ڵ㡣	*/
void iput (struct m_inode *inode)
{
/* �����жϲ���������i�ڵ����Ч�ԣ����ȴ�inode�ڵ����������������Ļ��������i
 * �ڵ�����ü���Ϊ0����ʾ��i�ڵ��Ѿ��ǿ��еġ��ں���Ҫ�������зŻز�����˵����
 * �����������������⡣������ʾ������Ϣ��ͣ����	*/
	if (!inode)
		return;
	wait_on_inode (inode);	/* �ȴ�inode �ڵ����(����������Ļ�)��	*/
	if (!inode->i_count)
		panic ("iput: trying to free free inode");
/* ����ǹܵ�i �ڵ㣬���ѵȴ��ùܵ��Ľ��̣����ô�����1��������������򷵻ء�����
 * �ͷŹܵ�ռ�õ��ڴ�ҳ�棬����λ�ýڵ�����ü���ֵ�����޸ı�־�͹ܵ���־�������ء�	
 * ����pipe �ڵ㣬inode->i_size ����������ڴ�ҳ��ַ��
 * �μ�get_pipe_inode()��228��234 �С�	*/
	if (inode->i_pipe)
	{
		wake_up (&inode->i_wait);
		if (--inode->i_count)
			return;
		free_page (inode->i_size);
		inode->i_count = 0;
		inode->i_dirt = 0;
		inode->i_pipe = 0;
		return;
	}
/* ���i�ڵ��Ӧ���豸��=0���򽫴˽ڵ�����ü����ݼ�1�����ء��������ڹܵ�������
 * i�ڵ㣬��i�ڵ���豸��Ϊ0��	*/
	if (!inode->i_dev)
	{
		inode->i_count--;
		return;
	}
/* ����ǿ��豸�ļ���i�ڵ㣬��ʱ�߼����ֶ�0(i��zone[0])�����豸�ţ���ˢ�¸��豸��
 * ���ȴ�i�ڵ������	*/
	if (S_ISBLK (inode->i_mode))
	{
		sync_dev (inode->i_zone[0]);
		wait_on_inode (inode);
	}
/* ���i�ڵ�����ü�������1��������ݼ�1���ֱ�ӷ��أ���Ϊ��i�ڵ㻹�������ã�����
 * �ͷţ��������˵��i�ڵ�����ü���ֵΪ1 (��Ϊ��155���Ѿ��жϹ����ü����Ƿ�Ϊ��)��
 * ���i�ڵ��������Ϊ0����˵��i�ڵ��Ӧ�ļ���ɾ���������ͷŸ�i�ڵ�������߼��飬
 * ���ͷŸ�i�ڵ㡣����free_inode()����ʵ���ͷ�i�ڵ����������λi�ڵ��Ӧ��i�ڵ�λ
 * ͼ����λ�����i�ڵ�ṹ���ݡ�
 */
repeat:
	if (inode->i_count > 1)
	{
		inode->i_count--;
		return;
	}
	if (!inode->i_nlinks)
	{
		truncate (inode);
		free_inode (inode);
		return;
	}
/* �����i�ڵ��������޸ģ����д���¸�i�ڵ㣬���ȴ���i�ڵ����������������дi��
 * ��ʱ��Ҫ�ȴ�˯�ߣ���ʱ���������п����޸ĸ�i�ڵ㣬����ڽ��̱����Ѻ���Ҫ�ٴ��ظ�
 * ���������жϹ��̣�repeat����	*/
	if (inode->i_dirt)
	{
		write_inode (inode);	/* we can sleep - so do again */
		wait_on_inode (inode);	/* ��Ϊ����˯���ˣ�������Ҫ�ظ��ж�	*/
		goto repeat;
	}
/* ��������ִ�е��ˣ���˵����i�ڵ�����ü���ֵi_count��1����������Ϊ�㣬��������
 * û�б��޸Ĺ�����˴�ʱֻҪ��i�ڵ����ü����ݼ�1�����ء���ʱ��i�ڵ��i_count=0��
 * ��ʾ���ͷš�	*/
	inode->i_count--;
	return;
}

/* ��i�ڵ��inode��table���л�ȡһ������i�ڵ��
 * Ѱ�����ü���countΪ0��i�ڵ㣬������д�̺����㣬������ָ�롣���ü�������1��	*/
struct m_inode * get_empty_inode (void)
{
	struct m_inode *inode;
	static struct m_inode *last_inode = inode_table;	/* last_inode ָ��i �ڵ���һ�	*/
	int i;

	do
	{
/* �ڳ�ʼ��last_inodeָ��ָ��i�ڵ��ͷһ���ѭ��ɨ������i�ڵ�����last_inode�Ѿ�
 * ָ��i�ڵ������1��֮������������ָ��i�ڵ��ʼ�����Լ���ѭ��Ѱ�ҿ���i�ڵ����
 * ��last_inode��ָ���i�ڵ�ļ���ֵΪ0����˵�������ҵ�����i�ڵ����inode ָ���i
 * �������i�ڵ�����޸ı�־��������־��Ϊ0�������ǿ���ʹ�ø�i�ڵ㣬�����˳�forѭ����	*/
		inode = NULL;
		for (i = NR_INODE; i; i--)
		{
			if (++last_inode >= inode_table + NR_INODE)
				last_inode = inode_table;
			if (!last_inode->i_count)
			{
				inode = last_inode;
				if (!inode->i_dirt && !inode->i_lock)
				break;
			}
		}
/* ���û���ҵ�����i �ڵ�(inode=NULL)��������i �ڵ���ӡ����������ʹ�ã���������	*/
		if (!inode)
		{
			for (i = 0; i < NR_INODE; i++)
				printk ("%04x: %6d\t", inode_table[i].i_dev,
					inode_table[i].i_num);
			panic ("No free inodes in mem");
		}
/* �ȴ���i�ڵ����������ֱ������Ļ����������i�ڵ����޸ı�־����λ�Ļ����򽫸�
 * i�ڵ�ˢ�£�ͬ��������Ϊˢ��ʱ���ܻ�˯�ߣ������Ҫ�ٴ�ѭ���ȴ���i�ڵ������	*/
		wait_on_inode (inode);
		while (inode->i_dirt)
		{
			write_inode (inode);
			wait_on_inode (inode);
		}
/* ���i�ڵ��ֱ�����ռ�õĻ���i�ڵ�ļ���ֵ��Ϊ0 �ˣ���������Ѱ�ҿ���i�ڵ㡣����
 * ˵�����ҵ�����Ҫ��Ŀ���i�ڵ���򽫸�i�ڵ����������㣬�������ü���Ϊ1������
 * ��i�ڵ�ָ�롣	*/
	}
	while (inode->i_count);	
	memset (inode, 0, sizeof (*inode));
	inode->i_count = 1;
	return inode;
}

/* ��ȡ�ܵ��ڵ㡣
 * ����ɨ��i�ڵ��Ѱ��һ������i�ڵ��Ȼ��ȡ��һҳ�����ڴ湩�ܵ�ʹ�á�Ȼ�󽫵�
 * ����i�ڵ�����ü�����Ϊ2(���ߺ�д��)����ʼ���ܵ�ͷ��β����i�ڵ�Ĺܵ����ͱ�ʾ��
 * ����Ϊi�ڵ�ָ�룬���ʧ���򷵻�NULL��	*/
struct m_inode *
get_pipe_inode (void)
{
	struct m_inode *inode;

/* ���ȴ��ڴ�i�ڵ����ȡ��һ������i�ڵ㡣����Ҳ�������i�ڵ��򷵻�NULL��Ȼ��Ϊ��
 * i�ڵ�����һҳ�ڴ棬���ýڵ��i_size�ֶ�ָ���ҳ�档�����û�п����ڴ棬���ͷŸ�
 * i�ڵ㣬������NULL��	*/
	if (!(inode = get_empty_inode ()))	/* ����Ҳ�������i �ڵ��򷵻�NULL��	*/
		return NULL;
	if (!(inode->i_size = get_free_page ()))
	{							/* �ڵ��i_size �ֶ�ָ�򻺳�����	*/
		inode->i_count = 0;		/* �����û�п����ڴ棬��	*/
		return NULL;			/* �ͷŸ�i �ڵ㣬������NULL��	*/
	}
/* Ȼ�����ø�i�ڵ�����ü���Ϊ2������λ��λ�ܵ�ͷβָ�롣i�ڵ��߼��������i_zone[]
 * ��i��zone[0]��i��zone[l]�зֱ�������Źܵ�ͷ�͹ܵ�βָ�롣�������i�ڵ��ǹܵ�i��
 * ���־�����ظ�i�ڵ�š�	*/
	inode->i_count = 2;			/* sum of readers/writers	��/д�����ܼ� */
	PIPE_HEAD (*inode) = PIPE_TAIL (*inode) = 0;	/* ��λ�ܵ�ͷβָ�롣	*/
	inode->i_pipe = 1;			/* �ýڵ�Ϊ�ܵ�ʹ�õı�־��	*/
	return inode;				/* ����i �ڵ�ָ�롣	*/
}

/* ȡ��һ��i�ڵ㡣
 * ������dev -�豸�ţ�nr - i�ڵ�š�
 * ���豸�϶�ȡָ���ڵ�ŵ�i�ڵ㵽�ڴ�i�ڵ���У������ظ�i�ڵ�ָ�롣
 * ������λ�ڸ��ٻ������е�i�ڵ������Ѱ�����ҵ�ָ���ڵ�ŵ�i�ڵ����ھ���һЩ�ж�
 * ����󷵻ظ�i�ڵ�ָ�롣������豸dev�϶�ȡָ��i�ڵ�ŵ�i�ڵ���Ϣ����i�ڵ��
 * �У������ظ�i�ڵ�ָ�롣	*/
struct m_inode * iget (int dev, int nr)
{
	struct m_inode *inode, *empty;

/* �����жϲ�����Ч�ԡ����豸����0��������ں˴������⣬��ʾ������Ϣ��ͣ����Ȼ��Ԥ
 * �ȴ�i�ڵ����ȡһ������i�ڵ㱸�á�	*/
	if (!dev)
		panic ("iget with dev==0");
	empty = get_empty_inode ();
/* ����ɨ��i�ڵ����Ѱ�Ҳ���ָ���ڵ��nr��i�ڵ㡣�������ýڵ�����ô����������
 * ǰɨ��i�ڵ���豸�Ų�����ָ�����豸�Ż��߽ڵ�Ų�����ָ���Ľڵ�ţ������ɨ�衣	*/
	inode = inode_table;
	while (inode < NR_INODE + inode_table)
	{
		if (inode->i_dev != dev || inode->i_num != nr)
		{
			inode++;
			continue;
		}
/* ����ҵ�ָ���豸��dev�ͽڵ��nr��i�ڵ㣬��ȴ��ýڵ����������������Ļ�����
 * �ڵȴ��ýڵ���������У�i�ڵ����ܻᷢ���仯�������ٴν���������ͬ�жϡ������
 * ���˱仯�����ٴ�����ɨ������i�ڵ��	*/
		wait_on_inode (inode);
		if (inode->i_dev != dev || inode->i_num != nr)
		{
			inode = inode_table;
			continue;
		}
/* �������ʾ�ҵ���Ӧ��i�ڵ㡣���ǽ���i�ڵ����ü�����1��Ȼ��������һ����飬����
 * �Ƿ�����һ���ļ�ϵͳ�İ�װ�㡣������Ѱ�ұ���װ�ļ�ϵͳ���ڵ㲢���ء������i�ڵ�
 * ��ȷ�������ļ�ϵͳ�İ�װ�㣬���ڳ����������Ѱ��װ�ڴ�i�ڵ�ĳ����顣���û����
 * ��������ʾ������Ϣ�����Żر�������ʼʱ��ȡ�Ŀ��нڵ�empty�����ظ�i�ڵ�ָ�롣	*/
		inode->i_count++;
		if (inode->i_mount)
		{
			int i;
			for (i = 0; i < NR_SUPER; i++)
				if (super_block[i].s_imount == inode)
					break;
			if (i >= NR_SUPER)
			{
				printk ("Mounted inode hasn't got sb\n");
				if (empty)
					iput (empty);
				return inode;
			}
/* ִ�е������ʾ�Ѿ��ҵ���װ��inode�ڵ���ļ�ϵͳ�����顣���ǽ���i�ڵ�д�̷Żأ�
 * ���Ӱ�װ�ڴ�i�ڵ��ϵ��ļ�ϵͳ��������ȡ�豸�ţ�����i�ڵ��ΪR00T_IN0����Ϊ1��
 * Ȼ������ɨ������i�ڵ���Ի�ȡ�ñ���װ�ļ�ϵͳ�ĸ�i�ڵ���Ϣ��	*/
			iput (inode);
			dev = super_block[i].s_dev;
			nr = ROOT_INO;
			inode = inode_table;
			continue;
		}
/* ���������ҵ�����Ӧ��i�ڵ㡣��˿��Է�����������ʼ����ʱ����Ŀ���i�ڵ㣬����
 * �ҵ���i�ڵ�ָ�롣	*/
		if (empty)
			iput (empty);
		return inode;
	}
/* ���������i�ڵ����û���ҵ�ָ����i�ڵ㣬������ǰ������Ŀ���i�ڵ�empty��i
 * �ڵ���н�����i�ڵ㡣������Ӧ�豸�϶�ȡ��i�ڵ���Ϣ�����ظ�i�ڵ�ָ�롣	*/
	if (!empty)
		return (NULL);
	inode = empty;
	inode->i_dev = dev;				/* ���� i �ڵ���豸��	*/
	inode->i_num = nr;				/* ���� i �ڵ�š�	*/
	read_inode (inode);
	return inode;
}

/* ��ȡָ��i�ڵ���Ϣ��
 * ���豸�϶�ȡ����ָ��i�ڵ���Ϣ��i�ڵ��̿飬Ȼ���Ƶ�ָ����i�ڵ�ṹ�С�Ϊ��
 * ȷ��i�ڵ����ڵ��豸�߼���ţ��򻺳�飩���������ȶ�ȡ��Ӧ�豸�ϵĳ����飬�Ի�ȡ
 * ���ڼ����߼���ŵ�ÿ��i�ڵ�����ϢINODES��PER��BLOCK���ڼ����i�ڵ����ڵ��߼���
 * �ź󣬾ͰѸ��߼������һ������С�Ȼ��ѻ��������Ӧλ�ô���i�ڵ����ݸ��Ƶ�����
 * ָ����λ�ô���	*/
static void read_inode (struct m_inode *inode)
{
	struct super_block *sb;
	struct buffer_head *bh;
	int block;

/* ����������i �ڵ㣬��ȡ�ýڵ������豸�ĳ����顣	*/
	lock_inode (inode);
	if (!(sb = get_super (inode->i_dev)))
		panic ("trying to read inode without dev");
/* ��i�ڵ����ڵ��豸�߼����=(������+������)+ i�ڵ�λͼռ�õĿ���+�߼���λ
 * ͼռ�õĿ���+ (i�ڵ��-1)/ÿ�麬�е�i�ڵ�������Ȼi�ڵ�Ŵ�0��ʼ��ţ�����1
 * ��0��i�ڵ㲻�ã����Ҵ�����Ҳ�������Ӧ��0��i�ڵ�ṹ����˴��i�ڵ���̿��
 * ��1���ϱ������i�ڵ������1--16��i�ڵ�ṹ������0--15�ġ�������������i��
 * ��Ŷ�Ӧ��i�ڵ�ṹ�����̿�ʱ��Ҫ��1������B=(i�ڵ��-1)/ÿ�麬��i�ڵ�ṹ����
 * ���磬�ڵ��16��i�ڵ�ṹӦ����B=(16-l)/16 = 0�Ŀ��ϡ��������Ǵ��豸�϶�ȡ��
 * i�ڵ����ڵ��߼��飬������ָ��i�ڵ����ݵ�inodeָ����ָλ�ô���	*/
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(inode->i_num - 1) / INODES_PER_BLOCK;
	if (!(bh = bread (inode->i_dev, block)))
		panic ("unable to read i-node block");
	*(struct d_inode *) inode =
		((struct d_inode *) bh->b_data)[(inode->i_num - 1) % INODES_PER_BLOCK];
/* ����ͷŶ���Ļ���������������i �ڵ㡣	*/
	brelse (bh);
	unlock_inode (inode);
}

/* ��i�ڵ���Ϣд�뻺�����С�
 * �ú����Ѳ���ָ����i�ڵ�д�뻺������Ӧ�Ļ�����У���������ˢ��ʱ��д�����С�Ϊ��
 * ȷ��i�ڵ����ڵ��豸�߼���ţ��򻺳�飩���������ȶ�ȡ��Ӧ�豸�ϵĳ����飬�Ի�ȡ
 * ���ڼ����߼���ŵ�ÿ��i�ڵ�����ϢINODES_PER_BLOCK���ڼ����i�ڵ����ڵ��߼���
 * �ź󣬾ͰѸ��߼������һ������С�Ȼ���i�ڵ����ݸ��Ƶ���������Ӧλ�ô���	*/
static void write_inode (struct m_inode *inode)
{
	struct super_block *sb;
	struct buffer_head *bh;
	int block;

/* ����������i�ڵ㣬�����i�ڵ�û�б��޸Ĺ����߸�i�ڵ���豸�ŵ����㣬�������
 * i�ڵ㣬���˳�������û�б��޸Ĺ���i�ڵ㣬�������뻺�����л��豸�е���ͬ��Ȼ��
 * ��ȡ��i�ڵ�ĳ����顣	*/
	lock_inode (inode);
	if (!inode->i_dirt || !inode->i_dev)
	{
		unlock_inode (inode);
		return;
	}
	if (!(sb = get_super (inode->i_dev)))
		panic ("trying to write inode without device");
/* ��i�ڵ����ڵ��豸�߼����=(������+������)+ i�ڵ�λͼռ�õĿ���+�߼���λ
 * ͼռ�õĿ���+ (i�ڵ��-1)/ÿ�麬�е�i�ڵ��������Ǵ��豸�϶�ȡ��i�ڵ����ڵ�
 * �߼��飬������i�ڵ���Ϣ���Ƶ��߼����Ӧ��i�ڵ����λ�ô���	*/

	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(inode->i_num - 1) / INODES_PER_BLOCK;
	if (!(bh = bread (inode->i_dev, block)))
		panic ("unable to read i-node block");
	((struct d_inode *) bh->b_data)
		[(inode->i_num - 1) % INODES_PER_BLOCK] = *(struct d_inode *) inode;
/* Ȼ���û��������޸ı�־����i�ڵ������Ѿ��뻺�����е�һ�£�����޸ı�־���㡣Ȼ��
 * �ͷŸú���i�ڵ�Ļ���������������i�ڵ㡣	*/
	bh->b_dirt = 1;
	inode->i_dirt = 0;
	brelse (bh);
	unlock_inode (inode);
}
