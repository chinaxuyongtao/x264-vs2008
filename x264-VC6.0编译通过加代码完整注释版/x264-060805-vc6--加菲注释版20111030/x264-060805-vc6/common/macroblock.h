/*****************************************************************************
 * macroblock.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: macroblock.h,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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

#ifndef _MACROBLOCK_H
#define _MACROBLOCK_H 1

/*
ö�٣����_λ��_ö��
����_e����ö�ٵ�enum
*/
enum macroblock_position_e
{
    MB_LEFT     = 0x01,	//��
    MB_TOP      = 0x02,	//��
    MB_TOPRIGHT = 0x04,	//����
    MB_TOPLEFT  = 0x08,	//����

    MB_PRIVATE  = 0x10,	//PRIVATE:˽�˵�; ���˵�2. ����

    ALL_NEIGHBORS = 0xf,	//����_��Ԫ�� (������ָ���ϡ����ϡ����϶���)
};

/*
neighbors:��Ԫ��,�ڽڵ�
��׼��8.3.1.2 Intra_4x4����Ԥ�� ����120ҳ
�򣬱Ϻ�ܵ��飬��200ҳ
֡��4x4Ԥ��ģʽ�£��ڽ��������
*/
static const int x264_pred_i4x4_neighbors[12] =
{
    MB_TOP,                         // I_PRED_4x4_V		[�Ϻ�ܣ���200ҳ,Intra_4x4_VerticalԤ��ģʽ]���_����ĺ��
    MB_LEFT,                        // I_PRED_4x4_H		[�Ϻ�ܣ���200ҳ,Intra_4x4_HorizontalԤ��ģʽ] ���_����ĺ��
    MB_LEFT | MB_TOP,               // I_PRED_4x4_DC	[�Ϻ�ܣ���200ҳ,Intra_4x4_DCԤ��ģʽ]	(A+B+C+D+I+J+K+L+4)/8	���_����ĺ�� | ���_����ĺ�� (|�ǰ�λ�򣬲���)
    MB_TOP  | MB_TOPRIGHT,          // I_PRED_4x4_DDL	[�Ϻ�ܣ���200ҳ,Intra_4x4_Diagonal_Down_LeftԤ��ģʽ]���_����� | ���_���� ���� 
    MB_LEFT | MB_TOPLEFT | MB_TOP,  // I_PRED_4x4_DDR	[�Ϻ�ܣ���200ҳ,Intra_4x4_Diagonal_Down_RightԤ��ģʽ]���_����� | ���_���Ϸ��� | ���_�����
    MB_LEFT | MB_TOPLEFT | MB_TOP,  // I_PRED_4x4_VR	[�Ϻ�ܣ���200ҳ,Intra_4x4_Vertical_RightԤ��ģʽ]ͬ��
    MB_LEFT | MB_TOPLEFT | MB_TOP,  // I_PRED_4x4_HD	[�Ϻ�ܣ���200ҳ,Intra_4x4_Horizontal_DownԤ��ģʽ]ͬ��
    MB_TOP  | MB_TOPRIGHT,          // I_PRED_4x4_VL	[�Ϻ�ܣ���200ҳ,Intra_4x4_Vertical_LeftԤ��ģʽ]���_����� | ���_���Ϸ���
    MB_LEFT,                        // I_PRED_4x4_HU	[�Ϻ�ܣ���200ҳ,Intra_4x4_Horizontal_UpԤ��ģʽ]	
    MB_LEFT,                        // I_PRED_4x4_DC_LEFT	[�Ϻ�ܣ���204ҳ,Intra_4x4_DC]	(I+J+K+L+2)/4
    MB_TOP,                         // I_PRED_4x4_DC_TOP	[�Ϻ�ܣ���204ҳ,Intra_4x4_DC] (A+B+C+D+2)/4
    0                               // I_PRED_4x4_DC_128	[�Ϻ�ܣ���204ҳ,Intra_4x4_DC]	(128,...,128)	DCģʽ��4���������Ӧ4��ͼ��ֻ����ߣ�ֻ���ϱߣ�ͬʱ������ϱߣ�����϶�û�У�ֱ��Ԥ��ֵΪ128
};


