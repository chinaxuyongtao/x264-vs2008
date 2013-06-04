/*****************************************************************************
 * x264: h264 encoder/decoder testing program.
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: x264.c,v 1.1 2004/06/03 19:24:12 fenrir Exp $
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

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <signal.h>
#define _GNU_SOURCE
#include <getopt.h>		/* ������һ������extern char *optarg;ʵ������getopt.c�ж���� */

#ifdef _MSC_VER
#include <io.h>			/* _setmode() */
#include <fcntl.h>		/* _O_BINARY  */
#endif

#ifndef _MSC_VER
#include "config.h"		/* �ļ������޴��ļ� */
#endif

#include "common/common.h"	/*  */
#include "x264.h"
#include "muxers.h"			/* �ļ���������mkv,mp4,avi,yuv,... */

#define DATA_MAX 3000000	//���ݵ���󳤶�
uint8_t data[DATA_MAX];		//ʵ�ʴ�����ݵ����� unsigned char data[3000000] ����3000000*8����

/* Ctrl+C ��ϼ� */
static int     b_ctrl_c = 0;				/* CTRL+C �����´���ϼ�ʱ�����´˱���Ϊ1 */
static int     b_exit_on_ctrl_c = 0;		/* �Ƿ�Ҫ�� CTRL+C ��ϼ� ����ʱ�˳����� */	
static void    SigIntHandler( int a )		/* �źŲ�׽���� ����main()���õ�������׽CTRL+C��ϼ� */
{
    if( b_exit_on_ctrl_c )	/* �������CTRL+C��ϼ� */				
        exit(0);			/* ��������Ľ��� */
    b_ctrl_c = 1;			/* �Ұ�����CTRL+C */
}							//��main(...)�У�ע��������źŲ�׽�������Ƿ��ڰ���CTRL+Cʱ�˳���ȡ���ڱ���b_exit_on_ctrl_c

typedef struct {
    int b_progress;		//�Ƿ���ʾ������ȡ�ȡֵΪ0,1
    int i_seek;			//��ʾ��ʼ����һ֡����
    hnd_t hin;			//Hin ָ������yuv�ļ���ָ��	//void *��C�������ָ�����м������Եģ�����һ��һ�㻯ָ�룬����ָ���κ�һ�����ͣ���ȴ���ܽ����ã���Ҫ�����õ�ʱ����Ҫ����ǿ��ת�������ÿ�ָ��Ĳ��ԣ�Ӧ����Ϊ�����������ļ���ͳһ��
    hnd_t hout;			//Hout ָ�����������ɵ��ļ���ָ��
    FILE *qpfile;		//Qpfile ��һ��ָ���ļ����͵�ָ�룬�����ı��ļ�����ÿһ�еĸ�ʽ��framenum frametype QP,����ǿ��ָ��ĳЩ֡����ȫ��֡��֡���ͺ�QP(quant param��������)��ֵ

} cli_opt_t;	/* �˽ṹ���Ǽ�¼һЩ������ϵ��С��������Ϣ�� */

/* 
 * �����ļ������ĺ���ָ��  input file operation function pointers 
 * �������ļ�
 * ȡ��֡��
 * ��֡����
 * �ر������ļ�
*/

int (*p_open_infile)( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param );
int (*p_get_frame_total)( hnd_t handle );
int (*p_read_frame)( x264_picture_t *p_pic, hnd_t handle, int i_frame );	/* !!!!!! */
int (*p_close_infile)( hnd_t handle );

/* 
 * ����ļ������ĺ���ָ��  output file operation function pointers 
 * �� ����ļ�
 * ���� ����ļ�����
 * д   nalu
 * ���� 
 * �ر� ����ļ�
*/
static int (*p_open_outfile)( char *psz_filename, hnd_t *p_handle );
static int (*p_set_outfile_param)( hnd_t handle, x264_param_t *p_param );
static int (*p_write_nalu)( hnd_t handle, uint8_t *p_nal, int i_size );
static int (*p_set_eop)( hnd_t handle, x264_picture_t *p_picture );
static int (*p_close_outfile)( hnd_t handle );

static void Help( x264_param_t *defaults, int b_longhelp );//����
static int  Parse( int argc, char **argv, x264_param_t *param, cli_opt_t *opt );//���������в���
static int  Encode( x264_param_t *param, cli_opt_t *opt );//����


/****************************************************************************
 * main: ������ڵ�
 * ������
 * http://wmnmtm.blog.163.com
 *
 ****************************************************************************/
int main( int argc, char **argv )
{
	//���������ṹ��
    x264_param_t param;
    cli_opt_t opt;	/*һ������*/

	#ifdef _MSC_VER							//stdin��STDIO.H����ϵͳ�����
    _setmode(_fileno(stdin), _O_BINARY);	//_setmode(_fileno(stdin), _O_BINARY)�����ǽ�stdin��(�������ļ��������ı�ģʽ   <--�л�-->   ������ģʽ ����stdin��(�������ļ��������ı�ģʽ   <--�л�-->   ������ģʽ
    _setmode(_fileno(stdout), _O_BINARY);
	#endif

	//�Ա��������������趨����ʼ���ṹ�����
    x264_param_default( &param );	//(common/common.c�ж���)

    /* ����������,����ļ��� */
    if( Parse( argc, argv, &param, &opt ) < 0 )	/* ���ǰ��û�ͨ���������ṩ�Ĳ������浽�����ṹ���У�δ�ṩ�Ĳ�������x264_param_default�������õ�ֵΪ׼ */
        return -1;

    /* �ú���signalע��һ���źŲ�׽���� ʵ�֡�Ctrl+C���˳�����֮���� */
    signal( SIGINT/*Ҫ��׽���ź�*/, SigIntHandler/*�źŲ�׽����*/ );//�ú���signalע��һ���źŲ�׽��������1������signum��ʾҪ��׽���źţ���2�������Ǹ�����ָ�룬��ʾҪ�Ը��źŽ��в�׽�ĺ������ò���Ҳ������SIG_DEF(��ʾ����ϵͳȱʡ�����൱�ڰ�ע����)��SIG_IGN(��ʾ���Ե����źŶ������κδ���)��signal������óɹ���������ǰ���źŵĴ������ĵ�ַ�����򷵻� SIG_ERR��
									//sighandler_t���źŲ�׽��������signal����ע�ᣬע���Ժ��������������й����о���Ч�����ҶԲ�ͬ���źſ���ע��ͬһ���źŲ�׽�������ú���ֻ��һ����������ʾ�ź�ֵ��
	printf("\n");
	printf("************************************");	printf("\n");
	printf("**   http://wmnmtm.blog.163.com   **");	printf("\n");
	printf("************************************");	printf("\n");
	/* ��ʼ����*/
    return Encode( &param, &opt );	//�����������ṩ��Encode���������Ѿ��������������еĲ���,�˺����� x264.c �ж��� 
									//Encode�ڲ�ѭ������Encode_frame��֡����
}

/* ȡ�ַ����ĵ�i���ַ� */
static char const *strtable_lookup( const char * const table[], int index )//static��̬������ֻ�ڶ��������ļ�����Ч
{
    int i = 0; 
	while( table[i] )	/* ��table[i]Ϊ0ʱ���������ַ���ĩβʱ�˳� */
		i++;			/* �����ַ������ַ��ĸ��� */
    return ( ( index >= 0 && index < i ) ? table[ index ] : "???" ); 
						/*
						index >=0 ����˵���ñ�����ʱָ����index����������
						index < i���Ǳ�ָ֤����index���ܳ����ַ������ַ��ĸ���
						��ǰ�����������������ʱ������table[index]����һ���ַ�'*'
						���table[index]Ϊ�ǣ��򷵻�"???"
						*/
}

/*****************************************************************************

 *****************************************************************************/
