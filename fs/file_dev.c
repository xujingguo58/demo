/*
* linux/fs/file_dev.c
*
* (C) 1991 Linus Torvalds
*/

#include <errno.h>			/* �����ͷ�ļ�������ϵͳ�и��ֳ���š�(Linus ��minix ��������)	*/
#include <fcntl.h>			/* �ļ�����ͷ�ļ��������ļ������������Ĳ������Ƴ������ŵĶ��塣	*/
#include <linux/sched.h>	/* ���ȳ���ͷ�ļ�������������ṹtask_struct����ʼ����0 �����ݣ�	*/
							/* ����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������䡣	*/
#include <linux/kernel.h>	/* �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣	*/
#include <asm/segment.h>	/* �β���ͷ�ļ����������йضμĴ���������Ƕ��ʽ��ຯ����	*/

#define MIN(a,b) (((a)<(b))?(a):(b))	/* ȡa,b �е���Сֵ��	*/
#define MAX(a,b) (((a)>(b))?(a):(b))	/* ȡa,b �е����ֵ��	*/

/* �ļ������� - ����i �ڵ���ļ��ṹ�����豸���ݡ�
 * ��i �ڵ����֪���豸�ţ���filp �ṹ����֪���ļ��е�ǰ��дָ��λ�á�buf ָ����
 * ��̬�л�������λ�ã�count Ϊ��Ҫ��ȡ���ֽ����� * ����ֵ��ʵ�ʶ�ȡ���ֽ�������
 * �����(С��0)��	*/
int file_read (struct m_inode *inode, struct file *filp, char *buf, int count)
{
	int left, chars, nr;
	struct buffer_head *bh;

/* �����жϲ�������Ч�ԡ�����Ҫ��ȡ���ֽڼ���countС�ڵ����㣬�򷵻�0��������Ҫ��
 * ȡ���ֽ���������0����ѭ��ִ�����������ֱ������ȫ���������������⡣�ڶ�ѭ������
 * �����У����Ǹ���i�ڵ���ļ���ṹ��Ϣ��������bmapO�õ������ļ���ǰ��дλ�õ�
 * ���ݿ����豸�϶�Ӧ���߼����nr����nr��Ϊ0�����i�ڵ�ָ�����豸�϶�ȡ���߼��顣
 * ���������ʧ�����˳�ѭ������nrΪ0����ʾָ�������ݿ鲻���ڣ��û����ָ��ΪNULL��
 * (filp->f_pos)/BLOCK_SIZE���ڼ�����ļ���ǰָ���������ݿ�š�	*/
	if ((left = count) <= 0)
		return 0;
	while (left)
	{
		if (nr = bmap (inode, (filp->f_pos) / BLOCK_SIZE))
		{
			if (!(bh = bread (inode->i_dev, nr)))
				break;
		}
		else
			bh = NULL;
/* �������Ǽ����ļ���дָ����_���е�ƫ��ֵnr�����ڸ����ݿ�������ϣ����ȡ���ֽ���
 * Ϊ��BLOCK��SIZE - nr����Ȼ������ڻ����ȡ���ֽ���left���Ƚϣ�����Сֵ��Ϊ���β���
 * ���ȡ���ֽ���chars�������BLOCK��SIZE - nr�� > left����˵���ÿ�����Ҫ��ȡ�����һ
 * �����ݣ���֮����Ҫ��ȡ��һ�����ݡ�֮�������д�ļ�ָ�롣ָ��ǰ�ƴ˴ν���ȡ����
 * ����chars��ʣ���ֽڼ���left��Ӧ��ȥchars��	*/
		nr = filp->f_pos % BLOCK_SIZE;
		chars = MIN (BLOCK_SIZE - nr, left);
		filp->f_pos += chars;
		left -= chars;
/* �����豸�϶��������ݣ���p ָ��������ݿ黺�����п�ʼ��ȡ��λ�ã����Ҹ���chars �ֽ�
 * ���û�������buf �С��������û�������������chars ��0 ֵ�ֽڡ�	*/
		if (bh)
		{
			char *p = nr + bh->b_data;
			while (chars-- > 0)
				put_fs_byte (*(p++), buf++);
			brelse (bh);
		}
		else
		{
			while (chars-- > 0)
				put_fs_byte (0, buf++);
		}
	}
/* �޸ĸ�i�ڵ�ķ���ʱ��Ϊ��ǰʱ�䡣���ض�ȡ���ֽ���������ȡ�ֽ���Ϊ0���򷵻س���š�
 * RRENT��TIME�Ƕ�����include/linux/sched.h��142���ϵĺ꣬���ڼ���UNIXʱ�䡣����
 * 1970��1��1��0ʱ0�뿪ʼ������ǰ��ʱ�䡣��λ���롣	*/
	inode->i_atime = CURRENT_TIME;
	return (count - left) ? (count - left) : -ERROR;
}