/* XXX mb_type isn't the one written in the bitstream -> only internal usage */
#define IS_INTRA(type) ( (type) == I_4x4 || (type) == I_8x8 || (type) == I_16x16 )
#define IS_SKIP(type)  ( (type) == P_SKIP || (type) == B_SKIP )
#define IS_DIRECT(type)  ( (type) == B_DIRECT )

/*
ö��
��������ö������֮�󣬿����������������,��C�����У�ö�������������ؼ���enum�����ϵĶ������дΪ
��C++������дenum��һ��Ҳ��дenum����������C���÷���
enum ö�������� {ö�ٳ�������}; 
*/
/*
���ģʽѡ��
ѡ��ģʽǰ���Ȱ�mbģ��������оٳ���
*/
enum mb_class_e
{
	//��I_��ʾ����I֡�ڵĺ��ģʽ������֡��Ԥ��
    I_4x4           = 0,//��ʾʹ��֡��4*4Ԥ�⣬���ֻ��һ��16*16��������4*4�ĺ��
    I_8x8           = 1,//??????????????????
    I_16x16         = 2,//??????????????????
    I_PCM           = 3,//ֱ�Ӵ�������ֵ

	//PƬ�е�mb_type [�Ϻ�ܣ���173ҳ] ��6.28
    P_L0            = 4,//��ʾ�òο��б�L0����ǰ��Ԥ��
    P_8x8           = 5,//Ԥ��ģʽ�ޣ���������Ⱥ͸߶Ⱦ�Ϊ8
    P_SKIP          = 6,//Ԥ��ģʽ(mb_type,0)ΪPred_L0��Ԥ��ģʽ(mb_type,1)Ԫ����������Ⱥ͸߶Ⱦ�Ϊ16

	//BƬ�е�mb_type [�Ϻ�ܣ���173ҳ] ��6.29
    B_DIRECT        = 7,//[�Ϻ�ܣ���173ҳ B_Direct_16x16] ��������Ⱥ͸߶�8//b_direct, һ�ֺ�����ͣ�����direct��Ԥ��ģʽ
    B_L0_L0         = 8,//[�Ϻ�ܣ���173ҳ ] 
    B_L0_L1         = 9,//[�Ϻ�ܣ���173ҳ ] 
    B_L0_BI         = 10,//[�Ϻ�ܣ���173ҳ ] 
    B_L1_L0         = 11,//[�Ϻ�ܣ���173ҳ ] 
    B_L1_L1         = 12,//[�Ϻ�ܣ���173ҳ ] 
    B_L1_BI         = 13,//[�Ϻ�ܣ���173ҳ ] 
    B_BI_L0         = 14,//[�Ϻ�ܣ���173ҳ ] 
    B_BI_L1         = 15,//[�Ϻ�ܣ���173ҳ ] 
    B_BI_BI         = 16,//[�Ϻ�ܣ���173ҳ ] 
    B_8x8           = 17,//[�Ϻ�ܣ���173ҳ ] 
    B_SKIP          = 18,//[�Ϻ�ܣ���173ҳ ] //p/b_skip��һ�ֺ�����ͣ���ͼ�����֡��Ԥ�����ʱ����ͼ��ƽ̹������ʹ�á���Ծ���飬����Ծ���鱾��Я���κ����ݣ��ڽ������ͨ�� direct��ʽԤ���MV����ֱ����Χ���ؽ��ĺ�����ָ�������BƬ�е�skip����ǲ���directģʽ����ʱ��Ϳռ��directԤ�ⷽʽ���� ��PƬ��skip������������Χ���ؽ��ĺ��copy����
};//[�Ϻ�ܣ�ֽ��ڶ���] 172,173ҳ������I��P��BƬ�е�mb_type��������

/*
һ����̬����
���涨���ö���е�Ԫ�أ�����ֱ�ӵ���Ӧ��int���������ã�����I_4x4��0�ȼۣ�B_SKIP��18�ȼۣ�����������������У��Ϳ��������
*/
static const int x264_mb_type_fix[19] =						/* �����������ֵȼ� */
{
    I_4x4, I_4x4, I_16x16, I_PCM,							/* 0��1��2��3 */
    P_L0, P_8x8, P_SKIP,									/* 4��5��6 */
    B_DIRECT, B_L0_L0, B_L0_L1, B_L0_BI, B_L1_L0, B_L1_L1,	/* 7��8��9��10��11��12 */
    B_L1_BI, B_BI_L0, B_BI_L1, B_BI_BI, B_8x8, B_SKIP		/* 13��14��15��16��17��18 */
};

