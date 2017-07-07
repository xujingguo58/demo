				!
				! setup.s (C) 1991 Linus Torvalds
				!
				! setup.s is responsible for getting the system data from the BIOS,
				! and putting them into the appropriate places in system memory.
				! both setup.s and system has been loaded by the bootblock.
				!
				! This code asks the bios for memory/disk/other parameters, and
				! puts them in a "safe" place: 0x90000-0x901FF, ie where the
				! boot-block used to be. It is then up to the protected mode
				! system to read them from there before the area is overwritten
				! for buffer-blocks.
				!
				! setup.s �����BIOS �л�ȡϵͳ���ݣ�������Щ���ݷŵ�ϵͳ�ڴ���ʵ��ط���
				! ��ʱsetup.s ��system �Ѿ���bootsect ��������ص��ڴ��С�
				!
				! ��δ���ѯ��bios �й��ڴ�/����/����������������Щ�����ŵ�һ��
				! ����ȫ�ġ��ط���0x90000-0x901FF��Ҳ��ԭ��bootsect �����������
				! �ĵط���Ȼ���ڱ�����鸲�ǵ�֮ǰ�ɱ���ģʽ��system ��ȡ��
				!

				! NOTE: These had better be the same as in bootsect.s
				! ������Щ������ú�bootsect.s �е���ͬ				!

INITSEG = 0x9000 				! we move boot here - out of the way 	! ԭ��bootsect �����ĶΡ�
SYSSEG	= 0x1000 				! system loaded at 0x10000 (65536). 	! system ��0x10000(64k)����
SETUPSEG= 0x9020 				! this is the current segment 			! ���������ڵĶε�ַ��

.globl begtext, begdata, begbss, endtext, enddata, endbss
.text
begtext:
.data
begdata:
.bss
begbss:
.text

entry start
start:
! ok, the read went well so we get current cursor position and save it for
! posterity.
				! ok�����������̹��̶����������ڽ����λ�ñ����Ա����ʹ�á�

! ��δ���ʹ�� BIOS �ж�ȡ��Ļ��ǰ���׼λ�ã��С��У������������ڴ� 0x90000 ����2 �ֽڣ���
!   ����̨��ʼ������ᵽ�˴���ȡ��ֵ��
! BIOS �ж� 0x10 ���ܺ� ah = 0x03�������׼λ�á�
! ���룺bh    ҳ��
! ���أ�ch    ɨ�迪ʼ�ߣ�cl    ɨ������ߣ�dh    �к�(0x00 ����)��dl    �к�(0x00�����)��
!
! �¾佫 ds �ó� INITSET(0x9000)�����Ѿ��� bootsect ���������ù������������� setup ����
! Linus ������Ҫ����������һ�¡�

	mov ax,#INITSEG ! this is done in bootsect already, but...
				! ��ds �ó�#INITSEG(0x9000)�����Ѿ���bootsect ������
				! ���ù�������������setup ����Linus ������Ҫ������
				! ����һ�¡�
	mov ds,ax
	mov ah,#0x03 				! read cursor pos
				! BIOS �ж�0x10 �Ķ���깦�ܺ� ah = 0x03
				! ���룺bh = ҳ��
				! ���أ�ch = ɨ�迪ʼ�ߣ�cl = ɨ������ߣ�
				! dh = �к�(0x00 �Ƕ���)��dl = �к�(0x00 �����)��
	xor bh,bh
	int 0x10 		! save it in known place, con_init fetches
	mov [0],dx 		! it from 0x90000.
				! ��������˵�����λ����Ϣ�����0x90000 ��������̨
				! ��ʼ��ʱ����ȡ��

! Get memory size (extended mem, kB) 	
				! ����3 ��ȡ��չ�ڴ�Ĵ�Сֵ��KB����
				! ���� BIOS �ж� 0x15 ���ܺ� ah    0x88 ȡϵͳ������չ�ڴ��С���������ڴ�0x90002 ����
				! ���أ�ax  �� 0x100000��1M������ʼ����չ�ڴ��С(KB)���������� CF ��λ��ax  �����롣

	mov ah,#0x88
	int 0x15
	mov [2],ax 				! ����չ�ڴ���ֵ����0x90002 ����1 ���֣���

! Get video-card data: 				! �����������ȡ��ʾ����ǰ��ʾģʽ��
				! ����BIOS �ж�0x10�����ܺ� ah = 0x0f
				! ���أ�ah = �ַ�������al = ��ʾģʽ��bh = ��ǰ��ʾҳ��
				! 0x90004(1 ��)��ŵ�ǰҳ��0x90006 ��ʾģʽ��0x90007 �ַ�������

	mov ah,#0x0f
	int 0x10
	mov [4],bx 		! bh = display page
	mov [6],ax 		! al = video mode, ah = window width

! check for EGA/VGA and some config parameters 	! �����ʾ��ʽ��EGA/VGA����ȡ������
				! �����ʾ��ʽ��EGA/VGA����ȡ������
				! ����BIOS �ж�0x10�����ӹ���ѡ�� -ȡ��ʽ��Ϣ
				! ���ܺţ�ah = 0x12��bl = 0x10
				! ���أ�bh = ��ʾ״̬
				! (0x00 - ��ɫģʽ��I/O �˿�=0x3dX)
				! (0x01 - ��ɫģʽ��I/O �˿�=0x3bX)
				! bl = ��װ����ʾ�ڴ�
				! (0x00 - 64k, 0x01 - 128k, 0x02 - 192k, 0x03 = 256k)
				! cx = ��ʾ�����Բ���(�μ�������˵��)��

	mov ah,#0x12
	mov bl,#0x10
	int 0x10
	mov [8],ax 		! 0x90008 = ??
	mov [10],bx 	! 0x9000A = ��װ����ʾ�ڴ棬0x9000B = ��ʾ״̬(��ɫ/��ɫ)
	mov [12],cx 	! 0x9000C = ��ʾ�����Բ�����

! Get hd0 data 				! ȡ��һ��Ӳ�̵���Ϣ������Ӳ�̲�������
				! ��1 ��Ӳ�̲�������׵�ַ��Ȼ���ж�����0x41 ������ֵ������2 ��Ӳ��
				! ��������ӵ�1 ����ĺ��棬�ж�����0x46 ������ֵҲָ�����2 ��Ӳ��
				! �Ĳ�������ַ����ĳ�����16 ���ֽ�(0x10)��
				! �������γ���ֱ���BIOS �й�����Ӳ�̵Ĳ�����0x90080 ����ŵ�1 ��
				! Ӳ�̵ı�0x90090 ����ŵ�2 ��Ӳ�̵ı�
!��69�������ڴ�ָ��λ�ô���ȡһ����ָ��ֵ������ds��si�Ĵ����С�ds�зŶε�ַ��si��
!����ƫ�Ƶ�ַ�������ǰ��ڴ��ַ4 * 0x41(= 0x104)�������4���ֽڣ��κ�ƫ��ֵ��������
	mov ax,#0x0000
	mov ds,ax
	lds si,[4*0x41] 			! ȡ�ж�����0x41 ��ֵ��Ҳ��hd0 ������ĵ�ַ���� ds:si
	mov ax,#INITSEG
	mov es,ax
	mov di,#0x0080 				! �����Ŀ�ĵ�ַ: 0x9000:0x0080 ���� es:di
	mov cx,#0x10 				! ������0x10 �ֽڡ�
	rep
	movsb

! Get hd1 data

	mov ax,#0x0000
	mov ds,ax
	lds si,[4*0x46] 			! ȡ�ж�����0x46 ��ֵ��Ҳ��hd1 ������ĵ�ַ����>ds:si
	mov ax,#INITSEG
	mov es,ax
	mov di,#0x0090 				! �����Ŀ�ĵ�ַ: 0x9000:0x0090 ����> es:di
	mov cx,#0x10
	rep
	movsb

