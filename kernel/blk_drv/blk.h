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
* OK，下面是request 结构的一个扩展形式，因而当实现以后，我们就可以在分页请求中
* 使用同样的request 结构。在分页处理中，'bh'是NULL，而'waiting'则用于等待读/写的完成。
*/
/* 下面是请求队列中项的结构。其中如果字段dev = -1，则表示队列中该项没有被使用。
字段cmd可取常量READ(O)或WRITE (1)(定义在include/linux/fs.h第26行开始处)。
*/
#define NR_REQUEST	32

/*
 * Ok, this is an expanded form so that we can use the same
 * request for paging requests when that is implemented. In
 * paging, 'bh' is NULL, and 'waiting' is used to wait for
 * read/write completion.
 */
struct request {
	int dev;						/* -1 if no request 发请求的设备号。	*/
	int cmd;						/* READ or WRITE 	命令(READ 或WRITE)。	*/
	int errors;					/*操作时产生的错误次数。	*/
	unsigned long sector;			/* 起始扇区。(1 块=2 扇区)	*/
	unsigned long nr_sectors;		/* 读/写扇区数。	*/
	char *buffer;					/* 数据缓冲区。	*/
	struct task_struct *waiting;	/* 任务等待操作执行完成的地方。	*/
	struct buffer_head *bh;		/* 缓冲区头指针(include/linux/fs.h,68)。	*/
	struct request *next;			/* 指向下一请求项。	*/
};

/*
 * This is used in the elevator algorithm: Note that
 * reads always go before writes. This is natural: reads
 * are much more time-critical than writes.
 */
/*
* 下面的定义用于电梯算法：注意读操作总是在写操作之前进行。
* 这是很自然的：读操作对时间的要求要比写严格得多。
*/
/* 下面宏中参数31和32的取值是上面定义的请求结构request的指针。该宏定义用于根据两个参数
指定的请求项结构中的信息（命令cmd(READ或WRITE)、设备号dev以及所操作的扇区号sector）
来判断出两个请求项结构的前后排列顺序。这个顺序将用作访问块设备时的请求项执行顺序。
这个宏会在程序blk—drv/11—rw—blk.c中函数add—request()中被调用（第96行）。该宏部分
地实现了对请求项的排序功能（另一个是请求项合并功能）。
*/
#define IN_ORDER(s1,s2) \
((s1)->cmd<(s2)->cmd || (s1)->cmd==(s2)->cmd && \
 ((s1)->dev < (s2)->dev || ((s1)->dev == (s2)->dev && \
							(s1)->sector < (s2)->sector)))

/* 块设备结构。	*/
struct blk_dev_struct
{
	void (*request_fn) (void);				/* 请求操作的函数指针。	*/
	struct request *current_request;			/* 请求信息结构。	*/
};

extern struct blk_dev_struct blk_dev[NR_BLK_DEV];	/* 块设备数组，每种块设备占用一项。	*/
extern struct request request[NR_REQUEST];			/* 请求队列数组。	*/
extern struct task_struct *wait_for_request;		/* 等待空闲请求项的进程队列头指针。。	*/

/* 在块设备驱动程序（如hd.c）包含此头文件时，必须先定义驱动程序处理设备的主设备号。
这样下面61行一87行就能为包含本文件的驱动程序给出正确的宏定义。
*/
#ifdef MAJOR_NR								/* 主设备号。	*/

/*
 * Add entries as needed. Currently the only block devices
 * supported are hard-disks and floppies.
 */
/*
* 需要时加入条目。目前块设备仅支持硬盘和软盘（还有虚拟盘）。
*/

#if (MAJOR_NR == 1)							/* RAM 盘的主设备号是1。根据这里的定义可以推理内存块主设备号也为1。	*/
/* ram disk  RAM 盘（内存虚拟盘） */
#define DEVICE_NAME "ramdisk"				/* 设备名称ramdisk。	*/
#define DEVICE_REQUEST do_rd_request		/* 设备请求函数do_rd_request()。	*/
#define DEVICE_NR(device) ((device) & 7)	/* 设备号(0--7)。	*/
#define DEVICE_ON(device)					/* 开启设备。虚拟盘无须开启和关闭。	*/
#define DEVICE_OFF(device)					/* 关闭设备。	*/

#elif (MAJOR_NR == 2)						/* 软驱的主设备号是2。	*/
/* floppy */
#define DEVICE_NAME "floppy"				/* 设备名称floppy。	*/
#define DEVICE_INTR do_floppy				/* 设备中断处理程序do_floppy()。	*/
#define DEVICE_REQUEST do_fd_request		/* 设备请求函数do_fd_request()。	*/
#define DEVICE_NR(device) ((device) & 3)	/* 设备号（0--3）。	*/
#define DEVICE_ON(device) floppy_on(DEVICE_NR(device))		/* 开启设备函数floppyon()。	*/
#define DEVICE_OFF(device) floppy_off(DEVICE_NR(device))	/* 关闭设备函数floppyoff()。	*/

