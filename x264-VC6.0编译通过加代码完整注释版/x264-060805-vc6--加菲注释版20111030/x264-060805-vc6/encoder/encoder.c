/*****************************************************************************
 * x264: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: encoder.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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
#include <math.h>

#include "common/common.h"
#include "common/cpu.h"

#include "set.h"
#include "analyse.h"
#include "ratecontrol.h"
#include "macroblock.h"

#if VISUALIZE

#include "common/visualize.h"//����
#endif

//#define DEBUG_MB_TYPE
//#define DEBUG_DUMP_FRAME
//#define DEBUG_BENCHMARK

#ifdef DEBUG_BENCHMARK
	static int64_t i_mtime_encode_frame = 0;
	static int64_t i_mtime_analyse = 0;
	static int64_t i_mtime_encode = 0;
	static int64_t i_mtime_write = 0;
	static int64_t i_mtime_filter = 0;
	#define TIMER_START( d ) \
    { \
        int64_t d##start = x264_mdate();

		#define TIMER_STOP( d ) \
        d += x264_mdate() - d##start;\
    }	//��ǰʱ�� - ��ʼʱ��
#else	//����ֻ��debugʱ�����
	#define TIMER_START( d )
	#define TIMER_STOP( d )
#endif

#define NALU_OVERHEAD 5 // startcode + NAL type costs 5 bytes per frame//OVERHEAD:����
	//0x000000->0x00000300
	//0x000001->0x00000301
	//0x000002->0x00000302
	//0x000003->0x00000303
	//��Щ��4�ֽ�
	//0bit+2bit+5bit(��ֹλ+���ȼ�+NAL_unit������) =1�ֽ�
	//��5�ֽ�


/****************************************************************************
 *
 ******************************* x264 libs **********************************
 *
 ****************************************************************************/
/*
 * ���ƣ�
 * ������i_size���������صĸ���(֡�ߴ�)����Ϊ�е��ô���Ϊ��
		 h->stat.f_psnr_mean_y[i_slice_type] += x264_psnr( i_sqe_y, h->param.i_width * h->param.i_height ) 
		 h->stat.f_psnr_mean_u[i_slice_type] += x264_psnr( i_sqe_u, h->param.i_width * h->param.i_height / 4 );
		 h->stat.f_psnr_mean_v[i_slice_type] +=  x264_psnr( i_sqe_v, h->param.i_width * h->param.i_height / 4 );
  * ���أ�
 * ���ܣ���������x264_psnr()
 * ˼·��
 * ���ϣ���ֵ����ȣ�PSNR����һ������ͼ��Ŀ͹۱�׼�������о����ԣ�PSNR�� ��PeakSignaltoNoiseRatio������д��peak��������˼�Ƕ��㡣��radio����˼�Ǳ��ʻ���еġ�������˼���ǵ����������ʵĶ� ���źţ�psnr��һ�����������ֵ�źźͱ�������֮���һ��������Ŀ��ͨ���ھ���Ӱ��ѹ��֮�������Ӱ��ͨ��������ĳ�̶ֳ���ԭʼӰ��?һ����Ϊ�˺� ������������Ӱ��Ʒ�ʣ�����ͨ����ο�PSNRֵ���϶�ĳ��������򹻲����������⡣����ԭͼ���봦��ͼ��֮�������������(2^n-1)^2�Ķ��� ֵ(�ź����ֵ��ƽ����n��ÿ������ֵ�ı�����)�����ĵ�λ��dB����ʽ���£�
		 http://wmnmtm.blog.163.com/blog/static/38245714201181722213153/
 * 
*/
static float x264_psnr( int64_t i_sqe, int64_t i_size )//PSNR(��ֵ�����)
{	//ʵ�ʾ��ǹ�ʽ��ʵ�֣���ʽ���� http://wmnmtm.blog.163.com/blog/static/38245714201181722213153/
    double f_mse = (double)i_sqe / ((double)65025.0 * (double)i_size);//������Mean Square Error��
    if( f_mse <= 0.0000000001 ) /* ͼ��ѹ���е��͵ķ�ֵ�����ֵ�� 30 �� 40dB ֮��  Max 100dB */
        return 100;

    return (float)(-10.0 * log( f_mse ) / log( 10.0 ));//MATH.H:double  __cdecl log(double);
}

//#ifdef DEBUG_DUMP_FRAME
/*
 * ���ƣ�
 * ������
 * ���أ�
 * ���ܣ�x264_frame_dump(...)��x264_frame_t��plane�д洢��420��ʽ��ͼ��д��һ��ָ�����ļ�
 * ˼·���ں����ڲ��򿪡�д�롢�ر��ļ�������ֻ�ڵ���ʱ�ã�����Դ���룬δ�����ù�
 * ���ϣ�http://wmnmtm.blog.163.com/blog/static/3824571420118188540701/
 * 
*/
static void x264_frame_dump( x264_t *h, x264_frame_t *fr, char *name )//dump:�㵹
{//��x264_frame_t��plane�д洢��420��ʽ��ͼ��д��һ��ָ�����ļ���������Դ���У�δ���ù��˺���
	//�������h��ֻΪȡ��ͼ��Ŀ�͸ߣ�ʵ�ʵ�ͼ����Դ�ڵڶ�������fr
    FILE *f = fopen( name, "r+b" );//ԭΪr+b׷�� ����wb+ //���ļ�//r+ �Կɶ�д��ʽ���ļ������ļ��������;����b �ַ��������ߺ�����򿪵��ļ�Ϊ�������ļ������Ǵ������ļ���
    int i, y;
    if( !f )//����ֵ�� �ļ�˳���򿪺�ָ��������ļ�ָ��ͻᱻ���ء�����ļ���ʧ���򷵻�NULL�����Ѵ���������errno ��
        return;//һ����ԣ����ļ������һЩ�ļ���ȡ��д��Ķ����������ļ�ʧ�ܣ��������Ķ�д����Ҳ�޷�˳�����У�����һ����fopen()���������жϼ�����

    /* д��֡������ʾ˳�� ע����������Ƕ�λ����׼ȷ��λ�ã������Ǵ��ļ�ͷ��ʼд��Ҳ����ֱ����ԭ���ݺ���׷�ӣ����ֻд��9֡��ǰ���֡���ļ����Դ��ڣ�ֻ������ȫ����ʵ�������000000 Write the frame in display order */
    fseek( f, fr->i_frame * h->param.i_height * h->param.i_width * 3 / 2, SEEK_SET );//�˾�Ϊ��λ�ļ�ָ�뵽����λ��
	//f������Ϊָ��λ�ã����ļ���ͷ(SEEK_SET)��ƫ��λ��Ϊ�ڶ�����������Ĵ�С(֡�ı��xͼ��Ŀ�x��x1.5)
	//
	//һ֡��Ϊ�ü����棬Y��U��V������Ҳ�������һ��������һ�㶼��3������ǰ�涨������ֵΪ4
    for( i = 0; i < fr->i_plane; i++ )
    {
        for( y = 0; y < h->param.i_height / ( i == 0 ? 1 : 2 ); y++ )//0ʱ��������,1��2����ɫ��
        {
            fwrite( &fr->plane[i][y*fr->i_stride[i]], 1, h->param.i_width / ( i == 0 ? 1 : 2 ), f );//ˮƽ����i=0����ˮƽ������ͼ���ȣ���0ʱ��������ͼ���ȵ�һ��

        }
		//�������forʵ�ʾ��ǣ�Y������ÿ���ض�������ɫ��ˮƽ�Ƕ�ȡһ����ֱ����Ҳ�Ƕ�ȡһ(��˼���⣬���˴���д�ļ�)
    }
    fclose( f );//�ر��ļ�
	/*
	i=0:
		for(ͼ���/1)//��ÿ�����ؾ�������Y
			fwrite(Դ������,Ҫд�����ݵĵ��ֽ���,Ҫ����д��size�ֽڵ�������ĸ���,Ŀ���ļ�ָ��)
			fwrite( &fr->plane[0][0*fr->i_stride[0]], 1, h->param.i_width , f );

	i=1:
	i=2:
	i=3:	

	*/


}
//#endif


/* 
 * ���Ĭ��ֵ ����ʼ�� Fill "default" values 
 * ����˼�飬Ƭͷ��ʼ��
*/
static void x264_slice_header_init( x264_t *h, x264_slice_header_t *sh,
                                    x264_sps_t *sps, x264_pps_t *pps,
                                    int i_type, int i_idr_pic_id, int i_frame, int i_qp )
{

    x264_param_t *param = &h->param;
    int i;
	//printf("\n encoder.c:i_frame = %d",i_frame);//zjh ��ӡЧ����0��1��2��3...249,0��1��2��3 (��������Ϊ300֡)��˵��i_frame���ֵΪ250��249

    /* һ��ʼ������������ֶ� First we fill all field */
    sh->sps = sps;		/* ���в����� ��ֵ�ǵ��ú���ʱ�ṩ�� */
    sh->pps = pps;		/* ͼ������� ��ֵ�ǵ��ú���ʱ�ṩ�� */

    sh->i_type      = i_type;	/* [�Ϻ�ܣ�Page 164] �൱��slice_type��ָ��Ƭ�����ͣ�0123456789��ӦP��B��I��SP��SI */
    sh->i_first_mb  = 0;	/* ��һ����飬ע�������int�ͣ�����ָ�룬������Ӧ����һ�������Ԫ����Ű� [�Ϻ�ܣ�Page 164] �൱�� first_mb_in_slice Ƭ�еĵ�һ�����ĵ�ַ��Ƭͨ������䷨Ԫ�����궨���Լ��ĵ�ַ��Ҫע����ǣ���֡������Ӧģʽ�£���鶼�ǳɶԳ��ֵģ���ʱ���䷨Ԫ�ر�ʾ���ǵڼ������ԣ���Ӧ�ĵ�һ��������ʵ��ַӦ����2*--- */
    sh->i_last_mb   = h->sps->i_mb_width * h->sps->i_mb_height;	/* ���һ����� ���� һ��ͼ���г�x��12x10����飬���һ�������� 12x10 */
    sh->i_pps_id    = pps->i_id;	/* [�Ϻ�ܣ�Page 164] �൱�� pic_parameter_set_id ; ͼ��������������ţ�ȡֵ��Χ[0,255] */

    sh->i_frame_num = i_frame;	/* ����˳�� ��0��ʼ��249��Ȼ���ִ�0��ʼ */

    sh->b_field_pic = 0;    /* ���ڲ�֧�ֳ� Not field support for now [�Ϻ�ܣ�Page 165] �൱�� field_pic_flag; ������Ƭ���ʶͼ�����ģʽ��Ψһһ���䷨Ԫ�ء���ν�ı���ģʽ��ָ��֡���롢�����롢֡������Ӧ���롣������䷨Ԫ��ȡֵΪ1ʱ���ڳ����룬0ʱΪ�ǳ����롣 */
    sh->b_bottom_field = 1; /* ��Ȼ��֧�֣��׳� not yet used */

    sh->i_idr_pic_id = i_idr_pic_id;	/* [�Ϻ�ܣ�Page 164] �൱��idr_pic_id ;IDRͼ��ı�ʶ����ͬ��IDRͼ���в�ͬ��idr_pic_idֵ��ֵ��ע����ǣ�IDRͼ�񲻵ȼ���Iͼ��ֻ������ΪIDRͼ���I֡��������䷨Ԫ�ء��ڳ�ģʽ�£�IDR֡������������ͬ��idr_pic_idֵ��idr_pic_id��ȡֵ��Χ��[0,65536]����frame_num���ƣ�������ֵ���������Χʱ��������ѭ���ķ�ʽ���¿�ʼ������ */
										/*  -1 if nal_type != 5  */
    /* poc stuff(���), fixed(fix:ȷ��) later */
    sh->i_poc_lsb = 0;				/* [�Ϻ�ܣ�Page 166] �൱�� pic_order_cnt_lsb����POC�ĵ�һ���㷨�б��䷨Ԫ��������POCֵ����POC�ĵ�һ���㷨������ʽ�ش���POC��ֵ�������������㷨��ͨ��frame_num��ӳ��POC��ֵ��ע������䷨Ԫ�صĶ�ȡ������u(v)�����v��ֵ�����в������ľ䷨Ԫ��--+4�����صõ��ġ�ȡֵ��Χ��... */
    sh->i_delta_poc_bottom = 0;	/* [�Ϻ�ܣ�Page 166] �൱�� delta_pic_order_cnt_bottom��������ڳ�ģʽ�£������е����������Ա�����Ϊһ��ͼ�������и��Ե�POC�㷨���ֱ������������POCֵ��Ҳ����һ������ӵ��һ��POCֵ������֡ģʽ��֡������Ӧģʽ�£�һ��ͼ��ֻ�ܸ���Ƭͷ�ľ䷨Ԫ�ؼ����һ��POCֵ������H.264�Ĺ涨���������п��ܳ��ֳ����������frame_mbs_only_flag��Ϊ1ʱ��ÿ��֡��֡������Ӧ��ͼ���ڽ���������ֽ�Ϊ���������Թ�����ͼ���еĳ���Ϊ�ο�ͼ�����Ե�ʱframe_mb_only_flag��Ϊ1ʱ��֡��֡������Ӧ */
    sh->i_delta_poc[0] = 0;	/* [�Ϻ�ܣ�Page 167] �൱�� delta_pic_order_cnt[0] , delta_pic_order_cnt[1] */
    sh->i_delta_poc[1] = 0;	/*  */

    sh->i_redundant_pic_cnt = 0;	/* [�Ϻ�ܣ�Page 167] �൱�� redundant_pic_cnt;����Ƭ��id�ţ�ȡֵ��Χ[0,127] redundant:����� */

    if( !h->mb.b_direct_auto_read )	/* ??? */
    {
        if( h->mb.b_direct_auto_write )	/*  */
            sh->b_direct_spatial_mv_pred = ( h->stat.i_direct_score[1] > h->stat.i_direct_score[0] );
        else
            sh->b_direct_spatial_mv_pred = ( param->analyse.i_direct_mv_pred == X264_DIRECT_PRED_SPATIAL );
    }
    /* else b_direct_spatial_mv_pred was read from the 2pass statsfile */

    sh->b_num_ref_idx_override = 0;		/* [�Ϻ�ܣ�Page 167] �൱�� num_ref_idx_active_override_flag����ͼ��������У��������䷨Ԫ��ָ����ǰ�ο�֡������ʵ�ʿ��õĲο�֡����Ŀ����Ƭͷ����������Ծ䷨Ԫ�أ��Ը�ĳ�ض�ͼ���������ȡ�����䷨Ԫ�ؾ���ָ��Ƭͷ�Ƿ�����أ�����þ䷨Ԫ�ص���1�����������µ�num_ref_idx_10_active_minus1��...11... */
    sh->i_num_ref_idx_l0_active = 1;	/* [�Ϻ�ܣ�Page 167] �൱�� num_ref_idx_10_active_minus1 */
    sh->i_num_ref_idx_l1_active = 1;	/* [�Ϻ�ܣ�Page 167] �൱�� num_ref_idx_11_active_minus1 */

    sh->b_ref_pic_list_reordering_l0 = h->b_ref_reorder[0];	/* ���ڲο�ͼ���б�������? */
    sh->b_ref_pic_list_reordering_l1 = h->b_ref_reorder[1];	/* ���ڲο�ͼ���б�������? */

    /* ����ο��б���ʹ��Ĭ��˳�򣬴���������ͷ If the ref list isn't in the default order, construct���� reordering header */
    /* �ο��б�1 ��Ȼ����Ҫ������ֻ�Բο��б�0���������� List1 reordering isn't needed yet */
    if( sh->b_ref_pic_list_reordering_l0 )	/*  */
    {
        int pred_frame_num = i_frame;	/*  */
        for( i = 0; i < h->i_ref0; i++ )	/*  */
        {
            int diff = h->fref0[i]->i_frame_num - pred_frame_num;	/*  */
            if( diff == 0 )	/*  */
                x264_log( h, X264_LOG_ERROR, "diff frame num == 0\n" );	/*  */
            sh->ref_pic_list_order[0][i].idc = ( diff > 0 );	/*  */
            sh->ref_pic_list_order[0][i].arg = abs( diff ) - 1;	/*  */
            pred_frame_num = h->fref0[i]->i_frame_num;	/*  */
        }
    }

    sh->i_cabac_init_idc = param->i_cabac_init_idc;	/* [�Ϻ�ܣ�Page 167] �൱�� cabac_init_idc������cabac��ʼ��ʱ����ѡ��ȡֵ��ΧΪ[0,2] */

    sh->i_qp = i_qp;	/*  */
    sh->i_qp_delta = i_qp - pps->i_pic_init_qp;	/* [�Ϻ�ܣ�P147 pic_init_qp_minus26] ���ȷ��������������ĳ�ʼֵ */
    sh->b_sp_for_swidth = 0;	/* [�Ϻ�ܣ�Page 167] �൱�� sp_for_switch_flag */
    sh->i_qs_delta = 0;	/* [�Ϻ�ܣ�Page 167] �൱�� slice_qs_delta����slice_qp_delta�������ƣ�����SI��SP�У�����ʽ���㣺... */

    /* ���ʵ�ʵ�qp<=15 ,�����û���κ�Ч�� If effective��Ч��,ʵ�ʵ� qp <= 15, deblocking��� would�� have noû�� effectЧ�� anyway */
    if( param->b_deblocking_filter
        && ( h->mb.b_variable_qp
        || 15 < i_qp + 2 * X264_MAX(param->i_deblocking_filter_alphac0, param->i_deblocking_filter_beta) ) )
    {
        sh->i_disable_deblocking_filter_idc = 0;	/* [�Ϻ�ܣ�Page 167] �൱�� disable_deblocking_filter_idc��H.264�涨һ�׿����ڽ������˶����ؼ���ͼ���и��߽���˲�ǿ�Ƚ��� */
    }		//disable:����
    else
    {
        sh->i_disable_deblocking_filter_idc = 1;	/* ȥ�� H.264 Э�� 200503 �汾 8.7 С�� */
    }
    sh->i_alpha_c0_offset = param->i_deblocking_filter_alphac0 << 1;	/*  */
    sh->i_beta_offset = param->i_deblocking_filter_beta << 1;	/*  */
}
/*
��˵��
http://bbs.chinavideo.org/viewthread.php?tid=10671
disable_deblocking_filter_idc = 0�����б߽�ȫ�˲�
disable_deblocking_filter_idc = 1�����б߽綼���˲�
disable_deblocking_filter_idc = 2��Ƭ�߽粻�˲�

*/