/*
 * Help:
 * D:\test>x2641 --longhelp
   ��--longhelp����ʱ�������H0��H1ȫ�����
   ��--longhelp����ʱ�������H1������������H0
*/
static void Help( x264_param_t *defaults, int b_longhelp )
{
	#define H0 printf							//int printf(const char *format,[argument]); 
	#define H1 if(b_longhelp) printf
    H0( "x264 core:%d%s\n"																											/* x264 core:49 */
        "Syntax: x264 [options] -o outfile infile [��x��]\n"																	/* Syntax: x264 [options] -o outfile infile [widthxheight] */
        "\n"
        "Infile can be raw YUV 4:2:0 (in which case resolution is required),\n"														/* Infile can be raw YUV 4:2:0 (in which case resolution is required),*/
        "  or YUV4MPEG 4:2:0 (*.y4m),\n"																							/*  or YUV4MPEG 4:2:0 (*.y4m), */
        "  or AVI or Avisynth if compiled with AVIS support (%s).\n"																/*  or AVI or Avisynth if compiled with AVIS support (no). */
        "Outfile type is selected by filename:\n"																					/* Outfile type is selected by filename: */
        " .264 -> Raw bytestream\n"																									/* .264 -> Raw bytestream */
        " .mkv -> Matroska\n"																										/* .mkv -> Matroska */
        " .mp4 -> MP4 if compiled with GPAC support (%s)\n"																			/* .mp4 -> MP4 if compiled with GPAC support (no) */
        "\n"																														/* ���� */
        "Options:\n"																												/* Options: */
        "\n"																														/* ���� */
        "  -h, --help                  List the more commonly used options\n"														/*   -h, --help                  List the more commonly used options */
        "      --longhelp              List all options\n"																			/*       --longhelp              List all options */
        "\n",																														/* ���� */
        X264_BUILD, X264_VERSION,																									/* ??? */
		#ifdef AVIS_INPUT
				"yes",
		#else
				"no",
		#endif
		#ifdef MP4_OUTPUT
				"yes"
		#else
				"no"
		#endif
      );
    H0( "Frame-type options:\n" );				/* Frame-type options: */
    H0( "\n" );
    H0( "  -I, --keyint <integer>      Maximum GOP size [%d]\n", defaults->i_keyint_max );											/* [H0] */  /*  -I, --keyint <integer>      Maximum GOP size [250] */
    H1( "  -i, --min-keyint <integer>  Minimum GOP size [%d]\n", defaults->i_keyint_min );											/* [H1] */  /*  -i, --min-keyint <integer>  Minimum GOP size [25] */
    H1( "      --scenecut <integer>    How aggressively to insert extra I-frames [%d]\n", defaults->i_scenecut_threshold );			/* [H1] */  /*      --scenecut <integer>    How aggressively to insert extra I-frames [40] */
    H0( "  -b, --bframes <integer>     Number of B-frames between I and P [%d]\n", defaults->i_bframe );							/* [H0] */  /*  -b, --bframes <integer>     Number of B-frames between I and P [0] */
    H1( "      --no-b-adapt            Disable adaptive B-frame decision\n" );														/* [H1] */  /*      --no-b-adapt            Disable adaptive B-frame decision */
    H1( "      --b-bias <integer>      Influences how often B-frames are used [%d]\n", defaults->i_bframe_bias );					/* [H1] */  /*      --b-bias <integer>      Influences how often B-frames are used [0] */
    H0( "      --b-pyramid             Keep some B-frames as references\n" );														/* [H0] */  /*      --b-pyramid             Keep some B-frames as references */
    H0( "      --no-cabac              Disable CABAC\n" );																			/* [H0] */  /*      --no-cabac              Disable CABAC */
    H0( "  -r, --ref <integer>         Number of reference frames [%d]\n", defaults->i_frame_reference );							/* [H0] */  /*  -r, --ref <integer>         Number of reference frames [1] */
    H1( "      --nf                    Disable loop filter\n" );																	/* [H1] */  /*      --b-pyramid             Keep some B-frames as references */
    H0( "  -f, --filter <alpha:beta>   Loop filter AlphaC0 and Beta parameters [%d:%d]\n",											/* [H0] */  /*  -f, --filter <alpha:beta>   Loop filter AlphaC0 and Beta parameters [0:0] */
                                       defaults->i_deblocking_filter_alphac0, defaults->i_deblocking_filter_beta );					/* ??? */
    H0( "\n" );																														/* ���� */
    H0( "Ratecontrol:\n" );																											/* [H0] */  /*Ratecontrol: */
    H0( "\n" );																														/* ���� */
    H0( "  -q, --qp <integer>          Set QP (0=lossless) [%d]\n", defaults->rc.i_qp_constant );									/* [H0] */  /*  -q, --qp <integer>          Set QP (0=lossless) [26] */
    H0( "  -B, --bitrate <integer>     Set bitrate (kbit/s)\n" );																	/* [H0] */  /*  -B, --bitrate <integer>     Set bitrate (kbit/s) */
    H0( "      --crf <integer>         Quality-based VBR (nominal QP)\n" );															/* [H0] */  /*      --crf <integer>         Quality-based VBR (nominal QP) */
    H1( "      --vbv-maxrate <integer> Max local bitrate (kbit/s) [%d]\n", defaults->rc.i_vbv_max_bitrate );						/* [H1] */  /*       --vbv-maxrate <integer> Max local bitrate (kbit/s) [0] */
    H0( "      --vbv-bufsize <integer> Enable CBR and set size of the VBV buffer (kbit) [%d]\n", defaults->rc.i_vbv_buffer_size );	/* [H0] */  /*      --vbv-bufsize <integer> Enable CBR and set size of the VBV buffer (kbit) [0] */
    H1( "      --vbv-init <float>      Initial VBV buffer occupancy [%.1f]\n", defaults->rc.f_vbv_buffer_init );					/* [H1] */  /*       --vbv-init <float>      Initial VBV buffer occupancy [0.9] */
    H1( "      --qpmin <integer>       Set min QP [%d]\n", defaults->rc.i_qp_min );													/* [H1] */  /*       --qpmin <integer>       Set min QP [10] */
    H1( "      --qpmax <integer>       Set max QP [%d]\n", defaults->rc.i_qp_max );													/* [H1] */  /*       --qpmax <integer>       Set max QP [51] */
    H1( "      --qpstep <integer>      Set max QP step [%d]\n", defaults->rc.i_qp_step );											/* [H1] */  /*       --qpstep <integer>      Set max QP step [4] */
    H0( "      --ratetol <float>       Allowed variance of average bitrate [%.1f]\n", defaults->rc.f_rate_tolerance );				/* [H0] */  /*      --ratetol <float>       Allowed variance of average bitrate [1.0] */
    H0( "      --ipratio <float>       QP factor between I and P [%.2f]\n", defaults->rc.f_ip_factor );								/* [H0] */  /*      --ipratio <float>       QP factor between I and P [1.40] */
    H0( "      --pbratio <float>       QP factor between P and B [%.2f]\n", defaults->rc.f_pb_factor );								/* [H0] */  /*      --pbratio <float>       QP factor between P and B [1.30] */
    H1( "      --chroma-qp-offset <integer>  QP difference between chroma and luma [%d]\n", defaults->analyse.i_chroma_qp_offset );	/* [H1] */  /*       --chroma-qp-offset <integer>  QP difference between chroma and luma [0] */
    H0( "\n" );																														/* ���� */
    H0( "  -p, --pass <1|2|3>          Enable multipass ratecontrol\n"																/* [H0] */  /*  -p, --pass <1|2|3>          Enable multipass ratecontrol */
        "                                  - 1: First pass, creates stats file\n"													/* [H0] */  /*                                  - 1: First pass, creates stats file */
        "                                  - 2: Last pass, does not overwrite stats file\n"											/* [H0] */  /*                                  - 2: Last pass, does not overwrite stats file */
        "                                  - 3: Nth pass, overwrites stats file\n" );												/* [H0] */  /*                                  - 3: Nth pass, overwrites stats file */
    H0( "      --stats <string>        Filename for 2 pass stats [\"%s\"]\n", defaults->rc.psz_stat_out );							/* [H0] */  /*      --stats <string>        Filename for 2 pass stats ["x264_2pass.log"] */
    H1( "      --rceq <string>         Ratecontrol equation [\"%s\"]\n", defaults->rc.psz_rc_eq );									/* [H1] */  /*       --rceq <string>         Ratecontrol equation ["blurCplx^(1-qComp)"] */
    H0( "      --qcomp <float>         QP curve compression: 0.0 => CBR, 1.0 => CQP [%.2f]\n", defaults->rc.f_qcompress );			/* [H0] */  /*      --qcomp <float>         QP curve compression: 0.0 => CBR, 1.0 => CQP [0.60] */
    H1( "      --cplxblur <float>      Reduce fluctuations in QP (before curve compression) [%.1f]\n", defaults->rc.f_complexity_blur );	/* [H1]      --cplxblur <float>      Reduce fluctuations in QP (before curve compression) [20.0] */
    H1( "      --qblur <float>         Reduce fluctuations in QP (after curve compression) [%.1f]\n", defaults->rc.f_qblur );		/* [H1] */  /*       --qblur <float>         Reduce fluctuations in QP (after curve compression) [0.5] */
    H0( "      --zones <zone0>/<zone1>/...  Tweak the bitrate of some regions of the video\n" );									/* [H0] */  /*      --zones <zone0>/<zone1>/...  Tweak the bitrate of some regions of the video */
    H1( "                              Each zone is of the form\n"																	/* [H1] */  /*                               Each zone is of the form */
        "                                  <start frame>,<end frame>,<option>\n"													/* [H1] */  /*                                   <start frame>,<end frame>,<option> */
        "                                  where <option> is either\n"																/* [H1] */  /*                                   where <option> is either */
        "                                      q=<integer> (force QP)\n"															/* [H1] */  /*                                       q=<integer> (force QP) */
        "                                  or  b=<float> (bitrate multiplier)\n" );													/* [H1] */  /*                                   or  b=<float> (bitrate multiplier) */
    H1( "      --qpfile <string>       Force frametypes and QPs\n" );																/* [H1] */  /*       --qpfile <string>       Force frametypes and QPs */
    H0( "\n" );																														/* ���� */
    H0( "Analysis:\n" );																											/* [H0] */  /* Analysis: */
    H0( "\n" );																														/* ���� */
    H0( "  -A, --analyse <string>      Partitions to consider [\"p8x8,b8x8,i8x8,i4x4\"]\n"											/* [H0] */  /*  -A, --analyse <string>      Partitions to consider ["p8x8,b8x8,i8x8,i4x4"] */
        "                                  - p8x8, p4x4, b8x8, i8x8, i4x4\n"														/* [H0] */  /*                                  - p8x8, p4x4, b8x8, i8x8, i4x4 */
        "                                  - none, all\n"																			/* [H0] */  /*                                  - none, all */
        "                                  (p4x4 requires p8x8. i8x8 requires --8x8dct.)\n" );										/* [H0] */  /*                                  (p4x4 requires p8x8. i8x8 requires --8x8dct.) */
    H0( "      --direct <string>       Direct MV prediction mode [\"%s\"]\n"														/* [H0] */  /*      --direct <string>       Direct MV prediction mode ["spatial"] */
        "                                  - none, spatial, temporal, auto\n",														/* [H0] */  /*                                  - none, spatial, temporal, auto */
                                       strtable_lookup( x264_direct_pred_names, defaults->analyse.i_direct_mv_pred ) );				/* ??? */
    H0( "  -w, --weightb               Weighted prediction for B-frames\n" );														/* [H0] */  /*  -w, --weightb               Weighted prediction for B-frames */
    H0( "      --me <string>           Integer pixel motion estimation method [\"%s\"]\n",											/* [H0] */  /*      --me <string>           Integer pixel motion estimation method ["hex"] */
                                       strtable_lookup( x264_motion_est_names, defaults->analyse.i_me_method ) );					/* [H0] */  /*                                  - dia, hex, umh */
    H1( "                                  - dia: diamond search, radius 1 (fast)\n"												/* [H1] */  /*                                   - dia: diamond search, radius 1 (fast) */
        "                                  - hex: hexagonal search, radius 2\n"														/* [H1] */  /*                                   - hex: hexagonal search, radius 2 */
        "                                  - umh: uneven multi-hexagon search\n"													/* [H1] */  /*                                   - umh: uneven multi-hexagon search */
        "                                  - esa: exhaustive search (slow)\n" );													/* [H1]  */  /*                                  - esa: exhaustive search (slow) */
    else			/*���else�����������Ƶģ������������Ķ���ͬʱ�������Ȼ--help�����H1��--longhelpʱ��else���������Ķ��������elseû�κ�Ӱ��*/																												/* else */
	H0( "                                  - dia, hex, umh\n" );																	/* [H0] */  /*                                  - dia, hex, umh */
    H0( "      --merange <integer>     Maximum motion vector search range [%d]\n", defaults->analyse.i_me_range );					/* [H0] */  /*      --merange <integer>     Maximum motion vector search range [16] */
    H0( "  -m, --subme <integer>       Subpixel motion estimation and partition\n"													/* [H0] */  /*  -m, --subme <integer>       Subpixel motion estimation and partition */
        "                                  decision quality: 1=fast, 7=best. [%d]\n", defaults->analyse.i_subpel_refine );			/* [H0] */  /*                                  decision quality: 1=fast, 7=best. [5] */
    H0( "      --b-rdo                 RD based mode decision for B-frames. Requires subme 6.\n" );									/* [H0] */  /*      --b-rdo                 RD based mode decision for B-frames. Requires subme 6. */
    H0( "      --mixed-refs            Decide references on a per partition basis\n" );												/* [H0] */  /*      --mixed-refs            Decide references on a per partition basis */
    H1( "      --no-chroma-me          Ignore chroma in motion estimation\n" );														/* [H1] */  /*       --no-chroma-me          Ignore chroma in motion estimation */
    H1( "      --bime                  Jointly optimize both MVs in B-frames\n" );													/* [H1] */  /*       --bime                  Jointly optimize both MVs in B-frames */
    H0( "  -8, --8x8dct                Adaptive spatial transform size\n" );														/* [H0] */  /*  -8, --8x8dct                Adaptive spatial transform size */
    H0( "  -t, --trellis <integer>     Trellis RD quantization. Requires CABAC. [%d]\n"												/* [H0] */  /*  -t, --trellis <integer>     Trellis RD quantization. Requires CABAC. [0] */
        "                                  - 0: disabled\n"																			/* [H0] */  /*                                  - 0: disabled */
        "                                  - 1: enabled only on the final encode of a MB\n"											/* [H0] */  /*                                  - 1: enabled only on the final encode of a MB */
        "                                  - 2: enabled on all mode decisions\n", defaults->analyse.i_trellis );					/* [H0] */  /*                                  - 2: enabled on all mode decisions */
    H0( "      --no-fast-pskip         Disables early SKIP detection on P-frames\n" );												/* [H0] */  /*      --no-fast-pskip         Disables early SKIP detection on P-frames */
    H0( "      --no-dct-decimate       Disables coefficient thresholding on P-frames\n" );											/* [H0] */  /*      --no-dct-decimate       Disables coefficient thresholding on P-frames */
    H0( "      --nr <integer>          Noise reduction [%d]\n", defaults->analyse.i_noise_reduction );								/* [H0] */  /*      --nr <integer>          Noise reduction [0] */
    H1( "\n" );																														/* ���� */
    H1( "      --cqm <string>          Preset quant matrices [\"flat\"]\n"		/*Ԥ����������*/									/* [H1] */  /*       --cqm <string>          Preset quant matrices ["flat"] */
        "                                  - jvt, flat\n" );																		/* [H1] */  /*                                   - jvt, flat */
    H0( "      --cqmfile <string>      Read custom quant matrices from a JM-compatible file\n" );									/* [H0] */  /*       --cqmfile <string>      Read custom quant matrices from a JM-compatible file */
    H1( "                                  Overrides any other --cqm* options.\n" );												/* [H1] */  /*                                   Overrides any other --cqm* options. */
    H1( "      --cqm4 <list>           Set all 4x4 quant matrices\n"																/* [H1] */  /*       --cqm4 <list>           Set all 4x4 quant matrices */
        "                                  Takes a comma-separated list of 16 integers.\n" );										/* [H1] */  /*                                  Takes a comma-separated list of 16 integers. */
    H1( "      --cqm8 <list>           Set all 8x8 quant matrices\n"																/* [H1] */  /*       --cqm8 <list>           Set all 8x8 quant matrices */
        "                                  Takes a comma-separated list of 64 integers.\n" );										/* [H1] */  /*                                  Takes a comma-separated list of 64 integers. */
    H1( "      --cqm4i, --cqm4p, --cqm8i, --cqm8p\n"																				/* [H1] */  /*       --cqm4i, --cqm4p, --cqm8i, --cqm8p */
        "                              Set both luma and chroma quant matrices\n" );												/* [H1] */  /*                               Set both luma and chroma quant matrices */
    H1( "      --cqm4iy, --cqm4ic, --cqm4py, --cqm4pc\n"																			/* [H1] */  /*       --cqm4iy, --cqm4ic, --cqm4py, --cqm4pc */
        "                              Set individual quant matrices\n" );															/* [H1] */  /*                               Set individual quant matrices */
    H1( "\n" );																														/* ���� */
    H1( "Video Usability Info (Annex E):\n" );																						/* [H1] */  /* Video Usability Info (Annex E): */
    H1( "The VUI settings are not used by the encoder but are merely suggestions to\n" );											/* [H1] */  /* The VUI settings are not used by the encoder but are merely suggestions to */
    H1( "the playback equipment. See doc/vui.txt for details. Use at your own risk.\n" );											/* [H1] */  /* the playback equipment. See doc/vui.txt for details. Use at your own risk. */
    H1( "\n" );
    H1( "      --overscan <string>     Specify crop overscan setting [\"%s\"]\n"													/* [H1] */  /*       --overscan <string>     Specify crop overscan setting ["undef"] */
        "                                  - undef, show, crop\n",																	/* [H1] */  /*                                   - undef, show, crop */
                                       strtable_lookup( x264_overscan_names, defaults->vui.i_overscan ) );							/* [H1] */
    H1( "      --videoformat <string>  Specify video format [\"%s\"]\n"																/* [H1] */  /*       --videoformat <string>  Specify video format ["undef"] */
        "                                  - component, pal, ntsc, secam, mac, undef\n",											/* [H1] */  /*                                   - component, pal, ntsc, secam, mac, undef */
                                       strtable_lookup( x264_vidformat_names, defaults->vui.i_vidformat ) );						/* [H1] */
    H1( "      --fullrange <string>    Specify full range samples setting [\"%s\"]\n"												/* [H1] */  /*       --fullrange <string>    Specify full range samples setting ["off"] */
        "                                  - off, on\n",																			/* [H1] */  /*                                   - off, on */
                                       strtable_lookup( x264_fullrange_names, defaults->vui.b_fullrange ) );						/* [H1] */
    H1( "      --colorprim <string>    Specify color primaries [\"%s\"]\n"															/* [H1] */  /*       --colorprim <string>    Specify color primaries ["undef"] */
        "                                  - undef, bt709, bt470m, bt470bg\n"														/* [H1] */  /*                                   - undef, bt709, bt470m, bt470bg */
        "                                    smpte170m, smpte240m, film\n",															/* [H1] */  /*                                     smpte170m, smpte240m, film */
                                       strtable_lookup( x264_colorprim_names, defaults->vui.i_colorprim ) );						/* [H1] */
    H1( "      --transfer <string>     Specify transfer characteristics [\"%s\"]\n"													/* [H1] */  /*       --transfer <string>     Specify transfer characteristics ["undef"] */
        "                                  - undef, bt709, bt470m, bt470bg, linear,\n"												/* [H1] */  /*                                   - undef, bt709, bt470m, bt470bg, linear, */
        "                                    log100, log316, smpte170m, smpte240m\n",												/* [H1] */  /*                                     log100, log316, smpte170m, smpte240m */
                                       strtable_lookup( x264_transfer_names, defaults->vui.i_transfer ) );							/* [H1]  */
    H1( "      --colormatrix <string>  Specify color matrix setting [\"%s\"]\n"														/* [H1] */  /*       --colormatrix <string>  Specify color matrix setting ["undef"] */
        "                                  - undef, bt709, fcc, bt470bg\n"															/* [H1] */  /*                                   - undef, bt709, fcc, bt470bg */
        "                                    smpte170m, smpte240m, GBR, YCgCo\n",													/* [H1] */  /*                                     smpte170m, smpte240m, GBR, YCgCo */
                                       strtable_lookup( x264_colmatrix_names, defaults->vui.i_colmatrix ) );						/* [H1] */
    H1( "      --chromaloc <integer>   Specify chroma sample location (0 to 5) [%d]\n",												/* [H1] */  /*       --chromaloc <integer>   Specify chroma sample location (0 to 5) [0] */
                                       defaults->vui.i_chroma_loc );																/* [H1] */
    H0( "\n" );																														/* ���� */
    H0( "Input/Output:\n" );																										/* [H0] */  /* Input/Output: */
    H0( "\n" );																														/* ���� */
    H0( "  -o, --output                Specify output file\n" );																	/* [H0] */  /*   -o, --output                Specify output file */
    H0( "      --sar width:height      Specify Sample Aspect Ratio\n" );															/* [H0] */  /*       --sar width:height      Specify Sample Aspect Ratio */
    H0( "      --fps <float|rational>  Specify framerate\n" );																		/* [H0] */  /*       --fps <float|rational>  Specify framerate */
    H0( "      --seek <integer>        First frame to encode\n" );																	/* [H0] */  /*       --seek <integer>        First frame to encode */
    H0( "      --frames <integer>      Maximum number of frames to encode\n" );														/* [H0] */  /*       --frames <integer>      Maximum number of frames to encode */
    H0( "      --level <string>        Specify level (as defined by Annex A)\n" );													/* [H0] */  /*       --level <string>        Specify level (as defined by Annex A) */
    H0( "\n" );																														/* ���� */
    H0( "  -v, --verbose               Print stats for each frame\n" );																/* [H0] */  /*   -v, --verbose               Print stats for each frame */
    H0( "      --progress              Show a progress indicator while encoding\n" );												/* [H0] */  /*       --progress              Show a progress indicator while encoding */
    H0( "      --quiet                 Quiet Mode\n" );																				/* [H0] */  /*       --quiet                 Quiet Mode */
    H0( "      --no-psnr               Disable PSNR computation\n" );																/* [H0] */  /*       --no-psnr               Disable PSNR computation */
    H0( "      --threads <integer>     Parallel encoding (uses slices)\n" );														/* [H0] */  /*       --threads <integer>     Parallel encoding (uses slices) */
    H0( "      --thread-input          Run Avisynth in its own thread\n" );															/* [H0] */  /*       --thread-input          Run Avisynth in its own thread */
    H1( "      --no-asm                Disable all CPU optimizations\n" );															/* [H1] */  /*       --no-asm                Disable all CPU optimizations */
    H1( "      --visualize             �ڱ��������Ƶ����ʾ���ǵĺ������Show MB types overlayed on the encoded video\n" );											/* [H1] */  /*       --visualize             Show MB types overlayed on the encoded video */
    H1( "      --sps-id <integer>      Set SPS and PPS id numbers [%d]\n", defaults->i_sps_id );									/* [H1] */  /*       --sps-id <integer>      Set SPS and PPS id numbers [0] */
    H1( "      --aud                   Use access unit delimiters\n" );																/* [H1] */  /*       --aud                   Use access unit delimiters */
    H0( "\n" );																														/* ���� */
}