! Check that there IS a hd1 :-) 
				! ���ϵͳ�Ƿ���ڵ�2 ��Ӳ�̣�������������2 �������㡣
				! ����BIOS �жϵ���0x13 ��ȡ�����͹��ܡ�
				! ���ܺ� ah = 0x15��
				! ���룺dl = �������ţ�0x8X ��Ӳ�̣�0x80 ָ��1 ��Ӳ�̣�0x81 ��2 ��Ӳ�̣�
				! �����ah = �����룻00 --û������̣�CF ��λ�� 01 --��������û��change-line ֧�֣�
				! 02 --������(���������ƶ��豸)����change-line ֧�֣� 03 --��Ӳ�̡�

	mov ax,#0x01500
	mov dl,#0x81
	int 0x13
	jc no_disk1
	cmp ah,#3 					! ��Ӳ����(���� = 3 ��)��
	je is_disk1
no_disk1:
	mov ax,#INITSEG 			! ��2 ��Ӳ�̲����ڣ���Ե�2 ��Ӳ�̱����㡣
	mov es,ax
	mov di,#0x0090
	mov cx,#0x10
	mov ax,#0x00
	rep
	stosb
is_disk1:

! now we want to move to protected mode ... ! �����￪ʼ����Ҫ����ģʽ����Ĺ����ˡ�

	cli 		! no interrupts allowed 	! ��ʱ�������жϡ�

				! first we move the system to it's rightful place
				! �������ǽ�system ģ���Ƶ���ȷ��λ�á�
				! bootsect ���������ǽ�system ģ����뵽��0x10000��64k����ʼ��λ�á����ڵ�ʱ����
				! system ģ����󳤶Ȳ��ᳬ��0x80000��512k����Ҳ����ĩ�˲��ᳬ���ڴ��ַ0x90000��
				! ����bootsect �Ὣ�Լ��ƶ���0x90000 ��ʼ�ĵط�������setup ���ص����ĺ��档
				! ������γ������;���ٰ�����system ģ���ƶ���0x00000 λ�ã����Ѵ�0x10000 ��0x8ffff
				! ���ڴ����ݿ�(512k)����������ڴ�Ͷ��ƶ���0x10000��64k����λ�á�

	mov ax,#0x0000
	cld 						! 'direction'=0, movs moves forward
do_move:
	mov es,ax 					! destination segment 	! es:di����>Ŀ�ĵ�ַ(��ʼΪ0x0000:0x0)
	add ax,#0x1000
	cmp ax,#0x9000 				! �Ѿ��Ѵ�0x8000 �ο�ʼ��64k �����ƶ��ꣿ
	jz end_move					! ������ת��
	mov ds,ax 					! source segment 	! ds:si����>Դ��ַ(��ʼΪ0x1000:0x0)
	sub di,di
	sub si,si
	mov cx,#0x8000 				! �ƶ�0x8000 �֣�64k �ֽڣ���
	rep
	movsw
	jmp do_move

! then we load the segment descriptors
				! �˺����Ǽ��ض���������
				! �����￪ʼ������32 λ����ģʽ�Ĳ����������ҪIntel 32 λ����ģʽ��̷����֪ʶ��,
				! �й��ⷽ�����Ϣ������б��ļ򵥽��ܻ�¼�е���ϸ˵�������������Ҫ˵����
				!
				! lidt ָ�����ڼ����ж���������(idt)�Ĵ��������Ĳ�������6 ���ֽڣ�0-1 �ֽ������������
				! ����ֵ(�ֽ�)��2-5 �ֽ������������32 λ���Ի���ַ���׵�ַ��������ʽ�μ�����
				! 219-220 �к�223-224 �е�˵�����ж����������е�ÿһ�����8 �ֽڣ�ָ�������ж�ʱ
				! ��Ҫ���õĴ������Ϣ�����ж�������Щ���ƣ���Ҫ�����������Ϣ��
				!
				! lgdt ָ�����ڼ���ȫ����������(gdt)�Ĵ��������������ʽ��lidt ָ�����ͬ��ȫ��������
				! ���е�ÿ����������(8 �ֽ�)�����˱���ģʽ�����ݺʹ���Σ��飩����Ϣ�����а����ε�
				! ��󳤶�����(16 λ)���ε����Ի�ַ��32 λ�����ε���Ȩ�������Ƿ����ڴ桢��д����Լ�
				! ����һЩ����ģʽ���еı�־���μ�����205-216 �С�
				!