#elif (MAJOR_NR == 3)						/* 硬盘主设备号是3。	*/
/* harddisk */
#define DEVICE_NAME "harddisk"				/* 硬盘名称harddisk。	*/
#define DEVICE_INTR do_hd					/* 设备中断处理程序do_hd()。	*/
#define DEVICE_REQUEST do_hd_request		/* 设备请求函数do_hd_request()。	*/
#define DEVICE_NR(device) (MINOR(device)/5)	/* 设备号（0--1）。每个硬盘可以有4 个分区。	*/
#define DEVICE_ON(device)					/* 硬盘一直在工作，无须开启和关闭。	*/
#define DEVICE_OFF(device)

//#elif
#else
/* unknown blk device 未知块设备	*/
#error "unknown blk device"

#endif

/* 为了便于编程表示，这里定义了两个宏。	*/
#define CURRENT (blk_dev[MAJOR_NR].current_request)	/* CURRENT 为指定主设备号的当前请求结构。	*/
#define CURRENT_DEV DEVICE_NR(CURRENT->dev)			/* CURRENT_DEV 为CURRENT 的设备号。	*/

/* 如果定义了设备中断处理函数符号常数DEHCE—INTR，则把它声明为一个函数指针，并默认为
NULL。例如对于硬盘块设备，前面第80—86行宏定义有效，因此下面第97行的函数指针定义
就是 “void (*do—hd)(void) = NULL;”。
*/
#ifdef DEVICE_INTR
void (*DEVICE_INTR)(void) = NULL;
#endif
static void (DEVICE_REQUEST)(void);

/* 解锁指定的缓冲区（块）。
如果指定的缓冲区bh并没有被上锁，则显示警告信息。否则将该缓冲区解锁，并唤醒等待
该缓冲区的进程。参数是缓冲区头指针。
*/
static inline void unlock_buffer(struct buffer_head * bh)
{
	if (!bh->b_lock)						/* 如果指定的缓冲区bh 并没有被上锁，则显示警告信息。	*/
		printk (DEVICE_NAME ": free buffer being unlocked\n");
	bh->b_lock = 0;						/* 否则将该缓冲区解锁。	*/
	wake_up (&bh->b_wait);				/* 唤醒等待该缓冲区的进程。	*/
}

/* 结束请求处理。
//首先关闭指定块设备，然后检查此次读写缓冲区是否有效。如果有效则根据参数值设置缓冲
区数据更新标志，并解锁该缓冲区。如果更新标志参数值是0，表示此次请求项的操作已失
败，因此显示相关块设备10错误信息。最后，唤醒等待该请求项的进程以及等待空闲请求
项出现的进程，释放并从请求项链表中删除本请求项，并把当前请求项指针指向下一请求项。
*/
static inline void end_request(int uptodate)
{
	DEVICE_OFF (CURRENT->dev);			/* 关闭设备。	*/
	if (CURRENT->bh)
    {									/* CURRENT 为指定主设备号的当前请求结构。	*/
		CURRENT->bh->b_uptodate = uptodate;/* 置更新标志。	*/
		unlock_buffer (CURRENT->bh);		/* 解锁缓冲区。	*/
    }
	if (!uptodate)
    {									/* 如果更新标志为0 则显示设备错误信息。	*/
		printk (DEVICE_NAME " I/O error\n\r");
		printk ("dev %04x, block %d\n\r", CURRENT->dev, CURRENT->bh->b_blocknr);
    }
	wake_up (&CURRENT->waiting);			/* 唤醒等待该请求项的进程。	*/
	wake_up (&wait_for_request);			/* 唤醒等待空闲请求项的进程。	*/
	CURRENT->dev = -1;					/* 释放该请求项。	*/
	CURRENT = CURRENT->next;				/* 从请求链表中删除该请求项，并且	*/
}										/*当前请求项指针指向下一个请求项。	*/

/* 定义初始化请求项宏。
由于几个块设备驱动程序开始处对请求项的初始化操作相似，因此这里为它们定义了一个
统一的初始化宏。该宏用于对当前请求项进行一些有效性判断。所做工作如下：
如果设备当前请求项为空（NULL），表示本设备目前已无需要处理的请求项。于是退出函数。
否则，如果当前请求项中设备的主设备号不等于驱动程序定义的主设备号，说明请求项队列
乱掉了，于是内核显示出错信息并停机。否则若请求项中用的缓冲块没有被锁定，也说明内
核程序出了问题，于是显示出错信息并停机。
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
/* 如果当前请求结构指针为null 则返回。	*/
/* 如果当前设备的主设备号不对则死机。		*/
/* 如果在进行请求操作时缓冲区没锁定则死机。	*/

#endif

#endif
