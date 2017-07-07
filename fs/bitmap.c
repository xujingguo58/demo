/*
 *  linux/fs/bitmap.c
 *
 *  (C) 1991  Linus Torvalds
 */

/* bitmap.c contains the code that handles the inode and block bitmaps */
/* bitmap.c �����д���i �ڵ�ʹ��̿�λͼ�Ĵ��� */
#include <string.h>				/* �ַ���ͷ�ļ�����Ҫ������һЩ�й��ַ���������Ƕ�뺯����	*/
								/* ��Ҫʹ�������е�memset()������	*/
#include <linux/sched.h>		/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 �����ݣ�	*/
								/* ����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/
#include <linux/kernel.h>		/* �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣	*/

/* ��ָ����ַ��addr������һ��1024�ֽ��ڴ����㡣
 * Ƕ�������ꡣ
 * ���룺eax = 0�� ecx =�Գ���Ϊ��λ�����ݿ鳤�ȣ�BLOCK��SIZE/4��; edi =ָ����ʼ��addr��*/
#define clear_block(addr) \
/* �巽��λ��			*/	__asm__("cld\n\t" \
/* �ظ�ִ�д洢���ݣ�0����	*/	"rep\n\t" \
							"stosl" \
							::"a" (0),"c" (BLOCK_SIZE/4),"D" ((long) (addr)):) 

/* ��ָ����ַ��ʼ�ĵ�nr��λƫ�ƴ��ı���λ��λ��nr�ɴ���32!��������ԭ����λֵ��
 * ���룺%0 -eax(����ֵ)��
 * 		%1 -eax(0);
 * 		%2 -nr��λƫ��ֵ��
 * 		%3 -(addr)��addr �����ݡ�
 * ��20�ж�����һ���ֲ��Ĵ�������res���ñ�������������ָ����eax�Ĵ����У��Ա���
 * ��Ч���ʺͲ��������ֶ�������ķ�����Ҫ������Ƕ�������С���ϸ˵���μ�gcc�ֲ�
 *  ����ָ���Ĵ����еı���������������һ�������ʽ����Բ������ס�������䣩����
 * ֵ�������������һ�����ʽ���res (��23��)��ֵ��
 * ��21���ϵ�btslָ�����ڲ��Բ����ñ���λ��Bit Test and Set�����ѻ���ַ��%3����
 * ����λƫ��ֵ��%2����ָ���ı���λֵ�ȱ��浽��λ��־CF�У�Ȼ�����øñ���λΪ1��
 * ָ��setb���ڸ��ݽ�λ��־CF���ò�������%al�������CF=l��%al =1������%al =0��	*/
#define set_bit(nr,addr) ({\
register int res __asm__("ax"); \
__asm__ __volatile__("btsl %2,%3\n\tsetb %%al": \
"=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \
res;})

/* ��λָ����ַ��ʼ�ĵ�nrλƫ�ƴ��ı���λ������ԭ����λֵ�ķ��롣
 * ���룺%0 -eax(����ֵ)��
 *		%1 -eax(0);
 * 		%2 -nr��λƫ��ֵ��
 * 		%3 -(addr)��addr �����ݡ�
 * ��27���ϵ�btrlָ�����ڲ��Բ���λ����λ��Bit Test and Reset�����������������
 * btsl���ƣ����Ǹ�λָ������λ��ָ��setnb���ڸ��ݽ�λ��־CF���ò�������%al����
 * ��� CF = 1��%al = 0������%al = 1��	*/
#define clear_bit(nr,addr) ({\
register int res __asm__("ax"); \
__asm__ __volatile__("btrl %2,%3\n\tsetnb %%al": \
"=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \
res;})

/* ��addr��ʼѰ�ҵ�1��0ֵ����λ��
 * ���룺%0 - ecx(����ֵ)��
 * 		%1 - ecx(0)��
 * 		%2 - esi(addr)��
 * ��addrָ����ַ��ʼ��λͼ��Ѱ�ҵ�1����0�ı���λ�����������addr�ı���λƫ��
 * ֵ���ء�addr�ǻ�����������ĵ�ַ��ɨ��Ѱ�ҵķ�Χ��1024�ֽڣ�8192����λ����	*/
#define find_first_zero(addr) ({ \
int __res; \
/* �巽��λ��								*/	__asm__("cld\n" \
/* ȡ[esi]����>eax��								*/	"1:\tlodsl\n\t" \
/* eax ��ÿλȡ����								*/	"notl %%eax\n\t" \
/* ��λ0 ɨ��eax ����1 �ĵ�1 ��λ����ƫ��ֵ����>edx��	*/	"bsfl %%eax,%%edx\n\t" \
/* ���eax ��ȫ��0������ǰ��ת�����2 ��(40 ��)�� 		*/	"je 2f\n\t" \
/* ƫ��ֵ����ecx(ecx����λͼ���׸���0�ı���λ��ƫ��ֵ)	*/	"addl %%edx,%%ecx\n\t" \
/* ��ǰ��ת�����3 ������������						*/	"jmp 3f\n" \
/* û���ҵ�0 ����λ����ecx ����1 �����ֵ�λƫ����32��	*/	"2:\taddl $32,%%ecx\n\t" \
/* �Ѿ�ɨ����8192 λ��1024 �ֽڣ�����				*/	"cmpl $8192,%%ecx\n\t" \
/* ����û��ɨ����1 �����ݣ�����ǰ��ת�����1 ����������	*/	"jl 1b\n" \
/* ������										*/	"3:" \
/* ��ʱecx ����λƫ������							*/	:"=c" (__res):"c" (0),"S" (addr):) ; \
__res;})




