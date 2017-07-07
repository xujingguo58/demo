/*
* linux/fs/namei.c
*
* (C) 1991 Linus Torvalds
*/

/*
* Some corrections by tytso.
*/
/*
* tytso ����һЩ������	
*/

#include <linux/sched.h>	/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 �����ݣ�	*/
							/* ����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/
#include <linux/kernel.h>	/* �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣	*/
#include <asm/segment.h>	/* �β���ͷ�ļ����������йضμĴ���������Ƕ��ʽ��ຯ����	*/
#include <string.h>			/* �ַ���ͷ�ļ�����Ҫ������һЩ�й��ַ���������Ƕ�뺯����	*/
#include <fcntl.h>			/* �ļ�����ͷ�ļ��������ļ������������Ĳ������Ƴ������ŵĶ��塣	*/
#include <errno.h>			/* �����ͷ�ļ�������ϵͳ�и��ֳ���š�(Linus ��minix ��������)��	*/
#include <const.h>			/* ��������ͷ�ļ���Ŀǰ��������i �ڵ���i_mode �ֶεĸ���־λ��	*/
#include <sys/stat.h>		/* �ļ�״̬ͷ�ļ��������ļ����ļ�ϵͳ״̬�ṹstat{}�ͳ�����	*/

/* �¶������Ҳ���ʽ�Ƿ��������һ������ʹ�÷�����������������һ����ʵ��������������
�����±�����ʾ��������(����a[b])��ֵ��ͬ��ʹ��������ָ��(��ַ)���ϸ���ƫ�Ƶ�ַ
����ʽ��ֵ*(a+b)��ͬʱ��֪��a[b]Ҳ���Ա�ʾ��b[a]����ʽ��ld �˶����ַ���������ʽ
Ϊ��LoveYou"[2](����2 ["LoveYou"])�͵�ͬ��*("LoveYou"+2)�����⣬�ַ�����LoveYou��
���ڴ��б��洢��λ�þ������ַ����������LoveYou"[2]��ֵ���Ǹ��ַ���������ֵΪ2
���ַ���v"����Ӧ��ASCII��ֵ0x76�����ð˽��Ʊ�ʾ����0166����C�����У��ַ�Ҳ������
��ASCII��ֵ����ʾ�����������ַ���ASCII��ֵǰ����һ����б�ܡ������ַ���v"���Ա�ʾ
�ɡ�\x76�����ߡ�\166"����˶��ڲ�����ʾ���ַ�(����ASCII��ֵΪ0x00--Oxlf�Ŀ����ַ�)
�Ϳ�����ASCII��ֵ����ʾ��
�¶��Ƿ���ģʽ�ꡣx��ͷ�ļ�include/fcntl.h�е�7�п�ʼ������ļ�����(��)��־��
���������ļ����ʱ�־x��ֵ������˫�����ж�Ӧ����ֵ��˫��������4���˽�����ֵ(ʵ
�ʱ�ʾ4�������ַ�):"\004\002\006\377"���ֱ��ʾ����������ִ�е�Ȩ��Ϊ:r,	w,	rw
��wxrwxrwx����Ŀ���ֱ��Ӧx������ֵ0--3�����磬���xΪ2����ú귵�ذ˽���ֵ006
��ʾ�ɶ���дCrw)�����⣬����0 l1CCNfOD1:��00003��������ֵx�������롣

	*/
#define ACC_MODE(x) ( "\004\002\006\377"[(x)&O_ACCMODE])

/*
* comment out this line if you want names > NAME_LEN chars to be
* truncated. Else they will be disallowed.
*/
/*
* ��������ļ�������>NAME_LEN ���ַ����ص����ͽ����涨��ע�͵���
*/
/* #define NO_TRUNCATE */

#define MAY_EXEC 1			/* ��ִ��(�ɽ���)��	*/
#define MAY_WRITE 2			/* ��д��	*/
#define MAY_READ 4			/* �ɶ���	*/

/*
* permission()
*
* is used to check for read/write/execute permissions on a file.
* I don't know if we should look at just the euid or both euid and
* uid, but that should be easily changed.
*/
/*
* permission()
* �ú������ڼ��һ���ļ��Ķ�/д/ִ��Ȩ�ޡ�
* �Ҳ�֪���Ƿ�ֻ����euid��������Ҫ���euid ��uid ���ߣ�������������޸ġ�
*/
/* ����ļ��������Ȩ�ޡ�	*/
/* ������inode - �ļ���Ӧ��i �ڵ㣻	*/
/* mask - �������������롣	*/
/* ���أ�������ɷ���1�����򷵻�0��	*/
static int
permission (struct m_inode *inode, int mask)
{
	int mode = inode->i_mode;	/* �ļ���������	*/
/* special case: not even root can read/write a deleted file */
/* �����������ʹ�ǳ����û���root��Ҳ���ܶ�/дһ���ѱ�ɾ�����ļ�*/
	
/* ���i�ڵ��ж�Ӧ���豸������i�ڵ�����Ӽ���ֵ����0����ʾ���ļ��ѱ�ɾ�����򷵻ء�
 * ����������̵���Ч�û�id(euid)��i�ڵ���û�id��ͬ����ȡ�ļ������ķ���Ȩ�ޡ�
 * ����������̵���Ч��id(egid)��i�ڵ����id��ͬ����ȡ���û��ķ���Ȩ�ޡ�	*/
	if (inode->i_dev && !inode->i_nlinks)
		return 0;
	else if (current->euid == inode->i_uid)
		mode >>= 6;
	else if (current->egid == inode->i_gid)
		mode >>= 3;
/* ����ж������ȡ�ĵķ���Ȩ������������ͬ�������ǳ����û����򷵻�1�����򷵻�0��	*/
	if (((mode & mask & 0007) == mask) || suser ())
		return 1;
	return 0;
}

/*
* ok, we cannot use strncmp, as the name is not in our data space.
* Thus we'll have to use match. No big problem. Match also makes
* some sanity tests.
*
* NOTE! unlike strncmp, match returns 1 for success, 0 for failure.
*/
/*
* ok�����ǲ���ʹ��strncmp �ַ����ȽϺ�������Ϊ���Ʋ������ǵ����ݿռ�(�����ں˿ռ�)��
* �������ֻ��ʹ��match()��	���ⲻ��match()ͬ��Ҳ����һЩ�����Ĳ��ԡ�
*
* ע�⣡��strncmp ��ͬ����match()�ɹ�ʱ����1��ʧ��ʱ����0��	
*/

/* ָ�������ַ����ȽϺ�����
 * ������len -�Ƚϵ��ַ������ȣ�name -�ļ���ָ�룻de -Ŀ¼��ṹ��
 * ���أ���ͬ����1����ͬ����0��
 * ��65���϶�����һ���ֲ��Ĵ�������same���ñ�������������eax�Ĵ����У��Ա��ڸ�Ч���ʡ�
 */
