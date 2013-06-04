/*****************************************************************************
 * common.h: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: common.h,v 1.1 2004/06/03 19:27:06 fenrir Exp $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#ifndef _COMMON_H
#define _COMMON_H 1

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif
#include <stdarg.h>
#include <stdlib.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#define X264_VERSION "" // no configure script for msvc
#endif

/* threads */
#ifdef __WIN32__
#include <windows.h>
#define pthread_t               HANDLE
#define pthread_create(t,u,f,d) *(t)=CreateThread(NULL,0,f,d,0,NULL)
#define pthread_join(t,s)       { WaitForSingleObject(t,INFINITE); \
                                  CloseHandle(t); } 
#define HAVE_PTHREAD 1

#elif defined(SYS_BEOS)
#include <kernel/OS.h>
#define pthread_t               thread_id
#define pthread_create(t,u,f,d) { *(t)=spawn_thread(f,"",10,d); \
                                  resume_thread(*(t)); }
#define pthread_join(t,s)       { long tmp; \
                                  wait_for_thread(t,(s)?(long*)(s):&tmp); }
#define HAVE_PTHREAD 1

#elif defined(HAVE_PTHREAD)
#include <pthread.h>
#endif

/****************************************************************************
 * Macros
 ****************************************************************************/
#define X264_MIN(a,b) ( (a)<(b) ? (a) : (b) )//����Сֵ
#define X264_MAX(a,b) ( (a)>(b) ? (a) : (b) )//�����ֵ
#define X264_MIN3(a,b,c) X264_MIN((a),X264_MIN((b),(c)))//����������Сֵ
#define X264_MAX3(a,b,c) X264_MAX((a),X264_MAX((b),(c)))//�����������ֵ
#define X264_MIN4(a,b,c,d) X264_MIN((a),X264_MIN3((b),(c),(d)))//4��������Сֵ
#define X264_MAX4(a,b,c,d) X264_MAX((a),X264_MAX3((b),(c),(d)))//4���������ֵ
#define XCHG(type,a,b) { type t = a; a = b; b = t; }//����a��b
#define FIX8(f) ((int)(f*(1<<8)+.5))//fix:ȡ����������

#if defined(__GNUC__) && (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ > 0)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#define CHECKED_MALLOC( var, size )\
{\
    var = x264_malloc( size );\
    if( !var )\
    {\
        x264_log( h, X264_LOG_ERROR, "malloc failed\n" );\
        goto fail;\
    }\
}

#define X264_BFRAME_MAX 16	//B֡�����Ϊ16
#define X264_SLICE_MAX 4	//Ƭ/���������Ϊ4
#define X264_NAL_MAX (4 + X264_SLICE_MAX)	//4 + 4

/****************************************************************************
 * Includes
 ****************************************************************************/
#include "x264.h"
#include "bs.h"
#include "set.h"
#include "predict.h"
#include "pixel.h"
#include "mc.h"
#include "frame.h"
#include "dct.h"
#include "cabac.h"
#include "csp.h"
#include "quant.h"

/****************************************************************************
 * Generals functions
 ****************************************************************************/
/* x264_malloc : will do or emulate(ģ��) a memalign(�ڴ����?)
 * XXX you HAVE TO(����) use x264_free(����) for buffer(����) allocated(�ѷ����)
 * with x264_malloc
 * �������ר�õĺ���x264_free���ͷţ���Щ��x264_malloc��������Ļ���
 */
void *x264_malloc( int );
void *x264_realloc( void *p, int i_size );
void  x264_free( void * );

/* x264_slurp_file: malloc space for the whole file and read it */
char *x264_slurp_file( const char *filename );

/* mdate: return the current date in microsecond( һ�����֮һ��,΢��) */
int64_t x264_mdate( void );

/* x264_param2string: return a (malloced) string containing(����) most of
 * the encoding options */
char *x264_param2string( x264_param_t *p, int b_res );

/* log */
void x264_log( x264_t *h, int i_level, const char *psz_fmt, ... );

void x264_reduce_fraction( int *n, int *d );

/*
 * ���ƣ�
 * ������
 * ���أ�
 * ���ܣ�v������[i_min,i_max]�����ʱ������������߽�,�������ұ�ʱ�����������ұ߽磬��������£�����v
 * ˼·��ʵ���Ͼ��ǿ�����v��ֵ���ܳ�������[i_min,i_max]
 * ���ϣ�
			i_min.........i_max
		v	v		v		v	v   (v�ļ��ֿ���λ��ʾ��)

 * 
*/
static inline int x264_clip3( int v, int i_min, int i_max )
{
    return ( (v < i_min) ? i_min : (v > i_max) ? i_max : v );
}
//x264_clip3f��x264_clip3��float�汾������һ����ֻ���������������Ϊfloat������ֵҲ��float
static inline float x264_clip3f( float v, float f_min, float f_max )//float�������v��ֵ��Խ���߽�
{
    return ( (v < f_min) ? f_min : (v > f_max) ? f_max : v );
}