/* �ͷ��豸dev ���������е��߼���block��	*/
/* ��λָ���߼���block ���߼���λͼ����λ��	*/
/* ������dev ���豸�ţ�block ���߼���ţ��̿�ţ���	*/
void free_block(int dev, int block)
{
	struct super_block * sb;
	struct buffer_head * bh;

/* ����ȡ�豸dev���ļ�ϵͳ�ĳ�������Ϣ������������������ʼ�߼���ź��ļ�ϵͳ���߼�
 * ��������Ϣ�жϲ���block����Ч�ԡ����ָ���豸�����鲻���ڣ������ͣ�������߼���
 * ��С��������������1���߼���Ŀ�Ż��ߴ����豸�����߼�������Ҳ����ͣ����	*/
	if (!(sb = get_super(dev)))
		panic("trying to free block on nonexistent device");
	if (block < sb->s_firstdatazone || block >= sb->s_nzones)
		panic("trying to free block not in datazone");
/* Ȼ���hash����Ѱ�Ҹÿ����ݡ����ҵ������ж�����Ч�ԣ��������޸ĺ͸��±�־���ͷŸ�
 * ���ݿ顣�öδ������Ҫ��;��������߼���Ŀǰ�����ڸ��ٻ������У����ͷŶ�Ӧ�Ļ���顣
 *
 * ע��������γ���L56-66�������⣬��������ݿ鲻���ͷš���Ϊ��b_count > 1ʱ��
 * ��δ�������ӡһ����Ϣ��û��ִ���ͷŲ�����Ӧ�����¸Ķ��Ϻ��ʣ�
 *	bh = get_hash_table(dev,block);
 *	if (bh) {
 *		if (bh->b_count != 1) {		* ������ô�������1�������brelse()��
 *			brelse(bh);				* b_count--���˳����ÿ黹�����á�
 *			return;
 *		}
 *		bh->b_dirt=0;				* ����λ���޸ĺ��Ѹ��±�־��
 *		bh->b_uptodate=0;			* ��λ���±�־��
 *		if (bh->b_count) {			* ����ʱb_countΪ1�������brelse()�ͷ�֮��
 *			brelse(bh);
 *	}	
 */
	bh = get_hash_table(dev,block);
	if (bh) {
		if (bh->b_count != 1) {		/* ������ô�������1�������brelse()��	*/
			printk("trying to free block (%04x:%d), count=%d\n",
				   dev,block,bh->b_count);
			return;
		}
		bh->b_dirt=0;				/* ��λ�ࣨ���޸ģ���־λ��	*/
		bh->b_uptodate=0;			/* ��λ���±�־��	*/
		brelse(bh);
	}
/* �������Ǹ�λblock���߼���λͼ�еı���λ����0�����ȼ���block����������ʼ�����
 * �����߼���ţ���1��ʼ��������Ȼ����߼��飨���飩λͼ���в�������λ��Ӧ�ı���λ��
 * �����Ӧ����λԭ������0�������ͣ��������1���������1024�ֽڣ���8192����λ��
 * ���block/8192���ɼ����ָ����block���߼�λͼ�е��ĸ����ϡ���block&8191��
 * �Եõ�block���߼���λͼ��ǰ���еı���ƫ��λ�á�	*/
	block -= sb->s_firstdatazone - 1 ;
	if (clear_bit(block&8191,sb->s_zmap[block/8192]->b_data)) {
		printk("block (%04x:%d) ",dev,block+sb->s_firstdatazone-1);
		panic("free_block: bit already cleared");
	}
/* �������Ӧ�߼���λͼ���ڻ��������޸ı�־��	*/
	sb->s_zmap[block/8192]->b_dirt = 1;
}

