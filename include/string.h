#ifndef _STRING_H_
#define _STRING_H_

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

extern char *strerror (int errno);

/*
* This string-include defines all string functions as inline
* functions. Use gcc. It also assumes ds=es=data space, this should be
* normal. Most of the string-functions are rather heavily hand-optimized,
* see especially strtok,strstr,str[c]spn. They should work, but are not
* very easy to understand. Everything is done entirely within the register
* set, making the functions fast and clean. String instructions have been
* used through-out, making for "slightly" unclear code :-)
*
* (C) 1991 Linus Torvalds
*/
/*
* ����ַ���ͷ�ļ�����Ƕ��������ʽ�����������ַ�������������ʹ��gcc ʱ��ͬʱ
* �ٶ���ds=es=���ݿռ䣬��Ӧ���ǳ���ġ���������ַ����������Ǿ��ֹ����д���
* �Ż��ģ������Ǻ���strtok��strstr��str[c]spn������Ӧ����������������ȴ������
* ô������⡣���еĲ��������϶���ʹ�üĴ���������ɵģ���ʹ�ú������������ࡣ
* ���еط���ʹ�����ַ���ָ�����ʹ�ô��롰��΢���������?
*
* (C) 1991 Linus Torvalds
*/

/* ��һ���ַ���(src)��������һ���ַ���(dest)��ֱ������NULL �ַ���ֹͣ��	*/
/* ������dest - Ŀ���ַ���ָ�룬src - Դ�ַ���ָ�롣	*/
/* %0 - esi(src)��%1 - edi(dest)��	*/
static inline char * strcpy(char * dest,const char *src)
{
/*	__asm__("cld\n"
			"1:\tlodsb\n\t"
			"stosb\n\t"
			"testb %%al,%%al\n\t"
			"jne 1b"
			::"S" (src),"D" (dest):) ;
*/
/* �巽��λ��							*/	__asm__("cld\n"
/* ����DS:[esi]��1 �ֽڡ���>al��������esi��	*/		"1:\tlodsb\n\t"
/* �洢�ֽ�al����>ES:[edi]��������edi��		*/		"stosb\n\t"
/* �մ洢���ֽ���0��						*/			"testb %%al,%%al\n\t"
/* �����������ת�����1 �������������		*/			"jne 1b"
/* ����Ŀ���ַ���ָ�롣						*/			::"S" (src),"D" (dest):) ;
	return dest;
}
/* �巽��λ��								*/
/* ����DS:[esi]��1 �ֽڡ���>al��������esi��	*/
/* �洢�ֽ�al����>ES:[edi]��������edi��		*/
/* �մ洢���ֽ���0��						*/
/* �����������ת�����1 �������������		*/
/* ����Ŀ���ַ���ָ�롣						*/




/* ����Դ�ַ���count ���ֽڵ�Ŀ���ַ�����	*/
/* ���Դ������С��count ���ֽڣ��͸��ӿ��ַ�(NULL)��Ŀ���ַ�����	*/
/* ������dest - Ŀ���ַ���ָ�룬src - Դ�ַ���ָ�룬count - �����ֽ�����	*/
/* %0 - esi(src)��%1 - edi(dest)��%2 - ecx(count)��	*/
static inline char * strncpy(char * dest,const char *src,int count)
{
	__asm__("cld\n"
			"1:\tdecl %2\n\t"
			"js 2f\n\t"
			"lodsb\n\t"
			"stosb\n\t"
			"testb %%al,%%al\n\t"
			"jne 1b\n\t"
			"rep\n\t"
			"stosb\n"
			"2:"
			::"S" (src),"D" (dest),"c" (count):) ;
	return dest;
}
/* �巽��λ��	*/
/* �Ĵ���ecx--��count--����	*/
/* ���count<0 ����ǰ��ת�����2��������	*/
/* ȡds:[esi]��1 �ֽڡ���>al������esi++��	*/
/* �洢���ֽڡ���>es:[edi]������edi++��	*/
/* ���ֽ���0��	*/
/* ���ǣ�����ǰ��ת�����1 ������������	*/
/* ������Ŀ�Ĵ��д��ʣ������Ŀ��ַ���	*/
/* ����Ŀ���ַ���ָ�롣	*/


/* ��Դ�ַ���������Ŀ���ַ�����ĩβ����	*/
/* ������dest - Ŀ���ַ���ָ�룬src - Դ�ַ���ָ�롣	*/
/* %0 - esi(src)��%1 - edi(dest)��%2 - eax(0)��%3 - ecx(-1)��	*/
static inline char * strcat(char * dest,const char * src)
{
	__asm__("cld\n\t"
			"repne\n\t"
			"scasb\n\t"
			"decl %1\n"
			"1:\tlodsb\n\t"
			"stosb\n\t"
			"testb %%al,%%al\n\t"
			"jne 1b"
			::"S" (src),"D" (dest),"a" (0),"c" (0xffffffff):) ;
	return dest;
}
/* �巽��λ��	*/
/* �Ƚ�al ��es:[edi]�ֽڣ�������edi++��	*/
/* ֱ���ҵ�Ŀ�Ĵ�����0 ���ֽڣ���ʱedi �Ѿ�ָ���1 �ֽڡ�	*/
/* ��es:[edi]ָ��0 ֵ�ֽڡ�	*/
/* ȡԴ�ַ����ֽ�ds:[esi]����>al����esi++��	*/
/* �����ֽڴ浽es:[edi]����edi++��	*/
/* ���ֽ���0��	*/
/* ���ǣ��������ת�����1 ���������������������	*/
/* ����Ŀ���ַ���ָ�롣	*/


