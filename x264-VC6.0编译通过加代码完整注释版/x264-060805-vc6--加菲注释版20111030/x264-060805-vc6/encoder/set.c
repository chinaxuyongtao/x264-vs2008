/*****************************************************************************
 * set: h264 encoder (SPS and PPS init and write)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: set.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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
#ifndef _MSC_VER
#include "config.h"
#endif

/*
transpose:�任˳��
*/
static void transpose( uint8_t *buf, int w )
{
    int i, j;
    for( i = 0; i < w; i++ )
        for( j = 0; j < i; j++ )
            XCHG( uint8_t, buf[w*i+j], buf[w*j+i] );//a��b��ֵ����,��common.h�ﶨ��
}

/*
���ű����б��﷨
��׼���Ӱ棺56/341
*/
static void scaling_list_write( bs_t *s, x264_pps_t *pps, int idx )
{
    const int len = idx<4 ? 16 : 64;
    const int *zigzag = idx<4 ? x264_zigzag_scan4 : x264_zigzag_scan8;
    const uint8_t *list = pps->scaling_list[idx];
    const uint8_t *def_list = (idx==CQM_4IC) ? pps->scaling_list[CQM_4IY]
                            : (idx==CQM_4PC) ? pps->scaling_list[CQM_4PY]
                            : x264_cqm_jvt[idx];
    if( !memcmp( list, def_list, len ) )
        bs_write( s, 1, 0 ); // scaling_list_present_flag
    else if( !memcmp( list, x264_cqm_jvt[idx], len ) )
    {
        bs_write( s, 1, 1 ); // scaling_list_present_flag
        bs_write_se( s, -8 ); // use jvt list
    }
    else
    {
        int j, run;
        bs_write( s, 1, 1 ); // scaling_list_present_flag

        // try run-length compression of trailing values
        for( run = len; run > 1; run-- )
            if( list[zigzag[run-1]] != list[zigzag[run-2]] )
                break;
        if( run < len && len - run < bs_size_se( (int8_t)-list[zigzag[run]] ) )
            run = len;

        for( j = 0; j < run; j++ )
            bs_write_se( s, (int8_t)(list[zigzag[j]] - (j>0 ? list[zigzag[j-1]] : 8)) ); // delta

        if( run < len )
            bs_write_se( s, (int8_t)-list[zigzag[run]] );
    }
}

