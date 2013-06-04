/*****************************************************************************
 * x264.h: h264 encoder library
 * ��ѶͨѶʵ���ң�http://www.powercam.cc/vclab
 * http://wmnmtm.blog.163.com ����һЩ����
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: x264.h,v 1.1 2004/06/03 19:24:12 fenrir Exp $
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

#ifndef _X264_H
#define _X264_H 1

#if !defined(_STDINT_H) && !defined(_STDINT_H_) && \
    !defined(_INTTYPES_H) && !defined(_INTTYPES_H_)
# ifdef _MSC_VER
#  pragma message("You must include stdint.h or inttypes.h before x264.h")
# else
#  pragma message("warning You must include stdint.h or inttypes.h before x264.h")//zjh
# endif
#endif

#include <stdarg.h>

#define X264_BUILD 49

/* x264_t:
 *      opaque handler for decoder and encoder */
typedef struct x264_t x264_t;

/****************************************************************************
 * Initialisation structure and function.
 ****************************************************************************/
/* CPU flags
 */
#define X264_CPU_MMX        0x000001    /* mmx */
#define X264_CPU_MMXEXT     0x000002    /* mmx-ext*/
#define X264_CPU_SSE        0x000004    /* sse */
#define X264_CPU_SSE2       0x000008    /* sse 2 */
#define X264_CPU_3DNOW      0x000010    /* 3dnow! */
#define X264_CPU_3DNOWEXT   0x000020    /* 3dnow! ext */
#define X264_CPU_ALTIVEC    0x000040    /* altivec */

/* Analyse(����, �ֽ�, ����) flags
 */
#define X264_ANALYSE_I4x4       0x0001  /* Analyse i4x4 */
#define X264_ANALYSE_I8x8       0x0002  /* Analyse i8x8 (requires 8x8 transform) */
#define X264_ANALYSE_PSUB16x16  0x0010  /* Analyse p16x8, p8x16 and p8x8 */
#define X264_ANALYSE_PSUB8x8    0x0020  /* Analyse p8x4, p4x8, p4x4 */
#define X264_ANALYSE_BSUB16x16  0x0100  /* Analyse b16x8, b8x16 and b8x8 */
#define X264_DIRECT_PRED_NONE        0
#define X264_DIRECT_PRED_SPATIAL     1
#define X264_DIRECT_PRED_TEMPORAL    2
#define X264_DIRECT_PRED_AUTO        3
#define X264_ME_DIA                  0
#define X264_ME_HEX                  1
#define X264_ME_UMH                  2
#define X264_ME_ESA                  3
#define X264_CQM_FLAT                0
#define X264_CQM_JVT                 1
#define X264_CQM_CUSTOM              2

#define X264_RC_CQP                  0
#define X264_RC_CRF                  1
#define X264_RC_ABR                  2

static const char * const x264_direct_pred_names[] = { "none", "spatial", "temporal", "auto", 0 };
static const char * const x264_motion_est_names[] = { "dia", "hex", "umh", "esa", 0 };
static const char * const x264_overscan_names[] = { "undef", "show", "crop", 0 };
static const char * const x264_vidformat_names[] = { "component", "pal", "ntsc", "secam", "mac", "undef", 0 };
static const char * const x264_fullrange_names[] = { "off", "on", 0 };
static const char * const x264_colorprim_names[] = { "", "bt709", "undef", "", "bt470m", "bt470bg", "smpte170m", "smpte240m", "film", 0 };
static const char * const x264_transfer_names[] = { "", "bt709", "undef", "", "bt470m", "bt470bg", "smpte170m", "smpte240m", "linear", "log100", "log316", 0 };
static const char * const x264_colmatrix_names[] = { "GBR", "bt709", "undef", "", "fcc", "bt470bg", "smpte170m", "smpte240m", "YCgCo", 0 };

/* 
Colorspace type (ɫ�ʿռ�����)
 */
