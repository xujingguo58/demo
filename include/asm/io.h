/* Ӳ���˿��ֽ����������	*/
/* ������value - ������ֽڣ�	*/
/* port - �˿ڡ�	*/
#define outb(value,port) \
__asm__ ( "outb %%al,%%dx":: "a" (value), "d" (port))


/* Ӳ���˿��ֽ����뺯����	*/
/* ������port - �˿ڡ�	*/
/* ���ض�ȡ���ֽڡ�	*/
#define inb(port) ({ \
unsigned char _v; \
__asm__ volatile ( "inb %%dx,%%al": "=a" (_v): "d" (port)); \
_v; \
})

/* ���ӳٵ�Ӳ���˿��ֽ����������ʹ��������ת������ӳ�һ�ᡣ	*/
/* ������value - ������ֽڣ�	*/
/* port - �˿ڡ�	*/
#define outb_p(value,port) \
							__asm__ ( "outb %%al,%%dx\n" \
/* ��ǰ��ת�����1��(����һ����䴦)��*/	"\tjmp 1f\n" \
/* ��ǰ��ת�����1����				*/	"1:\tjmp 1f\n" \
									"1:":: "a" (value), "d" (port))

/* ���ӳٵ�Ӳ���˿��ֽ����뺯����ʹ��������ת������ӳ�һ�ᡣ	*/
/* ������port - �˿ڡ�	*/
/* ���ض�ȡ���ֽڡ�	*/
#define inb_p(port) ({ \
unsigned char _v; \
							__asm__ volatile ( "inb %%dx,%%al\n" \
/* ��ǰ��ת�����1��(����һ����䴦)��*/	"\tjmp 1f\n" \
/* ��ǰ��ת�����1����				*/	"1:\tjmp 1f\n" \
									"1:": "=a" (_v): "d" (port)); \
									_v; \
})
