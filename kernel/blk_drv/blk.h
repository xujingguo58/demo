#ifndef _BLK_H
#define _BLK_H

#define NR_BLK_DEV	7
/*
 * NR_REQUEST is the number of entries in the request-queue.
 * NOTE that writes may use only the low 2/3 of these: reads
 * take precedence.
 *
 * 32 seems to be a reasonable number: enough to get some benefit
 * from the elevator-mechanism, but not so much as to lock a lot of
 * buffers when they are in the queue. 64 seems to be too many (easily
 * long pauses in reading when heavy writing/syncing is going on)
 */
/*
* OK��������request �ṹ��һ����չ��ʽ�������ʵ���Ժ����ǾͿ����ڷ�ҳ������
* ʹ��ͬ����request �ṹ���ڷ�ҳ�����У�'bh'��NULL����'waiting'�����ڵȴ���/д����ɡ�
*/
/* �����������������Ľṹ����������ֶ�dev = -1�����ʾ�����и���û�б�ʹ�á�
�ֶ�cmd��ȡ����READ(O)��WRITE (1)(������include/linux/fs.h��26�п�ʼ��)��
*/
#define NR_REQUEST	32

/*
 * Ok, this is an expanded form so that we can use the same
 * request for paging requests when that is implemented. In
 * paging, 'bh' is NULL, and 'waiting' is used to wait for
 * read/write completion.
 */
struct request {
	int dev;						/* -1 if no request ��������豸�š�	*/
	int cmd;						/* READ or WRITE 	����(READ ��WRITE)��	*/
	int errors;					/*����ʱ�����Ĵ��������	*/
	unsigned long sector;			/* ��ʼ������(1 ��=2 ����)	*/
	unsigned long nr_sectors;		/* ��/д��������	*/
	char *buffer;					/* ���ݻ�������	*/
	struct task_struct *waiting;	/* ����ȴ�����ִ����ɵĵط���	*/
	struct buffer_head *bh;		/* ������ͷָ��(include/linux/fs.h,68)��	*/
	struct request *next;			/* ָ����һ�����	*/
};

/*
 * This is used in the elevator algorithm: Note that
 * reads always go before writes. This is natural: reads
 * are much more time-critical than writes.
 */
/*
* ����Ķ������ڵ����㷨��ע�������������д����֮ǰ���С�
* ���Ǻ���Ȼ�ģ���������ʱ���Ҫ��Ҫ��д�ϸ�öࡣ
*/
/* ������в���31��32��ȡֵ�����涨�������ṹrequest��ָ�롣�ú궨�����ڸ�����������
ָ����������ṹ�е���Ϣ������cmd(READ��WRITE)���豸��dev�Լ���������������sector��
���жϳ�����������ṹ��ǰ������˳�����˳���������ʿ��豸ʱ��������ִ��˳��
�������ڳ���blk��drv/11��rw��blk.c�к���add��request()�б����ã���96�У����ú겿��
��ʵ���˶�������������ܣ���һ����������ϲ����ܣ���
*/
#define IN_ORDER(s1,s2) \
((s1)->cmd<(s2)->cmd || (s1)->cmd==(s2)->cmd && \
 ((s1)->dev < (s2)->dev || ((s1)->dev == (s2)->dev && \
							(s1)->sector < (s2)->sector)))

/* ���豸�ṹ��	*/
struct blk_dev_struct
{
	void (*request_fn) (void);				/* ��������ĺ���ָ�롣	*/
	struct request *current_request;			/* ������Ϣ�ṹ��	*/
};

extern struct blk_dev_struct blk_dev[NR_BLK_DEV];	/* ���豸���飬ÿ�ֿ��豸ռ��һ�	*/
extern struct request request[NR_REQUEST];			/* ����������顣	*/
extern struct task_struct *wait_for_request;		/* �ȴ�����������Ľ��̶���ͷָ�롣��	*/

/* �ڿ��豸����������hd.c��������ͷ�ļ�ʱ�������ȶ��������������豸�����豸�š�
��������61��һ87�о���Ϊ�������ļ����������������ȷ�ĺ궨�塣
*/
#ifdef MAJOR_NR								/* ���豸�š�	*/

/*
 * Add entries as needed. Currently the only block devices
 * supported are hard-disks and floppies.
 */
/*
* ��Ҫʱ������Ŀ��Ŀǰ���豸��֧��Ӳ�̺����̣����������̣���
*/

#if (MAJOR_NR == 1)							/* RAM �̵����豸����1����������Ķ�����������ڴ�����豸��ҲΪ1��	*/
/* ram disk  RAM �̣��ڴ������̣� */
#define DEVICE_NAME "ramdisk"				/* �豸����ramdisk��	*/
#define DEVICE_REQUEST do_rd_request		/* �豸������do_rd_request()��	*/
#define DEVICE_NR(device) ((device) & 7)	/* �豸��(0--7)��	*/
#define DEVICE_ON(device)					/* �����豸�����������뿪���͹رա�	*/
#define DEVICE_OFF(device)					/* �ر��豸��	*/

#elif (MAJOR_NR == 2)						/* ���������豸����2��	*/
/* floppy */
#define DEVICE_NAME "floppy"				/* �豸����floppy��	*/
#define DEVICE_INTR do_floppy				/* �豸�жϴ�������do_floppy()��	*/
#define DEVICE_REQUEST do_fd_request		/* �豸������do_fd_request()��	*/
#define DEVICE_NR(device) ((device) & 3)	/* �豸�ţ�0--3����	*/
#define DEVICE_ON(device) floppy_on(DEVICE_NR(device))		/* �����豸����floppyon()��	*/
#define DEVICE_OFF(device) floppy_off(DEVICE_NR(device))	/* �ر��豸����floppyoff()��	*/

