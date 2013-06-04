/*****************************************************************************
 * common.c: h264 library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: common.c,v 1.1 2004/06/03 19:27:06 fenrir Exp $
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "common.h"
#include "cpu.h"

/*
����ȱʡ��־����
*/
static void x264_log_default( void *, int, const char *, va_list );


/****************************************************************************
 * x264_test_my1:(�򵥵Ĳ��Ժ���)
 ****************************************************************************/

void    x264_test_my1()
{
//	AfxMessageBox("x264_test_my1");
}

/****************************************************************************
 * x264_test_my2:(�򵥵Ĳ��Ժ���)
 ****************************************************************************/

void    x264_test_my2( char * name)
{

//	AfxMessageBox(name);
}

/****************************************************************************
 * x264_param_default:(Ϊ�˽ṹ���ʼ��һЩĬ��ֵ)
 ����ȱʡ����

 ****************************************************************************/
void    x264_param_default( x264_param_t *param )
{
    /* ���ѷ���õ��ڴ�ռ��ʼ��Ϊ0 */
    memset( param, 0, sizeof( x264_param_t ) );

    /* CPU autodetect(�Զ����) */
    param->cpu = x264_cpu_detect();		/* ������ȡCPU���кŵĲ�������ͷ detect:���*/
    param->i_threads = 1;				//���б����֡

    /* ��Ƶ���� */
    param->i_csp           = X264_CSP_I420;		/* ɫ�ʿռ� */	
    param->i_width         = 0;		/* �� */
    param->i_height        = 0;		/* �� */
    param->vui.i_sar_width = 0;		/* ��׼313ҳ �䷨Ԫ��sar_width��ʾ����߿�ȵ�ˮƽ�ߴ�(�����ⵥλ) */
    param->vui.i_sar_height= 0;		/* ��׼313ҳ �䷨Ԫ��sar_height��ʾ����߿�ȵĴ�ֱ�ߴ�(����䷨Ԫ��sar_width��ͬ�����ⵥλ) */
    param->vui.i_overscan  = 0;		/* undef ��ɨ���ߣ�Ĭ��"undef"(������)����ѡ�show(�ۿ�)/crop(ȥ��) 0=undef, 1=no overscan, 2=overscan */
    param->vui.i_vidformat = 5;		/* undef ��׼314 �䷨Ԫ��video_format ��ʾͼ�����������������ǰ����ʽ������E-2�Ĺ涨����video_format�﷨Ԫ�ز����ڣ�video_format��ֵӦ���ƶ�Ϊ5*/
    param->vui.b_fullrange = 0;		/* off ��׼314ҳ video_full_rang_flag��ʾ�ڵ�ƽ��������ɫ���źŵķ�Χ��E'y����E'pr��...ģ���źŷ����õ�*/
    param->vui.i_colorprim = 2;		/* undef ��׼314���䷨Ԫ��colour_primaries��ʾ�����ԭɫ��ɫ�����꣬����EIE 1931�Ĺ涨(��E-3)��x��y�Ķ�����SO/CIE 10527�涨����colour_primaries�﷨Ԫ�ز�����ʱ��colour_primaries��ֵӦ���ƶ�Ϊ����2(ɫ��δ�������Ӧ�þ���)*/
    param->vui.i_transfer  = 2;		/* undef ��׼315���﷨Ԫ��transfer_characteristics��ʾԴͼ��Ĺ��ת�����ԣ�ģ������Χ��0��1�����Թ�ǿ������Lc�ĺ���������E4����transfer_characteristics�﷨Ԫ�ز�����ʱ��transfer_characteristics��ֵӦ���ƶ�Ϊ����2(ת������δ�������Ӧ�þ���)*/
    param->vui.i_colmatrix = 2;		/* undef ��׼316���﷨Ԫ��matrix_coefficients�������ڸ��ݺ졢�̡�����ԭɫ�õ����Ⱥ�ɫ���źŵľ���ϵ��������E-5��matrix_coefficients��Ӧ����0��������������������Ϊ��... ...*/
    param->vui.i_chroma_loc= 0;		/* left center �﷨Ԫ��chroma_loc_info_present_flag����1��ʾchroma_sample_loc_type_top_field��chroma_sample_loc_type_bottom_field���ڡ�chroma_loc_info_present_flag����0��ʾ���ǲ����� */
    param->i_fps_num       = 25;	/* ��Ƶ����֡�� */
    param->i_fps_den       = 1;		/* ���������͵����ı�ֵ������ʾ֡�� */
    param->i_level_idc     = 51;	/* as close to "unrestricted" as we can get */

    /* (�������)Encoder parameters */
    param->i_frame_reference = 1;			/* �ο�֡�����Ŀ */
    param->i_keyint_max = 250;				/* �ڴ˼������IDR�ؼ�֡ */
    param->i_keyint_min = 25;				/* �����л����ڴ�ֵ����λI, ������ IDR */
    param->i_bframe = 0;					/* �������ͼ���P֡����Ŀ */
    param->i_scenecut_threshold = 40;		/* ��λ����ز�������I֡ scene:����, �ֳ� threshold:�У����ޣ���ʼ�� */
    param->b_bframe_adaptive = 1;			/* adaptive:��Ӧ */
    param->i_bframe_bias = 0;				/* ���Ʋ���B֡�ж�����Χ-100~+100��Խ��Խ���ײ���B֡��Ĭ��0 bias: ƫ��, ƫ�� */
    param->b_bframe_pyramid = 0;			/* pyramid: ������,��׶(��), ��׶(��) */

    param->b_deblocking_filter = 1;			/* ȥ���˲� */
    param->i_deblocking_filter_alphac0 = 0;	/*  */
    param->i_deblocking_filter_beta = 0;	/* ϣ����ĸ�ڶ���[B, ��] */

    param->b_cabac = 1;						/* ��������������Ӧ�����������ر��� */
    param->i_cabac_init_idc = 0;			/*  */

	/*���ʿ���*/
    param->rc.i_rc_method = X264_RC_CQP;	/*�㶨����*/
    param->rc.i_bitrate = 0;				/*����ƽ�����ʴ�С*/
    param->rc.f_rate_tolerance = 1.0;		/* rate:����, ��;tolerance:����, ���̣�ƫ��, ����,ʧ��*/
    param->rc.i_vbv_max_bitrate = 0;		/*ƽ������ģʽ�£����˲ʱ���ʣ�Ĭ��0(��-B������ͬ) */
    param->rc.i_vbv_buffer_size = 0;		/*���ʿ��ƻ������Ĵ�С����λkbit��Ĭ��0 */
    param->rc.f_vbv_buffer_init = 0.9;		/* <=1: fraction of buffer_size. >1: kbit���ʿ��ƻ��������ݱ���������������뻺������С֮�ȣ���Χ0~1.0��Ĭ��0.9*/
    param->rc.i_qp_constant = 26;			/*��Сqpֵ*/
    param->rc.i_rf_constant = 0;			/*  */
    param->rc.i_qp_min = 10;				/*�������С����ֵ */
    param->rc.i_qp_max = 51;				/*������������ֵ*/
    param->rc.i_qp_step = 4;				/*֡������������� */
    param->rc.f_ip_factor = 1.4;				/* factor: ����, Ҫ��*/
    param->rc.f_pb_factor = 1.3;				/*  */

    param->rc.b_stat_write = 0;					/*  */
    param->rc.psz_stat_out = "x264_2pass.log";	/*  */
    param->rc.b_stat_read = 0;					/*  */
    param->rc.psz_stat_in = "x264_2pass.log";	/*  */
    param->rc.psz_rc_eq = "blurCplx^(1-qComp)";	/*  */
    param->rc.f_qcompress = 0.6;				/* compress: ѹ��*/
    param->rc.f_qblur = 0.5;					/*ʱ����ģ������ */
    param->rc.f_complexity_blur = 20;			/* ʱ����ģ�������� complexity:������ */
    param->rc.i_zones = 0;						/* zone: (���ֳ�����)����, ����, �ش�*/

    /* ��־ */
    param->pf_log = x264_log_default;			/* ò�ƺ���ָ�� common.c:static void x264_log_default( void *p_unused, int i_level, const char *psz_fmt, va_list arg ) */
    param->p_log_private = NULL;				/*  */
    param->i_log_level = X264_LOG_INFO;			/* ��־���� */

    /* ���� analyse:����, �ֽ�*/
    param->analyse.intra = X264_ANALYSE_I4x4 | X264_ANALYSE_I8x8;	/* 0x0001 | 0x0002 */
    param->analyse.inter = X264_ANALYSE_I4x4 | X264_ANALYSE_I8x8	/* 0x0001 | 0x0002 */	
                         | X264_ANALYSE_PSUB16x16 | X264_ANALYSE_BSUB16x16;	/* 0x0010 | 0x0100 */
    param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_SPATIAL;	/* ʱ��ռ���˶�Ԥ�� #define X264_DIRECT_PRED_SPATIAL	1 */
    param->analyse.i_me_method = X264_ME_HEX;	/* �˶������㷨 (X264_ME_*) */
    param->analyse.i_me_range = 16;				/* �˶����Ʒ�Χ */
    param->analyse.i_subpel_refine = 5;			/* �������˶��������� */
    param->analyse.b_chroma_me = 1;				/* ������ɫ���˶����ƺ�P֡��ģʽѡ�� */
    param->analyse.i_mv_range = -1; 			/* �˶�ʸ����󳤶�(in pixels). -1 = auto, based on level */ // set from level_idc
    param->analyse.i_chroma_qp_offset = 0;		/* ɫ����������ƫ���� */
    param->analyse.b_fast_pskip = 1;			/* ����P֡������� */		
    param->analyse.b_dct_decimate = 1;			/* ��P-framesת�������� */
    param->analyse.b_psnr = 1;					/*�Ƿ���ʾPSNR ����ʹ�ӡPSNR��Ϣ*/

	/*����*/
    param->i_cqm_preset = X264_CQM_FLAT;		/*�Զ�����������(CQM),��ʼ������ģʽΪflat 0*/
    memset( param->cqm_4iy, 16, 16 );			/*  */
    memset( param->cqm_4ic, 16, 16 );			/*  */
    memset( param->cqm_4py, 16, 16 );			/*  */
    memset( param->cqm_4pc, 16, 16 );			/*  */
    memset( param->cqm_8iy, 16, 64 );			/*  */
    memset( param->cqm_8py, 16, 64 );			/*���ٿռ�*/

	/*muxing*/									/* repeat:�ظ� */
    param->b_repeat_headers = 1;				/* ��ÿ���ؼ�֡ǰ����SPS/PPS*/
    param->b_aud = 0;							/*���ɷ��ʵ�Ԫ�ָ���*/
}

