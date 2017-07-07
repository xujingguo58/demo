#ifndef _STDARG_H
#define _STDARG_H

typedef char *va_list;		/* ����va_list ��һ���ַ�ָ�����͡�	*/

/* Amount of space required in an argument list for an arg of type TYPE.
TYPE may alternatively be an expression whose type is used. */
/* �������������ΪTYPE ��arg �����б���Ҫ��Ŀռ�������
 * TYPE Ҳ������ʹ�ø����͵�һ�����ʽ */

/* ������䶨����ȡ�����TYPE ���͵��ֽڳ���ֵ����int ����(4)�ı�����	*/
#define __va_rounded_size(TYPE) \
(((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

/* ����������ʼ��ָ��AP��ʹ��ָ�򴫸������Ŀɱ������ĵ�һ��������
 * �ڵ�һ�ε���va_arg��va_end֮ǰ���������ȵ���var_start�ꡣ����LASTARG�Ǻ�������
 * �����ұ߲����ı�ʶ��,����...����ߵ�һ����ʶ����AP�ǿɱ���������ָ�룬LASTARG����
 * ��һ��ָ���Ĳ�����&(LASTARG)����ȡ���ַ������ָ�룩�����Ҹ�ָ�����ַ����͡�����
 * LASTARG�Ŀ��ֵ��AP���ǿɱ�������е�һ��������ָ�롣�ú�û�з���ֵ��
 * ��17���ϵĺ�����builtin��saveregsO����gcc�Ŀ����libgcc2.c �ж���ģ����ڱ����
 * ���������˵���μ� gcc �ֲ� ��Target Description Macros�� ���� ��Implementing
 * the Varargs Macros����С�ڡ�
 */
#ifndef __sparc__
#define va_start(AP, LASTARG) \
(AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))
#else
#define va_start(AP, LASTARG) \
(__builtin_saveregs (), \
AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))
#endif

/* ����ú����ڱ����ú������һ���������ء�va_end �����޸�AP ʹ�������µ���	*/
/* va_start ֮ǰ���ܱ�ʹ�á�va_end ������va_arg �������еĲ������ٱ����á�	*/
void va_end (va_list);		/* Defined in gnulib *//* ��gnulib �ж��� */
#define va_end(AP)

/* �����������չ���ʽʹ������һ�������ݲ���������ͬ�����ͺ�ֵ��
 * v_arg�����չ�ɺ��������б�����һ�����������ͺ�ֵ��APӦ����va_start��ʼ����
 * va_list AP��ͬ��ÿ�ε���va_argʱ�����޸�AP��ʹ����һ��������ֵ�����ء�TYPE
 * ��һ������������va_start��ʼ��֮�󣬵�1�ε���va_arg�᷵��LASTARGָ��������
 * �Ĳ���ֵ�����ĵ��û᷵����������ֵ��
 */
#define va_arg(AP, TYPE) \
(AP += __va_rounded_size (TYPE), \
*((TYPE *) (AP - __va_rounded_size (TYPE))))

#endif /* _STDARG_H */