/*  ��Դ�ַ�����count ���ֽڸ��Ƶ�Ŀ���ַ�����ĩβ���������һ���ַ���	*/
/* ������dest - Ŀ���ַ�����src - Դ�ַ�����count - �����Ƶ��ֽ�����	*/
/* %0 - esi(src)��%1 - edi(dest)��%2 - eax(0)��%3 - ecx(-1)��%4 - (count)��	*/
static inline char * strncat(char * dest,const char * src,int count)
{
	__asm__("cld\n\t"
			"repne\n\t"
			"scasb\n\t"
			"decl %1\n\t"
			"movl %4,%3\n"
			"1:\tdecl %3\n\t"
			"js 2f\n\t"
			"lodsb\n\t"
			"stosb\n\t"
			"testb %%al,%%al\n\t"
			"jne 1b\n"
			"2:\txorl %2,%2\n\t"
			"stosb"
			::"S" (src),"D" (dest),"a" (0),"c" (0xffffffff),"g" (count)
			:) ;
	return dest;
}
/* �巽��λ��	*/
/* �Ƚ�al ��es:[edi]�ֽڣ�edi++��	*/
/* ֱ���ҵ�Ŀ�Ĵ���ĩ��0 ֵ�ֽڡ�	*/
/* edi ָ���0 ֵ�ֽڡ�	*/
/* �������ֽ�������>ecx��	*/
/* ecx--����0 ��ʼ��������	*/
/* ecx <0 ?��������ǰ��ת�����2 ����	*/
/* ����ȡds:[esi]�����ֽڡ���>al��esi++��	*/
/* �洢��es:[edi]����edi++��	*/
/* ���ֽ�ֵΪ0��	*/
/* �����������ת�����1 �����������ơ�	*/
/* ��al ���㡣	*/
/* �浽es:[edi]����	*/
/* ����Ŀ���ַ���ָ�롣	*/


/* ��һ���ַ�������һ���ַ������бȽϡ�	*/
/* ������cs - �ַ���1��ct - �ַ���2��	*/
/* %0 - eax(__res)����ֵ��%1 - edi(cs)�ַ���1 ָ�룬%2 - esi(ct)�ַ���2 ָ�롣	*/
/* ���أ������1 > ��2���򷵻�1��	*/
/* ��1 = ��2���򷵻�0����1 < ��2���򷵻�-1��	*/
static inline int strcmp(const char * cs,const char * ct)
{
	register int __res __asm__("ax");
	__asm__("cld\n"
			"1:\tlodsb\n\t"
			"scasb\n\t"
			"jne 2f\n\t"
			"testb %%al,%%al\n\t"
			"jne 1b\n\t"
			"xorl %%eax,%%eax\n\t"
			"jmp 3f\n"
			"2:\tmovl $1,%%eax\n\t"
			"jl 3f\n\t"
			"negl %%eax\n"
			"3:"
			:"=a" (__res):"D" (cs),"S" (ct):) ;
	return __res;
}
/* __res �ǼĴ�������(eax)��	*/
/* �巽��λ��	*/
/* ȡ�ַ���2 ���ֽ�ds:[esi]����>al������esi++��	*/
/* al ���ַ���1 ���ֽ�es:[edi]���Ƚϣ�����edi++��	*/
/* �������ȣ�����ǰ��ת�����2��	*/
/* ���ֽ���0 ֵ�ֽ����ַ�����β����	*/
/* ���ǣ��������ת�����1�������Ƚϡ�	*/
/* �ǣ��򷵻�ֵeax ���㣬	*/
/* ��ǰ��ת�����3��������	*/
/* eax ����1��	*/
/* ��ǰ��Ƚ��д�2 �ַ�<��1 �ַ����򷵻���ֵ��������	*/
/* ����eax = -eax�����ظ�ֵ��������	*/
/* ���رȽϽ����	*/