#define X264_CSP_MASK           0x00ff  /* ֻ�õ�8λ���ɣ����Ը�8λ��0x00 ����ᰴλ�����һЩ�ж� ��x264_frame_copy_picture������ */
#define X264_CSP_NONE           0x0000  /* Invalid mode     */
#define X264_CSP_I420           0x0001  /* yuv 4:2:0 planar */
#define X264_CSP_I422           0x0002  /* yuv 4:2:2 planar */
#define X264_CSP_I444           0x0003  /* yuv 4:4:4 planar */
#define X264_CSP_YV12           0x0004  /* yuv 4:2:0 planar */
#define X264_CSP_YUYV           0x0005  /* yuv 4:2:2 packed */
#define X264_CSP_RGB            0x0006  /* rgb 24bits       */
#define X264_CSP_BGR            0x0007  /* bgr 24bits       */
#define X264_CSP_BGRA           0x0008  /* bgr 32bits       */
#define X264_CSP_VFLIP          0x1000  /* */

/* 
Slice type (Ƭ/��������)
 */
#define X264_TYPE_AUTO          0x0000  /* ��x264ѡ����ȷң���� Let x264 choose the right type */
#define X264_TYPE_IDR           0x0001	/* IDRƬ */
#define X264_TYPE_I             0x0002	/* IƬ */
#define X264_TYPE_P             0x0003	/* PƬ */
#define X264_TYPE_BREF          0x0004  /* Non-disposable(һ����) B-frame(B-֡) ???*/
#define X264_TYPE_B             0x0005	/* BƬ */
#define IS_X264_TYPE_I(x) ((x)==X264_TYPE_I || (x)==X264_TYPE_IDR)	/* ֡�����ͣ�IƬ����IDRƬ���� */
#define IS_X264_TYPE_B(x) ((x)==X264_TYPE_B || (x)==X264_TYPE_BREF)	/* ֡�����ͣ�BƬ����BREF */
																	//"||"�߼��������


/* Log level ��־�ȼ�
 */
#define X264_LOG_NONE          (-1)	//����־
#define X264_LOG_ERROR          0	//����
#define X264_LOG_WARNING        1	//����
#define X264_LOG_INFO           2	//��Ϣ
#define X264_LOG_DEBUG          3	//����
 
typedef struct
{
    int i_start, i_end;
    int b_force_qp;//�Ƿ�_ǿ��_qp
    int i_qp;
    float f_bitrate_factor;//float_������_����,ϵ��
} x264_zone_t;//zone:����

typedef struct
{
    /* (CPU ��־λ)*/
    unsigned int cpu;
    int         i_threads;  /* (���б����֡) divide(��, ����) each(ÿһ) frame(֡) into multiple(�����) slices(Ƭ), encode(����) in parallel(ƽ�е�,���е�) */
							//��ÿһ֡�ֳɶ��Ƭ�����б���
    /* ��Ƶ���� */
    int         i_width;		/* ���*/
    int         i_height;		/* �߶�*/
    int         i_csp;			/* (֧��i420��ɫ�ʿռ�) CSP of encoded bitstream, only i420 supported */
    int         i_level_idc;	/* levelֵ������ */
    int         i_frame_total;	/* ����֡������, Ĭ�� 0 */

	/* Vui��������Ƶ��������Ϣ��Ƶ��׼��ѡ�� ��׼��311ҳ*/
    struct
    {
        /* they will be reduced(���ٵ�,�򻯵�) to be 0 < x <= 65535 and prime(��Ҫ��; ������) */
        int         i_sar_height;	/* ��׼313ҳ �䷨Ԫ��sar_width��ʾ����߿�ȵĴ�ֱ�ߴ�(�����ⵥλ) */
        int         i_sar_width;	/* ��׼313ҳ �䷨Ԫ��sar_height��ʾ����߿�ȵ�ˮƽ�ߴ�(����䷨Ԫ��sar_width��ͬ�����ⵥλ) ���ó���� */

        int         i_overscan;    /* ��ɨ���ߣ�Ĭ��"undef"(������)����ѡ�show(�ۿ�)/crop(ȥ��) 0=undef, 1=no overscan, 2=overscan */
        
        /* see h264 annex(��¼) E for the values of the following */
        int         i_vidformat;	/* ��Ƶ��ʽ��Ĭ��"undef"��component/pal/ntsc/secam/mac/undef */
        int         b_fullrange;	/* Specify full range samples setting��Ĭ��"off"����ѡ�off/on */
        int         i_colorprim;	/* ԭʼɫ�ȸ�ʽ��Ĭ��"undef"����ѡ�undef/bt709/bt470m/bt470bg��smpte170m/smpte240m/film */
        int         i_transfer;		/* ת����ʽ��Ĭ��"undef"����ѡ�undef/bt709/bt470m/bt470bg/linear,log100/log316/smpte170m/smpte240m */
        int         i_colmatrix;	/* ɫ�Ⱦ������ã�Ĭ��"undef",undef/bt709/fcc/bt470bg,smpte170m/smpte240m/GBR/YCgCo */
        int         i_chroma_loc;   /* ɫ������ָ������Χ0~5��Ĭ��0 both top & bottom */
    } vui;

    int         i_fps_num;
    int         i_fps_den;//i_fps_den
					/*
					��������������fps֡��ȷ���ģ���ֵ�Ĺ��̼��£�
					{        float fps;      
					if( sscanf( value, "%d/%d", &p->i_fps_num, &p->i_fps_den ) == 2 )
					;
					else if( sscanf( value, "%f", &fps ) )
					{
					p->i_fps_num = (int)(fps * 1000 + .5);
					p->i_fps_den = 1000;
					}
					else
					b_error = 1;
					}
					�����Value��ֵ����fps��
					*/



    /* (������)Bitstream parameters */
    int         i_frame_reference;  /* �ο�֡�����Ŀ */
    int         i_keyint_max;       /* �ڴ˼������IDR�ؼ�֡(ÿ������֡����һ��IDR֡) Force an IDR keyframe at this interval */
    int         i_keyint_min;       /* �����л����ڴ�ֵ����ΪI֡, ������ IDR֡Scenecuts closer together than this are coded as I, not IDR. */
    int         i_scenecut_threshold; /* ���ƶ���������I֡how aggressively to insert extra I frames */
    int         i_bframe;   /* �������ο�֮֡��B֡����Ŀhow many b-frame between 2 references pictures */
    int         b_bframe_adaptive;/* ����ӦB֡�ж� */
    int         i_bframe_bias;/*���Ʋ���B֡�ж�����Χ-100~+100��Խ��Խ���ײ���B֡��Ĭ��0*/

    int         b_bframe_pyramid;   /* ������BΪ�ο�֡,��ѡֵΪ0��1��2 Keep some B-frames as references */
									//pyramid:������
	/*ȥ�����˲�����Ҫ�Ĳ�����alpha��beta��ȥ�����˲����Ĳ���*/
    int         b_deblocking_filter;			//�Ƿ�ʹ��ȥ���˲���
    int         i_deblocking_filter_alphac0;    /* [-6, 6] -6 light(���) filter, 6 strong(ǿ�ҵ�) */
    int         i_deblocking_filter_beta;       /* [-6, 6]  idem(ͬ��) ;beta:����,ϣ����ĸ�еĵڶ�����ĸ*/

	/*�ر��� */
    int         b_cabac;		//CABAC�����������ĵ�����Ӧ�����������ر���
    int         i_cabac_init_idc; //_idc:id cabac

    int         i_cqm_preset;		/* �Զ�����������(CQM),��ʼ������ģʽΪflat */ //preset:Ԥ��,Ԥ�Ȳ���,���ȵ���
    char        *psz_cqm_file;      /* JM format��ȡJM��ʽ���ⲿ���������ļ����Զ�����������cqm ѡ�� JM format */
    uint8_t     cqm_4iy[16];        /* used only if i_cqm_preset == X264_CQM_CUSTOM */
    uint8_t     cqm_4ic[16];		//???
    uint8_t     cqm_4py[16];
    uint8_t     cqm_4pc[16];
    uint8_t     cqm_8iy[64];
    uint8_t     cqm_8py[64];

    /* Log */
    void        (*pf_log)( void *, int i_level, const char *psz, va_list );/* ����ָ�룿common.c��h->param.pf_log( h->param.p_log_private, i_level, psz_fmt, arg ) */
    void        *p_log_private;//private:˽�е�
    int         i_log_level;//ǰ�涨����4����������Ҫ����������ֵ(X264_LOG_NONE X264_LOG_ERROR X264_LOG_WARNING  X264_LOG_INFO X264_LOG_DEBUG)
    int         b_visualize;//visualize:����, ����, ����

    /* �����������Encoder analyser parameters */
    struct
    {
        unsigned int intra;     /* ֡�����intra partitions */
        unsigned int inter;     /* ֡�ڷ���inter partitions */

        int          b_transform_8x8;		/* ֡����� */
        int          b_weighted_bipred;		/* Ϊb֡��ʽ��Ȩimplicit weighting for B-frames */
        int          i_direct_mv_pred;		/* ʱ��ռ���˶�Ԥ��spatial vs temporal mv prediction */
        int          i_chroma_qp_offset;	/*ɫ����������ƫ���� */

        int          i_me_method;		/* motion(�˶�) estimation(����) algorithm(�㷨,���㷨��) to use (X264_ME_*) */
        int          i_me_range;		/* integer pixel motion estimation search range (from predicted(Ԥ���) mv) �������˶�����������Χ����Ԥ���mv*/
        int          i_mv_range;		/* maximum length of a mv (in pixels) */
        int          i_subpel_refine;	/* subpixel(������) motion(�˶�) estimation(����) quality(����) */
        int          b_bidir_me;		/* jointly(��ͬ��,���ϵ�) optimize(ʹ���Ż�) both MVs in B-frames */
        int          b_chroma_me;		/* chroma(ɫ��) ME(�˶�����) for subpel(������) and mode(ģʽ) decision(�ж�,���) in P-frames */
        int          b_bframe_rdo;		/* RD based mode decision for B-frames */
        int          b_mixed_references; /* allow each mb partition in P-frames to have it's own reference number */
        int          i_trellis;			/* trellis RD quantization */
        int          b_fast_pskip;		/* early SKIP detection on P-frames */
        int          b_dct_decimate;	/* transform coefficient thresholding on P-frames */
        int          i_noise_reduction; /* adaptive pseudo-deadzone (�ٵ�-��������) ;zero:���� ;reduction:����, ��С*/

        int          b_psnr;			/* ����ʹ�ӡPSNR��Ϣ || Do we compute PSNR stats���� (save a few % of cpu) */
    } analyse;//����, �ֽ�, ����
	/*
	--deadzone-inter <����>  ����interģʽ�£�������������ֵ����Χ0~32��Ĭ��21��
	--deadzone-intra <����>  ����intraģʽ�£�������������ֵ����Χ0~32��Ĭ��11��

	http://bbs.chinavideo.org/viewthread.php?tid=6769&extra=page%3D3
	������ʽ��z����Y*MF+f��>>qbits�����У�YΪ�任��Ĳв�ֵ��MFΪ����ϵ����fΪ��������ϵ��������ֵf��1<<qbits /3(intra)��1<<qbits/6��inter������21��11���Ǹ����������ֵת��������
	ֻ��X264��51��qp�ȼ��µ�����ֵ���ò��ķ�ʽ����ǰ����ã���������ʽ��x264������ʽ���գ��Ϳ�֪��21��11������
	*/


    /* Rate control parameters ���ʿ��Ʋ��� */
    struct
    {
        int         i_rc_method;    /* X264_RC_* method:����, �취 */

        int         i_qp_constant;  /* 0-51 constant:����,����*/
        int         i_qp_min;       /* �������С����ֵ || min allowed QP value */
        int         i_qp_max;       /* ������������ֵ || max allowed QP value */
        int         i_qp_step;      /* ֡������������� || max QP step between frames */

        int         i_bitrate;		//����ƽ������
        int         i_rf_constant;  /* 1pass VBR, nominal(�����ϵ�) QP */ //constant:����
        float       f_rate_tolerance;//tolerance:ƫ��
        int         i_vbv_max_bitrate;/*ƽ������ģʽ�£����˲ʱ���ʣ�Ĭ��0(��-B������ͬ) */
        int         i_vbv_buffer_size;/*���ʿ��ƻ������Ĵ�С����λkbit��Ĭ��0 */
        float       f_vbv_buffer_init;/* <=1: fraction of buffer_size. >1: kbit���ʿ��ƻ��������ݱ���������������뻺������С֮�ȣ���Χ0~1.0��Ĭ��0.9*/
        float       f_ip_factor;//factor:����, ����,Ҫ��
        float       f_pb_factor;

        /* 2pass */
        int         b_stat_write;   /* Enable(ʹ�ܹ�) stat writing in psz_stat_out */
        char        *psz_stat_out;
        int         b_stat_read;    /* Read stat from psz_stat_in and use it */
        char        *psz_stat_in;

        /* 2pass params (same as ffmpeg ones) */
        char        *psz_rc_eq;     /* 2 pass rate control equation ��ʽ */
        float       f_qcompress;    /* 0.0 => cbr, 1.0 => constant qp */
        float       f_qblur;        /* ʱ����ģ������ || temporally blur quants(����������) */
        float       f_complexity_blur; /* ʱ����ģ�������� || temporally(ʱ���) blur(ģ��) complexity(������) */
        x264_zone_t *zones;         /* ratecontrol overrides(���ԣ�����) */
        int         i_zones;        /* sumber(����) of zone_t's */
        char        *psz_zones;     /* ָ��������һ�ַ��� || alternate(�����) method(����) of specifying(ָ��) zones(����) */
    } rc;

    /* Muxing parameters */
    int b_aud;                  /* ���ɷ��ʵ�Ԫ�ָ��� generate(����) access(����,��ڣ����ţ��뾶��ͨ����ʹ��;����) unit(��Ԫ) delimiters(�����,�ָ���) */
    int b_repeat_headers;       /* ��ÿ���ؼ�֡ǰ����SPS/PPS || put SPS/PPS before each keyframe */
    int i_sps_id;               /*  SPS �� PPS id �� || SPS and PPS id number */
} x264_param_t;

