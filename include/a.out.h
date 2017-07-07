#ifndef _A_OUT_H
#define _A_OUT_H

#define __GNU_EXEC_MACROS__

/* ִ���ļ��ṹ��	*/
/* =============================
 * unsigned long a_magic	ִ���ļ�ħ����ʹ��N_MAGIC �Ⱥ���ʡ�
 * unsigned a_text			���볤�ȣ��ֽ�����
 * unsigned a_data			���ݳ��ȣ��ֽ�����
 * unsigned a_bss			�ļ��е�δ��ʼ�����������ȣ��ֽ�����
 * unsigned a_syms			�ļ��еķ��ű����ȣ��ֽ�����
 * unsigned a_entry			ִ�п�ʼ��ַ��
 * unsigned a_trsize		�����ض�λ��Ϣ���ȣ��ֽ�����
 * unsigned a_drsize		�����ض�λ��Ϣ���ȣ��ֽ�����
 * -----------------------------
 */
struct exec
{
	unsigned long a_magic;	/* Use macros N_MAGIC, etc for access */
	unsigned a_text;		/* length of text, in bytes */
	unsigned a_data;		/* length of data, in bytes */
	unsigned a_bss;			/* length of uninitialized data area for file, in bytes */
	unsigned a_syms;		/* length of symbol table data in file, in bytes */
	unsigned a_entry;		/* start address */
	unsigned a_trsize;		/* length of relocation info for text, in bytes */
	unsigned a_drsize;		/* length of relocation info for data, in bytes */
};

/* ����ȡ����exec�ṹ�е�ħ����	*/
#ifndef N_MAGIC
#define N_MAGIC(exec) ((exec).a_magic)
#endif

#ifndef OMAGIC
/* ��ʷ��������PDP-11������ϣ�ħ�����������ǰ˽�����0407 (0x107)����λ��ִ�г���
 * ͷ�ṹ�Ŀ�ʼ����ԭ����PDP-11��һ����תָ���ʾ��ת�����7���ֺ�Ĵ��뿪ʼ����
 * �������س���loader���Ϳ����ڰ�ִ���ļ������ڴ��ֱ����ת��ָ�ʼ�����С�����
 * ��û�г���ʹ�����ַ�����������˽�����ȴ��Ϊʶ���ļ����͵ı�־��ħ����������������
 * OMAGIC������Ϊ��Old Magic����˼��	*/

#define OMAGIC 0407			/* Code indicating object file or impure executable. */
							/* ָ��ΪĿ���ļ����߲����Ŀ�ִ���ļ��Ĵ��� */
#define NMAGIC 0410			/* Code indicating pure executable. */
							/* ָ��Ϊ����ִ���ļ��Ĵ��� */
#define ZMAGIC 0413			/* Code indicating demand-paged executable. */
							/* ָ��Ϊ�����ҳ�����Ŀ�ִ���ļ� */
#endif 		/* not OMAGIC */
			/* ���ħ�����ܱ�ʶ���򷵻��档	*/
/* ���⻹��һ��QMAGIC����Ϊ�˽�Լ����������������ִ���ļ���ͷ�ṹ�������մ�š�	*/

/* ����������ж�ħ���ֶε���ȷ�ԡ����ħ�����ܱ�ʶ���򷵻��档	*/
#ifndef N_BADMAG
#define N_BADMAG(x)					\
(N_MAGIC(x) != OMAGIC && N_MAGIC(x) != NMAGIC		\
 && N_MAGIC(x) != ZMAGIC)
#endif

#define _N_BADMAG(x)					\
(N_MAGIC(x) != OMAGIC && N_MAGIC(x) != NMAGIC		\
 && N_MAGIC(x) != ZMAGIC)

/* ����ͷ���ڴ��е�ƫ��λ�á�Ŀ���ļ�ͷ�ṹĩ�˵�1024�ֽ�֮��ĳ��ȡ�	*/
#define _N_HDROFF(x) (SEGMENT_SIZE - sizeof (struct exec))

