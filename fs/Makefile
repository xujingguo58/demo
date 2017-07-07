AR =ar# GNU �Ķ������ļ������������ڴ������޸��Լ��ӹ鵵�ļ��г�ȡ�ļ���
AS =as --32# GNU �Ļ�����
CC =gcc -m32 # GNU C ���Ա�������
LD =ld -melf_i386# GNU �����ӳ���
# C �������ѡ�-Wall -fleading-underscore ��ʾ���еľ�����Ϣ��-O �Ż�ѡ��Ż����볤�Ⱥ�ִ��ʱ�䣻
# -fstrength-reduce �Ż�ѭ��ִ�д��룬�ų��ظ�������-fomit-frame-pointer ʡ�Ա��治��Ҫ
# �Ŀ��ָ�룻-fcombine-regs �ϲ��Ĵ��������ټĴ������ʹ�ã�-mstring-insns Linus �Լ�
# ��ӵ��Ż�ѡ��Ժ���ʹ�ã�-nostdinc -I../include ��ʹ��Ĭ��·���еİ����ļ�����ʹ
# ������ָ��Ŀ¼�е�(../include)��
CFLAGS =-Wall -fleading-underscore -fno-stack-protector -O -fstrength-reduce  -fomit-frame-pointer \
	-nostdinc -I../include
# C ǰ����ѡ�-E ֻ����C ǰ������������ָ����C �������Ԥ����������������������׼��
# ���豸��ָ��������ļ��У�-nostdinc -I../include ͬǰ��
CPP =gcc -m32  -E -nostdinc -I../include

# ����Ĺ���ָʾmake ���������������е�.c �ļ���������.s �����򡣸ù��������
# ָʹgcc -m32  ����CFLAGS ��ָ����ѡ���C �������󲻽��л���ֹͣ��-S�����Ӷ�������
# ����ĸ���C �ļ���Ӧ�Ļ������ļ���Ĭ��������������Ļ������ļ�����ԭC �ļ���
# ȥ��.c ������.s ��׺��-o ��ʾ���������ļ������ơ�����$*.s����$@�����Զ�Ŀ�������
# $<������һ���Ⱦ����������Ｔ�Ƿ�������*.c ���ļ���
.c.s:
	$(CC) $(CFLAGS) \
	-S -o $*.s $<
# ������*.c �ļ������*.o Ŀ���ļ������������ӡ�
.c.o:
	$(CC) $(CFLAGS) \
	-c -o $*.o $<
# ��������ʾ������.s �������ļ������.o Ŀ���ļ���16 ����ʵ�ָò����ľ������
.s.o:
	$(AS) -o $*.o $<

# ����Ŀ���ļ�����OBJS��
OBJS=	open.o read_write.o inode.o file_table.o buffer.o super.o \
	block_dev.o char_dev.o file_dev.o stat.o exec.o pipe.o namei.o \
	bitmap.o fcntl.o ioctl.o truncate.o select.o

# �������Ⱦ�����OBJS ��ʹ��������������ӳ�Ŀ��fs.o
fs.o: $(OBJS)
	$(LD) -r -o fs.o $(OBJS)

# ����Ĺ�������������������ִ��'make clean'ʱ���ͻ�ִ��26--27 ���ϵ����ȥ�����б���
# �������ɵ��ļ���'rm'���ļ�ɾ�����ѡ��-f �����Ǻ��Բ����ڵ��ļ������Ҳ���ʾɾ����Ϣ��
clean:
	rm -f core *.o *.a tmp_make
	for i in *.c;do rm -f `basename $$i .c`.s;done

# �����Ŀ���������ڼ����ļ�֮���������ϵ���������£�
# ʹ���ַ����༭����sed ��Makefile �ļ������Ｔ���Լ������д��������Ϊɾ��Makefile
# �ļ���'### Dependencies'�к���������У������35 ��ʼ���У���������tmp_make
# ��ʱ�ļ���30 �е����ã���Ȼ���fs/Ŀ¼�µ�ÿһ��C �ļ�ִ��gcc -m32  Ԥ��������.
# -M ��־����Ԥ���������������ÿ��Ŀ���ļ�����ԵĹ��򣬲�����Щ�������make �﷨��
# ����ÿһ��Դ�ļ���Ԥ�����������һ��make ����������ʽ����ӦԴ�����ļ���Ŀ��
# �ļ���������������ϵ--��Դ�ļ��а���������ͷ�ļ��б�����Ԥ������������ӵ���ʱ
# �ļ�tmp_make �У�Ȼ�󽫸���ʱ�ļ����Ƴ��µ�Makefile �ļ���
dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do $(CPP) -M $$i;done) >> tmp_make
	cp tmp_make Makefile