end_move:
	mov ax,#SETUPSEG 				! right, forgot this at first. didn't work :-)
	mov ds,ax 				! ds ָ�򱾳���(setup)�Ρ�
	lidt idt_48 				! load idt with 0,0
				! �����ж���������(idt)�Ĵ�����idt_48 ��6 �ֽڲ�������λ��
				! (��218 ��)��ǰ2 �ֽڱ�ʾidt ����޳�����4 �ֽڱ�ʾidt ��
				! �����Ļ���ַ��
	lgdt gdt_48 				! load gdt with whatever appropriate
				! ����ȫ����������(gdt)�Ĵ�����gdt_48 ��6 �ֽڲ�������λ��
				! (��222 ��)��

! that was painless, now we enable A20
				! ���ϵĲ����ܼ򵥣��������ǿ���A20��ַ�ߡ�
				! Ϊ���ܹ����ʺ�ʹ��1MB���ϵ������ڴ棬������Ҫ���ȿ���A20��ַ�ߡ��μ��������б��
				! �й�A20�ź��ߵ�˵�����������漰��һЩ�˿ں�����ɲο�kernel/chr��drv/keyboard.S
				! �����Լ��̽ӿڵ�˵�������ڻ����Ƿ����������� A20��ַ�ߣ����ǻ���Ҫ�ڽ��뱣��ģʽ 
				! ֮���ܷ���1MB�����ڴ�֮���ڲ���һ�¡�������������� head.S�����У�32��36�У���


	call empty_8042 				! �ȴ����뻺�����ա�
				! ֻ�е����뻺����Ϊ��ʱ�ſ��Զ������д���
	mov al,#0xD1 				! command write 				! 0xD1 ������-��ʾҪд���ݵ�
	out #0x64,al 				! 8042 ��P2 �˿ڡ�P2 �˿ڵ�λ1 ����A20 �ߵ�ѡͨ��
				! ����Ҫд��0x60 �ڡ�
	call empty_8042 				! �ȴ����뻺�����գ��������Ƿ񱻽��ܡ�
	mov al,#0xDF 				! A20 on 				! ѡͨA20 ��ַ�ߵĲ�����
	out #0x60,al
	call empty_8042 				! ���뻺����Ϊ�գ����ʾA20 ���Ѿ�ѡͨ��

				! well, that went ok, I hope. Now we have to reprogram the interrupts :-(
				! we put them right after the intel-reserved hardware interrupts, at
				! int 0x20-0x2F. There they won't mess up anything. Sadly IBM really
				! messed this up with the original PC, and they haven't been able to
				! rectify it afterwards. Thus the bios puts interrupts at 0x08-0x0f,
				! which is used for the internal hardware interrupts as well. We just
				! have to reprogram the 8259's, and it isn't fun.
				! ϣ������һ���������������Ǳ������¶��жϽ��б�̣�-(���ǽ����Ƿ�������
				! ����Intel������Ӳ���жϺ��棬��int 0x20-0x2F�����������ǲ��������ͻ��
				! ���ҵ���IBM��ԭPC���и����ˣ��Ժ�Ҳû�о�������������PC��BIOS���ж� 
				! ������ 0x08-0x0f����Щ�ж�Ҳ�������ڲ�Ӳ���жϡ��������Ǿͱ������¶�8259 
				! �жϿ��������б�̣���һ�㶼û��˼��

				! 	PC��ʹ��2��8259AоƬ�����ڶԿɱ�̿�����8259AоƬ�ı�̷�����μ��������Ľ��ܡ�
				! ��156���϶���������֣�OxOOeb����ֱ��ʹ�û������ʾ�����������תָ�����ʱ���á�
				! 	Oxeb��ֱ�ӽ���תָ��Ĳ����룬��1���ֽڵ����λ��ֵ�������ת��Χ��-127��127"CPU 
				! ͨ����������λ��ֵ�ӵ�EIP�Ĵ����о��γ�һ���µ���Ч��ַ����ʱEIPָ����һ����ִ�� 
				! ��ָ�ִ��ʱ�����ѵ�croʱ����������7��10����OxOOeb��ʾ��תֵ��0��һ��ָ��� 
				! �˻���ֱ��ִ����һ��ָ�������ָ����ṩ14һ20��CPUʱ�����ڵ��ӳ�ʱ�䡣��as86 
				! ��û�б�ʾ��Ӧָ������Ƿ������Linus��setup.s��һЩ�������о�ֱ��ʹ�û��������� 
				! ʾ����ָ����⣬ÿ���ղ���ָ��N0P��ʱ����������3���������Ҫ�ﵽ��ͬ���ӳ�Ч���� 
				! ��Ҫ6��7��N0Pָ�

	mov		al,#0x11 				! initialization sequence
									! 0x11 ��ʾ��ʼ�����ʼ����ICW1 �����֣���ʾ��
									! �ش�������Ƭ8259 ���������Ҫ����ICW4 �����֡�
	out		#0x20,al 				! send it to 8259A-1 			! ���͵�8259A ��оƬ��
	.word	0x00eb,0x00eb 			! jmp $+2, jmp $+2 				! $ ��ʾ��ǰָ��ĵ�ַ��
									! ������תָ�������һ��ָ�����ʱ���á�
	out		#0xA0,al 				! and to 8259A-2 				! �ٷ��͵�8259A ��оƬ��
	.word	0x00eb,0x00eb
	mov		al,#0x20 				! start of hardware int's (0x20)
	out		#0x21,al 				! ����оƬICW2 �����֣���ʼ�жϺţ�Ҫ�����ַ��
	.word	0x00eb,0x00eb
	mov		al,#0x28 				! start of hardware int's 2 (0x28)
	out		#0xA1,al 				! �ʹ�оƬICW2 �����֣���оƬ����ʼ�жϺš�
	.word	0x00eb,0x00eb
	mov		al,#0x04 				! 8259-1 is master
	out		#0x21,al 				! ����оƬICW3 �����֣���оƬ��IR2 ����оƬINT��
	.word	0x00eb,0x00eb 				!�μ������б���˵����
	mov		al,#0x02 				! 8259-2 is slave
	out		#0xA1,al 				! �ʹ�оƬICW3 �����֣���ʾ��оƬ��INT ������о
									! Ƭ��IR2 �����ϡ�
	.word	0x00eb,0x00eb
	mov		al,#0x01 				! 8086 mode for both
	out		#0x21,al 				! ����оƬICW4 �����֡�8086 ģʽ����ͨEOI ��ʽ��
									! �跢��ָ������λ����ʼ��������оƬ������
	.word	0x00eb,0x00eb
	out		#0xA1,al 				!�ʹ�оƬICW4 �����֣�����ͬ�ϡ�
	.word	0x00eb,0x00eb
	mov		al,#0xFF 				! mask off all interrupts for now
	out		#0x21,al 				! ������оƬ�����ж�����
	.word	0x00eb,0x00eb
	out		#0xA1,al 				!���δ�оƬ�����ж�����

				! well, that certainly wasn't fun :-(. Hopefully it works, and we don't
				! need no steenking BIOS anyway (except for the initial loading :-).
				! The BIOS-routine wants lots of unnecessary data, and it's less
				! "interesting" anyway. This is how REAL programmers do it.
				!
				! Well, now's the time to actually move into protected mode. To make
				! things as simple as possible, we do no register set-up or anything,
				! we let the gnu-compiled 32-bit programs do that. We just jump to
				! absolute address 0x00000, in 32-bit protected mode.
				
				! �ߣ�������α�̵�Ȼû��:-(����ϣ�������ܹ�������������Ҳ������Ҫ��ζ��BIOS
				! �ˣ����˳�ʼ����:-����BIOS�ӳ���Ҫ��ܶ಻��Ҫ�����ݣ�������һ�㶼ûȤ������
				! "����"�ĳ���Ա�������¡� 
				! ���ˣ�������������ʼ���뱣��ģʽ��ʱ���ˡ�Ϊ�˰��������þ����򵥣����ǲ����� 
				! �Ĵ������ݽ����κ����á�������gnu�����32λ����ȥ������Щ�¡��ڽ���32λ�� 
				! ��ģʽʱ���ǽ��Ǽ򵥵���ת�����Ե�ַ0x00000����
				! �������ò�����32λ����ģʽ���С����ȼ��ػ���״̬�֣�lmsw-Load Machine Status Word����
				! Ҳ�ƿ��ƼĴ���CR0�������λ0��1������CPU�л�������ģʽ��������������Ȩ��0�У���
				! ��ǰ��Ȩ��CPL=0����ʱ�μĴ�����Ȼָ����ʵ��ַģʽ����ͬ�����Ե�ַ������ʵ��ַģʽ�� 
				! ���Ե�ַ�������ڴ��ַ��ͬ���������øñ���λ�����һ��ָ�������һ���μ���תָ���� 
				! ����ˢ��CPU��ǰָ����С���ΪCPU����ִ��һ��ָ��֮ǰ���Ѵ��ڴ��ȡ��ָ�������� 
				! ���롣Ȼ���ڽ��뱣��ģʽ�Ժ���Щ����ʵģʽ��Ԥ��ȡ�õ�ָ����Ϣ�ͱ�ò�����Ч����һ�� 
				! �μ���תָ��ͻ�ˢ��CPU�ĵ�ǰָ����У���������Щ��Ч��Ϣ�����⣬��Intel��˾���ֲ� 
				! �Ͻ���80386������CPUӦ��ʹ��ָ��"mov cr0��ax"�л�������ģʽ��lmswָ������ڼ����� 
				! ǰ�� 286 CPU��

	mov ax,#0x0001 			! protected mode (PE) bit 			! ����ģʽ����λ(PE)��
	lmsw ax 				! This is it						! ���������ػ���״̬��				!
	jmpi 0,8 				! jmp offset 0 of segment 8 (cs)	! ��ת��cs ��8��ƫ��0 ����
				! �����Ѿ���system ģ���ƶ���0x00000 ��ʼ�ĵط������������ƫ�Ƶ�ַ��0������Ķ�
				! ֵ��8 �Ѿ��Ǳ���ģʽ�µĶ�ѡ����ˣ�����ѡ����������������������Լ���Ҫ�����Ȩ����
				! ��ѡ�������Ϊ16 λ��2 �ֽڣ���λ0-1 ��ʾ�������Ȩ��0-3��linux ����ϵͳֻ
				! �õ�������0 ����ϵͳ������3 �����û�������λ2 ����ѡ��ȫ����������(0)���Ǿֲ���
				! ������(1)��λ3-15 �������������������ָ��ѡ��ڼ��������������Զ�ѡ���
				! 8(0b0000,0000,0000,1000)��ʾ������Ȩ��0��ʹ��ȫ�����������еĵ�1 �����ָ��
				! ����Ļ���ַ��0���μ�209 �У�������������תָ��ͻ�ȥִ��system �еĴ��롣

				! This routine checks that the keyboard command queue is empty
				! No timeout is used - if this hangs there is something wrong with
				! the machine, and we probably couldn't proceed anyway.
				! ��������ӳ����������������Ƿ�Ϊ�ա����ﲻʹ�ó�ʱ���� - �������������
				! ��˵��PC �������⣬���Ǿ�û�а취�ٴ�����ȥ�ˡ�
				! ֻ�е����뻺����Ϊ��ʱ��״̬�Ĵ���λ2 = 0���ſ��Զ������д���
empty_8042:
	.word 0x00eb,0x00eb 		! ����������תָ��Ļ�����(��ת����һ��)���൱����ʱ�ղ�����
	in al,#0x64 				! 8042 status port 			! ��AT ���̿�����״̬�Ĵ�����
	test al,#2 					! is input buffer full? 	! ����λ2�����뻺��������
	jnz empty_8042 				! yes - loop
	ret

gdt: 			! ȫ����������ʼ�������������ɶ��8 �ֽڳ�������������ɡ�
				! ���������3 �����������1 �����ã�206 �У���������ڡ���2 ����ϵͳ�����
				! ��������208-211 �У�����3 ����ϵͳ���ݶ�������(213-216 ��)��ÿ���������ľ���
				! ����μ��б��˵����
	.word 0,0,0,0 				! dummy 				! ��1 �������������á�
				! ������gdt ���е�ƫ����Ϊ0x08�������ش���μĴ���(��ѡ���)ʱ��ʹ�õ������ƫ��ֵ��
	.word 0x07FF 				! 8Mb - limit=2047 (2048*4096=8Mb)
	.word 0x0000 				! base address=0
	.word 0x9A00 				! code read/exec
	.word 0x00C0 				! granularity=4096, 386
				! ������gdt ���е�ƫ������0x10�����������ݶμĴ���(��ds ��)ʱ��ʹ�õ������ƫ��ֵ��
	.word 0x07FF 				! 8Mb - limit=2047 (2048*4096=8Mb)
	.word 0x0000 				! base address=0
	.word 0x9200 				! data read/write
	.word 0x00C0 				! granularity=4096, 386

! �����Ǽ����ж���������Ĵ���idtr��ָ��lidtҪ���6�ֽڲ�������ǰ2�ֽ���IDT����޳���
! ��4�ֽ���idt�������Ե�ַ�ռ��е�32λ����ַ��CPUҪ���ڽ��뱣��ģʽ֮ǰ������IDT��
! �������������һ������Ϊ0�Ŀձ�

idt_48:
	.word 0 					! idt limit=0
	.word 0,0 					! idt base=0L
	
! ���Ǽ���ȫ����������Ĵ���gdtr��ָ��lgdtҪ���6�ֽڲ�������ǰ2�ֽ���gdt����޳���
! ��4�ֽ���gdt������Ի���ַ������ȫ�ֱ�������Ϊ2KB(0x7ff����)����Ϊÿ8�ֽ���� 
! һ��������������Ա��й�����256�4�ֽڵ����Ի���ַΪ0x0009"16 + 0x0200 + gdt,
! ��0x90200 + gdt��������gdt��ȫ�ֱ��ڱ�������е�ƫ�Ƶ�ַ����205�С���

gdt_48:
	.word 0x800 				! gdt limit=2048, 256 GDT entries
				! ȫ�ֱ���Ϊ2k �ֽڣ���Ϊÿ8 �ֽ����һ������������
				! ���Ա��й�����256 �
	.word 512+gdt,0x9 			! gdt base = 0X9xxxx
				! 4 ���ֽڹ��ɵ��ڴ����Ե�ַ��0x0009<<16 + 0x0200+gdt
				! Ҳ��0x90200 + gdt(���ڱ�������е�ƫ�Ƶ�ַ��205 ��)��

.text
endtext:
.data
enddata:
.bss
endbss:
