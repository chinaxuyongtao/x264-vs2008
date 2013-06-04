/*	$NetBSD: getopt_long.c,v 1.15 2002/01/31 22:43:40 tv Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Dieter Baron and Thomas Klausner.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * x264 --crf 22 --profile main --tune animation --preset medium --b-pyramid none -o test.mp4 oldFile.avi
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>

#define REPLACE_GETOPT

#define _DIAGASSERT(x) do {} while (0)	/* ѭ��ִֻ��һ�Σ��ڶ�����ʱ�����б�Ҫ���ȷ�ʹ����if��䣩��Ҫʹ��do{��}while(0)������������� */
										/* http://wmnmtm.blog.163.com/blog/static/38245714201172921555477/ */
#ifdef REPLACE_GETOPT
	#ifdef __weak_alias
	__weak_alias(getopt,_getopt)		/* weak:����;alias:���� */
	#endif
int opterr = 1;		/* if error message should be printed �Ƿ��ӡ������Ϣ */
int optind = 1;		/* index into parent argv vector �������������� */
int optopt = '?';	/* character(����) checked for validity(��Ч) */
int optreset;		/* reset(����) getopt */
char *optarg;		/* argument(����) associated(��صġ��������) with option(ѡ��) */
#endif

#ifdef __weak_alias
__weak_alias(getopt_long,_getopt_long)
#endif

char *__progname = "x264";

#define IGNORE_FIRST	(*options == '-' || *options == '+')	/* IGNORE:���� */
#define PRINT_ERROR	((opterr) && ((*options != ':') || (IGNORE_FIRST && options[1] != ':')))	/* ��ӡ���� */

#define IS_POSIXLY_CORRECT (getenv("POSIXLY_INCORRECT_GETOPT") == NULL)	/* POSIX:�ַ��� */

#define PERMUTE         (!IS_POSIXLY_CORRECT && !IGNORE_FIRST)	/* PERMUTE:�ı�˳�� */
/* XXX: GNU ignores PC if *options == '-' */
#define IN_ORDER        (!IS_POSIXLY_CORRECT && *options == '-')

/* return values ����ֵ */
#define	BADCH	(int)'?'
#define	BADARG		((IGNORE_FIRST && options[1] == ':') \
			 || (*options == ':') ? (int)':' : (int)'?')
#define INORDER (int)1	/* INORDER:��˳�� */

static char EMSG[1];

static int getopt_internal (int, char * const *, const char *);
static int gcd (int, int);
static void permute_args (int, int, int, char * const *);	/* permute�ı�˳�� */

static char *place = EMSG; /* optionѡ�� letter��ĸ processing(����)����, �ӹ� */

/* XXX: set optreset to 1 rather than these two (rather than :���䡭�����硭)*/
static int nonopt_start = -1; /* first non option argument (for permute) */
static int nonopt_end = -1;   /* first option after non options (for permute���иı�) */

/* ������Ϣ Error messages */
static const char recargchar[] = "option requires an argument(ѡ��Ҫ��һ������) -- %c";		/* ������Ϣ��ѡ��Ҫ��һ������ -- */
static const char recargstring[] = "option requires an argument(ѡ��Ҫ��һ������) -- %s";	/* ������Ϣ��ѡ��Ҫ��һ������ */
static const char ambig[] = "ambiguous option(���������ѡ��) -- %.*s";						/* ������Ϣ�����������ѡ�� */
static const char noarg[] = "option doesn't take an argument(ѡ���Ҫһ������) -- %.*s";	/* ������Ϣ��ѡ���Ҫһ������ */
static const char illoptchar[] = "unknown option(δ֪ѡ��) -- %c";							/* ������Ϣ��δ֪ѡ�� */
static const char illoptstring[] = "unknown option(δ֪ѡ��) -- %s";						/* ������Ϣ��δ֪ѡ�� */