/* 
 * ����ö��
 * const char *arg				:ֵ
 * const char * const *names	:ö��
 * int *dst						:Ŀ�����
 * 
 * ����ʾ����
 * parse_enum( value, x264_overscan_names, &p->vui.i_overscan ); //static const char * const x264_overscan_names[] = { "undef", "show", "crop", 0 }; //�����ʾ���ڶ���������һ��ö��
*/
static int parse_enum( const char *arg, const char * const *names, int *dst )
{
    int i;
    for( i = 0; names[i]; i++ )
        if( !strcmp( arg, names[i] ) )
        {
            *dst = i;	/* ��ԭֵ�ı��� */
            return 0;
        }
    return -1;
}

static int parse_cqm( const char *str, uint8_t *cqm, int length )
{
    int i = 0;
    do {
        int coef;
        if( !sscanf( str, "%d", &coef ) || coef < 1 || coef > 255 )	/* ��Чֵ��Χ[1,255] */
            return -1;
        cqm[i++] = coef;	/* i����������� */
    } while( i < length && (str = strchr( str, ',' )) && str++ );	/* lengthָ��Ҫ���೤  */
    return (i == length) ? 0 : -1;	/* ѭ������i��length��ȣ�����0�����򷵻�1������0��˵��������ָ�����ȵĶ��� */
}
										/* 
										��һ���ַ����ж�����ָ����ʽ��������� 
									����int sscanf( const char *, const char *, ...);
									����int sscanf(const char *buffer,const char *format[,argument ]...);
									����buffer �洢������
									����format ��ʽ�����ַ���
									����argument ѡ�����趨�ַ���
									����sscanf���buffer��������ݣ�����argument���趨������д�ء�
										sscanf��scanf���ƣ�������������ģ�ֻ�Ǻ����Լ���(stdin)Ϊ����Դ��ǰ���Թ̶��ַ���Ϊ����Դ��
										��һ������������һ������ {%[*] [width] [{h | l | I64 | L}]type | ' ' | '\t' | '\n' | ��%����}
										����ע��
										����1�� * ������ڸ�ʽ��, (�� %*d �� %*s) �����Ǻ� (*) ��ʾ���������ݲ�����. (Ҳ���ǲ��Ѵ����ݶ��������)
										����2��{a|b|c}��ʾa,b,c��ѡһ��[d],��ʾ������dҲ����û��d��
										����3��width��ʾ��ȡ��ȡ�
										����4��{h | l | I64 | L}:������size,ͨ��h��ʾ���ֽ�size��I��ʾ2�ֽ� size,L��ʾ4�ֽ�size(double����),l64��ʾ8�ֽ�size��
										����5��type :��ͺܶ��ˣ�����%s,%d֮�ࡣ
										����6���ر�ģ�%*[width] [{h | l | I64 | L}]type ��ʾ����������ı����˵���������Ŀ�������д��ֵ
										����ʧ�ܷ���0 �����򷵻ظ�ʽ���Ĳ�������
										*/

										/*
											extern char *strchr(const char *s,char c);
										����const char *strchr(const char* _Str,int _Val)
										����char *strchr(char* _Str,int _Ch)
										����ͷ�ļ���#include <string.h>
										�������ܣ������ַ���s���״γ����ַ�c��λ��
										����˵���������״γ���c��λ�õ�ָ�룬���s�в�����c�򷵻�NULL��
										��������ֵ��Returns the address of the first occurrence of the character in the string if successful, or NULL otherwise
										*/
/*
 * ���ַ���תΪ��������
 * "1","true","yes" => 1
 * "0","false","no" => 0
*/
static int atobool( const char *str )
{
	/* "1","true","yes" => 1 */
    if( !strcmp(str, "1") || !strcmp(str, "true") || !strcmp(str, "yes") )	/* strcmp(const char *s1,const char * s2); ��s1=s2ʱ������ֵ=0 ;  */
        return 1;
	/* "0","false","no" => 0 */
    if( !strcmp(str, "0") ||			/* !strcmp(str, "0")Ϊ�棬��strcmp(str, "0")=0����str="0" */
        !strcmp(str, "false") || 
        !strcmp(str, "no") )
        return 0;
    return -1;
}

#define atobool(str) ( (i = atobool(str)) < 0 ? (b_error = 1) : i )