typedef struct {
    int level_idc;
    int mbps;        // ����鴦���ٶȣ���λ�� �����/�� max macroblock processing(����) rate (macroblocks/sec)
    int frame_size;  // ���֡�ߴ磬�Ժ��Ϊ��λ max frame size (macroblocks)
    int dpb;         // ������ͼ�񻺴�������λ�Ǳ��� max decoded picture buffer (bytes)//DPB ����ͼ�񻺴���
    int bitrate;     // �������� k����/�� max bitrate (kbit/sec)
    int cpb;         // ���vbv���壬��λkbit max vbv buffer (kbit)
    int mv_range;    // max vertica(��ֱ)l mv component(����) range��Χ (pixels)
    int mvs_per_2mb; // max mvs per 2 consecutive(������) mbs.
    int slice_rate;  // rate:����,��,�ٶ�; ����
    int bipred8x8;   // limit bipred to >=8x8	//˫��Ԥ��8x8
    int direct8x8;   // limit b_direct to >=8x8	//ֱ��Ԥ��8x8
    int frame_only;  // forbid(��ֹ) interlacing(����,����)		//
} x264_level_t;

/* all of the levels defined(�涨, ȷ��) in the standard, terminated(����) by .level_idc=0 */
extern const x264_level_t x264_levels[];

