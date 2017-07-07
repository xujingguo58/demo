/*
* linux/kernel/blk_drv/ramdisk.c
*
* Written by Theodore Ts'o, 12/2/91
*/
/* ��Theodore Ts'o ���ƣ�12/2/91
*/
/* Theodore Ts��o (Ted Ts��o)��Linux�����е��������Linux�����緶Χ�ڵ�����Ҳ������
��Ĺ��͡�����Linux����ϵͳ������ʱ�����ͻ��ż��������ΪLinux�ķ�չ�ṩ�˵����ʼ���
�����maillist�����ڱ����������������� Linux��ftp������վ�㣨tsx-ll.mit.edu����
����������Ϊ���Linux�û��ṩ��������Linux�����������֮һ�������ʵ���� ext2��
��ϵͳ�����ļ�ϵͳ�ѳ�ΪLinux��������ʵ�ϵ��ļ�ϵͳ��׼����������Ƴ��� ext3�ļ�ϵͳ��
�������Ĺ��������������й�LSB (Linux Standard Base)�ȷ���Ĺ����������ĸ�����ҳ�ǣ�
http://thunk.org/tytso/��
*/

#include <string.h>			/* �ַ���ͷ�ļ�����Ҫ������һЩ�й��ַ���������Ƕ�뺯����	*/
#include <linux/config.h>	/* �ں�����ͷ�ļ�������������Ժ�Ӳ�����ͣ�HD_TYPE����ѡ�	*/
#include <linux/sched.h>	/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 �����ݣ�	*/
							/* ����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/
#include <linux/fs.h>		/* �ļ�ϵͳͷ�ļ��������ļ���ṹ��file,buffer_head,m_inode �ȣ���	*/
#include <linux/kernel.h>	/* �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣	*/
#include <asm/system.h>		/* ϵͳͷ�ļ������������û��޸�������/�ж��ŵȵ�Ƕ��ʽ���ꡣ	*/
#include <asm/segment.h>	/* �β���ͷ�ļ����������йضμĴ���������Ƕ��ʽ��ຯ����	*/
#include <asm/memory.h>		/* �ڴ濽��ͷ�ļ�������memcpy()Ƕ��ʽ���꺯����	*/

/* ����RAM�����豸�ŷ��ų��������������������豸�ű����ڰ���blk.h�ļ�֮ǰ�����塣
 * ��Ϊblk.h�ļ���Ҫ�õ�������ų���ֵ��ȷ��һЩ�е������������źͺꡣ
 */
#define MAJOR_NR 1			/* �ڴ����豸����1��	*/
#include "blk.h"

/* ���������ڴ��е���ʼλ�á���λ�û��ڵ�52���ϳ�ʼ������rd��init()��ȷ�����μ��ں�
��ʼ������init/main.c����124�С���rd���ǡ�ramdisk������д��
*/
char *rd_start;				/* ���������ڴ��е���ʼλ�á���52 �г�ʼ������rd_init()��	*/
							/* ȷ�����μ�(init/main.c,124)����дrd_����ramdisk_����	*/
int rd_length = 0;			/* ��������ռ�ڴ��С���ֽڣ���	*/

/* �����̵�ǰ���������������
�ú����ĳ���ṹ��Ӳ�̵�do��hd��requestO�������ƣ��μ�hd.c��294�С��ڵͼ����豸
�ӿں���ll��rW��block()�����������̣�rd�����������ӵ�rd��������֮�󣬾ͻ��
�øú�����rd��ǰ��������д����ú������ȼ��㵱ǰ��������ָ������ʼ������Ӧ��
���������ڴ����ʼλ��addr��Ҫ�����������Ӧ���ֽڳ���ֵlen��Ȼ�������������
��������в���������д����TOITE���Ͱ���������ָ�������е�����ֱ�Ӹ��Ƶ��ڴ�λ��
addr�������Ƕ�������֮�����ݸ�����ɺ󼴿�ֱ�ӵ���end��requestO�Ա���������
����������Ȼ����ת��������ʼ����ȥ������һ�����������û�����������˳���
*/
void
do_rd_request (void)
{
	int len;
	char *addr;

/* ���ȼ��������ĺϷ��ԣ�����û�����������˳����μ�blk.h����127�У���Ȼ�������
����������������ʼ�����������ڴ��ж�Ӧ�ĵ�ַaddr��ռ�õ��ڴ��ֽڳ���ֵlen��
�¾�����ȡ���������е���ʼ������Ӧ���ڴ���ʼλ�ú��ڴ泤�ȡ�����sector << 9��ʾ
sector * 512��������ֽ�ֵ��CURRENT ������Ϊ��blk_dev[MAJOR��NR].current��request����
*/

	INIT_REQUEST;				/* �������ĺϷ���(�μ�kernel/blk_drv/blk.h,127)��	*/
/* �������ȡ��ramdisk ����ʼ������Ӧ���ڴ���ʼλ�ú��ڴ泤�ȡ�	*/
/* ����sector << 9 ��ʾsector * 512��CURRENT ����Ϊ(blk_dev[MAJOR_NR].current_request)��	*/
	addr = rd_start + (CURRENT->sector << 9);
	len = CURRENT->nr_sectors << 9;
/* �����ǰ�����������豸�Ų�Ϊ1���߶�Ӧ�ڴ���ʼλ�ô���������ĩβ��������������
����ת��repeat��ȥ������һ����������������repeat�����ں�INIT��REQUEST�ڣ�
λ�ں�Ŀ�ʼ�����μ�blk.h�ļ���127�С�	*/
	if ((MINOR (CURRENT->dev) != 1) || (addr + len > rd_start + rd_length))
		{
			end_request (0);
			goto repeat;
		}
/* Ȼ�����ʵ�ʵĶ�д�����������д���WRITE�������������л����������ݸ��Ƶ���ַ
addr��������Ϊlen�ֽڡ�����Ƕ����READ������addr��ʼ���ڴ����ݸ��Ƶ�������
�������У�����Ϊlen�ֽڡ�������ʾ������ڣ�������
*/
	if (CURRENT->cmd == WRITE)
		{
			(void) memcpy (addr, CURRENT->buffer, len);
/* ����Ƕ�����(READ)����addr ��ʼ�����ݸ��Ƶ��������л������У�����Ϊlen �ֽڡ�	*/
		}
	else if (CURRENT->cmd == READ)
		{
			(void) memcpy (CURRENT->buffer, addr, len);
/* ������ʾ������ڣ�������	*/
		}
	else
		panic ("unknown ramdisk-command");
/* Ȼ����������ɹ������ø��±�־�������������豸����һ�����	*/
	end_request (1);
	goto repeat;
}