/*
 * x264_param_parse
 * x264.c�еĵ��ô��룺
 * b_error |= x264_param_parse( param, long_options[long_options_index].name, optarg ? optarg : "true" );
 * �ڽ���main����ʱ��������param������param���������Ǻܳ��ģ�����������в������з������浽���param�ṹ�����ĸ��ֶι���������ʹ��
 * p�������ṹ����ֶ��д�ֵ
 * name�ǲ�������
 * value�ǲ���ֵ
 * ���ϼ������У�ѭ���ṩ��ͬ�Ĳ������ñ�����������������в�����ֵ�浽�ṹ����
*/
int x264_param_parse( x264_param_t *p, const char *name, const char *value )
{
    int b_error = 0;
    int i;

    if( !name )
	{
        return X264_PARAM_BAD_NAME;	/* #define X264_PARAM_BAD_NAME  (-1) x264.h */
	}
    if( !value )
	{
        return X264_PARAM_BAD_VALUE;	/* #define X264_PARAM_BAD_VALUE (-2) x264.h  */
	}

    if( value[0] == '=' )
	{
        value++;
	}

    if( (!strncmp( name, "no-", 3 ) && (i = 3)) || (!strncmp( name, "no", 2 ) && (i = 2)) )	/* ?i�������ô�����0��? */
    {
        name += i;
        value = atobool(value) ? "false" : "true";
    }

	#define OPT(STR) else if( !strcmp( name, STR ) )		/* strcmp(const char *s1,const char * s2); �Ƚ��ַ���s1��s2�� ˵���� ��s1<s2ʱ������ֵ<0 ; ��s1=s2ʱ������ֵ=0 ; ��s1>s2ʱ������ֵ>0 , ���������ַ���������������ַ���ȣ���ASCIIֵ��С��Ƚϣ���ֱ�����ֲ�ͬ���ַ�����'\0'Ϊֹ�� */
    if(0);	/* http://wmnmtm.blog.163.com/blog/static/3824571420117303531845/ */
    OPT("asm")												/* else if( !strcmp( name, "asm" ) )  */
        p->cpu = atobool(value) ? x264_cpu_detect() : 0;	/*  */
    OPT("threads")											/*  */
    {
        if( !strcmp(value, "auto") )						/*  */
            p->i_threads = 0;
        else
            p->i_threads = atoi(value);						/*  */
    }
    OPT("level")							/*  */
    {
        if( atof(value) < 6 )
            p->i_level_idc = (int)(10*atof(value)+.5);		/*  */
        else
            p->i_level_idc = atoi(value);					/*  */
    }
    OPT("sar")							/*  */
    {
        b_error = ( 2 != sscanf( value, "%d:%d", &p->vui.i_sar_width, &p->vui.i_sar_height ) &&
                    2 != sscanf( value, "%d/%d", &p->vui.i_sar_width, &p->vui.i_sar_height ) );/* ��value�����ݵ����������� */
    }
    OPT("overscan")							/* Ĭ��ֵ��undef����δ������ɨ�裨overscan�������ɨ�����˼��װ��ֻ��ʾӰ���һ���֡����õ�ֵ��undef��δ���塣show��ָʾҪ��ʾ����Ӱ��������������趨����뱻���ء�crop��ָʾ��Ӱ���ʺ��������ɨ�蹦�ܵ�װ���ϲ��š���һ�������ء����飺�ڱ���֮ǰ�ü���Crop����Ȼ�����װ��֧Ԯ��ʹ��show��������� */		/* else if( !strcmp( name, "overscan" ) ) */
        b_error |= parse_enum( value, x264_overscan_names, &p->vui.i_overscan );
    OPT("vidformat")							/* ֻ�ҵ���videoformat��Ĭ��ֵ��undef��ָʾ����Ѷ�ڱ��룯��λ����digitizing��֮ǰ��ʲô��ʽ�����õ�ֵ��component, pal, ntsc, secam, mac, undef ���飺��Դ��Ѷ�ĸ�ʽ������δ���� */	/* else if( !strcmp( name, "vidformat" ) ) */
        b_error |= parse_enum( value, x264_vidformat_names, &p->vui.i_vidformat );
    OPT("fullrange")							/* Ĭ��ֵ��off��ָʾ�Ƿ�ʹ�����Ⱥ�ɫ�Ȳ㼶��ȫ��Χ�������Ϊoff�����ʹ�����޷�Χ�����飺�����Դ�Ǵ������Ѷ��λ����������Ϊoff��������Ϊon */
        b_error |= parse_enum( value, x264_fullrange_names, &p->vui.b_fullrange );
    OPT("colourprim")							/* ֻ�ҵ���colorprim��Ĭ��ֵ��undef���趨��ʲôɫ��ԭɫת����RGB�����õ�ֵ��undef, bt709, bt470m, bt470bg, smpte170m, smpte240m, film�����飺Ĭ��ֵ��������֪����Դʹ��ʲôɫ��ԭɫ */
        b_error |= parse_enum( value, x264_colorprim_names, &p->vui.i_colorprim );
    OPT("transfer")							/* Ĭ��ֵ��undef���趨Ҫʹ�õĹ���ӣ�opto-electronic���������ԣ��趨����������ɫ���(gamma)���ߣ������õ�ֵ��undef, bt709, bt470m, bt470bg, linear, log100, log316, smpte170m, smpte240m�����飺Ĭ��ֵ��������֪����Դʹ��ʲô�������� */
        b_error |= parse_enum( value, x264_transfer_names, &p->vui.i_transfer );
    OPT("colourmatrix")
        b_error |= parse_enum( value, x264_colmatrix_names, &p->vui.i_colmatrix );
    OPT("chromaloc")						/* Ĭ��ֵ��0;�趨ɫ�Ȳ���λ�ã���ITU-T���ĸ�¼E�����壩.���õ�ֵ��0~5�����飺����Ǵ���ȷ�β���4:2:0��MPEG1ת�룬����û�����κ�ɫ�ʿռ�ת������Ӧ�ý���ѡ����Ϊ1������Ǵ���ȷ�β���4:2:0��MPEG2ת�룬����û�����κ�ɫ�ʿռ�ת������Ӧ�ý���ѡ����Ϊ0������Ǵ���ȷ�β���4:2:0��MPEG4ת�룬����û�����κ�ɫ�ʿռ�ת������Ӧ�ý���ѡ����Ϊ0������ά��Ĭ��ֵ�� */
    {
        p->vui.i_chroma_loc = atoi(value);	/* chroma:ɫ�� */
        b_error = ( p->vui.i_chroma_loc < 0 || p->vui.i_chroma_loc > 5 );
    }
    OPT("fps")												/* ָ���̶�֡�� */
    {
        float fps;
        if( sscanf( value, "%d/%d", &p->i_fps_num, &p->i_fps_den ) == 2 )
            ;
        else if( sscanf( value, "%f", &fps ) )
        {
            p->i_fps_num = (int)(fps * 1000 + .5);			/*  */
            p->i_fps_den = 1000;							/*  */
        }
        else
            b_error = 1;
    }
    OPT("ref")												/* ���ƽ���ͼƬ���壨DPB��Decoded Picture Buffer���Ĵ�С����Χ�Ǵ�0��16����֮����ֵ��ÿ��P֡����ʹ����ǰ����֡��Ϊ����֡����Ŀ;��B֡����ʹ�õ���ĿҪ��һ��������ȡ���������Ƿ���Ϊ����֡�������Ա����յ���Сref����1�� ��Ҫע����ǣ�H.264���������ÿ��level��DPB��С http://nmm-hd.org/doc/X264%E8%A8%AD%E5%AE%9A#bitrate */
        p->i_frame_reference = atoi(value);					/*  */
    OPT("keyint")											/* �趨x264�����������֮���IDR֡�����Ϊ�ؼ�֡�����������ָ��infinite��x264��Զ��Ҫ����ǳ��������IDR֡ */
    {
        p->i_keyint_max = atoi(value);						/*  */
        if( p->i_keyint_min > p->i_keyint_max )				/*  */
            p->i_keyint_min = p->i_keyint_max;				/*  */
    }
    OPT("min-keyint")										/* �趨IDR֮֡�����С����,��ѡ��������ÿ��IDR֮֡��Ҫ�ж���֡�ſ���������һ��IDR֡����С����,min-keyint���������ֵ��--keyint/2+1,��С��keyint��Χ�ᵼ�¡�����ȷ�ġ�IDR֡λ�ã����������������� else if( !strcmp( name, "min-keyint" ) ) */
    {
        p->i_keyint_min = atoi(value);						/*  */
        if( p->i_keyint_max < p->i_keyint_min )
            p->i_keyint_max = p->i_keyint_min;				/*  */
    }
    OPT("scenecut")											/* �趨I/IDR֡λ�õ���ֵ�����������⣩�� x264Ϊÿһ֡����һ������ֵ����������ǰһ֡�Ĳ�ͬ�̶ȡ������ֵ����scenecut��������⵽һ��������������������ʱ�����һ��IDR֡�ľ������--min-keyint�������һ��I֡���������һ��IDR֡��Խ���scenecutֵ��������⵽�����������Ŀ�������������αȽϵ���ϸ��Ѷ���Բ���http://forum.doom9.org/showthread.php?t=121116 */
        p->i_scenecut_threshold = atoi(value);				/* scene:����,�ֳ�;threshold:��,����,��ʼ�� ��scenecut��Ϊ0�൱���趨--no-scenecut */
    OPT("bframes")											/* Ĭ��ֵ��3 �趨x264����ʹ�õ������B֡��,û��B֡ʱ��һ�����͵�x264������������������֡���ͣ�IPPPPP...PI��������--bframes 2ʱ���������������P֡���Ա�B֡ȡ��������IBPBBPBPPPB...PI,B֡������P֡������B֡���ܴ���֮���֡����̬Ԥ�⣨motion prediction������ѹ������˵Ч�ʻ�����ߡ����ǵ�ƽ��Ʒ������--pbratio������,��Ȥ���£�x264���������ֲ�ͬ�����B֡��"B"�Ǵ���һ��������֡��Ϊ����֡��B֡������--b-pyramid������"b"�����һ����������֡��Ϊ����֡��B֡���������һ�λ�ϵ�"B"��"b"��ԭ��ͨ���������йء�����𲢲���Ҫʱ��ͨ������"B"��������B֡;x264�����Ϊÿ����ѡ֡ѡ��ΪP֡��B֡����ϸ��Ѷ���Բ���http://article.gmane.org/gmane.comp.video.ffmpeg.devel/29064���ڴ�����£�֡���Ϳ�������������������--bframes 3����IBBBPBBBPBPI */
        p->i_bframe = atoi(value);							/*  */
    OPT("b-adapt")											/* Ĭ��ֵ��1;�趨����B֡λ�þ����㷨�����趨����x264��ξ���Ҫ����P֡��B֡;0��ͣ�ã�������ѡB֡������ɵ�no-b-adapt�趨��ͬ����;1�������١��㷨���Ͽ죬Խ���--bframesֵ����΢����ٶȡ���ʹ�ô�ģʽʱ�������Ͻ������--bframes 16ʹ�á�2������ѡ��㷨��������Խ���--bframesֵ���������ٶȡ� */
        p->b_bframe_adaptive = atobool(value);//ԭֵ				/* B֡_����Ӧ */
    OPT("b-bias")											/* Ĭ��ֵ��0;����ʹ��B֡����ʹ��P֡�Ŀ����ԡ�����0��ֵ����ƫ��B֡�ļ�Ȩ����С��0��ֵ���෴����Χ�Ǵ�-100��100��100������֤ȫ��B֡��Ҫȫ��B֡��ʹ��--b-adapt 0������-100Ҳ����֤ȫ��P֡����������Ϊ�ܱ�x264�������õ�λԪ�ʿ��ƾ���ʱ��ʹ�ô�ѡ� */
        p->i_bframe_bias = atoi(value);						/*  */
    OPT("b-pyramid")										/* Ĭ��ֵ��normal;����B֡��Ϊ����֡�Ĳ���֡��û�д��趨ʱ��ֻ֡�ܲ���I/P֡����ȻI/P֡����ϸߵ�Ʒ����Ϊ����֡���м�ֵ����B֡Ҳ�Ǻ����õġ���Ϊ����֡��B֡��õ�һ������P֡����ͨB֮֡�������ֵ��b-pyramid��Ҫ�����������ϵ�--bframes�Ż��������������Ϊ������룬��ʹ��none��strict��none��������B֡��Ϊ����֡��strict��ÿminigop����һ��B֡��Ϊ����֡�����������׼ǿ��ִ�е����ơ�normal��ÿminigop������B֡��Ϊ����֡�� */
        p->b_bframe_pyramid = atobool(value);				/*  */
    OPT("nf")
        p->b_deblocking_filter = 0;							/* ȥ���˲� */
    OPT("filter")											/*  */
    {
        int count;
        if( 0 < (count = sscanf( value, "%d:%d", &p->i_deblocking_filter_alphac0, &p->i_deblocking_filter_beta )) ||
            0 < (count = sscanf( value, "%d,%d", &p->i_deblocking_filter_alphac0, &p->i_deblocking_filter_beta )) )
        {
            p->b_deblocking_filter = 1;						/*  */
            if( count == 1 )
                p->i_deblocking_filter_beta = p->i_deblocking_filter_alphac0;	/*  */
        }
        else
            p->b_deblocking_filter = atobool(value);		/* ȥ���˲� */
    }
    OPT("cabac")											/* ֻ�ҵ���no-cabac��Ĭ��ֵ���ޣ�ͣ�õ������ݵĶ������������루CABAC��Context Adaptive Binary Arithmetic Coder��������ѹ�����л���Ч�ʽϵ͵ĵ������ݵĿɱ䳤�ȱ��루CAVLC��Context Adaptive Variable Length Coder��ϵͳ���������ѹ��Ч�ʣ�ͨ��10~20%���ͽ����Ӳ������ */
        p->b_cabac = atobool(value);						/*  */
    OPT("cabac-idc")										/*  */
        p->i_cabac_init_idc = atoi(value);					/*  */
    OPT("cqm")												/* Ĭ��ֵ��flat���趨�����Զ���������custom quantization matrices��Ϊ�ڽ���Ĭ��֮һ���ڽ�Ĭ����flat��JVT�����飺Ĭ��ֵ */
    {
        if( strstr( value, "flat" ) )
            p->i_cqm_preset = X264_CQM_FLAT;				/*  */
        else if( strstr( value, "jvt" ) )
            p->i_cqm_preset = X264_CQM_JVT;					/*  */
        else
            b_error = 1;
    }
    OPT("cqmfile")											/* Ĭ��ֵ���ޣ���һ��ָ����JM���ݵ������趨�Զ��������󡣸�д��������--cqm��ͷ��ѡ����飺Ĭ��ֵ */
        p->psz_cqm_file = strdup(value);					/*  */
    OPT("cqm4")												/* �趨����4x4����������Ҫ16���Զ��ŷָ��������嵥�� */
    {
        p->i_cqm_preset = X264_CQM_CUSTOM;					/* #define X264_CQM_JVT	1 x264.h*/
        b_error |= parse_cqm( value, p->cqm_4iy, 16 );
        b_error |= parse_cqm( value, p->cqm_4ic, 16 );
        b_error |= parse_cqm( value, p->cqm_4py, 16 );
        b_error |= parse_cqm( value, p->cqm_4pc, 16 );
    }
	/*
	cqm4* / cqm8*Ĭ��ֵ���� 

	--cqm4���趨����4x4����������Ҫ16���Զ��ŷָ��������嵥�� 
	--cqm8���趨����8x8����������Ҫ64���Զ��ŷָ��������嵥�� 
	--cqm4i��--cqm4p��--cqm8i��--cqm8p���趨���Ⱥ�ɫ���������� 
	--cqm4iy��--cqm4ic��--cqm4py��--cqm4pc���趨������������ 
	���飺Ĭ��ֵ 
	���ϣ�http://nmm-hd.org/doc/X264%E8%A8%AD%E5%AE%9A#no-8x8dct	
	*/
    OPT("cqm8")												/* �趨����8x8����������Ҫ64���Զ��ŷָ��������嵥�� */
    {
        p->i_cqm_preset = X264_CQM_CUSTOM;					/*  */
        b_error |= parse_cqm( value, p->cqm_8iy, 64 );
        b_error |= parse_cqm( value, p->cqm_8py, 64 );
    }
    OPT("cqm4i")											/* �趨���Ⱥ�ɫ���������� */
    {
        p->i_cqm_preset = X264_CQM_CUSTOM;					/*  */
        b_error |= parse_cqm( value, p->cqm_4iy, 16 );
        b_error |= parse_cqm( value, p->cqm_4ic, 16 );
    }
    OPT("cqm4p")											/* �趨���Ⱥ�ɫ���������� */
    {
        p->i_cqm_preset = X264_CQM_CUSTOM;					/*  */
        b_error |= parse_cqm( value, p->cqm_4py, 16 );
        b_error |= parse_cqm( value, p->cqm_4pc, 16 );
    }
    OPT("cqm4iy")											/* �趨������������ */
    {
        p->i_cqm_preset = X264_CQM_CUSTOM;					/*  */
        b_error |= parse_cqm( value, p->cqm_4iy, 16 );
    }
    OPT("cqm4ic")											/* �趨������������ */
    {
        p->i_cqm_preset = X264_CQM_CUSTOM;					/*  */
        b_error |= parse_cqm( value, p->cqm_4ic, 16 );
    }
    OPT("cqm4py")											/* �趨������������ */
    {
        p->i_cqm_preset = X264_CQM_CUSTOM;					/*  */
        b_error |= parse_cqm( value, p->cqm_4py, 16 );
    }
    OPT("cqm4pc")											/* �趨������������ */
    {
        p->i_cqm_preset = X264_CQM_CUSTOM;					/*  */
        b_error |= parse_cqm( value, p->cqm_4pc, 16 );
    }
    OPT("cqm8i")											/* �趨���Ⱥ�ɫ���������� */
    {
        p->i_cqm_preset = X264_CQM_CUSTOM;					/*  */
        b_error |= parse_cqm( value, p->cqm_8iy, 64 );
    }
    OPT("cqm8p")											/* �趨���Ⱥ�ɫ���������� */
    {
        p->i_cqm_preset = X264_CQM_CUSTOM;					/*  */
        b_error |= parse_cqm( value, p->cqm_8py, 64 );
    }
    OPT("log")												/*  */
        p->i_log_level = atoi(value);						/*  */
    OPT("analyse")											/* analyse:����, �ֽ�, ���� */
    {
        p->analyse.inter = 0;								/*  */
        if( strstr( value, "none" ) )  p->analyse.inter =  0;					/*  */
        if( strstr( value, "all" ) )   p->analyse.inter = ~0;					/*  */

        if( strstr( value, "i4x4" ) )  p->analyse.inter |= X264_ANALYSE_I4x4;	/*  */
        if( strstr( value, "i8x8" ) )  p->analyse.inter |= X264_ANALYSE_I8x8;	/*  */
        if( strstr( value, "p8x8" ) )  p->analyse.inter |= X264_ANALYSE_PSUB16x16;	/*  */
        if( strstr( value, "p4x4" ) )  p->analyse.inter |= X264_ANALYSE_PSUB8x8;	/*  */
        if( strstr( value, "b8x8" ) )  p->analyse.inter |= X264_ANALYSE_BSUB16x16;	/*  */
    }
    OPT("8x8dct")										/* ֻ�ҵ���no-8x8dct;Ĭ��ֵ���޵���8x8��ɢ����ת����Adaptive 8x8 DCT��ʹx264�ܹ��ǻ۵��Ե�ʹ��I֡��8x8ת������ѡ��ͣ�øù��ܡ����飺Ĭ��ֵ */
        p->analyse.b_transform_8x8 = atobool(value);	/*  */
    OPT("weightb")										/* ֻ�ҵ���no-weightb��Ĭ��ֵ���ޣ�H.264������Ȩ��B֡�Ĳ��գ���������ÿ������Ӱ��Ԥ��ͼƬ�ĳ̶ȡ���ѡ��ͣ�øù��ܡ����飺Ĭ��ֵ */
        p->analyse.b_weighted_bipred = atobool(value);	/*  */
    OPT("direct")										/* Ĭ��ֵ��spatial;�趨"direct"��̬������motion vectors����Ԥ��ģʽ��������ģʽ���ã�spatial��temporal������ָ��none��ͣ��direct��̬��������ָ��auto������x264������֮���л�Ϊ�ʺϵ�ģʽ�������Ϊauto��x264���ڱ������ʱ���ʹ���������Ѷ��auto���ʺ��������׶α��룬��Ҳ������һ�׶α��롣�ڵ�һ�׶�autoģʽ��x264������¼ÿ������ִ�е�ĿǰΪֹ�ĺû������Ӹü�¼��ѡ��һ��Ԥ��ģʽ��ע�⣬���ڵ�һ�׶���ָ��autoʱ����Ӧ���ڵڶ��׶�ָ��auto�������һ�׶β���ָ��auto���ڶ��׶ν���Ĭ��Ϊtemporal��noneģʽ���˷�λԪ�������ǿ�Ҳ����顣���飺auto */
        b_error |= parse_enum( value, x264_direct_pred_names, &p->analyse.i_direct_mv_pred );	/*  */
    OPT("chroma-qp-offset")								/* Ĭ��ֵ��0;�ڱ���ʱ����ɫ��ƽ������ֵ��ƫ�ơ�ƫ�ƿ���Ϊ��������ʹ��psy-rd��psy-trellisʱ��x264�Զ����ʹ�ֵ��������ȵ�Ʒ�ʣ���󽵵�ɫ�ȵ�Ʒ�ʡ���Щ�趨��Ĭ��ֵ��ʹchroma-qp-offset�ټ�ȥ2��ע�⣺x264����ͬһ����ֵ��������ƽ���ɫ��ƽ�棬ֱ������ֵ29���ڴ�֮��ɫ�����Ա����ȵ͵�����������ֱ��������q51��ɫ����q39Ϊֹ������Ϊ����H.264��׼��Ҫ�� */
        p->analyse.i_chroma_qp_offset = atoi(value);	/*  */
    OPT("me")											/* Ĭ��ֵ��hex;�趨ȫ���أ�full-pixel����̬���㣨motion estimation���ķ����������ѡ�dia��diamond������򵥵���Ѱ��������ʼ�����Ԥ������predictor��������ϡ����¡��ҷ�һ�����صĶ�̬��������ѡ������õ�һ�������ظ��˹���ֱ���������ҵ��κθ��õĶ�̬����Ϊֹ��hex��hexagon���������Ʋ�����ɣ�������ʹ����Χ6�㷶ΧΪ2����Ѱ����˽��������Ρ�����dia����Ч���Ҽ���û�б����������Ϊһ����;�ı����Ǹ������ѡ��umh��uneven multi-hex������hex����������Ѱ���ӵĶ�������ͼ���Ա�����©�����ҵ��Ķ�̬����������hex��dia��merange����ֱ�ӿ���umh����Ѱ�뾶���������ӻ���ٹ�����Ѱ�Ĵ�С��esa��exhaustive����һ����merange��������̬��Ѱ�ռ�ĸ߶���ѻ��ǻ���Ѱ����Ȼ�ٶȽϿ죬����ѧ���൱����Ѱ������ÿ����һ��̬�����ı�����bruteforce������������������Ȼ��UMH��Ҫ��������û�д����ܴ�ĺô������Զ����ճ��ı��벻���ر����á�tesa��transformed exhaustive����һ�ֳ��Խӽ���ÿ����̬����ִ��Hadamardת�����Ƚϵ�Ч��֮�㷨������exhaustive����Ч����һ����ٶ���һ�㡣 */
        b_error |= parse_enum( value, x264_motion_est_names, &p->analyse.i_me_method );			/*  */
    OPT("merange")										/*  */
        p->analyse.i_me_range = atoi(value);			/*  */
    OPT("mvrange")										/* Ĭ��ֵ��-1 (�Զ�)���趨��̬��������󣨴�ֱ����Χ����λ�����أ���Ĭ��ֵ��level��ͬ��Level 1/1b��64; Level 1.1~2.0��128; Level 2.1~3.0��256; Level 3.1+��512��ע�⣺�����Ҫ�ֶ���дmvrange�����趨ʱ������ֵ��ȥ0.25������--mvrange 127.75���� ���飺Ĭ��ֵ */
        p->analyse.i_mv_range = atoi(value);			/*  */
    OPT("subme")										/* Ĭ��ֵ��7���趨������((subpixel)���㸴�Ӷȡ�ֵԽ��Խ�á��㼶1~5ֻ�ǿ���������ϸ�֣�refinement��ǿ�ȡ��㼶6Ϊģʽ��������RDO�����㼶8Ϊ��̬�������ڲ�Ԥ��ģʽ����RDO��RDO�㼶����������ǰ�Ĳ㼶��ʹ��С��2��ֵ���������ýϿ���Ʒ�ʽϵ͵�lookaheadģʽ�����ҵ��½ϲ��--scenecut���ߣ���˲����顣���õ�ֵ��0��Fullpel only;1��QPel SAD 1 iteration;2��QPel SATD 2 iterations;3��HPel on MB then QPel;4��Always QPel;5��Multi QPel + bi-directional motion estimation;6��RD on I/P frames;7��RD on all frames;8��RD refinement on I/P frames;9��RD refinement on all frames;10��QP-RD (requires --trellis=2, --aq-mode>0); */
        p->analyse.i_subpel_refine = atoi(value);		/*  */
    OPT("bime")											/*  */
        p->analyse.b_bidir_me = atobool(value);			/*  */
    OPT("chroma-me")									/* ֻ�ҵ���no-chroma-me��Ĭ��ֵ���ޣ�ͨ�������ȣ�luma����ɫ�ȣ�chroma������ƽ�涼������̬���㡣��ѡ��ͣ��ɫ�ȶ�̬���������Щ΢�ٶȡ����飺Ĭ��ֵ */
        p->analyse.b_chroma_me = atobool(value);		/*  */
    OPT("b-rdo")										/*  */
        p->analyse.b_bframe_rdo = atobool(value);		/*  */
    OPT("mixed-refs")									/* ֻ�ҵ���no-mixed-refs��Ĭ��ֵ����;��ϲ��ջ���ÿ��8x8�ָ�Ϊ������ѡȡ���գ���������ÿ��������Ϊ��������ʹ�ö������֡ʱ������Ʒ�ʣ���ȻҪ��ʧһЩ�ٶȡ��趨��ѡ���ͣ�øù��ܡ����飺Ĭ��ֵ; */
        p->analyse.b_mixed_references = atobool(value);	/*  */
    OPT("trellis")										/* Ĭ��ֵ��1��ִ��Trellis quantization�����Ч�ʡ�0��ͣ�á�1��ֻ��һ������������ձ��������á�2��������ģʽ���������á��ں�����ʱ�ṩ���ٶȺ�Ч��֮�������ƽ�⡣�����о���ʱ����ӽ����ٶȡ����飺Ĭ��ֵע�⣺��Ҫ--cabac  */
        p->analyse.i_trellis = atoi(value);				/*  */
    OPT("fast-pskip")									/* ֻ�ҵ���no-fast-pskip��Ĭ��ֵ���ޣ�ͣ��P֡�������Թ���⣨early skip detection�����ǳ���΢�����Ʒ�ʣ���Ҫ��ʧ�ܶ��ٶȡ����飺Ĭ��ֵ */
        p->analyse.b_fast_pskip = atobool(value);		/*  */
    OPT("dct-decimate")									/* ֻ�ҵ���no-dct-decimate:Ĭ��ֵ����;DCT Decimation����������Ϊ������Ҫ�ġ�DCT���顣�����Ʊ���Ч�ʣ������͵�Ʒ��ͨ��΢��������趨��ѡ���ͣ�øù��ܡ����飺Ĭ��ֵ */
        p->analyse.b_dct_decimate = atobool(value);		/*  */
    OPT("nr")											/*  */
        p->analyse.i_noise_reduction = atoi(value);		/*  */
    OPT("bitrate")										/* Ĭ��ֵ����;����λԪ�ʿ��Ʒ���֮������Ŀ��λԪ��ģʽ��������Ѷ��Ŀ��λԪ��ģʽ��ζ�����յ�����С����֪�ģ�������Ʒ����δ֪��x264�᳢�԰Ѹ�����λԪ����Ϊ����ƽ��ֵ��������Ѷ�������ĵ�λ��ǧλԪ/�루8λԪ=1�ֽڣ���ע�⣬1ǧλԪ(kilobit)��1000λԪ��������1024λԪ�����趨ͨ����--pass�����׶Σ�two-pass������һ��ʹ�á���ѡ����--qp��--crf���⡣ */
    {
        p->rc.i_bitrate = atoi(value);					/*  */
        p->rc.i_rc_method = X264_RC_ABR;				/*  */
    }
    OPT("qp")											/* Ĭ��ֵ����;����λԪ�ʿ��Ʒ���֮һ���趨x264�Թ̶�����ֵ��Constant Quantizer��ģʽ��������Ѷ���������ֵ��ָ��P֡������ֵ��I֡��B֡������ֵ���Ǵ�--ipratio��--pbratio��ȡ�á�CQģʽ��ĳ������ֵ��ΪĿ�꣬����ζ�����յ�����С��δ֪�ģ���Ȼ����͸��һЩ������׼ȷ�ع��ƣ�����ֵ��Ϊ0�������ʧ�������������ͬ�Ӿ�Ʒ�ʣ�qp���--crf��������ĵ�����qpģʽҲ��ͣ�õ�����������Ϊ���ն��塰�̶�����ֵ����ζ��û�е�����������ѡ����--bitrate��--crf���⡣��Ȼqp����Ҫlookahead��ִ������ٶȽϿ죬��ͨ��Ӧ�ø���--crf */
    {
        p->rc.i_qp_constant = atoi(value);	/*  */
        p->rc.i_rc_method = X264_RC_CQP;	/*  */
    }
    OPT("crf")		/* Ĭ��ֵ��23.0;���һ��λԪ�ʿ��Ʒ������̶�λԪ��ϵ����Constant Ratefactor������qp�ǰ�ĳ������ֵ��ΪĿ�꣬��bitrate�ǰ�ĳ��������С��ΪĿ��ʱ��crf���ǰ�ĳ����Ʒ�ʡ���ΪĿ�ꡣ��������crf n�ṩ���Ӿ�Ʒ����qp n��ͬ��ֻ�ǵ�����Сһ�㡣crfֵ�Ķ�����λ�ǡ�λԪ��ϵ����ratefactor������CRF�ǽ��ɽ��͡��ϲ���Ҫ����֮֡Ʒ�����ﵽ��Ŀ�ġ��ڴ�����£����ϲ���Ҫ����ָ�ڸ��ӻ�߶�̬������֡����Ʒ�ʲ��Ǻܺķ�λԪ�����ǲ��ײ�������Ի�������ǵ�����ֵ������Щ֡������ʡ������λԪ�������·��䵽���Ը���Ч���õ�֡��CRF���ѵ�ʱ�������׶α����٣���Ϊ���׶α����еġ���һ�׶Ρ����Թ��ˡ���һ���棬ҪԤ��CRF���������λԪ���ǲ����ܵġ������������λԪ�ʿ���ģʽ������������������ ��ѡ����--qp��--bitrate���⡣ */
    {
        p->rc.i_rf_constant = atoi(value);	/*  */
        p->rc.i_rc_method = X264_RC_CRF;	/*  */
    }
    OPT("qpmin")				/* Ĭ��ֵ��0;����x264����ʹ�õ���С����ֵ������ֵԽС�������Խ�ӽ����롣����һ����ֵ��x264������������������һ������ʹ��������ȫ��ͬ��ͨ��û����������x264���ѱ�������λԪ�����κ��ض��ĺ������ϡ���������������ʱ��Ĭ�����ã������������qpmin����Ϊ��ή��֡����ƽ�汳�������Ʒ�ʡ� */
        p->rc.i_qp_min = atoi(value);		/*  */
    OPT("qpmax")				/* Ĭ��ֵ��51;����x264����ʹ�õ��������ֵ��Ĭ��ֵ51��H.264���ɹ�ʹ�õ��������ֵ������Ʒ�ʼ��͡���Ĭ��ֵ��Ч��ͣ����qpmax�������Ҫ����x264������������Ʒ�ʣ����Խ���ֵ��Сһ�㣨ͨ��30~40������ͨ���������������ֵ�� */
        p->rc.i_qp_max = atoi(value);		/*  */
    OPT("qpstep")							/* Ĭ��ֵ��4;�趨��֮֡������ֵ����������ȡ� */
        p->rc.i_qp_step = atoi(value);		/*  */
    OPT("ratetol")				/*  */
        p->rc.f_rate_tolerance = !strncmp("inf", value, 3) ? 1e9 : atof(value);		/*  */
    OPT("vbv-maxrate")							/* Ĭ��ֵ��0;�趨��������VBV��������λԪ�ʡ�VBV�ή��Ʒ�ʣ����Ա�Ҫʱ��ʹ�á� */
        p->rc.i_vbv_max_bitrate = atoi(value);	/*  */
    OPT("vbv-bufsize")							/* Ĭ��ֵ��0;�趨VBV����Ĵ�С����λ��ǧλԪ���� VBV�ή��Ʒ�ʣ����Ա�Ҫʱ��ʹ�á� */
        p->rc.i_vbv_buffer_size = atoi(value);	/*  */
    OPT("vbv-init")								/* Ĭ��ֵ��0.9;�趨VBV��������������ٲŻῪʼ���š����ֵС��1����ʼ���������ǣ�vbv-init * vbv-bufsize�������ֵ���ǳ�ʼ������������λ��ǧλԪ���� */
        p->rc.f_vbv_buffer_init = atof(value);	/*  */
    OPT("ipratio")								/* Ĭ��ֵ��1.40;�޸�I֡����ֵ���P֡����ֵ��Ŀ��ƽ��������Խ���ֵ�����I֡��Ʒ�ʡ� */
        p->rc.f_ip_factor = atof(value);		/*  */
    OPT("pbratio")								/* Ĭ��ֵ��1.30;�޸�B֡����ֵ���P֡����ֵ��Ŀ��ƽ��������Խ���ֵ�ή��B֡��Ʒ�ʡ���mbtree����ʱ��Ĭ�����ã������趨�����ã�mbtree���Զ��������ֵ�� */
        p->rc.f_pb_factor = atof(value);		/*  */
    OPT("pass")									/* Ĭ��ֵ����;��Ϊ���׶α����һ����Ҫ�趨��������x264��δ���--stats�������������趨��1������һ���µ�ͳ�����ϵ������ڵ�һ�׶�ʹ�ô�ѡ�2����ȡͳ�����ϵ����������ս׶�ʹ�ô�ѡ�3����ȡͳ�����ϵ��������¡�ͳ�����ϵ�������ÿ������֡����Ѷ���������뵽x264�Ը��������������ִ�е�һ�׶�������ͳ�����ϵ�����Ȼ��ڶ��׶ν�����һ����ѻ�����Ѷ���롣���Ƶĵط���Ҫ�ǴӸ��õ�λԪ�ʿ����л��档 */
    {
        int i = x264_clip3( atoi(value), 0, 3 );	/*  */
        p->rc.b_stat_write = i & 1;					/*  */
        p->rc.b_stat_read = i & 2;					/*  */
    }
    OPT("stats")									/* Ĭ��ֵ��"x264_2pass.log";�趨x264��ȡ��д��ͳ�����ϵ�����λ�á� */
    {
        p->rc.psz_stat_in = strdup(value);			/*  */
        p->rc.psz_stat_out = strdup(value);			/*  */
    }
    OPT("rceq")									/*  */
        p->rc.psz_rc_eq = strdup(value);		/*  */
    OPT("qcomp")								/* Ĭ��ֵ��0.60;����ֵ����ѹ��ϵ����0.0�ǹ̶�λԪ�ʣ�1.0���ǹ̶�����ֵ����mbtree����ʱ������Ӱ��mbtree��ǿ�ȣ�qcompԽ��mbtreeԽ������ ���飺Ĭ��ֵ */
        p->rc.f_qcompress = atof(value);		/*  */
    OPT("qblur")								/* Ĭ��ֵ��0.5;������ѹ��֮���Ը����İ뾶��Χ���ø�˹ģ��������ֵ���ߡ�����ô��Ҫ���趨 */
        p->rc.f_qblur = atof(value);			/*  */
    OPT("cplxblur")								/* Ĭ��ֵ��20.0;�Ը����İ뾶��Χ���ø�˹ģ����gaussian blur��������ֵ���ߡ�����ζ�ŷ����ÿ��֡������ֵ�ᱻ�����ڽ�֡ģ�������Դ�����������ֵ���� */
        p->rc.f_complexity_blur = atof(value);	/*  */
    OPT("zones")								/* Ĭ��ֵ����;������Ѷ���ض�Ƭ��֮�趨�������޸�ÿ���εĴ����x264ѡ��;һ����һ���ε���ʽΪ<��ʼ֡>,<����֡>,<ѡ��>�� ������α˴���"/"�ָ���������0,1000,b=2/1001,2000,q=20,me=3,b-bias=-1000;���飺Ĭ��ֵ */
        p->rc.psz_zones = strdup(value);		/*  */
    OPT("psnr")									/*  */
        p->analyse.b_psnr = atobool(value);		/*  */
    OPT("aud")									/*  */
        p->b_aud = atobool(value);				/*  */
    OPT("sps-id")								/*  */
        p->i_sps_id = atoi(value);				/*  */
    OPT("repeat-headers")						/*  */
        p->b_repeat_headers = atobool(value);	/*  */
    else
        return X264_PARAM_BAD_NAME;
#undef OPT
#undef atobool

    return b_error ? X264_PARAM_BAD_VALUE : 0;
}