/* ��������ڲ���Ŀ���ļ������ݣ�����}ģ���ļ��Ϳ�ִ���ļ���	*/
/* ���벿����ʼƫ��ֵ��
 * ����ļ���ZMAGIC���͵ģ�����ִ���ļ�����ô���벿���Ǵ�ִ���ļ���1024�ֽ�ƫ�ƴ�
 * ��ʼ������ִ�д��벿�ֽ���ִ��ͷ�ṹĩ�ˣ�32�ֽڣ���ʼ�����ļ���ģ���ļ���OMAGIC
 * ���ͣ���	*/
#ifndef N_TXTOFF
#define N_TXTOFF(x) \
(N_MAGIC(x) == ZMAGIC ? _N_HDROFF((x)) + sizeof (struct exec) : sizeof (struct exec))
#endif

/* ���ݲ�����ʼƫ��ֵ���Ӵ��벿��ĩ�˿�ʼ��	*/
#ifndef N_DATOFF
#define N_DATOFF(x) (N_TXTOFF(x) + (x).a_text)
#endif

/* �����ض�λ��Ϣƫ��ֵ�������ݲ���ĩ�˿�ʼ��	*/
#ifndef N_TRELOFF
#define N_TRELOFF(x) (N_DATOFF(x) + (x).a_data)
#endif

/* �����ض�λ��Ϣƫ��ֵ���Ӵ����ض�λ��Ϣĩ�˿�ʼ��	*/
#ifndef N_DRELOFF
#define N_DRELOFF(x) (N_TRELOFF(x) + (x).a_trsize)
#endif

/* ���ű�ƫ��ֵ�����������ݶ��ض�λ��ĩ�˿�ʼ��	*/
#ifndef N_SYMOFF
#define N_SYMOFF(x) (N_DRELOFF(x) + (x).a_drsize)
#endif

/* �ַ�����Ϣƫ��ֵ���ڷ��ű�֮��	*/
#ifndef N_STROFF
#define N_STROFF(x) (N_SYMOFF(x) + (x).a_syms)
#endif

/* ����Կ�ִ���ļ������ص��ڴ棨�߼��ռ䣩�е�λ��������в�����	*/
/* Address of text segment in memory after it is loaded.	*/
/* ����μ��ص��ڴ��к�ĵ�ַ */
#ifndef N_TXTADDR
#define N_TXTADDR(x) 0
#endif

/* Address of data segment in memory after it is loaded.
  Note that it is up to you to define SEGMENT_SIZE
  on machines not listed here.	*/
/* ���ݶμ��ص��ڴ��к�ĵ�ַ��
 * ע�⣬��������û���г����ƵĻ�������Ҫ���Լ�������	
 * ��Ӧ��SEGMENT_SIZE	*/
#if defined(vax) || defined(hp300) || defined(pyr)
#define SEGMENT_SIZE PAGE_SIZE
#endif
#ifdef	hp300
#define	PAGE_SIZE	4096
#endif
#ifdef	sony
#define	SEGMENT_SIZE	0x2000
#endif	/* Sony.	*/
#ifdef is68k
#define SEGMENT_SIZE 0x20000
#endif
#if defined(m68k) && defined(PORTAR)
#define PAGE_SIZE 0x400
#define SEGMENT_SIZE PAGE_SIZE
#endif

/* ���Linux0.11�ں˰��ڴ�ҳ����Ϊ4KB���δ�С����Ϊ1KB�����û��ʹ������Ķ��塣	*/
#define PAGE_SIZE 4096
#define SEGMENT_SIZE 1024

/* �Զ�Ϊ��Ĵ�С����λ��ʽ����	*/
#define _N_SEGMENT_ROUND(x) (((x) + SEGMENT_SIZE - 1) & ~(SEGMENT_SIZE - 1))

/* �����β��ַ��	*/
#define _N_TXTENDADDR(x) (N_TXTADDR(x)+(x).a_text)

/* ���ݶο�ʼ��ַ��
 * ����ļ���OMAGIC���͵ģ���ô���ݶξ�ֱ�ӽ������κ��档����Ļ����ݶε�ַ�Ӵ���
 * �κ���α߽翪ʼ��1KB�߽���룩������ZMAGIC���͵��ļ���	*/