/*
 * ���ܣ����в�������ʼ��
 * ע�⣺���ֻ�Ƕ�sps���ֶθ�ֵ������write�����������ָ�����ײ�����
 * 
*/
void x264_sps_init( x264_sps_t *sps, int i_id, x264_param_t *param )
{
    sps->i_id = i_id;

    sps->b_qpprime_y_zero_transform_bypass = param->rc.i_rc_method == X264_RC_CQP && param->rc.i_qp_constant == 0;
    if( sps->b_qpprime_y_zero_transform_bypass )
		//profile_idc 0 u(8)
        sps->i_profile_idc  = PROFILE_HIGH444;
    else if( param->analyse.b_transform_8x8 || param->i_cqm_preset != X264_CQM_FLAT )
		//profile_idc 0 u(8)
        sps->i_profile_idc  = PROFILE_HIGH;
    else if( param->b_cabac || param->i_bframe > 0 )
		//profile_idc 0 u(8)
        sps->i_profile_idc  = PROFILE_MAIN;
    else
		//profile_idc 0 u(8)
        sps->i_profile_idc  = PROFILE_BASELINE;
	//level_idc 0 u(8)
    sps->i_level_idc = param->i_level_idc;

	//constraint_set0_flag 0 u(1)
    sps->b_constraint_set0  = sps->i_profile_idc == PROFILE_BASELINE;
    /* x264 doesn't support the features(����) that are in Baseline(����) and not in Main,
     * namely(��, Ҳ����) arbitrary_slice_order(�����_Ƭ_����) and slice_groups(Ƭ��). */
	//constraint_set1_flag 0 u(1)
    sps->b_constraint_set1  = sps->i_profile_idc <= PROFILE_MAIN;
    /* Never set constraint_set2, it is not necessary and not used in real world. */
	//constraint_set2_flag 0 u(1)
    sps->b_constraint_set2  = 0;
	//constraint_set3_flag 0 u(1)

	//reserved_zero_4bits /* equal to 0 */ 0 u(4)
    sps->i_log2_max_frame_num = 4;  /* at least(����, ��С) 4 */ //��Ӧ�䷨Ԫ�� log2_max_frame_num_minus4������䷨Ԫ����Ҫ��Ϊ��ȡ��һ���䷨Ԫ��frame_num����ģ�frame_num������Ҫ�ľ䷨Ԫ��֮һ������ʶ����ͼ��Ľ���˳�򡣿����ھ䷨������frame_num�Ľ��뺯����ue(v)�������е�v������ָ����v=log2_max_frame_num_minus4
    while( (1 << sps->i_log2_max_frame_num) <= param->i_keyint_max )
    {
		//log2_max_frame_num_minus4 0 ue(v)
        sps->i_log2_max_frame_num++;
    }
	//log2_max_frame_num_minus4 0 ue(v)
    sps->i_log2_max_frame_num++;    /* just in case(����, ʵ��) */

	//pic_order_cnt_type 0 ue(v)
    sps->i_poc_type = 0;	//POC�ı��뷽�����Ϻ�ܣ���160
	//if( pic_order_cnt_type = = 0 )
    if( sps->i_poc_type == 0 )//POC���õ�0�ֱ��뷽��ʱ
    {
		//log2_max_pic_order_cnt_lsb_minus4 0 ue(v)
        sps->i_log2_max_poc_lsb = sps->i_log2_max_frame_num + 1;    /* max poc = 2*frame_num */
    }
	//else if( pic_order_cnt_type = = 1 ) {
    else if( sps->i_poc_type == 1 )//POC���õ�1�ֱ��뷽��ʱ
    {
        int i;

        /* FIXME */
		//delta_pic_order_always_zero_flag 0 u(1)
        sps->b_delta_pic_order_always_zero = 1;
		//offset_for_non_ref_pic 0 se(v)
        sps->i_offset_for_non_ref_pic = 0;
		//offset_for_top_to_bottom_field 0 se(v)
        sps->i_offset_for_top_to_bottom_field = 0;
		//num_ref_frames_in_pic_order_cnt_cycle 0 ue(v)
        sps->i_num_ref_frames_in_poc_cycle = 0;

		//for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
        for( i = 0; i < sps->i_num_ref_frames_in_poc_cycle; i++ )
        {
			//offset_for_ref_frame[ i ]
            sps->i_offset_for_ref_frame[i] = 0;
        }
    }

	//vui_parameters_present_flag 0 u(1)
    sps->b_vui = 1;//Ĭ��VUI�ӽṹ��������������
	//gaps_in_frame_num_value_allowed_flag 0 u(1)
    sps->b_gaps_in_frame_num_value_allowed = 0;//
	//pic_width_in_mbs_minus1 0 ue(v)
    sps->i_mb_width = ( param->i_width + 15 ) / 16;
	//pic_height_in_map_units_minus1 0 ue(v)
    sps->i_mb_height= ( param->i_height + 15 )/ 16;
	//frame_mbs_only_flag 0 u(1)
    sps->b_frame_mbs_only = 1;
	//mb_adaptive_frame_field_flag 0 u(1)
    sps->b_mb_adaptive_frame_field = 0;
	//direct_8x8_inference_flag 0 u(1)
    sps->b_direct8x8_inference = 0;
	//if( !frame_mbs_only_flag )
    if( sps->b_frame_mbs_only == 0 ||
        !(param->analyse.inter & X264_ANALYSE_PSUB8x8) )
    {
        sps->b_direct8x8_inference = 1;
    }

	//direct_8x8_inference_flag 0 u(1)
	//frame_crop_left_offset 0 ue(v)
    sps->crop.i_left   = 0;
	//frame_crop_right_offset 0 ue(v)
    sps->crop.i_top    = 0;
	//frame_crop_top_offset 0 ue(v)
    sps->crop.i_right  = (- param->i_width)  & 15;
	//frame_crop_bottom_offset 0 ue(v)
    sps->crop.i_bottom = (- param->i_height) & 15;
	//frame_cropping_flag 0 u(1)
    sps->b_crop = sps->crop.i_left  || sps->crop.i_top ||
                  sps->crop.i_right || sps->crop.i_bottom;

	//(H.264�ٷ����İ�) VUI�����﷨ 327/341 aspect_ratio_info_present_flag 0 u(1)
    sps->vui.b_aspect_ratio_info_present = 0;
    if( param->vui.i_sar_width > 0 && param->vui.i_sar_height > 0 )
    {
        sps->vui.b_aspect_ratio_info_present = 1;
		//sar_width 0 u(16)
        sps->vui.i_sar_width = param->vui.i_sar_width;
		//sar_height 0 u(16)
        sps->vui.i_sar_height= param->vui.i_sar_height;
    }
    
	//overscan_info_present_flag 0 u(1)
    sps->vui.b_overscan_info_present = ( param->vui.i_overscan ? 1 : 0 );
	//if( overscan_info_present_flag )
    if( sps->vui.b_overscan_info_present )
		//overscan_appropriate_flag 0 u(1)
        sps->vui.b_overscan_info = ( param->vui.i_overscan == 2 ? 1 : 0 );

    //video_signal_type_present_flag 0 u(1)
    sps->vui.b_signal_type_present = 0;
	//video_format 0 u(3)
    sps->vui.i_vidformat = ( param->vui.i_vidformat <= 5 ? param->vui.i_vidformat : 5 );
	//video_full_range_flag 0 u(1)
    sps->vui.b_fullrange = ( param->vui.b_fullrange ? 1 : 0 );
	//colour_description_present_flag 0 u(1)
    sps->vui.b_color_description_present = 0;
	
	//colour_primaries 0 u(8)
    sps->vui.i_colorprim = ( param->vui.i_colorprim <=  9 ? param->vui.i_colorprim : 2 );
	//transfer_characteristics 0 u(8)
    sps->vui.i_transfer  = ( param->vui.i_transfer  <= 11 ? param->vui.i_transfer  : 2 );
	//matrix_coefficients 0 u(8)
    sps->vui.i_colmatrix = ( param->vui.i_colmatrix <=  9 ? param->vui.i_colmatrix : 2 );
    if( sps->vui.i_colorprim != 2 ||
        sps->vui.i_transfer  != 2 ||
        sps->vui.i_colmatrix != 2 )
    {
		//colour_description_present_flag 0 u(1)
        sps->vui.b_color_description_present = 1;
    }

	//
    if( sps->vui.i_vidformat != 5 ||
        sps->vui.b_fullrange ||
        sps->vui.b_color_description_present )
    {
		//video_signal_type_present_flag 0 u(1)
        sps->vui.b_signal_type_present = 1;
    }
    
    /* FIXME: not sufficient for interlaced video */
    sps->vui.b_chroma_loc_info_present = ( param->vui.i_chroma_loc ? 1 : 0 );
    if( sps->vui.b_chroma_loc_info_present )
    {
		//chroma_sample_loc_type_top_field 0 ue(v)
        sps->vui.i_chroma_loc_top = param->vui.i_chroma_loc;
		//chroma_sample_loc_type_bottom_field 0 ue(v)
        sps->vui.i_chroma_loc_bottom = param->vui.i_chroma_loc;
    }

	//timing_info_present_flag 0 u(1)
    sps->vui.b_timing_info_present = 0;
    if( param->i_fps_num > 0 && param->i_fps_den > 0)
    {
		//timing_info_present_flag 0 u(1)
        sps->vui.b_timing_info_present = 1;
		//num_units_in_tick 0 u(32)
        sps->vui.i_num_units_in_tick = param->i_fps_den;
		//time_scale 0 u(32)
        sps->vui.i_time_scale = param->i_fps_num * 2;
		//fixed_frame_rate_flag 0 u(1)
        sps->vui.b_fixed_frame_rate = 1;
    }

	//num_reorder_frames 0 ue(v)
    sps->vui.i_num_reorder_frames = param->b_bframe_pyramid ? 2 : param->i_bframe ? 1 : 0;
    /* extra(�����) slot(λ�ã�ʱ�䣬����) with pyramid(�������ε�����) so that we don't have to override(���ԣ�����) the
     * order of forgetting old pictures */
	//max_dec_frame_buffering 0 ue(v)
    sps->vui.i_max_dec_frame_buffering =
    sps->i_num_ref_frames = X264_MIN(16, param->i_frame_reference + sps->vui.i_num_reorder_frames + param->b_bframe_pyramid);

	//bitstream_restriction_flag 0 u(1)
    sps->vui.b_bitstream_restriction = 1;
    if( sps->vui.b_bitstream_restriction )
    {	
		//motion_vectors_over_pic_boundaries_flag 0 u(1)
		sps->vui.b_motion_vectors_over_pic_boundaries = 1;
		//max_bytes_per_pic_denom 0 ue(v)
        sps->vui.i_max_bytes_per_pic_denom = 0;
		//max_bits_per_mb_denom 0 ue(v)
        sps->vui.i_max_bits_per_mb_denom = 0;
		//log2_max_mv_length_horizontal 0 ue(v)
		//log2_max_mv_length_vertical 0 ue(v)
        sps->vui.i_log2_max_mv_length_horizontal =
        sps->vui.i_log2_max_mv_length_vertical = (int)(log(param->analyse.i_mv_range*4-1)/log(2)) + 1;
    }
}