static void _vwarnx(const char *fmt, va_list ap)
{
  (void)fprintf(stderr, "�ӷ�_%s: ", __progname);	/* char *__progname = "x264_";(��getopt.c�ļ�ǰ��) */
  if (fmt != NULL)								/* ������Ϣ != NULL */
    (void)vfprintf(stderr, fmt, ap);			/* ��ʽ�������������ָ������������ */
  (void)fprintf(stderr, "\n");					/* ��ӡ������Ϣ */
}
												/*
												������: vfprintf
											������ ��: ��ʽ�������������ָ������������
											������ ��: int vfprintf(FILE *stream, char *format, va_list param);
											��������˵����vfprintf()����ݲ���format�ַ�����ת������ʽ�����ݣ�Ȼ�󽫽�����������streamָ�����ļ��У�ֱ�������ַ�����������\0����Ϊֹ�����ڲ���format�ַ����ĸ�ʽ��� ��printf������
											��������ֵ���ɹ��򷵻�ʵ��������ַ�����ʧ���򷵻�-1������ԭ�����errno�С�
												���ϣ�http://baike.baidu.com/view/1081188.htm
												*/

static void warnx(const char *fmt, ...)					
{
  va_list ap;			/* typedef char *  va_list; Ҳ������һ��struct (typedef struct {char *a0; int offset; } va_list;) ��STDIO.H�� */
  va_start(ap, fmt);	/* start:������va_start��ʼ������va_end��β */
  _vwarnx(fmt, ap);		/* ap�ǵ�ǰ������ָ�� */
  va_end(ap);			/* end	:������va_end��β����apָ����ΪNULL */
}	//�ں�����������һ��va_list��Ȼ����va_start��������ȡ�����б��еĲ�����ʹ����Ϻ����va_end()������
												/*
												http://wmnmtm.blog.163.com/blog/static/382457142011729112244313/ 
												va_startʹargpָ���һ����ѡ������
												va_arg���ز����б��еĵ�ǰ������ʹargpָ������б��е���һ��������va_end��argpָ����ΪNULL��
												�������ڿ��Զ�α�����Щ���������Ƕ�������va_start��ʼ������va_end��β��
												*/

/*
 * Compute(����) the greatest(����) common( ��ͬ��) divisor(����) of a and b.
 * ����a��b�����Լ��
 * ��Լ������ơ��������������Ǽ�������ͬʱ�������������������һ������ͬʱ�Ǽ���������Լ�������������Ϊ���ǵġ���Լ��������Լ�������ĳ�Ϊ���Լ��
 * http://wmnmtm.blog.163.com/blog/static/382457142011730254697/
 */
static int gcd(a, b)
	int a;
	int b;
{
	int c;

	c = a % b;			/* ��ģ����a/b������ */
	while (c != 0) {	/* ��a����b�������� */
		a = b;
		b = c;
		c = a % b;
	}

	return b;
}

/*
 * Exchange(����) the block from nonopt_start to nonopt_end with the block
 * from nonopt_end to opt_end (keeping the same order of arguments in each block).
 * 
 */
static void
permute_args(panonopt_start, panonopt_end, opt_end, nargv)	/* permute:���иı�,����ת�� */
	int panonopt_start;
	int panonopt_end;
	int opt_end;
	char * const *nargv;
{
	int cstart, cyclelen, i, j, ncycle, nnonopts, nopts, pos;
	char *swap;

	_DIAGASSERT(nargv != NULL);

	/*
	 * compute lengths of blocks and number and size of cycles(����)
	 */
	nnonopts = panonopt_end - panonopt_start;
	nopts = opt_end - panonopt_end;
	ncycle = gcd(nnonopts, nopts);	/* �����Լ�� */
	cyclelen = (opt_end - panonopt_start) / ncycle;

	for (i = 0; i < ncycle; i++) {
		cstart = panonopt_end+i;
		pos = cstart;
		for (j = 0; j < cyclelen; j++) {
			if (pos >= panonopt_end)
				pos -= nnonopts;
			else
				pos += nopts;
			swap = nargv[pos];
			/* LINTED const cast */
			((char **) nargv)[pos] = nargv[cstart];
			/* LINTED const cast */
			((char **)nargv)[cstart] = swap;
		}
	}
}

