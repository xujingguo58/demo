/*
*  linux/kernel/asm.s
*
*  (C) 1991  Linus Torvalds
*/

/*
* asm.s contains the low-level code for most hardware faults.
* page_exception is handled by the mm, so that isn't here. This
* file also handles (hopefully) fpu-exceptions due to TS-bit, as
* the fpu must be properly saved/resored. This hasn't been tested.
*/
/*
* asm.s �����а����󲿷ֵ�Ӳ�����ϣ����������ĵײ�δ��롣ҳ�쳣�����ڴ�������
* mm ����ģ����Բ������	�˳��򻹴���ϣ��������������TS-λ����ɵ�fpu �쳣��
* ��Ϊfpu ������ȷ�ؽ��б���/�ָ�������Щ��û�в��Թ���	
*/

/* �������ļ���Ҫ�漰��Intel �������ж�int0--int16 �Ĵ���int17-int31 �������ʹ�ã���	*/
/* ������һЩȫ�ֺ���������������ԭ����traps.c ��˵����	*/
.globl _divide_error,_debug,_nmi,_int3,_overflow,_bounds,_invalid_op
.globl _double_fault,_coprocessor_segment_overrun
.globl _invalid_TSS,_segment_not_present,_stack_segment
.globl _general_protection,_coprocessor_error,_irq13,_reserved

/*������γ������޳���ŵ�������μ�ͼ8-4(a)��
* int0 -��������������������ͣ�����Fault��:����ţ��ޡ�
* ��ִ��DIV��IDIVָ��ʱ����������0��CPU�ͻ��������쳣����EAX (��AX��AL)���� 
* ����һ���Ϸ��������Ľ��ʱҲ���������쳣��20���ϱ�š���do��divide��error��ʵ������
* C���Ժ���do_divide_error()�����������ģ���ж�Ӧ�����ơ�������do��divide��error����
* traps.c��ʵ�֣���97�п�ʼ����
*/
_divide_error:
	pushl $_do_divide_error			/* ���Ȱѽ�Ҫ���õĺ�����ַ��ջ��	��γ���ĳ����Ϊ0��	*/
no_error_code:						/* �������޳���Ŵ������ڴ����������55 �еȡ�		*/
	xchgl %eax,(%esp)			/* _do_divide_error �ĵ�ַ ����> eax��eax ��������ջ��	*/
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	pushl %ebp
	push %ds					/* 16 λ�ĶμĴ�����ջ��ҲҪռ��4 ���ֽڡ�	*/
	push %es
	push %fs
	pushl $0					/* "error code"		����������ջ��	*/
	lea 44(%esp),%edx		/* ȡԭ���÷��ص�ַ����ջָ��λ�ã���ѹ���ջ��	*/
	pushl %edx
	movl $0x10,%edx			/* ��ʼ���μĴ���ds��es��fs�������ں����ݶ�ѡ�����	*/
	mov %dx,%ds
	mov %dx,%es
	mov %dx,%fs

/* �����ϵĺű�ʾ���ò�����ָ����ַ���ĺ�������Ϊ��ӵ��á����ĺ����ǵ������𱾴� 
* �쳣��C������������do_divide_errorO�ȡ���40���ǽ���ջָ���8�൱��ִ������pop 
* ������������������������ջ������C����������32�к�34����ջ��ֵ�����ö�ջָ������ 
* ָ��Ĵ���fs��ջ����
*/
	call *%eax				/* ����C ����do_divide_error()��	*/
	addl $8,%esp				/* �ö�ջָ������ָ��Ĵ���fs ��ջ����	*/
	pop %fs
	pop %es
	pop %ds
	popl %ebp
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax				/* ����ԭ��eax �е����ݡ�	*/
	iret

/* int1 --debug�����ж���ڵ㡣�������ͬ�ϡ����ͣ�����/���壨Fault/Trap��:����ţ��ޡ� 
* ��EFLAGS��TF��־��λʱ���������жϡ�������Ӳ���ϵ㣨���ݣ����壬���룺���󣩣����� 
* ������ָ�������������񽻻����壬���ߵ��ԼĴ���������Ч�����󣩣�CPU�ͻ�������쳣��
*/
_debug:
	pushl $_do_int3			/* _do_debug C ����ָ����ջ��	*/
	jmp no_error_code

/* int2 -- �������жϵ�����ڵ㡣���ͣ�����:�޴���š�
* ���ǽ��еı�����̶��ж�������Ӳ���жϡ�ÿ�����յ�һ��NMI�źţ�CPU�ڲ��ͻ�����ж� 
* ����2����ִ�б�׼�ж�Ӧ�����ڣ���˺ܽ�ʡʱ�䡣��Iͨ������Ϊ��Ϊ��Ҫ��Ӳ���¼�ʹ�á�
* ��CPU�յ�һ��NMI�źŲ��ҿ�ʼִ�����жϴ������ʱ��������е�Ӳ���ж϶��������ԡ�
*/
_nmi:
	pushl $_do_nmi
	jmp no_error_code

/* int3 -- ָ���������жϣ���Ӳ���ж��޹ء���ָ��ͨ���ɵ�ʽ�����뱻��ʽ����Ĵ����С��������ͬ��debug��*/
_int3:
	pushl $_do_int3
	jmp no_error_code

/* int4 -- ����������ж���ڵ㡣���ͣ�����:�޴���š�
* EFLAGS��OF��־��λʱCPUִ��INTOָ��ͻ��������жϡ�ͨ�����ڱ����������������������
*/
_overflow:
	pushl $_do_overflow
	jmp no_error_code