/*
 * ���ƣ�
 * ���ܣ�x264_median(...) �ڸ������������У��ҳ��м���Ǹ��� 
 * ������
 * ���أ�
 * ���ϣ�
 * ˼·����������ʱ��min��max������ÿ�αȽϵ���С��������һ��returnʱ��ֱ�Ӱ�����������������ȥ��С���������м����
 * 
*/
static inline int x264_median( int a, int b, int c )//median:���м��,ͨ���е��
{
    int min = a, max =a;
    if( b < min )
        min = b;
    else
        max = b;    /* no need to do 'b > max' (more consuming than always doing affectation) */
	//��������a��b�бȽϴ�С��С���Ǹ�����min��

	//�ѵ�3����c�����ϲ��е���С�����Ƚ�
    if( c < min )
        min = c;
    else if( c > max )
        max = c;

    return a + b + c - min - max;	//a + b + c - ��С�� - ���� -> �м��
}


/****************************************************************************
 *Ƭ_����//�Ϻ���飬��164ҳ
 ****************************************************************************/
enum slice_type_e
{
    SLICE_TYPE_P  = 0,//PƬ
    SLICE_TYPE_B  = 1,//BƬ
    SLICE_TYPE_I  = 2,//IƬ
    SLICE_TYPE_SP = 3,//SPƬ
    SLICE_TYPE_SI = 4//SIƬ
};

static const char slice_type_to_char[] = { 'P', 'B', 'I', 'S', 'S' };