/* x264_param_default:
 *      fill(���) x264_param_t with default values and do CPU detection(����; ��飻̽�⣻���������) 
 *		��Ĭ��ֵ���ṹ��x264_param_t
 */
void    x264_param_default( x264_param_t * );

/* x264_param_parse:
 *		parse:����
 *      set one parameter by name.
 *      returns 0 on success(����0��ʾ�ɹ�), or returns one of the following errors(����1��ʾ����Ĵ���).
 *      note: bad value occurs(����) only if it can't even(����, ��ʹ) parse the value,
 *      numerical(���ֵ�, �����ֱ�ʾ��, ��ֵ��) range(��Χ) is not checked(���, �˶�) until(����Ϊֹ,�ڡ���ǰ) x264_encoder_open() or x264_encoder_reconfig(). */
#define X264_PARAM_BAD_NAME  (-1)
#define X264_PARAM_BAD_VALUE (-2)
int x264_param_parse( x264_param_t *, const char *name, const char *value );

/****************************************************************************
 * Picture structures and functions.
 * ��read_frame_yuv�������ļ��ж�ȡ�����ݷŵ�����ṹ���
 ****************************************************************************/
typedef struct
{
    int     i_csp;			//Csp: color space parameter ɫ�ʿռ���� X264ֻ֧��I420

    int     i_plane;		//i_Plane ����ɫ�ʿռ�ĸ�����һ��Ϊ3��YUV����ʼ��Ϊ
    int     i_stride[4];	//��ࡣ����Ŀ�࣬��ʱҲ��Ϊ��ָ࣬���Ǳ���Ŀ�ȣ����ֽ�����ʾ������һ������ԭ��λ�����Ͻǵı�����˵�������������
    uint8_t *plane[4];		//typedef unsigned char   uint8_t //sizeof(unsigned char) = 1
} x264_image_t;