/* �ַ���1 ���ַ���2 ��ǰcount ���ַ����бȽϡ�	*/
/* ������cs - �ַ���1��ct - �ַ���2��count - �Ƚϵ��ַ�����	*/
/* %0 - eax(__res)����ֵ��%1 - edi(cs)��1 ָ�룬%2 - esi(ct)��2 ָ�룬%3 - ecx(count)��	*/
/* ���أ������1 > ��2���򷵻�1��	*/
/* ��1 = ��2���򷵻�0����1 < ��2���򷵻�-1��	*/
static inline int strncmp(const char * cs,const char * ct,int count)
{
	register int __res __asm__("ax");
	__asm__("cld\n"
			"1:\tdecl %3\n\t"
			"js 2f\n\t"
			"lodsb\n\t"
			"scasb\n\t"
			"jne 3f\n\t"
			"testb %%al,%%al\n\t"
			"jne 1b\n"
			"2:\txorl %%eax,%%eax\n\t"
			"jmp 4f\n"
			"3:\tmovl $1,%%eax\n\t"
			"jl 4f\n\t"
			"negl %%eax\n"
			"4:"
			:"=a" (__res):"D" (cs),"S" (ct),"c" (count):) ;
	return __res;
}
/* __res �ǼĴ�������(eax)��	*/
/* �巽��λ��	*/
/* count--��	*/
/* ���count<0������ǰ��ת�����2��	*/
/* ȡ��2 ���ַ�ds:[esi]����>al������esi++��	*/
/* �Ƚ�al �봮1 ���ַ�es:[edi]������edi++��	*/
/* �������ȣ�����ǰ��ת�����3��	*/
/* ���ַ���NULL �ַ���	*/
/* ���ǣ��������ת�����1�������Ƚϡ�	*/
/* ��NULL �ַ�����eax ���㣨����ֵ����	*/
/* ��ǰ��ת�����4��������	*/
/* eax ����1��	*/
/* ���ǰ��Ƚ��д�2 �ַ�<��2 �ַ����򷵻�1��������	*/
/* ����eax = -eax�����ظ�ֵ��������	*/
/* ���رȽϽ����	*/


/*  ���ַ�����Ѱ�ҵ�һ��ƥ����ַ���	*/
/* ������s - �ַ�����c - ��Ѱ�ҵ��ַ���	*/
/* %0 - eax(__res)��%1 - esi(�ַ���ָ��s)��%2 - eax(�ַ�c)��	*/
/* ���أ������ַ����е�һ�γ���ƥ���ַ���ָ�롣��û���ҵ�ƥ����ַ����򷵻ؿ�ָ�롣	*/
static inline char * strchr(const char * s,char c)
{
	register char * __res __asm__("ax");
	__asm__("cld\n\t"
			"movb %%al,%%ah\n"
			"1:\tlodsb\n\t"
			"cmpb %%ah,%%al\n\t"
			"je 2f\n\t"
			"testb %%al,%%al\n\t"
			"jne 1b\n\t"
			"movl $1,%1\n"
			"2:\tmovl %1,%0\n\t"
			"decl %0"
			:"=a" (__res):"S" (s),"0" (c):) ;
	return __res;
}
/* __res �ǼĴ�������(eax)��	*/
/* �巽��λ��	*/
/* �����Ƚ��ַ��Ƶ�ah��	*/
/* ȡ�ַ������ַ�ds:[esi]����>al������esi++��	*/
/* �ַ������ַ�al ��ָ���ַ�ah ��Ƚϡ�	*/
/* ����ȣ�����ǰ��ת�����2 ����	*/
/* al ���ַ���NULL �ַ��𣿣��ַ�����β����	*/
/* �����ǣ��������ת�����1�������Ƚϡ�	*/
/* �ǣ���˵��û���ҵ�ƥ���ַ���esi ��1��	*/
/* ��ָ��ƥ���ַ���һ���ֽڴ���ָ��ֵ����eax	*/
/* ��ָ�����Ϊָ��ƥ����ַ���	*/
/* ����ָ�롣	*/


/* Ѱ���ַ�����ָ���ַ����һ�γ��ֵĵط��������������ַ�����	*/
/* ������s - �ַ�����c - ��Ѱ�ҵ��ַ���	*/
/* %0 - edx(__res)��%1 - edx(0)��%2 - esi(�ַ���ָ��s)��%3 - eax(�ַ�c)��	*/
/* ���أ������ַ��������һ�γ���ƥ���ַ���ָ�롣��û���ҵ�ƥ����ַ����򷵻ؿ�ָ�롣	*/
static inline char * strrchr(const char * s,char c)
{
	register char * __res __asm__("dx");
	__asm__("cld\n\t"
			"movb %%al,%%ah\n"
			"1:\tlodsb\n\t"
			"cmpb %%ah,%%al\n\t"
			"jne 2f\n\t"
			"movl %%esi,%0\n\t"
			"decl %0\n"
			"2:\ttestb %%al,%%al\n\t"
			"jne 1b"
			:"=d" (__res):"0" (0),"S" (s),"a" (c):) ;
	return __res;
}
/* __res �ǼĴ�������(edx)��	*/
/* �巽��λ��	*/
/* ����Ѱ�ҵ��ַ��Ƶ�ah��	*/
/* ȡ�ַ������ַ�ds:[esi]����>al������esi++��	*/
/* �ַ������ַ�al ��ָ���ַ�ah ���Ƚϡ�	*/
/* ������ȣ�����ǰ��ת�����2 ����	*/
/* ���ַ�ָ�뱣�浽edx �С�	*/
/* ָ�����һλ��ָ���ַ�����ƥ���ַ�����	*/
/* �Ƚϵ��ַ���0 �𣨵��ַ���β����	*/
/* �����������ת�����1 ���������Ƚϡ�	*/
/* ����ָ�롣	*/