/*
 * 
   
*/
static void x264_slice_header_write( bs_t *s, x264_slice_header_t *sh, int i_nal_ref_idc )
{
    int i;	/*  */

    bs_write_ue( s, sh->i_first_mb );	/*  */
    bs_write_ue( s, sh->i_type + 5 );   /* same type things */
    bs_write_ue( s, sh->i_pps_id );	/*  */
    bs_write( s, sh->sps->i_log2_max_frame_num, sh->i_frame_num );	/*  */

    if( sh->i_idr_pic_id >= 0 ) /* NAL IDR */
    {
        bs_write_ue( s, sh->i_idr_pic_id );	/*  */
    }

    if( sh->sps->i_poc_type == 0 )	/*  */
    {
        bs_write( s, sh->sps->i_log2_max_poc_lsb, sh->i_poc_lsb );	/*  */
        if( sh->pps->b_pic_order && !sh->b_field_pic )	/*  */
        {
            bs_write_se( s, sh->i_delta_poc_bottom );	/*  */
        }
    }
    else if( sh->sps->i_poc_type == 1 && !sh->sps->b_delta_pic_order_always_zero )	/*  */
    {
        bs_write_se( s, sh->i_delta_poc[0] );	/*  */
        if( sh->pps->b_pic_order && !sh->b_field_pic )	/*  */
        {
            bs_write_se( s, sh->i_delta_poc[1] );	/*  */
        }
    }

    if( sh->pps->b_redundant_pic_cnt )	/*  */
    {
        bs_write_ue( s, sh->i_redundant_pic_cnt );	/*  */
    }

    if( sh->i_type == SLICE_TYPE_B )	/*  */
    {
        bs_write1( s, sh->b_direct_spatial_mv_pred );	/*  */
    }
    if( sh->i_type == SLICE_TYPE_P || sh->i_type == SLICE_TYPE_SP || sh->i_type == SLICE_TYPE_B )
    {
        bs_write1( s, sh->b_num_ref_idx_override );	/*  */
        if( sh->b_num_ref_idx_override )	/*  */
        {
            bs_write_ue( s, sh->i_num_ref_idx_l0_active - 1 );	/*  */
            if( sh->i_type == SLICE_TYPE_B )	/*  */
            {
                bs_write_ue( s, sh->i_num_ref_idx_l1_active - 1 );	/*  */
            }
        }
    }

    /* ����ͼ���б������� ref pic list reordering */
    if( sh->i_type != SLICE_TYPE_I )	/* IƬ */
    {
        bs_write1( s, sh->b_ref_pic_list_reordering_l0 );	/*  */
        if( sh->b_ref_pic_list_reordering_l0 )	/*  */
        {
            for( i = 0; i < sh->i_num_ref_idx_l0_active; i++ )	/*  */
            {
                bs_write_ue( s, sh->ref_pic_list_order[0][i].idc );	/*  */
                bs_write_ue( s, sh->ref_pic_list_order[0][i].arg );	/*  */
                        
            }
            bs_write_ue( s, 3 );	/*  */
        }
    }
    if( sh->i_type == SLICE_TYPE_B )	/* BƬ */
    {
        bs_write1( s, sh->b_ref_pic_list_reordering_l1 );	/*  */
        if( sh->b_ref_pic_list_reordering_l1 )	/*  */
        {
            for( i = 0; i < sh->i_num_ref_idx_l1_active; i++ )	/*  */
            {
                bs_write_ue( s, sh->ref_pic_list_order[1][i].idc );	/*  */
                bs_write_ue( s, sh->ref_pic_list_order[1][i].arg );	/*  */
            }
            bs_write_ue( s, 3 );	/*  */
        }
    }

    if( ( sh->pps->b_weighted_pred && ( sh->i_type == SLICE_TYPE_P || sh->i_type == SLICE_TYPE_SP ) ) ||
        ( sh->pps->b_weighted_bipred == 1 && sh->i_type == SLICE_TYPE_B ) )	/*  */
    {
        /* FIXME ��������*/
    }

    if( i_nal_ref_idc != 0 )	/* [�Ϻ�� 159ҳ] nal_ref_idc��ָʾ��ǰNAL�����ȼ���ȡֵ��Χ[0,1,2,3]��ֵԽ�ߣ���ʾ��ǰNALԽ��Ҫ��Խ��Ҫ�����ܵ�������H.264�涨�����ǰNAL��һ�����в�������������һ��ͼ����������������ڲο�ͼ���Ƭ��Ƭ��������Ҫ�����ݵ�λʱ�����䷨Ԫ�ر������0�����Ǵ���0ʱ�����ȡ��ֵ����û�н�һ���Ĺ涨��ͨ��˫�����������ƶ����ԡ���nal_unit_type����5ʱ��nal_ref_idc����0��nal_unit_type����6��9��10��11��12ʱ��nal_ref_idc����0���Ϻ��159ҳnal_ref_idc��ָʾ��ǰNAL�����ȼ���ȡֵ��Χ[0,1,2,3]��ֵԽ�ߣ���ʾ��ǰNALԽ��Ҫ��Խ��Ҫ�����ܵ�������H.264�涨�����ǰNAL��һ�����в�������������һ��ͼ����������������ڲο�ͼ���Ƭ��Ƭ��������Ҫ�����ݵ�λʱ�����䷨Ԫ�ر������0�����Ǵ���0ʱ�����ȡ��ֵ����û�н�һ���Ĺ涨��ͨ��˫�����������ƶ����ԡ���nal_unit_type����5ʱ��nal_ref_idc����0��nal_unit_type����6��9��10��11��12ʱ��nal_ref_idc����0�� */
    {
        if( sh->i_idr_pic_id >= 0 )	/* [�Ϻ�ܣ�Page 164] �൱��idr_pic_id ;IDRͼ��ı�ʶ����ͬ��IDRͼ���в�ͬ��idr_pic_idֵ��ֵ��ע����ǣ�IDRͼ�񲻵ȼ���Iͼ��ֻ������ΪIDRͼ���I֡��������䷨Ԫ�ء��ڳ�ģʽ�£�IDR֡������������ͬ��idr_pic_idֵ��idr_pic_id��ȡֵ��Χ��[0,65536]����frame_num���ƣ�������ֵ���������Χʱ��������ѭ���ķ�ʽ���¿�ʼ������  */
        {
            bs_write1( s, 0 );  /* û���������ͼ���־ no output of prior���ȵ� pics flag */
            bs_write1( s, 0 );  /* ���ڲο���־ long term reference flag */
        }
        else
        {
            bs_write1( s, 0 );  /* adaptive_ref_pic_marking_mode_flag ��Ӧ_�ο�_ͼ��_���_ģʽ_��־*/
        }
    }

    if( sh->pps->b_cabac && sh->i_type != SLICE_TYPE_I )	/* ͼ�������b_cacbc��Ϊ0 && I���� */
    {
        bs_write_ue( s, sh->i_cabac_init_idc );	/*  */
    }
    bs_write_se( s, sh->i_qp_delta );      /* slice qp delta */

    if( sh->pps->b_deblocking_filter_control )	/*  */
    {
        bs_write_ue( s, sh->i_disable_deblocking_filter_idc );	/*  */
        if( sh->i_disable_deblocking_filter_idc != 1 )	/*  */
        {
            bs_write_se( s, sh->i_alpha_c0_offset >> 1 );	/*  */
            bs_write_se( s, sh->i_beta_offset >> 1 );	/*  */
        }
    }
}

/****************************************************************************
 *
 ****************************************************************************
 ****************************** External(����) API*************************
 ****************************************************************************
 *
 ****************************************************************************/

static int x264_validate_parameters( x264_t *h )//���ṹ���Ƿ���Ч����ͼУ������
{
    if( h->param.i_width <= 0 || h->param.i_height <= 0 )	/* ������h.param. �� �� ��<=0 */
    {
        x264_log( h, X264_LOG_ERROR, "invalid width x height (%dx%d)\n",	/* ������־:����Ŀ�x�� */
                  h->param.i_width, h->param.i_height );	/*  */
        return -1;	//�˳�
    }

    if( h->param.i_width % 2 || h->param.i_height % 2 )	/*  */
    {
        x264_log( h, X264_LOG_ERROR, "width or height not divisible�ɳ����� by 2 (%dx%d)\n",
                  h->param.i_width, h->param.i_height );	/* ���߲��ܱ�2���� */
        return -1;
    }
    if( h->param.i_csp != X264_CSP_I420 )
    {
        x264_log( h, X264_LOG_ERROR, "invalid CSP (only I420 supported)\n" );	/*  */
        return -1;
    }

    if( h->param.i_threads == 0 )//������������0�����óɺ�cpu����һ��
        h->param.i_threads = x264_cpu_num_processors();	/* ����cpu����������i_threads */
    h->param.i_threads = x264_clip3( h->param.i_threads, 1, X264_SLICE_MAX );	/* v������[i_min,i_max]�����ʱ������������߽�,�������ұ�ʱ�����������ұ߽磬��������£�����v */
    h->param.i_threads = X264_MIN( h->param.i_threads, (h->param.i_height + 15) / 16 );	/*  */
#ifndef HAVE_PTHREAD//���û�ж���HAVE_PTHREAD
    if( h->param.i_threads > 1 )	/*  */
    {
		//�����ö��߳�֧�ֱ���
        x264_log( h, X264_LOG_WARNING, "not compiled with pthread support!\n");
        x264_log( h, X264_LOG_WARNING, "multislicing anyway, but you won't see any speed gain.\n" );
    }
#endif

    if( h->param.rc.i_rc_method < 0 || h->param.rc.i_rc_method > 2 )	/*  */
    {
        x264_log( h, X264_LOG_ERROR, "invalid RC method\n" );	/*  */
        return -1;
    }
    h->param.rc.i_rf_constant = x264_clip3( h->param.rc.i_rf_constant, 0, 51 );	/*  */
    h->param.rc.i_qp_constant = x264_clip3( h->param.rc.i_qp_constant, 0, 51 );	/*  */
    if( h->param.rc.i_rc_method == X264_RC_CRF )	/*  */
        h->param.rc.i_qp_constant = h->param.rc.i_rf_constant;	/*  */
    if( (h->param.rc.i_rc_method == X264_RC_CQP || h->param.rc.i_rc_method == X264_RC_CRF)	/*  */
        && h->param.rc.i_qp_constant == 0 )	/*  */
    {
        h->mb.b_lossless = 1;	/*  */
        h->param.i_cqm_preset = X264_CQM_FLAT;	/*  */
        h->param.psz_cqm_file = NULL;	/*  */
        h->param.rc.i_rc_method = X264_RC_CQP;	/*  */
        h->param.rc.f_ip_factor = 1;	/*  */
        h->param.rc.f_pb_factor = 1;	/*  */
        h->param.analyse.b_transform_8x8 = 0;	/*  */
        h->param.analyse.b_psnr = 0;	/*  */
        h->param.analyse.i_chroma_qp_offset = 0;	/*  */
        h->param.analyse.i_trellis = 0;	/*  */
        h->param.analyse.b_fast_pskip = 0;	/*  */
        h->param.analyse.i_noise_reduction = 0;	/*  */
        h->param.analyse.i_subpel_refine = x264_clip3( h->param.analyse.i_subpel_refine, 1, 6 );	/*  */
    }

    if( ( h->param.i_width % 16 || h->param.i_height % 16 ) && !h->mb.b_lossless )	/*  */
    {
        x264_log( h, X264_LOG_WARNING, 	/*  */
                  "width or height not divisible by 16 (%dx%d), compression will suffer.\n",	/*  */
                  h->param.i_width, h->param.i_height );	/*  */
    }

    h->param.i_frame_reference = x264_clip3( h->param.i_frame_reference, 1, 16 );	/*  */
    if( h->param.i_keyint_max <= 0 )	/*  */
        h->param.i_keyint_max = 1;	/*  */
    h->param.i_keyint_min = x264_clip3( h->param.i_keyint_min, 1, h->param.i_keyint_max/2+1 );

    h->param.i_bframe = x264_clip3( h->param.i_bframe, 0, X264_BFRAME_MAX );
    h->param.i_bframe_bias = x264_clip3( h->param.i_bframe_bias, -90, 100 );
    h->param.b_bframe_pyramid = h->param.b_bframe_pyramid && h->param.i_bframe > 1;
    h->param.b_bframe_adaptive = h->param.b_bframe_adaptive && h->param.i_bframe > 0;
    h->param.analyse.b_weighted_bipred = h->param.analyse.b_weighted_bipred && h->param.i_bframe > 0;
    h->mb.b_direct_auto_write = h->param.analyse.i_direct_mv_pred == X264_DIRECT_PRED_AUTO
                                && h->param.i_bframe
                                && ( h->param.rc.b_stat_write || !h->param.rc.b_stat_read );

    h->param.i_deblocking_filter_alphac0 = x264_clip3( h->param.i_deblocking_filter_alphac0, -6, 6 );
    h->param.i_deblocking_filter_beta    = x264_clip3( h->param.i_deblocking_filter_beta, -6, 6 );

    h->param.i_cabac_init_idc = x264_clip3( h->param.i_cabac_init_idc, 0, 2 );

    if( h->param.i_cqm_preset < X264_CQM_FLAT || h->param.i_cqm_preset > X264_CQM_CUSTOM )
        h->param.i_cqm_preset = X264_CQM_FLAT;

    if( h->param.analyse.i_me_method < X264_ME_DIA ||
        h->param.analyse.i_me_method > X264_ME_ESA )
        h->param.analyse.i_me_method = X264_ME_HEX;
    if( h->param.analyse.i_me_range < 4 )
        h->param.analyse.i_me_range = 4;
    if( h->param.analyse.i_me_range > 16 && h->param.analyse.i_me_method <= X264_ME_HEX )
        h->param.analyse.i_me_range = 16;
    h->param.analyse.i_subpel_refine = x264_clip3( h->param.analyse.i_subpel_refine, 1, 7 );
    h->param.analyse.b_bframe_rdo = h->param.analyse.b_bframe_rdo && h->param.analyse.i_subpel_refine >= 6;
    h->param.analyse.b_mixed_references = h->param.analyse.b_mixed_references && h->param.i_frame_reference > 1;
    h->param.analyse.inter &= X264_ANALYSE_PSUB16x16|X264_ANALYSE_PSUB8x8|X264_ANALYSE_BSUB16x16|
                              X264_ANALYSE_I4x4|X264_ANALYSE_I8x8;
    h->param.analyse.intra &= X264_ANALYSE_I4x4|X264_ANALYSE_I8x8;
    if( !(h->param.analyse.inter & X264_ANALYSE_PSUB16x16) )
        h->param.analyse.inter &= ~X264_ANALYSE_PSUB8x8;
    if( !h->param.analyse.b_transform_8x8 )
    {
        h->param.analyse.inter &= ~X264_ANALYSE_I8x8;
        h->param.analyse.intra &= ~X264_ANALYSE_I8x8;
    }
    h->param.analyse.i_chroma_qp_offset = x264_clip3(h->param.analyse.i_chroma_qp_offset, -12, 12);
    if( !h->param.b_cabac )
        h->param.analyse.i_trellis = 0;
    h->param.analyse.i_trellis = x264_clip3( h->param.analyse.i_trellis, 0, 2 );
    h->param.analyse.i_noise_reduction = x264_clip3( h->param.analyse.i_noise_reduction, 0, 1<<16 );

    {
        const x264_level_t *l = x264_levels;
        while( l->level_idc != 0 && l->level_idc != h->param.i_level_idc )
            l++;
        if( l->level_idc == 0 )
        {
            x264_log( h, X264_LOG_ERROR, "invalid level_idc: %d\n", h->param.i_level_idc );
            return -1;
        }
        if( h->param.analyse.i_mv_range <= 0 )
            h->param.analyse.i_mv_range = l->mv_range;
        else
            h->param.analyse.i_mv_range = x264_clip3(h->param.analyse.i_mv_range, 32, 2048);
    }

    if( h->param.rc.f_qblur < 0 )
        h->param.rc.f_qblur = 0;
    if( h->param.rc.f_complexity_blur < 0 )
        h->param.rc.f_complexity_blur = 0;

    h->param.i_sps_id &= 31;

    /* ensure the booleans are 0 or 1 so they can be used in math */
#define BOOLIFY(x) h->param.x = !!h->param.x
    BOOLIFY( b_cabac );
    BOOLIFY( b_deblocking_filter );
    BOOLIFY( analyse.b_transform_8x8 );
    BOOLIFY( analyse.b_bidir_me );
    BOOLIFY( analyse.b_chroma_me );
    BOOLIFY( analyse.b_fast_pskip );
    BOOLIFY( rc.b_stat_write );
    BOOLIFY( rc.b_stat_read );
#undef BOOLIFY

    return 0;
}