/****************************************************************************
 * x264_log:����log����

 ****************************************************************************/
void x264_log( x264_t *h, int i_level, const char *psz_fmt, ... )
{
    if( i_level <= h->param.i_log_level )
    {
        va_list arg;	/* �ں�����������һ��va_list��Ȼ����va_start��������ȡ�����б��еĲ�����ʹ����Ϻ����va_end()������ */
        va_start( arg, psz_fmt );	/* start:������va_start��ʼ������va_end��β */
        h->param.pf_log( h->param.p_log_private, i_level, psz_fmt, arg );/* ò�ƺ���ָ�룿 */
        va_end( arg );	/* end	:������va_end��β����apָ����ΪNULL */
    }
}

/*
 * ��־_Ĭ�� (����־д�����ļ�)
 * x264 error ...
 * x264 warning ...
 * x264 info ...
 * x264 debug ...
 * x264 unknown ...
*/
static void x264_log_default( void *p_unused, int i_level, const char *psz_fmt, va_list arg )
{
    char *psz_prefix;
    switch( i_level )
    {
        case X264_LOG_ERROR:
            psz_prefix = "error";	//prefix:ǰ׺
            break;
        case X264_LOG_WARNING:
            psz_prefix = "warning";	/* ���� */
            break;
        case X264_LOG_INFO:
            psz_prefix = "info";	/* ��Ϣ */
            break;
        case X264_LOG_DEBUG:
            psz_prefix = "debug";	/* ���� */
            break;
        default:
            psz_prefix = "unknown";	/* δ֪ */
            break;
    }
    fprintf( stderr, "x264 [%s]: ", psz_prefix );	/* fprintf()��������ָ����format(��ʽ)(��ʽ)������Ϣ(����)����stream(��)ָ�����ļ�. fprintf()ֻ�ܺ�printf()һ������. fprintf()�ķ���ֵ��������ַ���,��������ʱ����һ����ֵ. http://baike.baidu.com/view/656682.htm */
    vfprintf( stderr, psz_fmt, arg );	/* vfprintf()����ݲ���format�ַ�����ת������ʽ�����ݣ�Ȼ�󽫽�����������streamָ�����ļ��У�ֱ�������ַ�����������\0����Ϊֹ��http://baike.baidu.com/view/1081188.htm */
}