#elif (MAJOR_NR == 3)						/* Ӳ�����豸����3��	*/
/* harddisk */
#define DEVICE_NAME "harddisk"				/* Ӳ������harddisk��	*/
#define DEVICE_INTR do_hd					/* �豸�жϴ�������do_hd()��	*/
#define DEVICE_REQUEST do_hd_request		/* �豸������do_hd_request()��	*/
#define DEVICE_NR(device) (MINOR(device)/5)	/* �豸�ţ�0--1����ÿ��Ӳ�̿�����4 ��������	*/
#define DEVICE_ON(device)					/* Ӳ��һֱ�ڹ��������뿪���͹رա�	*/
#define DEVICE_OFF(device)

//#elif
#else
/* unknown blk device δ֪���豸	*/
#error "unknown blk device"

#endif

/* Ϊ�˱��ڱ�̱�ʾ�����ﶨ���������ꡣ	*/
#define CURRENT (blk_dev[MAJOR_NR].current_request)	/* CURRENT Ϊָ�����豸�ŵĵ�ǰ����ṹ��	*/
#define CURRENT_DEV DEVICE_NR(CURRENT->dev)			/* CURRENT_DEV ΪCURRENT ���豸�š�	*/

/* ����������豸�жϴ����������ų���DEHCE��INTR�����������Ϊһ������ָ�룬��Ĭ��Ϊ
NULL���������Ӳ�̿��豸��ǰ���80��86�к궨����Ч����������97�еĺ���ָ�붨��
���� ��void (*do��hd)(void) = NULL;����
*/
#ifdef DEVICE_INTR
void (*DEVICE_INTR)(void) = NULL;
#endif
static void (DEVICE_REQUEST)(void);

/* ����ָ���Ļ��������飩��
���ָ���Ļ�����bh��û�б�����������ʾ������Ϣ�����򽫸û����������������ѵȴ�
�û������Ľ��̡������ǻ�����ͷָ�롣
*/
static inline void unlock_buffer(struct buffer_head * bh)
{
	if (!bh->b_lock)						/* ���ָ���Ļ�����bh ��û�б�����������ʾ������Ϣ��	*/
		printk (DEVICE_NAME ": free buffer being unlocked\n");
	bh->b_lock = 0;						/* ���򽫸û�����������	*/
	wake_up (&bh->b_wait);				/* ���ѵȴ��û������Ľ��̡�	*/
}

/* ������������
//���ȹر�ָ�����豸��Ȼ����˴ζ�д�������Ƿ���Ч�������Ч����ݲ���ֵ���û���
�����ݸ��±�־���������û�������������±�־����ֵ��0����ʾ�˴�������Ĳ�����ʧ
�ܣ������ʾ��ؿ��豸10������Ϣ����󣬻��ѵȴ���������Ľ����Լ��ȴ���������
����ֵĽ��̣��ͷŲ���������������ɾ������������ѵ�ǰ������ָ��ָ����һ�����
*/
static inline void end_request(int uptodate)
{
	DEVICE_OFF (CURRENT->dev);			/* �ر��豸��	*/
	if (CURRENT->bh)
    {									/* CURRENT Ϊָ�����豸�ŵĵ�ǰ����ṹ��	*/
		CURRENT->bh->b_uptodate = uptodate;/* �ø��±�־��	*/
		unlock_buffer (CURRENT->bh);		/* ������������	*/
    }
	if (!uptodate)
    {									/* ������±�־Ϊ0 ����ʾ�豸������Ϣ��	*/
		printk (DEVICE_NAME " I/O error\n\r");
		printk ("dev %04x, block %d\n\r", CURRENT->dev, CURRENT->bh->b_blocknr);
    }
	wake_up (&CURRENT->waiting);			/* ���ѵȴ���������Ľ��̡�	*/
	wake_up (&wait_for_request);			/* ���ѵȴ�����������Ľ��̡�	*/
	CURRENT->dev = -1;					/* �ͷŸ������	*/
	CURRENT = CURRENT->next;				/* ������������ɾ�������������	*/
}										/*��ǰ������ָ��ָ����һ�������	*/

/* �����ʼ��������ꡣ
���ڼ������豸��������ʼ����������ĳ�ʼ���������ƣ��������Ϊ���Ƕ�����һ��
ͳһ�ĳ�ʼ���ꡣ�ú����ڶԵ�ǰ���������һЩ��Ч���жϡ������������£�
����豸��ǰ������Ϊ�գ�NULL������ʾ���豸Ŀǰ������Ҫ����������������˳�������
���������ǰ���������豸�����豸�Ų�������������������豸�ţ�˵�����������
�ҵ��ˣ������ں���ʾ������Ϣ��ͣ�������������������õĻ����û�б�������Ҳ˵����
�˳���������⣬������ʾ������Ϣ��ͣ����
*/
#define INIT_REQUEST \
repeat: \
if (!CURRENT) \
	return; \
if (MAJOR(CURRENT->dev) != MAJOR_NR) \
	panic(DEVICE_NAME ": request list destroyed"); \
	if (CURRENT->bh) { \
	if (!CURRENT->bh->b_lock) \
		panic(DEVICE_NAME ": block not locked"); \
}
/* �����ǰ����ṹָ��Ϊnull �򷵻ء�	*/
/* �����ǰ�豸�����豸�Ų�����������		*/
/* ����ڽ����������ʱ������û������������	*/

#endif

#endif