### Dependencies:
bitmap.o : bitmap.c ../include/string.h ../include/linux/sched.h \
  ../include/linux/head.h ../include/linux/fs.h ../include/sys/types.h \
  ../include/linux/mm.h ../include/signal.h ../include/linux/kernel.h 
block_dev.o : block_dev.c ../include/errno.h ../include/linux/sched.h \
  ../include/linux/head.h ../include/linux/fs.h ../include/sys/types.h \
  ../include/linux/mm.h ../include/signal.h ../include/linux/kernel.h \
  ../include/asm/segment.h ../include/asm/system.h 
buffer.o : buffer.c ../include/stdarg.h ../include/linux/config.h \
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \
  ../include/sys/types.h ../include/linux/mm.h ../include/signal.h \
  ../include/linux/kernel.h ../include/asm/system.h ../include/asm/io.h 
char_dev.o : char_dev.c ../include/errno.h ../include/sys/types.h \
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \
  ../include/linux/mm.h ../include/signal.h ../include/linux/kernel.h \
  ../include/asm/segment.h ../include/asm/io.h 
exec.o : exec.c ../include/errno.h ../include/string.h \
  ../include/sys/stat.h ../include/sys/types.h ../include/a.out.h \
  ../include/linux/fs.h ../include/linux/sched.h ../include/linux/head.h \
  ../include/linux/mm.h ../include/signal.h ../include/linux/kernel.h \
  ../include/asm/segment.h 
fcntl.o : fcntl.c ../include/string.h ../include/errno.h \
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \
  ../include/sys/types.h ../include/linux/mm.h ../include/signal.h \
  ../include/linux/kernel.h ../include/asm/segment.h ../include/fcntl.h \
  ../include/sys/stat.h 
file_dev.o : file_dev.c ../include/errno.h ../include/fcntl.h \
  ../include/sys/types.h ../include/linux/sched.h ../include/linux/head.h \
  ../include/linux/fs.h ../include/linux/mm.h ../include/signal.h \
  ../include/linux/kernel.h ../include/asm/segment.h 
file_table.o : file_table.c ../include/linux/fs.h ../include/sys/types.h 
inode.o : inode.c ../include/string.h ../include/sys/stat.h \
  ../include/sys/types.h ../include/linux/sched.h ../include/linux/head.h \
  ../include/linux/fs.h ../include/linux/mm.h ../include/signal.h \
  ../include/linux/kernel.h ../include/asm/system.h 
ioctl.o : ioctl.c ../include/string.h ../include/errno.h \
  ../include/sys/stat.h ../include/sys/types.h ../include/linux/sched.h \
  ../include/linux/head.h ../include/linux/fs.h ../include/linux/mm.h \
  ../include/signal.h 
namei.o : namei.c ../include/linux/sched.h ../include/linux/head.h \
  ../include/linux/fs.h ../include/sys/types.h ../include/linux/mm.h \
  ../include/signal.h ../include/linux/kernel.h ../include/asm/segment.h \
  ../include/string.h ../include/fcntl.h ../include/errno.h \
  ../include/const.h ../include/sys/stat.h 
open.o : open.c ../include/string.h ../include/errno.h ../include/fcntl.h \
  ../include/sys/types.h ../include/utime.h ../include/sys/stat.h \
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \
  ../include/linux/mm.h ../include/signal.h ../include/linux/tty.h \
  ../include/termios.h ../include/linux/kernel.h ../include/asm/segment.h 
pipe.o : pipe.c ../include/signal.h ../include/sys/types.h \
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \
  ../include/linux/mm.h ../include/asm/segment.h 
read_write.o : read_write.c ../include/sys/stat.h ../include/sys/types.h \
  ../include/errno.h ../include/linux/kernel.h ../include/linux/sched.h \
  ../include/linux/head.h ../include/linux/fs.h ../include/linux/mm.h \
  ../include/signal.h ../include/asm/segment.h 
stat.o : stat.c ../include/errno.h ../include/sys/stat.h \
  ../include/sys/types.h ../include/linux/fs.h ../include/linux/sched.h \
  ../include/linux/head.h ../include/linux/mm.h ../include/signal.h \
  ../include/linux/kernel.h ../include/asm/segment.h 
super.o : super.c ../include/linux/config.h ../include/linux/sched.h \
  ../include/linux/head.h ../include/linux/fs.h ../include/sys/types.h \
  ../include/linux/mm.h ../include/signal.h ../include/linux/kernel.h \
  ../include/asm/system.h ../include/errno.h ../include/sys/stat.h 
truncate.o : truncate.c ../include/linux/sched.h ../include/linux/head.h \
  ../include/linux/fs.h ../include/sys/types.h ../include/linux/mm.h \
  ../include/signal.h ../include/sys/stat.h 