/*****************************************************************************
 * Parse				�����������в���
 * ����					��
 * int argc				����������
 * char **argv			��������
 * x264_param_t *param	���Ӳ�����ȡ��ֵ��浽�����棬�磺param->i_log_level = X264_LOG_NONE;
 * cli_opt_t *opt		���Ӳ�����ȡ��ֵ��浽�����棬�磺opt->i_seek = atoi( optarg );
 *****************************************************************************/
static int  Parse( int argc, char **argv,
                   x264_param_t *param, cli_opt_t *opt )
{
    char *psz_filename = NULL;			/* ��������������ļ��� */
    x264_param_t defaults = *param;		/* �������ڲ�����һ������������ֻ�ޱ������ڲ� main(...)��{x264_param_t param;x264_param_default( &param );Parse( argc, argv, &param, &opt );} */
    char *psz;							/*  */
    int b_avis = 0;						/* ? .avi������.avs */
    int b_y4m = 0;						/* ? .y4m */
    int b_thread_input = 0;				/*  */

    memset( opt, 0, sizeof(cli_opt_t) );/* �ڴ������00000 (optָ����ָ����ʵ���Ѿ��������ڴ棬���ֻ�ǳ�ʼ��һ������ڴ�) */

    /* 
	Default input file driver 
	=���Ķ��Ǻ���ָ�룬��x264.c(�����ļ�)��ʼ�������
	=�Ҳ�Ķ����ڡ�\muxers.c�ж���ĺ������˴��÷����ݣ�������������ָ�롱
	*/
    p_open_infile = open_file_yuv;				/*����ָ�븳ֵ*/
    p_get_frame_total = get_frame_total_yuv;	//p_get_frame_total��һ������ָ�룬�Ҳ��get_frame_total_yuv��һ���������ƣ�//muxers.c(82):int get_frame_total_yuv( hnd_t handle )
    p_read_frame = read_frame_yuv;				/*����ָ�븳ֵ*/
    p_close_infile = close_file_yuv;			/*����ָ�븳ֵ*/

    /*
	Default output file driver 
	����ļ��Ĵ�������������.mkv��.mp4����ָ���������������Ĭ�ϵ��ձ麯����δ��ָ���ĺ�׺��ʽ��������Щ������http://wmnmtm.blog.163.com/blog/static/38245714201181824344687/
	*/
    p_open_outfile = open_file_bsf;				/*����ָ�븳ֵ*/
    p_set_outfile_param = set_param_bsf;		/*����ָ�븳ֵ*/
    p_write_nalu = write_nalu_bsf;				/*����ָ�븳ֵ*/
    p_set_eop = set_eop_bsf;					/*����ָ�븳ֵ*/
    p_close_outfile = close_file_bsf;			/*����ָ�븳ֵ*/
	//printf("\n (x264.c\Parse(...) ϵ�к���ָ�븳ֵ��) p_open_outfile p_close_outfile");//zjh

    /* ���������в��� Parse command line options */
    for( ;; )//forѭ���е�"��ʼ��"��"�������ʽ"��"����"����ѡ����, ������ȱʡ, ��";"����ȱʡ��ʡ���˳�ʼ��, ��ʾ����ѭ�����Ʊ�������ֵ�� ʡ�����������ʽ, ������������ʱ���Ϊ��ѭ����ʡ��������, �򲻶�ѭ�����Ʊ������в���, ��ʱ����������м����޸�ѭ�����Ʊ�������䡣
    {
        int b_error = 0;
        int long_options_index = -1;

		#define OPT_FRAMES 256
		#define OPT_SEEK 257
		#define OPT_QPFILE 258
		#define OPT_THREAD_INPUT 259
		#define OPT_QUIET 260
		#define OPT_PROGRESS 261
		#define OPT_VISUALIZE 262
		#define OPT_LONGHELP 263

		/*option���͵Ľṹ������*/ /*struct�������һ�����ͣ����������ڴ�ռ�*/
        static struct option long_options[] =				/* option������һ���ṹ�����ͣ�����4���ֶΣ���������ÿ�Ի���������4�� */
        {
			/* ѡ������	 Ҫ�����?          flag   ֵ  */

            { "help",    no_argument,       NULL, 'h' },	/* {��������,�Ƿ��в���,���,ֵ} */	
            { "longhelp",no_argument,       NULL, OPT_LONGHELP },
            { "version", no_argument,       NULL, 'V' },	/* argument:���� */
            { "bitrate", required_argument, NULL, 'B' },	/* # define required_argument	1 */
            { "bframes", required_argument, NULL, 'b' },	/* # define no_argument		0	  */
            { "no-b-adapt", no_argument,    NULL, 0 },		/* # define optional_argument	2 */
            { "b-bias",  required_argument, NULL, 0 },
            { "b-pyramid", no_argument,     NULL, 0 },
            { "min-keyint",required_argument,NULL,'i' },
            { "keyint",  required_argument, NULL, 'I' },
            { "scenecut",required_argument, NULL, 0 },
            { "nf",      no_argument,       NULL, 0 },
            { "filter",  required_argument, NULL, 'f' },
            { "no-cabac",no_argument,       NULL, 0 },
            { "qp",      required_argument, NULL, 'q' },
            { "qpmin",   required_argument, NULL, 0 },
            { "qpmax",   required_argument, NULL, 0 },
            { "qpstep",  required_argument, NULL, 0 },			//argument:����
            { "crf",     required_argument, NULL, 0 },			/* crf */
            { "ref",     required_argument, NULL, 'r' },
            { "no-asm",  no_argument,       NULL, 0 },
            { "sar",     required_argument, NULL, 0 },
            { "fps",     required_argument, NULL, 0 },
            { "frames",  required_argument, NULL, OPT_FRAMES },	/* #define OPT_FRAMES 256 */
            { "seek",    required_argument, NULL, OPT_SEEK },	/* #define OPT_SEEK 257 */
            { "output",  required_argument, NULL, 'o' },
            { "analyse", required_argument, NULL, 'A' },
            { "direct",  required_argument, NULL, 0 },
            { "weightb", no_argument,       NULL, 'w' },
            { "me",      required_argument, NULL, 0 },
            { "merange", required_argument, NULL, 0 },
            { "subme",   required_argument, NULL, 'm' },
            { "b-rdo",   no_argument,       NULL, 0 },
            { "mixed-refs", no_argument,    NULL, 0 },
            { "no-chroma-me", no_argument,  NULL, 0 },
            { "bime",    no_argument,       NULL, 0 },
            { "8x8dct",  no_argument,       NULL, '8' },
            { "trellis", required_argument, NULL, 't' },
            { "no-fast-pskip", no_argument, NULL, 0 },
            { "no-dct-decimate", no_argument, NULL, 0 },
            { "level",   required_argument, NULL, 0 },
            { "ratetol", required_argument, NULL, 0 },
            { "vbv-maxrate", required_argument, NULL, 0 },
            { "vbv-bufsize", required_argument, NULL, 0 },
            { "vbv-init", required_argument,NULL,  0 },
            { "ipratio", required_argument, NULL, 0 },
            { "pbratio", required_argument, NULL, 0 },
            { "chroma-qp-offset", required_argument, NULL, 0 },
            { "pass",    required_argument, NULL, 'p' },
            { "stats",   required_argument, NULL, 0 },
            { "rceq",    required_argument, NULL, 0 },
            { "qcomp",   required_argument, NULL, 0 },
            { "qblur",   required_argument, NULL, 0 },
            { "cplxblur",required_argument, NULL, 0 },
            { "zones",   required_argument, NULL, 0 },
            { "qpfile",  required_argument, NULL, OPT_QPFILE },
            { "threads", required_argument, NULL, 0 },
            { "thread-input", no_argument,  NULL, OPT_THREAD_INPUT },
            { "no-psnr", no_argument,       NULL, 0 },
            { "quiet",   no_argument,       NULL, OPT_QUIET },
            { "verbose", no_argument,       NULL, 'v' },
            { "progress",no_argument,       NULL, OPT_PROGRESS },
            { "visualize",no_argument,      NULL, OPT_VISUALIZE },
            { "sps-id",  required_argument, NULL, 0 },
            { "aud",     no_argument,       NULL, 0 },
            { "nr",      required_argument, NULL, 0 },
            { "cqm",     required_argument, NULL, 0 },
            { "cqmfile", required_argument, NULL, 0 },
            { "cqm4",    required_argument, NULL, 0 },
            { "cqm4i",   required_argument, NULL, 0 },
            { "cqm4iy",  required_argument, NULL, 0 },
            { "cqm4ic",  required_argument, NULL, 0 },
            { "cqm4p",   required_argument, NULL, 0 },
            { "cqm4py",  required_argument, NULL, 0 },
            { "cqm4pc",  required_argument, NULL, 0 },
            { "cqm8",    required_argument, NULL, 0 },
            { "cqm8i",   required_argument, NULL, 0 },
            { "cqm8p",   required_argument, NULL, 0 },
            { "overscan", required_argument, NULL, 0 },
            { "videoformat", required_argument, NULL, 0 },
            { "fullrange", required_argument, NULL, 0 },
            { "colorprim", required_argument, NULL, 0 },
            { "transfer", required_argument, NULL, 0 },
            { "colormatrix", required_argument, NULL, 0 },
            { "chromaloc", required_argument, NULL, 0 },
            {0, 0, 0, 0}
        };	/* ��������ߺ��棬��Ҫ������ô��"��������"������ָ����ÿ�������Ƿ�Ҫ��"����" */

        int c = getopt_long( argc, argv, "8A:B:b:f:hI:i:m:o:p:q:r:t:Vvw",/* ��ð�ŷֳ�13�� */
                             long_options, &long_options_index);
							//������ڵ�ַ�����������c �õ����� ���в�������-o test.264 foreman.yuv  352x288������ǰ�桰-o���С�o����ASCIIֵ �� c = 111 ����ͨ��VC Debug�鿴�� getopt_long() ������getopt.c�С������õ� getopt_internal(nargc, nargv, options)Ҳ������extras/getopt.c�У�������ڵ�ַ������
							/* ��getopt.c�ж��� */
							/* ǰ����������main�Ӳ���ϵͳ�õ��ģ�һ����intһ�����ַ��� */
		//printf("\n(x264.c) getopt_long�ķ���ֵc = %c \n",c);//zjh
		//printf("%d \n",c);//zjh

        if( c == -1 )//
        {
            break;//�˳�for
        }

		

        switch( c )	//int c //����o��ӣ����������
        {
            case 'h':	/* �������� */
                Help( &defaults, 0 );		//��ʾ������Ϣ
                exit(0);					//exit�����е�ʵ���Ƿ��ظ�����ϵͳ����ʾ�����ǳɹ����н�������ʧ�����н��������ڳ������ʹ��û��ʲô̫ʵ�ʵĲ��ϰ���ϣ�һ��ʹ��������������exit(0)��exit(��0����������������������
            case OPT_LONGHELP:	/* ������ϸ */
                Help( &defaults, 1 );		//��ʾ������Ϣ(�϶��)
                exit(0);					//STDLIB.H������
            case 'V':	/* �汾��Ϣ */
				#ifdef X264_POINTVER
					printf( "x264 "X264_POINTVER"\n" );		//������ʽ������ĺ���(������ stdio.h ��)��
				#else
					printf( "x264 0.%d.X\n", X264_BUILD );	/* x264 0.49.X ||(#define X264_BUILD 49)*/
				#endif
                exit(0);
            case OPT_FRAMES:	/* 256 */	//--frames <����> ������֡��
                param->i_frame_total = atoi( optarg );//atoi(const char *);���ַ���ת���������� ����˵��: ����nptr�ַ����������һ���ǿո��ַ������ڻ��߲�������Ҳ�����������򷵻��㣬����ʼ������ת����֮���⵽�����ֻ������ \0 ʱֹͣת����������������
                break;
            case OPT_SEEK://�����У�--seek <����> �趨��ʼ֡  //

                opt->i_seek = atoi( optarg );
                break;
            case 'o':			//�����в���(��-o test.264 foreman.yuv  352x288��)��c = 111 ,������ת��case 'o'��ִ��p_open_outfile ( optarg, &opt->hout )�������뺯��open_file_bsf����������Ϊ�Զ�����д�ķ�ʽ������ļ�test.264��������nuxers.c��
								/*
								������ʾ����
								x264 --crf 22 --profile main --tune animation --preset medium --b-pyramid none 
								-o test.mp4 oldFile.avi
								���ϣ�-o���������ļ�������һ������������ļ����ڶ�����ԭʼ�ļ�
								*/
				//mp4�ļ�
                if( !strncasecmp(optarg + strlen(optarg) - 4, ".mp4", 4) )	/* strncasecmp:�Ƚ��ַ���s1��s2��ǰn���ַ��������ִ�Сд */
                {
					#ifdef MP4_OUTPUT
						p_open_outfile = open_file_mp4;//muxers.c
						p_write_nalu = write_nalu_mp4;
						p_set_outfile_param = set_param_mp4;
						p_set_eop = set_eop_mp4;
						p_close_outfile = close_file_mp4;
						printf("\n (x264.c\case 'o')");//zjh
					#else
						fprintf( stderr, "x264 [error]: not compiled with MP4 output support\n" );
						return -1;
					#endif
                }
				//MKV�ļ�
                else if( !strncasecmp(optarg + strlen(optarg) - 4, ".mkv", 4) )//int strncasecmp(const char *s1, const char *s2, size_t n) �����Ƚϲ���s1��s2�ַ���ǰn���ַ����Ƚ�ʱ���Զ����Դ�Сд�Ĳ���, ������s1��s2�ַ�����ͬ�򷵻�0; s1������s2�򷵻ش���0��ֵ; s1��С��s2�򷵻�С��0��ֵ
                {	
                    p_open_outfile = open_file_mkv;//muxers.c
                    p_write_nalu = write_nalu_mkv;
                    p_set_outfile_param = set_param_mkv;
                    p_set_eop = set_eop_mkv;
                    p_close_outfile = close_file_mkv;
                }
                if( !strcmp(optarg, "-") )
                    opt->hout = stdout;
                else if( p_open_outfile( optarg, &opt->hout ) )	/* ���ļ����ӿ����������ָ�룬ʵ�ʵ����ʵ��Ĵ��ļ��ĺ��� */
                {
                    fprintf( stderr, "x264 [error]: can't open output file `%s'\n", optarg );
                    return -1;
                }
                break;
            case OPT_QPFILE:
                opt->qpfile = fopen( optarg, "r" );
                if( !opt->qpfile )
                {
                    fprintf( stderr, "x264 [error]: can't open `%s'\n", optarg );
                    return -1;
                }
                param->i_scenecut_threshold = -1;//threshold:��ֵ  �����л���ֵ=-1
                param->b_bframe_adaptive = 0;	//b֡��Ӧ=0 adaptive:��Ӧ��
                break;
            case OPT_THREAD_INPUT:
                b_thread_input = 1;
                break;
            case OPT_QUIET:
                param->i_log_level = X264_LOG_NONE;
                param->analyse.b_psnr = 0;
                break;
            case 'v':
                param->i_log_level = X264_LOG_DEBUG;
                break;
            case OPT_PROGRESS:
                opt->b_progress = 1;
                break;
            case OPT_VISUALIZE:				//VISUALIZE:����
				#ifdef VISUALIZE
					param->b_visualize = 1;	
					b_exit_on_ctrl_c = 1;
				#else
					fprintf( stderr, "x264 [warning]: not compiled with visualization support\n" );
				#endif
                break;
            default:
            {
                int i;
                if( long_options_index < 0 )
                {
                    for( i = 0; long_options[i].name; i++ )
                        if( long_options[i].val == c )
                        {
                            long_options_index = i;	/* long_options_index�����ʵ����++ */
                            break;
                        }
                    if( long_options_index < 0 )
                    {
                        /* getopt_long already printed an error message */
                        return -1;
                    }
                }

                b_error |= x264_param_parse( param, long_options[long_options_index].name, optarg ? optarg : "true" );	/* �����������.name��Ȼ����� common/common.c�ж���*/
							/*
							main()
							{
							...
							x264_param_t param;
							...
							}
														

							Parse()
							{
							int x264_param_parse( x264_param_t *p, const char *name, const char *value )
							}

							*/
            }
        }

        if( b_error )
        {
            const char *name = long_options_index > 0 ? long_options[long_options_index].name : argv[optind-2];
            fprintf( stderr, "x264 [error]: invalid argument: %s = %s\n", name, optarg );
            return -1;
        }
    }

    /* ��ȡ�ļ��� Get the file name */
    if( optind > argc - 1 || !opt->hout )
    {
        fprintf( stderr, "x264 [error]: No %s file. Run x264 --help for a list of options.\n",
                 optind > argc - 1 ? "input" : "output" );
				/*
				D:\test>x2641 --no-b-adapt
				x264 [error]: No input file. Run x264 --help for a list of options.
				*/
        return -1;
    }
    psz_filename = argv[optind++];	/* �ļ��� */

    /* check demuxer(ƥ��) type (muxer�Ǻϲ�����Ƶ�ļ�����Ƶ�ļ�����Ļ�ļ��ϲ�Ϊĳһ����Ƶ��ʽ���磬�ɽ�a.avi, a.mp3, a.srt��muxer�ϲ�Ϊmkv��ʽ����Ƶ�ļ���demuxer�ǲ����Щ�ļ��ġ�)*/
    psz = psz_filename + strlen(psz_filename) - 1;	/* �����ַ���s��(unsigned int��)����,������'\0'����, strlen�����Ľ�����һ���������Ĺ����������ڴ��ĳ��λ�ã��������ַ�����ͷ���м�ĳ��λ�ã�������ĳ����ȷ�����ڴ����򣩿�ʼɨ�裬ֱ��������һ���ַ���������'\0'Ϊֹ��Ȼ�󷵻ؼ�����ֵ�� */
    while( psz > psz_filename && *psz != '.' ) //psz_filename + strlen(psz_filename) - 1 > psz_filename
	{
        psz--;	/* psz��ʲô���Ͱ�����ô--�� */
	}//�Ӻ������뿴�����whileѭ����ȡ���ļ����ĺ�׺�����������whileѭ���ǿ�����
	/* ʵ����֤��psz������Ѿ��Ǻ�׺�ˣ���֤���̣�
	http://wmnmtm.blog.163.com/blog/static/382457142011813638733/ 
	http://wmnmtm.blog.163.com/blog/static/3824571420118151053445/
	http://wmnmtm.blog.163.com/blog/static/3824571420118194830397/  abc100x200def.yuv 100x200.yuv
	*/
    if( !strncasecmp( psz, ".avi", 4 ) || !strncasecmp( psz, ".avs", 4 ) )	/* ��׺��.avi��.avs AVS��AviSynth�ļ�ơ���˼��AVI�ϳ���������һ����Ӱ���ļ���һ������ת��������һ������Ĺ��� , ���û����ʱ�ļ����н��ļ�����������Rip������˵��AVS���ǻ��������ǹؼ�����VirtualDubMod��MeGuid֮���ǿ��ѹ�����������ͨ��AVS���롣���Ĺ������̣��½����ı��ļ���Ȼ�󽫺�׺��Ϊ.avs���ļ��������⣬����׺������.avs���磺01.txt->01.avs��AVS�ļ��а�������һ���е��ض�������ı�����֮Ϊ"�ű�"��*/
        b_avis = 1;	/* ��.avi��.avs��׺ */									/* .avs: http://baike.baidu.com/view/63605.htm */
    if( !strncasecmp( psz, ".y4m", 4 ) )	/* .y4mҲ��һ���ļ���ʽ ��http://media.xiph.org/video/derf �кܶ�y4m������ */
        b_y4m = 1;	/* ��.y4m��׺ */		/* �������͵ı��� */
	/*
������ͷ�ļ���#include <string.h> ����
	�������壺int strncasecmp(const char *s1, const char *s2, size_t n)
��������˵����strncasecmp()�����Ƚϲ���s1��s2�ַ���ǰn���ַ����Ƚ�ʱ���Զ����Դ�Сд�Ĳ��� ����
	����ֵ ��������s1��s2�ַ�����ͬ�򷵻�0 s1������s2�򷵻ش���0��ֵ s1��С��s2�򷵻�С��0��ֵ
	*/
    if( !(b_avis || b_y4m) ) /* ����������==0��������.avi��Ҳ����.avs ��Ҳ����.y4m */ // raw(��Ȼ��; δ�ӹ�����) yuv
    {						
        if( optind > argc - 1 ) /* ??? */
        {
            /* 
			try to parse the file name 
			���Խ����ļ���
			һ���for��ʽ��for (int i=0;i<10;i++)
			for( psz = psz_filename; *psz; psz++ )
			for( psz = psz_filename; *psz��Ϊ0; psz++ )����*pszΪ��(��*psz==0 �ַ�����β��\0)ʱ�˳�ѭ��

			C/C++ char*�����ַ����Ľ�β�����Ը�ֵΪ���������ַ���һ����'\0'������һ�������ֵ�0�������������ڼ��������һЩ����Ļ�����������û���أ��������⻷����Ҳ��֪����ʲô��Ϊ���⻷����ֻ����֪������֮�䵽����û��ȴ��лл��
			������֮ǰ���ʼ��У����ᵽ�ĵ�������0ǿ�и�ֵ��char�ǣ��ڱ������ڲ���ʽ�Ľ�����һ������ת����char*��0, �����Ͱ�����0ת��Ϊһ����ַΪ�յ�ָ�롣�����ʹ�á�\0����0 ����ֵ����ȡ����Եõ��Ľ����ͬ
			*/
            for( psz = psz_filename; *psz; psz++ )	/* psz�ڴ˴����¸�ֵ�ˣ��ֱ�����������ļ���xxxx.yuv */
            {
                if( *psz >= '0' && *psz <= '9'
                    && sscanf( psz, "%ux%u", &param->i_width, &param->i_height ) == 2 )	/* ��֤��http://wmnmtm.blog.163.com/blog/static/3824571420118175830667/ */
                {
                    if( param->i_log_level >= X264_LOG_INFO )
                        fprintf( stderr, "x264 [info]: file name gives %dx%d\n", param->i_width, param->i_height );
                    break;
                }
            }
        }
        else
        {
            sscanf( argv[optind++], "%ux%u", &param->i_width, &param->i_height );	/* sscanf() - ��һ���ַ����ж�����ָ����ʽ��������ݡ�*/


        }
    }
        
    if( !(b_avis || b_y4m) && ( !param->i_width || !param->i_height ) )	/* !(b_avis || b_y4m)��ʾ�����⼸�ֺ�׺����������yuv ��( !param->i_width || !param->i_height )��ʾǰ��û��ȡ������������ֵ */
    {
        fprintf( stderr, "x264 [error]: Rawyuv input requires a resolution(�ֱ���).\n" );
        return -1;
    }

    /* open the input
	������
	*/
    {
        if( b_avis )	/* .avi��.avs */
        {
		#ifdef AVIS_INPUT
            p_open_infile = open_file_avis;				/* muxers.c�еĺ��� int open_file_avis( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param ) */
            p_get_frame_total = get_frame_total_avis;	/* muxers.c�еĺ��� int get_frame_total_avis( hnd_t handle ) */
            p_read_frame = read_frame_avis;				/* muxers.c�еĺ��� int read_frame_avis( x264_picture_t *p_pic, hnd_t handle, int i_frame ) */
            p_close_infile = close_file_avis;			/* muxers.c�еĺ��� int close_file_avis( hnd_t handle ) */
		#else
            fprintf( stderr, "x264 [error]: not compiled(����) with AVIS input support\n" );
            return -1;
		#endif
        }
        if ( b_y4m )	/* .y4m */
        {
            p_open_infile = open_file_y4m;				/* muxers.c�еĺ��� int open_file_y4m( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param ) */
            p_get_frame_total = get_frame_total_y4m;	/* muxers.c�еĺ��� int get_frame_total_y4m( hnd_t handle ) */
            p_read_frame = read_frame_y4m;				/* muxers.c�еĺ��� int read_frame_y4m( x264_picture_t *p_pic, hnd_t handle, int i_frame ) */
            p_close_infile = close_file_y4m;			/* muxers.c�еĺ��� int close_file_y4m(hnd_t handle) */
        }

        if( p_open_infile( psz_filename, &opt->hin, param ) )	/* p_open_infile��һ������ָ�룬����ָ��ͬ�ĺ�����Ҳ����˵���������ǲ�ͬ�ĺ����ĵ��� */
        {
            fprintf( stderr, "x264 [error]: could not open input file '%s'\n", psz_filename );
            return -1;
        }//��������Ѿ�����Ƶ�ļ���
    }

	#ifdef HAVE_PTHREAD
    if( b_thread_input || param->i_threads > 1 )
    {
        if( open_file_thread( NULL, &opt->hin, param ) )	/* muxers.c�еĺ��� int open_file_thread( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param ) */
        {
            fprintf( stderr, "x264 [warning]: threaded input failed\n" );
        }
        else
        {
            p_open_infile = open_file_thread;				/* muxers.c�еĺ��� int open_file_thread( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param ) */
            p_get_frame_total = get_frame_total_thread;		/* muxers.c�еĺ��� int get_frame_total_thread( hnd_t handle ) */
            p_read_frame = read_frame_thread;				/* muxers.c�еĺ��� void read_frame_thread_int( thread_input_arg_t *i ) �� int read_frame_thread( x264_picture_t *p_pic, hnd_t handle, int i_frame ) */
            p_close_infile = close_file_thread;				/* muxers.c�еĺ��� int close_file_thread( hnd_t handle ) */
        }
    }
	#endif

    return 0;
}