typedef struct
{
    x264_sps_t *sps;//���в�����
    x264_pps_t *pps;//ͼ�������

    int i_type;		//[�Ϻ�ܣ�Page 164] �൱��slice_type��ָ��Ƭ�����ͣ�0123456789��ӦP��B��I��SP��SI 
    int i_first_mb;	//[�Ϻ�ܣ�Page 164] �൱�� first_mb_in_slice Ƭ�еĵ�һ�����ĵ�ַ��Ƭͨ������䷨Ԫ�����궨���Լ��ĵ�ַ��Ҫע����ǣ���֡������Ӧģʽ�£���鶼�ǳɶԳ��ֵģ���ʱ���䷨Ԫ�ر�ʾ���ǵڼ������ԣ���Ӧ�ĵ�һ��������ʵ��ַӦ����2*---
    int i_last_mb;	//���һ�����

    int i_pps_id;	//[�Ϻ�ܣ�Page 164] �൱�� pic_parameter_set_id ; ͼ��������������ţ�ȡֵ��Χ[0,255]

    int i_frame_num;	//[�Ϻ�ܣ�Page 164] �൱�� frame_num ; ֻ�е����ͼ���ǲο�֡ʱ������Я��������䷨Ԫ���ڽ���ʱ�������壬
						//[�Ϻ�ܣ�Page 164] �൱�� frame_num ; ÿ���ο�֡����һ������������frame_num��Ϊ���ǵı�ʶ����ָ���˸�ͼ��Ľ���˳��frame_num���ܴﵽ�����ֵ��ǰ�����в������еľ䷨Ԫ���Ƴ�����P164
						//Frame Numָʾͼ���еı���˳��POCָʾ��ͼ�����ʾ˳�� //��Ƶ������ÿһ��ͼ��Ҫ����˳����б��룬����H��264�д���˫��Ԥ�⣬���˳��Ͳ�һ������ʾ˳��Ϊ��ʹ�������������ؽ��벢��ʾ��H��264��ʹ��Frame Numָʾͼ���еı���˳��POCָʾ��ͼ�����ʾ˳��
    int b_field_pic;	//[�Ϻ�ܣ�Page 165] �൱�� field_pic_flag; ������Ƭ���ʶͼ�����ģʽ��Ψһһ���䷨Ԫ�ء���ν�ı���ģʽ��ָ��֡���롢�����롢֡������Ӧ���롣������䷨Ԫ��ȡֵΪ1ʱ���ڳ����룬0ʱΪ�ǳ����롣
    int b_bottom_field;	//[�Ϻ�ܣ�Page 166] �൱�� bottom_field_flag ;  =1ʱ��ʾ��ǰ��ͼ�������ڵ׳���=0ʱ��ʾ��ǰͼ�����ڶ���

    int i_idr_pic_id;   /* -1 if nal_type != 5 */ 
						//[�Ϻ�ܣ�Page 164] �൱��idr_pic_id ;IDRͼ��ı�ʶ����ͬ��IDRͼ���в�ͬ��idr_pic_idֵ��ֵ��ע����ǣ�IDRͼ�񲻵ȼ���Iͼ��ֻ������ΪIDRͼ���I֡��������䷨Ԫ�ء��ڳ�ģʽ�£�IDR֡������������ͬ��idr_pic_idֵ��idr_pic_id��ȡֵ��Χ��[0,65536]����frame_num���ƣ�������ֵ���������Χʱ��������ѭ���ķ�ʽ���¿�ʼ������

    int i_poc_lsb;		//[�Ϻ�ܣ�Page 166] �൱�� pic_order_cnt_lsb����POC�ĵ�һ���㷨�б��䷨Ԫ��������POCֵ����POC�ĵ�һ���㷨������ʽ�ش���POC��ֵ�������������㷨��ͨ��frame_num��ӳ��POC��ֵ��ע������䷨Ԫ�صĶ�ȡ������u(v)�����v��ֵ�����в������ľ䷨Ԫ��--+4�����صõ��ġ�ȡֵ��Χ��...
    int i_delta_poc_bottom;	//[�Ϻ�ܣ�Page 166] �൱�� delta_pic_order_cnt_bottom��������ڳ�ģʽ�£������е����������Ա�����Ϊһ��ͼ�������и��Ե�POC�㷨���ֱ������������POCֵ��Ҳ����һ������ӵ��һ��POCֵ������֡ģʽ��֡������Ӧģʽ�£�һ��ͼ��ֻ�ܸ���Ƭͷ�ľ䷨Ԫ�ؼ����һ��POCֵ������H.264�Ĺ涨���������п��ܳ��ֳ����������frame_mbs_only_flag��Ϊ1ʱ��ÿ��֡��֡������Ӧ��ͼ���ڽ���������ֽ�Ϊ���������Թ�����ͼ���еĳ���Ϊ�ο�ͼ�����Ե�ʱframe_mb_only_flag��Ϊ1ʱ��֡��֡������Ӧ

    int i_delta_poc[2]; //[�Ϻ�ܣ�Page 167] �൱�� delta_pic_order_cnt[0] , delta_pic_order_cnt[1]
    int i_redundant_pic_cnt;	//[�Ϻ�ܣ�Page 167] �൱�� redundant_pic_cnt;����Ƭ��id�ţ�ȡֵ��Χ[0,127]

    int b_direct_spatial_mv_pred;	//[�Ϻ�ܣ�Page 167] �൱�� direct_spatial_mv_pred��ָ����Bͼ���ֱ��Ԥ��ģʽ�£���ʱ��Ԥ�⻹���ÿռ�Ԥ�⣬1��ʾ�ռ�Ԥ�⣬0��ʾʱ��Ԥ�⡣ֱ��Ԥ��������94ҳ

    int b_num_ref_idx_override;		//[�Ϻ�ܣ�Page 167] �൱�� num_ref_idx_active_override_flag����ͼ��������У��������䷨Ԫ��ָ����ǰ�ο�֡������ʵ�ʿ��õĲο�֡����Ŀ����Ƭͷ����������Ծ䷨Ԫ�أ��Ը�ĳ�ض�ͼ���������ȡ�����䷨Ԫ�ؾ���ָ��Ƭͷ�Ƿ�����أ�����þ䷨Ԫ�ص���1�����������µ�num_ref_idx_10_active_minus1��...11...
    int i_num_ref_idx_l0_active;	//[�Ϻ�ܣ�Page 167] �൱�� num_ref_idx_10_active_minus1
    int i_num_ref_idx_l1_active;	//[�Ϻ�ܣ�Page 167] �൱�� num_ref_idx_11_active_minus1

    int b_ref_pic_list_reordering_l0;//���ڲο�ͼ���б�������?
    int b_ref_pic_list_reordering_l1;//���ڲο�ͼ���б�������?
    struct {
        int idc;
        int arg;
    } ref_pic_list_order[2][16];	 

    int i_cabac_init_idc;//[�Ϻ�ܣ�Page 167] �൱�� cabac_init_idc������cabac��ʼ��ʱ����ѡ��ȡֵ��ΧΪ[0,2]

    int i_qp;//
    int i_qp_delta;//[�Ϻ�ܣ�Page 167] �൱�� slice_qp_delta��ָ�����ڵ�ǰƬ�����к������������ĳ�ʼֵ
    int b_sp_for_swidth;////[�Ϻ�ܣ�Page 167] �൱�� sp_for_switch_flag
    int i_qs_delta;//[�Ϻ�ܣ�Page 167] �൱�� slice_qs_delta����slice_qp_delta�������ƣ�����SI��SP�У�����ʽ���㣺...

    /* deblocking filter */
    int i_disable_deblocking_filter_idc;//[�Ϻ�ܣ�Page 167] �൱�� disable_deblocking_filter_idc��H.264�涨һ�׿����ڽ������˶����ؼ���ͼ���и��߽���˲�ǿ�Ƚ���
    int i_alpha_c0_offset;
    int i_beta_offset;
	/*
	http://bbs.chinavideo.org/viewthread.php?tid=10671&extra=page%3D2
	disable_deblocking_filter_idc = 0�����б߽�ȫ�˲�
	disable_deblocking_filter_idc = 1�����б߽綼���˲�
	disable_deblocking_filter_idc = 2��Ƭ�߽粻�˲�
	*/
} x264_slice_header_t;	//x264_Ƭ_ͷ_
//x264���ı�����������Ϻ������˵�ı�����һЩ���룬x264�������ͷ��ţ�����i_��b_������ʡ����_flag����������д����_poc_��ʵ����pic_order_cnt_lsb��_pps_��ʵ��Ϊpic_parameter_set_id
//���ֽṹ��Ķ����ʽ������http://wmnmtm.blog.163.com/blog/static/38245714201181744856220/