/****************************************************************************
 * x264_encoder_open:
 * ����x264���������������б�������������������ǶԲ���ȷ�Ĳ��������޸�,���Ը��ṹ�������cabac ����,Ԥ�����Ҫ�Ĳ������г�ʼ��
 ****************************************************************************/
x264_t *x264_encoder_open   ( x264_param_t *param )
{
    x264_t *h = x264_malloc( sizeof( x264_t ) );	/* ����ռ䲢���г�ʼ��,x264_malloc( )��common.c�� */
    int i;

    memset( h, 0, sizeof( x264_t ) );	/* ��ʼ����̬������ڴ�ռ� */

    /* ����һ�������Ŀ��� Create a copy of param */
    memcpy( &h->param, param, sizeof( x264_param_t ) );	/* �Ѵ������Ĳ�������һ�ݵ���̬������ڴ��� */

    if( x264_validate_parameters( h ) < 0 )	/* ����x264_validate_parameters( h )��encoder.c��,����Ϊ�жϲ����Ƿ���Ч�����Բ����ʵĲ��������޸� */
    {
        x264_free( h );	/*  */
        return NULL;
    }//validate:ȷ��,ʹ��Ч

    if( h->param.psz_cqm_file )	/* һ���ַ���(char *)��ָ��һ���ļ� */
        if( x264_cqm_parse_file( h, h->param.psz_cqm_file ) < 0 )	/* ��������ļ���ȡ����һЩֵ����h���ֶ��� */
        {
            x264_free( h );	/* �����ļ�ʧ�ܣ����ٱ����� */
            return NULL;
        }

	/* x264_param_t/rc�ӽṹ */
    if( h->param.rc.psz_stat_out )	/* �ַ���(char *) */
        h->param.rc.psz_stat_out = strdup( h->param.rc.psz_stat_out );	/*  */
    if( h->param.rc.psz_stat_in )	/* �ַ���(char *) */
        h->param.rc.psz_stat_in = strdup( h->param.rc.psz_stat_in );	/*  */
    if( h->param.rc.psz_rc_eq )		/* �ַ���(char *) */
        h->param.rc.psz_rc_eq = strdup( h->param.rc.psz_rc_eq );	/*  */
		/*
		ͷ�ļ���#include <string.h>
	�����÷���char *strdup(char *s);
		���ܣ������ַ���s
		˵��������ָ�򱻸��Ƶ��ַ�����ָ��(���ַ���)������ռ���malloc()�����ҿ�����free()�ͷš�
		���ӣ�
		char *s="Golden Global View";
		char *d;
		clrscr();
		d=strdup(s);
		printf("%s",d);
		free(d); 			  
		*/


    /* VUI */
    if( h->param.vui.i_sar_width > 0 && h->param.vui.i_sar_height > 0 )	/*  */
    {
        int i_w = param->vui.i_sar_width;	/*  */
        int i_h = param->vui.i_sar_height;

        x264_reduce_fraction( &i_w, &i_h );	/* ��������,common.c */

        while( i_w > 65535 || i_h > 65535 ) /* 65536��2��16�η� */
        {
            i_w /= 2; /*  */
            i_h /= 2; /*  */
        }

        h->param.vui.i_sar_width = 0; /* ��ֵ0 */
        h->param.vui.i_sar_height = 0; /* ��ֵ0  */
        if( i_w == 0 || i_h == 0 ) /*  */
        {
            x264_log( h, X264_LOG_WARNING, "cannot create valid(��Ч��) sample aspect(����) ratio\n" );	/* ��־ */
        }
        else
        {
            x264_log( h, X264_LOG_INFO, "using SAR=%d/%d\n", i_w, i_h );	/* ��־ */
            h->param.vui.i_sar_width = i_w; /* ��ֵ */
            h->param.vui.i_sar_height = i_h; /* ��ֵ */
        }
    }

    x264_reduce_fraction( &h->param.i_fps_num, &h->param.i_fps_den );	/* ��������,common.c */
																		/* i_fps_num:��Ƶ����֡�� i_fps_den:���������͵����ı�ֵ������ʾ֡�� common.c */
    /* ��ʼ��x264_t�ṹ��  Init x264_t */
	/* out�ӽṹ:�ֽ������ */
    h->out.i_nal = 0;
    h->out.i_bitstream = X264_MAX( 1000000, h->param.i_width * h->param.i_height * 1.7
        * ( h->param.rc.i_rc_method == X264_RC_ABR ? pow( 0.5, h->param.rc.i_qp_min )
          : pow( 0.5, h->param.rc.i_qp_constant ) * X264_MAX( 1, h->param.rc.f_ip_factor )));/* �����ֵ */

	//��̬�����ڴ�
    h->out.p_bitstream = x264_malloc( h->out.i_bitstream );/* �������nal */

    h->i_frame = 0;
    h->i_frame_num = 0;
    h->i_idr_pic_id = 0;

	//���в�����h->sps��ʼ��
    h->sps = &h->sps_array[0];
    x264_sps_init( h->sps, h->param.i_sps_id, &h->param );

    h->pps = &h->pps_array[0];
    x264_pps_init( h->pps, h->param.i_sps_id, &h->param, h->sps);

    x264_validate_levels( h );

    x264_cqm_init( h );
    
    h->mb.i_mb_count = h->sps->i_mb_width * h->sps->i_mb_height;

    /* ��ʼ��֡ Init frames. */
    h->frames.i_delay = h->param.i_bframe;
    h->frames.i_max_ref0 = h->param.i_frame_reference;
    h->frames.i_max_ref1 = h->sps->vui.i_num_reorder_frames;
    h->frames.i_max_dpb  = h->sps->vui.i_max_dec_frame_buffering + 1;
    h->frames.b_have_lowres = !h->param.rc.b_stat_read
        && ( h->param.rc.i_rc_method == X264_RC_ABR
          || h->param.rc.i_rc_method == X264_RC_CRF
          || h->param.b_bframe_adaptive );

    for( i = 0; i < X264_BFRAME_MAX + 3; i++ )
    {
        h->frames.current[i] = NULL;
        h->frames.next[i]    = NULL;
        h->frames.unused[i]  = NULL;
    }
    for( i = 0; i < 1 + h->frames.i_delay; i++ )//delay:�ӳ�
    {
        h->frames.unused[i] =  x264_frame_new( h );
        if( !h->frames.unused[i] )
            return NULL;
    }
    for( i = 0; i < h->frames.i_max_dpb; i++ )
    {
        h->frames.reference[i] = x264_frame_new( h );//reference:�ο�
        if( !h->frames.reference[i] )
            return NULL;
    }
    h->frames.reference[h->frames.i_max_dpb] = NULL;
    h->frames.i_last_idr = - h->param.i_keyint_max;
    h->frames.i_input    = 0;
    h->frames.last_nonb  = NULL;

    h->i_ref0 = 0;
    h->i_ref1 = 0;

    h->fdec = h->frames.reference[0];

    if( x264_macroblock_cache_init( h ) < 0 )//��黺���ʼ����
        return NULL;
    x264_rdo_init( );	/*  */	//rdo:��ʧ���Ż����ԣ��Ϻ��:P87

    /* init CPU functions */
    x264_predict_16x16_init( h->param.cpu, h->predict_16x16 );	/*  */
    x264_predict_8x8c_init( h->param.cpu, h->predict_8x8c );	/*  */
    x264_predict_8x8_init( h->param.cpu, h->predict_8x8 );		/*  */
    x264_predict_4x4_init( h->param.cpu, h->predict_4x4 );		/*  */

    x264_pixel_init( h->param.cpu, &h->pixf );		/*  */
    x264_dct_init( h->param.cpu, &h->dctf );		/*  */
    x264_mc_init( h->param.cpu, &h->mc );			/*  */
    x264_csp_init( h->param.cpu, h->param.i_csp, &h->csp );	/*  */
    x264_quant_init( h, h->param.cpu, &h->quantf );			/*  */
    x264_deblock_init( h->param.cpu, &h->loopf );			/*  */

    memcpy( h->pixf.mbcmp,
            ( h->mb.b_lossless || h->param.analyse.i_subpel_refine <= 1 ) ? h->pixf.sad : h->pixf.satd,
            sizeof(h->pixf.mbcmp) );	/*  */

    /* �ٶȿ��� rate control */
    if( x264_ratecontrol_new( h ) < 0 )
        return NULL;

    x264_log( h, X264_LOG_INFO, "using cpu capabilities %s%s%s%s%s%s\n",
             param->cpu&X264_CPU_MMX ? "MMX " : "",
             param->cpu&X264_CPU_MMXEXT ? "MMXEXT " : "",
             param->cpu&X264_CPU_SSE ? "SSE " : "",
             param->cpu&X264_CPU_SSE2 ? "SSE2 " : "",
             param->cpu&X264_CPU_3DNOW ? "3DNow! " : "",
             param->cpu&X264_CPU_ALTIVEC ? "Altivec " : "" );

    h->thread[0] = h;
    h->i_thread_num = 0;
    for( i = 1; i < h->param.i_threads; i++ )
        h->thread[i] = x264_malloc( sizeof(x264_t) );//Ϊ�����̴߳���x264_t���󣬵����ǵ�ֵ���Ķ������أ�

	#ifdef DEBUG_DUMP_FRAME		//DUMP:ת��,�㵹
    {
        /* create or truncate(�ض�) the reconstructed(�ؽ���) video file */
        FILE *f = fopen( "fdec.yuv", "w" );
        if( f )
            fclose( f );
        else
        {
            x264_log( h, X264_LOG_ERROR, "can't write to fdec.yuv\n" );
            x264_free( h );
            return NULL;
        }
    }
	#endif

    return h;
}

/****************************************************************************
 * x264_encoder_reconfig:
 ****************************************************************************/
int x264_encoder_reconfig( x264_t *h, x264_param_t *param )
{
    h->param.i_bframe_bias = param->i_bframe_bias;
    h->param.i_deblocking_filter_alphac0 = param->i_deblocking_filter_alphac0;
    h->param.i_deblocking_filter_beta    = param->i_deblocking_filter_beta;
    h->param.analyse.i_me_method = param->analyse.i_me_method;
    h->param.analyse.i_me_range = param->analyse.i_me_range;
    h->param.analyse.i_subpel_refine = param->analyse.i_subpel_refine;
    h->param.analyse.i_trellis = param->analyse.i_trellis;
    h->param.analyse.intra = param->analyse.intra;
    h->param.analyse.inter = param->analyse.inter;

    memcpy( h->pixf.mbcmp,
            ( h->mb.b_lossless || h->param.analyse.i_subpel_refine <= 1 ) ? h->pixf.sad : h->pixf.satd,
            sizeof(h->pixf.mbcmp) );

    return x264_validate_parameters( h );
}

/* internal�ڲ��� usage�÷�,ϰ�� 
 * 
*/
static void x264_nal_start( x264_t *h, int i_type, int i_ref_idc )
{
	//��ǰ�ǵڼ���nal��һ��ͼ����ܲ����ü���nal
    x264_nal_t *nal = &h->out.nal[h->out.i_nal];//x264_nal_t  nal[X264_NAL_MAX]; 

	//������ʵ����&h->out.nal[h->out.i_nal]
    nal->i_ref_idc = i_ref_idc;		//���ȼ�
    nal->i_type    = i_type;		//����

    bs_align_0( &h->out.bs );   /* not needed */

    nal->i_payload= 0;//payload:��Ч�غɳ���
    nal->p_payload= &h->out.p_bitstream[bs_pos( &h->out.bs) / 8];//��ʼ��ַ  bitstream:������
										//bs_pos:��ȡ��ǰ����ȡ���ĸ�λ���ˣ����ܵķ���ֵ�ǣ�105��106
}