/****************************************************************************
 * x264_picture_alloc:����picture����,�������ͼ���ʽ����ռ�
 ****************************************************************************/
void x264_picture_alloc( x264_picture_t *pic, int i_csp, int i_width, int i_height )
{
	//�Դ���ĵ�һ������������һ���ṹ����ֶθ�ֵ
    pic->i_type = X264_TYPE_AUTO;//#define X264_TYPE_AUTO  0x0000  /* Let x264 choose the right type */
    pic->i_qpplus1 = 0;
    pic->img.i_csp = i_csp; //x264_picture_t���ĸ��ֶΣ����һ���ֶ��Ǹ�ָ�����飬������Ŷ�̬������ڴ�ĵ�ַ

    switch( i_csp & X264_CSP_MASK )	//����ɫ�ʿռ����� //ʵ�ʾ���0x0001 & 0x00ff ��λ��
    {
        case X264_CSP_I420://0x0001
        case X264_CSP_YV12://0x0004
            pic->img.i_plane = 3;
			//�����ڴ棬�ѷ��صĵ�ַ���浽�ֶ�������
            pic->img.plane[0] = x264_malloc( 3 * i_width * i_height / 2 );	/* �������ڴ棬�����׵�ַ */ //�������� * 1.5
            pic->img.plane[1] = pic->img.plane[0] + i_width * i_height;		/* �׵�ַ + �������� ->��һ���ڴ��ַ */
            pic->img.plane[2] = pic->img.plane[1] + i_width * i_height / 4; /* ������ƶ���ַ */
            pic->img.i_stride[0] = i_width;
            pic->img.i_stride[1] = i_width / 2;
            pic->img.i_stride[2] = i_width / 2;
            break;

        case X264_CSP_I422://0x0002
            pic->img.i_plane = 3;
            pic->img.plane[0] = x264_malloc( 2 * i_width * i_height );
            pic->img.plane[1] = pic->img.plane[0] + i_width * i_height;
            pic->img.plane[2] = pic->img.plane[1] + i_width * i_height / 2;
            pic->img.i_stride[0] = i_width;
            pic->img.i_stride[1] = i_width / 2;
            pic->img.i_stride[2] = i_width / 2;
            break;

        case X264_CSP_I444:
            pic->img.i_plane = 3;
            pic->img.plane[0] = x264_malloc( 3 * i_width * i_height );
            pic->img.plane[1] = pic->img.plane[0] + i_width * i_height;
            pic->img.plane[2] = pic->img.plane[1] + i_width * i_height;
            pic->img.i_stride[0] = i_width;
            pic->img.i_stride[1] = i_width;
            pic->img.i_stride[2] = i_width;
            break;

        case X264_CSP_YUYV://YUY2����YUYV����ʽΪÿ�����ر���Y��������UV������ˮƽ������ÿ�������ز���һ��
            pic->img.i_plane = 1;
            pic->img.plane[0] = x264_malloc( 2 * i_width * i_height );//��������*2
            pic->img.i_stride[0] = 2 * i_width;
            break;

        case X264_CSP_RGB:
        case X264_CSP_BGR://RGB->BGR���� R �� B ��λ�û�һ�¾�����
            pic->img.i_plane = 1;
            pic->img.plane[0] = x264_malloc( 3 * i_width * i_height );//��������*3
            pic->img.i_stride[0] = 3 * i_width;
            break;

        case X264_CSP_BGRA:
            pic->img.i_plane = 1;
            pic->img.plane[0] = x264_malloc( 4 * i_width * i_height );//��������*4
            pic->img.i_stride[0] = 4 * i_width;
            break;

        default:
            fprintf( stderr, "invalid CSP\n" );
            pic->img.i_plane = 0;
            break;
    }//��ƽ���ʽ�У�Y��U �� V �����Ϊ����������ƽ����д洢�� 
}