/*
 * getopt_internal --
 *	Parse argc/argv argument vector.  Called by user level routines�����.
 *  Returns -2 if -- is found (can be long option or end of options marker).
 *  http://blog.csdn.net/yui/article/details/5669922
 */
static int
getopt_internal(nargc, nargv, options)	/* ע������������еĲ���ûָ�����ͣ����������ⶨ��� */
	int nargc;							//��Ӧ��main()�Ӳ���ϵͳ���ĵ�1��������ѡ�����
	char * const *nargv;				//��Ӧ��main()�Ӳ���ϵͳ���ĵ�2��������������ѡ���ַ���
	const char *options;
{

	char *oli;				/* option(ѡ��) letter(��ĸ) list(�б�) index(����) */
	int optchar;			/* ѡ���ַ� */

	_DIAGASSERT(nargv != NULL);
	_DIAGASSERT(options != NULL);
	//printf("\n(extras\getopt.c)getopt_internal���������ã��ڴ˴� options=%s  \n",options);//zjh
	optarg = NULL;

	/*
	 * XXX Some programs (like rsyncdʹ��) expectԤ��; Ԥ�� to be able to
	 * XXX re-initialize��ʼ�� optind to 0 and have getopt_long(3)
	 * XXX properly��ȷ�� function again.  Work around this braindamage������.
	 */
	if (optind == 0)	//ѡ��˳��ţ��ʼ��Ϊ1�������ǵ�����0��
		optind = 1;		//��ʼ��1�������Ǵ�0����

	if (optreset)
		nonopt_start = nonopt_end = -1;
start:
	if (optreset || !*place) {		/* ����ɨ��ָ��update scanning pointer */
		optreset = 0;
		if (optind >= nargc) {          /* ���ѡ����Ŵ��ڵ���ѡ��ĸ�����˵���������ˡ����������Ľ�β end of argument vector */
			place = EMSG;
			if (nonopt_end != -1) {
				/* do permutation����, if we have to */
				permute_args(nonopt_start, nonopt_end,  optind, nargv);
				optind -= nonopt_end - nonopt_start;
			}
			else if (nonopt_start != -1) {
				/*
				 * If we skipped non-options, set optind
				 * to the first of them.
				 */
				optind = nonopt_start;
			}
			nonopt_start = nonopt_end = -1;
			return -1;
		}
		if ((*(place = nargv[optind]) != '-')
		    || (place[1] == '\0')) {    /* found non-option */
			place = EMSG;
			if (IN_ORDER) {
				/*
				 * GNU extension��չ:
				 * return non-option as argument to option 1
				 */
				optarg = nargv[optind++];
				return INORDER;
			}
			if (!PERMUTE) {
				/*
				 * if no permutation���� wanted, stop parsing
				 * at first non-option
				 */
				return -1;
			}
			/* do permutation���� */
			if (nonopt_start == -1)
				nonopt_start = optind;
			else if (nonopt_end != -1) {
				permute_args(nonopt_start, nonopt_end,
				    optind, nargv);
				nonopt_start = optind -
				    (nonopt_end - nonopt_start);
				nonopt_end = -1;
			}
			optind++;
			/* process���� next argument */
			goto start;//goto ��ʼ
		}
		if (nonopt_start != -1 && nonopt_end == -1)
			nonopt_end = optind;
		if (place[1] && *++place == '-') {	/* found "--" */
			place++;
			return -2;
		}
	}
	if ((optchar = (int)*place++) == (int)':' ||
	    (oli = strchr(options + (IGNORE_FIRST ? 1 : 0), optchar)) == NULL) {
		/* option letter��ĸ unknown or ':' */
		if (!*place)
			++optind;
		if (PRINT_ERROR)
			warnx(illoptchar, optchar);
		optopt = optchar;
		return BADCH;
	}
	if (optchar == 'W' && oli[1] == ';') {		/* -W long-option */
		/* XXX: what if no long options provided (called by getopt)? */
		if (*place)
			return -2;

		if (++optind >= nargc) {	/* no arg */
			place = EMSG;
			if (PRINT_ERROR)
				warnx(recargchar, optchar);
			optopt = optchar;
			return BADARG;
		} else				/* white space */
			place = nargv[optind];
		/*
		 * Handle -W arg the same as --arg (which causes getopt to
		 * stop parsing).
		 */
		return -2;
	}
	if (*++oli != ':') {			/* doesn't take argument */
		if (!*place)
			++optind;
	} else {				/* takes (optional) argument */
		optarg = NULL;
		if (*place)			/* no white space */
			optarg = place;
		/* XXX: disable test for :: if PC? (GNU doesn't) */
		else if (oli[1] != ':') {	/* arg not optional */
			if (++optind >= nargc) {	/* no arg */
				place = EMSG;
				if (PRINT_ERROR)
					warnx(recargchar, optchar);
				optopt = optchar;
				return BADARG;
			} else
				optarg = nargv[optind];
		}
		place = EMSG;
		++optind;
	}
	/* dump back option letter */
	return optchar;
}