static void x264_nal_end( x264_t *h )
{
    x264_nal_t *nal = &h->out.nal[h->out.i_nal];

    bs_align_0( &h->out.bs );   /* not needed */

    nal->i_payload = &h->out.p_bitstream[bs_pos( &h->out.bs)/8] - nal->p_payload;////bitstream:������  payload:��Ч�غ�

    h->out.i_nal++;
}

/****************************************************************************
 * x264_encoder_headers:
 * �������û�ã����ڲ�ȫע�͵��Կ��������룬����������Ҳδ�����е������ĵط�
 * �����������PPS��SPS���ǲ�������Ҫʱ������Ȼ�����ⷢ�������ǿ��Ե�
 ****************************************************************************/
int x264_encoder_headers( x264_t *h, x264_nal_t **pp_nal, int *pi_nal )
{
    /* init bitstream(������) context(������,����) */
    h->out.i_nal = 0;
    bs_init( &h->out.bs, h->out.p_bitstream, h->out.i_bitstream );

    /* Put SPS(���в�����) and PPS(ͼ�������) */
    if( h->i_frame == 0 )
    {
        /* identify(ʶ��,֧��) ourself(����) */
        x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
        x264_sei_version_write( h, &h->out.bs );//SEI_�汾_д��
        x264_nal_end( h );	//

        /* generate(����) sequence(�Ⱥ����, ˳��, ����) parameters(����) */
		//���Ӧ�����������в�����SPS
        x264_nal_start( h, NAL_SPS, NAL_PRIORITY_HIGHEST );	//
        x264_sps_write( &h->out.bs, h->sps );	//���в�����_д��
        x264_nal_end( h );	//

        /* generate(����) picture parameters(����ͼ�������PPS) */
        x264_nal_start( h, NAL_PPS, NAL_PRIORITY_HIGHEST );	/* PRIORITY:����Ȩ HIGHEST:��ߵ� */
        x264_pps_write( &h->out.bs, h->pps );	//ͼ�������_д��
        x264_nal_end( h );
    }
    /* ������� now set output */
    *pi_nal = h->out.i_nal;
    *pp_nal = &h->out.nal[0];

    return 0;
}

/*
 * ���ƣ�
 * ���ܣ�->ջ����һ֡�ŵ�list�ο��������ں��棬�½����ķ���list[i]������i��һ��Ϊ0
 * ������
 * ���أ�
 * ���ϣ�
 * ˼·��
 *  i
 * [0]  
 * [1]
 * [2]
 * [3]
 * [4] <---------�½��
*/
static void x264_frame_put( x264_frame_t *list[X264_BFRAME_MAX], x264_frame_t *frame )//->ջ����һ֡�ŵ�list�ο��������ں��棬�½����ķ���list[i]������i��һ��Ϊ0
{
    int i = 0;
    while( list[i] ) i++;
    list[i] = frame;
}

/*
 * ���ƣ�
 * ���ܣ�push:ѹջ,��, �ƶ����ƶ����½����ķ���list[0]
 * ������
 * ���أ�
 * ���ϣ�
 * ˼·��
 *  i
 * [0] <---------�½��
 * [1]
 * [2]
 * [3]
 * [4]
*/
static void x264_frame_push( x264_frame_t *list[X264_BFRAME_MAX], x264_frame_t *frame )
{
    int i = 0;
    while( list[i] ) i++;	//�ҵ�i�����ֵ
    while( i-- )
        list[i+1] = list[i];		//�������������ƣ����ѵ�i������Ƶ���i+1����㣬�����Ͱ���ջ���ճ�����
    list[0] = frame;				//ջ�������ṩ��ָ��(ָ��һ���ṹ�����)
}

/*
 * ���ƣ�
 * ���ܣ�ȡ������ȡ��0��Ԫ�أ����ǵ�0֡
 * ������
 * ���أ�
 * ���ϣ�#define X264_BFRAME_MAX 16
 * ˼·��
 * 
*/
static x264_frame_t *x264_frame_get( x264_frame_t *list[X264_BFRAME_MAX+1] )//ȡ������ȡ��0��Ԫ�أ����ǵ�0֡
{
    x264_frame_t *frame = list[0];	/* ȡ������ȡ��0��Ԫ�أ����ǵ�0֡ */
    int i;
    for( i = 0; list[i]; i++ )//for(i = 0 ; list[0]; i++)//�о�����ȡ����0֡��Ȼ������֡��ǰ��һ��
        list[i] = list[i+1];//��һ��ǰ��
    return frame;	//����list[0]
}


/*
 * ���ƣ�
 * ���ܣ�
 * ��������һ��������֡�ṹ���ͣ��ڶ���������0��1��ָ���Ƿ���...��ֻ����һ���ط���
		 #define x264_frame_sort_dts(list) x264_frame_sort(list, 1)
		 #define x264_frame_sort_pts(list) x264_frame_sort(list, 0)
 * ���أ�
 * ���ϣ�sort:����,����,�ѡ�����
 * ˼·��
 * 
*/
static void x264_frame_sort( x264_frame_t *list[X264_BFRAME_MAX+1], int b_dts )
{
    int i, b_ok;
    do {
        b_ok = 1;
        for( i = 0; list[i+1]; i++ )
        {
            int dtype = list[i]->i_type - list[i+1]->i_type;
            int dtime = list[i]->i_frame - list[i+1]->i_frame;
            int swap = b_dts ? dtype > 0 || ( dtype == 0 && dtime > 0 )
                             : dtime > 0;
            if( swap )
            {
                XCHG( x264_frame_t*, list[i], list[i+1] );////a��b��ֵ����,��common.h�ﶨ��(#define XCHG(type,a,b) { type t = a; a = b; b = t; }//����a��b)
                b_ok = 0;
            }
        }
    } while( !b_ok );
}
#define x264_frame_sort_dts(list) x264_frame_sort(list, 1)
#define x264_frame_sort_pts(list) x264_frame_sort(list, 0)

/*
reference:�ο�
build:����, ����
�ο�_����_�б�

Encode_frame(...)����x264_encoder_encode(...),x264_encoder_encode(...)�ٵ���x264_reference_build_list(...)
���ϣ�http://wmnmtm.blog.163.com/blog/static/38245714201181234153295/
main()�ж���һ��x264_param_t����param��������ѡ��--keyint��ֵ��2��5... ���������param��һ���ֶ���x264_param_t.i_keyint_max 

���Ա���������ָ���Ĳο�֡�������������Ĳο�֡�б����keyintָ����5�������for��ѭ��5��
������keyint������--ref http://wmnmtm.blog.163.com/blog/static/3824571420118191170628/
keyint���趨x264�����������֮���IDR֡�����Ϊ�ؼ�֡�����,Ĭ��ֵ��250 

��JM����Ĳο�֡�Ƿ�ǰ��ͺ���ο��ģ�list0������ǰ��ο��ģ�list1�Ǻ���ο��ģ���ÿһ��list����ŵĲο�֡���ֶַ��ںͳ��ڲο��������ǰ��������У������ǰ��������е�
������û�п��Ƕ��ںͳ��ڲο��������ˣ�ֻ�ǽ��ο�֡�ĵȼ���ΪHIGHEST��DISPOSABLE(һ���Ե�)

*/
static inline void x264_reference_build_list( x264_t *h, int i_poc, int i_slice_type )
{
    int i;
    int b_ok;

    /* build ref list 0/1 �����ο��б�0�Ͳο��б�1*/
    h->i_ref0 = 0;
    h->i_ref1 = 0;

	
    for( i = 1; i < h->frames.i_max_dpb; i++ )	/* i_max_dpbͼ�񻺳����з����֡������ --ref �������� */
    {	/* ���������� --ref n ָ�������ο�֡����ѭ������ */
        if( h->frames.reference[i]->i_poc >= 0 )		//x264_t�ṹ���frames�ӽṹ�У�x264_frame_t *reference[16+2+1+2]
        {//ע����������Ѿ����ܱ�ʹ�ù�
            if( h->frames.reference[i]->i_poc < i_poc )	//��������i_poc�����ο�֡i�ȵ�ǰ֡�Ĳ���˳��С��reference:�ο�
            {
				//�����б�0
				//list0 ?
                h->fref0[h->i_ref0++] = h->frames.reference[i];	//list0//x264 --crf 22 --ref 6 -o test.264 hall_cif.yuv 352x288��������ʱ��ִֻ�����
				//printf("a\n");//zjh
            }
            else if( h->frames.reference[i]->i_poc > i_poc )	//frames;//ָʾ�Ϳ���֡������̵Ľṹ
            {
				//list1 ?
                h->fref1[h->i_ref1++] = h->frames.reference[i];	//list1
				//printf("b\n");//zjh
            }
        }
//		printf(" h->fenc->i_frame=%d,  i_ref0=%d,  i_ref1=%d \n",h->fenc->i_frame,h->i_ref0,h->i_ref1);//zjh//http://wmnmtm.blog.163.com/blog/static/38245714201181234153295/
		//printf("           i_ref1=%d  \n",h->fenc->i_frame,h->i_ref1);//zjh
    }

    /* 
	Order ref0 from higher to lower poc 
	����ο�0����POC�Ӹߵ������� 
	�൱��JM�����list0����ref0�еĲο�֡����POC��������
	*/
    do
    {
        b_ok = 1;
        for( i = 0; i < h->i_ref0 - 1; i++ )//i_ref0��ĳ���ط�Ӧ�ûᱻ���ϵĸ��£����Ӧ���ǲο�������Ľ����
        {
            if( h->fref0[i]->i_poc < h->fref0[i+1]->i_poc )//�����������һ����i_poc�Ƚ�
            {
                XCHG( x264_frame_t*, h->fref0[i], h->fref0[i+1] );//����a��b:XCHG(type,a,b) { type t = a; a = b; b = t; }//����a��b
                b_ok = 0;
                break;
            }
        }
    } while( !b_ok );
    /* 
	Order ref1 from lower to higher poc (bubble��,ˮ��,���� sort���� [ð������] ) for B-frame (B֡)
	���вο�1����POC�ӵ͵��߶�B-֡
	*/
    do
    {
        b_ok = 1;
        for( i = 0; i < h->i_ref1 - 1; i++ )
        {
            if( h->fref1[i]->i_poc > h->fref1[i+1]->i_poc )
            {
                XCHG( x264_frame_t*, h->fref1[i], h->fref1[i+1] );//����a��b:XCHG(type,a,b),common.h�ﶨ�壬ʵ��
                b_ok = 0;
                break;
            }
        }
    } while( !b_ok );

    /* In the standard, a P-frame's ref list is sorted����� by frame_num.
     * We use POC, but check��� whether�Ƿ� explicit��ȷ�� reordering is needed��Ҫ 
	 * �ڱ�׼�һ��p֡�Ĳο�������ͨ��frame_num�����
	 * ����ʹ��POC�����Ǽ���ǲ�ʱȷȷ����������Ȼ�Ǳ�Ҫ��
	 * ���������µ�һ���ж���Ԫ�ص��������������״̬
	*/
    h->b_ref_reorder[0] =
    h->b_ref_reorder[1] = 0;
    if( i_slice_type == SLICE_TYPE_P )
    {
        for( i = 0; i < h->i_ref0 - 1; i++ )
            if( h->fref0[i]->i_frame_num < h->fref0[i+1]->i_frame_num )
            {
                h->b_ref_reorder[0] = 1;
                break;
            }
    }

    h->i_ref1 = X264_MIN( h->i_ref1, h->frames.i_max_ref1 );//����Сֵ(�ο�1)
    h->i_ref0 = X264_MIN( h->i_ref0, h->frames.i_max_ref0 );//����Сֵ(�ο�0)
    h->i_ref0 = X264_MIN( h->i_ref0, 16 - h->i_ref1 );
}

/*
deblock:���
deblocking:���
disable:ʹ��Ч
*/
static inline void x264_fdec_deblock( x264_t *h )//
{
    /* 
	apply(ʹ��) deblocking(ȥ��) filter(�˲���) to the current(��ǰ) decoded(����) picture(ͼ��) 
	ʹ��ȥ���˲������Ե�ǰ�����ͼ��
	*/
    if( !h->sh.i_disable_deblocking_filter_idc )//
    {
        TIMER_START( i_mtime_filter );
        x264_frame_deblocking_filter( h, h->sh.i_type );//֡ȥ���˲�������
        TIMER_STOP( i_mtime_filter );
    }
}


/*
 * ���ƣ�x264_reference_update
 * ���ܣ�����Ҫ�Ĺ����ǽ���һ���ο�֡����ο�֡���У�
		 ���ӿ���֡������ȡ��һ֡��Ϊ��ǰ�Ĳο�����֡�������������Ŀ��֡������h->fdec��
		 ������h->frames.reference ������Ҫ�Ĳο�֡��Ȼ����ݲο�֡���еĴ�С���ƣ��Ƴ���ʹ�õĲο�֡
 * ������
 * ���أ�
 * ���ϣ�
 * ˼·��
 * 
*/
static inline void x264_reference_update( x264_t *h )
{
    int i;

    x264_fdec_deblock( h );

    /* expand border���� */
    x264_frame_expand_border( h->fdec );

    /* create filtered images */
    x264_frame_filter( h->param.cpu, h->fdec );

    /* expand border of filtered images */
    x264_frame_expand_border_filtered( h->fdec );

    /* move lowres copy of the image to the ref frame */
    for( i = 0; i < 4; i++)
        XCHG( uint8_t*, h->fdec->lowres[i], h->fenc->lowres[i] );

    /* adaptive B decision�ж� needs a pointer, since�Դӡ�֮�� it can't use the ref lists */
    if( h->sh.i_type != SLICE_TYPE_B )
        h->frames.last_nonb = h->fdec;

    /* move frame in the buffer */
    h->fdec = h->frames.reference[h->frames.i_max_dpb-1];
    for( i = h->frames.i_max_dpb-1; i > 0; i-- )
    {
        h->frames.reference[i] = h->frames.reference[i-1];
    }
    h->frames.reference[0] = h->fdec;//h->fdec:���ڱ��ؽ���֡
}