/* ���豸����һ���߼��飨�̿飬���飩��
 * ��������ȡ���豸�ĳ����飬���ڳ������е��߼���λͼ��Ѱ�ҵ�һ��0ֵ����λ������
 * һ�������߼��飩��Ȼ����λ��Ӧ�߼������߼���λͼ�еı���λ������Ϊ���߼����ڻ�
 * ������ȡ��һ���Ӧ����顣��󽫸û�������㣬���������Ѹ��±�־�����޸ı�־��
 * �������߼���š�����ִ�гɹ��򷵻��߼���ţ��̿�ţ������򷵻�0��	*/
int new_block(int dev)
{
	struct buffer_head * bh;
	struct super_block * sb;
	int i,j;

/* ���Ȼ�ȡ�豸dev�ĳ����顣���ָ���豸�ĳ����鲻���ڣ������ͣ����Ȼ��ɨ���ļ�
 * ϵͳ��8���߼���λͼ��Ѱ���׸�0ֵ����λ����Ѱ�ҿ����߼��飬��ȡ���ø��߼����
 * ��š����ȫ��ɨ����8���߼���λͼ�����б���λ��i>=8��j>= 8192����û�ҵ�
 * 0ֵ����λ����λͼ���ڵĻ����ָ����Ч(bh =NULL)�򷵻�0�˳���û�п����߼��飩��	*/
	if (!(sb = get_super(dev)))
		panic("trying to get new block from nonexistant device");
	j = 8192;
	for (i=0 ; i<8 ; i++)
		if (bh=sb->s_zmap[i])
			if ((j=find_first_zero(bh->b_data))<8192)
				break;
	if (i>=8 || !bh || j>=8192)
		return 0;
/* ���������ҵ������߼���j��Ӧ�߼���λͼ�еı���λ������Ӧ����λ�Ѿ���λ�������
 * ͣ���������ô��λͼ�Ķ�Ӧ�����������޸ı�־����Ϊ�߼���λͼ����ʾ������������
 * �߼����ռ����������߼���λͼ�б���λƫ��ֵ��ʾ����������ʼ������Ŀ�ţ����
 * ������Ҫ������������1���߼���Ŀ�ţ���jת�����߼���š���ʱ������߼������
 * ���豸�ϵ����߼���������˵��ָ���߼����ڶ�Ӧ�豸�ϲ����ڡ�����ʧ�ܣ�����0�˳���	*/
	if (set_bit(j,bh->b_data))
		panic("new_block: bit already set");
	bh->b_dirt = 1;
	j += i*8192 + sb->s_firstdatazone-1;
	if (j >= sb->s_nzones)
		return 0;
/* Ȼ���ڸ��ٻ�������Ϊ���豸��ָ�����߼����ȡ��һ������飬�����ػ����ͷָ�롣
 * ��Ϊ��ȡ�õ��߼��������ô���һ��Ϊl(getblk()�л�����)���������Ϊ1��ͣ����
 * ������߼������㣬���������Ѹ��±�־�����޸ı�־��Ȼ���ͷŶ�Ӧ����飬����
 * �߼���š�	*/
	if (!(bh=getblk(dev,j)))
		panic("new_block: cannot get block");
	if (bh->b_count != 1)
		panic("new block: count is != 1");
	clear_block(bh->b_data);
	bh->b_uptodate = 1;
	bh->b_dirt = 1;
	brelse(bh);
	return j;
}

/* �ͷ�ָ����i�ڵ㡣
 * �ú��������жϲ���������i�ڵ�ŵ���Ч�ԺͿ��ͷ��ԡ���i�ڵ���Ȼ��ʹ��������
 * ���ͷš�Ȼ�����ó�������Ϣ��i�ڵ�λͼ���в�������λi�ڵ�Ŷ�Ӧ��i�ڵ�λͼ��
 * ����λ�������i�ڵ�ṹ��	*/