/*
fix��ȷ��
��ά����
������Ͳ��ұ�list0

����������Ǹ����������ĳ�ֶ�Ӧ��ϵ���ͺ���ͬʱ�����������Ǹ�����һ��
*/
static const int x264_mb_type_list0_table[19][2] =
{
    {0,0}, {0,0}, {0,0}, {0,0}, /* INTRA */

    {1,1},                  /* P_L0 */
    {0,0},                  /* P_8x8 */
    {1,1},                  /* P_SKIP */

    {0,0},                  /* B_DIRECT */
    {1,1}, {1,0}, {1,1},    /* B_L0_* */
    {0,1}, {0,0}, {0,1},    /* B_L1_* */
    {1,1}, {1,0}, {1,1},    /* B_BI_* */
    {0,0},                  /* B_8x8 */
    {0,0}                   /* B_SKIP */
};

/*
*/
static const int x264_mb_type_list1_table[19][2] =
{
    {0,0}, {0,0}, {0,0}, {0,0}, /* INTRA */
    {0,0},                  /* P_L0 */
    {0,0},                  /* P_8x8 */
    {0,0},                  /* P_SKIP */
    {0,0},                  /* B_DIRECT */
    {0,0}, {0,1}, {0,1},    /* B_L0_* */
    {1,0}, {1,1}, {1,1},    /* B_L1_* */
    {1,0}, {1,1}, {1,1},    /* B_BI_* */
    {0,0},                  /* B_8x8 */
    {0,0}                   /* B_SKIP */
};

//�ж��Ƿ���ĳ���ӿ�
#define IS_SUB4x4(type) ( (type ==D_L0_4x4)||(type ==D_L1_4x4)||(type ==D_BI_4x4))// 4*4		||�߼���,ֻҪ��һ��Ϊtrue�������Ϊtrue��������������typeֻ������һ�֣����Ժ����==�ж�ֻ��һ��������true��Ҳ������������������������
#define IS_SUB4x8(type) ( (type ==D_L0_4x8)||(type ==D_L1_4x8)||(type ==D_BI_4x8))// 4*8		�Ǹ��������type��һ�����������ö�ٵ����� mb_partition_e
#define IS_SUB8x4(type) ( (type ==D_L0_8x4)||(type ==D_L1_8x4)||(type ==D_BI_8x4))// 8*4
#define IS_SUB8x8(type) ( (type ==D_L0_8x8)||(type ==D_L1_8x8)||(type ==D_BI_8x8)||(type ==D_DIRECT_8x8))// 8*8
/*
�����ǽ�Mb�ָ�Ŀ��о�
*/
enum mb_partition_e	//���ķָ�//http://wmnmtm.blog.163.com/blog/static/382457142011773583805/
{
    /* sub partition type for P_8x8 and B_8x8 (��������P_8x8 and B_8x8)*/
    D_L0_4x4        = 0,
    D_L0_8x4        = 1,
    D_L0_4x8        = 2,
    D_L0_8x8        = 3,

    /* sub partition type for B_8x8 only (ֻ����B_8x8)*/
    D_L1_4x4        = 4,
    D_L1_8x4        = 5,
    D_L1_4x8        = 6,
    D_L1_8x8        = 7,

    D_BI_4x4        = 8,
    D_BI_8x4        = 9,
    D_BI_4x8        = 10,
    D_BI_8x8        = 11,
    D_DIRECT_8x8    = 12,

    /* partition */
    D_8x8           = 13,
    D_16x8          = 14,
    D_8x16          = 15,
    D_16x16         = 16,
};

/*
����������ö�����йصģ������ȡֵ0-16����17�������������ά�����һ���ռ䳤��������17

*/
static const int x264_mb_partition_listX_table[2][17] =
{{
    1, 1, 1, 1, /* D_L0_* */
    0, 0, 0, 0, /* D_L1_* */
    1, 1, 1, 1, /* D_BI_* */
    0,          /* D_DIRECT_8x8 */
    0, 0, 0, 0  /* 8x8 .. 16x16 */
},
{
    0, 0, 0, 0, /* D_L0_* */
    1, 1, 1, 1, /* D_L1_* */
    1, 1, 1, 1, /* D_BI_* */
    0,          /* D_DIRECT_8x8 */
    0, 0, 0, 0  /* 8x8 .. 16x16 */
}};
static const int x264_mb_partition_count_table[17] =
{
    /* sub L0 */
    4, 2, 2, 1,
    /* sub L1 */
    4, 2, 2, 1,
    /* sub BI */
    4, 2, 2, 1,
    /* Direct */
    1,
    /* Partition */
    4, 2, 2, 1
};
static const int x264_mb_partition_pixel_table[17] =
{
    6, 4, 5, 3, 6, 4, 5, 3, 6, 4, 5, 3, 3, 3, 1, 2, 0
};