/*  ���ַ���1 ��Ѱ�ҵ�1 ���ַ����У����ַ������е��κ��ַ����������ַ���2 �С�	*/
/* ������cs - �ַ���1 ָ�룬ct - �ַ���2 ָ�롣	*/
/* %0 - esi(__res)��%1 - eax(0)��%2 - ecx(-1)��%3 - esi(��1 ָ��cs)��%4 - (��2 ָ��ct)��	*/
/* �����ַ���1 �а����ַ���2 ���κ��ַ����׸��ַ����еĳ���ֵ��	*/
static inline int strspn(const char * cs, const char * ct)
{
	register char * __res __asm__("si");
	__asm__("cld\n\t"
			"movl %4,%%edi\n\t"
			"repne\n\t"
			"scasb\n\t"
			"notl %%ecx\n\t"
			"decl %%ecx\n\t"
			"movl %%ecx,%%edx\n"
			"1:\tlodsb\n\t"
			"testb %%al,%%al\n\t"
			"je 2f\n\t"
			"movl %4,%%edi\n\t"
			"movl %%edx,%%ecx\n\t"
			"repne\n\t"
			"scasb\n\t"
			"je 1b\n"
			"2:\tdecl %0"
			:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
			:) ;
	return __res-cs;
}
/* __res �ǼĴ�������(esi)��	*/
/* �巽��λ��	*/
/* ���ȼ��㴮2 �ĳ��ȡ���2 ָ�����edi �С�	*/
/* �Ƚ�al(0)�봮2 �е��ַ���es:[edi]������edi++��	*/
/* �������Ⱦͼ����Ƚ�(ecx �𲽵ݼ�)��	*/
/* ecx ��ÿλȡ����	*/
/* ecx--���ô�2 �ĳ���ֵ��	*/
/* ����2 �ĳ���ֵ�ݷ���edx �С�	*/
/* ȡ��1 �ַ�ds:[esi]����>al������esi++��	*/
/* ���ַ�����0 ֵ�𣨴�1 ��β����	*/
/* ����ǣ�����ǰ��ת�����2 ����	*/
/* ȡ��2 ͷָ�����edi �С�	*/
/* �ٽ���2 �ĳ���ֵ����ecx �С�	*/
/* �Ƚ�al �봮2 ���ַ�es:[edi]������edi++��	*/
/* �������Ⱦͼ����Ƚϡ�	*/
/* �����ȣ��������ת�����1 ����	*/
/* esi--��ָ�����һ�������ڴ�2 �е��ַ���	*/
/* �����ַ����еĳ���ֵ��	*/

/*  Ѱ���ַ���1 �в������ַ���2 ���κ��ַ����׸��ַ����С�	*/
/* ������cs - �ַ���1 ָ�룬ct - �ַ���2 ָ�롣	*/
/* %0 - esi(__res)��%1 - eax(0)��%2 - ecx(-1)��%3 - esi(��1 ָ��cs)��%4 - (��2 ָ��ct)��	*/
/* �����ַ���1 �в������ַ���2 ���κ��ַ����׸��ַ����еĳ���ֵ��	*/
static inline int strcspn(const char * cs, const char * ct)
{
	register char * __res __asm__("si");
	__asm__("cld\n\t"
			"movl %4,%%edi\n\t"
			"repne\n\t"
			"scasb\n\t"
			"notl %%ecx\n\t"
			"decl %%ecx\n\t"
			"movl %%ecx,%%edx\n"
			"1:\tlodsb\n\t"
			"testb %%al,%%al\n\t"
			"je 2f\n\t"
			"movl %4,%%edi\n\t"
			"movl %%edx,%%ecx\n\t"
			"repne\n\t"
			"scasb\n\t"
			"jne 1b\n"
			"2:\tdecl %0"
			:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
			:) ;
	return __res-cs;
}

/* __res �ǼĴ�������(esi)��	*/
/* �巽��λ��	*/
/* ���ȼ��㴮2 �ĳ��ȡ���2 ָ�����edi �С�	*/
/* �Ƚ�al(0)�봮2 �е��ַ���es:[edi]������edi++��	*/
/* �������Ⱦͼ����Ƚ�(ecx �𲽵ݼ�)��	*/
/* ecx ��ÿλȡ����	*/
/* ecx--���ô�2 �ĳ���ֵ��	*/
/* ����2 �ĳ���ֵ�ݷ���edx �С�	*/
/* ȡ��1 �ַ�ds:[esi]����>al������esi++��	*/
/* ���ַ�����0 ֵ�𣨴�1 ��β����	*/
/* ����ǣ�����ǰ��ת�����2 ����	*/
/* ȡ��2 ͷָ�����edi �С�	*/
/* �ٽ���2 �ĳ���ֵ����ecx �С�	*/
/* �Ƚ�al �봮2 ���ַ�es:[edi]������edi++��	*/
/* �������Ⱦͼ����Ƚϡ�	*/
/* �������ȣ��������ת�����1 ����	*/
/* esi--��ָ�����һ�������ڴ�2 �е��ַ���	*/
/* �����ַ����еĳ���ֵ��	*/