/*
 * ���ƣ�
 * ���ܣ�
 * ������
 * ���أ�
 * ���ϣ�
 * ˼·��ȫ���ο�֡��poc��Ϊ-1���ѵ�һ����Ϊ0
 * 
*/
static inline void x264_reference_reset( x264_t *h )//reference:�ο�
{
    int i;

    /* 
	�������� reset ref pictures 
	����������i_max_dpb��ָ�ؽ�֡������
	���Ǿ�Ӣ�ķ���������i_max_dpb���ڽ����ͼ�񻺳����з����֡��������Ҳ�������˼
	�����������ѭ�������Ǳ����ؽ�֡���߿ɲο�֡
	*/
	/*
	printf("\n (filename:function) h->frames.i_max_dpb=%d",h->frames.i_max_dpb);//zjh
	printf("\n (filename:function) h->sps->vui.i_max_dec_frame_buffering=%d",h->sps->vui.i_max_dec_frame_buffering);//zjh
	��������--ref ʱ�����2 ��������--ref 12 ��ʱ������ı�ָ��ֵ��1��--ref 16ʱ�����17 ,ref�ٴ�Ҳֻ���17
	ref�������ͣ����ƽ���ͼƬ���壨DPB��Decoded Picture Buffer���Ĵ�С����Χ�Ǵ�0��16����֮����ֵ��ÿ��P֡����ʹ����ǰ����֡��Ϊ����֡����Ŀ
	printf("\n\n");//zjh
	system("pause");//��ͣ�����������	
	*/
    for( i = 1; i < h->frames.i_max_dpb;/* --ref �޸�*/ i++ )	/* i_max_dpb:Number of frames allocated (����) in the decoded picture buffer */
    {
        h->frames.reference[i]->i_poc = -1;	//ȫ���ο�֡��poc��Ϊ-1
    }
    h->frames.reference[0]->i_poc = 0;		//�ѵ�һ����Ϊ0 /* poc��ʶͼ��Ĳ���˳�� ���Ϻ�ܣ���160ҳ */
}
/*
 * ���ƣ�
 * ���ܣ�
 * ������
 * ���أ�
 * ���ϣ�
 * ˼·��
 * 
*/
static inline void x264_slice_init( x264_t *h, int i_nal_type, int i_slice_type, int i_global_qp )
{
    /* ------------------------ Create slice header  ����Ƭͷ ----------------------- */
    if( i_nal_type == NAL_SLICE_IDR )
    {
        x264_slice_header_init( h, &h->sh, h->sps, h->pps, i_slice_type, h->i_idr_pic_id, h->i_frame_num, i_global_qp );

        /* increment id ���� id */
        h->i_idr_pic_id = ( h->i_idr_pic_id + 1 ) % 65536;
    }
    else
    {
        x264_slice_header_init( h, &h->sh, h->sps, h->pps, i_slice_type, -1, h->i_frame_num, i_global_qp );

        /* always set the real higher num of ref frame used */
        h->sh.b_num_ref_idx_override = 1;
        h->sh.i_num_ref_idx_l0_active = h->i_ref0 <= 0 ? 1 : h->i_ref0;
        h->sh.i_num_ref_idx_l1_active = h->i_ref1 <= 0 ? 1 : h->i_ref1;
    }

    h->fdec->i_frame_num = h->sh.i_frame_num;

    if( h->sps->i_poc_type == 0 )
    {
        h->sh.i_poc_lsb = h->fdec->i_poc & ( (1 << h->sps->i_log2_max_poc_lsb) - 1 );
        h->sh.i_delta_poc_bottom = 0;   /* XXX won't work for field */
    }
    else if( h->sps->i_poc_type == 1 )
    {
        /* FIXME TODO FIXME */
    }
    else
    {
        /* Nothing to do ? */
    }

    x264_macroblock_slice_init( h );
}

/*
 * ���ƣ�
 * ������
 * ���أ�
 * ���ܣ�
 * ˼·��
 * ���ϣ�
 * 
*/
static int x264_slice_write( x264_t *h )
{
    int i_skip;
    int mb_xy;
    int i;
	//printf("HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH");
    /* init stats */
    memset( &h->stat.frame, 0, sizeof(h->stat.frame) );

    /* Slice */
    x264_nal_start( h, h->i_nal_type, h->i_nal_ref_idc );	//�������ȼ������͡��غɳ��ȡ��غ���ʼ��ַ

    /* Slice header */
    x264_slice_header_write( &h->out.bs, &h->sh, h->i_nal_ref_idc ); /* Slice header */
    if( h->param.b_cabac )
    {
        /* alignment needed */
        bs_align_1( &h->out.bs );

        /* init cabac */
        x264_cabac_context_init( &h->cabac, h->sh.i_type, h->sh.i_qp, h->sh.i_cabac_init_idc );//��ʼ��������
        x264_cabac_encode_init ( &h->cabac, &h->out.bs );
    }
    h->mb.i_last_qp = h->sh.i_qp;
    h->mb.i_last_dqp = 0;

    for( mb_xy = h->sh.i_first_mb, i_skip = 0; mb_xy < h->sh.i_last_mb; mb_xy++ )
    {
        const int i_mb_y = mb_xy / h->sps->i_mb_width;
        const int i_mb_x = mb_xy % h->sps->i_mb_width;

        int mb_spos = bs_pos(&h->out.bs);

        /* load cache */
        x264_macroblock_cache_load( h, i_mb_x, i_mb_y );

        /* analyse parameters
         * Slice I: choose I_4x4 or I_16x16 mode
         * Slice P: choose between using P mode or intra (4x4 or 16x16)
         * */
        TIMER_START( i_mtime_analyse );
        x264_macroblock_analyse( h );
        TIMER_STOP( i_mtime_analyse );

        /* encode this macrobock -> be carefull it can change the mb type to P_SKIP if needed */
        TIMER_START( i_mtime_encode );
        x264_macroblock_encode( h );
        TIMER_STOP( i_mtime_encode );

        TIMER_START( i_mtime_write );
        if( h->param.b_cabac )
        {
            if( mb_xy > h->sh.i_first_mb )
                x264_cabac_encode_terminal( &h->cabac, 0 );

            if( IS_SKIP( h->mb.i_type ) )
                x264_cabac_mb_skip( h, 1 );
            else
            {
                if( h->sh.i_type != SLICE_TYPE_I )
                    x264_cabac_mb_skip( h, 0 );
                x264_macroblock_write_cabac( h, &h->cabac );
            }
        }
        else
        {
            if( IS_SKIP( h->mb.i_type ) )
                i_skip++;
            else
            {
                if( h->sh.i_type != SLICE_TYPE_I )
                {
                    bs_write_ue( &h->out.bs, i_skip );  /* skip run */
                    i_skip = 0;
                }
                x264_macroblock_write_cavlc( h, &h->out.bs );
            }
        }
        TIMER_STOP( i_mtime_write );

#if VISUALIZE
        if( h->param.b_visualize )
            x264_visualize_mb( h );
#endif

        /* save cache */
        x264_macroblock_cache_save( h );

        /* accumulate���� mb stats */
        h->stat.frame.i_mb_count[h->mb.i_type]++;
        if( !IS_SKIP(h->mb.i_type) && !IS_INTRA(h->mb.i_type) && !IS_DIRECT(h->mb.i_type) )
        {
            if( h->mb.i_partition != D_8x8 )
                h->stat.frame.i_mb_count_size[ x264_mb_partition_pixel_table[ h->mb.i_partition ] ] += 4;
            else
                for( i = 0; i < 4; i++ )
                    h->stat.frame.i_mb_count_size[ x264_mb_partition_pixel_table[ h->mb.i_sub_partition[i] ] ] ++;
            if( h->param.i_frame_reference > 1 )
            {
                for( i = 0; i < 4; i++ )
                {
                    int i_ref = h->mb.cache.ref[0][ x264_scan8[4*i] ];
                    if( i_ref >= 0 )
                        h->stat.frame.i_mb_count_ref[i_ref] ++;
                }
            }
        }
        if( h->mb.i_cbp_luma && !IS_INTRA(h->mb.i_type) )
        {
            h->stat.frame.i_mb_count_8x8dct[0] ++;
            h->stat.frame.i_mb_count_8x8dct[1] += h->mb.b_transform_8x8;
        }

        if( h->mb.b_variable_qp )
            x264_ratecontrol_mb(h, bs_pos(&h->out.bs) - mb_spos);
    }

    if( h->param.b_cabac )
    {
        /* end of slice */
        x264_cabac_encode_terminal( &h->cabac, 1 );
    }
    else if( i_skip > 0 )
    {
        bs_write_ue( &h->out.bs, i_skip );  /* last skip run */
    }

    if( h->param.b_cabac )
    {
        x264_cabac_encode_flush( &h->cabac );

    }
    else
    {
        /* rbsp_slice_trailing_bits */
        bs_rbsp_trailing( &h->out.bs );	//��β�䷨
    }

    x264_nal_end( h );

    /* Compute���� misc���ָ����ģ������ bits */
    h->stat.frame.i_misc_bits = bs_pos( &h->out.bs )
                              + NALU_OVERHEAD * 8
                              - h->stat.frame.i_itex_bits
                              - h->stat.frame.i_ptex_bits
                              - h->stat.frame.i_hdr_bits;

    return 0;
}
/*
 * ���ƣ�
 * ���ܣ�
 * ������
 * ���أ�
 * ���ϣ�
 * ˼·��
 * 
*/
static inline int x264_slices_write( x264_t *h )
{
    int i_frame_size;
	//printf("EEEEEEEEEEEEEEEEEEEEEEEEEE");
	#if VISUALIZE//����, ����, ����, ����
		//printf("VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV");
		if( h->param.b_visualize )
			x264_visualize_init( h );
	#endif

	if( h->param.i_threads == 1 )//
	{
		x264_ratecontrol_threads_start( h );
		x264_slice_write( h );
		//printf("gfgfgfgfdfdfdfdfdfdfdfsfsfsfsdfdsfG");
		i_frame_size = h->out.nal[h->out.i_nal-1].i_payload;
	}
    else
    {
        int i_nal = h->out.i_nal;
        int i_bs_size = h->out.i_bitstream / h->param.i_threads;
        int i;
		
        /* duplicate���� contexts������ */

        for( i = 0; i < h->param.i_threads; i++ )
        {
			
            x264_t *t = h->thread[i];
			//printf("GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG");
            if( i > 0 )
            {
                memcpy( t, h, sizeof(x264_t) );
                t->out.p_bitstream += i*i_bs_size;
                bs_init( &t->out.bs, t->out.p_bitstream, i_bs_size );
                t->i_thread_num = i;
            }
            t->sh.i_first_mb = (i    * h->sps->i_mb_height / h->param.i_threads) * h->sps->i_mb_width;
            t->sh.i_last_mb = ((i+1) * h->sps->i_mb_height / h->param.i_threads) * h->sps->i_mb_width;
            t->out.i_nal = i_nal + i;
        }
        x264_ratecontrol_threads_start( h );

        /* dispatch��ǲ���� */
		#ifdef HAVE_PTHREAD
        {
            pthread_t handles[X264_SLICE_MAX];
			//printf("DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
            for( i = 0; i < h->param.i_threads; i++ )
                pthread_create( &handles[i], NULL, (void*)x264_slice_write, (void*)h->thread[i] );//���������̣߳�ʵ�ʾ���CreateThread(...)
            for( i = 0; i < h->param.i_threads; i++ )
                pthread_join( handles[i], NULL );
        }
		#else
			for( i = 0; i < h->param.i_threads; i++ )//
			{
				x264_slice_write( h->thread[i] );

			}
		#endif

        /* merge�ϲ� contexts */
        i_frame_size = h->out.nal[i_nal].i_payload;
        for( i = 1; i < h->param.i_threads; i++ )
        {
            int j;
            x264_t *t = h->thread[i];
            h->out.nal[i_nal+i] = t->out.nal[i_nal+i];
            i_frame_size += t->out.nal[i_nal+i].i_payload;
            // all entries in stat.frame are ints
            for( j = 0; j < sizeof(h->stat.frame) / sizeof(int); j++ )
                ((int*)&h->stat.frame)[j] += ((int*)&t->stat.frame)[j];
        }
        h->out.i_nal = i_nal + h->param.i_threads;
    }

	#if VISUALIZE//����
		if( h->param.b_visualize )
		{
			x264_visualize_show( h );
			x264_visualize_close( h );
		}
	#endif
	//printf("GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG");
    return i_frame_size;
}

/****************************************************************************
 * x264_encoder_encode:
 ��picture���Ƴ�һ��frame
 x264_encoder_encodeÿ�λ��Բ�������һ֡�������֡pic_in���������Ȼ�ӿ��ж�����
 ȡ��һ֡���ڳ��ظ���֡��������i_frame���趨Ϊ����˳��������磺fenc->i_frame = h->frames.i_input++�� 
 x264_encoder_encode�ڸ��������о�ȷ��B֡�������������²Ž��к������빤��
 *  XXX: i_poc   : is the poc of the current given picture
 *       i_frame : is the number of the frame being coded
 *  ex:  type frame poc
 *       I      0   2*0	//poc��ʵ�ʵ�֡��λ��
 *       P      1   2*3	//frame�Ǳ����˳��
 *       B      2   2*1
 *       B      3   2*2
 *       P      4   2*6
 *       B      5   2*4
 *       B      6   2*5
 ****************************************************************************/