typedef struct
{
    /* In: force picture type (if not auto) XXX: ignored (����) for now
     * Out: type of the picture encoded */
    int i_type;		//I_type ָ��������ͼ������ͣ���X264_TYPE_AUTO X264_TYPE_IDR X264_TYPE_I X264_TYPE_P X264_TYPE_BREF X264_TYPE_B�ɹ�ѡ�񣬳�ʼ��ΪAUTO��˵����x264�ڱ�����������п��ơ�
    /* In: force (ǿ��) quantizer (������) for > 0 */
    int i_qpplus1;	//I_qpplus1 ���˲�����1����ǰ�������������ֵ
    /* In: user pts, Out: pts of encoded picture (user)*/
    int64_t i_pts;	//I_pts ��program time stamp ����ʱ�����ָʾ�����������ʱ�����

    /* In: raw(ԭʼ��) data */
    x264_image_t img;//Img :�������һ��ͼ���ԭʼ���ݡ�////x264_picture_t->img.plane[0] x264_picture_t->img.plane[1] x264_picture_t->img.plane[2] �������ԭʼͼ��
} x264_picture_t;
//x264_picture_t��x264_frame_t������.ǰ����˵��һ����Ƶ������ÿ֡���ص�.���ߴ��ÿ֡ʵ�ʵ�����ֵ.ע������

/* x264_picture_alloc:
 *  alloc data for a picture. You must call x264_picture_clean on it. */
/*  �������ݸ�һ��ͼƬ���������x264_picture_clean��� */
void x264_picture_alloc( x264_picture_t *pic, int i_csp, int i_width, int i_height );

/* x264_picture_clean:
 *  free associated resource for a x264_picture_t allocated with
 *  x264_picture_alloc ONLY 
 * �ͷ���x264_picture_alloc�������Դ
*/
void x264_picture_clean( x264_picture_t *pic );

/****************************************************************************
 * NAL structure and functions:
 ****************************************************************************/
/* nal */
//�Ϻ�ܣ���6.20����159ҳ��Ӧ����0��12,13��23,24��31
enum nal_unit_type_e
{
    NAL_UNKNOWN = 0,		//δʹ��
    NAL_SLICE   = 1,		//����������IDRͼ���Ƭ
    NAL_SLICE_DPA   = 2,	//Ƭ����A
    NAL_SLICE_DPB   = 3,	//Ƭ����B
    NAL_SLICE_DPC   = 4,	//Ƭ����C
    NAL_SLICE_IDR   = 5,    /* ref_idc != 0 nal_unit_type=5ʱ����ʾ��ǰNAL��IDRͼ���һ��Ƭ������������£�IDRͼ���е�ÿ��Ƭ��nal_unit_type��Ӧ�õ���5��ע��Ƭ������������IDRͼ�� ����nal_unit_type=5ʱ��nal_ref_idc����0��nal_unit_type����6,9,10,11,12ʱ��nal_ref_idc����0*/
    NAL_SEI         = 6,    /* ref_idc == 0 */
    NAL_SPS         = 7,	//���в�����
    NAL_PPS         = 8,	//ͼ�������
    NAL_AUD         = 9,	//�ֽ��
    /* ref_idc == 0 for 6,9,10,11,12 */
};
enum nal_priority_e//priority:����Ȩ,���ȼ�
{
    NAL_PRIORITY_DISPOSABLE = 0,//NAL_���ȼ�_�ɶ�����
    NAL_PRIORITY_LOW        = 1,//NAL_���ȼ�_��
    NAL_PRIORITY_HIGH       = 2,//NAL_���ȼ�_��
    NAL_PRIORITY_HIGHEST    = 3,//NAL_���ȼ�_���
};	//[�Ϻ�ܣ�Page159 nal_ref_idc] ָʾ��ǰNAL�����ȼ���ȡֵ��Χ0-3��ֵԽ�ߣ���ʾ��ǰNALԽ��Ҫ��Խ��Ҫ�����ܵ�������H.264�涨�����ǰNAL��һ�����в���������һ��ͼ��������������ڲο�ͼ���Ƭ��Ƭ��������Ҫ�����ݵ�λʱ�����䷨Ԫ�ر������0�����ڴ���0ʱ�����ȡ��ֵ����û�н�һ���Ĺ涨��

typedef struct
{
    int i_ref_idc;			/* nal_priority_e (nal���ȼ�) */
    int i_type;				/* nal_unit_type_e (nal��Ԫ����) */

    /* This data are raw(ԭʼ��) payload(�غ�) */
    int i_payload;			/* ���صĴ�С Size of payload in bytes */
							/* 
							If param->b_annexb is set, Annex-B bytestream with 4-byte startcode.
							* Otherwise, startcode is replaced with a 4-byte size.
							* This size is the size used in mp4/similar muxing; it is equal to i_payload-4 
							*/
    uint8_t *p_payload;		/* ָ����Ч���ص���ʼ��ַ */
} x264_nal_t;

/* x264_nal_encode:
 *      encode a nal into a buffer, setting the size.
 *		����һ��nal��һ�������������óߴ�
 *      if b_annexeb then a long synch(ͬʱ;ͬ��;ͬ���ź�) work is added
 *      XXX: it currently doesn't check for overflow 
 *		b_annexeb:�Ƿ����ʼ��
 */
int x264_nal_encode( void *, int *, int b_annexeb, x264_nal_t *nal );

/* x264_nal_decode:
 *      decode a buffer nal into a x264_nal_t 
 *      ����һ�������� nal �� һ�� x264_nal_t �ṹ��
*/
int x264_nal_decode( x264_nal_t *nal, void *, int );

/****************************************************************************
 * Encoder functions:
 * x264_encoder_open����һ������������������Ϊx264_t
 * ����Ķ����������һ��������Ϊx264_t��x264_t�ʹ���ָ��һ��������
 ****************************************************************************/

/* x264_encoder_open:
 *      create a new encoder handler, all parameters from x264_param_t are copied 
 *      ����һ���µı����� �����в����ӽṹ��x264_param_t����
*/
x264_t *x264_encoder_open   ( x264_param_t * );

/* x264_encoder_reconfig:
 *      change encoder options while encoding,
 *      analysis(����)-related(��ص�) parameters from x264_param_t are copied 
 *      ��������иı������ѡ��
 *		��Ҫ��x264_t�ṹ���x264_param_t�ӽṹ��������
*/
int     x264_encoder_reconfig( x264_t *, x264_param_t * );

/* x264_encoder_headers:
 *      return the SPS and PPS that will be used for the whole stream 
 *		���� SPS �� pps ������������������
*/
int     x264_encoder_headers( x264_t *, x264_nal_t **, int * );

/* x264_encoder_encode:
 *      encode one picture 
 *		����һ��ͼƬ
*/
int     x264_encoder_encode ( x264_t *, x264_nal_t **, int *, x264_picture_t *, x264_picture_t * );

/* x264_encoder_close:
 *      close an encoder handler 
 *		�ر�һ�����봦����
*/
void    x264_encoder_close  ( x264_t * );

/* XXX: decoder isn't working so no need to export it */

/****************************************************************************
 * Private((˽�е�) stuff(���ϣ��������ѡ�װ��) for internal(�ڲ���) usage(�÷�):
 ****************************************************************************/
#ifdef __X264__
#   ifdef _MSC_VER
#       define inline __inline
#       define DECLARE_ALIGNED( type, var, n ) __declspec(align(n)) type var
#		define strncasecmp(s1, s2, n) strnicmp(s1, s2, n)	/* �Ƚ��ַ���s1��s2��ǰn���ַ��������ִ�Сд */
#   else
#       define DECLARE_ALIGNED( type, var, n ) type var __attribute__((aligned(n)))
#   endif
#endif

#endif