/****************************************************************************
 * x264_picture_clean:�ͷŷ����ͼ��ռ�
 ****************************************************************************/
void x264_picture_clean( x264_picture_t *pic )
{
    x264_free( pic->img.plane[0] );//

    /* 
	just to be safe
	����Ϊ�˰�ȫʹ���ڴ�
	*/
    memset( pic, 0, sizeof( x264_picture_t ) );
}

/****************************************************************************
 * x264_nal_encode:nal��Ԫ����
 * ����һ��nal��һ�������������óߴ�
 * ����b_annexeb:�Ƿ����ʼ��
 * nalu ���������ǰ׺��header, �����м���0x03
 * ����Ǹ������Ǹ��ṹ�壬���ĸ��ֶΣ��ֱ��ǣ����ȼ������x����Ч�غ�
 ****************************************************************************/
int x264_nal_encode( void *p_data, int *pi_data, int b_annexeb, x264_nal_t *nal )
{
    uint8_t *dst = p_data;
    uint8_t *src = nal->p_payload;//ָ����Ч���ص���ʼ��ַ
    uint8_t *end = &nal->p_payload[nal->i_payload];//���һ���ֽڵĵ�ַ��
					//ȡ����������һ��Ԫ�أ��Ը�Ԫ��ȡ��ַ���õ����������ַ
    int i_count = 0;

    /* FIXME this code doesn't check overflow */

	//nalu surfix(��׺) 0x 00 00 00 01//��������ѵ�ע��[�Ϻ�ܣ�Page158]����ʼ��
    if( b_annexeb )//annex:����������
    {
        /* long nal start code (we always use long ones)
		 * ���������Ǵ洢�ڽ�����ʱ����ÿ��NAL ǰ�����ʼ�룺0x000001
		 * ��ĳЩ���͵Ľ����ϣ�Ϊ��Ѱַ�ķ��㣬Ҫ���������ڳ����϶��룬�������ĳ�������ı�������
		 * �ǵ����������H.264 ��������ʼ��ǰ��������ֽڵ�0 ����䣬ֱ����NAL �ĳ��ȷ���Ҫ��
		 * �������Ļ����£��������������м����ʼ�룬��Ϊһ��NAL ����ʼ��ʶ������⵽��һ��
		 * ��ʼ��ʱ��ǰNAL ������H.264 �涨����⵽0x000000 ʱҲ���Ա�����ǰNAL �Ľ�����������Ϊ
		 * ���ŵ������ֽڵ�0 �е��κ�һ���ֽڵ�0 Ҫô������ʼ��Ҫô����ʼ��ǰ����ӵ�0��
		*/
        *dst++ = 0x00;//++���ã���ʹ��dst������ִ�������ִ�е���
        *dst++ = 0x00;//*dst=0x00;dst++;
        *dst++ = 0x00;
        *dst++ = 0x01;
		//dstʵ��ָ��ľ���x264.c:data[3000000]
    }
	//data�����ȴ�����: 0x 00 00 00 01
	//����ʵ��nalͷ�Ĵ���
	//��1λ��ֹλ
	//��2��3λ�����ȼ�
	//����5λ������
	//���������Ƶ���Ӧλ�ã��ٰ�λ�򣬾Ͱ���λ�ŵ�һ���ֽ�����
    /* nal header [�Ϻ�ܣ�Page 145 181] ͷ��1�ֽڣ�������ֹλ1bit���ȼ�2bit��NAL����5bit�����������Ч��RBSP*/
    *dst++ = ( 0x00 << 7 ) | ( nal->i_ref_idc << 5 ) | nal->i_type;//��λ��C++primer������Page136ҳλ������
							//�����������ĺ�����ʽ�����õ�ǰֵ��Ȼ��ŵ���C++primer������Page127�����͵ݼ�������
							//����7λ�����ðѵ�һ������λ�����ˣ�
							//����5λ�������������õ�5λ����λ����λ�м������Ƕ�λ�����������ȼ�
							//���ұߵ�5λ�����ÿ��Ա�ʾ32������������
	//insert 0x03//������������Ǽ�2���ֽھͲ���03���������ǲ��˺ܶ���

	//��ֹԭʼ��������ʼ���ͻ
    while( src < end )//���׵�ַѭ����β��ַ
    {
        if( i_count == 2 && *src <= 0x03 )//�Ϻ�ܣ�ͼ6.6 page158��4���ֽ����ж�Ҫ��0x03
        {									//0x01,0x02,0x03
            *dst++ = 0x03;
            i_count = 0;//���0x03�������0
        }
		/*
		�����⵽��Щ���д��ڣ��������������һ���ֽ�ǰ����һ���µ��ֽڣ�0x03
		0x 00 00 00 -> 0x 00 00 03 00
		0x 00 00 01 -> 0x 00 00 03 01
		0x 00 00 02 -> 0x 00 00 03 02
		0x 00 00 03 -> 0x 00 00 03 03
		*/
        if( *src == 0 )	//��һ��0x00
        {
            i_count++;
        }
        else
        {
            i_count = 0;
        }
        *dst++ = *src++;
    }

	//count nalu length in byte//ͳ�Ƴ�nalu�ĳ��ȣ���λ���ֽ�
    *pi_data = dst - (uint8_t*)p_data;//p_data�Ǻ���������û������dstһֱ���ƶ�,pi_data����ר��������������³��ȵ�

    return *pi_data;
}