/*
���в�����д�룬����д�Ķ���
�ο��Ϻ�ܵ��飬��146��147ҳ�����в��������﷨��
��160��161��162ҳ�����в������������
   bs.h bs_write( bs_t *s, int i_count, uint32_t i_bits )
*/
void x264_sps_write( bs_t *s, x264_sps_t *sps )
{
	//���в�������ǰ�ĸ��䷨Ԫ��
	//(�Ϻ�ܵ��Ӱ�) 172/326 profile_idc 0 u(8)
    bs_write( s, 8, sps->i_profile_idc );//(�Ϻ�ܣ���145ҳ)�����κͼ�,
	//(�Ϻ�ܵ��Ӱ�) 172/326 constraint_set0_flag 0 u(1)
    bs_write( s, 1, sps->b_constraint_set0 );//(�Ϻ�ܣ���145ҳ)������1ʱ��������Ӹ�¼A.2.1...
	//(�Ϻ�ܵ��Ӱ�) 172/326 constraint_set1_flag 0 u(1)
    bs_write( s, 1, sps->b_constraint_set1 );//(�Ϻ�ܣ���145ҳ)������1ʱ��������Ӹ�¼A.2.2...
	//(�Ϻ�ܵ��Ӱ�) 172/326 constraint_set2_flag 0 u(1)
    bs_write( s, 1, sps->b_constraint_set2 );//(�Ϻ�ܣ���145ҳ)������1ʱ��������Ӹ�¼A.2.3...

	//(�Ϻ�ܵ��Ӱ�) 172/326 reserved_zero_5bits /* equal to 0 */ 0 u(5)
    bs_write( s, 5, 0 );    /* reserved(Ԥ����,������) *///(�Ϻ�ܣ���145,160ҳ)����Ŀǰ�ı�׼�У��䷨Ԫ��reserved_zero_5bits�������0��������ֵ���������ã�������Ӧ�ú��Ա��䷨Ԫ�ص�ֵ��

	//(�Ϻ�ܵ��Ӱ�) 172/326 level_idc 0 u(8)
    bs_write( s, 8, sps->i_level_idc );	//(�Ϻ�ܣ���146ҳ)����Ӧ�䷨Ԫ��level_idc��

	//(�Ϻ�ܵ��Ӱ�) 172/326 seq_parameter_set_id 0 ue(v)
    bs_write_ue( s, sps->i_id );//(�Ϻ�ܣ���146ҳ)�����Ӧ�þ������в�������id����ͼ�����������ʱ�õ�

	//??????
    if( sps->i_profile_idc >= PROFILE_HIGH )
    {
        bs_write_ue( s, 1 ); // chroma_format_idc = 4:2:0
        bs_write_ue( s, 0 ); // bit_depth_luma_minus8
        bs_write_ue( s, 0 ); // bit_depth_chroma_minus8
        bs_write( s, 1, sps->b_qpprime_y_zero_transform_bypass );
        bs_write( s, 1, 0 ); // seq_scaling_matrix_present_flag
    }

	//(�Ϻ�ܵ��Ӱ�) 172/326 log2_max_frame_num_minus4 0 ue(v)
    bs_write_ue( s, sps->i_log2_max_frame_num - 4 );

	//(�Ϻ�ܵ��Ӱ�) 172/326 pic_order_cnt_type 0 ue(v)
	bs_write_ue( s, sps->i_poc_type );//��Ӧ�䷨Ԫ��pic_order_cnt_type��ָ����POC�ı��뷽����POC��ʶͼ��Ĳ���˳��H.264һ��������3��POC�ı��뷽��������䷨Ԫ�ؾ�������֪ͨ�������������ַ���������POC

	//(�Ϻ�ܵ��Ӱ�) 172/326 if( pic_order_cnt_type = = 0 )
    if( sps->i_poc_type == 0 )
    {
		//(�Ϻ�ܵ��Ӱ�) 172/326 log2_max_pic_order_cnt_lsb_minus4
        bs_write_ue( s, sps->i_log2_max_poc_lsb - 4 );
    }
    else if( sps->i_poc_type == 1 )//(�Ϻ�ܵ��Ӱ�) 172/326 else if( pic_order_cnt_type = = 1 )
    {
        int i;

		//(�Ϻ�ܵ��Ӱ�) 172/326 delta_pic_order_always_zero_flag 0 u(1)
        bs_write( s, 1, sps->b_delta_pic_order_always_zero );//��Ӧ�䷨Ԫ��delta_pic_order_always_zero_flag������1ʱ���䷨Ԫ��delta_pic_order_cnt[0]��delta_pic_order_cnt[1]����Ƭͷ���֣��������ǵ�ֵĬ��Ϊ0�����䷨Ԫ�ص���0ʱ�������������䷨Ԫ�ؽ���Ƭͷ���֡�
		//(�Ϻ�ܵ��Ӱ�) 172/326 offset_for_non_ref_pic 0 se(v)
        bs_write_se( s, sps->i_offset_for_non_ref_pic );//��Ӧ�䷨Ԫ��offset_for_non_ref_pic������������ǲο�֡�򳡵�picture order count�����䷨Ԫ�ص�ֵӦ����[-2^31,2^31-1]��Χ�ڡ�
		//(�Ϻ�ܵ��Ӱ�) 172/326 offset_for_top_to_bottom_field 0 se(v)
        bs_write_se( s, sps->i_offset_for_top_to_bottom_field );//��Ӧ�䷨Ԫ��offset_for_top_to_bottom_field������������ͼ��֡�еĵ׳���picture order count�����䷨Ԫ�ص�ֵӦ����[-2^31,2^31-1]��Χ�ڡ�
		//(�Ϻ�ܵ��Ӱ�) 172/326 num_ref_frames_in_pic_order_cnt_cycle 0 ue(v)
        bs_write_ue( s, sps->i_num_ref_frames_in_poc_cycle );//��Ӧ�䷨Ԫ��num_ref_frames_in_pic_order_cnt_cyle������������picuter order count�����䷨Ԫ�ص�ֵӦ����[0,255]��Χ�ڡ�

		//(�Ϻ�ܵ��Ӱ�) 172/326 for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
        for( i = 0; i < sps->i_num_ref_frames_in_poc_cycle; i++ )
        {
			//(�Ϻ�ܵ��Ӱ�) 172/326 offset_for_ref_frame[ i ] 0 se(v)
            bs_write_se( s, sps->i_offset_for_ref_frame[i] );//��Ӧ�䷨Ԫ��offset_for_ref_frame[i]����pictuer order count type =1ʱ�ã����ڽ���POC�����䷨Ԫ�ض�ѭ��num_ref_frames_in_pic_order_cycle�е�ÿһ��Ԫ��ָ��һ��ƫ��
        }
    }

	//(�Ϻ�ܵ��Ӱ�) num_ref_frames 0 ue(v)
    bs_write_ue( s, sps->i_num_ref_frames );//��Ӧ�ھ䷨Ԫ��num_ref_frames��ָ���ο�֡���п��ܴﵽ����󳤶ȣ���������������䷨Ԫ�ص�ֵ���ڴ洢��������洢�����ڴ���ѽ���Ĳο�֡��H.264�涨������16���ο�֡�����䷨Ԫ�����ֵΪ16��ֵ��ע��������������֡Ϊ��λ������ڳ�ģʽ�£�Ӧ����Ӧ����չһ����
	//(�Ϻ�ܵ��Ӱ�) gaps_in_frame_num_value_allowed_flag 0 u(1)
    bs_write( s, 1, sps->b_gaps_in_frame_num_value_allowed );//��Ӧ�ھ䷨Ԫ��gaps_in_frame_num_value_allowed_flag������䷨Ԫ�ص���1ʱ����ʾ����䷨Ԫ��frame_num���Բ��������������ŵ���������ʱ����������������������ͼ��ȫ����������ʱ����������֡ͼ�������������ÿһ֡ͼ��������������frame_numֵ����������鵽���frame_num������������ȷ����ͼ�񱻱�������������ʱ���������������������ڲػ��������ӵػָ���Щͼ����Ϊ��Щͼ���п��ܱ�����ͼ�������ο�֡��
	//(�Ϻ�ܵ��Ӱ�) 172/326 pic_width_in_mbs_minus1 0 ue(v)
    bs_write_ue( s, sps->i_mb_width - 1 );//���
	//(�Ϻ�ܵ��Ӱ�) 172/326 pic_height_in_map_units_minus1 0 ue(v)
    bs_write_ue( s, sps->i_mb_height - 1);//�߶�(�Ժ��Ϊ��λ��ʾ)

	/*
	 * (�Ϻ�ܵ��Ӱ�) 172/326 frame_mbs_only_flag 0 u(1)
	*/
    bs_write( s, 1, sps->b_frame_mbs_only );//��Ӧ�䷨Ԫ��frame_mbs_only_flag�����䷨Ԫ��Ϊ1ʱ����ʾ������������ͼ��ı���ģʽ����֡��û����������ģʽ���ڣ����䷨Ԫ�ص���0ʱ����ʾ��������ͼ��ı���ģʽ������֡Ҳ�����ǳ���֡������Ӧ��ĳ��ͼ���������һ��Ҫ�������䷨Ԫ�ؾ�����
	/*
	 * (�Ϻ�ܵ��Ӱ�) 172/326 if( !frame_mbs_only_flag )
	*/
    if( !sps->b_frame_mbs_only )
    {
		/*
		 * (�Ϻ�ܵ��Ӱ�) 172/326 mb_adaptive_frame_field_flag 0 u(1)
		*/
        bs_write( s, 1, sps->b_mb_adaptive_frame_field );//ָ���������Ƿ�����֡������Ӧģʽ������1ʱ�������ڱ������е�ͼ��������ǳ�ģʽ������֡������Ӧģʽ������0ʱ����ʾ�������е�ͼ��������ǳ�ģʽ������֡ģʽ��
    }
	/*
	 * (�Ϻ�ܵ��Ӱ�) 172/326 direct_8x8_inference_flag 0 u(1)
	*/
    bs_write( s, 1, sps->b_direct8x8_inference );//����ָ��BƬ��ֱ�Ӻ�skipģʽ���˶�ʸ����Ԥ�ⷽ��
	/*
	 * (�Ϻ�ܵ��Ӱ�) 172/326 frame_cropping_flag 0 u(1)
	*/
    bs_write( s, 1, sps->b_crop );//�ü�

	/*
	 * �Ƿ�ü�
	 * (�Ϻ�ܵ��Ӱ�) 172/326
	*/
    if( sps->b_crop )//�ü�,����ָ���������Ƿ�Ҫ��ͼ��ü������������ǵĻ�������������ĸ��䷨Ԫ�طֱ�ָ���������²ü��Ŀ�ȡ��Ϻ�ܣ���162ҳ
    {
        bs_write_ue( s, sps->crop.i_left   / 2 );//�� frame_crop_left_offset    0  ue(v)
        bs_write_ue( s, sps->crop.i_right  / 2 );//�� frame_crop_right_offset   0  ue(v)
        bs_write_ue( s, sps->crop.i_top    / 2 );//�� frame_crop_top_offset     0  ue(v)
        bs_write_ue( s, sps->crop.i_bottom / 2 );//�� frame_crop_bottom_offset  0  ue(v)
    }

	/*
	 * 
	 * �Ϻ�ܵ��Ӱ棺172/326
	 * vui_parameters_present_flag 0 u(1)
	*/
    bs_write( s, 1, sps->b_vui );//��Ӧvui_parameters_present_flag�䷨Ԫ�أ�ָ��vui�ӽṹ�Ƿ�����������У�vui�������ṹ�ڱ�׼����˵����
    if( sps->b_vui ) //if( vui_parameters_present_flag )
    {
		//start:vui_parameters( ) //�Ϻ�ܵ��Ӱ棺173/326
		//VUI�����﷨ H.264�ٷ����İ�.pdf 327/341 
		//H.264�ٷ����İ�.pdf 327/341 aspect_ratio_info_present_flag 0 u(1)
        bs_write1( s, sps->vui.b_aspect_ratio_info_present );//aspect:����;ratio:����;present:���ڵ�, ���е�;����; Ŀǰ��
		
        if( sps->vui.b_aspect_ratio_info_present )//if( aspect_ratio_info_present_flag ) {
        {
            int i;
            static const struct { int w, h; int sar; } sar[] =
            {
                { 1,   1, 1 }, { 12, 11, 2 }, { 10, 11, 3 }, { 16, 11, 4 },
                { 40, 33, 5 }, { 24, 11, 6 }, { 20, 11, 7 }, { 32, 11, 8 },
                { 80, 33, 9 }, { 18, 11, 10}, { 15, 11, 11}, { 64, 33, 12},
                { 160,99, 13}, { 0, 0, -1 }
            };
            for( i = 0; sar[i].sar != -1; i++ )
            {
                if( sar[i].w == sps->vui.i_sar_width &&
                    sar[i].h == sps->vui.i_sar_height )
                    break;
            }
            if( sar[i].sar != -1 )
            {
                bs_write( s, 8, sar[i].sar );//
            }
            else
            {
				//aspect_ratio_idc 0 u(8)
                bs_write( s, 8, 255);   /* aspect_ratio_idc (extented) */ //if( aspect_ratio_idc = = Extended_SAR )
				//sar_width 0 u(16)
                bs_write( s, 16, sps->vui.i_sar_width );
				//sar_height 0 u(16)
                bs_write( s, 16, sps->vui.i_sar_height );
            }
        }

		//overscan_info_present_flag 0 u(1)
        bs_write1( s, sps->vui.b_overscan_info_present );//
		//if( overscan_info_present_flag )
        if( sps->vui.b_overscan_info_present )
			//overscan_appropriate_flag 0 u(1)
            bs_write1( s, sps->vui.b_overscan_info );

		//video_signal_type_present_flag 0 u(1)
        bs_write1( s, sps->vui.b_signal_type_present );
		//if( video_signal_type_present_flag ) {
        if( sps->vui.b_signal_type_present )
        {
			//video_format 0 u(3)
            bs_write( s, 3, sps->vui.i_vidformat );
			//video_full_range_flag 0 u(1)
            bs_write1( s, sps->vui.b_fullrange );
			//colour_description_present_flag 0 u(1)
            bs_write1( s, sps->vui.b_color_description_present );
			//if( colour_description_present_flag ) {
            if( sps->vui.b_color_description_present )
            {
				//H.264�ٷ����İ�.pdf 327/341 colour_primaries 0 u(8)
                bs_write( s, 8, sps->vui.i_colorprim );
				//transfer_characteristics 0 u(8)
                bs_write( s, 8, sps->vui.i_transfer );
				//matrix_coefficients 0 u(8)
                bs_write( s, 8, sps->vui.i_colmatrix );
            }
        }

		//H.264�ٷ����İ�.pdf 327/341 chroma_loc_info_present_flag 0 u(1)
        bs_write1( s, sps->vui.b_chroma_loc_info_present );
		//if( chroma_loc_info_present_flag ) {
        if( sps->vui.b_chroma_loc_info_present )
        {
			//chroma_sample_loc_type_top_field 0 ue(v)
            bs_write_ue( s, sps->vui.i_chroma_loc_top );
			//chroma_sample_loc_type_bottom_field 0 ue(v)
            bs_write_ue( s, sps->vui.i_chroma_loc_bottom );
        }
		//H.264�ٷ����İ�.pdf timing_info_present_flag 0 u(1)
        bs_write1( s, sps->vui.b_timing_info_present );
		//if( timing_info_present_flag ) {
        if( sps->vui.b_timing_info_present )
        {
			//num_units_in_tick 0 u(32)
            bs_write( s, 32, sps->vui.i_num_units_in_tick );
			//time_scale 0 u(32)
            bs_write( s, 32, sps->vui.i_time_scale );
			//fixed_frame_rate_flag 0 u(1)
            bs_write1( s, sps->vui.b_fixed_frame_rate );
        }

		//nal_hrd_parameters_present_flag 0 u(1)
        bs_write1( s, 0 );      /* nal_hrd_parameters_present_flag */
//		if( nal_hrd_parameters_present_flag )
//			hrd_parameters( )
		//vcl_hrd_parameters_present_flag 0 u(1)
        bs_write1( s, 0 );      /* vcl_hrd_parameters_present_flag */
//		if( vcl_hrd_parameters_present_flag )
//			hrd_parameters( )
//		if( nal_hrd_parameters_present_flag | | vcl_hrd_parameters_present_flag )
//			low_delay_hrd_flag 0 u(1)
		//pic_struct_present_flag 0 u(1)
        bs_write1( s, 0 );      /* pic_struct_present_flag */
		//bitstream_restriction_flag 0 u(1)
        bs_write1( s, sps->vui.b_bitstream_restriction );
		//if( bitstream_restriction_flag ) {
        if( sps->vui.b_bitstream_restriction )
        {
			//H.264�ٷ����İ�.pdf motion_vectors_over_pic_boundaries_flag 0 u(1)
            bs_write1( s, sps->vui.b_motion_vectors_over_pic_boundaries );
			//H.264�ٷ����İ�.pdf max_bytes_per_pic_denom 0 ue(v)
            bs_write_ue( s, sps->vui.i_max_bytes_per_pic_denom );
			//H.264�ٷ����İ�.pdf max_bits_per_mb_denom 0 ue(v)
            bs_write_ue( s, sps->vui.i_max_bits_per_mb_denom );
			//H.264�ٷ����İ�.pdf log2_max_mv_length_horizontal 0 ue(v)
            bs_write_ue( s, sps->vui.i_log2_max_mv_length_horizontal );
			//H.264�ٷ����İ�.pdf log2_max_mv_length_vertical 0 ue(v)
            bs_write_ue( s, sps->vui.i_log2_max_mv_length_vertical );
			//H.264�ٷ����İ�.pdf num_reorder_frames 0 ue(v)
            bs_write_ue( s, sps->vui.i_num_reorder_frames );
			//H.264�ٷ����İ�.pdf max_dec_frame_buffering 0 ue(v)
            bs_write_ue( s, sps->vui.i_max_dec_frame_buffering );
        }
		//end:vui_parameters()
    } 

    bs_rbsp_trailing( s );	//��β�䷨
}