/*
 * 
 * ���ô���ʾ����
 * parse_qpfile( opt, &pic, i_frame + opt->i_seek );//��x264.c ֮ Encode(...)�ڲ�
 * 
*/
static void parse_qpfile( cli_opt_t *opt, x264_picture_t *pic, int i_frame )
{

    int num = -1, qp;		/* 2��int */
    char type;				/* 1��char ,'a'֮��� */

    while( num < i_frame )	/* ??? */
    {
        int ret = fscanf( opt->qpfile, "%d %c %d\n", &num, &type, &qp );
		printf("\n (x264.c:parse_qpfile)num=%d",num);printf("\n\n");//zjh//����Ч�������õ��������ûִ�й�
		
		//system("pause");//��ͣ�����������
		/* 
		%d %c %d ʮ�������� һ���ַ� ʮ�������� 
		ret ��������,3
		*/
		/*
	����������: fscanf
	������ ��: ��һ������ִ�и�ʽ������
	������ ��: int fscanf(FILE *stream, char *format,[argument...]);
	����int fscanf(�ļ�ָ�룬��ʽ�ַ����������б�); 
	��������ֵ�����ͣ���ֵ����[argument...]�ĸ���	

	�������û����������գ� 
	����%d������һ��ʮ��������.
	����%i ������ʮ���ƣ��˽��ƣ�ʮ��������������%d���ƣ������ڱ���ʱͨ������ǰ�������ֽ��ƣ�����롰0x������ʮ�����ƣ����롰0����Ϊ�˽��ơ����紮��031��ʹ��%dʱ�ᱻ����31������ʹ��%iʱ������25. 
	����%u������һ���޷���ʮ��������. 
	����%f %F %g %G : ��������ʵ����������С����ʽ��ָ����ʽ����. 
	����%x %X�� ����ʮ����������.
	����%o': ����˽�������. 
	����%s : ����һ���ַ��������ո������
	����%c : ����һ���ַ����޷������ֵ���ո���Ա����롣

	�������Ӹ�ʽ˵���ַ������η�˵��
	����L/l �������η� ����"��"����
	����h �������η� ����"��"����	

		*/
        if( num < i_frame )
            continue;
        pic->i_qpplus1 = qp+1;

		/* Ƭ���� */
        if     ( type == 'I' ) pic->i_type = X264_TYPE_IDR;		/* x264.h: #define X264_TYPE_IDR 0x0001 */
        else if( type == 'i' ) pic->i_type = X264_TYPE_I;		/*  */
        else if( type == 'P' ) pic->i_type = X264_TYPE_P;		/*  */
        else if( type == 'B' ) pic->i_type = X264_TYPE_BREF;	/*  */
        else if( type == 'b' ) pic->i_type = X264_TYPE_B;		/* Ƭ���� */
        else ret = 0;
        if( ret != 3 || qp < 0 || qp > 51 || num > i_frame )	/* num,type,qp ���ļ��������,ret�ǲ�������3  */
        {	/*
			����ֵqp������[0,51]֮�䣬
			
			*/
            fprintf( stderr, "x264 [error]: can't parse qpfile for frame %d\n", i_frame );
            fclose( opt->qpfile );			/* �ر��ļ�:STDIO.H:fclose(FILE *); */
            opt->qpfile = NULL;				/* ���½ṹ���ֶ� */
            pic->i_type = X264_TYPE_AUTO;	/* x264.c:#define X264_TYPE_AUTO	0x0000 */
            pic->i_qpplus1 = 0;				/*  */
            break;
        }
    }
}