/* zigzags(�����) are transposed(1. ʹ��λ2. �任˳��3. <��>����) with respect(����) to the tables(������) in the standard(��׼) */
static const int x264_zigzag_scan4[16] =
{
    0,  4,  1,  2,  5,  8, 12,  9,  6,  3,  7, 10, 13, 14, 11, 15
};
/*
4x4��
	0	1	2	3

	4	5	6	7

	8	9	10	11

	12	13	14	15

	����һ�Ƚϣ����Կ�����һ�־����ɨ�裬����ʼ

*/

/*
��һ��һά���飬����������������Ԫ�������˳�����ζ���������ʵ���˾����ɨ��
*/
static const int x264_zigzag_scan8[64] =
{
    0,  8,  1,  2,  9, 16, 24, 17, 10,  3,  4, 11, 18, 25, 32, 40,
   33, 26, 19, 12,  5,  6, 13, 20, 27, 34, 41, 48, 56, 49, 42, 35,
   28, 21, 14,  7, 15, 22, 29, 36, 43, 50, 57, 58, 51, 44, 37, 30,
   23, 31, 38, 45, 52, 59, 60, 53, 46, 39, 47, 54, 61, 62, 55, 63
};

/*	
8x8��(���濴��һ��һά����)
	0	1	2	3	4	5	6	7	

	8	9	10	11	12	13	14	15

	16	17	18	19	20	21	22	23

	24	25	26	27	28	29	30	31

	32	33	34	35	36	37	38	39

	40	41	42	43	44	45	46	47

	48	49	50	51	52	53	54	55

	56	57	58	59	60	61	62	63
*/

/*
x����
*/
static const uint8_t block_idx_x[16] =
{
    0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3
};
/*
Y����
*/
static const uint8_t block_idx_y[16] =
{
    0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3
};

/*
xy
�����������Ժ��������������ϵ��������ƽ��������һά�����һ�ֶ�Ӧ��������ʲô�أ�
һ�������Է�Ϊ4*4=16�� 4x4�飬���������Ǳ��Ǹ�˳���
[�Ϻ�ܣ���108ҳ]DCTֱ��ϵ���ı任��������һ��ͼ5.49 ��������ȿ���������ں����λ�õĶ�Ӧ��ϵ
������������ͼ5.49�Ĺ�ϵ�����沩��
http://wmnmtm.blog.163.com/blog/static/3824571420117815650365/

*/
static const uint8_t block_idx_xy[4][4] =
{
    { 0, 2, 8,  10 },
    { 1, 3, 9,  11 },
    { 4, 6, 12, 14 },
    { 5, 7, 13, 15 }
};

/*
��׼���� 8-15����ΪqPI������QPC�Ĺ淶
qPI  <30 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51
QPC =qPI 29 30 31 32 32 33 34 34 35 35 36 36 37 37 37 38 38 38 39 39 39 39
*/
static const int i_chroma_qp_table[52] =		//ɫ��������
{
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9,		/* 0-9 */
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,		/* 10-19 */
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,		/* 20-29 */
    29, 30, 31, 32, 32, 33, 34, 34, 35, 35,
    36, 36, 37, 37, 37, 38, 38, 38, 39, 39,
    39, 39
};//29��ʼ���������QPCһ��

/*
ctxBlockCat
��׼��251/341:�� 9-33����ͬ��ctxBlockCat�Ĺ涨
*/
enum cabac_ctx_block_cat_e
{
    DCT_LUMA_DC   = 0,
    DCT_LUMA_AC   = 1,
    DCT_LUMA_4x4  = 2,
    DCT_CHROMA_DC = 3,
    DCT_CHROMA_AC = 4,
    DCT_LUMA_8x8  = 5,
};