/* From ffmpeg
 */
#define X264_SCAN8_SIZE (6*8)	//
#define X264_SCAN8_0 (4+1*8)	//

/*
����һ������任�õĲ��ұ�,�Ǹ�����,��0--23�任Ϊһ��8x8�����е�4x4 ��2��2x2 �Ŀ�ɨ��
http://wmnmtm.blog.163.com/blog/static/3824571420118175553244/
Ҳ����˵����������Ԫ�ص�ֵ��������Ϊ��Ż����±꣬������Ϊָ���������ģ�������һ�����������Ȳ��ֺ�ɫ�Ȳ��֣����Һ����������ǣ�����4x4��Ϊ��λ�ġ�
��ô����Ҫȥ������أ�����
*/
static const int x264_scan8[16+2*4] =	//16+8=24
{
    /* Luma���� */
    4+1*8, 5+1*8, 4+2*8, 5+2*8,	/* 12, 13, 20, 21 */
    6+1*8, 7+1*8, 6+2*8, 7+2*8,	/* 14, 15, 22, 23 */
    4+3*8, 5+3*8, 4+4*8, 5+4*8,	/* 28, 29, 36, 37 */
    6+3*8, 7+3*8, 6+4*8, 7+4*8,	/* 30, 31, 38, 39 */

    /* Cb */
    1+1*8, 2+1*8, /*  9, 10 */
    1+2*8, 2+2*8, /* 17, 18 */

    /* Cr */
    1+4*8, 2+4*8, /* 33, 34 */
    1+5*8, 2+5*8, /* 41, 42 */
};
/*
�任��������,��luma����,Ȼ��chroma(ɫ��) b chroma(ɫ��) r,���Ǵ�һ��2x2С�鿪ʼ,raster(��դ) scan
����L���ȵ�һ44��raster scane �ٵڶ�44�� scane
ÿһ�������ߺ��ϱ��ǿճ�����,�������left��top mb�� 4x4С���intra pre mode
*/

/*
   0 1 2 3 4 5 6 7
 0
 1   B B   L L L L
 2   B B   L L L L
 3         L L L L
 4   R R   L L L L
 5   R R
*/

typedef struct x264_ratecontrol_t   x264_ratecontrol_t;//ratecontrol.c(85):struct x264_ratecontrol_t
typedef struct x264_vlc_table_t     x264_vlc_table_t;

//x264_t�ṹ��ά����CODEC�������Ҫ��Ϣ
struct x264_t
{
    /* encoder parameters ( ���������� )*/
    x264_param_t    param;

	//һ��Ƭ��Ӧһ��x264_t(���붨��HAVE_PTHREADʱ�����ö��̺߳���)
    x264_t *thread[X264_SLICE_MAX];	//#define X264_SLICE_MAX 4	//Ƭ/���������Ϊ4
									//��x264_encoder_open�лᶯ̬����ṹ���ڴ棬������ַ���ڴ�����
									//    h->thread[0] = h;
									//	h->i_thread_num = 0;
									//	for( i = 1; i < h->param.i_threads; i++ )
									//		h->thread[i] = x264_malloc( sizeof(x264_t) );//Ϊ����
    /* bitstream output ( bit����� ) 
	 * x264_encoder_open����i_bitstream��̬����һ����ڴ棬�׵�ַ����p_bitstream
	 * дspsʱ���ȵ���x264_nal_start( h, NAL_SPS, NAL_PRIORITY_HIGHEST );
	*/
    struct
    {
        int         i_nal;				//out����x264_nal_t�������飬i_nalָʾ���ڲ����ڼ�������Ԫ��
        x264_nal_t  nal[X264_NAL_MAX]; 	//#define X264_NAL_MAX (4 + X264_SLICE_MAX)
        int         i_bitstream;        /* 1000000����� size of p_bitstream */
        uint8_t     *p_bitstream;       /* ���������nal������  will hold���� data for all nal
										 * ��x264_encoder_open�ж�̬�����ڴ棬��ַ��������
										*/
        bs_t        bs;	                /* common/bs.h�ﶨ�壬�洢�Ͷ�ȡ���ݵ�һ���ṹ��,5���ֶΣ�ָ���˿�ʼ�ͽ�β��ַ����ǰ��ȡλ�õ� */
    } out;

