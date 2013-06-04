/*****************************************************************************
 * frame.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: frame.h,v 1.1 2004/06/03 19:27:06 fenrir Exp $
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

#ifndef _FRAME_H
	#define _FRAME_H 1


	/* x264_picture_t��x264_frame_t������
	ǰ����˵��һ����Ƶ������ÿ֡���ص�.���ߴ��ÿ֡ʵ�ʵ�����ֵ 
	*/
	typedef struct
	{
		/* */
		int     i_poc;		/* poc��ʶͼ��Ĳ���˳�� ���Ϻ�ܣ���160ҳ */
		int     i_type;		/* I_type ָ��������ͼ������ͣ���X264_TYPE_AUTO X264_TYPE_IDR X264_TYPE_I X264_TYPE_P X264_TYPE_BREF X264_TYPE_B�ɹ�ѡ�񣬳�ʼ��ΪAUTO��˵����x264�ڱ�����������п��� */
		int     i_qpplus1;	/* �˲�����1����ǰ�������������ֵ */
		int64_t i_pts;		/* program time stamp ����ʱ�����ָʾ�����������ʱ��� */
		int     i_frame;    /* Presentation(��ʾ) frame number */
		int     i_frame_num; /* Coded(�����) frame number */
		int     b_kept_as_ref;	/*  */
		float   f_qp_avg;		/*  */

		/* YUV buffer */
		int     i_plane;			/*  */
		int     i_stride[4];		/* ���� */
		int     i_lines[4];			/*  */
		int     i_stride_lowres;	/*  */
		int     i_lines_lowres;		/*  */
		uint8_t *plane[4];			/* ������һ�㶼��3��YUV */
		uint8_t *filtered[4];		/* plane[0], H, V, HV */ 
									/*
										0���˲���������ص�ƽ��
										1���˲���������ص��ұ� 1/2 ���ص�ƽ��
										2���˲���������ص��±� 1/2 ���ص�ƽ��
										3���˲���������ص����� 1/2 ���ص�ƽ��
 
									*/


		uint8_t *lowres[4]; /* half-size copy of input frame: Orig, H, V, HV () */
							/*
							�ӱ������жϿ����Ǵ���²���ͼ��X264 ���и�ģ��Ҫ�õ��²���ͼ�����ȼ��㣬�ƺ������ʿ��ƣ��ǲ�����ˡ��������� lowres ��һ�ֻ���ʹ�õļ��ɣ��൱��˫����ƹ�Ҳ�����
							��ʼ�������ȷ����
							1�������²���ͼ��õ���MV
							2.
							X (previous frame,this MB)        
							Y
							Z
							Fig 1 Sources of candidate MVs for motion search.
							����"�����²���ͼ��õ���MV" �ǲ���ֱ�ӻ�ȡZ��MV
							��
							http://www.chinavideo.org/viewthread.php?tid=9942&highlight=x264%5C_frame%5C_t
							*/
		uint16_t *integral; 	/* integral:�����ģ��걸�� */

		/* for unrestricted(�����Ƶ�) mv we allocate(����) more data than needed
		 * allocated data are stored in buffer */
		void    *buffer[12];

		/* motion(�˶�) data(����) */
		int8_t  *mb_type;	/* mbģʽ */
		int16_t (*mv[2])[2];//int16_t (*mv[2])[2];����һ��ָ�����飬ָ��mv[2],mvΪ����2��Ԫ�ص�ָ������,ÿ��Ԫ�غ���2��int16_t��һά���� //http://wmnmtm.blog.163.com/blog/static/382457142011762857642/
							//��ɭ(18147145):�����2,ָ��Ӧ����x��y��ǰ���2 ָ����ǰ��ͺ���,������Ǵ��mv��Ϣ������
		int8_t  *ref[2];
		int     i_ref[2];
		int     ref_poc[2][16];//

		/* for adaptive(����Ӧ) B-frame decision(����).
		 * contains(����) the SATD cost(�ɱ�) of the lowres frame encoded in various(���ָ�����) modes
		 * FIXME: ����Ҫʱ�������һ������how big an array do we need? */
		int     i_cost_est[X264_BFRAME_MAX+2][X264_BFRAME_MAX+2];	/*  */
		int     i_satd; 	/*  */ // the i_cost_est of the selected frametype
		int     i_intra_mbs[X264_BFRAME_MAX+2];		/*  */
		int     *i_row_satds[X264_BFRAME_MAX+2][X264_BFRAME_MAX+2];		/*  */
		int     *i_row_satd;	/*  */
		int     *i_row_bits;	/*  */
		int     *i_row_qp;		/* row:��,�� */

	} x264_frame_t;	//x264_picture_t��x264_frame_t���������ڣ�ǰ����˵��һ����Ƶ������ÿ֡���ص㣬���ߴ��ÿ֡ʵ�ʵ�����ֵ��Ҫע������

	typedef void (*x264_deblock_inter_t)( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
	typedef void (*x264_deblock_intra_t)( uint8_t *pix, int stride, int alpha, int beta );
	typedef struct
	{
		x264_deblock_inter_t deblock_v_luma;
		x264_deblock_inter_t deblock_h_luma;
		x264_deblock_inter_t deblock_v_chroma;
		x264_deblock_inter_t deblock_h_chroma;
		x264_deblock_intra_t deblock_v_luma_intra;
		x264_deblock_intra_t deblock_h_luma_intra;
		x264_deblock_intra_t deblock_v_chroma_intra;
		x264_deblock_intra_t deblock_h_chroma_intra;
	} x264_deblock_function_t;

	x264_frame_t *x264_frame_new( x264_t *h );
	void          x264_frame_delete( x264_frame_t *frame );

	void          x264_frame_copy_picture( x264_t *h, x264_frame_t *dst, x264_picture_t *src );/*ͼ��֡*/

	void          x264_frame_expand_border( x264_frame_t *frame );	/* ��չ�� */
	void          x264_frame_expand_border_filtered( x264_frame_t *frame );
	void          x264_frame_expand_border_lowres( x264_frame_t *frame );
	void          x264_frame_expand_border_mod16( x264_t *h, x264_frame_t *frame );/* ��չ��16�ı��� */

	void          x264_frame_deblocking_filter( x264_t *h, int i_slice_type );

	void          x264_frame_filter( int cpu, x264_frame_t *frame );
	void          x264_frame_init_lowres( int cpu, x264_frame_t *frame );

	void          x264_deblock_init( int cpu, x264_deblock_function_t *pf );

#endif
