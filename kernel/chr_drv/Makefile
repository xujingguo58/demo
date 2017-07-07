#
# Makefile for the FREAX-kernel character device drivers.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# FREAX(Linux)�ں��ַ��豸���������Makefile �ļ���
# ע�⣡������ϵ����'make dep'�Զ����еģ���Ҳ���Զ�ȥ��ԭ����������Ϣ����Ҫ�����Լ���
# ������ϵ��Ϣ��������������ر��ļ��ģ�Ҳ������һ��.c �ļ�����Ϣ����

AR	=ar 			# GNU �Ķ������ļ������������ڴ������޸��Լ��ӹ鵵�ļ��г�ȡ�ļ���
AS	=as --32 			# GNU �Ļ�����
LD	=ld -melf_i386 			# GNU �����ӳ���
LDFLAGS	=-x 			#-s # ���ӳ������еĲ�����-s ����ļ���ʡ�����з�����Ϣ��-x ɾ�����оֲ����š�
CC	=gcc -m32  			# GNU C ���Ա�������
# ��һ����C �������ѡ�-Wall -fleading-underscore ��ʾ���еľ�����Ϣ��-O �Ż�ѡ��Ż����볤�Ⱥ�ִ��ʱ�䣻
# -fstrength-reduce �Ż�ѭ��ִ�д��룬�ų��ظ�������-fomit-frame-pointer ʡ�Ա��治��Ҫ
# �Ŀ��ָ�룻 �ϲ��Ĵ��������ټĴ������ʹ�ã�-finline-functions �����м�
# ����С�ĺ�������Ƕ����ó����У�-mstring-insns Linus �Լ���ӵ��Ż�ѡ��Ժ���ʹ�ã�
# -nostdinc -I../include ��ʹ��Ĭ��·���еİ����ļ�����ʹ��ָ��Ŀ¼�е�(../../include)��
CFLAGS	=-Wall -fleading-underscore -fno-stack-protector -O -g -fstrength-reduce -fomit-frame-pointer  \
	-finline-functions -nostdinc -I../../include
# C ǰ����ѡ�-E ֻ����C ǰ������������ָ����C �������Ԥ����������������������׼��
# ���豸��ָ��������ļ��У�-nostdinc -I../../include ͬǰ��
CPP	=gcc -m32  -E -nostdinc -I../../include

# ����Ĺ���ָʾmake ���������������е�.c �ļ���������.s �����򡣸ù��������
# ָʹgcc -m32  ����CFLAGS ��ָ����ѡ���C �������󲻽��л���ֹͣ��-S�����Ӷ�������
# ����ĸ���C �ļ���Ӧ�Ļ������ļ���Ĭ��������������Ļ������ļ�����ԭC �ļ���
# ȥ��.c ������.s ��׺��-o ��ʾ���������ļ������ơ�����$*.s����$@�����Զ�Ŀ�������
# $<������һ���Ⱦ����������Ｔ�Ƿ�������*.c ���ļ���
.c.s:
	$(CC) $(CFLAGS) \
	-S -o $*.s $<
.s.o:
	$(AS) -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-c -o $*.o $<

OBJS  = tty_io.o console.o keyboard.o serial.o rs_io.o \
	tty_ioctl.o

chr_drv.a: $(OBJS)
	$(AR) rcs chr_drv.a $(OBJS)
	sync

keyboard.s: keyboard.S ../../include/linux/config.h
	$(CPP) -traditional keyboard.S -o keyboard.s

clean:
	rm -f core *.o *.a tmp_make keyboard.s
	for i in *.c;do rm -f `basename $$i .c`.s;done

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CPP) -M $$i;done) >> tmp_make
	cp tmp_make Makefile

### Dependencies:
console.s console.o : console.c ../../include/linux/sched.h \
  ../../include/linux/head.h ../../include/linux/fs.h \
  ../../include/sys/types.h ../../include/linux/mm.h ../../include/signal.h \
  ../../include/linux/tty.h ../../include/termios.h ../../include/asm/io.h \
  ../../include/asm/system.h 
serial.s serial.o : serial.c ../../include/linux/tty.h ../../include/termios.h \
  ../../include/linux/sched.h ../../include/linux/head.h \
  ../../include/linux/fs.h ../../include/sys/types.h ../../include/linux/mm.h \
  ../../include/signal.h ../../include/asm/system.h ../../include/asm/io.h 
tty_io.s tty_io.o : tty_io.c ../../include/ctype.h ../../include/errno.h \
  ../../include/signal.h ../../include/sys/types.h \
  ../../include/linux/sched.h ../../include/linux/head.h \
  ../../include/linux/fs.h ../../include/linux/mm.h ../../include/linux/tty.h \
  ../../include/termios.h ../../include/asm/segment.h \
  ../../include/asm/system.h 
tty_ioctl.s tty_ioctl.o : tty_ioctl.c ../../include/errno.h ../../include/termios.h \
  ../../include/linux/sched.h ../../include/linux/head.h \
  ../../include/linux/fs.h ../../include/sys/types.h ../../include/linux/mm.h \
  ../../include/signal.h ../../include/linux/kernel.h \
  ../../include/linux/tty.h ../../include/asm/io.h \
  ../../include/asm/segment.h ../../include/asm/system.h 