    /* frame number/poc ( ֡��� )*/
    int             i_frame;

    int             i_frame_offset; /* decoding only */
    int             i_frame_num;    /* decoding only */	//����ͼ���ʶ���������ǰͼ����һ��IDRͼ�� frame_num Ӧ����0����׼��76ҳ
    int             i_poc_msb;      /* decoding only */	//MSB �����Чλ
    int             i_poc_lsb;      /* decoding only */	//LSB �����Чλ
    int             i_poc;          /* decoding only */

    int             i_thread_num;   /* threads only */
    int             i_nal_type;     /* threads only */	//ָ����ǰNAL_unit�����͡�0δʹ�ã�1����������IDRͼ���Ƭ��2��3��4Ƭ����A��B��C��5IDRͼ���е�Ƭ��6������ǿ��Ϣ��Ԫ(SEI)��7���в�������8ͼ���������9�ֽ����10�����н�����11����������12��䡣�Ϻ��159ҳ
    int             i_nal_ref_idc;  /* threads only */ //nal_ref_idc��ָʾ��ǰNAL�����ȼ���ȡֵ��Χ[0,1,2,3]��ֵԽ�ߣ���ʾ��ǰNALԽ��Ҫ��Խ��Ҫ�����ܵ�������H.264�涨�����ǰNAL��һ�����в�������������һ��ͼ����������������ڲο�ͼ���Ƭ��Ƭ��������Ҫ�����ݵ�λʱ�����䷨Ԫ�ر������0�����Ǵ���0ʱ�����ȡ��ֵ����û�н�һ���Ĺ涨��ͨ��˫�����������ƶ����ԡ���nal_unit_type����5ʱ��nal_ref_idc����0��nal_unit_type����6��9��10��11��12ʱ��nal_ref_idc����0���Ϻ��159ҳ

    /* We use only one SPS(���в�����) and one PPS(ͼ�������) 
	   ����ֻʹ��һ�����в�������һ��ͼ�������
	*/
    x264_sps_t      sps_array[1];	//�ṹ������顣set.h�ﶨ��Ľṹ��
    x264_sps_t      *sps;
    x264_pps_t      pps_array[1];	//�ṹ������顣set.h�ﶨ��Ľṹ��
    x264_pps_t      *pps;
    int             i_idr_pic_id;

    int             (*dequant4_mf[4])[4][4]; /* [4][6][4][4] */
    int             (*dequant8_mf[2])[8][8]; /* [2][6][8][8] */
    int             (*quant4_mf[4])[4][4];   /* [4][6][4][4] */
    int             (*quant8_mf[2])[8][8];   /* [2][6][8][8] */
    int             (*unquant4_mf[4])[16];   /* [4][52][16] */
    int             (*unquant8_mf[2])[64];   /* [2][52][64] */

    uint32_t        nr_residual_sum[2][64];	/* residual:���� */
    uint32_t        nr_offset[2][64];		/* offset:ƫ�� */
    uint32_t        nr_count[2];

    /* Slice header (Ƭͷ��) */
    x264_slice_header_t sh;

    /* cabac(��Ӧ�Զ�Ԫ��������) context */
    x264_cabac_t    cabac;	/* �ṹ��,cabac.h�ﶨ�� */

    struct
    {
        /* Frames to be encoded (whose types have been decided(��ȷ��) ) */
        x264_frame_t *current[X264_BFRAME_MAX+3];//current���Ѿ�׼���������Ա����֡���������Ѿ�ȷ��
        /* Temporary(��ʱ��) buffer (frames types not yet decided) */
        x264_frame_t *next[X264_BFRAME_MAX+3];//next����δȷ�����͵�֡
        /* δʹ�õ�֡Unused frames */
        x264_frame_t *unused[X264_BFRAME_MAX+3];//unused���ڻ��ղ�ʹ�õ�frame�ṹ���Ա�����ٴ�ʹ��
        /* For adaptive(��Ӧ��) B decision(����) */
        x264_frame_t *last_nonb;

        /* frames used for reference +1 for decoding + sentinels (������,���) */
        x264_frame_t *reference[16+2+1+2];//ָ����ǰ���ڱ����ͼ��Ĳο�ͼ����������21���ο�ͼ��

        int i_last_idr; /* ����IDRͼ�����ţ����±�����һ��IDRʱ������´˱��� */

        int i_input;    /* ��ǰ�����֡������(������ļ��ж�ȡ������150֡��) */