/*  ���ַ���1 ��Ѱ���׸��������ַ���2 �е��κ��ַ���	*/
/* ������cs - �ַ���1 ��ָ�룬ct - �ַ���2 ��ָ�롣	*/
/* %0 -esi(__res)��%1 -eax(0)��%2 -ecx(0xffffffff)��%3 -esi(��1 ָ��cs)��%4 -(��2 ָ��ct)��	*/
/* �����ַ���1 ���׸������ַ���2 ���ַ���ָ�롣	*/
static inline char * strpbrk(const char * cs,const char * ct)
{
	register char * __res __asm__("si");
	__asm__("cld\n\t"
			"movl %4,%%edi\n\t"
			"repne\n\t"
			"scasb\n\t"
			"notl %%ecx\n\t"
			"decl %%ecx\n\t"
			"movl %%ecx,%%edx\n"
			"1:\tlodsb\n\t"
			"testb %%al,%%al\n\t"
			"je 2f\n\t"
			"movl %4,%%edi\n\t"
			"movl %%edx,%%ecx\n\t"
			"repne\n\t"
			"scasb\n\t"
			"jne 1b\n\t"
			"decl %0\n\t"
			"jmp 3f\n"
			"2:\txorl %0,%0\n"
			"3:"
			:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
			:) ;
	return __res;
}

/* __res �ǼĴ�������(esi)��	*/
/* �巽��λ��	*/
/* ���ȼ��㴮2 �ĳ��ȡ���2 ָ�����edi �С�	*/
/* �Ƚ�al(0)�봮2 �е��ַ���es:[edi]������edi++��	*/
/* �������Ⱦͼ����Ƚ�(ecx �𲽵ݼ�)��	*/
/* ecx ��ÿλȡ����	*/
/* ecx--���ô�2 �ĳ���ֵ��	*/
/* ����2 �ĳ���ֵ�ݷ���edx �С�	*/
/* ȡ��1 �ַ�ds:[esi]����>al������esi++��	*/
/* ���ַ�����0 ֵ�𣨴�1 ��β����	*/
/* ����ǣ�����ǰ��ת�����2 ����	*/
/* ȡ��2 ͷָ�����edi �С�	*/
/* �ٽ���2 �ĳ���ֵ����ecx �С�	*/
/* �Ƚ�al �봮2 ���ַ�es:[edi]������edi++��	*/
/* �������Ⱦͼ����Ƚϡ�	*/
/* �������ȣ��������ת�����1 ����	*/
/* esi--��ָ��һ�������ڴ�2 �е��ַ���	*/
/* ��ǰ��ת�����3 ����	*/
/* û���ҵ����������ģ�������ֵΪNULL��	*/
/* ����ָ��ֵ��	*/


/*  ���ַ���1 ��Ѱ���׸�ƥ�������ַ���2 ���ַ�����	*/
/* ������cs - �ַ���1 ��ָ�룬ct - �ַ���2 ��ָ�롣	*/
/* %0 -eax(__res)��%1 -eax(0)��%2 -ecx(0xffffffff)��%3 -esi(��1 ָ��cs)��%4 -(��2 ָ��ct)��	*/
/* ���أ������ַ���1 ���׸�ƥ���ַ���2 ���ַ���ָ�롣	*/
static inline char * strstr(const char * cs,const char * ct)
{
	register char * __res __asm__("ax");
	__asm__("cld\n\t" \
			"movl %4,%%edi\n\t"
			"repne\n\t"
			"scasb\n\t"
			"notl %%ecx\n\t"
			"decl %%ecx\n\t"	/* NOTE! This also sets Z if searchstring='' */
			"movl %%ecx,%%edx\n"
			"1:\tmovl %4,%%edi\n\t"
			"movl %%esi,%%eax\n\t"
			"movl %%edx,%%ecx\n\t"
			"repe\n\t"
			"cmpsb\n\t"
			"je 2f\n\t"		/* also works for empty string, see above */
			"xchgl %%eax,%%esi\n\t"
			"incl %%esi\n\t"
			"cmpb $0,-1(%%eax)\n\t"
			"jne 1b\n\t"
			"xorl %%eax,%%eax\n\t"
			"2:"
			:"=a" (__res):"0" (0),"c" (0xffffffff),"S" (cs),"g" (ct)
			:) ;
	return __res;
}
/* __res �ǼĴ�������(eax)��	*/
/* �巽��λ��	*/
/* ���ȼ��㴮2 �ĳ��ȡ���2 ָ�����edi �С�	*/
/* �Ƚ�al(0)�봮2 �е��ַ���es:[edi]������edi++��	*/
/* �������Ⱦͼ����Ƚ�(ecx �𲽵ݼ�)��	*/
/* ecx ��ÿλȡ����	*/
/* NOTE! This also sets Z if searchstring='' */
/* ע�⣡���������Ϊ�գ�������Z ��־���ô�2 �ĳ���ֵ��	*/
/* ����2 �ĳ���ֵ�ݷ���edx ��	*/
/* ȡ��2 ͷָ�����edi �С�	*/
/* ����1 ��ָ�븴�Ƶ�eax �С�	*/
/* �ٽ���2 �ĳ���ֵ����ecx �С�	*/
/* �Ƚϴ�1 �ʹ�2 �ַ�(ds:[esi],es:[edi])��esi++,edi++��	*/
/* ����Ӧ�ַ���Ⱦ�һֱ�Ƚ���ȥ��	*/
/* also works for empty string, see above */
/* �Կմ�ͬ����Ч�������� */	/* ��ȫ��ȣ���ת�����2��	*/
/* ��1 ͷָ�롪��>esi���ȽϽ���Ĵ�1 ָ�롪��>eax��	*/
/* ��1 ͷָ��ָ����һ���ַ���	*/
/* ��1 ָ��(eax-1)��ָ�ֽ���0 ��	*/
/* ��������ת�����1�������Ӵ�1 �ĵ�2 ���ַ���ʼ�Ƚϡ�	*/
/* ��eax����ʾû���ҵ�ƥ�䡣	*/
/* ���رȽϽ����	*/