#ifndef N_DATADDR
#define N_DATADDR(x) \
(N_MAGIC(x)==OMAGIC? (_N_TXTENDADDR(x)) \
 : (_N_SEGMENT_ROUND (_N_TXTENDADDR(x))))
#endif

/* Address of bss segment in memory after it is loaded.	*/
/* bss �μ��ص��ڴ��Ժ�ĵ�ַ */
/* δ��ʼ�����ݶ�bbsλ�����ݶκ��棬�������ݶΡ�	*/
#ifndef N_BSSADDR
#define N_BSSADDR(x) (N_DATADDR(x) + (x).a_data)
#endif

/* ��110��185���ǵ�2���֡���Ŀ���ļ��еķ��ű������ز�������ж����˵����
 * a.outĿ���ļ��з��ű���ṹ�����ű���¼�ṹ�����μ���������ϸ˵����	*/

/* nlist �ṹ��	*/
#ifndef N_NLIST_DECLARED
struct nlist
{
	union
	{
		char *n_name;
		struct nlist *n_next;
		long n_strx;
	} n_un;
	unsigned char n_type;	/* ���ֽڷֳ�3���ֶΣ�146��154������Ӧ�ֶε������롣	*/
	char n_other;
	short n_desc;
	unsigned long n_value;
};
#endif

/* ���涨��nlist�ṹ��n��type�ֶ�ֵ�ĳ������š�	*/
#ifndef N_UNDF
#define N_UNDF	0
#endif
#ifndef N_ABS
#define N_ABS	2
#endif
#ifndef N_TEXT
#define N_TEXT	4
#endif
#ifndef N_DATA
#define N_DATA	6
#endif
#ifndef N_BSS
#define N_BSS	8
#endif
#ifndef N_COMM
#define N_COMM	18
#endif
#ifndef N_FN
#define N_FN	15
#endif

/* ����3������������nlist�ṹ��n_type�ֶε������루�˽��̱�ʾ����	*/
#ifndef N_EXT
#define N_EXT	1		/* 0x01(0b0000��0001)�����Ƿ����ⲿ�ģ�ȫ�ֵģ���	*/
#endif
#ifndef N_TYPE			/* 0xle(0b0001,1110)���ŵ�����λ��	*/
#define N_TYPE	036
#endif
#ifndef N_STAB			/* STAB �����ű����ͣ�Symbol table types����	*/
#define N_STAB	0340	/* 0xe0(0blll0,0000)�⼸���������ڷ��ŵ�������	*/
#endif

/* The following type indicates the definition of a symbol as being
  an indirect reference to another symbol.	The other symbol
  appears as an undefined reference, immediately following this symbol.
 
  Indirection is asymmetrical.	The other symbol's value will be used
  to satisfy requests for the indirect symbol, but not vice versa.
  If the other symbol does not have a definition, libraries will
  be searched to find a definition.	*/
/* ���������ָ���˷��ŵĶ�����Ϊ����һ�����ŵļ�����á�
* ���Ӹ÷��ŵ�����
* �ķ��ų���Ϊδ��������á�
*
* ������ǲ��ԳƵġ�
*�������ŵ�ֵ�������������ӷ��ŵ����󣬵���֮��Ȼ��
* ����������Ų�û�ж��壬����������Ѱ��һ������
*/
#define N_INDR 0xa

/* The following symbols refer to set elements.
  All the N_SET[ATDB] symbols with the same name form one set.
  Space is allocated for the set in the text section, and each set
  element's value is stored into one word of the space.
  The first word of the space is the length of the set (number of elements).
 
  The address of the set is made into an N_SETV symbol
  whose name is the same as the name of the set.
  This symbol acts like a N_DATA global symbol
  in that it can satisfy undefined external references.	*/