        int i_max_dpb;  /* Number of frames allocated (����) in the decoded picture buffer */
        int i_max_ref0;
        int i_max_ref1;
        int i_delay;    //i_delay����Ϊ��B֡�������̸߳�����ȷ����֡�����ӳ٣��ڶ��߳������Ϊi_delay = i_bframe + i_threads - 1�����ж�B֡��������Ƿ��㹻��ͨ�������жϣ�h->frames.i_input <= h->frames.i_delay + 1 - h->param.i_threads�� /* Number of frames buffered for B reordering (��������) */
        int b_have_lowres;  /* Whether 1/2 resolution (�ֱ���) luma(����) planes(ƽ��) are being used */
    } frames;//ָʾ�Ϳ���֡������̵Ľṹ

    /* ��������ĵ�ǰ֡ current frame being encoded */
    x264_frame_t    *fenc;//��ŵ���YCbCr(420)��ʽ�Ļ��� http://wmnmtm.blog.163.com/blog/static/3824571420118188540701/

    /* ���ڱ��ؽ���֡ frame being reconstructed(�ؽ���) */
    x264_frame_t    *fdec;////��ŵ���YCbCr(420)��ʽ�Ļ��� http://wmnmtm.blog.163.com/blog/static/3824571420118188540701/

    /* references(�ο�) lists */
    int             i_ref0;
    x264_frame_t    *fref0[16+3];     /* ref list 0 */
    int             i_ref1;
    x264_frame_t    *fref1[16+3];     /* ref list 1 */
    int             b_ref_reorder[2];



    /* Current MB DCT coeffs(����ϵ��) */
    struct
    {
        DECLARE_ALIGNED( int, luma16x16_dc[16], 16 );
        DECLARE_ALIGNED( int, chroma_dc[2][4], 16 );
        // FIXME merge(�ϲ�) with union(���,����)
        DECLARE_ALIGNED( int, luma8x8[4][64], 16 );
        union
        {
            DECLARE_ALIGNED( int, residual_ac[15], 16 );
            DECLARE_ALIGNED( int, luma4x4[16], 16 );
        } block[16+8];
    } dct;

    /* MB table and cache(���ٻ���洢��) for current frame/mb */
    struct
    {
        int     i_mb_count;	/* number of mbs in a frame */ /* ��һ֡�еĺ������ */

        /* Strides (��,Խ) */
        int     i_mb_stride;//��16x16���Ϊ��ȵ�ͼ��Ŀ�� ������ͼ��һ���м�����飩
        int     i_b8_stride;
        int     i_b4_stride;

        /* 
		Current index ��ǰ��������
		���ϣ�http://wmnmtm.blog.163.com/blog/static/382457142011885411173/
		*/
        int     i_mb_x;//��ǰ�����X�������ϵ�λ��
        int     i_mb_y;//��ǰ�����Y�������ϵ�λ��
        int     i_mb_xy;//��ǰ����ǵڼ������
        int     i_b8_xy;//��8X8��ʱ���ǵڼ������ ��Ŀ����Ϊ�˼���������ڵĺ���λ�ã��ҵ����ں��������Ϣ���Դ������Ƶ�ǰ���Ĵ���ʽ��
        int     i_b4_xy;//��ͬ�ϣ�
        
        /* Search parameters (��������) */
        int     i_me_method;			//luma�˶����Ʒ���
        int     i_subpel_refine;		//�������˶��������� �˶����Ƶľ���
        int     b_chroma_me;			//ɫ���Ƿ�����˶�����
        int     b_trellis;				//trellis������
        int     b_noise_reduction;		//�Ƿ��� noise:����;reduction:���� /*����Ӧαä�� */

        /* Allowed qpel(�ķ�֮һӳ���) MV range to stay (����,ͣ��) within the picture + emulated(ģ��) edge (��) pixels */
        int     mv_min[2];	//MV�ķ�Χ
        int     mv_max[2];
        /* Subpel������ MV�˶� range��Χ for motion�˶� search����.
         * same mv_min/max but includes levels' i_mv_range. */
        int     mv_min_spel[2];	//�����صķ�Χ
        int     mv_max_spel[2];
        /* Fullpel MV range for motion search ȫ����mv��Χ���˶�����*/
        int     mv_min_fpel[2];	//�����ص�������Χ
        int     mv_max_fpel[2];

        /* neighboring MBs ���ں�����Ϣ �Ƿ����*/
        unsigned int i_neighbour;
        unsigned int i_neighbour8[4];       /* ??? neighbours�ڽ� of eachÿ�� 8x8 or 4x4 block�� that are available���õ� */
        unsigned int i_neighbour4[16];      /* ??? at the time the block is coded���� */
        int     i_mb_type_top;				/* ������ͣ��� */
        int     i_mb_type_left;				/* ������ͣ��� */		
        int     i_mb_type_topleft; 			/* ������ͣ����� */
        int     i_mb_type_topright; 		/* ������ͣ����� */

		/*		
		1 2 3 
		4 C 5
		6 7 8
		CΪ��ǰ��飬��������Χ�ĺ��������8��
		�ϡ������ϡ�����		
		*/