/*  �����ַ������ȡ�	*/
/* ������s - �ַ�����	*/
/* %0 - ecx(__res)��%1 - edi(�ַ���ָ��s)��%2 - eax(0)��%3 - ecx(0xffffffff)��	*/
/* ���أ������ַ����ĳ��ȡ�	*/
static inline int strlen(const char * s)
{
	register int __res __asm__("cx");
	__asm__("cld\n\t"
			"repne\n\t"
			"scasb\n\t"
			"notl %0\n\t"
			"decl %0"
			:"=c" (__res):"D" (s),"a" (0),"0" (0xffffffff):) ;
	return __res;
}
/* __res �ǼĴ�������(ecx)��	*/
/* �巽��λ��	*/
/* al(0)���ַ������ַ�es:[edi]�Ƚϣ�	*/
/* ������Ⱦ�һֱ�Ƚϡ�	*/
/* ecx ȡ����	*/
/* ecx--�����ַ����ó���ֵ��	*/
/* �����ַ�������ֵ��	*/


extern char *___strtok;		/* ������ʱ���ָ�����汻�����ַ���1(s)��ָ�롣	*/

/*  �����ַ���2 �е��ַ����ַ���1 �ָ�ɱ��(tokern)���С�
 * ����1 �����ǰ��������������(token)�����У����ɷָ���ַ���2 �е�һ�������ַ��ֿ���
 * ��һ�ε���strtok()ʱ��������ָ���ַ���1 �е�1 ��token ���ַ���ָ�룬���ڷ���token ʱ��
 * һnull �ַ�д���ָ����������ʹ��null ��Ϊ�ַ���1 �ĵ��ã��������ַ�������ɨ���ַ���1��
 * ֱ��û��token Ϊֹ���ڲ�ͬ�ĵ��ù����У��ָ����2 ���Բ�ͬ��
 
 * ������s - ��������ַ���1��ct - ���������ָ�����ַ���2��
 * ��������%0 - ebx(__res)��%1 - esi(__strtok)��
 * ������룺%2 - ebx(__strtok)��%3 - esi(�ַ���1 ָ��s)��%4 - ���ַ���2 ָ��ct����
 * ���أ������ַ���s �е�1 ��token�����û���ҵ�token���򷵻�һ��null ָ�롣
 * ����ʹ���ַ���s ָ��Ϊnull �ĵ��ã�����ԭ�ַ���s ��������һ��token��
 */