/*****************************************************************************
 * Decode:
 *****************************************************************************/


static void x264_frame_dump( x264_t *h, x264_frame_t *fr, char *name,int k )
{
	FILE *f = fopen( name, "wb+" );
    int i, y;
    //printf("\n x264.c \ x264_frame_dump");//zjh
    if( !f )
	{
        return;
	};
    fseek( f, fr->i_frame * h->param.i_height * h->param.i_width * 3 / 2, SEEK_SET );
	//printf("(\n(x264.c-x264_frame_dump) h->param.i_height=%d, param.i_width=%d ) ",h->param.i_height,h->param.i_width);//zjh;
	//printf("\n((x264.c-x264_frame_dump)) fr->i_plane = %d",fr->i_plane);//zjh;
	
	//printf("\n\n");
	//system("pause");//��ͣ�����������

	//��һ����ͣ���ûس�������
	/*
	if (1)
	{
		int c;
		while ((c = getchar()) != '\n') 
		{
		printf("%c", c);
		}
	}
	*/

	for( i = 0; i < fr->i_plane; i++ )
	{
		for( y = 0; y < h->param.i_height / ( i == 0 ? 1 : 2 ); y++ )
		{
			fwrite( &fr->plane[i][y*fr->i_stride[i]], 1, h->param.i_width / ( i == 0 ? 1 : 2 ), f );
		}
	}

    fclose( f );

}