        /* mb table ���� */
        int8_t  *type;                      /* ָ��һ���洢�����Ѿ�ȷ��������͵ĺ�����ͱ�� mb type */
        int8_t  *qp;                        /* ͬ�ϣ����������Ǵ洢���������� mb qp */
        int16_t *cbp;                       /* ͬ�� �����Ǻ��в�ı��뷽ʽ mb cbp: 0x0?: luma, 0x?0: chroma, 0x100: luma dc, 0x0200 and 0x0400: chroma dc  (all set for PCM)*/
        int8_t  (*intra4x4_pred_mode)[7];   /* ÿ�����Ԥ��ģʽ intra4x4 pred mode. for non I4x4 set to I_PRED_4x4_DC(2) */
        uint8_t (*non_zero_count)[16+4+4];  /* ÿ�����ķ���ϵ�� nzc. for I_PCM set to 16 */
        int8_t  *chroma_pred_mode;          /* ÿ�����ɫ�ȵ�Ԥ��ģʽ chroma_pred_mode. cabac only. for non intra I_PRED_CHROMA_DC(0) */
        int16_t (*mv[2])[2];                /* ÿ��ʵ�ʵ�mv   mb mv. set to 0 for intra mb */
        int16_t (*mvd[2])[2];               /* ÿ�� �в� mb mv difference with predict. set to 0 if intra. cabac only */
        int8_t   *ref[2];                   /* �ο��б��ָ�� mb ref. set to -1 if non used (intra or Lx only) */
        int16_t (*mvr[2][16])[2];           /* 16x16 mv for eachÿ�� possible���ܵ� ref�ο� */
        int8_t  *skipbp;                    /* block pattern��ʽ for SKIP����; or DIRECTֱ�� (sub)mbs. B-frames + cabacǰ�Ĳο�֮��Ӧ�Զ�Ԫ�������� only */
        int8_t  *mb_transform_size;         /* (�任)transform_size_8x8_flag of each mb */
		/* ������ÿ����鶼�е� */

        /* current value ��ǰֵ */
        int     i_type;				//��ǰ��������
        int     i_partition;		//��ǰ���Ļ���ģʽ
        int     i_sub_partition[4];	//�Ӻ��Ļ���ģʽ
        int     b_transform_8x8;	//�Ƿ�8X8�仯

		//CBP��⣺http://wmnmtm.blog.163.com/blog/static/38245714201181611143750/ cbp���ڱ�ʾ��ǰ����Ƿ���ڷ���ֵ,cbpһ��6bit����2bit��ʾcbpc(2��cb��cr������һ��4x4���ACϵ����ȫΪ0��1��cb��cr������һ��2x2��DCϵ����ȫΪ0��0������ɫ��ϵ��ȫ0��,��4bit�ֱ��ʾ4��8x8���ȿ飬���д����һλ��ʼ��4λ�ֱ��Ӧ00��10��01��11λ�õ�8*8���ȿ顣���ĳλΪ1����ʾ�ö�Ӧ8*8���4��4*4����������һ����ϵ����ȫΪ0��

        int     i_cbp_luma;		//��ǰ������������ �в����ģʽ
        int     i_cbp_chroma;	//��ǰ����ɫ������ �в����ģʽ

        int     i_intra16x16_pred_mode;//��ǰ���֡��Ԥ��ģʽ ֡��16x16Ԥ��ģʽ
        int     i_chroma_pred_mode;//ɫ��Ԥ��ģʽ

        struct
        {
            /* space for p_fenc and p_fdec �ռ�for��ǰ����֡���ؽ�֡*/
#define FENC_STRIDE 16
#define FDEC_STRIDE 32
            DECLARE_ALIGNED( uint8_t, fenc_buf[24*FENC_STRIDE], 16 );
            DECLARE_ALIGNED( uint8_t, fdec_buf[27*FDEC_STRIDE], 16 );

            /* pointer over mb of the frame to be compressed */
            uint8_t *p_fenc[3];

            /* pointer over mb of the frame to be reconstrucated  */
            uint8_t *p_fdec[3];

            /* pointer over mb of the references */
            uint8_t *p_fref[2][16][4+2]; /* last: lN, lH, lV, lHV, cU, cV */
            uint16_t *p_integral[2][16];

            /* fref stride �ο�֡��ȣ�*/
            int     i_stride[3];
        } pic;

        /* cache ����Ԥ��ģʽѡ���ʱ����Ҫ�Ļ���ռ� */
        struct
        {
            /* real intra4x4_pred_mode if I_4X4 or I_8X8, I_PRED_4x4_DC if mb available, -1 if not */
            int     intra4x4_pred_mode[X264_SCAN8_SIZE];

            /* i_non_zero_count if availble else 0x80 */
            int     non_zero_count[X264_SCAN8_SIZE];

            /* -1 if unused, -2 if unavaible���Ի�õ� */
            int8_t  ref[2][X264_SCAN8_SIZE];