/*

*/
void x264_pps_init( x264_pps_t *pps, int i_id, x264_param_t *param, x264_sps_t *sps )
{
    int i, j;

    pps->i_id = i_id;
    pps->i_sps_id = sps->i_id;
    pps->b_cabac = param->b_cabac;

    pps->b_pic_order = 0;
    pps->i_num_slice_groups = 1;

    pps->i_num_ref_idx_l0_active = 1;
    pps->i_num_ref_idx_l1_active = 1;

    pps->b_weighted_pred = 0;
    pps->b_weighted_bipred = param->analyse.b_weighted_bipred ? 2 : 0;

    pps->i_pic_init_qp = param->rc.i_rc_method == X264_RC_ABR ? 26 : param->rc.i_qp_constant;
    pps->i_pic_init_qs = 26;

    pps->i_chroma_qp_index_offset = param->analyse.i_chroma_qp_offset;
    pps->b_deblocking_filter_control = 1;
    pps->b_constrained_intra_pred = 0;
    pps->b_redundant_pic_cnt = 0;

    pps->b_transform_8x8_mode = param->analyse.b_transform_8x8 ? 1 : 0;

    pps->i_cqm_preset = param->i_cqm_preset;
    switch( pps->i_cqm_preset )
    {
    case X264_CQM_FLAT:
        for( i = 0; i < 6; i++ )
            pps->scaling_list[i] = x264_cqm_flat16;
        break;
    case X264_CQM_JVT:
        for( i = 0; i < 6; i++ )
            pps->scaling_list[i] = x264_cqm_jvt[i];
        break;
    case X264_CQM_CUSTOM:
        /* match the transposed DCT & zigzag */
        transpose( param->cqm_4iy, 4 );
        transpose( param->cqm_4ic, 4 );
        transpose( param->cqm_4py, 4 );
        transpose( param->cqm_4pc, 4 );
        transpose( param->cqm_8iy, 8 );
        transpose( param->cqm_8py, 8 );
        pps->scaling_list[CQM_4IY] = param->cqm_4iy;
        pps->scaling_list[CQM_4IC] = param->cqm_4ic;
        pps->scaling_list[CQM_4PY] = param->cqm_4py;
        pps->scaling_list[CQM_4PC] = param->cqm_4pc;
        pps->scaling_list[CQM_8IY+4] = param->cqm_8iy;
        pps->scaling_list[CQM_8PY+4] = param->cqm_8py;
        for( i = 0; i < 6; i++ )
            for( j = 0; j < (i<4?16:64); j++ )
                if( pps->scaling_list[i][j] == 0 )
                    pps->scaling_list[i] = x264_cqm_jvt[i];
        break;
    }
}