/*
Encode_frame
�������������ÿ֡��YUV���ݣ�Ȼ�����nal�������������ľ��幤������API x264_encoder_encode
*/

static int  Encode_frame( x264_t *h, hnd_t hout, x264_picture_t *pic )
{

    x264_picture_t pic_out;
    x264_nal_t *nal;	//�ֶΰ��������ͣ����ȼ������ش�С��������ʼ��ַ
						//ʵ���õ��������h->out�ǿ��ڴ�
    int i_nal, i;
    int i_file = 0;
	printf("\n***********************************Encode_frame(...)��in****************");//zjh

	//printf("\n i_frame=%d \n",h->i_frame);//zjh



	/*	printf("\n i_frame_offset = %d \n", h->i_frame_offset );
	 printf("\n i_frame_num = %d \n", h->i_frame_num );
	printf("\n i_poc_msb = %d \n", h->i_poc_msb );
	printf("\n i_poc_lsb = %d \n", h->i_poc_lsb );
	printf("\n i_poc = %d \n", h->i_poc );
	printf("\n i_thread_num = %d \n", h->i_thread_num );
	printf("\n i_nal_ref_idc = %d \n", h->i_nal_ref_idc );
	*/

	/*
	 * x264_encoder_encode�ڲ������´��룺
	 int     x264_encoder_encode( x264_t *h,
                             x264_nal_t **pp_nal,
							 int *pi_nal,	
                             x264_picture_t *pic_in,
                             x264_picture_t *pic_out )
	   {
	   *pp_nal = h->out.nal;
	   *pi_nal = h->out.i_nal;
	   }
	 */
	//����ͼ�񣬽��б���
	//�������h->out.nal����&nal
    if( x264_encoder_encode( h, &nal, &i_nal, pic/* ������ */, &pic_out ) < 0 )//264_encoder_encodeÿ�λ��Բ�������һ֡�������֡pic_in(��3������)
    {
        fprintf( stderr, "x264 [error]: x264_encoder_encode failed\n" );
    }

	/*
	if (1)
	{
		int tpi;
		tpi = x264_encoder_encode( h, &nal, &i_nal, pic, &pic_out );
		printf("\n (x264.t:encoder_frame) x264_encoder_encode�ķ���ֵ��%d ",tpi);//zjh ��ӡ�����ÿ�ξ�Ϊ0
	}
	*/
	printf("\n (x264.c Encode_frame(){x264_encoder_encode()��}:)i_nal=%d",i_nal);//zjh//http://wmnmtm.blog.163.com/blog/static/38245714201181104551687/
	//printf("\n~~~~~~~~~~~~~~���潫��ʼforѭ��~~~~~~~~~~~~~~~~~~~~~~~~~\n");//zjh
	for( i = 0; i < i_nal; i++ )
    {
        int i_size;
        int i_data;

        i_data = DATA_MAX;	//�ұ������3���򣬱��ļ����������
		/*
		 * ����������
		 * ���������²�����
		 * �ڿ�ͷ��0x 00 00 00 01
		 * ����NALͷ����һ�ֽ�{��ֹλ,���ȼ�,����}
		 * ���ԭʼ�������Ͽ��ܳ�ͻ�ļ���0x03
		 */
        if( ( i_size = x264_nal_encode( data, &i_data, 1, &nal[i] ) ) > 0 )//nalu ���������ǰ׺,��һ��������һ��ȫ�ֱ������Ǹ�����
        {
			//�������д�뵽����ļ���ȥ
            i_file += p_write_nalu( hout, data, i_size );//����Ǻ���ָ�����ʽ��//p_write_nalu = write_nalu_mp4;p_write_nalu = write_nalu_mkv;p_write_nalu = write_nalu_bsf;
					
			//printf("\n(for�ڲ�) i_size=%d",i_size);//zjh
			//printf("\n(for�ڲ�) i_data=%d",i_data);//zjh
			//printf("\n(for�ڲ�) i_file=%d",i_file);//zjh
			//printf("\n\n");//zjh
			//system("pause");//��ͣ�����������


        }
        else if( i_size < 0 )
        {
            fprintf( stderr, "x264 [error]: need to increase buffer size (size=%d)\n", -i_size );
        }
		
		
    }//system("pause");//��ͣ�����������

	//�ѵ�һ֡��ɶ������ļ�
//	if (i_nal == 4)
//	{
//		x264_frame_t   *frame_psnr = h->fdec;
//		x264_frame_dump(h,frame_psnr,"d:\\testfile1.test",(int)1);//zjh
//	}


    if (i_nal)
        p_set_eop( hout, &pic_out );
	
	//system("pause");//��ͣ�����������
	printf("\n***********************************Encode_frame(...)��out****************\n");//zjh
	
    return i_file;
}