/****************************************************************************
 * x264_nal_decode:nal��Ԫ����
 * ����һ�������� nal (p_data) �� һ�� x264_nal_t �ṹ�� (*nal)
 * ˼·:���紫���������һ���ֽ�,��������ṹ��x264_nal_t�������int��ʾ��һ�����ṹ���ֶ�
 *      ����Ҫ�Ѵ����ĵ�һ���ֽڲ��ýṹ���е�int�ֶ�����ʾ,��ǰ3���طŵ�x264_nal_t.i_ref_idc
 *      ��5���طŵ�x264_nal_t.i_type
 *		�������x264_nal_t�������ֶ�ֵ��x264_nal_t��4���ֶΣ���NAL��һ�ֽ��ѵõ����е�ǰ2����
 ****************************************************************************/
int x264_nal_decode( x264_nal_t *nal, void *p_data, int i_data )
{
    uint8_t *src = p_data;				//��������ʼ��ַ,   unsigned char,����һ��8λ��char,һ���ֽ�
    uint8_t *end = &src[i_data];		//������������ַ,
    uint8_t *dst = nal->p_payload;		//ָ����Ч���ص���ʼ��ַ

	/*
	 * 0 1 2 3 4 5 6 7 ��һ�ֽڵ�8����
	 * | | | + + + + + ǰ3���������ȼ�;��5������nal��Ԫ����
	 * --3-- ----5----
	*/
    nal->i_type    = src[0]&0x1f;		//����ռ���ֽڵ�5λ 0x1f-> 0001 1111 ���ֽ���0001 1111��λ�룬ȡ��nal������ֵ
    nal->i_ref_idc = (src[0] >> 5)&0x03;//��1���ֽ�����5���õ����ȼ�,ռ���ֽڵ�2λ

    src++;	//��2���ֽڼ�������ֽ�

    while( src < end )	//ѭ����2���Ժ��ֽ�
    {
        if( src < end - 3 && src[0] == 0x00 && src[1] == 0x00  && src[2] == 0x03 )
			//��ʼ��ַ < ������ַ ���� ��1�ֽ� == 0x00
			//					  ���� ��2�ֽ� == 0x00
			//					  ���� ��3�ֽ� == 0x03
        {
            *dst++ = 0x00;//dst����1�ֽ�
            *dst++ = 0x00;//ͬ��

            src += 3;	//src=src+3
						/*
						 * src   �����src��ַ����ʼ�ĵ�һ�ֽ�
						 * src++ �Ƶ��˵�2�ֽ�(whileǰ���Ǿ�)
						 * src += 3 �� src = src + 3
						 * ������ʼ��ַΪ0, src = 0 + 3
						 * 0 1 2 3  
						 * ���Կ���,3ʵ���ǵ�4���ֽ���
						 * ����,0x 00 00 03 01 ��Ȼֻ�ǱȽ���ǰ3�ֽ�,����
						 * src += 3 ,ʵ���ϰѺ����01Ҳ������
						*/
            continue;
        }
        *dst++ = *src++;//�����dst��ֵ(*dst)++,�Ҳ���(*src)++
						//�Ƚ��* ,Ȼ��ָ������1
						// ������⣬++���õ�����£���������ֵ������++
						//��*src��*dst
						/* �ȼ������µ����д��룺
						* *dst = *src
						* dst++;
						* src++;
						* -----------------
						* *j++����⻹Ӧ���� *��j++��
						* *(j++)����*j
						* ע�⣺�����ȼ������������++
						*/
    }//ȥ����ӵ�0x00 00 03 01 (4�ֽ�)

    nal->i_payload = dst - (uint8_t*)p_data;//i_payload����Ч�غɵĳ���,���ֽ�Ϊ��λ
					/* ֱ�۵���⣬���صĳ���Ϊ nal->p_payload �ĵ�ַ ��
					 * 1�ֽڵ�Nal Header ��
					 * 0x00 00 03 01 (4�ֽ�)(����еĻ�) ��					 * 
					 * NAL��ʼ��ַp_data
					 * -----------------------
					 * dst���������ƶ�
					 * ��һ�Σ�uint8_t *dst = nal->p_payload;		//ָ����Ч���ص���ʼ��ַ
					 * �ڶ��Σ�*dst++ = 0x00;
					 * �����Σ�*dst++ = 0x00;
					 * ���ĴΣ�*dst++ = *src++;
					*/
					
    return 0;
}



/****************************************************************************
 * x264_malloc:X264�ڲ�������ڴ����

 ****************************************************************************/
void *x264_malloc( int i_size )
{
	#ifdef SYS_MACOSX
		/* Mac OS X always returns 16 bytes aligned memory */
		return malloc( i_size );
	#elif defined( HAVE_MALLOC_H )
		return memalign( 16, i_size );//void *memalign( size_t alignment, size_t size );  ����alignmentָ��������ֽ���,������2������������,size�������ڴ�ĳߴ�,���ص��ڴ��׵�ַ��֤�Ƕ���alignment��
	#else
		uint8_t * buf;
		uint8_t * align_buf;
		buf = (uint8_t *) malloc( i_size + 15 + sizeof( void ** ) + sizeof( int ) );
		align_buf = buf + 15 + sizeof( void ** ) + sizeof( int );	/* �ֽڶ��룬�ں�����ż��˿� */
		align_buf -= (long) align_buf & 15;	//align_buf = align_buf - ((long) align_buf & 15)
		*( (void **) ( align_buf - sizeof( void ** ) ) ) = buf;
		*( (int *) ( align_buf - sizeof( void ** ) - sizeof( int ) ) ) = i_size;
		return align_buf;	/* uint8_t * align_buf; */
	#endif
}