/* �ļ�д����-����i�ڵ���ļ��ṹ��Ϣ�����û�����д���ļ��С�
 * ��i�ڵ����ǿ���֪���豸�ţ�����file�ṹ����֪���ļ��е�ǰ��дָ��λ�á�bufָ��
 * �û�̬�л�������λ�ã�countΪ��Ҫд����ֽ���������ֵ��ʵ��д����ֽ����������
 * �ţ�С��0����	*/
int file_write (struct m_inode *inode, struct file *filp, char *buf, int count)
{
	off_t pos;
	int block, c;
	struct buffer_head *bh;
	char *p;
	int i = 0;

/*
* ok, append may not work when many processes are writing at the same time
* but so what. That way leads to madness anyway.
*/
/*
* ok����������ͬʱдʱ��append �������ܲ��У�������������
* ���������������ᵼ�»���һ�š�
*/
/* ����ȷ������д���ļ���λ�á������Ҫ���ļ���������ݣ����ļ���дָ���Ƶ��ļ�β
 * ��������ͽ����ļ���ǰ��дָ�봦д�롣	*/
	if (filp->f_flags & O_APPEND)
		pos = inode->i_size;
	else
		pos = filp->f_pos;
/* Ȼ������д���ֽ���i (�տ�ʼʱΪ0)С��ָ��д���ֽ���countʱ��ѭ��ִ�����²�����
 * ��ѭ�����������У�������ȡ�ļ����ݿ�ţ�pos/BLOCK��SIZE �����豸�϶�Ӧ���߼����
 * block�������Ӧ���߼��鲻���ھʹ���һ�顣����õ����߼����=0�����ʾ����ʧ�ܣ�
 * �����˳�ѭ�����������Ǹ��ݸ��߼���Ŷ�ȡ�豸�ϵ���Ӧ�߼��飬������Ҳ�˳�ѭ����	*/
	while (i < count)
	{
		if (!(block = create_block (inode, pos / BLOCK_SIZE)))
			break;
		if (!(bh = bread (inode->i_dev, block)))
			break;
/* ��ʱ�����ָ��bh��ָ��ն�����ļ����ݿ顣����������ļ���ǰ��дָ���ڸ����ݿ���
 * ��ƫ��ֵc������ָ��pָ�򻺳���п�ʼд�����ݵ�λ�ã����øû�������޸ı�־������
 * ���е�ǰָ�룬�ӿ�ʼ��дλ�õ���ĩ����д��c =(BLOCK_SIZE - c)���ֽڡ���c����ʣ��
 * ����д����ֽ�����count - i������˴�ֻ����д��c = (count - i)���ֽڼ��ɡ�	*/
		c = pos % BLOCK_SIZE;
		p = c + bh->b_data;
		bh->b_dirt = 1;
		c = BLOCK_SIZE - c;
		if (c > count - i)
			c = count - i;
/* ��д������֮ǰ�����ǀ�����ú��¶ܻ�����Ҫ��д�ļ��е�λ�á�������ǰ�pos
 * ָ��ǰ�ƴ˴���д����ֽ����������ʱPOSλ��ֵ�������ļ���ǰ���ȣ����޸�i�ڵ���
 * �ļ������ֶΣ�����i�ڵ����޸ı�־��Ȼ��Ѵ˴�Ҫд����ֽ���C�ۼӵ���д���ֽڼ�
 * ��ֵi�У���ѭ���ж�ʹ�á����Ŵ��û�������buf�и���C���ֽڵ����ٻ������pָ��
 * �Ŀ�ʼλ�ô������������ͷŸû���顣	*/
		pos += c;
		if (pos > inode->i_size)
		{
			inode->i_size = pos;
			inode->i_dirt = 1;
		}
		i += c;
		while (c-- > 0)
			*(p++) = get_fs_byte (buf++);
		brelse (bh);
	}
/* �������Ѿ�ȫ��д���ļ�������д���������з�������ʱ�ͻ��˳�ѭ������ʱ���Ǹ����ļ�
 * �޸�ʱ��Ϊ��ǰʱ�䣬�������ļ���дָ�롣����˴β����������ļ�β������ݣ������
 * ����дָ���������ǰ��дλ��POS�����������ļ�i�ڵ���޸�ʱ��Ϊ��ǰʱ�䡣���
 * ��д����ֽ�������д���ֽ���Ϊ0���򷵻س����-1��	*/
	inode->i_mtime = CURRENT_TIME;
	if (!(filp->f_flags & O_APPEND))
	{
		filp->f_pos = pos;
		inode->i_ctime = CURRENT_TIME;
	}
	return (i ? i : -1);
}