/*****************************************************************************
 * Encode: ����
 * �������ִ���꣬ȫ�����빤���ͽ�����
 * 
 * 
 * 
 * 
 * 
 *****************************************************************************/
static int  Encode( x264_param_t *param, cli_opt_t *opt )
{
    x264_t *h;				//����һ���ṹ���ָ��
    x264_picture_t pic;		//����һ���ṹ��Ķ��������Ѿ��������ڴ�ռ䣬�����ٶ�̬�����ˣ����ṹ������һ�����飬��������ͼ��ģ�����ֻ�Ǵ��ָ�룬���Ժ��滹�÷���ʵ�ʴ�ͼ����ڴ棬Ȼ������������������ڴ�ĵ�ַ

    int     i_frame=0, i_frame_total;		//x264_param_t�ṹ���ֶ���Ҳ�и�i_frame_total�ֶΣ�Ĭ��ֵΪ0
    int64_t i_start, i_end;				//��i_start��ʼ���룬ֱ��i_end����
										/* i_start = x264_mdate();  i_end = x264_mdate(); */
    int64_t i_file;						//�����Ǽ���������ĳ��ȵ�
    int     i_frame_size;				//������һ�䣬���ñ���һ֡�ķ���ֵ��ֵ��
    int     i_progress;					//��¼����

    i_frame_total = p_get_frame_total( opt->hin );	/* �õ������ļ�����֡�� ����p_get_frame_total = get_frame_total_yuv����Parse���������������Ե��ú���int get_frame_total_yuv( hnd_t handle )�����ļ�muxers.c��*/
    i_frame_total -= opt->i_seek;					/* Ҫ�������֡��= Դ�ļ���֡��-��ʼ֡(��һ���ӵ�1֡��ʼ����) */
    if( ( i_frame_total == 0 || param->i_frame_total < i_frame_total ) && param->i_frame_total > 0 )	/* ��������������֡������һЩ�ж� */
        i_frame_total = param->i_frame_total;		/* ��֤i_frame_total����Ч�� */
    param->i_frame_total = i_frame_total;			/* ��ȡҪ������֡��param->i_frame_total ���������������������õ�����֡��������ṹ����ֶ�*/

    if( ( h = x264_encoder_open( param ) ) == NULL )	/* ���ļ�encoder.c�У����������������Բ���ȷ��264_t�ṹ��(h��������264_t * )���������޸ģ����Ը��ṹ����������롢Ԥ�����Ҫ�Ĳ������г�ʼ�� */
    {
        fprintf( stderr, "x264 [error]: x264_encoder_open failed\n" );		/* ��ӡ������Ϣ */
        p_close_infile( opt->hin );					/* �ر������ļ� */
        p_close_outfile( opt->hout );				/* �ر�����ļ� */
        return -1;									/* ������� */
    }

    if( p_set_outfile_param( opt->hout, param ) )	/* ��������ļ���ʽ */
    {
        fprintf( stderr, "x264 [error]: can't set outfile param\n" );//�����������������ļ�����
        p_close_infile( opt->hin );	/* �ر�YUV�ļ� �ر������ļ� */
        p_close_outfile( opt->hout );	/* �ر������ļ� �ر�����ļ� */
        return -1;
    }

    /* ����һ֡ͼ��ռ� Create a new pic (���ͼ�����õ��ڴ��׵�ַ����䵽��pic�ṹ)*/
    x264_picture_alloc( &pic, X264_CSP_I420, param->i_width, param->i_height );	/* #define X264_CSP_I420  0x0001  /* yuv 4:2:0 planar */ 

    i_start = x264_mdate();	/* ���ص�ǰ���ڣ���ʼ�ͽ���ʱ�������Լ����ܵĺ�ʱ��ÿ���Ч�� return the current date in microsecond */


    /* (ѭ������ÿһ֡) Encode frames */
    for( i_frame = 0, i_file = 0, i_progress = 0; b_ctrl_c == 0 && (i_frame < i_frame_total || i_frame_total == 0); )
    {
		printf("\n Encode() : i_frame=%d \n",i_frame);//�������ʱi_frame��0,1,2,...,300
        if( p_read_frame( &pic, opt->hin, i_frame + opt->i_seek ) )/*��ȡһ֡��������֡��Ϊprev; �����i_frame֡������ʼ֡�㣬��������Ƶ�ĵ�0֡ */
            break;

        pic.i_pts = (int64_t)i_frame * param->i_fps_den; //�ֶΣ�I_pts ��program time stamp ����ʱ�����ָʾ�����������ʱ���
														//֡��*֡��������Ҳ�����ʱ�����
														//param->i_fps_den���������е��ѱ���֡������Ϊ�ұ���һ��300֡�����У��˲�����ӡ���Ϊ0,1,2,...,298,299

//		printf("\npic.i_pts=%d;  \n",pic.i_pts);//zjh//http://wmnmtm.blog.163.com/blog/static/382457142011810113418553/
//		printf("\npic.i_pts=%d;     i_frame=%d;      param->i_fps_den=%d;   \n",\
			pic.i_pts,(int64_t)i_frame,param->i_fps_den);//zjh//http://wmnmtm.blog.163.com/blog/static/382457142011810115333413/

        if( opt->qpfile )//���������һ��ʲô���ã������Ǵ���һ�������ļ�
            ;//parse_qpfile( opt, &pic, i_frame + opt->i_seek );//parse_qpfile() Ϊ��ָ�����ļ��ж���qp��ֵ���µĽӿڣ�qpfileΪ�ļ����׵�ַ
        else//û�����õĻ�����ȫ�Զ��ˣ��Ǻ�
        {
            /* δǿ���κα������Do not force any parameters */
            pic.i_type = X264_TYPE_AUTO;
            pic.i_qpplus1 = 0;//I_qpplus1 ���˲�����1����ǰ�������������ֵ
        }

		/*����һ֡ͼ��h�����������hout�����ļ���picԤ����֡ͼ��ʵ�ʾ����������Ƶ�ļ��ж����Ķ���*/
        i_file += Encode_frame( h, opt->hout, &pic );//������ı����

        i_frame++;//�ѱ����֡��ͳ��(����1)


//		printf("\n i_frame=%d;i_file=%d;\n",i_frame,i_file);//zjh//http://wmnmtm.blog.163.com/blog/static/382457142011810115333413/
		//??i_frame��ӡ�����Ϊʲôһֱ��0//��i_frame��i_file����һ�¾��ֱ���
		
//		printf("\n pic.i_type=%d;pic.i_qpplus1=%d;\n",pic.i_type,pic.i_qpplus1);//pic.i_type=0;pic.i_qpplus1=0;


        /* ���������У�������ʾ����������̵Ľ��� update status line (up to 1000 times per input file) */
		
		if( opt->b_progress && param->i_log_level < X264_LOG_DEBUG &&		// #define X264_LOG_DEBUG 3 ���� //
            ( i_frame_total ? i_frame * 1000 / i_frame_total > i_progress	//
                            : i_frame % 10 == 0 ) )		
        {
            int64_t i_elapsed = x264_mdate() - i_start;//elapsed:(ʱ��)��ȥ;��ʼ�����ڵ�ʱ���
            double fps = i_elapsed > 0 ? i_frame * 1000000. / i_elapsed : 0;
            if( i_frame_total )
            {
                int eta = i_elapsed * (i_frame_total - i_frame) / ((int64_t)i_frame * 1000000);//���õ�ʱ��*(�ܹ�Ҫ�����֡��-�ѱ����֡��=�������֡��)/(�ѱ����֡��*1000000)������ȥ������Ԥ��ʣ��ʱ�䣬Ҳ���ǹ��Ƶĵ���ʱ�����ݹ�ȥ��Ч�ʶ�̬�Ĺ������ʱ��
                i_progress = i_frame * 1000 / i_frame_total;
                fprintf( stderr, "�ѱ����֡��:: %d/%d (%.1f%%), %.2f fps, eta %d:%02d:%02d  \r",	
                         i_frame, i_frame_total, (float)i_progress / 10, fps,
                         eta/3600, (eta/60)%60, eta%60 );//ETA��Estimated Time of Arrival��Ӣ����д��ָ Ԥ�Ƶ���ʱ��// ״̬��ʾ FPS��Frames Per Second����ÿ�봫��֡��,ÿ�������ͼ���֡����֡/��)
            }
            else
                fprintf( stderr, "�ѱ����֡��: %d, %.2f fps   \r", i_frame, fps );	// ״̬��ʾ,��������...//
            fflush( stderr ); // needed in windows
       }

    }

    /* Flush delayed(�ӳ�) B-frames */