int  x264_macroblock_cache_init( x264_t *h );////����x264_t�ṹ�����ӽṹ��mb��Ӧ��qp��cbp��skipbp��mb_transform_size��intra4x4_pred_mode��non_zero_count�ȵ��ں�����ʱ���ڿ��ƺʹ�����õ��ı�
void x264_macroblock_slice_init( x264_t *h );//
void x264_macroblock_cache_load( x264_t *h, int i_mb_x, int i_mb_y );//���ǽ�Ҫ����ĺ�����Χ�ĺ���ֵ������, Ҫ��õ���ǰ���Ԥ��ֵ��Ҫ��֪�����棬�����Ԥ��ֵ�����ο�֡�ı�����Ϣ�洢��h->mb.cache�У����Է���ʹ��
void x264_macroblock_cache_save( x264_t *h );
void x264_macroblock_cache_end( x264_t *h );

void x264_macroblock_bipred_init( x264_t *h );

/* x264_mb_predict_mv_16x16:
 *      set mvp with predicted mv for D_16x16 block
 *      h->mb. need only valid values from other blocks */
void x264_mb_predict_mv_16x16( x264_t *h, int i_list, int i_ref, int mvp[2] );
/* x264_mb_predict_mv_pskip:
 *      set mvp with predicted mv for P_SKIP
 *      h->mb. need only valid values from other blocks */
void x264_mb_predict_mv_pskip( x264_t *h, int mv[2] );
/* x264_mb_predict_mv:
 *      set mvp with predicted mv for all blocks except SKIP and DIRECT
 *      h->mb. need valid ref/partition/sub of current block to be valid
 *      and valid mv/ref from other blocks. */
void x264_mb_predict_mv( x264_t *h, int i_list, int idx, int i_width, int mvp[2] );
/* x264_mb_predict_mv_direct16x16:
 *      set h->mb.cache.mv and h->mb.cache.ref for B_SKIP or B_DIRECT
 *      h->mb. need only valid values from other blocks.
 *      return 1 on success, 0 on failure.
 *      if b_changed != NULL, set it to whether refs or mvs differ from
 *      before this functioncall. */
int x264_mb_predict_mv_direct16x16( x264_t *h, int *b_changed );
/* x264_mb_load_mv_direct8x8:
 *      set h->mb.cache.mv and h->mb.cache.ref for B_DIRECT
 *      must be called only after x264_mb_predict_mv_direct16x16 */
void x264_mb_load_mv_direct8x8( x264_t *h, int idx );
/* x264_mb_predict_mv_ref16x16:
 *      set mvc with D_16x16 prediction.
 *      uses all neighbors, even those that didn't end up using this ref.
 *      h->mb. need only valid values from other blocks */
void x264_mb_predict_mv_ref16x16( x264_t *h, int i_list, int i_ref, int mvc[8][2], int *i_mvc );


int  x264_mb_predict_intra4x4_mode( x264_t *h, int idx );
int  x264_mb_predict_non_zero_code( x264_t *h, int idx );

/* x264_mb_transform_8x8_allowed:
 *      check whether any partition is smaller than 8x8 (or at least
 *      might be, according to just partition type.)
 *      doesn't check for intra or cbp */
int  x264_mb_transform_8x8_allowed( x264_t *h );

void x264_mb_encode_i4x4( x264_t *h, int idx, int i_qscale );
void x264_mb_encode_i8x8( x264_t *h, int idx, int i_qscale );

void x264_mb_mc( x264_t *h );
void x264_mb_mc_8x8( x264_t *h, int i8 );