/****************************************************************************
 * x264_free:X264�ڴ��ͷ�

 ****************************************************************************/
void x264_free( void *p )
{
    if( p )
    {
	#if defined( HAVE_MALLOC_H ) || defined( SYS_MACOSX )
		free( p );//C���ԵĿ⺯�����ͷ��ѷ���Ŀ�
	#else
		free( *( ( ( void **) p ) - 1 ) );
	#endif
    }
}

/****************************************************************************
 * x264_realloc:X264���·���ͼ��ռ�

 ****************************************************************************/
void *x264_realloc( void *p, int i_size )
{
#ifdef HAVE_MALLOC_H
    return realloc( p, i_size );
#else
    int       i_old_size = 0;
    uint8_t * p_new;
    if( p )
    {
        i_old_size = *( (int*) ( (uint8_t*) p ) - sizeof( void ** ) -
                         sizeof( int ) );	/* ?????? */
    }
    p_new = x264_malloc( i_size );		/* �¿���һ���ڴ�飬��С�ɴ���Ĳ���ȷ�� */
    if( i_old_size > 0 && i_size > 0 )
    {
        memcpy( p_new, p, ( i_old_size < i_size ) ? i_old_size : i_size );	/* �Ѿ��ڴ���е����ݿ����� */
    }
    x264_free( p );		/* �ͷžɵ��ڴ�� */
    return p_new;		/* �����ڴ��ĵ�ַ����ȥ */
#endif
}

/****************************************************************************
 * x264_reduce_fraction:��������
 * reduce:��������[��]
 * fraction:����
 * ����4/12�����������1/3
 * http://wmnmtm.blog.163.com/blog/static/38245714201173073922482/
 ****************************************************************************/
void x264_reduce_fraction( int *n, int *d )
{
    int a = *n;
    int b = *d;
    int c;
    if( !a || !b )	/* ��a��b�У�ֻҪ�� 0 ���� */
        return;
    c = a % b;
    while(c)
    {
	a = b;
	b = c;
	c = a % b;
    }
    *n /= b;	/* ��ԭֵ�ĵ��� */
    *d /= b;	/* ��ԭֵ�ĵ��� */
}

/****************************************************************************
 * x264_slurp_file:���ļ��������Ļ�����
 * slurp:�ʳ
 * 
 ****************************************************************************/
char *x264_slurp_file( const char *filename )
{
    int b_error = 0;
    int i_size;
    char *buf;
    FILE *fh = fopen( filename, "rb" );			/* r ��ֻ����ʽ���ļ������ļ�������ڣ���������̬�ַ����������ټ�һ��b�ַ�����rb��w+b��ab������ϣ�����b �ַ��������ߺ�����򿪵��ļ�Ϊ�������ļ������Ǵ������ļ���������POSIXϵͳ������Linux������Ը��ַ��� */
    if( !fh )									/* if(fp==NULL) //���ʧ���� */
        return NULL;
    b_error |= fseek( fh, 0, SEEK_END ) < 0;	/* http://baike.baidu.com/view/656699.htm */
    b_error |= ( i_size = ftell( fh ) ) <= 0;	/* ���� ftell() ���ڵõ��ļ�λ��ָ�뵱ǰλ��������ļ��׵�ƫ���ֽ������������ʽ��ȡ�ļ�ʱ�������ļ�λ��Ƶ����ǰ���ƶ�����������ȷ���ļ��ĵ�ǰλ�á����ú���ftell()���ܷǳ����׵�ȷ���ļ��ĵ�ǰλ�á� */
    b_error |= fseek( fh, 0, SEEK_SET ) < 0;	/* ftell(fp);���ú��� ftell() Ҳ�ܷ����֪��һ���ļ��ĳ���������������У� fseek(fp, 0L,SEEK_END); len =ftell(fp); ���Ƚ��ļ��ĵ�ǰλ���Ƶ��ļ���ĩβ��Ȼ����ú���ftell()��õ�ǰλ��������ļ��׵�λ�ƣ���λ��ֵ�����ļ������ֽ����� */
    if( b_error )								/* fseek( fh, 0, SEEK_SET );��λ���ļ���ͷ */
        return NULL;
    buf = x264_malloc( i_size+2 );				/* ?+2 */
    if( buf == NULL )
        return NULL;
    b_error |= fread( buf, 1, i_size, fh ) != i_size;
    if( buf[i_size-1] != '\n' )					/* !+2 */
        buf[i_size++] = '\n';
    buf[i_size] = 0;							/* ������'\n'��Ϊʲô��Ū��0 */
    fclose( fh );								/* �ر�һ������ע�⣺ʹ��fclose()�����Ϳ��԰ѻ����������ʣ�����������������ļ��У����ͷ��ļ�ָ����йصĻ������� */
    if( b_error )								/* �����������buf */
    {
        x264_free( buf );						/*  */
        return NULL;
    }
    return buf;									/* ���ػ�����(�Ѵ����ļ�����) */
}

/****************************************************************************
 * x264_param2string:ת������Ϊ�ַ���,�����ַ�����ŵĵ�ַ

 ****************************************************************************/
char *x264_param2string( x264_param_t *p, int b_res )
{
    char *buf = x264_malloc( 1000 );
    char *s = buf;

    if( b_res )
    {
        s += sprintf( s, "%dx%d ", p->i_width, p->i_height );	/* sprintf:�Ѹ�ʽ��������д��ĳ���ַ��� http://baike.baidu.com/view/1295144.htm */
        s += sprintf( s, "fps=%d/%d ", p->i_fps_num, p->i_fps_den );
    }

    s += sprintf( s, "cabac=%d", p->b_cabac );
    s += sprintf( s, " ref=%d", p->i_frame_reference );
    s += sprintf( s, " deblock=%d:%d:%d", p->b_deblocking_filter,
                  p->i_deblocking_filter_alphac0, p->i_deblocking_filter_beta );
    s += sprintf( s, " analyse=%#x:%#x", p->analyse.intra, p->analyse.inter );
    s += sprintf( s, " me=%s", x264_motion_est_names[ p->analyse.i_me_method ] );
    s += sprintf( s, " subme=%d", p->analyse.i_subpel_refine );
    s += sprintf( s, " brdo=%d", p->analyse.b_bframe_rdo );
    s += sprintf( s, " mixed_ref=%d", p->analyse.b_mixed_references );
    s += sprintf( s, " me_range=%d", p->analyse.i_me_range );
    s += sprintf( s, " chroma_me=%d", p->analyse.b_chroma_me );
    s += sprintf( s, " trellis=%d", p->analyse.i_trellis );
    s += sprintf( s, " 8x8dct=%d", p->analyse.b_transform_8x8 );
    s += sprintf( s, " cqm=%d", p->i_cqm_preset );
    s += sprintf( s, " chroma_qp_offset=%d", p->analyse.i_chroma_qp_offset );
    s += sprintf( s, " slices=%d", p->i_threads );
    s += sprintf( s, " nr=%d", p->analyse.i_noise_reduction );
    s += sprintf( s, " decimate=%d", p->analyse.b_dct_decimate );

    s += sprintf( s, " bframes=%d", p->i_bframe );
    if( p->i_bframe )
    {
        s += sprintf( s, " b_pyramid=%d b_adapt=%d b_bias=%d direct=%d wpredb=%d bime=%d",
                      p->b_bframe_pyramid, p->b_bframe_adaptive, p->i_bframe_bias,
                      p->analyse.i_direct_mv_pred, p->analyse.b_weighted_bipred,
                      p->analyse.b_bidir_me );
    }

    s += sprintf( s, " keyint=%d keyint_min=%d scenecut=%d",
                  p->i_keyint_max, p->i_keyint_min, p->i_scenecut_threshold );

    s += sprintf( s, " rc=%s", p->rc.i_rc_method == X264_RC_ABR ?
                               ( p->rc.b_stat_read ? "2pass" : p->rc.i_vbv_buffer_size ? "cbr" : "abr" )
                               : p->rc.i_rc_method == X264_RC_CRF ? "crf" : "cqp" );
    if( p->rc.i_rc_method == X264_RC_ABR || p->rc.i_rc_method == X264_RC_CRF )
    {
        if( p->rc.i_rc_method == X264_RC_CRF )
            s += sprintf( s, " crf=%d", p->rc.i_rf_constant );
        else
            s += sprintf( s, " bitrate=%d ratetol=%.1f",
                          p->rc.i_bitrate, p->rc.f_rate_tolerance );
        s += sprintf( s, " rceq='%s' qcomp=%.2f qpmin=%d qpmax=%d qpstep=%d",
                      p->rc.psz_rc_eq, p->rc.f_qcompress,
                      p->rc.i_qp_min, p->rc.i_qp_max, p->rc.i_qp_step );
        if( p->rc.b_stat_read )
            s += sprintf( s, " cplxblur=%.1f qblur=%.1f",
                          p->rc.f_complexity_blur, p->rc.f_qblur );
        if( p->rc.i_vbv_buffer_size )
            s += sprintf( s, " vbv_maxrate=%d vbv_bufsize=%d",
                          p->rc.i_vbv_max_bitrate, p->rc.i_vbv_buffer_size );
    }
    else if( p->rc.i_rc_method == X264_RC_CQP )
        s += sprintf( s, " qp=%d", p->rc.i_qp_constant );
    if( !(p->rc.i_rc_method == X264_RC_CQP && p->rc.i_qp_constant == 0) )
    {
        s += sprintf( s, " ip_ratio=%.2f", p->rc.f_ip_factor );
        if( p->i_bframe )
            s += sprintf( s, " pb_ratio=%.2f", p->rc.f_pb_factor );
        if( p->rc.i_zones )
            s += sprintf( s, " zones" );
    }

    return buf;
}