#ifdef REPLACE_GETOPT
/*
 * getopt --
 *	Parse argc/argv argument vector.
 *
 * [eventually(���) this will replace the real getopt]
 */
int
getopt(nargc, nargv, options)
	int nargc;
	char * const *nargv;
	const char *options;
{
	int retval;

	_DIAGASSERT(nargv != NULL);//???????
	_DIAGASSERT(options != NULL);

	if ((retval = getopt_internal(nargc, nargv, options)) == -2) {
		++optind;
		/*
		 * We found an option (--), so if we skipped non-options,
		 * we have to permute.
		 */
		if (nonopt_end != -1) {
			permute_args(nonopt_start, nonopt_end, optind,
				       nargv);
			optind -= nonopt_end - nonopt_start;
		}
		nonopt_start = nonopt_end = -1;
		retval = -1;
	}
	return retval;
}
#endif

/*
 * getopt_long --
 * Parse argc/argv argument vector.
 * ��x264.c�еĵ��ô������£�
 * int c = getopt_long( argc, argv, "8A:B:b:f:hI:i:m:o:p:q:r:t:Vvw",long_options, &long_options_index); 
*/
int getopt_long(nargc, nargv, options, long_options, idx)//��1���͵�2����������Ӧmain()�Ӳ���ϵͳ���Ĳ���
	int nargc;										//��3��������ð�ŷָ���ַ���,��4��������option long_options[]�ṹ�����飬��5��������
	char * const *nargv;							//��5��������&long_options_index�����������
	const char *options;/* �ַ��� */
	const struct option *long_options;/* option���͵�ָ�� */
	int *idx;