            /* 0 if non avaible */
            int16_t mv[2][X264_SCAN8_SIZE][2];
            int16_t mvd[2][X264_SCAN8_SIZE][2];

            /* 1 if SKIP���� or DIRECTֱ�ӵ�. set only for B-frames + CABAC */
            int8_t  skip[X264_SCAN8_SIZE];

            int16_t direct_mv[2][X264_SCAN8_SIZE][2];
            int8_t  direct_ref[2][X264_SCAN8_SIZE];

            /* number of neighbors (top and left) that used 8x8 dct */
            int     i_neighbour_transform_size;	//
            int     b_transform_8x8_allowed;	//
        } cache;

        /* ���ʿ��� �������� */
        int     i_qp;			/* current qp */
        int     i_last_qp;		/* ��һ�������� last qp */
        int     i_last_dqp;		/* ���������� last delta qp */
        int     b_variable_qp;	/*  whether�Ƿ� qp is allowed to vary�仯 per���� macroblock */
        int     b_lossless;		//�Ƿ��������
        int     b_direct_auto_read; /* take stats for --direct auto from the 2pass log */
        int     b_direct_auto_write; /* analyse���� directֱ�� modesģʽ, to use and/or save */

        /* B_direct and weighted��Ȩ�� predictionԤ�� */
        int     dist_scale_factor[16][16];//��ȨԤ��
        int     bipred_weight[16][16];
        /* maps fref1[0]'s ref indices into the current list0 */
        int     map_col_to_list0_buf[2]; // for negative�ܾ� indices(����,index�ĸ���)
        int     map_col_to_list0[16];
    } mb;

    /* ���ʿ��Ʊ���ʹ�� rate control encoding only */
    x264_ratecontrol_t *rc;//struct x264_ratecontrol_t,ratecontrol.c�ж���

    /* stats */
    struct
    {
        /* Current frame stats */
        struct
        {
            /* Headers bits (MV+Ref+MB Block Type */
            int i_hdr_bits;
            /* Texture bits (Intra/Predicted) */
            int i_itex_bits;
            int i_ptex_bits;
            /* ? */
            int i_misc_bits;
            /* MB type counts */
            int i_mb_count[19];
            int i_mb_count_i;
            int i_mb_count_p;
            int i_mb_count_skip;
            int i_mb_count_8x8dct[2];//�����_8x8DCT
            int i_mb_count_size[7];
            int i_mb_count_ref[16];
            /* Estimated (SATD) cost as Intra/Predicted frame */
            /* XXX: both omit the cost of MBs coded as P_SKIP */
            int i_intra_cost;
            int i_inter_cost;
            /* Adaptive direct mv pred */
            int i_direct_score[2];
        } frame;

        /* Cummulated stats */

        /* per(ÿ��) slice info */
        int     i_slice_count[5];
        int64_t i_slice_size[5];
        int     i_slice_qp[5];
        /* */
        int64_t i_sqe_global[5];
        float   f_psnr_average[5];
        float   f_psnr_mean_y[5];
        float   f_psnr_mean_u[5];
        float   f_psnr_mean_v[5];
        /* */
        int64_t i_mb_count[5][19];
        int64_t i_mb_count_8x8dct[2];
        int64_t i_mb_count_size[2][7];
        int64_t i_mb_count_ref[2][16];
        /* */
        int     i_direct_score[2];//score:�÷�,�ɼ�,�������ܶ�
        int     i_direct_frames[2];

    } stat;

    /* CPU functions dependants(����) */
    x264_predict_t      predict_16x16[4+3];//predict:Ԥ��
    x264_predict_t      predict_8x8c[4+3];
    x264_predict8x8_t   predict_8x8[9+3];
    x264_predict_t      predict_4x4[9+3];

    x264_pixel_function_t pixf;
    x264_mc_functions_t   mc;
    x264_dct_function_t   dctf;
    x264_csp_function_t   csp;//x264_csp_function_t��һ���ṹ�壬��common/csp.h�ж���
    x264_quant_function_t quantf;//��common/quant.h�ж���
    x264_deblock_function_t loopf;//��common/frame.h�ж���

    /* vlc table for decoding(����) purpose(1. Ŀ��; ��ͼ2. ����; ��;; Ч��) only */
    x264_vlc_table_t *x264_coeff_token_lookup[5];
    x264_vlc_table_t *x264_level_prefix_lookup;
    x264_vlc_table_t *x264_total_zeros_lookup[15];
    x264_vlc_table_t *x264_total_zeros_dc_lookup[3];
    x264_vlc_table_t *x264_run_before_lookup[7];

#if VISUALIZE
    struct visualize_t *visualize;//visualize:����, ����, ����
#endif
};

void    x264_test_my1();
void    x264_test_my2(char * name);

//��������������Ϊ����Ҫx264_t
// included at the end because it needs x264_t
#include "macroblock.h"

#endif