/* int5 -- �߽�������ж���ڵ㡣���ͣ������޴���š�
* ������������Ч��Χ����ʱ�������жϡ���BOUNDָ�����ʧ�ܾͻ�������жϡ�BOUNDָ����
* 3���������������1��������������֮�䣬�Ͳ����쳣5��
*/
_bounds:
	pushl $_do_bounds
	jmp no_error_code

/* int6 -- ��Ч����ָ������ж���ڵ㡣���ͣ������޴���š�
* CPUִ�л�����⵽һ����Ч�Ĳ������������жϡ�
*/
_invalid_op:
	pushl $_do_invalid_op
	jmp no_error_code

/* int9 -- Э�������γ��������ж���ڵ㡣���ͣ��������޴���š�
* ���쳣�����ϵ�ͬ��Э����������������Ϊ�ڸ���ָ�������̫��ʱ�����Ǿ������������ 
* ���ػ򱣴泬�����ݶεĸ���ֵ��
*/
_coprocessor_segment_overrun:
	pushl $_do_coprocessor_segment_overrun
	jmp no_error_code

/* int15 �C ������	*/
_reserved:
	pushl $_do_reserved
	jmp no_error_code

/* int45 -- (= 0x20 + 13) Linux���õ���ѧЭ������Ӳ���жϡ�
* ��Э������ִ����һ������ʱ�ͻᷢ��IRQ13�ж��źţ���֪ͨCPU������ɡ�80387��ִ�� 
* ����ʱ��CPU��ȴ��������ɡ�����88����OxTO��Э����˿ڣ�������æ��������ͨ��д 
* �ö˿ڣ����жϽ�����CPU��BUSY�����źţ������¼���80387�Ĵ�������չ��������PEREQ��
* �ò�����Ҫ��Ϊ��ȷ���ڼ���ִ��80387���κ�ָ��֮ǰ��CPU��Ӧ���жϡ�
*/
/* ��Э������ִ����һ������ʱ�ͻᷢ��IRQ13 �ж��źţ���֪ͨCPU ������ɡ�	*/
_irq13:
	pushl %eax
	xorb %al,%al
	outb %al,$0xF0
	movb $0x20,%al
	outb %al,$0x20			/* ��8259 ���жϿ���оƬ����EOI���жϽ������źš�	*/
	jmp 1f					/* ��������תָ������ʱ���á�	*/
1:	jmp 1f
1:	outb %al,$0xA0			/* ����8259 ���жϿ���оƬ����EOI���жϽ������źš�	*/
	popl %eax
	jmp _coprocessor_error	/* _coprocessor_error ԭ���ڱ��ļ��У������Ѿ��ŵ�5.3 asm.s ����	*/

/* �����ж��ڵ���ʱCPU�����жϷ��ص�ַ֮�󽫳����ѹ���ջ����˷���ʱҲ��Ҫ������ŵ������μ�ͼ5.3(b)����
* int8 ��˫������ϡ����ͣ��������д����롣
* ͨ����CPU�ڵ���ǰһ���쳣�Ĵ��������ּ�⵽һ���µ��쳣ʱ���������쳣�ᱻ���еؽ���
* ������Ҳ���������ٵ������CPU���ܽ��������Ĵ��д����������ʱ�ͻ��������жϡ�
*/
_double_fault:
	pushl $_do_double_fault
error_code:
	xchgl %eax,4(%esp)		/* error code <->	%eax��eax ԭ����ֵ�������ڶ�ջ�ϡ�	*/
	xchgl %ebx,(%esp)			/* &function <->	%ebx��ebx ԭ����ֵ�������ڶ�ջ�ϡ�	*/
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	pushl %ebp
	push %ds
	push %es
	push %fs
	pushl %eax				/* error code	 �������ջ��	*/
	lea 44(%esp),%eax		/* offset ���򷵻ص�ַ����ջָ��λ��ֵ��ջ��	*/
	pushl %eax
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	call *%ebx
	addl $8,%esp
	pop %fs
	pop %es
	pop %ds
	popl %ebp
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret

/* int10 -- ��Ч������״̬��(TSS)�� ���ͣ������г����롣
* CPU��ͼ�л���һ�����̣����ý��̵�TSS��Ч������TSS����һ�����������쳣��������TSS 
* ���ȳ���104�ֽ�ʱ������쳣�ڵ�ǰ�����в���������л�����ֹ������������ᵼ�����л� 
* ����������в������쳣��
*/
_invalid_TSS:
	pushl $_do_invalid_TSS
	jmp error_code

/* int11 -- �β����ڡ����ͣ������г����롣
* �����õĶβ����ڴ��С����������б�־�Ŷβ����ڴ��С�
*/
_segment_not_present:
	pushl $_do_segment_not_present
	jmp error_code

/* int12 -- ��ջ�δ������ͣ������г����롣
* ָ�������ͼ������ջ�η�Χ�����߶�ջ�β����ڴ��С������쳣11��13����������Щ���� 
* ϵͳ������������쳣��ȷ��ʲôʱ��Ӧ��Ϊ�����������ջ�ռ䡣
*/
_stack_segment:
	pushl $_do_stack_segment
	jmp error_code

/* int13 -- һ�㱣���Գ������ͣ������г����롣
* �����ǲ������κ�������Ĵ�����һ���쳣����ʱû�ж�Ӧ�Ĵ���������0-16����ͨ���ͻ�鵽���ࡣ
*/
_general_protection:
	pushl $_do_general_protection
	jmp error_code

/* int7 -- �豸������(_device_not_available)��(kernel/system_call.s,148)
* int14 -- ҳ����(_page_fault)��(mm/page.s,14)
* int16 -- Э����������(_coprocessor_error)��(kernel/system_call.s,131)
* ʱ���ж�int 0x20 (_timer_interrupt)��(kernel/system_call.s,176)
* ϵͳ����int 0x80 (_system_call)�ڣ�kernel/system_call.s,80��
*/