void x264_pps_write( bs_t *s, x264_pps_t *pps )
{
	//pic_parameter_set_id 1 ue(v)
    bs_write_ue( s, pps->i_id );
	//seq_parameter_set_id 1 ue(v)
    bs_write_ue( s, pps->i_sps_id );

	//entropy_coding_mode_flag 1 u(1)
    bs_write( s, 1, pps->b_cabac );
	//pic_order_present_flag 1 u(1)
    bs_write( s, 1, pps->b_pic_order );
	//num_slice_groups_minus1 1 ue(v)
    bs_write_ue( s, pps->i_num_slice_groups - 1 );

	//num_ref_idx_l0_active_minus1 1 ue(v)
    bs_write_ue( s, pps->i_num_ref_idx_l0_active - 1 );
	//num_ref_idx_l1_active_minus1 1 ue(v)
    bs_write_ue( s, pps->i_num_ref_idx_l1_active - 1 );
	//weighted_pred_flag 1 u(1)
    bs_write( s, 1, pps->b_weighted_pred );
	//weighted_bipred_idc 1 u(2)
    bs_write( s, 2, pps->b_weighted_bipred );

	//pic_init_qp_minus26 /* relative to 26 */ 1 se(v)
    bs_write_se( s, pps->i_pic_init_qp - 26 );
	//pic_init_qs_minus26 /* relative to 26 */ 1 se(v)
    bs_write_se( s, pps->i_pic_init_qs - 26 );
	//chroma_qp_index_offset 1 se(v)
    bs_write_se( s, pps->i_chroma_qp_index_offset );

	//deblocking_filter_control_present_flag 1 u(1)
    bs_write( s, 1, pps->b_deblocking_filter_control );
	//constrained_intra_pred_flag 1 u(1)
    bs_write( s, 1, pps->b_constrained_intra_pred );
	//redundant_pic_cnt_present_flag 1 u(1)
    bs_write( s, 1, pps->b_redundant_pic_cnt );

	//if( more_rbsp_data( ) ) {
    if( pps->b_transform_8x8_mode || pps->i_cqm_preset != X264_CQM_FLAT )
    {
		//transform_8x8_mode_flag 1 u(1)
        bs_write( s, 1, pps->b_transform_8x8_mode );
        bs_write( s, 1, (pps->i_cqm_preset != X264_CQM_FLAT) );
        if( pps->i_cqm_preset != X264_CQM_FLAT )
        {
            scaling_list_write( s, pps, CQM_4IY );
            scaling_list_write( s, pps, CQM_4IC );
            bs_write( s, 1, 0 ); // Cr = Cb
            scaling_list_write( s, pps, CQM_4PY );
            scaling_list_write( s, pps, CQM_4PC );
            bs_write( s, 1, 0 ); // Cr = Cb
            if( pps->b_transform_8x8_mode )
            {
                scaling_list_write( s, pps, CQM_8IY+4 );
                scaling_list_write( s, pps, CQM_8PY+4 );
            }
        }
		//second_chroma_qp_index_offset 1 se(v)
        bs_write_se( s, pps->i_chroma_qp_index_offset );
    }

	//rbsp_trailing_bits() 1
    bs_rbsp_trailing( s );
}