/* ����ķ����뼯��Ԫ���йء�
* ���о�����ͬ����N_SET[ATDB]�ķ���
* �γ�һ�����ϡ�
* �ڴ��벿������Ϊ���Ϸ����˿ռ䣬����ÿ������Ԫ��
* ��ֵ�����һ���֣�word���Ŀռ䡣
* �ռ�ĵ�һ���ִ��м��ϵĳ��ȣ�����Ԫ����Ŀ����
* ���ϵĵ�ַ������һ��N_SETV ���ţ����������뼯��ͬ����
* ������δ������ⲿ���÷��棬�÷��ŵ���Ϊ��һ��N_DATA ȫ�ַ��š�*/

/* These appear as input to LD, in a .o file.	*/
/* ������Щ������Ŀ���ļ�������Ϊ���ӳ���LD �����롣	*/
#define	N_SETA	0x14		/* Absolute set element symbol */	/* ���Լ���Ԫ�ط��� */
#define	N_SETT	0x16		/* Text set element symbol */		/* ���뼯��Ԫ�ط��� */
#define	N_SETD	0x18		/* Data set element symbol */		/* ���ݼ���Ԫ�ط��� */
#define	N_SETB	0x1A		/* Bss set element symbol */		/* Bss ����Ԫ�ط��� */

/* This is output from LD.	*/	/* ������LD �������	*/
#define N_SETV	0x1C	/* Pointer to set vector in data area.	*/ /* ָ���������м���������*/

#ifndef N_RELOCATION_INFO_DECLARED

/* This structure describes a single relocation to be performed.
  The text-relocation section of the file is a vector of these structures,
  all of which apply to the text section.
  Likewise, the data-relocation section applies to the data section.	*/
/* ����Ľṹ����ִ��һ���ض�λ�Ĳ�����
* �ļ��Ĵ����ض�λ��������Щ�ṹ��һ��������������Щ�����ڴ��벿�֡�
* ���Ƶأ������ض�λ�������������ݲ��֡�*/

/* a.outĿ���ļ��д���������ض�λ��Ϣ�ṹ��	*/
struct relocation_info
{
	/* Address (within segment) to be relocated.	*/
	/* ������Ҫ�ض�λ�ĵ�ַ��	*/
	int r_address;
	/* The meaning of r_symbolnum depends on r_extern.	*/
	/* r_symbolnum �ĺ�����r_extern �йء�	*/
	unsigned int r_symbolnum:24;
	/* Nonzero means value is a pc-relative offset
	  and it should be relocated for changes in its own address
	  as well as for changes in the symbol or section specified.	*/
	/* ������ζ��ֵ��һ��pc��ص�ƫ��ֵ����������Լ���ַ�ռ��Լ����Ż�ָ���Ľڸı�ʱ����Ҫ���ض�λ */
	unsigned int r_pcrel:1;
	/* Length (as exponent of 2) of the field to be relocated.
	  Thus, a value of 2 indicates 1<<2 bytes.	*/
	/* ��Ҫ���ض�λ���ֶγ��ȣ���2�Ĵη�������ˣ���ֵ��2���ʾ1?2�ֽ�����*/
	unsigned int r_length:2;
	/* 1 => relocate with value of symbol.
		r_symbolnum is the index of the symbol
	 in file's the symbol table.
	  0 => relocate with the address of a segment.
		r_symbolnum is N_TEXT, N_DATA, N_BSS or N_ABS
	 (the N_EXT bit may be set also, but signifies nothing).	*/
	/* 1 => �Է��ŵ�ֵ�ض�λ��
	*	r_symbolnum ���ļ����ű��з��ŵ�������
	*	0 => �Զεĵ�ַ�����ض�λ��
	*	r_symbolnum ��N_TEXT��N_DATA��N_BSS ��N_ABS
	*	(N_EXT ����λҲ���Ա����ã����Ǻ�������)��
	*/
	unsigned int r_extern:1;
	/* Four bits that aren't used, but when writing an object file
	  it is desirable to clear them.	*/
	/* û��ʹ�õ�4 ������λ�����ǵ�����дһ��Ŀ���ļ�ʱ��ý����Ǹ�λ����*/
	unsigned int r_pad:4;
};
#endif /* no N_RELOCATION_INFO_DECLARED.	*/


#endif /* __A_OUT_GNU_H__ */