/*
* Returns amount of memory which needs to be reserved.
*/
/* �����ڴ�������ramdisk������ڴ���
�����̳�ʼ��������
�ú������������������豸�����������ָ��ָ��do��rd��requestO��Ȼ��ȷ��������
�������ڴ��е���ʼ��ַ��ռ���ֽڳ���ֵ���������������������㡣��󷵻��������ȡ�
��linux/Makefile�ļ������ù�RAMDISKֵ��Ϊ��ʱ����ʾϵͳ�лᴴ��RAM�������豸��
����������µ��ں˳�ʼ�������У��������ͻᱻ���ã�init/main.c��L124�У����ú���
�ĵ�2������length�ᱻ��ֵ��RAMDISK * 1024����λΪ�ֽڡ�
*/
long
rd_init (long mem_start, int length)
{
	int i;
	char *cp;

	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;/* do_rd_request()��	*/
	rd_start = (char *) mem_start;				/* ����16MBϵͳ��ֵΪ 4MB��	*/
	rd_length = length;							/* ���������򳤶�ֵ��	*/
	cp = rd_start;
	for (i = 0; i < length; i++)
		*cp++ = '\0';
	return (length);
}

/*
* If the root device is the ram disk, try to load it.
* In order to do this, the root device is originally set to the
* floppy, and we later change it to be ram disk.
*/
/*
* ������ļ�ϵͳ�豸(root device)��ramdisk �Ļ������Լ�������root device ԭ����ָ��
* ���̵ģ����ǽ����ĳ�ָ��ramdisk��
*/
/* ���԰Ѹ��ļ�ϵͳ���ص��������С�
�ú��������ں����ú���Setup()(hd.c��156��)�б����á����⣬1���̿�=1024�ֽڡ�
��75���ϵı���block=256��ʾ���ļ�ϵͳӳ���ļ����洢��boot�̵�256���̿鿪ʼ����
*/
void
rd_load (void)
{
	struct buffer_head *bh;		/* ���ٻ����ͷָ�롣	*/
	struct super_block s;			/* �ļ�������ṹ��	*/
	int block = 256;				/* Start at block 256 */
	int i = 1;
	int nblocks;					/* �ļ�ϵͳ�̿�������	*/
	char *cp;						/* Move pointer */

/* ���ȼ�������̵���Ч�Ժ������ԡ����ramdisk�ĳ���Ϊ�㣬���˳���������ʾramdisk
�Ĵ�С�Լ��ڴ���ʼλ�á������ʱ���ļ��豸���������豸����Ҳ�˳���
*/
	if (!rd_length)				/* ���ramdisk �ĳ���Ϊ�㣬���˳���	*/
		return;
	printk ("Ram disk: %d bytes, starting at 0x%x\n", rd_length, (int) rd_start);	/* ��ʾramdisk �Ĵ�С�Լ��ڴ���ʼλ�á�	*/
	if (MAJOR (ROOT_DEV) != 2)	/* �����ʱ���ļ��豸�������̣����˳���	*/
		return;
/* Ȼ������ļ�ϵͳ�Ļ����������������̿�256+1��256��256+2������block+1��ָ������
�ĳ����顣breadaO���ڶ�ȡָ�������ݿ飬���������Ҫ���Ŀ飬Ȼ�󷵻غ������ݿ��
������ָ�롣�������NULL�����ʾ���ݿ鲻�ɶ���fs/buffer.c��322����Ȼ��ѻ�������
�Ĵ��̳����飨d��super��block�Ǵ��̳�����ṹ�����Ƶ�s�����У����ͷŻ�����������
���ǿ�ʼ�Գ��������Ч�Խ����жϡ�������������ļ�ϵͳħ�����ԣ���˵�����ص�����
�鲻��MINIX�ļ�ϵͳ�������˳����й�MINIX������Ľṹ��μ��ļ�ϵͳһ�����ݡ�
*/
	bh = breada (ROOT_DEV, block + 1, block, block + 2, -1);
	if (!bh)
		{
			printk ("Disk error while looking for ramdisk!\n");
			return;
		}
/* ��s ָ�򻺳����еĴ��̳����顣(d_super_block �����г�����ṹ)��	*/
	*((struct d_super_block *) &s) = *((struct d_super_block *) bh->b_data);
	brelse (bh);					/* [Ϊʲô����û�и��ƾ������ͷ��أ�]	*/
	if (s.s_magic != SUPER_MAGIC)	/* No ram disk image present, assume normal floppy boot */
									/*������û��ramdiskӳ���ļ����˳�ȥִ��ͨ������������ȴ	*/

/* Ȼ��������ͼ���������ļ�ϵͳ���뵽�ڴ����������С�����һ���ļ�ϵͳ��˵���䳬����
�ṹ��s_nzones�ֶ��б��������߼����������Ϊ����������һ���߼����к��е����ݿ�
�������ֶ�s_log_zone_sizeָ��������ļ�ϵͳ�е����ݿ�����nblocks�͵��ڣ��߼���
�� * 2^ (ÿ���ο����Ĵη�)������ nblocks = (s_nzones * 2~s_log_zone_ize)���������
�ļ�ϵͳ�����ݿ����������ڴ���������������&���������������ִ�м��ز�������ֻ
����ʾ������Ϣ�����ء�
*/
		return;
	nblocks = s.s_nzones << s.s_log_zone_size;
	if (nblocks > (rd_length >> BLOCK_SIZE_BITS))
		{
			printk ("Ram disk image too big! (%d blocks, %d avail)\n",
				nblocks, rd_length >> BLOCK_SIZE_BITS);
			return;
		}
/* �����������������ɵ����ļ�ϵͳ�����ݿ�������������ʾ�������ݿ���Ϣ������cpָ��
�ڴ���������ʼ����Ȼ��ʼִ��ѭ�������������ϸ��ļ�ϵͳӳ���ļ����ص��������ϡ�
�ڲ��������У����һ����Ҫ���ص��̿�������2�飬���Ǿ����ó�ǰԤ������breadaO��
�����ʹ��breadO�������е����ȡ�����ڶ��̹����г���I/O�������󣬾�ֻ�ܷ�����
�ع��̷��ء�����ȡ�Ĵ��̿��ʹ��memcpyO�����Ӹ��ٻ������и��Ƶ��ڴ���������Ӧ
λ�ô���ͬʱ��ʾ�Ѽ��صĿ�������ʾ�ַ����еİ˽�������\010����ʾ��ʾһ���Ʊ����
*/
	printk ("Loading %d bytes into ram disk... 0000k",
		nblocks << BLOCK_SIZE_BITS);
/* cp ָ����������ʼ����Ȼ�󽫴����ϵĸ��ļ�ϵͳӳ���ļ����Ƶ��������ϡ�	*/
	cp = rd_start;
	while (nblocks)
		{
			if (nblocks > 2)		/* ������ȡ�Ŀ�������3 ������ó�ǰԤ����ʽ�����ݿ顣	*/
	bh = breada (ROOT_DEV, block, block + 1, block + 2, -1);
			else					/* ����͵����ȡ��	*/
	bh = bread (ROOT_DEV, block);
			if (!bh)
	{
		printk ("I/O error on block %d, aborting load\n", block);
		return;
	}
			(void) memcpy (cp, bh->b_data, BLOCK_SIZE);	/* ���������е����ݸ��Ƶ�cp ����	*/
			brelse (bh);								/* �ͷŻ�������	*/
			printk ("\010\010\010\010\010%4dk", i);		/* ��ӡ���ؿ����ֵ��	*/
			cp += BLOCK_SIZE;							/* ������ָ��ǰ�ơ�	*/
			block++;
			nblocks--;
			i++;
		}
/* ��boot���д�256�̿鿪ʼ���������ļ�ϵͳ������Ϻ�������ʾ��done��������Ŀǰ
���ļ��豸���޸ĳ������̵��豸��0x0101����󷵻ء�
*/
	printk ("\010\010\010\010\010done \n");
	ROOT_DEV = 0x0101;		/* �޸�ROOT_DEV ʹ��ָ��������ramdisk��	*/
}
