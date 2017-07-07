/*
* NOTE!!! memcpy(dest,src,n) assumes ds=es=normal data segment. This
* goes for all kernel functions (ds=es=kernel space, fs=local data,
* gs=null), as well as for all well-behaving user programs (ds=es=
* user data space). This is NOT a bug, as any user program that changes
* es deserves to die if it isn't careful.
*/
/*
* ע��!!!memcpy(dest,src,n)����μĴ���ds=es=ͨ�����ݶΡ�* ���ں���ʹ�õ�
* ���к��������ڸü��裨ds=es=�ں˿ռ䣬fs=�ֲ����ݿռ䣬gs=null��,��������
* ��Ϊ��Ӧ�ó���Ҳ��������ds=es=�û����ݿռ䣩������κ��û���������Ķ���
* es �Ĵ������������򲢲�������ϵͳ���������ɵġ�
*/

/* �ڴ�鸴�ơ���Դ��ַsrc ����ʼ����n ���ֽڵ�Ŀ�ĵ�ַdest ����
* ������dest - ���Ƶ�Ŀ�ĵ�ַ��src - ���Ƶ�Դ��ַ��n - �����ֽ�����
* 		%0 - edi(Ŀ�ĵ�ַdest)��
* 		%1 - esi(Դ��ַsrc)
* 		%2 - ecx(�ֽ���n)��
*/
#define memcpy(dest,src,n) ({ \
	void * _res = dest; \
	__asm__ ("cld;rep;movsb" \
	::"D" ((long)(_res)),"S" ((long)(src)),"c" ((long) (n)) \
	:) ; \
	_res; \
})
/* ��ds:[esi]���Ƶ�es:[edi]������esi++��edi++��������ecx(n)�ֽڡ�	*/