//    do {
//        i_file +=
//        i_frame_size = Encode_frame( h, opt->hout, NULL );//������õ���i_file += Encode_frame( h, opt->hout, &pic );
//    } while( i_frame_size );
//printf("p_write_nalu=%s",p_write_nalu);
    i_end = x264_mdate();		/* ���ص�ǰʱ��return the current date in microsecond//ǰ���о䣺 i_start = x264_mdate(); */
    x264_picture_clean( &pic );	/* common.c:void x264_picture_clean( x264_picture_t *pic ){free()} x264_picture_t.x264_image_t.plane[4]�������ʵ�ʵ�ͼ�� */
    x264_encoder_close( h );	/* �ر�һ�����봦���� ncoder.c:void x264_encoder_close  ( x264_t *h ) */
    fprintf( stderr, "\n" );

    if( b_ctrl_c )	/* ������ctrl+c��ϼ� ������ֹ����һЩ��β���� */
        fprintf( stderr, "aborted(ʧ�ܵ�) at input frame %d\n", opt->i_seek + i_frame );	/* ����֡ʱʧ�� */

    p_close_infile( opt->hin );		//�ر������ļ�
    p_close_outfile( opt->hout );	//�ر�����ļ�

    if( i_frame > 0 )
    {
        double fps = (double)i_frame * (double)1000000 / (double)( i_end - i_start );/* ÿ�����֡ */

        fprintf( stderr, "encoded %d frames, %.2f fps, %.2f kb/s\n", i_frame, fps,
                 (double) i_file * 8 * param->i_fps_num /
                 ( (double) param->i_fps_den * i_frame * 1000 ) );	/* �ڴ�����ʾencoded 26 frames�Ƚ�����ʾ */
    }

    return 0;
}


/*ֱ�Ӵ�������ֵ��IPCMģʽ

  FME:fast motion estimation ����
  \tools\q_matrix_jvt.cfg ���������ļ� matrix:����
*/