{
	int retval;
	int ti;//zjh
	char *tm;
	tm = nargv;
	_DIAGASSERT(nargv != NULL);
	_DIAGASSERT(options != NULL);
	_DIAGASSERT(long_options != NULL);
	/* idx may be NULL */

	/*
	printf("getopt_long����������,��������1������nargc=%d \n",nargc);//zjh
	for (ti=0;ti<nargc;ti++)
	{
		printf("getopt_long����������,��������2������nargv=%s \n",*nargv);//zjh
		nargv++;
	}
	*/
//	nargv = tm;//zjh	
//	printf("getopt_long����������,��������3������options=%s \n",options);//zjh

	if ((retval = getopt_internal(nargc, nargv, options)) == -2) //internal:�ڲ���//ǰ����������Ӧ��main(...)����������Ϊð�ŷָ����ַ���
	{
		char *current_argv, *has_equal;
		size_t current_argv_len;
		int i, match;

		current_argv = place;
		match = -1;

		optind++;
		place = EMSG;

		if (*current_argv == '\0') {		/* found "--" */
			/*
			 * We found an option (--), so if we skipped
			 * non-options, we have to permute.
			 */
			if (nonopt_end != -1) {
				permute_args(nonopt_start, nonopt_end,
				    optind, nargv);
				optind -= nonopt_end - nonopt_start;
			}
			nonopt_start = nonopt_end = -1;
			return -1;
		}
		if ((has_equal = strchr(current_argv, '=')) != NULL) {
			/* argument found (--option=arg) */
			current_argv_len = has_equal - current_argv;
			has_equal++;
		} else
			current_argv_len = strlen(current_argv);

		for (i = 0; long_options[i].name; i++) {//
			/* ���ҳ�ѡ�� find matching long option */
			if (strncmp(current_argv, long_options[i].name,// ���Ƚ�;˵��:�Ƚ��ַ���str1��str2�Ĵ�С�����str1С��str2������ֵ��<0����֮���str1����str2������ֵ��>0�����str1����str2������ֵ��=0��maxlenָ����str1��str2�ıȽϵ��ַ������˺������ܼ��Ƚ��ַ���str1��str2��ǰmaxlen���ַ�
			    current_argv_len))//ע��for���������м����������ƽ���ıȽϣ�long_options[i].nameΪ��ͽ���ѭ��
				continue;//�����ַ������ʱ����0��ִ�к�����䣬�����ʱ�������´�ѭ����������䱻������

			if (strlen(long_options[i].name) ==//��ѡ���name����==��ǰ�����ĳ���(��ΪҪһ������鴦��һ�Ѳ������ֵ����������)
			    (unsigned)current_argv_len) {//��0�������ͳ�������Ϊ�棬����Ҳ�Ƿ�0��Ҳ�����������������Թ���http://wmnmtm.blog.163.com/blog/static/38245714201181333128280/
				/* exact׼ȷ��, ȷ�е� matchƥ�� */
				match = i;
				break;
			}
			if (match == -1)		/* partial����ȫ�� matchƥ�� */
				match = i;
			else {
				/* ambiguous(���������; ģ�����ɵ�, ���������) abbreviation(��д; ��д��) */
				if (PRINT_ERROR)
					warnx(ambig, (int)current_argv_len,
					     current_argv);
				optopt = 0;
				return BADCH;
			}
		}
		if (match != -1) {			/* option found */
			if (long_options[match].has_arg == no_argument
			    && has_equal) {
				if (PRINT_ERROR)
					warnx(noarg, (int)current_argv_len,
					     current_argv);
				/*
				 * XXX: GNU sets optopt to val regardless of
				 * flag
				 */
				if (long_options[match].flag == NULL)
					optopt = long_options[match].val;
				else
					optopt = 0;
				return BADARG;
			}
			if (long_options[match].has_arg == required_argument ||
			    long_options[match].has_arg == optional_argument) {
				if (has_equal)
					optarg = has_equal;
				else if (long_options[match].has_arg ==
				    required_argument) {
					/*
					 * optional argument doesn't use
					 * next nargv
					 */
					optarg = nargv[optind++];
				}
			}
			if ((long_options[match].has_arg == required_argument)
			    && (optarg == NULL)) {
				/*
				 * Missing argument; leading ':'
				 * indicates no error should be generated
				 */
				if (PRINT_ERROR)
					warnx(recargstring, current_argv);
				/*
				 * XXX: GNU sets optopt to val regardless
				 * of flag
				 */
				if (long_options[match].flag == NULL)
					optopt = long_options[match].val;
				else
					optopt = 0;
				--optind;
				return BADARG;
			}
		} else {			/* unknown option */
			if (PRINT_ERROR)
				warnx(illoptstring, current_argv);
			optopt = 0;
			return BADCH;
		}
		if (long_options[match].flag) /* �ṹ��ĵ�3���ֶ� != 0 */
		{
			*long_options[match].flag = long_options[match].val;
			retval = 0;
		} else
			retval = long_options[match].val;	/* �ṹ������ĵڼ���Ԫ��(�Ǹ��ṹ��)��val�ֶ� */
		if (idx)
			*idx = match;//���������,idx�Ǵ�x264.c�д�����ָ�룬��������ã���ԭֵ�������ˣ����Ƿ�����ȥ��
	}
	return retval;
}