int     x264_encoder_encode( x264_t *h,/* ָ�������� */
                             x264_nal_t **pp_nal, /* x264_nal_t * */
							 int *pi_nal,	/* int */
                             x264_picture_t *pic_in,
                             x264_picture_t *pic_out )
{
    x264_frame_t   *frame_psnr = h->fdec;	/* just to keep the current decoded frame for psnr(��ֵ�����) calculation(Ԥ��) */
											/* ����ָ��ǰ�������ؽ���֡ frame being reconstructed(�ؽ���) */
    int     i_nal_type;
    int     i_nal_ref_idc;
    int     i_slice_type;
    int     i_frame_size;

    int i;

    int   i_global_qp;

	//char psz_message[80];//��ʾ��Ϣ����ʲô�ô�
	//printf("\n( encoder.c:x264_encoder_encode )h->out.i_nal = %d ",h->out.i_nal);//zjh��ӡ�����һֱ��1
    //printf("\n( encoder.c:x264_encoder_encode ) * pi_nal = %d ",*pi_nal);//zjh�˴���ӡ��1244268����Ȼ����ȷ����Ϊ��û�ж�����ֵ
	
	/* 
	no data out 
	���������
	*/
    *pi_nal = 0;	//int
    *pp_nal = NULL;	//x264_nal_t *

	printf("\n          x264_encoder_encode(...) in \n");//zjh
    /* -------------------��Ҫ�ǽ�ͼƬ��ԭʼ���ݸ�ֵ��һ��δʹ�õ�֡�����ڱ��� Setup new frame from picture -------------------- */
    TIMER_START( i_mtime_encode_frame );//ȡ��ʼʱ�䣬��һ��define;#define TIMER_START( d ) {int64_t d##start = x264_mdate();  #define TIMER_STOP( d ) d += x264_mdate() - d##start;}
    if( pic_in != NULL )//�����ԭʼͼ��Ϊ��
    {
        /* 1: Copy the picture to a frame and move it to a buffer ����ͼƬ��һ��֡�����ƶ�����һ��������*/
        x264_frame_t *fenc = x264_frame_get( h->frames.unused );//frames��x264_t�ṹ���ﶨ�����һ���ṹ�壬frames�ṹ������һ���ֶ���x264_frame_t *unused[X264_BFRAME_MAX+3];����X264_BFRAME_MAX=16
																//x264_frame_get�Ĳ�����x264_frame_t *list[X264_BFRAME_MAX+1]�������unused������������
        x264_frame_copy_picture( h, fenc, pic_in );//��x264_picture_t�ļ����ֶ�ֵ����x264_frame_t��������ɫ�ʿռ�����

        if( h->param.i_width % 16 || h->param.i_height % 16 ) /* ����Ϊ�棬˵����͸�������һ������16�������� */
            x264_frame_expand_border_mod16( h, fenc );//�����Ϳ���չΪ16�ı�����������Ϊ�˱�֤֡����������� ��frame.c��ʵ��

        fenc->i_frame = h->frames.i_input++;	//�������֡��

		printf("\n          h->frames.i_input = %d " , h->frames.i_input);

        x264_frame_put( h->frames.next, fenc );	//�µ�֡����ԭ֡�ĺ��棬��������list[i]

        if( h->frames.b_have_lowres )//���������Ƶ������һ��
            x264_frame_init_lowres( h->param.cpu, fenc );//��Ƶ:http://www.powercam.cc/slide/8377

        if( h->frames.i_input <= h->frames.i_delay )	/* �ӳ���֡���룬����ǰ���β�����do_encode: */
        {
            /* Nothing yet to encode ��Ȼû��ȥ���� */
            /* waiting for filling bframe buffer �ȴ����b֡������ */
            pic_out->i_type = X264_TYPE_AUTO;
			//printf("\n%d\n",h->frames.i_delay);//zjh
            return 0;//��0��1�ε���Encode_Frame()��������ͷ����ˣ���3�βŻ�ִ��do_encode:����
        }
    }

	/* ����Ѿ�׼���õĿɱ����֡[0]û�� */
    if( h->frames.current[0] == NULL )//x264_t�У�x264_frame_t *current[X264_BFRAME_MAX+3];//current���Ѿ�׼���������Ա����֡���������Ѿ�ȷ��
    {
        int bframes = 0;
		//printf("\n h->frames.current[0] == NULL \n");//zjh

        /* 
		2: Select frame types 
		���δȷ�����͵�֡Ҳû�У�ֻ���˳�
		*/
        if( h->frames.next[0] == NULL )	//x264_t�У�x264_frame_t *next[X264_BFRAME_MAX+3];//next����δȷ�����͵�֡
            return 0;	//�˳�������

		//���˴�����ζ�ţ�ȷ�������͵Ĵ������֡�����ڣ�������δȷ�����͵�֡
        x264_slicetype_decide( h );//�������ȷ����ǰ������֡�������� decide:����

        /* 3: move some B-frames and 1 non-B to encode���� queue���� 
		 * ��һЩB֡��xx?�ƽ��������
		*/
        while( IS_X264_TYPE_B( h->frames.next[bframes]->i_type ) )	//�ж������ļ�֡�Ƿ���X264_TYPE_B��X264_TYPE_BREF
            bframes++;	//֪���м���֡��X264_TYPE_B��X264_TYPE_BREF

        x264_frame_put( h->frames.current, x264_frame_get( &h->frames.next[bframes] ) );	//x264_frame_put:��������list[i]�����ں���
        /* FIXME: when max B-frames > 3, BREF may no longer be centered after GOP closing */
        if( h->param.b_bframe_pyramid && bframes > 1 )	//pyramid:������
        {
            x264_frame_t *mid = x264_frame_get( &h->frames.next[bframes/2] );//����list[0]
            mid->i_type = X264_TYPE_BREF;
            x264_frame_put( h->frames.current, mid );	////�µ�֡����ԭ֡�ĺ��棬��������list[i]
            bframes--;
        }
        while( bframes-- )
            x264_frame_put( h->frames.current, x264_frame_get( h->frames.next ) );////�µ�֡����ԭ֡�ĺ��棬��������list[i]
    }

    TIMER_STOP( i_mtime_encode_frame );

//	printf("\n......\n");//zjh

    /* ------------------- Get frame to be encoded �õ�֡ȥ����------------------------- */
    /* 4: get picture to encode 
	 * �õ�ͼ��ȥ����
	*/
    h->fenc = x264_frame_get( h->frames.current );

    if( h->fenc == NULL )
    {
        /* Nothing yet to encode (ex: waiting for I/P with B frames) */
        /* waiting for filling bframe buffer */
        pic_out->i_type = X264_TYPE_AUTO;

        return 0;
    }

do_encode:

	/*
	 * �ײ�ó����½���
	 * Encode()�����е�i_frame���0��������֡��-1
	 * h->frames.i_last_idr:��һ��IDR֡�ǵڼ����������ţ���Encode()��i_frame��2(�ӳٱ���2֡)
	 * ÿ����������IDR֡���������h->frames.i_last_idr
	 * ����ʱ��h->frames.i_last_idr = h->fenc->i_frame;
	 * h->fenc->i_frame��������˶���֡
	 * ÿ������IDR֡��h->fenc->i_poc��0
	*/

	//printf("\n%d\n",h->frames.i_delay);//zjh����ӳ�ʼ�ղ��䣬��2
	//�����IDR����ˢ��֡��Ҫ��������i_frame����Ϊ������IDR֡������ƣ�Ҫ��ʱ�����Ƿ���ٲ�һ��IDR
    if( h->fenc->i_type == X264_TYPE_IDR )//IDR
    {
        h->frames.i_last_idr = h->fenc->i_frame;//http://wmnmtm.blog.163.com/blog/static/38245714201181110467887/
		//printf("\n          h->frames.i_last_idr=%d ",h->frames.i_last_idr);//zjh//����о��Ǿ����һ��idr֡�ľ��룬�����Ǿ�����һ��idr֡  0 20 40 60 ...
    }
printf("\n          h->frames.i_last_idr=%d ",h->frames.i_last_idr);
    /* ------------------- Setup frame context ----------------------------- */
    /* 5: Init data dependant of frame type 
	 * ��ʼ������ȡ����֡����
	 * dependant:ȡ����
	*/
    TIMER_START( i_mtime_encode_frame );

	//printf("h->fenc->i_frame=%d    ",h->fenc->i_frame);//zjh
	
	/*
	 * h->fenc:��������ĵ�ǰ֡(x264_frame_t *fenc)
	 * �����ǰ�����֡��IDR֡����ô������һЩ���ƣ�(��elseif��䶼����)
	 * ��ǰ��Ƭ����ֻ����IƬ
	 * NAL������ֻ����NAL_SLICE_IDR
	 * NAL�����ȼ�ֻ�������
	 * --keyint 5 ����ÿ5֡���һ��IDRͼ�񣬾����ӦΪ h->fenc->i_frame=0    h->fenc->i_type == X264_TYPE_IDR //h->fenc->i_frame=1    h->fenc->i_type == X264_TYPE_P //h->fenc->i_frame=5    h->fenc->i_type == X264_TYPE_IDR
	*/
    if( h->fenc->i_type == X264_TYPE_IDR ) //fenc : ��������ĵ�ǰ֡(x264_frame_t  *fenc) //IDR
    {
        /* reset ref pictures 
		 * ����ο�ͼ����ΪIDRͼ��֮���ͼ�񣬲��ܲο���IDRǰ���κ�ͼ�񣬷�ֹ������ɢ
		 * 
		*/
        x264_reference_reset( h );	//����ο�ͼ��ע�������x264_tָ�룬������ȫ���Ĳο�֡

		//��Ϊ��IDRͼ����������������Щ
        i_nal_type    = NAL_SLICE_IDR;			//NAL�����ͣ���ΪIDRͼ��
        i_nal_ref_idc = NAL_PRIORITY_HIGHEST;	//NAL�����ȼ����˴���Ϊ���
        i_slice_type = SLICE_TYPE_I;			//Ƭ���ͣ���ΪIƬ
		//printf("h->fenc->i_type == X264_TYPE_IDR\n");//zjh
    }
	//֡�ڱ���֡
    else if( h->fenc->i_type == X264_TYPE_I )
    {
        i_nal_type    = NAL_SLICE;				//NAL_SLICE   = 1 //����������IDRͼ���Ƭ
        i_nal_ref_idc = NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        i_slice_type = SLICE_TYPE_I;			//Ƭ���ͣ���ΪIƬ
		//printf("h->fenc->i_type == X264_TYPE_I\n");//zjh
    }
	//P֡
    else if( h->fenc->i_type == X264_TYPE_P )
    {
        i_nal_type    = NAL_SLICE;
        i_nal_ref_idc = NAL_PRIORITY_HIGH;		/* Not completely true but for now it is (as all I/P are kept as ref)*/
        i_slice_type = SLICE_TYPE_P;
		//printf("h->fenc->i_type == X264_TYPE_P\n");//zjh
    }
	//�����ο���B֡
    else if( h->fenc->i_type == X264_TYPE_BREF )/* Non-disposable(һ����) B-frame(B-֡) ???*/
    {
        i_nal_type    = NAL_SLICE;
        i_nal_ref_idc = NAL_PRIORITY_HIGH;		/* maybe add MMCO to forget it? -> low */
        i_slice_type = SLICE_TYPE_B;			//BƬ
		//printf("h->fenc->i_type == X264_TYPE_BREF\n");//zjh
    }
	//B֡
    else    /* B frame */
    {
        i_nal_type    = NAL_SLICE;				//NAL_SLICE   = 1 //����������IDRͼ���Ƭ
        i_nal_ref_idc = NAL_PRIORITY_DISPOSABLE;//NAL_���ȼ�_�ɶ�����
        i_slice_type = SLICE_TYPE_B;			//BƬ
		//printf("else\n");//zjh
    }

	/*
	 *
	 * POC��ʶͼ��Ĳ���˳��
	*/
    h->fdec->i_poc =
    h->fenc->i_poc = 2 * (h->fenc->i_frame - h->frames.i_last_idr);	/* x264_frame_t    *fdec; ���ڱ��ؽ���֡ frame being reconstructed(�ؽ���) */

	printf("\n          h->fenc->i_poc = %d ;h->fenc->i_frame = %d ;h->fenc->i_frame_num = %d \n \n ",h->fenc->i_poc,h->fenc->i_frame,h->fenc->i_frame_num);
	//zjh//http://wmnmtm.blog.163.com/blog/static/38245714201181111372460/

//system("pause");//��ͣ�����������
	/* ���ڱ��ؽ���֡ ... = ��������ĵ�ǰ֡... */
    h->fdec->i_type = h->fenc->i_type;
    h->fdec->i_frame = h->fenc->i_frame;
    h->fenc->b_kept_as_ref =
    h->fdec->b_kept_as_ref = i_nal_ref_idc != NAL_PRIORITY_DISPOSABLE;

    /* ------------------- Init��ʼ��----------------------------- */
    /* �����ο��б�list0��list1   build ref list 0/1 */
    x264_reference_build_list( h, h->fdec->i_poc, i_slice_type );//����ÿһ֡ʱ�����Ὠ���ο��б�����ʱ����������������һ����ָ�����������ڶ����ǲ���˳�򣬵�������Ƭ����

    /* Init the rate control */
    x264_ratecontrol_start( h, i_slice_type, h->fenc->i_qpplus1 );
    i_global_qp = x264_ratecontrol_qp( h );

    pic_out->i_qpplus1 =
    h->fdec->i_qpplus1 = i_global_qp + 1;

    if( i_slice_type == SLICE_TYPE_B )
        x264_macroblock_bipred_init( h );

    /* ------------------------ Create slice header  ----------------------- */
    x264_slice_init( h, i_nal_type, i_slice_type, i_global_qp );

    if( h->fenc->b_kept_as_ref )
        h->i_frame_num++;

    /* ---------------------- Write the bitstream д������� -------------------------- */
    /* ��ʼ�������������� */
    h->out.i_nal = 0;
	//��ʼ��������Ŀռ�(�Ӵ˴�������ÿ�ν��뱾�������룬�������¿�ʼ)
    bs_init( &h->out.bs, h->out.p_bitstream, h->out.i_bitstream );
											//p_bitstream�Ƕ�̬�����ڴ�ʱ�������ĳ�ʼ��ʼ��ַ
											//����ڴ��Ǹ���h->out.i_bitstream�����
											//i_bitstream��1000000���߸���(1000000�����û��Զ�������)

    /*
	 * �ֽ�����Ϻ�ܣ�191/326 nal_unit_type = 9 �ֽ��
	 * x264Ĭ���ǲ����ɷֽ����
	 * ���ȼ���NAL_PRIORITY_DISPOSABLE = 0
	 * �������ļ�����0x 09
	 * 0x 09 10 �����е�10����β�䷨bs_rbsp_trailing(&h->out.bs);���ɵ�
	*/
	//h->param.b_aud=0;//��Ϊ1��������һ��0x 00 00 00 01 09 10 //http://wmnmtm.blog.163.com/blog/static/38245714201192523012109/
	if(h->param.b_aud)//���ɷ��ʵ�Ԫ�ָ���,Ĭ��ֵΪ0����������
	{
        int pic_type;

        if(i_slice_type == SLICE_TYPE_I)		//IƬ
		{
            pic_type = 0;
			printf("***8888888888888888888888888888 pic_type = %d",pic_type);
		}
        else if(i_slice_type == SLICE_TYPE_P)	//PƬ
		{
            pic_type = 1;
			printf("***8888888888888888888888888888 pic_type = %d",pic_type);
		}
        else if(i_slice_type == SLICE_TYPE_B)	//BƬ
            pic_type = 2;
        else
            pic_type = 7;

        x264_nal_start(h, NAL_AUD/* 9 */, NAL_PRIORITY_DISPOSABLE);//DISPOSABLE:�ú�����ģ�һ��������
        bs_write(&h->out.bs, 3, pic_type);//:::::::pic_type
			/*
			 * ������Ϊ����֮һ��
			 * bs_write(&h->out.bs, 3, 0);//�����ô��д��0x 09 10��,�������
			 * bs_write(&h->out.bs, 3, 1);
			 * bs_write(&h->out.bs, 3, 2);
			 * bs_write(&h->out.bs, 3, 7);
			*/
		//printf(":::::::::::::::::::::::");system("pause");//��ͣ�����������
        bs_rbsp_trailing(&h->out.bs);
        x264_nal_end(h);
    }

    h->i_nal_type = i_nal_type;			//ָ����ǰNAL_unit������
    h->i_nal_ref_idc = i_nal_ref_idc;	//ָʾ��ǰNAL�����ȼ�

    /* Write SPS and PPS 
	 * д���в�������ͼ��������Լ�SEI�汾��Ϣ��������д���ļ�������д����������� 
	 * ���ͨ��p_write_nalu�е�fwriteд�뵽�ļ�
	*/
    if( i_nal_type == NAL_SLICE_IDR && h->param.b_repeat_headers )//ֻ���ض��������²����SPS��PPS
    {
		printf("         encoder.c : Write SPS and PPS");
		system("pause");//��ͣ�����������

        if( h->fenc->i_frame == 0 )//
        {
            /* identify ourself */
            x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
            //x264_sei_version_write( h, &h->out.bs );//ע�͵�������ffplay���ţ�������ǰ�Ȩ����
            x264_nal_end( h );
			
        }

        /* generate sequence parameters 
		 * ���в���SPSд����
		*/
        x264_nal_start( h, NAL_SPS, NAL_PRIORITY_HIGHEST );
		x264_sps_write( &h->out.bs, h->sps );//ע�͵����Ͳ��ܲ���
        x264_nal_end( h );
		/*
		 * �����д���ļ�����һ�±����sps��ʲô����
		*/
		if (1)
		{
		//FILE *f11 = fopen( "c:\\sps.sps", "wb+" );
		//fwrite(data, 1024, 1, f11);
		}

        /* generate picture parameters 
		 * ͼ�������PPSд����
		*/
        x264_nal_start( h, NAL_PPS, NAL_PRIORITY_HIGHEST );
        x264_pps_write( &h->out.bs, h->pps );//ע�͵����Ͳ��ܲ���
        x264_nal_end( h );
    }


    /* Write frame д��֡��������д���ļ�������д�����������*/
    i_frame_size = x264_slices_write( h );

    /* �ָ�CPU״̬ restore CPU state (before using float again) */
    x264_cpu_restore( h->param.cpu );

    if( i_slice_type == SLICE_TYPE_P && !h->param.rc.b_stat_read 
        && h->param.i_scenecut_threshold >= 0 )
    {
        const int *mbs = h->stat.frame.i_mb_count;
        int i_mb_i = mbs[I_16x16] + mbs[I_8x8] + mbs[I_4x4];
        int i_mb_p = mbs[P_L0] + mbs[P_8x8];
        int i_mb_s = mbs[P_SKIP];
        int i_mb   = h->sps->i_mb_width * h->sps->i_mb_height;
        int64_t i_inter_cost = h->stat.frame.i_inter_cost;
        int64_t i_intra_cost = h->stat.frame.i_intra_cost;

        float f_bias;
        int i_gop_size = h->fenc->i_frame - h->frames.i_last_idr;
        float f_thresh_max = h->param.i_scenecut_threshold / 100.0;
        /* magic numbers pulled out of thin air */
        float f_thresh_min = f_thresh_max * h->param.i_keyint_min
                             / ( h->param.i_keyint_max * 4 );
        if( h->param.i_keyint_min == h->param.i_keyint_max )
             f_thresh_min= f_thresh_max;

        /* macroblock_analyse() doesn't further��һ���� analyse���� skipped mbs,
         * so we have to guess���� their cost���� */
        if( i_mb_s < i_mb )
            i_intra_cost = i_intra_cost * i_mb / (i_mb - i_mb_s);

        if( i_gop_size < h->param.i_keyint_min / 4 )
            f_bias = f_thresh_min / 4;
        else if( i_gop_size <= h->param.i_keyint_min )
            f_bias = f_thresh_min * i_gop_size / h->param.i_keyint_min;
        else
        {
            f_bias = f_thresh_min
                     + ( f_thresh_max - f_thresh_min )
                       * ( i_gop_size - h->param.i_keyint_min )
                       / ( h->param.i_keyint_max - h->param.i_keyint_min );
        }
        f_bias = X264_MIN( f_bias, 1.0 );

        /* ����P֡������ΪI֡���±��� Bad P will be reencoded���±��� as I */
        if( i_mb_s < i_mb &&
            i_inter_cost >= (1.0 - f_bias) * i_intra_cost )
        {
            int b;

            x264_log( h, X264_LOG_DEBUG, "scene cut at %d Icost:%.0f Pcost:%.0f ratio:%.3f bias=%.3f lastIDR:%d (I:%d P:%d S:%d)\n",
                      h->fenc->i_frame,
                      (double)i_intra_cost, (double)i_inter_cost,
                      (double)i_inter_cost / i_intra_cost,
                      f_bias, i_gop_size,
                      i_mb_i, i_mb_p, i_mb_s );
				//printf("\n");//zjh
				//printf("h->i_frame_num%d",h->i_frame_num);//zjh
				//printf("\n");//zjh

            /* Restore frame num */
            h->i_frame_num--;
				//printf("\n");//zjh
				//printf("h->i_frame_num--");//zjh
				//printf("\n");//zjh
				//printf("\n");//zjh
				//printf("h->i_frame_num%d",h->i_frame_num);//zjh
				//printf("\n");//zjh

            for( b = 0; h->frames.current[b] && IS_X264_TYPE_B( h->frames.current[b]->i_type ); b++ );
            if( b > 0 )
            {
                /* If using B-frames, force GOP to be closed.
                 * Even if this frame is going to be I and not IDR, forcing a
                 * P-frame before the scenecut will probably help compression.
                 * 
                 * We don't yet know exactly which frame is the scene cut, so
                 * we can't assign an I-frame. Instead, change the previous
                 * B-frame to P, and rearrange coding order. */

                if( h->param.b_bframe_adaptive || b > 1 )
                    h->fenc->i_type = X264_TYPE_AUTO;
                x264_frame_sort_pts( h->frames.current );
                x264_frame_push( h->frames.next, h->fenc );
                h->fenc = h->frames.current[b-1];
                h->frames.current[b-1] = NULL;
                h->fenc->i_type = X264_TYPE_P;
                x264_frame_sort_dts( h->frames.current );
            }
            /* �����Ҫ��ִ��IDR, Do IDR if needed */
            else if( i_gop_size >= h->param.i_keyint_min )	/* ��ѡ��˵��֪������������СIDR���ʱ���ﵽ������ֵʱ�ͻ����IDR */
            {
                x264_frame_t *tmp;

				printf("\n");//zjh
				printf("i_gop_size >= h->param.i_keyint_min");//zjh
				printf("\n");//zjh

                /* Reset���� */
                h->i_frame_num = 0;

                /* Reinit�س�ʼ�� field�ֶ� of fenc */
                h->fenc->i_type = X264_TYPE_IDR;
                h->fenc->i_poc = 0;

                /* Put enqueued���� frames back in the poolˮ�� */
                while( (tmp = x264_frame_get( h->frames.current ) ) != NULL )
                    x264_frame_put( h->frames.next, tmp );////�µ�֡����ԭ֡�ĺ��棬��������list[i]
                x264_frame_sort_pts( h->frames.next );
            }
            else
            {
                h->fenc->i_type = X264_TYPE_I;
				printf("\n");//zjh
				printf("i_gop_size >= h->param.i_keyint_min else");//zjh
				printf("\n");//zjh
            }
            goto do_encode;//�����������Ƕ�
        }
    }

    /* 
	End bitstream, set output  
	�������������������������x264.c��Encode_Frame�ͻ�ȡ����
	*/
    *pi_nal = h->out.i_nal;//�������������һ�γ��ִ˱���
    *pp_nal = h->out.nal;//�������������һ�γ��ִ˱���

    /* �������ͼ������ Set output picture properties���� */
    if( i_slice_type == SLICE_TYPE_I )
        pic_out->i_type = i_nal_type == NAL_SLICE_IDR ? X264_TYPE_IDR : X264_TYPE_I;//����x264_picture_t�ĵ�1���ֶ�
    else if( i_slice_type == SLICE_TYPE_P )
        pic_out->i_type = X264_TYPE_P;//x264_picture_t.i_type
    else
        pic_out->i_type = X264_TYPE_B;//x264_picture_t.i_type
    pic_out->i_pts = h->fenc->i_pts;//��������ĵ�ǰ֡��ʱ���
	//printf("\n��������ĵ�ǰ֡��ʱ���%d",h->fenc->i_pts);//zjh  ��ӡ����0��1��2��3��4....,298��299

    pic_out->img.i_plane = h->fdec->i_plane;//x264_picture_t.img.i_plane ɫ�ʿռ�ĸ���,
	//printf("\npic_out->img.i_plane = h->fdec->i_plane=%d",h->fdec->i_plane);//zjh ��ӡ����3

    for(i = 0; i < 4; i++){
        pic_out->img.i_stride[i] = h->fdec->i_stride[i];//�����ؽ���֡��YUV buffer����
		//i_stride �ǿ�ȣ���Ϊ��ʱ��Ҫ��ͼ����б߽���չ����Ⱦ�����չ��Ŀ��http://bbs.chinavideo.org/viewthread.php?tid=12772&extra=page%3D3
		//printf("\n pic_out->img.i_stride[%d] = h->fdec->i_stride[%d] = %d",i,i,h->fdec->i_stride[i]);//zjh  ��ӡ����
		/*��ӡ����
		pic_out->img.i_stride[0] = h->fdec->i_stride[0] = 416
		pic_out->img.i_stride[1] = h->fdec->i_stride[1] = 208
		pic_out->img.i_stride[2] = h->fdec->i_stride[2] = 208
		pic_out->img.i_stride[3] = h->fdec->i_stride[3] = 0		
		*/
        
		
		pic_out->img.plane[i] = h->fdec->plane[i];	/* ������һ��YUV����3�� */
		//printf("pic_out->img.plane[i] = h->fdec->plane[i] = %s ",h->fdec->plane[i]);//������ӡ���������������ַ����к�������ĸhttp://wmnmtm.blog.163.com/blog/static/382457142011816115130313/
    }

    /* ���±�����״̬---------------------- Update encoder state״̬, ״�� ------------------------- */

    /* update rc */
    x264_cpu_restore( h->param.cpu );
    x264_ratecontrol_end( h, i_frame_size * 8 );//������һ֡�󣬱���״̬���Ҹ������ʿ�����״̬//X264���ʿ������̷���http://wmnmtm.blog.163.com/blog/static/3824571420118170113300/
	
    /* handle references */
    if( i_nal_ref_idc != NAL_PRIORITY_DISPOSABLE )
        x264_reference_update( h );
	#ifdef DEBUG_DUMP_FRAME
    else
        x264_fdec_deblock( h );
	#endif
    x264_frame_put( h->frames.unused, h->fenc );////�µ�֡����ԭ֡�ĺ��棬��������list[i]

    /* increase���� frame count */
    h->i_frame++;

    /* �ָ�CPU״̬restore CPU state (before using float again) */
    x264_cpu_restore( h->param.cpu );//�ָ�CPU״̬

    x264_noise_reduction_update( h );//�������

    TIMER_STOP( i_mtime_encode_frame );

	//��ԭʼ֡���ؽ�֡ѡ���Ե�������Ա�Ƚϵ���
	//if (h->i_frame==20)//��һ֡4==*pi_nal
	//{
	//	x264_frame_dump( h, h->fdec, "d:\\fdec.yuv" );
	//	x264_frame_dump( h, h->fenc, "d:\\fenc.yuv" );
	//	printf("\n(encoder.c \ x264_encoder_encode()),i_frame=%d",h->i_frame);
	//	printf("\n\n");//zjh
	//	system("pause");//��ͣ�����������
	//}



    /* ����ʹ�ӡͳ������---------------------- Compute/Print statistics ͳ������--------------------- */
    /* Slice stat */
	/*
    h->stat.i_slice_count[i_slice_type]++;
    h->stat.i_slice_size[i_slice_type] += i_frame_size + NALU_OVERHEAD;
    h->stat.i_slice_qp[i_slice_type] += i_global_qp;

    for( i = 0; i < 19; i++ )
        h->stat.i_mb_count[h->sh.i_type][i] += h->stat.frame.i_mb_count[i];
    for( i = 0; i < 2; i++ )
        h->stat.i_mb_count_8x8dct[i] += h->stat.frame.i_mb_count_8x8dct[i];
    if( h->sh.i_type != SLICE_TYPE_I )
    {
        for( i = 0; i < 7; i++ )
            h->stat.i_mb_count_size[h->sh.i_type][i] += h->stat.frame.i_mb_count_size[i];
        for( i = 0; i < 16; i++ )
            h->stat.i_mb_count_ref[h->sh.i_type][i] += h->stat.frame.i_mb_count_ref[i];
    }
    if( i_slice_type == SLICE_TYPE_B )
    {
        h->stat.i_direct_frames[ h->sh.b_direct_spatial_mv_pred ] ++;
        if( h->mb.b_direct_auto_write )
        {
            //FIXME somewhat arbitrary time constants
            if( h->stat.i_direct_score[0] + h->stat.i_direct_score[1] > h->mb.i_mb_count )
            {
                for( i = 0; i < 2; i++ )
                    h->stat.i_direct_score[i] = h->stat.i_direct_score[i] * 9/10;
            }
            for( i = 0; i < 2; i++ )
                h->stat.i_direct_score[i] += h->stat.frame.i_direct_score[i];
        }
    }
	*/
	/*
    if( h->param.analyse.b_psnr )
    {
        int64_t i_sqe_y, i_sqe_u, i_sqe_v;

        // PSNR(��ֵ�����) 
        i_sqe_y = x264_pixel_ssd_wxh( &h->pixf, frame_psnr->plane[0], frame_psnr->i_stride[0], h->fenc->plane[0], h->fenc->i_stride[0], h->param.i_width, h->param.i_height );
        i_sqe_u = x264_pixel_ssd_wxh( &h->pixf, frame_psnr->plane[1], frame_psnr->i_stride[1], h->fenc->plane[1], h->fenc->i_stride[1], h->param.i_width/2, h->param.i_height/2);
        i_sqe_v = x264_pixel_ssd_wxh( &h->pixf, frame_psnr->plane[2], frame_psnr->i_stride[2], h->fenc->plane[2], h->fenc->i_stride[2], h->param.i_width/2, h->param.i_height/2);
        x264_cpu_restore( h->param.cpu );

        h->stat.i_sqe_global[i_slice_type] += i_sqe_y + i_sqe_u + i_sqe_v;
        h->stat.f_psnr_average[i_slice_type] += x264_psnr( i_sqe_y + i_sqe_u + i_sqe_v, 3 * h->param.i_width * h->param.i_height / 2 );
        h->stat.f_psnr_mean_y[i_slice_type] += x264_psnr( i_sqe_y, h->param.i_width * h->param.i_height );
        h->stat.f_psnr_mean_u[i_slice_type] += x264_psnr( i_sqe_u, h->param.i_width * h->param.i_height / 4 );
        h->stat.f_psnr_mean_v[i_slice_type] += x264_psnr( i_sqe_v, h->param.i_width * h->param.i_height / 4 );

        snprintf( psz_message, 80, " PSNR Y:%2.2f U:%2.2f V:%2.2f",
                  x264_psnr( i_sqe_y, h->param.i_width * h->param.i_height ),
                  x264_psnr( i_sqe_u, h->param.i_width * h->param.i_height / 4),
                  x264_psnr( i_sqe_v, h->param.i_width * h->param.i_height / 4) );
        psz_message[79] = '\0';
    }
    else
    {
        psz_message[0] = '\0';
    }
    
    x264_log( h, X264_LOG_DEBUG,
                  "frame=%4d QP=%i NAL=%d Slice:%c Poc:%-3d I:%-4d P:%-4d SKIP:%-4d size=%d bytes%s\n",
              h->i_frame - 1,
              i_global_qp,
              i_nal_ref_idc,
              i_slice_type == SLICE_TYPE_I ? 'I' : (i_slice_type == SLICE_TYPE_P ? 'P' : 'B' ),
              frame_psnr->i_poc,
              h->stat.frame.i_mb_count_i,
              h->stat.frame.i_mb_count_p,
              h->stat.frame.i_mb_count_skip,
              i_frame_size,
              psz_message );
	*/

#ifdef DEBUG_MB_TYPE
{
    static const char mb_chars[] = { 'i', 'i', 'I', 'C', 'P', '8', 'S',
        'D', '<', 'X', 'B', 'X', '>', 'B', 'B', 'B', 'B', '8', 'S' };
    int mb_xy;
    for( mb_xy = 0; mb_xy < h->sps->i_mb_width * h->sps->i_mb_height; mb_xy++ )//�ж��ٸ�����ѭ�����ٴ�
	/*            ;       < ����ʾ�Ŀ�x����ʾ�ĸߣ�ʵ�ʾ��Ǻ������      */
    {
        if( h->mb.type[mb_xy] < 19 && h->mb.type[mb_xy] >= 0 )
            fprintf( stderr, "%c ", mb_chars[ h->mb.type[mb_xy] ] );
        else
            fprintf( stderr, "? " );

        if( (mb_xy+1) % h->sps->i_mb_width == 0 )
            fprintf( stderr, "\n" );
    }
}
#endif

	#ifdef DEBUG_DUMP_FRAME	//���������Ϊ�˵��Խ��������
    /* Dump���� reconstructed�ؽ��� frame */
    x264_frame_dump( h, frame_psnr, "fdec.yuv" );printf("\n( encoder.c:x264_encoder_encode ) * pi_nal = %d ",*pi_nal);//zjh
	#endif
	/*
	if (4==*pi_nal)//��һ֡
	{
	x264_frame_dump( h, h->fdec, "d:\\fdec.yuv" );//д��������yuv420�ļ���������yuvviewer�鿴//http://wmnmtm.blog.163.com/blog/static/3824571420118188540701/
	x264_frame_dump( h, h->fenc, "d:\\fenc.yuv" );//д��������yuv420�ļ���������yuvviewer�鿴//http://wmnmtm.blog.163.com/blog/static/3824571420118188540701/
	}
	*/
    return 0;
}