static int match(int len,const char * name,struct dir_entry * de)
{
	register int same __asm__("ax");
/* �����жϺ�����������Ч�ԡ����Ŀ¼��ָ��գ�����Ŀ¼��i�ڵ����0������Ҫ�Ƚϵ�
 * �ַ������ȳ����ļ������ȣ��򷵻�0�����Ҫ�Ƚϵĳ���lenС��NAME��LEN������Ŀ¼��
 * ���ļ������ȳ���len��Ҳ����0��
 * ��69���϶�Ŀ¼�����ļ��������Ƿ񳬹�len���жϷ����Ǽ��name[len]�Ƿ�ΪNULL��
 * �����ȳ���len����name[len]������һ������NULL����ͨ�ַ��������ڳ���Ϊlen���ַ�
 * ��name,�ַ�name[len]��Ӧ����NULL��	*/

	if (!de || !de->inode || len > NAME_LEN)
		return 0;
	if (len < NAME_LEN && de->name[len])
		return 0;
/* Ȼ��ʹ��Ƕ���������п��ٱȽϲ������������û����ݿռ䣨fs�Σ�ִ���ַ����ıȽ�
 * ������%0 - eax (�ȽϽ�� same);
 *		%l - eax(eax��ֵ0);
 *		%2 - esi (����ָ��)��
 *		%3 - edi (Ŀ¼����ָ��)��
 *		%4 - ecx(�Ƚϵ��ֽڳ���ֵlen)��	*/
/* �巽��λ��								*/	__asm__("cld\n\t"
/* �û��ռ�ִ��ѭ���Ƚ�[esi++]��[edi++]������*/		"fs ; repe ; cmpsb\n\t"
/* ���ȽϽ��һ��(z=0)������al=1(same=eax)��*/		"setz %%al"
												:"=a" (same)
												:"0" (0),"S" ((long) name),"D" ((long) de->name),"c" (len)
												:) ;
	return same;		/* ���رȽϽ����	*/
}
/*
* find_entry()
*
* finds an entry in the specified directory with the wanted name. It
* returns the cache buffer in which the entry was found, and the entry
* itself (as a parameter - res_dir). It does NOT read the inode of the
* entry - you'll have to do that yourself if you want to.
*
* This also takes care of the few special cases due to '..'-traversal
* over a pseudo-root and a mount point.
*/
/*
* find_entry()
* ��ָ��Ŀ¼��Ѱ��һ��������ƥ���Ŀ¼�����һ�������ҵ�Ŀ¼��ĸ���
* ������Լ�Ŀ¼�����Ϊһ������-res��dir�����ú���������ȡĿ¼��
* ��i�ڵ�-�����Ҫ�Ļ����Լ�������
*
* ������Ŀ¼�����ڲ����ڼ�Ҳ��Լ�����������ֱ���-�����Խ
* һ��α��Ŀ¼�Լ���װ�㡣
*/
/* ����ָ��Ŀ¼���ļ�����Ŀ¼�
* ������*dir -ָ��Ŀ¼i�ڵ��ָ�룻name -�ļ�����namelen -�ļ������ȣ�
* �ú�����ָ��Ŀ¼�����ݣ��ļ���������ָ���ļ�����Ŀ¼�����ָ���ļ����ǵ���
* �����ݵ�ǰ���е�������ý������⴦�����ں�����������ָ���ָ������ã����
* ��linux/sched.c��151��ǰ��ע�͡�
* ���أ��ɹ��������ٻ�����ָ�룬������es_dir�����ص�Ŀ¼��ṹָ�롣ʧ���򷵻�
* ��ָ��NULL��
*/
static struct buffer_head *
find_entry (struct m_inode **dir,
			const char *name, int namelen, struct dir_entry **res_dir)
{
	int entries;
	int block, i;
	struct buffer_head *bh;
	struct dir_entry *de;
	struct super_block *sb;

/* ͬ����������һ����Ҳ��Ҫ�Ժ�����������Ч�Խ����жϺ���֤�����������ǰ���27��
 * �����˷��ų���NO��TRUNCATE����ô����ļ������ȳ�����󳤶�NAME��LEN�����账��
 * ���û�ж����NO��TRUNCATE����ô���ļ������ȳ�����󳤶�NAME��LENʱ�ض�֮��	*/
#ifdef NO_TRUNCATE
	if (namelen > NAME_LEN)
		return NULL;
#else
	if (namelen > NAME_LEN)
		namelen = NAME_LEN;
#endif
/* ���ȼ��㱾Ŀ¼��Ŀ¼������entries��Ŀ¼i�ڵ�i_size�ֶ��к��б�Ŀ¼����������
 * ���ȣ���������һ��Ŀ¼��ĳ��ȣ�16�ֽڣ����ɵõ���Ŀ¼��Ŀ¼������Ȼ���ÿշ���
 * Ŀ¼��ṹָ�롣����ļ������ȵ���0���򷵻�NULL���˳���	*/
	entries = (*dir)->i_size / (sizeof (struct dir_entry));
	*res_dir = NULL;
	if (!namelen)
		return NULL;
/* ���������Ƕ�Ŀ¼���ģ����ǡ�..��������������⴦�������ǰ����ָ���ĸ�i�ڵ����
 * ��������ָ����Ŀ¼����B�������ڱ�������˵�����Ŀ¼��������α��Ŀ¼��������ֻ�ܷ�
 * �ʸ�Ŀ¼�е�������ܺ��˵��丸Ŀ¼��ȥ��Ҳ�����ڸý��̱�Ŀ¼����ͬ���ļ�ϵͳ�ĸ�
 * Ŀ¼�����������Ҫ���ļ����޸�Ϊ
 * ���������Ŀ¼��i�ڵ�ŵ���R00T_IN0(1��)�Ļ���˵��ȷʵ���ļ�ϵͳ�ĸ�i�ڵ㡣
 * ��ȡ�ļ�ϵͳ�ĳ����顣�������װ����i�ڵ���ڣ����ȷŻ�ԭi�ڵ㣬Ȼ��Ա���װ��
 * ��i�ڵ���д�������������*dirָ��ñ���װ����i�ڵ㣻���Ҹ�i�ڵ����������1��
 * ���������������������ĵؽ����ˡ�͵�����������̡�
*/
/* check for '..', as we might have to do some "magic" for it */
/* ���Ŀ¼��'..'����Ϊ������Ҫ�����ر��� */
	if (namelen == 2 && get_fs_byte (name) == '.'
			&& get_fs_byte (name + 1) == '.')
	{
/* '..' in a pseudo-root results in a faked '.' (just change namelen) */
/* α���е�'..'��ͬһ����'.'(ֻ��ı����ֳ���) */
		if ((*dir) == current->root)
			namelen = 1;
		else if ((*dir)->i_num == ROOT_INO)
		{
/* '..' over a mount-point results in 'dir' being exchanged for the mounted
directory-inode. NOTE! We set mounted, so that we can iput the new dir */
/* ��һ����װ���ϵ�'..'������Ŀ¼��������װ���ļ�ϵͳ��Ŀ¼i �ڵ㡣
 * ע�⣡����������mounted ��־����������ܹ�ȡ������Ŀ¼ */
			sb = get_super ((*dir)->i_dev);
			if (sb->s_imount)
				{
					iput (*dir);
					(*dir) = sb->s_imount;
					(*dir)->i_count++;
				}
		}
	}
/* �������ǿ�ʼ��������������ָ���ļ�����Ŀ¼����ʲô�ط������������Ҫ��ȡĿ¼����
 * �ݣ���ȡ��Ŀ¼i�ڵ��Ӧ���豸�������е����ݿ飨�߼��飩��Ϣ����Щ�߼���Ŀ�ű�
 * ����i�ڵ�ṹ��i_zone[9]�����С�������ȡ���е�1����š����Ŀ¼i�ڵ�ָ��ĵ�
 * һ��ֱ�Ӵ��̿��Ϊ0����˵����Ŀ¼��Ȼ�������ݣ��ⲻ���������Ƿ���NULL�˳�������
 * ���Ǿʹӽڵ������豸��ȡָ����Ŀ¼�����ݿ顣��Ȼ��������ɹ�����Ҳ����NULL�˳���	*/
	if (!(block = (*dir)->i_zone[0]))
		return NULL;
	if (!(bh = bread ((*dir)->i_dev, block)))
		return NULL;
/* ��ʱ���Ǿ��������ȡM¼i�ڵ����ݿ�������ƥ��ָ���ļ�����Ŀ¼�������deָ
 * �򻺳���е����ݿ鲿�֣����ڲ�����Ŀ¼��Ŀ¼�����������£�ѭ��ִ������������i��
 * Ŀ¼�е�Ŀ¼�������ţ���ѭ����ʼʱ��ʼ��Ϊ0��	*/
	i = 0;
	de = (struct dir_entry *) bh->b_data;
	while (i < entries)
	{
/* �����ǰĿ¼�����ݿ��Ѿ������꣬��û���ҵ�ƥ���Ŀ¼����ͷŵ�ǰĿ¼�����ݿ顣
 * �ٶ���Ŀ¼����һ���߼��顣�����Ϊ�գ���ֻҪ��û��������Ŀ¼�е�����Ŀ¼���
 * �����ÿ飬������Ŀ¼����һ�߼��顣���ÿ鲻�գ�����deָ������ݿ飬Ȼ��������
 * ��������������137����i/DIR_ENTRIES��PER��BLOCK�ɵõ���ǰ������Ŀ¼������Ŀ¼��
 * ���еĿ�ţ���bmapO������inode.c����142�У���ɼ�������豸�϶�Ӧ���߼���š�	*/
		if ((char *) de >= BLOCK_SIZE + bh->b_data)
		{
			brelse (bh);
			bh = NULL;
			if (!(block = bmap (*dir, i / DIR_ENTRIES_PER_BLOCK)) ||
					!(bh = bread ((*dir)->i_dev, block)))
				{
					i += DIR_ENTRIES_PER_BLOCK;
					continue;
				}
			de = (struct dir_entry *) bh->b_data;
		}
/* ����ҵ�ƥ���Ŀ¼��Ļ����򷵻ظ�Ŀ¼��ṹָ��de�͸�Ŀ¼��i�ڵ�ָ��*dir��
 * ����Ŀ¼�����ݿ�ָ��bh�����˳����������������Ŀ¼�����ݿ��бȽ���һ��Ŀ¼�	*/
		if (match (namelen, name, de))
		{
			*res_dir = de;
			return bh;
		}
			de++;
			i++;
	}
/* ���ָ��Ŀ¼�е�����Ŀ¼�������󣬻�û���ҵ���Ӧ��Ŀ¼����ͷ�Ŀ¼������
 * �飬��󷵻�NULL (ʧ��)��	*/
	brelse (bh);
	return NULL;
}

/*
* add_entry()
*
* adds a file entry to the specified directory, using the same
* semantics as find_entry(). It returns NULL if it failed.
*
* NOTE!! The inode part of 'de' is left at 0 - which means you
* may not sleep between calling this and putting something into
* the entry, as someone else might have used it while you slept.
*/
/*
* add_entry()
* ʹ����find_entry()ͬ���ķ�������ָ��Ŀ¼�����һ�ļ�Ŀ¼�
* ���ʧ���򷵻�NULL��
*
* ע�⣡��'de'(ָ��Ŀ¼��ṹָ��)��i �ڵ㲿�ֱ�����Ϊ0 - ���ʾ
* �ڵ��øú�������Ŀ¼���������Ϣ֮�䲻��˯�ߣ���Ϊ��˯����ô����
* ��(����)���ܻ��Ѿ�ʹ���˸�Ŀ¼�
*/

/* ����ָ����Ŀ¼���ļ������Ŀ¼�
 * ������dir -ָ��Ŀ¼��i�ڵ㣻name -�ļ�����namelen -�ļ������ȣ�
 * ���أ����ٻ�����ָ�룻res��dir -���ص�Ŀ¼��ṹָ�룻	*/