void x264_sei_version_write( x264_t *h, bs_t *s )
{
    int i;
    // random ID number generated according to ISO-11578
    const uint8_t uuid[16] = {
        0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7,
        0x96, 0x2c, 0xd8, 0x20, 0xd9, 0x23, 0xee, 0xef
    };
    char version[1200];
    int length;
    char *opts = x264_param2string( &h->param, 0 );

    sprintf( version, "x264 - core %d%s - H.264/MPEG-4 AVC codec - "
             "Copyleft 2005 - http://www.videolan.org/x264.html - options: %s",
             X264_BUILD, X264_VERSION, opts );//Copyleft��ӯ����Ȩ
    x264_free( opts );
    length = strlen(version)+1+16;

    bs_write( s, 8, 0x5 ); // payload_type = user_data_unregistered
    // payload_size
    for( i = 0; i <= length-255; i += 255 )
        bs_write( s, 8, 255 );
    bs_write( s, 8, length-i );

    for( i = 0; i < 16; i++ )
        bs_write( s, 8, uuid[i] );
    for( i = 0; i < length-16; i++ )
        bs_write( s, 8, version[i] );

    bs_rbsp_trailing( s );
}

const x264_level_t x264_levels[] =
{
    { 10,   1485,    99,   152064,     64,    175,  64, 64,  0, 0, 0, 1 },
//  {"1b",  1485,    99,   152064,    128,    350,  64, 64,  0, 0, 0, 1 },
    { 11,   3000,   396,   345600,    192,    500, 128, 64,  0, 0, 0, 1 },
    { 12,   6000,   396,   912384,    384,   1000, 128, 64,  0, 0, 0, 1 },
    { 13,  11880,   396,   912384,    768,   2000, 128, 64,  0, 0, 0, 1 },
    { 20,  11880,   396,   912384,   2000,   2000, 128, 64,  0, 0, 0, 1 },
    { 21,  19800,   792,  1824768,   4000,   4000, 256, 64,  0, 0, 0, 0 },
    { 22,  20250,  1620,  3110400,   4000,   4000, 256, 64,  0, 0, 0, 0 },
    { 30,  40500,  1620,  3110400,  10000,  10000, 256, 32, 22, 0, 1, 0 },
    { 31, 108000,  3600,  6912000,  14000,  14000, 512, 16, 60, 1, 1, 0 },
    { 32, 216000,  5120,  7864320,  20000,  20000, 512, 16, 60, 1, 1, 0 },
    { 40, 245760,  8192, 12582912,  20000,  25000, 512, 16, 60, 1, 1, 0 },
    { 41, 245760,  8192, 12582912,  50000,  62500, 512, 16, 24, 1, 1, 0 },
    { 42, 522240,  8704, 13369344,  50000,  62500, 512, 16, 24, 1, 1, 1 },
    { 50, 589824, 22080, 42393600, 135000, 135000, 512, 16, 24, 1, 1, 1 },
    { 51, 983040, 36864, 70778880, 240000, 240000, 512, 16, 24, 1, 1, 1 },
    { 0 }
};

//x264���õļ�
void x264_validate_levels( x264_t *h )//validate:��Ч��֤ʵ;ȷ֤
{
    int mbs;

    const x264_level_t *l = x264_levels;
    while( l->level_idc != 0 && l->level_idc != h->param.i_level_idc )
        l++;

    mbs = h->sps->i_mb_width * h->sps->i_mb_height;
    if( l->frame_size < mbs
        || l->frame_size*8 < h->sps->i_mb_width * h->sps->i_mb_width
        || l->frame_size*8 < h->sps->i_mb_height * h->sps->i_mb_height )
        x264_log( h, X264_LOG_WARNING, "frame MB size (%dx%d) > level limit (%d)\n",
                  h->sps->i_mb_width, h->sps->i_mb_height, l->frame_size );

#define CHECK( name, limit, val ) \
    if( (val) > (limit) ) \
        x264_log( h, X264_LOG_WARNING, name " (%d) > level limit (%d)\n", (int)(val), (limit) );

    CHECK( "DPB size", l->dpb, mbs * 384 * h->sps->i_num_ref_frames );
    CHECK( "VBV bitrate", l->bitrate, h->param.rc.i_vbv_max_bitrate );
    CHECK( "VBV buffer", l->cpb, h->param.rc.i_vbv_buffer_size );
    CHECK( "MV range", l->mv_range, h->param.analyse.i_mv_range );

    if( h->param.i_fps_den > 0 )
        CHECK( "MB rate", l->mbps, (int64_t)mbs * h->param.i_fps_num / h->param.i_fps_den );

    /* TODO check the rest(��Ϣ, ˯�ߣ���ֹ����ֹ��) of the limits(����) */
}