/****************************************************************************
 * x264_encoder_close:
 ****************************************************************************/
void    x264_encoder_close  ( x264_t *h )
{
#ifdef DEBUG_BENCHMARK
    int64_t i_mtime_total = i_mtime_analyse + i_mtime_encode + i_mtime_write + i_mtime_filter + 1;
#endif
    int64_t i_yuv_size = 3 * h->param.i_width * h->param.i_height / 2;	//��x��x1.5
    int i;

#ifdef DEBUG_BENCHMARK	//BENCHMARK:��׼,��׼
	//��־
    x264_log( h, X264_LOG_INFO,
              "analyse=%d(%lldms) encode=%d(%lldms) write=%d(%lldms) filter=%d(%lldms)\n",
              (int)(100*i_mtime_analyse/i_mtime_total), i_mtime_analyse/1000,
              (int)(100*i_mtime_encode/i_mtime_total), i_mtime_encode/1000,
              (int)(100*i_mtime_write/i_mtime_total), i_mtime_write/1000,
              (int)(100*i_mtime_filter/i_mtime_total), i_mtime_filter/1000 );
#endif

    /* Ƭʹ�õģ��ͷ�ֵ����� Slices used and PSNR(��ֵ�����) */
    for( i=0; i<5; i++ )
    {
        static const int slice_order[] = { SLICE_TYPE_I, SLICE_TYPE_SI, SLICE_TYPE_P, SLICE_TYPE_SP, SLICE_TYPE_B };
        static const char *slice_name[] = { "P", "B", "I", "SP", "SI" };
        int i_slice = slice_order[i];

        if( h->stat.i_slice_count[i_slice] > 0 )
        {
            const int i_count = h->stat.i_slice_count[i_slice];
            if( h->param.analyse.b_psnr )
            {
				//��־
                x264_log( h, X264_LOG_INFO,
                          "slice %s:%-5d Avg QP:%5.2f  size:%6.0f  PSNR Mean Y:%5.2f U:%5.2f V:%5.2f Avg:%5.2f Global:%5.2f\n",
                          slice_name[i_slice],
                          i_count,
                          (double)h->stat.i_slice_qp[i_slice] / i_count,
                          (double)h->stat.i_slice_size[i_slice] / i_count,
                          h->stat.f_psnr_mean_y[i_slice] / i_count, h->stat.f_psnr_mean_u[i_slice] / i_count, h->stat.f_psnr_mean_v[i_slice] / i_count,
                          h->stat.f_psnr_average[i_slice] / i_count,
                          x264_psnr( h->stat.i_sqe_global[i_slice], i_count * i_yuv_size ) );
            }
            else
            {
				//��־
                x264_log( h, X264_LOG_INFO,
                          "slice %s:%-5d Avg QP:%5.2f  size:%6.0f\n",
                          slice_name[i_slice],
                          i_count,
                          (double)h->stat.i_slice_qp[i_slice] / i_count,
                          (double)h->stat.i_slice_size[i_slice] / i_count );
            }
        }
    }

    /* �������ʹ�õ� MB types used */
    if( h->stat.i_slice_count[SLICE_TYPE_I] > 0 )
    {
        const int64_t *i_mb_count = h->stat.i_mb_count[SLICE_TYPE_I];
        const double i_count = h->stat.i_slice_count[SLICE_TYPE_I] * h->mb.i_mb_count / 100.0;
        x264_log( h, X264_LOG_INFO,
                  "mb I  I16..4: %4.1f%% %4.1f%% %4.1f%%\n",
                  i_mb_count[I_16x16]/ i_count,
                  i_mb_count[I_8x8]  / i_count,
                  i_mb_count[I_4x4]  / i_count );
    }
    if( h->stat.i_slice_count[SLICE_TYPE_P] > 0 )
    {
        const int64_t *i_mb_count = h->stat.i_mb_count[SLICE_TYPE_P];
        const int64_t *i_mb_size = h->stat.i_mb_count_size[SLICE_TYPE_P];
        const double i_count = h->stat.i_slice_count[SLICE_TYPE_P] * h->mb.i_mb_count / 100.0;
        x264_log( h, X264_LOG_INFO,
                  "mb P  I16..4: %4.1f%% %4.1f%% %4.1f%%  P16..4: %4.1f%% %4.1f%% %4.1f%% %4.1f%% %4.1f%%    skip:%4.1f%%\n",
                  i_mb_count[I_16x16]/ i_count,
                  i_mb_count[I_8x8]  / i_count,
                  i_mb_count[I_4x4]  / i_count,
                  i_mb_size[PIXEL_16x16] / (i_count*4),
                  (i_mb_size[PIXEL_16x8] + i_mb_size[PIXEL_8x16]) / (i_count*4),
                  i_mb_size[PIXEL_8x8] / (i_count*4),
                  (i_mb_size[PIXEL_8x4] + i_mb_size[PIXEL_4x8]) / (i_count*4),
                  i_mb_size[PIXEL_4x4] / (i_count*4),
                  i_mb_count[P_SKIP] / i_count );
    }
    if( h->stat.i_slice_count[SLICE_TYPE_B] > 0 )
    {
        const int64_t *i_mb_count = h->stat.i_mb_count[SLICE_TYPE_B];
        const int64_t *i_mb_size = h->stat.i_mb_count_size[SLICE_TYPE_B];
        const double i_count = h->stat.i_slice_count[SLICE_TYPE_B] * h->mb.i_mb_count / 100.0;
        x264_log( h, X264_LOG_INFO,
                  "mb B  I16..4: %4.1f%% %4.1f%% %4.1f%%  B16..8: %4.1f%% %4.1f%% %4.1f%%  direct:%4.1f%%  skip:%4.1f%%\n",
                  i_mb_count[I_16x16]  / i_count,
                  i_mb_count[I_8x8]    / i_count,
                  i_mb_count[I_4x4]    / i_count,
                  i_mb_size[PIXEL_16x16] / (i_count*4),
                  (i_mb_size[PIXEL_16x8] + i_mb_size[PIXEL_8x16]) / (i_count*4),
                  i_mb_size[PIXEL_8x8] / (i_count*4),
                  i_mb_count[B_DIRECT] / i_count,
                  i_mb_count[B_SKIP]   / i_count );
    }

    x264_ratecontrol_summary( h );	/* summary:���̵�, ������ */

    if( h->stat.i_slice_count[SLICE_TYPE_I] + h->stat.i_slice_count[SLICE_TYPE_P] + h->stat.i_slice_count[SLICE_TYPE_B] > 0 )
    {
        const int i_count = h->stat.i_slice_count[SLICE_TYPE_I] +
                            h->stat.i_slice_count[SLICE_TYPE_P] +
                            h->stat.i_slice_count[SLICE_TYPE_B];
        float fps = (float) h->param.i_fps_num / h->param.i_fps_den;
#define SUM3(p) (p[SLICE_TYPE_I] + p[SLICE_TYPE_P] + p[SLICE_TYPE_B])
#define SUM3b(p,o) (p[SLICE_TYPE_I][o] + p[SLICE_TYPE_P][o] + p[SLICE_TYPE_B][o])
        float f_bitrate = fps * SUM3(h->stat.i_slice_size) / i_count / 125;

        if( h->param.analyse.b_transform_8x8 )
        {
            int64_t i_i8x8 = SUM3b( h->stat.i_mb_count, I_8x8 );
            int64_t i_intra = i_i8x8 + SUM3b( h->stat.i_mb_count, I_4x4 )
                                     + SUM3b( h->stat.i_mb_count, I_16x16 );
            x264_log( h, X264_LOG_INFO, "8x8 transform  intra:%.1f%%  inter:%.1f%%\n",
                      100. * i_i8x8 / i_intra,
                      100. * h->stat.i_mb_count_8x8dct[1] / h->stat.i_mb_count_8x8dct[0] );
        }

        if( h->param.analyse.i_direct_mv_pred == X264_DIRECT_PRED_AUTO
            && h->stat.i_slice_count[SLICE_TYPE_B] )
        {
            x264_log( h, X264_LOG_INFO, "direct mvs  spatial:%.1f%%  temporal:%.1f%%\n",
                      h->stat.i_direct_frames[1] * 100. / h->stat.i_slice_count[SLICE_TYPE_B],
                      h->stat.i_direct_frames[0] * 100. / h->stat.i_slice_count[SLICE_TYPE_B] );
        }

        if( h->param.i_frame_reference > 1 )
        {
            int i_slice;
            for( i_slice = 0; i_slice < 2; i_slice++ )
            {
                char buf[200];
                char *p = buf;
                int64_t i_den = 0;
                int i_max = 0;
                for( i = 0; i < h->param.i_frame_reference; i++ )
                    if( h->stat.i_mb_count_ref[i_slice][i] )
                    {
                        i_den += h->stat.i_mb_count_ref[i_slice][i];
                        i_max = i;
                    }
                if( i_max == 0 )
                    continue;
                for( i = 0; i <= i_max; i++ )
                    p += sprintf( p, " %4.1f%%", 100. * h->stat.i_mb_count_ref[i_slice][i] / i_den );
                x264_log( h, X264_LOG_INFO, "ref %c %s\n", i_slice==SLICE_TYPE_P ? 'P' : 'B', buf );
            }
        }

        if( h->param.analyse.b_psnr )
            x264_log( h, X264_LOG_INFO,
                      "PSNR Mean Y:%6.3f U:%6.3f V:%6.3f Avg:%6.3f Global:%6.3f kb/s:%.2f\n",
                      SUM3( h->stat.f_psnr_mean_y ) / i_count,
                      SUM3( h->stat.f_psnr_mean_u ) / i_count,
                      SUM3( h->stat.f_psnr_mean_v ) / i_count,
                      SUM3( h->stat.f_psnr_average ) / i_count,
                      x264_psnr( SUM3( h->stat.i_sqe_global ), i_count * i_yuv_size ),
                      f_bitrate );
        else
            x264_log( h, X264_LOG_INFO, "kb/s:%.1f\n", f_bitrate );
    }

    /* frames */
    for( i = 0; i < X264_BFRAME_MAX + 3; i++ )
    {
        if( h->frames.current[i] ) x264_frame_delete( h->frames.current[i] );
        if( h->frames.next[i] )    x264_frame_delete( h->frames.next[i] );
        if( h->frames.unused[i] )  x264_frame_delete( h->frames.unused[i] );
    }
    /* ref frames */
    for( i = 0; i < h->frames.i_max_dpb; i++ )
    {
        x264_frame_delete( h->frames.reference[i] );
    }

    /* rc */
    x264_ratecontrol_delete( h );

    /* param */
    if( h->param.rc.psz_stat_out )
        free( h->param.rc.psz_stat_out );
    if( h->param.rc.psz_stat_in )
        free( h->param.rc.psz_stat_in );
    if( h->param.rc.psz_rc_eq )
        free( h->param.rc.psz_rc_eq );

    x264_cqm_delete( h );
    x264_macroblock_cache_end( h );
    x264_free( h->out.p_bitstream );
    for( i = 1; i < h->param.i_threads; i++ )
        x264_free( h->thread[i] );
    x264_free( h );
}