static inline char * strtok(char * s,const char * ct)
{
	register char * __res __asm__("si");
	__asm__("testl %1,%1\n\t"
			"jne 1f\n\t"
			"testl %0,%0\n\t"
			"je 8f\n\t"
			"movl %0,%1\n"
			"1:\txorl %0,%0\n\t"
			"movl $-1,%%ecx\n\t"
			"xorl %%eax,%%eax\n\t"
			"cld\n\t"
			"movl %4,%%edi\n\t"
			"repne\n\t"
			"scasb\n\t"
			"notl %%ecx\n\t"
			"decl %%ecx\n\t"
			"je 7f\n\t"			/* empty delimeter-string */
			"movl %%ecx,%%edx\n"
			"2:\tlodsb\n\t"
			"testb %%al,%%al\n\t"
			"je 7f\n\t"
			"movl %4,%%edi\n\t"
			"movl %%edx,%%ecx\n\t"
			"repne\n\t"
			"scasb\n\t"
			"je 2b\n\t"
			"decl %1\n\t"
			"cmpb $0,(%1)\n\t"
			"je 7f\n\t"
			"movl %1,%0\n"
			"3:\tlodsb\n\t"
			"testb %%al,%%al\n\t"
			"je 5f\n\t"
			"movl %4,%%edi\n\t"
			"movl %%edx,%%ecx\n\t"
			"repne\n\t"
			"scasb\n\t"
			"jne 3b\n\t"
			"decl %1\n\t"
			"cmpb $0,(%1)\n\t"
			"je 5f\n\t"
			"movb $0,(%1)\n\t"
			"incl %1\n\t"
			"jmp 6f\n"
			"5:\txorl %1,%1\n"
			"6:\tcmpb $0,(%0)\n\t"
			"jne 7f\n\t"
			"xorl %0,%0\n"
			"7:\ttestl %0,%0\n\t"
			"jne 8f\n\t"
			"movl %0,%1\n"
			"8:"
			:"=b" (__res),"=S" (___strtok)
			:"0" (___strtok),"1" (s),"g" (ct)
			:) ;
	return __res;
}
/* ���Ȳ���esi(�ַ���1 ָ��s)�Ƿ���NULL��	*/
/* ������ǣ���������״ε��ñ���������ת���1��	*/
/* �����NULL�����ʾ�˴��Ǻ������ã���ebx(__strtok)��	*/
/* ���ebx ָ����NULL�����ܴ�����ת������	*/
/* ��ebx ָ�븴�Ƶ�esi��	*/
/* ��ebx ָ�롣	*/
/* ��ecx = 0xffffffff��	*/
/* ����eax��	*/
/* �巽��λ��	*/
/* �������ַ���2 �ĳ��ȡ�edi ָ���ַ���2��	*/
/* ��al(0)��es:[edi]�Ƚϣ�����edi++��	*/
/* ֱ���ҵ��ַ���2 �Ľ���null �ַ��������ecx==0��	*/
/* ��ecx ȡ����	*/
/* ecx--���õ��ַ���2 �ĳ���ֵ��	*/
/* empty delimeter-string */
/* �ָ���ַ����� */	/* ����2 ����Ϊ0����ת���7��	*/
/* ����2 �����ݴ���edx��	*/
/* ȡ��1 ���ַ�ds:[esi]����>al������esi++��	*/
/* ���ַ�Ϊ0 ֵ��(��1 ����)��	*/
/* ����ǣ�����ת���7��	*/
/* edi �ٴ�ָ��2 �ס�	*/
/* ȡ��2 �ĳ���ֵ���������ecx��	*/
/* ��al �д�1 ���ַ��봮2 �������ַ��Ƚϣ�	*/
/* �жϸ��ַ��Ƿ�Ϊ�ָ����	*/
/* �����ڴ�2 ���ҵ���ͬ�ַ����ָ����������ת���2��	*/
/* �����Ƿָ������1 ָ��esi ָ���ʱ�ĸ��ַ���	*/
/* ���ַ���NULL �ַ���	*/
/* ���ǣ�����ת���7 ����	*/
/* �����ַ���ָ��esi �����ebx��	*/
/* ȡ��1 ��һ���ַ�ds:[esi]����>al������esi++��	*/
/* ���ַ���NULL �ַ���	*/
/* ���ǣ���ʾ��1 ��������ת�����5��	*/
/* edi �ٴ�ָ��2 �ס�	*/
/* ��2 ����ֵ���������ecx��	*/
/* ��al �д�1 ���ַ��봮2 ��ÿ���ַ��Ƚϣ�	*/
/* ����al �ַ��Ƿ��Ƿָ����	*/
/* �����Ƿָ������ת���3����⴮1 ����һ���ַ���	*/
/* ���Ƿָ������esi--��ָ��÷ָ���ַ���	*/
/* �÷ָ����NULL �ַ���	*/
/* ���ǣ�����ת�����5��	*/
/* �����ǣ��򽫸÷ָ����NULL �ַ��滻����	*/
/* esi ָ��1 ����һ���ַ���Ҳ��ʣ�മ�ס�	*/
/* ��ת���6 ����	*/
/* esi ���㡣	*/
/* ebx ָ��ָ��NULL �ַ���	*/
/* �����ǣ�����ת���7��	*/
/* ���ǣ�����ebx=NULL��	*/
/* ebx ָ��ΪNULL ��	*/
/* ����������ת8�����������롣	*/
/* ��esi ��ΪNULL��	*/
/* ����ָ����token ��ָ�롣	*/


/*  �ڴ�鸴�ơ���Դ��ַsrc ����ʼ����n ���ֽڵ�Ŀ�ĵ�ַdest ����
 * ������dest - ���Ƶ�Ŀ�ĵ�ַ��src - ���Ƶ�Դ��ַ��n - �����ֽ�����
 * %0 - ecx(n)��%1 - esi(src)��%2 - edi(dest)��
 */
static inline void * memcpy(void * dest,const void * src, int n)
{
	__asm__("cld\n\t"
			"rep\n\t"
			"movsb"
			::"c" (n),"S" (src),"D" (dest)
			:) ;
	return dest;
}
/* �巽��λ��	*/
/* �ظ�ִ�и���ecx ���ֽڣ�	*/
/* ��ds:[esi]��es:[edi]��esi++��edi++��	*/
/* ����Ŀ�ĵ�ַ��	*/