static inline void x264_macroblock_cache_ref( x264_t *h, int x, int y, int width, int height, int i_list, int ref )
{
    int dy, dx;
    for( dy = 0; dy < height; dy++ )
    {
        for( dx = 0; dx < width; dx++ )
        {
            h->mb.cache.ref[i_list][X264_SCAN8_0+x+dx+8*(y+dy)] = ref;
        }
    }
}
static inline void x264_macroblock_cache_mv( x264_t *h, int x, int y, int width, int height, int i_list, int mvx, int mvy )
{
    int dy, dx;
    for( dy = 0; dy < height; dy++ )
    {
        for( dx = 0; dx < width; dx++ )
        {
            h->mb.cache.mv[i_list][X264_SCAN8_0+x+dx+8*(y+dy)][0] = mvx;
            h->mb.cache.mv[i_list][X264_SCAN8_0+x+dx+8*(y+dy)][1] = mvy;
        }
    }
}
static inline void x264_macroblock_cache_mvd( x264_t *h, int x, int y, int width, int height, int i_list, int mdx, int mdy )
{
    int dy, dx;
    for( dy = 0; dy < height; dy++ )
    {
        for( dx = 0; dx < width; dx++ )
        {
            h->mb.cache.mvd[i_list][X264_SCAN8_0+x+dx+8*(y+dy)][0] = mdx;
            h->mb.cache.mvd[i_list][X264_SCAN8_0+x+dx+8*(y+dy)][1] = mdy;
        }
    }
}
static inline void x264_macroblock_cache_skip( x264_t *h, int x, int y, int width, int height, int b_skip )
{
    int dy, dx;
    for( dy = 0; dy < height; dy++ )
    {
        for( dx = 0; dx < width; dx++ )
        {
            h->mb.cache.skip[X264_SCAN8_0+x+dx+8*(y+dy)] = b_skip;
        }
    }
}
static inline void x264_macroblock_cache_intra8x8_pred( x264_t *h, int x, int y, int i_mode )
{
    int *cache = &h->mb.cache.intra4x4_pred_mode[X264_SCAN8_0+x+8*y];
    cache[0] = cache[1] = cache[8] = cache[9] = i_mode;
}

#endif

/*
A. ֡��Ԥ�⣺
����H.264��׼�涨��9��֡��4x4���ȷ���Ԥ�⡢4��֡��16xl6���ȷ���Ԥ���Լ�4��֡��8x8ɫ�����Ԥ��ģʽ����Ժ�����ں����ں����� ��ȱʧ�Ĳ�ͬ������ֱ�ֱ�ӵ��ò�ͬ��Ԥ�⺯�����Խ�Լ�߼��жϵ�ʱ�䡣����һ����Ƶ�����׼��P104
B. ֡��Ԥ�⣺
�ڶ�P֡��B֡�ĺ�����Ԥ��֮ǰ�����жϵ�ǰ֡�Ƿ�������֡��ģʽ����������ٽ��ѱ�����
��������֡��ģʽ��������������ڵ�sliceΪp�Ļ����ο�֡��Ӧλ�õĺ��Ҳ������֡��ģʽ�Ļ�����ú�����֡��Ԥ��Ŀ����Ծͺ�С����ô�ڸú����֡��ģʽ�õ�����С��SAD��ֻҪ����֡��16*16Ԥ��ģʽ��SAD����������ȣ�����ֵ��������ֵʱ������֡��Ԥ��ģʽ�������ü���֡��4*4��ģʽ�ˡ�

1. P_SKIPģʽ�����ж��Ƿ���SKIPģʽ����ģʽ���ж������¼���������
��1�����ģʽѡ��ΪInter16��16��
��2��MC�õ��������˶�ʸ������Ԥ���˶�ʸ�������˶�ʸ���Ĳв�Ϊ0��
��3���任ϵ����������Ϊ0��

2.���Ⱥ�黮���Ӻ��Ԥ��ģʽ��ѡ�񣺵����жϲ���skipģʽ��ʱ�򣬿��Ը��������е������������Ƿ���ж�16*16��8*8�ĺ������Ӻ��Ļ��֡�
1),���ȼ���16*16�����˶�ʸ������cost.
2).�����Ӻ��Ļ��֣�����4��8*8�Ӻ����˶�ʸ������Ӧ��cost�ͣ���16*16ģʽ���бȽϣ���16*16��cost��Сʱ����������ָ�С �ĺ�顣���򣬼�����8*8�Ŀ�����·֣�8*4��4*8��4*4���ֱ���бȽϣ��õ���С��cost���������Ӧ����ѵ�Mv.
3).����8*16��16*8ģʽ��cost���������mv��cost���бȽϣ��õ����յ�mv��1or2or4o8or16������

ctxIdx:
H.264��һ��Ƭ�ڿ��ܳ��ֵ����ݻ���Ϊ399��������ģ�ͣ�ÿ��ģ����ctxIdx��ʶ����ÿ��ģ���ڲ����и��ʵĲ��Һ͸��¡�H.264��Ҫ����399�����ʱ�....(�Ϻ�ܣ���120ҳ����������CABAC��������ģ��)
*/