/*
����ԭ�ͣ�FILE * fopen(const char * path,const char * mode);
��غ�����open��fclose��fopen_s[1] ��_wfopen ����
����⣺ <stdio.h>
����ֵ�� �ļ�˳���򿪺�ָ��������ļ�ָ��ͻᱻ���ء�����ļ���ʧ���򷵻�NULL�����Ѵ���������errno �С�
 ����һ����ԣ����ļ������һЩ�ļ���ȡ��д��Ķ����������ļ�ʧ�ܣ��������Ķ�д����Ҳ�޷�˳�����У�����һ����fopen()���������жϼ����� ����
����˵���� ����
����path�ַ����������򿪵��ļ�·�����ļ���������mode�ַ��������������̬�� ����
mode�����м�����̬�ַ���: ����
r   ��ֻ����ʽ���ļ������ļ�������ڡ� ����
r+  �Կɶ�д��ʽ���ļ������ļ�������ڡ� ����
rb+ ��д��һ���������ļ���ֻ�����д���ݡ� ����
rt+ ��д��һ���ı��ļ����������д�� ����
w   ��ֻд�ļ������ļ��������ļ�������Ϊ0�������ļ����ݻ���ʧ�����ļ��������������ļ��� ����
w+  �򿪿ɶ�д�ļ������ļ��������ļ�������Ϊ�㣬�����ļ����ݻ���ʧ�����ļ��������������ļ��� ����
a   �Ը��ӵķ�ʽ��ֻд�ļ������ļ������ڣ���Ὠ�����ļ�������ļ����ڣ�д������ݻᱻ�ӵ��ļ�β�����ļ�ԭ�ȵ����ݻᱻ��������EOF�������� ����
a+  �Ը��ӷ�ʽ�򿪿ɶ�д���ļ������ļ������ڣ���Ὠ�����ļ�������ļ����ڣ�д������ݻᱻ�ӵ��ļ�β�󣬼��ļ�ԭ�ȵ����ݻᱻ������ ��ԭ����EOF���������� ����
wb  ֻд�򿪻��½�һ���������ļ���ֻ����д���ݡ� ����
wb+ ��д�򿪻���һ���������ļ����������д�� ����
wt+ ��д�򿪻��Ž���һ���ı��ļ��������д�� ����
at+ ��д��һ���ı��ļ�������������ı�ĩ׷�����ݡ� ����
ab+ ��д��һ���������ļ�������������ļ�ĩ׷�����ݡ� ����
��������̬�ַ����������ټ�һ��b�ַ�����rb��w+b��ab������ϣ�����b �ַ��������ߺ�����򿪵��ļ�Ϊ�������ļ������Ǵ������ļ���������POSIXϵͳ������Linux������Ը��ַ�����fopen()�����������ļ������S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH(0666)Ȩ�ޣ����ļ�Ȩ��Ҳ��ο�umaskֵ�� ����
��ЩC����ϵͳ���ܲ���ȫ�ṩ������Щ���ܣ��е�C�汾����"r+","w+","a+",����"rw","wr","ar"�ȣ�����ע������ϵͳ�Ĺ涨��
 
*/


/*
ԭ�ͣ�
	int fseek(FILE *stream, long offset, int fromwhere);
������
	���������ļ�ָ��stream��λ�á����ִ�гɹ���stream��ָ����fromwhere��ƫ����ʼλ�ã��ļ�ͷ0����ǰλ��1���ļ�β2��Ϊ��׼��ƫ��offset��ָ��ƫ���������ֽڵ�λ�á����ִ��ʧ��(����offset�����ļ������С)���򲻸ı�streamָ���λ�á�
���أ�
	�ɹ�������0�����򷵻�����ֵ�� ����
	fseek position the file���ļ��� position��λ�ã� pointer��ָ�룩 for the file referenced by stream to the byte location calculated by offset.

fseek�������ļ�ָ�룬Ӧ��Ϊ�Ѿ��򿪵��ļ������û�д򿪵��ļ�����ô������ִ��� 
fseek����Ҳ����������⣬�൱�����ļ����ж�λ�������ڶ�ȡ�����Դ洢���ļ�ʱ����������OFFSETƫ������ȡ�ļ�����������ݡ� ����
fseek����һ�����ڶ������ļ�����Ϊ�ı��ļ�Ҫ�����ַ�ת��������λ��ʱ�����ᷢ�����ҡ�

*/