void free_inode(struct m_inode * inode)
{
	struct super_block * sb;
	struct buffer_head * bh;

/* �����жϲ�����������Ҫ�ͷŵ�i�ڵ���Ч�Ի�Ϸ��ԡ����i�ڵ�ָ��=NULL�����˳���
 * ���i�ڵ��ϵ��豸���ֶ�Ϊ0��˵���ýڵ�û��ʹ�á�������0��ն�Ӧi�ڵ���ռ�ڴ�
 * �������ء�memsetO������include/string.h��395�п�ʼ�����������0��дinode
 * ָ��ָ������������sizeof(*inode)���ڴ�顣	*/
	if (!inode)
		return;
	if (!inode->i_dev) {
		memset(inode,0,sizeof(*inode));
		return;
	}
/* �����i�ڵ㻹�������������ã������ͷţ�˵���ں������⣬ͣ��������ļ�������
 * ��Ϊ0�����ʾ���������ļ�Ŀ¼����ʹ�øýڵ㣬���Ҳ��Ӧ�ͷţ���Ӧ�÷Żصȡ�	*/
	if (inode->i_count>1) {
		printk("trying to free inode with count=%d\n",inode->i_count);
		panic("free_inode");
	}
	if (inode->i_nlinks)
		panic("trying to free inode with links");
/* ���ж���i�ڵ�ĺ�����֮�����ǿ�ʼ�����䳬������Ϣ�����е�i�ڵ�λͼ���в�����
 * ����ȡi�ڵ������豸�ĳ����飬�����豸�Ƿ���ڡ�Ȼ���ж�i�ڵ�ŵķ�Χ�Ƿ���ȷ��
 * ���i�ڵ�ŵ���0����ڸ��豸��i�ڵ������������0��i�ڵ㱣��û��ʹ�ã���
 * �����i�ڵ��Ӧ�Ľڵ�λͼ�����ڣ��������Ϊһ��������i�ڵ�λͼ��8192��
 * ��λ�����i_num?13(��i_num/8192)���Եõ���ǰi�ڵ�����ڵ�s_imap�������
 * ���̿顣	*/
	if (!(sb = get_super(inode->i_dev)))
		panic("trying to free inode on nonexistent device");
	if (inode->i_num < 1 || inode->i_num > sb->s_ninodes)
		panic("trying to free inode 0 or nonexistant inode");
	if (!(bh=sb->s_imap[inode->i_num>>13]))
		panic("nonexistent imap in superblock");
/* �������Ǹ�λ��Ӧ�Ľڵ�λͼ�еı���λ������ñ���λ�Ѿ�����0������ʾ����
//������Ϣ�������i�ڵ�λͼ���ڻ��������޸ı�־������ո�i�ڵ�ṹ��ռ�ڴ�����	*/
	if (clear_bit(inode->i_num&8191,bh->b_data))
		printk("free_inode: bit already cleared.\n\r");
	bh->b_dirt = 1;
	memset(inode,0,sizeof(*inode));
}

/* Ϊ�豸dev ����һ����i �ڵ㡣	���ظ���i �ڵ��ָ�롣
 * ���ڴ�i �ڵ���л�ȡһ������i �ڵ�������i �ڵ�λͼ����һ������i �ڵ㡣	*/
struct m_inode * new_inode(int dev)
{
	struct m_inode * inode;
	struct super_block * sb;
	struct buffer_head * bh;
	int i,j;

/* ���ȴ��ڴ�i�ڵ��inode_table���л�ȡһ������i�ڵ������ȡָ���豸�ĳ�����
 * �ṹ��Ȼ��ɨ�賬������8��i�ڵ�λͼ��Ѱ���׸�0����λ��Ѱ�ҿ��нڵ㣬��ȡ����
 * ��i�ڵ�Ľڵ�š����ȫ��ɨ���껹û�ҵ�������λͼ���ڵĻ������Ч��bh = NULL��,
 * ��Ż���ǰ�����i�ڵ���е�i�ڵ㣬�����ؿ�ָ���˳���û�п���i�ڵ㣩��	*/
	if (!(inode=get_empty_inode()))
		return NULL;
	if (!(sb = get_super(dev)))
		panic("new_inode with unknown device");
	j = 8192;
	for (i=0 ; i<8 ; i++)
		if (bh=sb->s_imap[i])
			if ((j=find_first_zero(bh->b_data))<8192)
				break;
	if (!bh || j >= 8192 || j+i*8192 > sb->s_ninodes) {
		iput(inode);
		return NULL;
	}
/* ���������Ѿ��ҵ��˻�δʹ�õ�i�ڵ��j��������λi�ڵ�j��Ӧ��i�ڵ�λͼ��Ӧ��
 * ��λ������Ѿ���λ���������Ȼ����i�ڵ�λͼ���ڻ�������޸ı�־������ʼ��
 * ��i�ڵ�ṹ��i��ctime��i�ڵ����ݸı�ʱ�䣩��		*/
	if (set_bit(j,bh->b_data))
		panic("new_inode: bit already set");
	bh->b_dirt = 1;
	inode->i_count=1;				/* ���ü�����	*/
	inode->i_nlinks=1;				/* �ļ�Ŀ¼����������	*/
	inode->i_dev=dev;				/* i �ڵ����ڵ��豸�š�	*/
	inode->i_uid=current->euid;		/* i �ڵ������û�id��	*/
	inode->i_gid=current->egid;		/* ��id��	*/
	inode->i_dirt=1;				/* ���޸ı�־��λ��	*/
	inode->i_num = j + i*8192;		/* ��Ӧ�豸�е�i �ڵ�š�	*/
	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;	/* ����ʱ�䡣	*/
	return inode;					/* ���ظ�i �ڵ�ָ�롣	*/
}