static struct buffer_head *
add_entry (struct m_inode *dir,
		 const char *name, int namelen, struct dir_entry **res_dir)
{
	int block, i;
	struct buffer_head *bh;
	struct dir_entry *de;

/* ͬ����������һ����Ҳ��Ҫ�Ժ�����������Ч�Խ����жϺ���֤�����������ǰ���27��
 * �����˷��ų���NO��TRUNCATE����ô����ļ������ȳ�����󳤶�NAME��LEN�����账��
 * ���û�ж����NO��TRUNCATE����ô���ļ������ȳ�����󳤶�NAME��LENʱ�ض�֮��	*/
	*res_dir = NULL;				/* ���ڷ���Ŀ¼��ṹָ�롣	*/
#ifdef NO_TRUNCATE
	if (namelen > NAME_LEN)
		return NULL;
#else
	if (namelen > NAME_LEN)
		namelen = NAME_LEN;
#endif
/* �������ǿ�ʼ��������ָ��Ŀ¼�����һ��ָ���ļ�����Ŀ¼����������Ҫ�ȶ�ȡĿ¼
 * �����ݣ���ȡ��Ŀ¼i�ڵ��Ӧ���豸�������е����ݿ飨�߼��飩��Ϣ����Щ�߼���Ŀ�
 * �ű�����i�ڵ�ṹ��i_zone[9]�����С�������ȡ���е�1����š����Ŀ¼i�ڵ�ָ��
 * �ĵ�һ��ֱ�Ӵ��̿��Ϊ0����˵����Ŀ¼��Ȼ�������ݣ��ⲻ���������Ƿ���NULL�˳���
 * �������Ǿʹӽڵ������豸��ȡָ����Ŀ¼�����ݿ顣��Ȼ��������ɹ�����Ҳ����NULL
 * �˳������⣬��������ṩ���ļ������ȵ���0����Ҳ����NULL�˳���	*/
	if (!namelen)
		return NULL;
	if (!(block = dir->i_zone[0]))
		return NULL;
	if (!(bh = bread (dir->i_dev, block)))
		return NULL;
/* ��ʱ���Ǿ������Ŀ¼�����ݿ���ѭ���������δʹ�õĿ�Ŀ¼�������Ŀ¼��ṹ
 * ָ��deָ�򻺳���е����ݿ鲿�֣�����һ��Ŀ¼�������i��Ŀ¼�е�Ŀ¼�������ţ�
 * ��ѭ����ʼʱ��ʼ��Ϊ0��	*/
	i = 0;
	de = (struct dir_entry *) bh->b_data;
	while (1)
	{
/* �����ǰĿ¼�����ݿ��Ѿ�������ϣ�����û���ҵ���Ҫ�Ŀ�Ŀ¼����ͷŵ�ǰĿ¼����
 * �ݿ飬�ٶ���Ŀ¼����һ���߼��顣�����Ӧ���߼��鲻���ھʹ���һ�顣����ȡ�򴴽���
 * ��ʧ���򷵻ؿա�����˴ζ�ȡ�Ĵ����߼������ݷ��صĻ����ָ��Ϊ�գ�˵������߼���
 * ��������Ϊ�����ڶ��´����Ŀտ飬���Ŀ¼������ֵ����һ���߼����������ɵ�Ŀ¼����
 * DIR_ENTRIES_PER_BLOCK�����������ÿ鲢��������������˵���¶���Ŀ�����Ŀ¼�����ݣ�
 * ������Ŀ¼��ṹָ��deָ��ÿ�Ļ�������ݲ��֣�Ȼ�������м�������������192��
 * �ϵ�i/DIR_ENTRIES_PER_BLOCK�ɼ���õ���ǰ������Ŀ¼��i����Ŀ¼�ļ��еĿ�ţ�
 * ��create_blockO������inode.c����145�У���ɶ�ȡ�򴴽������豸�϶�Ӧ���߼��顣	*/
		if ((char *) de >= BLOCK_SIZE + bh->b_data)
		{
			brelse (bh);
			bh = NULL;
			block = create_block (dir, i / DIR_ENTRIES_PER_BLOCK);
			if (!block)
				return NULL;
			if (!(bh = bread (dir->i_dev, block)))
			{								/* �����������ÿ������	*/
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			de = (struct dir_entry *) bh->b_data;
		}
/* �����ǰ��������Ŀ¼�����i����Ŀ¼�ṹ��С���ĳ���ֵ�Ѿ������˸�Ŀ¼i�ڵ���Ϣ
 * ��ָ����Ŀ¼���ݳ���ֵi_size����˵������Ŀ¼�ļ�������û������ɾ���ļ����µĿ�
 * Ŀ¼��������ֻ�ܰ���Ҫ��ӵ���Ŀ¼��ӵ�Ŀ¼�ļ����ݵ�ĩ�˴������ǶԸô�Ŀ
 * ¼��������ã��ø�Ŀ¼���i�ڵ�ָ��Ϊ�գ��������¸�Ŀ¼�ļ��ĳ���ֵ������һ��Ŀ
 * ¼��ĳ��ȣ���Ȼ������Ŀ¼��i�ڵ����޸ı�־���ٸ��¸�Ŀ¼�ĸı�ʱ��Ϊ��ǰʱ�䡣	*/
		if (i * sizeof (struct dir_entry) >= dir->i_size)
		{
			de->inode = 0;
			dir->i_size = (i + 1) * sizeof (struct dir_entry);
			dir->i_dirt = 1;
			dir->i_ctime = CURRENT_TIME;
		}
		if (!de->inode)
		{
			dir->i_mtime = CURRENT_TIME;
			for (i = 0; i < NAME_LEN; i++)
				de->name[i] = (i < namelen) ? get_fs_byte (name + i) : 0;
			bh->b_dirt = 1;
			*res_dir = de;
			return bh;
		}
			de++;		/* �����Ŀ¼���Ѿ���ʹ�ã�����������һ��Ŀ¼�	*/
			i++;
	}
/* ������ִ�в��������Ҳ����Linus��д��δ���ʱ���ȸ���������Hnd��entryO����
 * �Ĵ��룬�����޸ĳɱ������ġ�	*/
	brelse (bh);
	return NULL;
}

/*
* get_dir()
*
* Getdir traverses the pathname until it hits the topmost directory.
* It returns NULL on failure.
*/
/*
* get_dir()
* �ú������ݸ�����·��������������ֱ���ﵽ��˵�Ŀ¼�����ʧ���򷵻�NULL��
*/

/* ��Ѱָ��·������Ŀ¼�����ļ�������i�ڵ㡣
 * ������pathname ��·������
 * ���أ�Ŀ¼���ļ���i�ڵ�ָ�롣ʧ��ʱ����NULL��	*/
static struct m_inode *
get_dir (const char *pathname)
{
	char c;
	const char *thisname;
	struct m_inode *inode;
	struct buffer_head *bh;
	int namelen, inr, idev;
	struct dir_entry *de;

/* ����������ӵ�ǰ��������ṹ�����õĸ�����α����i�ڵ��ǰ����Ŀ¼i�ڵ㿪ʼ��
 * ���������Ҫ�жϽ��̵ĸ�i�ڵ�ָ��͵�ǰ����Ŀ¼i�ڵ�ָ���Ƿ���Ч�������ǰ����
 * û���趨��i�ڵ㣬���߸ý��̸�i�ڵ�ָ����һ������i�ڵ㣨����Ϊ0������ϵͳ����
 * ͣ����������̵ĵ�ǰ����Ŀ¼i�ڵ�ָ��Ϊ�գ����߸õ�ǰ����Ŀ¼ָ���i�ڵ���һ��
 * ����i�ڵ㣬��Ҳ��ϵͳ�Ѓ��⣬ͣ����	*/
	if (!current->root || !current->root->i_count)
		panic ("No root inode");
	if (!current->pwd || !current->pwd->i_count)
		panic ("No cwd inode");
/* ����û�ָ����·�����ĵ�1���ַ��ǡ�/������˵��·�����Ǿ���·��������Ӹ�i�ڵ㿪
 * ʼ��������������һ���ַ��������ַ������ʾ�����������·������Ӧ�ӽ��̵ĵ�ǰ����
 * Ŀ¼��ʼ��������ȡ���̵�ǰ����Ŀ¼��i�ڵ㡣���·����Ϊ�գ��������NULL�˳���
 * ��ʱ����inodeָ������ȷ��i�ڵ�һ���̵ĸ�i�ڵ��ǰ����Ŀ¼i�ڵ�֮һ��	*/
	if ((c = get_fs_byte (pathname)) == '/')
	{
		inode = current->root;
		pathname++;
	}
	else if (c)
		inode = current->pwd;
	else
		return NULL;		/* empty name is bad	�յ�·�����Ǵ���� */
/* /Ȼ�����·�����еĸ�¼�����ֺ��ļ�������ѭ���������Ȱѵõ���i�ڵ����ü���
 * ��1����ʾ��������ʹ�á���ѭ����������У�������Ҫ�Ե�ǰ���ڴ����Ŀ¼�����֣���
 * �ļ�������i�ڵ������Ч���жϣ����Ұѱ���thisnameָ��ǰ���ڴ����Ŀ¼������
 * (���ļ���)�������i�ڵ㲻��Ŀ¼���͵�i�ڵ㣬����û�пɽ����Ŀ¼�ķ�����ɣ�
 * ��Żظ�i�ڵ㣬������NULL�˳�����Ȼ���ս���ѭ��ʱ����ǰ��i�ڵ���ǽ��̸�i��
 * ������ǵ�ǰ����Ŀ¼��i�ڵ㡣	*/
	inode->i_count++;
	while (1)
	{
		thisname = pathname;
		if (!S_ISDIR (inode->i_mode) || !permission (inode, MAY_EXEC))
		{
			iput (inode);
			return NULL;
		}
/* ÿ��ѭ�����Ǵ���·������һ��Ŀ¼�������ļ��������֡������ÿ��ѭ�������Ƕ�Ҫ��·
 * �����ַ����з����һ��Ŀ¼�������ļ������������Ǵӵ�ǰ·����ָ��pathname��ʼ��
 * ��������ַ���ֱ���ַ���һ����β����NULL��������һ����/���ַ�����ʱ����namelen��
 * ���ǵ�ǰ����Ŀ¼�����ֵĳ��ȣ�������thisname��ָ���Ŀ¼�����ֵĿ�ʼ������ʱ��
 * ���ַ��ǽ�β��NULL��������Ѿ�������·����ĩβ�����ѵ������ָ��Ŀ¼�����ļ�����
 * �򷵻ظ�i�ڵ�ָ���˳���
 * ע�⣡���·���������һ������Ҳ��һ��Ŀ¼�����������û�м��ϡ�/���ַ���������
 * �᷵�ظ����Ŀ¼��i�ڵ㣡���磺����·����/usr/src/linux���ú�����ֻ����src/Ŀ¼
 * ����i�ڵ㡣	*/
		for (namelen = 0; (c = get_fs_byte (pathname++)) && (c != '/');
			namelen++)
						/* nothing */ ;
		if (!c)
			return inode;
/* �ڵõ���ǰĿ¼�����֣����ļ����������ǵ��ò���Ŀ¼���find��entryO�ڵ�ǰ��
 * ���Ŀ¼��Ѱ��ָ�����Ƶ�Ŀ¼����û���ҵ�����Żظ�i�ڵ㣬������NULL�˳���
 * Ȼ�����ҵ���Ŀ¼����ȡ����i�ڵ��inr���豸��idev���ͷŰ�����Ŀ¼��ĸ��ٻ���
 * �鲢�Żظ�i�ڵ㡣Ȼ��ȡ�ڵ��inr��i�ڵ�inode�����Ը�Ŀ¼��Ϊ��ǰĿ¼����ѭ
 * ������·�����е���һĿ¼�����֣����ļ�������	*/
		if (!(bh = find_entry (&inode, thisname, namelen, &de)))
		{
			iput (inode);					/* ��ǰĿ¼�����ֵ�i�ڵ�š�	*/
			return NULL;
		}
		inr = de->inode;
		idev = inode->i_dev;
		brelse (bh);
		iput (inode);
		if (!(inode = iget (idev, inr)))	/* ȡ i �ڵ����ݡ�	*/
	return NULL;
	}
}

/*
* dir_namei()
*
* dir_namei() returns the inode of the directory of the
* specified name, and the name within that directory.
*/
/*
* dir_namei()
* dir_namei()��������ָ��Ŀ¼����i �ڵ�ָ�룬�Լ������Ŀ¼�����ơ�
*/
/* ������pathname -Ŀ¼·������namelen -·�������ȣ�name -���ص����Ŀ¼����
 * ���أ�ָ��Ŀ¼�����Ŀ¼��i�ڵ�ָ������Ŀ¼���Ƽ����ȡ�����ʱ����NULL��
 * ע�⣡��������Ŀ¼����ָ·���������ĩ�˵�Ŀ¼��	*/
static struct m_inode *
dir_namei (const char *pathname, int *namelen, const char **name)
{
	char c;
	const char *basename;
	struct m_inode *dir;

/* ����ȡ��ָ��·�������Ŀ¼��i�ڵ㡣Ȼ���·����pathname����������⣬���
 * ���һ����/���ַ�����������ַ����������䳤�ȣ����ҷ������Ŀ¼��i�ڵ�ָ�롣
 * ע�⣡���·�������һ���ַ���б���ַ���/������ô���ص�Ŀ¼��Ϊ�գ����ҳ���Ϊ0��
 * �����ص�i�ڵ�ָ����Ȼָ�����һ����/���ַ�ǰĿ¼����i�ڵ㡣�μ���255���ϵ�
 *  ��ע�⡱˵����	*/
	if (!(dir = get_dir (pathname)))
		return NULL;
	basename = pathname;
	while (c = get_fs_byte (pathname++))
		if (c == '/')
			basename = pathname;
	*namelen = pathname - basename - 1;
	*name = basename;
	return dir;
}

/*
* namei()
*
* is used by most simple commands to get the inode of a specified name.
* Open, link etc use their own routines, but this is enough for things
* like 'chmod' etc.
*/
/*
* namei()
* �ú��������򵥵���������ȡ��ָ��·�����Ƶ�i �ڵ㡣* open��link ����ʹ������
* �Լ�����Ӧ���������������޸�ģʽ'chmod'������������ú������㹻���ˡ�
*/
/* ȡָ��·������i �ڵ㡣	*/
/* ������pathname - ·������	*/
/* ���أ���Ӧ��i �ڵ㡣	*/
struct m_inode *
namei (const char *pathname)
{
	const char *basename;
	int inr, dev, namelen;
	struct m_inode *dir;
	struct buffer_head *bh;
	struct dir_entry *de;

/* ���Ȳ���ָ��·�������Ŀ¼��Ŀ¼�����õ���i�ڵ㣬�������ڣ��򷵻�NULL�˳���
 * ������ص�������ֵĳ�����0�����ʾ��·������һ��Ŀ¼��Ϊ���һ��������
 * �Ѿ��ҵ���ӦĿ¼��i�ڵ㣬����ֱ�ӷ��ظ�i�ڵ��˳���	*/
	if (!(dir = dir_namei (pathname, &namelen, &basename)))
		return NULL;
	if (!namelen)			/* special case: '/usr/' etc */
		return dir;			/* ��Ӧ��'/usr/'����� */
/* Ȼ���ڷ��صĶ���Ŀ¼��Ѱ��ָ���ļ���Ŀ¼���i�ڵ㡣ע�⣡��Ϊ������Ҳ��һ��Ŀ
 * ¼���������û�мӡ�/�����򲻻᷵�ظ����Ŀ¼��i�ڵ㣡���磺/usr/src/linux����ֻ
 * ����src/Ŀ¼����i�ڵ㡣��Ϊ����dir��nameiO�Ѳ��ԡ�/�����������һ�����ֵ���һ��
 * �ļ���������������������Ҫ�������������ʹ��Ѱ��Ŀ¼��i�ڵ㺯��find��entryO����
 * ������ʱde�к���Ѱ�ҵ���Ŀ¼��ָ�룬��dir�ǰ�����Ŀ¼���Ŀ¼��i�ڵ�ָ�롣	*/
	bh = find_entry (&dir, basename, namelen, &de);
	if (!bh)
		{
			iput (dir);
			return NULL;
		}
/* ����ȡ��Ŀ¼���i�ڵ�ź��豸�ţ����ͷŰ�����Ŀ¼��ĸ��ٻ���鲢�Ż�Ŀ¼i�ڵ㡣
 * Ȼ��ȡ��Ӧ�ڵ�ŵ�i�ڵ㣬�޸��䱻����ʱ��Ϊ��ǰʱ�䣬�������޸ı�־����󷵻ظ�i
 * �ڵ�ָ�롣	*/
	inr = de->inode;
	dev = dir->i_dev;
	brelse (bh);
	iput (dir);
	dir = iget (dev, inr);
	if (dir)
		{
			dir->i_atime = CURRENT_TIME;
			dir->i_dirt = 1;
		}
	return dir;
}

/*
* open_namei()
*
* namei for open - this is in fact almost the whole open-routine.
*/
/*
* open_namei()
* open()��ʹ�õ�namei ���� - ����ʵ�����������Ĵ��ļ�����
*/
/* �ļ���namei������
 * ����filename���ļ�����flag�Ǵ��ļ���־������ȡֵ��0_RD0NLY (ֻ��)��0_WR0NLY
 * (ֻд)��0_RDWR (��д)���Լ�0_CREAT (����)��0_EXCL (�������ļ����벻����)��
 * 0_APPEND (���ļ�β�������)������һЩ��־����ϡ���������ô�����һ�����ļ�����
 * mode������ָ���ļ���������ԡ���Щ������S_IRWXU (�ļ��������ж���д��ִ��Ȩ��)��
 * S��IRUSR (�û����ж��ļ�Ȩ��)��S��IRWXG(���Ա���ж���д��ִ��Ȩ��)�ȵȡ�������
 * �������ļ�����Щ����ֻӦ���ڽ������ļ��ķ��ʣ�������ֻ���ļ��Ĵ򿪵���Ҳ������һ
 * ���ɶ�д���ļ�������μ�����ļ�sys/stat.h��fcntl.h��
 * ���أ��ɹ�����0�����򷵻س����룻res_inode -���ض�Ӧ�ļ�·������i�ڵ�ָ�롣
	*/
int open_namei (const char *pathname, int flag, int mode,
			struct m_inode **res_inode)
{
	const char *basename;
	int inr, dev, namelen;
	struct m_inode *dir, *inode;
	struct buffer_head *bh;
	struct dir_entry *de;

/* ���ȶԺ����������к���Ĵ�������ļ�����ģʽ��־��ֻ����0���������ļ������־
 * 0_TRUNCȴ��λ�ˣ������ļ��򿪱�־�����ֻд��־0_WR0NLY����������ԭ�������ڽ���
 * ��־0_TRUNC�������ļ���д����²���Ч��Ȼ��ʹ�õ�ǰ���̵��ļ�������������룬��
 * �ε�����ģʽ�е���Ӧλ����������ͨ�ļ���־I_REGULAR���ñ�־�����ڴ򿪵��ļ�����
 * �ڶ���Ҫ�����ļ�ʱ����Ϊ���ļ���Ĭ�����ԡ��μ�����360���ϵ�ע�͡�	*/
	if ((flag & O_TRUNC) && !(flag & O_ACCMODE))
		flag |= O_WRONLY;
	mode &= 0777 & ~current->umask;
	mode |= I_REGULAR;
/* Ȼ�����ָ����·����Ѱ�ҵ���Ӧ��i�ڵ㣬�Լ����Ŀ¼�����䳤�ȡ���ʱ������
 * Ŀ¼������Ϊ0 (���硯/usr/������·���������)����ô���������Ƕ�д���������ļ���
 * �Ƚ�0�����ʾ���ڴ�һ��Ŀ¼���ļ�����������ֱ�ӷ��ظ�Ŀ¼��i�ڵ㲢����0�˳���
 * ����˵�����̲����Ƿ������ǷŻظ�i�ڵ㣬���س����롣	*/
	if (!(dir = dir_namei (pathname, &namelen, &basename)))
		return -ENOENT;
	if (!namelen)
	{				/* special case: '/usr/' etc */
		if (!(flag & (O_ACCMODE | O_CREAT | O_TRUNC)))
		{
			*res_inode = dir;
			return 0;
		}
		iput (dir);
		return -EISDIR;
	}
/* ���Ÿ�������õ������Ŀ¼����i�ڵ�dir�������в���ȡ��·�����ַ�����������
 * ������Ӧ��Ŀ¼��ṹde����ͬʱ�õ���Ŀ¼�����ڵĸ��ٻ�����ָ�롣����ø��ٻ���
 * ָ��ΪNULL�����ʾû���ҵ���Ӧ�ļ�����Ŀ¼����ֻ�����Ǵ����ļ���������ʱ�� 
 * �����Ǵ����ļ�����Żظ�Ŀ¼��i�ڵ㣬���س�����˳�������û��ڸ�Ŀ¼û��д��Ȩ
 * ������Żظ�Ŀ¼��i�ڵ㣬���س�����˳���	*/
	bh = find_entry (&dir, basename, namelen, &de);
	if (!bh)
	{
		if (!(flag & O_CREAT))
		{
			iput (dir);
			return -ENOENT;
		}
		if (!permission (dir, MAY_WRITE))
		{
			iput (dir);
			return -EACCES;
		}
/* ��������ȷ�����Ǵ�������������д������ɡ�������Ǿ���Ŀ¼i�ڵ��Ӧ�豸������
 * һ���µ�i�ڵ��·������ָ�����ļ�ʹ�á���ʧ����Ż�Ŀ¼��i�ڵ㣬������û�п�
 * ������롣����ʹ�ø���i�ڵ㣬������г�ʼ���ã��ýڵ���û�id;��Ӧ�ڵ����ģ
 * ʽ�������޸ı�־��Ȼ����ָ��Ŀ¼dir�����һ����Ŀ¼�	*/
		inode = new_inode (dir->i_dev);
		if (!inode)
		{
			iput (dir);
			return -ENOSPC;
		}
		inode->i_uid = current->euid;
		inode->i_mode = mode;
		inode->i_dirt = 1;
		bh = add_entry (dir, basename, namelen, &de);
/* ������ص�Ӧ�ú�����Ŀ¼��ĸ��ٻ�����ָ��ΪNULL�����ʾ���Ŀ¼�����ʧ�ܡ�����
 * ������i�ڵ���������Ӽ�����1���Żظ�i�ڵ���Ŀ¼��i�ڵ㲢���س������˳�������
 * ˵�����Ŀ¼������ɹ����������������ø���Ŀ¼���һЩ��ʼֵ����i�ڵ��Ϊ������
 * ����i�ڵ�ĺ��룻���ø��ٻ��������޸ı�־��Ȼ���ͷŸø��ٻ��������Ż�Ŀ¼��i��
 * �㡣������Ŀ¼���i�ڵ�ָ�룬���ɹ��˳���	*/
		if (!bh)
		{
			inode->i_nlinks--;
			iput (inode);
			iput (dir);
			return -ENOSPC;
		}
		de->inode = inode->i_num;
		bh->b_dirt = 1;
		brelse (bh);
		iput (dir);
		*res_inode = inode;
		return 0;
	}
/* �����棨360�У���Ŀ¼��ȡ�ļ�����ӦĿ¼��ṹ�Ĳ����ɹ�����bh��ΪNULL������˵
 * ��ָ���򿪵��ļ��Ѿ����ڡ�����ȡ����Ŀ¼���i�ڵ�ź��������豸�ţ����ͷŸø���
 * �������Լ��Ż�Ŀ¼��i�ڵ㡣�����ʱ��ռ������־0_XCL��λ���������ļ��Ѿ����ڣ�
 * �򷵻��ļ��Ѵ��ڳ������˳���	*/
	inr = de->inode;
	dev = dir->i_dev;
	brelse (bh);
	iput (dir);
	if (flag & O_EXCL)
		return -EEXIST;
/* Ȼ�����Ƕ�ȡ��Ŀ¼���i�ڵ����ݡ�����i�ڵ���һ��Ŀ¼��i�ڵ㲢�ҷ���ģʽ��ֻ
 * д���д������û�з��ʵ����Ȩ�ޣ���Żظ�i�ڵ㣬���ط���Ȩ�޳������˳���	*/
	if (!(inode = iget (dev, inr)))
		return -EACCES;
	if ((S_ISDIR (inode->i_mode) && (flag & O_ACCMODE)) ||
			!permission (inode, ACC_MODE (flag)))
	{
		iput (inode);
		return -EPERM;
	}
/* �������Ǹ��¸�i�ڵ�ķ���ʱ���ֶ�ֵΪ��ǰʱ�䡣��������˽�0��־���򽫸�i��
 * ����ļ����Ƚ�Ϊ0����󷵻ظ�Ŀ¼��i�ڵ��ָ�룬������0(�ɹ�)��	*/
	inode->i_atime = CURRENT_TIME;
	if (flag & O_TRUNC)
		truncate (inode);
	*res_inode = inode;
	return 0;
}

/* ����һ���豸�����ļ�����ͨ�ļ��ڵ㣨node����
 * �ú�����������Ϊfilename����mode��devָ�����ļ�ϵͳ�ڵ㣨��ͨ�ļ����豸������
 * ���������ܵ�����
 * ������filename -·������mode -ָ��ʹ������Լ��������ڵ�����ͣ�dev -�豸�š�
 * ���أ��ɹ��򷵻�0�����򷵻س����롣	*/
int sys_mknod (const char *filename, int mode, int dev)
{
	const char *basename;
	int namelen;
	struct m_inode *dir, *inode;
	struct buffer_head *bh;
	struct dir_entry *de;

/* ���ȼ�������ɺͲ�������Ч�Բ�ȡ·�����ж���Ŀ¼��i�ڵ㡣������ǳ����û�����
 * ���ط�����ɳ����롣����Ҳ�����Ӧ·�����ж���Ŀ¼��i�ڵ㣬�򷵻س����롣�����
 * ���˵��ļ�������Ϊ0����˵��������·�������û��ָ���ļ������Żظ�Ŀ¼i�ڵ㣬��
 * �س������˳�������ڸ�Ŀ¼��û��д��Ȩ�ޣ���Żظ�Ŀ¼��i�ڵ㣬���ط�����ɳ���
 * ���˳���������ǳ����û����򷵻ط�����ɳ����롣	*/
	if (!suser ())
		return -EPERM;
	if (!(dir = dir_namei (filename, &namelen, &basename)))
		return -ENOENT;
	if (!namelen)
	{
		iput (dir);
		return -ENOENT;
	}
	if (!permission (dir, MAY_WRITE))
	{
		iput (dir);
		return -EPERM;
	}
/* Ȼ����������һ��·����ָ�����ļ��Ƿ��Ѿ����ڡ����Ѿ��������ܴ���ͬ���ļ��ڵ㡣
 * �����Ӧ·�����������ļ�����Ŀ¼���Ѿ����ڣ����ͷŰ�����Ŀ¼��Ļ������鲢�Ż�
 * Ŀ¼��i�ڵ㣬�����ļ��Ѿ����ڵĳ������˳���	*/
	bh = find_entry (&dir, basename, namelen, &de);
	if (bh)
	{
		brelse (bh);
		iput (dir);
		return -EEXIST;
	}
/* �������Ǿ�����һ���µ�i�ڵ㣬�����ø�i�ڵ������ģʽ�����Ҫ�������ǿ��豸�ļ�
 * �������ַ��豸�ļ�������i�ڵ��ֱ���߼���ָ��0�����豸�š��������豸�ļ���˵��
 * ��i�ڵ��i_zone[0]�д�ŵ��Ǹ��豸�ļ��������豸���豸�š�Ȼ�����ø�i�ڵ����
 * ��ʱ�䡢����ʱ��Ϊ��ǰʱ�䣬������i�ڵ����޸ı�־��	*/
	inode = new_inode (dir->i_dev);
	if (!inode)
	{
		iput (dir);
		return -ENOSPC;
	}
	inode->i_mode = mode;
	if (S_ISBLK (mode) || S_ISCHR (mode))
		inode->i_zone[0] = dev;
	inode->i_mtime = inode->i_atime = CURRENT_TIME;
	inode->i_dirt = 1;
/* ����Ϊ����µ�i�ڵ���Ŀ¼�������һ��Ŀ¼����ʧ�ܣ�������Ŀ¼��ĸ��ٻ���
 * ��ָ��ΪNULL������ݔ��Ŀ¼��i�ڵ㣻���������i�ڵ��������Ӽ�����λ�����Żظ�
 * i�ڵ㣬���س������˳���	*/
	bh = add_entry (dir, basename, namelen, &de);
	if (!bh)
	{
		iput (dir);
		inode->i_nlinks = 0;
		iput (inode);
		return -ENOSPC;
	}
/* �������Ŀ¼�����Ҳ�ɹ��ˣ������������������Ŀ¼�����ݡ����Ŀ¼���i�ڵ���
 * �ε�����i�ڵ�ţ����ø��ٻ��������޸ı�־���Ż�Ŀ¼���µ�i�ڵ㣬�ͷŸ��ٻ���
 * ������󷵻�0(�ɹ�)��	*/
	de->inode = inode->i_num;
	bh->b_dirt = 1;
	iput (dir);
	iput (inode);
	brelse (bh);
	return 0;
}

/* ϵͳ���ú��� - ����Ŀ¼��
 * ������pathname - ·������
 * mode - Ŀ¼ʹ�õ�Ȩ�����ԡ�
 * ���أ��ɹ��򷵻�0�����򷵻س����롣	*/
int
sys_mkdir (const char *pathname, int mode)
{
	const char *basename;
	int namelen;
	struct m_inode *dir, *inode;
	struct buffer_head *bh, *dir_block;
	struct dir_entry *de;

/* ���ȼ�������ɺͲ�������Ч�Բ�ȡ·�����ж���Ŀ¼��i�ڵ㡣������ǳ����û�����
 * ���ط�����ɳ����롣����Ҳ�����Ӧ·�����ж���Ŀ¼��i�ڵ㣬�򷵻س����롣�����
 * ���˵��ļ�������Ϊ0����˵��������·�������û��ָ���ļ������Żظ�Ŀ¼i�ڵ㣬��
 * �س������˳�������ڸ�Ŀ¼��û��д��Ȩ�ޣ���Żظ�Ŀ¼��i�ڵ㣬���ط�����ɳ���
 * ���˳���������ǳ����û����򷵻ط�����ɳ����롣	*/
	if (!suser ())
		return -EPERM;
	if (!(dir = dir_namei (pathname, &namelen, &basename)))
		return -ENOENT;
	if (!namelen)
	{
		iput (dir);
		return -ENOENT;
	}
	if (!permission (dir, MAY_WRITE))
	{
		iput (dir);
		return -EPERM;
	}
/* Ȼ����������һ��·����ָ����Ŀ¼���Ƿ��Ѿ����ڡ����Ѿ��������ܴ���ͬ��Ŀ¼�ڵ㡣
 * �����Ӧ·����������Ŀ¼����Ŀ¼���Ѿ����ڣ����ͷŰ�����Ŀ¼��Ļ������鲢�Ż�
 * Ŀ¼��i�ڵ㣬�����ļ��Ѿ����ڵĳ������˳����������Ǿ�����һ���µ�i�ڵ㣬������
 * ��i�ڵ������ģʽ���ø���i�ڵ��Ӧ���ļ�����Ϊ32�ֽڣ�2��Ŀ¼��Ĵ�С������
 * �ڵ����޸ı�־���Լ��ڵ���޸�ʱ��ͷ���ʱ�䡣2��Ŀ¼��ֱ�����'.'��'..'Ŀ¼��	*/
	bh = find_entry (&dir, basename, namelen, &de);
	if (bh)
		{
			brelse (bh);
			iput (dir);
			return -EEXIST;
		}
	inode = new_inode (dir->i_dev);
	if (!inode)
		{
			iput (dir);
			return -ENOSPC;
		}
	inode->i_size = 32;
	inode->i_dirt = 1;
	inode->i_mtime = inode->i_atime = CURRENT_TIME;
/* ����Ϊ����i�ڵ�����һ���ڱ���Ŀ¼�����ݵĴ��̿飬���ڱ���Ŀ¼��ṹ��Ϣ������i
 * �ڵ�ĵ�һ��ֱ�ӿ�ָ����ڸÿ�š��������ʧ����Żض�ӦĿ¼��i�ڵ㣻��λ������
 * ��i�ڵ����Ӽ������Żظ��µ�i�ڵ㣬����û�пռ�������˳��������ø��µ�i�ڵ���
 * �޸ı�־��	*/
	if (!(inode->i_zone[0] = new_block (inode->i_dev)))
	{
		iput (dir);
		inode->i_nlinks--;
		iput (inode);
		return -ENOSPC;
	}
	inode->i_dirt = 1;
/* ��������Ĵ��̿顣
 * ���豸�϶�ȡ������Ĵ��̿飨Ŀ���ǰѶ�Ӧ��ŵ����ٻ������У�����������Żض�Ӧ
 * Ŀ¼��i�ڵ㣻�ͷ�����Ĵ��̿飻��λ�������i�ڵ����Ӽ������Żظ��µ�i�ڵ㣬�� 
 * ��û�пռ�������˳���	*/
	if (!(dir_block = bread (inode->i_dev, inode->i_zone[0])))
	{
		iput (dir);
		free_block (inode->i_dev, inode->i_zone[0]);
		inode->i_nlinks--;
		iput (inode);
		return -ERROR;
	}
/* Ȼ�������ڻ�����н�����������Ŀ¼�ļ��е�2��Ĭ�ϵ���Ŀ¼��('.'��'..')�ṹ��
 * �ݡ�������deָ����Ŀ¼������ݿ飬Ȼ���ø�Ŀ¼���i�ڵ���ֶε����������i
 * �ڵ�ţ������ֶε��ڡ�.����Ȼ��deָ����һ��Ŀ¼��ṹ�����ڸýṹ�д���ϼ�Ŀ¼
 * ��i�ڵ�ź�����".."��Ȼ�����øø��ٻ�������޸ı�־�����ͷŸû���顣�ٳ�ʼ��
 * ������i�ڵ��ģʽ�ֶΣ����ø�i�ڵ����޸ı�־��	*/
	de = (struct dir_entry *) dir_block->b_data;
	de->inode = inode->i_num;				/* ����'.'Ŀ¼�	*/
	strcpy (de->name, ".");
	de++;
	de->inode = dir->i_num;					/* ����'..'Ŀ¼�	*/
	strcpy (de->name, "..");
	inode->i_nlinks = 2;
	dir_block->b_dirt = 1;
	brelse (dir_block);
	inode->i_mode = I_DIRECTORY | (mode & 0777 & ~current->umask);
	inode->i_dirt = 1;
/* ����������ָ��Ŀ¼�������һ��Ŀ¼����ڴ���½�Ŀ¼��i�ڵ�ź�Ŀ¼�������
 * ʧ�ܣ�������Ŀ¼��ĸ��ٻ�����ָ��ΪNULL������Ż�Ŀ¼��i�ڵ㣻�������i�ڵ�
 * �������Ӽ�����λ�����Żظ�i�ڵ㡣���س������˳���	*/
	bh = add_entry (dir, basename, namelen, &de);
	if (!bh)
	{
		iput (dir);
		free_block (inode->i_dev, inode->i_zone[0]);
		inode->i_nlinks = 0;
		iput (inode);
		return -ENOSPC;
	}
/* ��������Ŀ¼���i�ڵ��ֶε�����i�ڵ�ţ����ø��ٻ�������޸ı�־���Ż�Ŀ¼
 * ���µ�i�ڵ㣬�ͷŸ��ٻ���飬��󷵻�0 (�ɹ�)��	*/
	de->inode = inode->i_num;
	bh->b_dirt = 1;
	dir->i_nlinks++;
	dir->i_dirt = 1;
	iput (dir);
	iput (inode);
	brelse (bh);
	return 0;
}

/*
* routine to check that the specified directory is empty (for rmdir)
*/
/*
* ���ڼ��ָ����Ŀ¼�Ƿ�Ϊ�յ��ӳ���(����rmdir ϵͳ���ú���)��
*/

/* ���ָ��Ŀ¼�Ƿ��ǿյġ�
 * ������inode - ָ��Ŀ¼��i �ڵ�ָ�롣
 * ���أ�0 - Ŀ¼���ǿյģ�	1 - ���ա�	*/
static int
empty_dir (struct m_inode *inode)
{
	int nr, block;
	int len;
	struct buffer_head *bh;
	struct dir_entry *de;

/* ���ȼ���ָ��Ŀ¼������Ŀ¼���������鿪ʼ�����ض�Ŀ¼������Ϣ�Ƿ���ȷ��һ��Ŀ¼
 * ��Ӧ��������2��Ŀ¼�����.���͡�..���� ���Ŀ¼���������2�����߸�Ŀ¼i�ڵ�ĵ�
 * 1 ��ֱ�ӿ�û��ָ���κδ��̿�ţ����߸�ֱ�ӿ������������ʾ������Ϣ"�豸dev��Ŀ
 * ¼��"������ 0(ʧ��)��	*/
	len = inode->i_size / sizeof (struct dir_entry);	/* Ŀ¼��Ŀ¼�������	*/
	if (len < 2 || !inode->i_zone[0] ||
			!(bh = bread (inode->i_dev, inode->i_zone[0])))
	{
		printk ("warning - bad directory on dev %04x\n", inode->i_dev);
		return 0;
	}
/* ��ʱ bh ��ָ������к���Ŀ¼�����ݡ�������Ŀ¼��ָ�� de ָ�򻺳���е� 1 ��Ŀ¼�
 * ���ڵ� 1 ��Ŀ¼���.���������� i �ڵ���ֶ� LQRGH Ӧ�õ��ڵ�ǰĿ¼�� i �ڵ�š�����
 * �� 2 ��Ŀ¼���..���������� i �ڵ���ֶ� LQRGH Ӧ�õ�����һ��Ŀ¼�� i �ڵ�ţ�����
 * Ϊ 0���������� 1 ��Ŀ¼��� i �ڵ���ֶ�ֵ�����ڸ�Ŀ¼�� i �ڵ�ţ����ߵ� 2 ��Ŀ¼
 * ���  i  �ڵ���ֶ�Ϊ�㣬��������Ŀ¼��������ֶβ��ֱ���ڡ�.���͡�..��������ʾ����
 * ����Ϣ"�豸 dev ��Ŀ¼��"�������� 0��	*/
	de = (struct dir_entry *) bh->b_data;
	if (de[0].inode != inode->i_num || !de[1].inode ||
			strcmp (".", de[0].name) || strcmp ("..", de[1].name))
	{
		printk ("warning - bad directory on dev %04x\n", inode->i_dev);
		return 0;
	}
/* Ȼ��������nr����Ŀ¼����ţ���0��ʼ�ƣ���deָ�������Ŀ¼���ѭ������Ŀ¼
 * ���������еģ�len - 2����Ŀ¼�����û��Ŀ¼���i�ڵ���ֶβ�Ϊ0 (��ʹ��)��	*/
	nr = 2;
	de += 2;
	while (nr < len)
	{
/* ����ÿ���̿��е�Ŀ¼���Ѿ�ȫ�������ϣ����ͷŸô��̿�Ļ���飬����ȡĿ¼����
 * �ļ�����һ�麬��Ŀ¼��Ĵ��̿顣��ȡ�ķ����Ǹ��ݵ�ǰ����Ŀ¼�����nr�������
 * ӦĿ¼����Ŀ¼�����ļ��е����ݿ�ţ�nr/DIR_ENTRIES_PER_BLOCK����Ȼ��ʹ��bmap()
 * ����ȡ�ö�Ӧ���̿��block����ʹ�ö��豸�̿麯��bread()����Ӧ�̿���뻺����У�
 * �����ظû�����ָ�롣������ȡ����Ӧ�̿�û��ʹ�ã����Ѿ����ã����ļ��Ѿ�ɾ���ȣ���
 * ���������һ�飬�����������������0��������deָ���������׸�Ŀ¼�	*/
		if ((void *) de >= (void *) (bh->b_data + BLOCK_SIZE))
		{
			brelse (bh);
			block = bmap (inode, nr / DIR_ENTRIES_PER_BLOCK);
			if (!block)
				{
					nr += DIR_ENTRIES_PER_BLOCK;
					continue;
				}
			if (!(bh = bread (inode->i_dev, block)))
				return 0;
			de = (struct dir_entry *) bh->b_data;
		}
/* ����deָ��ĵ�ǰĿ¼������Ŀ¼���i�ڵ���ֶβ�����0�����ʾ��Ŀ¼��Ŀǰ��
 * ��ʹ�ã����ͷŸø��ٻ�����������0�˳�����������û�в�ѯ���Ŀ¼�е�����Ŀ¼�
 * ���Ŀ¼�����nr��l��deָ����һ��Ŀ¼�������⡣	*/
		if (de->inode)
		{
			brelse (bh);
			return 0;
		}
		de++;
		nr++;
	}
/* ִ�е�����˵����Ŀ¼��û���ҵ����õ�Ŀ¼���Ȼ����ͷ�������⣩�����ͷŻ���鷵��1��	*/
	brelse (bh);
	return 1;
}

/* ϵͳ���ú��� - ɾ��ָ�����Ƶ�Ŀ¼��	*/
/* ������ name - Ŀ¼��(·����)��	*/
/* ���أ�����0 ��ʾ�ɹ������򷵻س���š�	*/
int
sys_rmdir (const char *name)
{
	const char *basename;
	int namelen;
	struct m_inode *dir, *inode;
	struct buffer_head *bh;
	struct dir_entry *de;

/* ���ȼ�������ɺͲ�������Ч�Բ�ȡ·�����ж���Ŀ¼��i�ڵ㡣������ǳ����û�����
 * ���ط�����ɳ����롣����Ҳ�����Ӧ·�����ж���Ŀ¼��i�ڵ㣬�򷵻س����롣�����
 * ���˵��ļ�������Ϊ0����˵��������·�������û��ָ��Ŀ¼�����Żظ�Ŀ¼i�ڵ㣬��
 * �س������˳�������ڸ�Ŀ¼��û��д��Ȩ�ޣ���Żظ�Ŀ¼��i�ڵ㣬���ط�����ɳ���
 * ���˳���������ǳ����û����򷵻ط�����ɳ����롣	*/
	if (!suser ())
		return -EPERM;
	if (!(dir = dir_namei (name, &namelen, &basename)))
		return -ENOENT;
	if (!namelen)
	{
		iput (dir);
		return -ENOENT;
	}
	if (!permission (dir, MAY_WRITE))
	{
		iput (dir);
		return -EPERM;
	}
/* Ȼ�����ָ��Ŀ¼��i�ڵ��Ŀ¼�����ú���find��entryOѰ�Ҷ�ӦĿ¼������ذ�����
 * Ŀ¼��Ļ����ָ��bh��������Ŀ¼���Ŀ¼��i�ڵ�ָ��dir�͸�Ŀ¼��ָ��de���ٸ���
 * ��Ŀ¼��de�е�i�ڵ������iget()�����õ���Ӧ��i�ڵ�inode�������Ӧ·��������
 * ��Ŀ¼����Ŀ¼����ڣ����ͷŰ�����Ŀ¼��ĸ��ٻ��������Ż�Ŀ¼��i�ڵ㣬������
 * ���Ѿ����ڳ����룬���˳������ȡĿ¼���i�ڵ������Ż�Ŀ¼��i�ڵ㣬���ͷź�
 * ��Ŀ¼��ĸ��ٻ����������س���š�	*/
	bh = find_entry (&dir, basename, namelen, &de);
	if (!bh)
	{
		iput (dir);
		return -ENOENT;
	}
	if (!(inode = iget (dir->i_dev, de->inode)))
	{
		iput (dir);
		brelse (bh);
		return -EPERM;
	}
/* ��ʱ�������а���Ҫ��ɾ��Ŀ¼���Ŀ¼i�ڵ�dir��Ҫ��ɾ��Ŀ¼���i�ڵ�inode��Ҫ
 * ��ɾ��Ŀ¼��ָ��de����������ͨ������3����������Ϣ�ļ������֤ɾ�������Ŀ����ԡ�
 * ����Ŀ¼����������ɾ����־���ҽ��̵���Ч�û�id(euid)����root�����ҽ��̵���Ч
 * �û�id(euid)�����ڸ�i�ڵ���û�id�����ʾ��ǰ����û��Ȩ��ɾ����Ŀ¼�����Ƿ�
 * �ذ���Ҫɾ��Ŀ¼����Ŀ¼i�ڵ�͸�Ҫɾ��Ŀ¼��i�ڵ㣬Ȼ���ͷŸ��ٻ�����������
 * �����롣	*/
	if ((dir->i_mode & S_ISVTX) && current->euid &&
			inode->i_uid != current->euid)
	{
		iput (dir);
		iput (inode);
		brelse (bh);
		return -EPERM;
	}
/* ���Ҫ��ɾ����Ŀ¼���i �ڵ���豸�Ų����ڰ�����Ŀ¼���Ŀ¼���豸�ţ����߸ñ�ɾ��Ŀ¼��
 * �������Ӽ�������1(��ʾ�з������ӵ�)������ɾ����Ŀ¼�������ͷŰ���Ҫɾ��Ŀ¼����Ŀ¼
 * i �ڵ�͸�Ҫɾ��Ŀ¼��i �ڵ㣬�ͷŸ��ٻ����������س����롣	*/
	if (inode->i_dev != dir->i_dev || inode->i_count > 1)
	{
		iput (dir);
		iput (inode);
		brelse (bh);
		return -EPERM;
	}
/* ���Ҫ��ɾ��Ŀ¼��Ŀ¼��i �ڵ�Ľڵ�ŵ��ڰ�������ɾ��Ŀ¼��i �ڵ�ţ����ʾ��ͼɾ��"."
 * Ŀ¼�������ͷŰ���Ҫɾ��Ŀ¼����Ŀ¼i �ڵ�͸�Ҫɾ��Ŀ¼��i �ڵ㣬�ͷŸ��ٻ�����������
 * �����롣	*/
	if (inode == dir)
	{				/* we may not delete ".", but "../dir" is ok */
		iput (inode);		/* ���ǲ�����ɾ��"."��������ɾ��"../dir" */
		iput (dir);
		brelse (bh);
		return -EPERM;
	}
/* ��Ҫ��ɾ��Ŀ¼i�ڵ�����Ա����ⲻ��һ��Ŀ¼����ɾ��������ǰ����ȫ�����ڡ�����
 * �Żذ���ɾ��Ŀ¼����Ŀ¼i�ڵ�͸�Ҫɾ��Ŀ¼��i�ڵ㣬�ͷŸ��ٻ���飬���س����롣	*/
	if (!S_ISDIR (inode->i_mode))
	{
		iput (inode);
		iput (dir);
		brelse (bh);
		return -ENOTDIR;
	}
/* �����豻ɾ����Ŀ¼���գ���Ҳ����ɾ�������ǷŻذ���Ҫɾ��Ŀ¼����Ŀ¼i�ڵ�͸�Ҫ
 * ɾ��Ŀ¼��i�ڵ㣬�ͷŸ��ٻ���飬���س����롣	*/
	if (!empty_dir (inode))
	{
		iput (inode);
		iput (dir);
		brelse (bh);
		return -ENOTEMPTY;
	}
/* ����һ����Ŀ¼����Ŀ¼��������Ӧ��Ϊ2 (���ӵ��ϲ�Ŀ¼�ͱ�Ŀ¼)�������豻ɾ��Ŀ
 * ¼��i�ڵ��������������2������ʾ������Ϣ����ɾ��������Ȼ����ִ�С������ø��豻
 * ɾ��Ŀ¼��Ŀ¼���i�ڵ���ֶ�Ϊ0����ʾ��Ŀ¼���ʹ�ã����ú��и�Ŀ¼��ĸ���
 * ��������޸ı�־�����ͷŸû���顣Ȼ�����ñ�ɾ��Ŀ¼i�ڵ��������Ϊ0(��ʾ����)��
 * ����i�ڵ����޸ı�־��	*/
	if (inode->i_nlinks != 2)
		printk ("empty directory has nlink!=2 (%d)", inode->i_nlinks);
	de->inode = 0;
	bh->b_dirt = 1;
	brelse (bh);
	inode->i_nlinks = 0;
	inode->i_dirt = 1;
/* �ٽ�������ɾ��Ŀ¼����Ŀ¼��i�ڵ����Ӽ�����1���޸���ı�ʱ����޸�ʱ��Ϊ��ǰʱ
 * �䣬���øýڵ����޸ı�־�����Żذ���Ҫɾ��Ŀ¼����Ŀ¼i�ڵ�͸�Ҫɾ��Ŀ¼��i
 * �ڵ㣬����0(ɾ�������ɹ�)	*/
	dir->i_nlinks--;
	dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	dir->i_dirt = 1;
	iput (dir);
	iput (inode);
	return 0;
}

/* ϵͳ���ú��� - ɾ���ļ����Լ�����Ҳɾ������ص��ļ���	*/
/* ���ļ�ϵͳɾ��һ�����֡������һ���ļ������һ�����ӣ�����û�н������򿪸��ļ�������ļ�
 * Ҳ����ɾ�������ͷ���ռ�õ��豸�ռ䡣
 * ������name - �ļ�����
 * ���أ��ɹ��򷵻�0�����򷵻س���š�	*/
int
sys_unlink (const char *name)
{
	const char *basename;
	int namelen;
	struct m_inode *dir, *inode;
	struct buffer_head *bh;
	struct dir_entry *de;

/* ���ȼ���������Ч�Բ�ȡ·�����ж���Ŀ¼��i�ڵ㡣����Ҳ�����Ӧ·�����ж���Ŀ¼
 * ��i�ڵ㣬�򷵻س����롣�����˵��ļ�������Ϊ0����˵��������·�������û��ָ
 * ���ļ������Żظ�Ŀ¼i�ڵ㣬���س������˳�������ڸ�Ŀ¼��û��д��Ȩ�ޣ���Żظ�
 * Ŀ¼��i�ڵ㣬���ط�����ɳ������˳�������Ҳ�����Ӧ·��������Ŀ¼��i�ڵ㣬��
 * �س����롣	*/
	if (!(dir = dir_namei (name, &namelen, &basename)))
		return -ENOENT;
	if (!namelen)
	{
		iput (dir);
		return -ENOENT;
	}
	if (!permission (dir, MAY_WRITE))
	{
		iput (dir);
		return -EPERM;
	}
/* Ȼ�����ָ��Ŀ¼��i�ڵ��Ŀ¼�����ú���find_entryOѰ�Ҷ�ӦĿ¼������ذ�����
 * Ŀ¼��Ļ����ָ��bh��������Ŀ¼���Ŀ¼��i�ڵ�ָ��dir�͸�Ŀ¼��ָ��de���ٸ���
 * ��Ŀ¼��de�е�i�ڵ������iget()�����õ���Ӧ��i�ڵ�inode�������Ӧ·��������
 * ��Ŀ¼����Ŀ¼����ڣ����ͷŰ�����Ŀ¼��ĸ��ٻ��������Ż�Ŀ¼��i�ڵ㣬������
 * ���Ѿ����ڳ����룬���˳������ȡĿ¼���i�ڵ������Ż�Ŀ¼��i�ڵ㣬���ͷź�
 * ��Ŀ¼��ĸ��ٻ����������س���š�	*/
	bh = find_entry (&dir, basename, namelen, &de);
	if (!bh)
	{
		iput (dir);
		return -ENOENT;
	}
	if (!(inode = iget (dir->i_dev, de->inode)))
	{
		iput (dir);
		brelse (bh);
		return -ENOENT;
	}
/* ��ʱ�������а���Ҫ��ɾ��Ŀ¼���Ŀ¼i�ڵ�dir��Ҫ��ɾ��Ŀ¼���i�ڵ�inode��Ҫ
 * ��ɾ��Ŀ¼��ָ��de����������ͨ������3����������Ϣ�ļ������֤ɾ�������Ŀ����ԡ�
 * ����Ŀ¼����������ɾ����־���ҽ��̵���Ч�û�id(euid)����root�����ҽ��̵�euid
 * �����ڸ�i�ڵ���û�id�����ҽ��̵�euidҲ������Ŀ¼i�ڵ���û�id�����ʾ��ǰ��
 * ��û��Ȩ��ɾ����Ŀ¼�����ǷŻذ���Ҫɾ��Ŀ¼����Ŀ¼i�ڵ�͸�Ҫɾ��Ŀ¼��i�ڵ㣬
 * Ȼ���ͷŸ��ٻ���飬���س����롣	*/
	if ((dir->i_mode & S_ISVTX) && !suser () &&
			current->euid != inode->i_uid && current->euid != dir->i_uid)
	{
		iput (dir);
		iput (inode);
		brelse (bh);
		return -EPERM;
	}
/* �����ָ���ļ�����һ��Ŀ¼����Ҳ����ɾ�����ͷŸ�Ŀ¼i �ڵ�͸��ļ���Ŀ¼���i �ڵ㣬�ͷ�
 * ������Ŀ¼��Ļ����������س���š�	*/
	if (S_ISDIR (inode->i_mode))
	{
		iput (inode);
		iput (dir);
		brelse (bh);
		return -EPERM;
	}
/* �����i�ڵ�����Ӽ���ֵ�Ѿ�Ϊ0������ʾ������Ϣ����������Ϊ1��	*/
	if (!inode->i_nlinks)
	{
		printk ("Deleting nonexistent file (%04x:%d), %d\n",
			inode->i_dev, inode->i_num, inode->i_nlinks);
		inode->i_nlinks = 1;
	}
/* �������ǿ���ɾ���ļ�����Ӧ��Ŀ¼���ˡ����ǽ����ļ���Ŀ¼���е�i�ڵ���ֶ���Ϊ0��
 * ��ʾ�ͷŸ�Ŀ¼������ð�����Ŀ¼��Ļ�������޸ı�־���ͷŸø��ٻ���顣	*/
	de->inode = 0;
	bh->b_dirt = 1;
	brelse (bh);
/* Ȼ����ļ�����Ӧi�ڵ����������1�������޸ı�־�����¸ı�ʱ��Ϊ��ǰʱ�䡣����
 * �ظ�i�ڵ��Ŀ¼��i�ڵ㣬����0(�ɹ�)��������ļ������һ�����ӣ���i�ڵ�����
 * ����1�����0�����Ҵ�ʱû�н������򿪸��ļ�����ô�ڵ���iput()�Ż�i�ڵ�ʱ������
 * ��Ҳ����ɾ�������ͷ���ռ�õ��豸�ռ䡣�μ�fs/inode.c����180�С�	*/
	inode->i_nlinks--;
	inode->i_dirt = 1;
	inode->i_ctime = CURRENT_TIME;
	iput (inode);
	iput (dir);
	return 0;
}

int sys_symlink()
{
	return -ENOSYS;
}

/* ϵͳ���ú��� - Ϊ�ļ�����һ���ļ�����	*/
/* Ϊһ���Ѿ����ڵ��ļ�����һ��������(Ҳ��ΪӲ���� - hard link)��	*/
/* ������oldname - ԭ·������ * newname - �µ�·������
 * ���أ����ɹ��򷵻�0�����򷵻س���š�	*/
int
sys_link (const char *oldname, const char *newname)
{
	struct dir_entry *de;
	struct m_inode *oldinode, *dir;
	struct buffer_head *bh;
	const char *basename;
	int namelen;

/* ���ȶ�ԭ�ļ���������Ч����֤����Ӧ�ô��ڲ��Ҳ���һ��Ŀ¼��������������ȡԭ�ļ�·
 * ������Ӧ��i�ڵ�oldinode�����Ϊ0�����ʾ�������س���š����ԭ·������Ӧ����
 * һ��Ŀ¼������Żظ�i�ڵ㣬Ҳ���س���š�	*/
	oldinode = namei (oldname);
	if (!oldinode)
		return -ENOENT;
	if (S_ISDIR (oldinode->i_mode))
	{
		iput (oldinode);
		return -EPERM;
	}
/* Ȼ�������·���������Ŀ¼��i�ڵ�dir�������������ļ������䳤�ȡ����Ŀ¼��
 * i�ڵ�û���ҵ�����Ż�ԭ·������i�ڵ㣬���س���š������·�����в������ļ�����
 * ��Ż�ԭ·����i�ڵ����·����Ŀ¼��i�ڵ㣬���س���š�	*/
	dir = dir_namei (newname, &namelen, &basename);
	if (!dir)
	{
		iput (oldinode);
		return -EACCES;
	}
	if (!namelen)
	{
		iput (oldinode);
		iput (dir);
		return -EPERM;
	}
/* ���ǲ��ܿ��豸����Ӳ���ӡ���������·��������Ŀ¼���豸����ԭ·�������豸��
 * ��һ������Ż���·����Ŀ¼��i�ڵ��ԭ·������i�ڵ㣬���س���š����⣬����û�
 * û������Ŀ¼��д��Ȩ�ޣ���Ҳ���ܽ������ӣ����ǷŻ���·����Ŀ¼��i�ڵ��ԭ·����
 * ��i�ڵ㣬���س���š�	*/
	if (dir->i_dev != oldinode->i_dev)
	{
		iput (dir);
		iput (oldinode);
		return -EXDEV;
	}
	if (!permission (dir, MAY_WRITE))
	{
		iput (dir);
		iput (oldinode);
		return -EACCES;
	}
/* ���ڲ�ѯ����·�����Ƿ��Ѿ����ڣ����������Ҳ���ܽ������ӡ������ͷŰ������Ѵ���Ŀ
 * ¼��ĸ��ٻ���飬�Ż���·����Ŀ¼��i�ڵ��ԭ·������i�ڵ㣬���س���š�	*/
	bh = find_entry (&dir, basename, namelen, &de);
	if (bh)
	{
		brelse (bh);
		iput (dir);
		iput (oldinode);
		return -EEXIST;
	}
/* �������������������ˣ�������������Ŀ¼�����һ��Ŀ¼���ʧ����Żظ�Ŀ¼��i��
 * ���ԭ·������i�ڵ㣬���س���š������ʼ���ø�Ŀ¼���i�ڵ�ŵ���ԭ·������i
 * �ڵ�ţ����ð���������Ŀ¼��Ļ�������޸ı�־���ͷŸû���飬�Ż�Ŀ¼��i�ڵ㡣	*/
	bh = add_entry (dir, basename, namelen, &de);
	if (!bh)
	{
		iput (dir);
		iput (oldinode);
		return -ENOSPC;
	}
	de->inode = oldinode->i_num;
	bh->b_dirt = 1;
	brelse (bh);
	iput (dir);
/* �ٽ�ԭ�ڵ�����Ӽ�����1���޸���ı�ʱ��Ϊ��ǰʱ�䣬������i�ڵ����޸ı�־�����
 * �Ż�ԭ·������i�ڵ㣬������0(�ɹ�)��	*/
	oldinode->i_nlinks++;
	oldinode->i_ctime = CURRENT_TIME;
	oldinode->i_dirt = 1;
	iput (oldinode);
	return 0;
}