/*  �ڴ���ƶ���ͬ�ڴ�鸴�ƣ��������ƶ��ķ���
 * ������dest - ���Ƶ�Ŀ�ĵ�ַ��src - ���Ƶ�Դ��ַ��n - �����ֽ�����
 * ��dest<src ��%0 - ecx(n)��%1 - esi(src)��%2 - edi(dest)��
 * ����%0 - ecx(n)��%1 - esi(src+n-1)��%2 - edi(dest+n-1)��
 *  ����������Ϊ�˷�ֹ�ڸ���ʱ������ص����ǡ�
 */
static inline void * memmove(void * dest,const void * src, int n)
{
	if (dest<src)
		__asm__("cld\n\t"
				"rep\n\t"
				"movsb"
				::"c" (n),"S" (src),"D" (dest)
				:) ;
	else
		__asm__("std\n\t"
				"rep\n\t"
				"movsb"
				::"c" (n),"S" (src+n-1),"D" (dest+n-1)
				:) ;
	return dest;
}
/* �巽��λ��	*/
/* ��ds:[esi]��es:[edi]������esi++��edi++��	*/
/* �ظ�ִ�и���ecx �ֽڡ�	*/
/* �÷���λ����ĩ�˿�ʼ���ơ�	*/
/* ��ds:[esi]��es:[edi]������esi--��edi--��	*/
/* ����ecx ���ֽڡ�	*/


/*  �Ƚ�n ���ֽڵ������ڴ棨�����ַ���������ʹ����NULL �ֽ�Ҳ��ֹͣ�Ƚϡ�
 * ������cs - �ڴ��1 ��ַ��ct - �ڴ��2 ��ַ��count - �Ƚϵ��ֽ�����
 * %0 - eax(__res)��%1 - eax(0)��%2 - edi(�ڴ��1)��%3 - esi(�ڴ��2)��%4 - ecx(count)��
 * ���أ�����1>��2 ����1��
 * ��1<��2������-1����1==��2���򷵻�0��
 */
static inline int memcmp(const void * cs,const void * ct,int count)
{
	register int __res __asm__("ax");
	__asm__("cld\n\t"
			"repe\n\t"
			"cmpsb\n\t"
			"je 1f\n\t"
			"movl $1,%%eax\n\t"
			"jl 1f\n\t"
			"negl %%eax\n"
			"1:"
			:"=a" (__res):"0" (0),"D" (cs),"S" (ct),"c" (count)
			:) ;
	return __res;
}
/* __res �ǼĴ���������	*/
/* �巽��λ��	*/
/* ���������ظ���	*/
/* �Ƚ�ds:[esi]��es:[edi]�����ݣ�����esi++��edi++��	*/
/* �������ͬ������ת�����1������0(eax)ֵ	*/
/* ����eax ��1��	*/
/* ���ڴ��2 ���ݵ�ֵ<�ڴ��1������ת���1��	*/
/* ����eax = -eax��	*/
/* ���رȽϽ����	*/


/*  ��n �ֽڴ�С���ڴ��(�ַ���)��Ѱ��ָ���ַ���
 * ������cs - ָ���ڴ���ַ��c - ָ�����ַ���count - �ڴ�鳤�ȡ�
 * %0 - edi(__res)��%1 - eax(�ַ�c)��%2 - edi(�ڴ���ַcs)��%3 - ecx(�ֽ���count)��
 * ���ص�һ��ƥ���ַ���ָ�룬���û���ҵ����򷵻�NULL �ַ���
 */
static inline void * memchr(const void * cs,char c,int count)
{
	register void * __res __asm__("di");
	if (!count)
		return NULL;
	__asm__("cld\n\t"
			"repne\n\t"
			"scasb\n\t"
			"je 1f\n\t"
			"movl $1,%0\n"
			"1:\tdecl %0"
			:"=D" (__res):"a" (c),"D" (cs),"c" (count)
			:) ;
	return __res;
}
/* __res �ǼĴ���������	*/
/* ����ڴ�鳤��==0���򷵻�NULL��û���ҵ���	*/
/* �巽��λ��	*/
/* �����������ظ�ִ��������䣬	*/
/* al ���ַ���es:[edi]�ַ����Ƚϣ�����edi++��	*/
/* ����������ǰ��ת�����1 ����	*/
/* ����edi ����1��	*/
/* ��edi ָ���ҵ����ַ�������NULL����	*/
/* �����ַ�ָ�롣	*/


/*  ���ַ���дָ�������ڴ�顣
 * ���ַ�c ��дs ָ����ڴ����򣬹���count �ֽڡ�
 * %0 - eax(�ַ�c)��%1 - edi(�ڴ��ַ)��%2 - ecx(�ֽ���count)��
 */
static inline void * memset(void * s,char c,int count)
{
	__asm__("cld\n\t"
			"rep\n\t"
			"stosb"
			::"a" (c),"D" (s),"c" (count)
			:) ;
	return s;
}
/* �巽��λ��	*/
/* �ظ�ecx ָ���Ĵ�����ִ��	*/
/* ��al ���ַ�����es:[edi]�У�����edi++��	*/

#endif
