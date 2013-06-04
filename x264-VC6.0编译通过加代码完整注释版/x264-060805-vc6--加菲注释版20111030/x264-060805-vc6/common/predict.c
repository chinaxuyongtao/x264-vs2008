/*****************************************************************************
 * predict.c: h264 encoder
 * ����416��ɫ��8֡��Ԥ��.doc
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: predict.c,v 1.1 2004/06/03 19:27:07 fenrir Exp $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
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

/* predict4x4 are inspired from ffmpeg h264 decoder */


#include "common.h"
#include "clip1.h"

#ifdef _MSC_VER
	#undef HAVE_MMXEXT  /* not finished now */
#endif
#ifdef HAVE_MMXEXT
	#include "i386/predict.h"
#endif

/****************************************************************************
 * 16x16 prediction for intra luma(����) block
 * 16x16 Ԥ�� ֡�����ȿ�

 ****************************************************************************/
/*
 * 16x16���ȿ��֡��Ԥ�ⷽʽ
 * Intra_16x16_DC
 * ��ߺ��ϱ��ڿ������,��ָ��������H��V�����ڣ�
 * ��ǰ������������ֵΪ(H+V)��ƽ��ֵ
 * 
 * ���ϣ��Ϻ�ܣ���205ҳ
 * #define FDEC_STRIDE 32	//STRIDE:���
 * ��������v��16x16���������ؽ��и�ֵ��ѭ��һ�θ�ֵһ��16������(�궨��)
 * ��һ��ȱ���pָ�����32λ�޷������ͱ�����Ҳ������ָ�����src(������ʼλ��)����Ϊ��32λ���Բ��к���һ���ܸ�4�����ظ�ֵ
 * ָ���1 ���ĸ��ֽڼ�32λ ,��һ����Ը��ĸ����ص㸳ֵ��һ������ռһ���ֽ�
 * FDEC_STRIDE=32 �� FENC_STRIDE=16 �Ƕ���ġ�����32��16 ���ֽ����������ݴ洢�ռ��ʹ�ù�������⣩
*/
#define PREDICT_16x16_DC(v) \
    for( i = 0; i < 16; i++ )\
    {\
        uint32_t *p = (uint32_t*)src;\
        *p++ = v;\
        *p++ = v;\
        *p++ = v;\
        *p++ = v;\
        src += FDEC_STRIDE;\
    }

/*
 * 16x16���ȿ��֡��Ԥ�ⷽʽ
 * Intra_16x16_DC
 * �Ϻ����ڿ����ʱ,֡��16*16���ȿ�DCģʽԤ��
 * 
 * ���ϣ��Ϻ�ܣ���205ҳ
 * ���ܣ����������õ�ǰ����Ϸ����󷽹�32�����صĺ͵ľ�ֵ����Ԥ�⣬Ԥ�������16x16�����ص�ֵ�����þ�ֵ���
*/
static void predict_16x16_dc( uint8_t *src )//typedef unsigned char   uint8_t;
{
    uint32_t dc = 0;	//����dcΪ32 λ�޷��������� ��Ŀ��ͬ�ϣ�
    int i;

    for( i = 0; i < 16; i++ )
    {
        dc += src[-1 + i * FDEC_STRIDE];	//��ǰ���ĵ�i���󷽵�����ֵ
        dc += src[i - FDEC_STRIDE];			//��ʱdcΪ������i���󷽵�����ֵ��+������i ���Ϸ�������ֵ��
    }										//ѭ��16�κ�dcΪ��ǰ����󷽺��Ϸ�����32 �����صĺ�
    dc = (( dc + 16 ) >> 5) * 0x01010101;	//����32������ȡ��ֵ��+16��Ϊ���������룬����5��ʾ����2��5�η���32��* 0x01010101Ϊ��һ���ܸ�4�����ظ�ֵ��
											//������0x12* 0x01010101=0x12121212����windows�Դ��ļ������ɲ鿴������PREDICT_16x16_DC(v)��������������
    PREDICT_16x16_DC(dc);					//���ó�����dc����ø�ֵ����,����16x16�����ص�ֵ��ͬ
}

/*
 * 16x16���ȿ��֡��Ԥ�ⷽʽ
 * Intra_16x16_DC
 * ����ڿ����ʱ,֡��16*16���ȿ�DCģʽԤ��
 * 
 * ���ϣ��Ϻ�ܣ���205ҳ
 * ���ܣ�����������ߵ�16�����صľ�ֵ�Ե�ǰ������Ԥ�⣬Ԥ���ǰ��������16x16�����ص�ֵ�����þ�ֵ���
*/
static void predict_16x16_dc_left( uint8_t *src )
{
    uint32_t dc = 0;
    int i;

    for( i = 0; i < 16; i++ )
    {
        dc += src[-1 + i * FDEC_STRIDE];	//��ǰ���ĵ�i�е�������ص�ֵ
    }										//ѭ��16�κ�dcΪ��ǰ����󷽵�16������ֵ�ĺ�
    dc = (( dc + 8 ) >> 4) * 0x01010101;	//��16������ȡ��ֵ

    PREDICT_16x16_DC(dc);					//��16x16�������ظ�ֵ
}

/*
 * 16x16���ȿ��֡��Ԥ�ⷽʽ
 * Intra_16x16_DC
 * �ϱ��ڿ����ʱ,֡��16*16���ȿ�DCģʽԤ��
 * 
 * ���ϣ��Ϻ�ܣ���205ҳ
 * ���ܣ����������Ϸ�16�����ص�ľ�ֵ���ǵ�ǰ16x16�����������ֵ
*/
static void predict_16x16_dc_top( uint8_t *src )
{
    uint32_t dc = 0;
    int i;

    for( i = 0; i < 16; i++ )
    {
        dc += src[i - FDEC_STRIDE];			//��ǰ����i ���Ϸ�������ֵ
    }
    dc = (( dc + 8 ) >> 4) * 0x01010101;	//���ֵ

    PREDICT_16x16_DC(dc);
}

/*
 * 16x16���ȿ��֡��Ԥ�ⷽʽ
 * Intra_16x16_DC
 * �ڿ��������ʱ,֡��16*16���ȿ�Ԥ��DCģʽ,Ԥ��ֵΪ128
 * 
 * ���ϣ��Ϻ�ܣ���205ҳ
 * ���ܣ��������ù̶�ֵ128��0x80��16x16����������ؽ��и���
*/
static void predict_16x16_dc_128( uint8_t *src )
{
    int i;
    PREDICT_16x16_DC(0x80808080);		//��16x16�����ظ�ֵ
}

/*
֡��16*16���ȿ�ˮƽԤ�� 
ˮƽ��horizontal;level
���ܣ��������õ�ǰ��������ؽ���ˮƽ�����Ԥ�⣨���ĵ�i ����������ֵ����������ߵ�����ֵ��
*/
static void predict_16x16_h( uint8_t *src )
{
    int i;

    for( i = 0; i < 16; i++ )
    {
        const uint32_t v = 0x01010101 * src[-1];	//src[-1]�ǵ�ǰ����i���󷽵�����ֵ( i = 0; i < 16; i++ )
        uint32_t *p = (uint32_t*)src;

        *p++ = v;
        *p++ = v;
        *p++ = v;
        *p++ = v;

        src += FDEC_STRIDE;		//ѭ��һ�Σ�ָ��������һ��

    }							//ѭ��һ��Ԥ��һ�У���ѭ��ʮ����
}

/*
֡��16*16���ȿ鴹ֱԤ��
��ֱ:vertical
���ܣ��������õ�ǰ����Ϸ����ؽ��д�ֱ����Ԥ�⣨���ĵ�i�����ض��������Ϸ�������ֵ��
*/
static void predict_16x16_v( uint8_t *src )
{
	//��Ϊv��32λ�����ֽ��޷��������������Խ�srcָ��������ĸ��ֽڵĵ�Ԫ�ڵ�ֵ������
    uint32_t v0 = *(uint32_t*)&src[ 0-FDEC_STRIDE];	//v0Ϊ��ǰ���Ϸ���0-3������ֵ
    uint32_t v1 = *(uint32_t*)&src[ 4-FDEC_STRIDE];	//v1Ϊ��ǰ���Ϸ���4-7������ֵ
    uint32_t v2 = *(uint32_t*)&src[ 8-FDEC_STRIDE];	//v2Ϊ��ǰ���Ϸ���8-11������ֵ
    uint32_t v3 = *(uint32_t*)&src[12-FDEC_STRIDE];	//v3Ϊ��ǰ���Ϸ���12-15������ֵ
    int i;

    for( i = 0; i < 16; i++ )
    {
        uint32_t *p = (uint32_t*)src;	//pָ��ǰ��ĵ�һ�����ص�Ԫ
        *p++ = v0;						//��v0������ǰ���i�е�0-3������
        *p++ = v1;						//��v1������ǰ���i�е�4-7������
        *p++ = v2;						//��v2������ǰ���i�е�8-11������
        *p++ = v3;						//��v3������ǰ���i�е�12-15������
        src += FDEC_STRIDE;				//ʹsrcָ����һ�п�ʼλ��
    }
}

/*
֡��16*16���ȿ�ƽ��Ԥ��
plane:ƽ��
���ܣ����ض��ļ��㷽���Ժ�����Ԥ��
*/
static void predict_16x16_p( uint8_t *src )
{
    int x, y, i;
    int a, b, c;
    int H = 0;
    int V = 0;
    int i00;

    /* calcule(����, ����) H and V */
    for( i = 0; i <= 7; i++ )//ѭ��8��
    {
        H += ( i + 1 ) * ( src[ 8 + i - FDEC_STRIDE ] - src[6 -i -FDEC_STRIDE] );	//����Ϸ���Ӧλ�õ�16�����ص��������ͣ��õ������Ͻǵĵ���������м�㣩
        V += ( i + 1 ) * ( src[-1 + (8+i)*FDEC_STRIDE] - src[-1 + (6-i)*FDEC_STRIDE] );	//����󷽶�Ӧλ�õ�16�����ص���������
    }

	//���ݹ�ʽ����H��V�Լ�a��b��c��ֵ
    a = 16 * ( src[-1 + 15*FDEC_STRIDE] + src[15 - FDEC_STRIDE] );
    b = ( 5 * H + 32 ) >> 6;
    c = ( 5 * V + 32 ) >> 6;

    i00 = a - b * 7 - c * 7 + 16;

    for( y = 0; y < 16; y++ )
    {
        int pix = i00;
        for( x = 0; x < 16; x++ )	//ѭ��һ�ζ�һ�����ص㸽ֵ
        {
            src[x] = x264_clip_uint8( pix>>5 );
            pix += b;		//-ѭ��һ������( 5 * H + 32 ) >> 6
        }					//Pred(x,j) =clip((a+b(-7+x)+c(-7+j))+16)>>5)��j���������ص�Ԥ��ֵ
        src += FDEC_STRIDE;	//ָ����һ��
        i00 += c;			//-ѭ��һ������( 5 * V + 32 ) >> 6
    }						//Pred(x,y) = clip((a+b(-7+x)+c(-7+y))+16)>>5)   16*16�������ص�Ԥ��ֵ
}
/*
����:A��255=2e8=1111 1111��(~255)= 0xffffff00
a&0xffffff00,���������0,���������
1.a<0,��ô���λ�϶���1,-a��Ϊ����,��λ31���ȻȫΪ0,��0x00000000,���Ϊ0
2.a>0,��ô���λ�϶���0,����a�϶�>255,-a��Ϊ����,��λ31���ȻȫΪ1,��0xffffffff,���Ϊ255
����ͨ��(-a)>>31����������������ж�:if(a<0)return 0;
else if(a>255) return 255;
else return a;
B��a&0xffffff00,�������0,˵��0<a<255,����Ҫ����,ֱ�����.

*/

/****************************************************************************
 * 8x8 prediction for intra chroma block
 * 8x8 Ԥ�� ֡�� ɫ�ȿ�
 ****************************************************************************/

/*
 * ���ܣ�8*8����������ض���128����
 * ���ϣ�
 * ˼·��
 *
 *    * 0 1 2 3 4 5 6 7
 *************************
 *  0 * 
 *  1 * 
 *  2 * 
 *  3 *     ����128
 *  4 * 
 *  5 * 
 *  6 * 
 *  7 * 
 *************************
 *
 *
 *
*/
static void predict_8x8c_dc_128( uint8_t *src )
{
    int y;

    for( y = 0; y < 8; y++ )			//ѭ��8�Σ�һ��Ԥ��һ��
    {
        uint32_t *p = (uint32_t*)src;	//ָ��ָ��Դ����λ��
        *p++ = 0x80808080;				//��128����һ�е�ǰ�ĸ�����
        *p++ = 0x80808080;				//��128����һ�еĺ��ĸ�����
        src += FDEC_STRIDE;				//��λ����һ��
    }
}

/*
 * �����ؿ���ߵ�һ�����ضԵ�ǰ8*8�����Ԥ��
 *
 *    * 0 1 2 3 4 5 6 7
 *************************
 *  I * 
 *  J * 
 *  K * 
 *  L *   (I+..+P)/8
 *  M * 
 *  N * 
 *  O * 
 *  P * 
 *************************
 *
*/
static void predict_8x8c_dc_left( uint8_t *src )
{
    int y;
    uint32_t dc0 = 0, dc1 = 0;

    for( y = 0; y < 4; y++ )					//ѭ��4�����dc0��dc1
    {
        dc0 += src[y * FDEC_STRIDE     - 1];
        dc1 += src[(y+4) * FDEC_STRIDE - 1];
    }
    dc0 = (( dc0 + 2 ) >> 2)*0x01010101;		//ʹ��DC0һ���ܹ�����4������
    dc1 = (( dc1 + 2 ) >> 2)*0x01010101;		//ʹ��DC1һ���ܹ�����4������

    for( y = 0; y < 4; y++ )					//ѭ���Ĵεõ���ǰ��ǰ���е�Ԥ��ֵ��һ����8������
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = dc0;								//Ԥ��һ����ǰ4������
        *p++ = dc0;								//Ԥ��һ���к�4������
        src += FDEC_STRIDE;						//����һ��
    }
    for( y = 0; y < 4; y++ )					//ѭ���Ĵεõ���ǰ������е�Ԥ��ֵ
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = dc1;
        *p++ = dc1;
        src += FDEC_STRIDE;
    }

}

/*
 * ���ܣ������ؿ��ϱߵ�һ�����ضԵ�ǰ�����Ԥ��
 *
 *
 *    * 0 1 2 3 4 5 6 7
 *************************
 *  I * 
 *  J * 
 *  K * 
 *  L *   (0+..+7)/8
 *  M * 
 *  N * 
 *  O * 
 *  P * 
 *************************
 *************************
 * ֱ��Ԥ�⣬���ϱߵ�һ�����ص�ƽ��ֵԤ��������?
 *
*/
static void predict_8x8c_dc_top( uint8_t *src )
{
    int y, x;
    uint32_t dc0 = 0, dc1 = 0;

    for( x = 0; x < 4; x++ )				//ѭ���Ĵεõ�dc0��dc1��ֵ
    {
        dc0 += src[x     - FDEC_STRIDE];	//
        dc1 += src[x + 4 - FDEC_STRIDE];	//
    }
    dc0 = (( dc0 + 2 ) >> 2)*0x01010101;	//dc0=��ǰ���ϱ�һ��������ǰ�ĸ����صľ�ֵ
    dc1 = (( dc1 + 2 ) >> 2)*0x01010101;	//dc1=��ǰ���ϱ�һ�������к��ĸ����صľ�ֵ

    for( y = 0; y < 8; y++ )				//ѭ��8�εõ���ǰ���8������ֵ
    {
        uint32_t *p = (uint32_t*)src;		//
        *p++ = dc0;							//ǰ���о�=dc0
        *p++ = dc1;							//�����о�=dc1
        src += FDEC_STRIDE;
    }
}

/*
 * �õ�ǰ���Ϸ�һ�к���һ���������������Ե�ǰ8*8��Ԥ��
 *
 *
 *
*/
static void predict_8x8c_dc( uint8_t *src )
{
    int y;
    int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    uint32_t dc0, dc1, dc2, dc3;
    int i;

    /*
          s0 s1
       s2
       s3
	   ����λ������

	�ο�word�ĵ�

    */
    for( i = 0; i < 4; i++ )
    {
        s0 += src[i - FDEC_STRIDE];
        s1 += src[i + 4 - FDEC_STRIDE];
        s2 += src[-1 + i * FDEC_STRIDE];
        s3 += src[-1 + (i+4)*FDEC_STRIDE];
    }
    /*
       dc0 dc1
       dc2 dc3
	   ����Ԥ��������ֵ����
     */
    dc0 = (( s0 + s2 + 4 ) >> 3)*0x01010101;
    dc1 = (( s1 + 2 ) >> 2)*0x01010101;
    dc2 = (( s3 + 2 ) >> 2)*0x01010101;
    dc3 = (( s1 + s3 + 4 ) >> 3)*0x01010101;

    for( y = 0; y < 4; y++ )			//ѭ���Ĵεõ�ǰ���е�����ֵ
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = dc0;						//8*8���е�һ��4*4���ֵ=dc0
        *p++ = dc1;						//8*8���еڶ���4*4���ֵ=dc1
        src += FDEC_STRIDE;
    }

    for( y = 0; y < 4; y++ )			//ѭ���Ĵεõ������е�����ֵ
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = dc2;						//8*8���е�����4*4�������ֵ
        *p++ = dc3;						//8*8���е��ĸ�4*4�������ֵ
        src += FDEC_STRIDE;
    }
}

/*
 * ���ܣ��õ�ǰ�����һ�����ضԵ�ǰ��Ԥ��
 *
 *
 * Src  * ��ǰ8*8�飬��8����ʾ
 * *****************************************************************
 * -1	* 0 1 2 3 4 5 6 7 8 
 * *****************************************************************
 *  0   * 0 0 0 0 0 0 0 0 0	
 *  1   * 1 1 1 1 1 1 1 1 1
 *  2   * 2 2 2 2 2 2 2 2 2
 *  3   * 3 3 3 3 3 3 3 3 3
 *  4   * 4 4 4 4 4 4 4 4 4
 *  5   * 5 5 5 5 5 5 5 5 5
 *  6   * 6 6 6 6 6 6 6 6 6
 *  7   * 7 7 7 7 7 7 7 7 7
 * *****************************************************************
 * ͬһ����8�����ص��ֵ����ͬ�����ڿ���߶�Ӧ�����ڵ�����ֵ
*/
static void predict_8x8c_h( uint8_t *src )
{
    int i;

    for( i = 0; i < 8; i++ )				//ѭ��8����ɶ�8������ֵ��Ԥ��
    {
        uint32_t v = 0x01010101 * src[-1];
        uint32_t *p = (uint32_t*)src;
        *p++ = v;
        *p++ = v;
        src += FDEC_STRIDE;
    }
}

/*
 * 
 * �õ�ǰ�������һ�����ضԵ�ǰ��Ԥ��

 *      ����������������
 *    * 0 1 2 3 4 5 6 7
 ***********************
 *  0 * 0 1 2 3 4 5 6 7
 *  1 * 0 1 2 3 4 5 6 7
 *  2 * 0 1 2 3 4 5 6 7
 *  3 * 0 1 2 3 4 5 6 7
 *  4 * 0 1 2 3 4 5 6 7
 *  5 * 0 1 2 3 4 5 6 7
 *  6 * 0 1 2 3 4 5 6 7
 *  7 * 0 1 2 3 4 5 6 7
 * 
*/
static void predict_8x8c_v( uint8_t *src )
{
    uint32_t v0 = *(uint32_t*)&src[0-FDEC_STRIDE];		//һ��ȡ��ǰ�ĸ��ο�����
    uint32_t v1 = *(uint32_t*)&src[4-FDEC_STRIDE];		//ȡ�����ĸ��ο�����
    int i;

    for( i = 0; i < 8; i++ )					//ѭ��8����ɶԵ�ǰ��8������ֵ��Ԥ��(����ͼ)
    {
        uint32_t *p = (uint32_t*)src;			//
        *p++ = v0;								//ѭ��һ�ζ�һ�е�ǰ�ĸ����ظ�ֵ
        *p++ = v1;								//ѭ��һ�ζ�һ�еĺ��ĸ����ظ�ֵ
        src += FDEC_STRIDE;						//
    }
}

/*
 * 
 * ���ܣ��õ�ǰ������һ�����غ��ϱ�һ���������������Ե�ǰ��Ԥ��,��16*16����

 * -1 *  0  1  2  3  4  5  6  7
 ******************************
 *  0 *
 *  1 *
 *  2 *
 *  3 *        8x8��
 *  4 *
 *  5 *
 *  6 *
 *  7 *

*/
static void predict_8x8c_p( uint8_t *src )
{
    int i;
    int x,y;
    int a, b, c;
    int H = 0;
    int V = 0;
    int i00;

    for( i = 0; i < 4; i++ )
    {
        H += ( i + 1 ) * ( src[4+i - FDEC_STRIDE] - src[2 - i -FDEC_STRIDE] );			//H=1*��4-2��+2*(5-1)+3*(6-0)+4*(7--1)
        V += ( i + 1 ) * ( src[-1 +(i+4)*FDEC_STRIDE] - src[-1+(2-i)*FDEC_STRIDE] );	//V=1*��4-2��+2*(5-1)+3*(6-0)+4*(7--1)
    }

    a = 16 * ( src[-1+7*FDEC_STRIDE] + src[7 - FDEC_STRIDE] );	//a=16*(7+7)
    b = ( 17 * H + 16 ) >> 5;	//b=(17*H+16)/32
    c = ( 17 * V + 16 ) >> 5;	//c=(17*V+16)/32
    i00 = a -3*b -3*c + 16;

    for( y = 0; y < 8; y++ )
    {
        int pix = i00;
        for( x = 0; x < 8; x++ )	//predC[ x, y ] = Clip1C( ( a + b * ( x - 3 - xCF ) + c * ( y - 3 - yCF ) + 16 ) >> 5 ),
        {
            src[x] = x264_clip_uint8( pix>>5 );
            pix += b;
        }
        src += FDEC_STRIDE;
        i00 += c;
    }
}

/****************************************************************************
 * 4x4 prediction for intra luma block
 * 4x4 Ԥ�� ֡�� ���ȿ�
 ****************************************************************************/
/*
 * 
 * ���ܣ���������4*4���ÿ�����ظ�ͬ����ֵ
 * ����һ�е�4�����ظ�ֵ
 * ���ڶ��е�4�����ظ�ֵ
 * �������е�4�����ظ�ֵ
 * �������е�4�����ظ�ֵ
*/
#define PREDICT_4x4_DC(v) \
{\
    *(uint32_t*)&src[0*FDEC_STRIDE] =\
    *(uint32_t*)&src[1*FDEC_STRIDE] =\
    *(uint32_t*)&src[2*FDEC_STRIDE] =\
    *(uint32_t*)&src[3*FDEC_STRIDE] = v;\
}

/*
 *
 * ���ܣ���������4*4������е㸳ֵ128
*/
static void predict_4x4_dc_128( uint8_t *src )
{
    PREDICT_4x4_DC(0x80808080);
}

/*
 *
 *
 * ���ܣ��������ú������ĸ����أ�I��J��K��L���ľ�ֵ���������ظ���
*/
static void predict_4x4_dc_left( uint8_t *src )
{
    uint32_t dc = (( src[-1+0*FDEC_STRIDE] + src[-1+FDEC_STRIDE]+
                     src[-1+2*FDEC_STRIDE] + src[-1+3*FDEC_STRIDE] + 2 ) >> 2)*0x01010101;
    PREDICT_4x4_DC(dc);
}

/*
 *
 * ���ܣ��������ú���Ϸ�4�����أ�A��B��C��D���ľ�ֵ���������ظ���
*/
static void predict_4x4_dc_top( uint8_t *src )
{
    uint32_t dc = (( src[0 - FDEC_STRIDE] + src[1 - FDEC_STRIDE] +
                     src[2 - FDEC_STRIDE] + src[3 - FDEC_STRIDE] + 2 ) >> 2)*0x01010101;
    PREDICT_4x4_DC(dc);
}

/*
 * 
 * ���ܣ�����������ߺ��ϱ߹�8�����أ�I��J��K��L ��A��B��C��D���ľ�ֵ���������ظ���
*/
static void predict_4x4_dc( uint8_t *src )
{
    uint32_t dc = (( src[-1+0*FDEC_STRIDE] + src[-1+FDEC_STRIDE] +
                     src[-1+2*FDEC_STRIDE] + src[-1+3*FDEC_STRIDE] +
                     src[0 - FDEC_STRIDE]  + src[1 - FDEC_STRIDE] +
                     src[2 - FDEC_STRIDE]  + src[3 - FDEC_STRIDE] + 4 ) >> 3)*0x01010101;
    PREDICT_4x4_DC(dc);
}

/*
 *
 * ���ܣ���������ÿ����ߵ����ض��н��и��ǣ�һ�е���������ֵ��ͬ��
*/
static void predict_4x4_h( uint8_t *src )
{
    *(uint32_t*)&src[0*FDEC_STRIDE] = src[0*FDEC_STRIDE-1] * 0x01010101;
    *(uint32_t*)&src[1*FDEC_STRIDE] = src[1*FDEC_STRIDE-1] * 0x01010101;
    *(uint32_t*)&src[2*FDEC_STRIDE] = src[2*FDEC_STRIDE-1] * 0x01010101;
    *(uint32_t*)&src[3*FDEC_STRIDE] = src[3*FDEC_STRIDE-1] * 0x01010101;
}

/*
 * 
 * ���ܣ���������ÿ���Ϸ������ض��н��и��ǣ�һ�е���������ֵ��ͬ��
 *
 * 0 (Vertical)
 * *********************
 * M A B C D E F G H
 * I v v v v 
 * J v v v v
 * K v v v v 
 * L v v v v
*/
static void predict_4x4_v( uint8_t *src )
{
    uint32_t top = *((uint32_t*)&src[-FDEC_STRIDE]);	//ȡ����ǰ���Ϸ�4�����ص��ֵ
    PREDICT_4x4_DC(top);	//ÿ�о��øղ�ȡ����ֵ����
}

/*
 *
 * ���ܣ��ö����ʾ����ȡ����ǰ�����ߵ�4������
 * l0�ǵ�һ���������I
 * l1�ǵڶ����������J
 * l2�ǵ������������K
 * l3�ǵ������������L
 * 
*/
#define PREDICT_4x4_LOAD_LEFT \
    const int l0 = src[-1+0*FDEC_STRIDE];   \
    const int l1 = src[-1+1*FDEC_STRIDE];   \
    const int l2 = src[-1+2*FDEC_STRIDE];   \
    UNUSED const int l3 = src[-1+3*FDEC_STRIDE];

/*
 * 
 * ���ܣ��ö����ʾ����ȡ����ǰ����Ϸ���4������
 * t0�ǵ�һ���Ϸ�������A
 * t1�ǵڶ����Ϸ�������B
 * t2�ǵ������Ϸ�������C
 * t3�ǵ������Ϸ�������D
 * 
*/
#define PREDICT_4x4_LOAD_TOP \
    const int t0 = src[0-1*FDEC_STRIDE];   \
    const int t1 = src[1-1*FDEC_STRIDE];   \
    const int t2 = src[2-1*FDEC_STRIDE];   \
    UNUSED const int t3 = src[3-1*FDEC_STRIDE];

/*
 * 
 * ���ܣ��ö����ʾ����ȡ����ǰ������Ϸ���4������
 * t4�����Ϸ���һ������E
 * t5�����Ϸ��ڶ�������F
 * t6�����Ϸ�����������G
 * t7�����Ϸ����ĸ�����H
 * 
*/
#define PREDICT_4x4_LOAD_TOP_RIGHT \
    const int t4 = src[4-1*FDEC_STRIDE];   \
    const int t5 = src[5-1*FDEC_STRIDE];   \
    const int t6 = src[6-1*FDEC_STRIDE];   \
    UNUSED const int t7 = src[7-1*FDEC_STRIDE];

/*
 * ģʽ3 ���¶Խ�Ԥ��
 * ���ܣ�45�����������¸���Ԥ��
 * 
 * 3 ()
 * *********************
 * M A B C D E F G H
 * I  
 * J 
 * K  
 * L 
 * 
 * 
 * 
*/
static void predict_4x4_ddl( uint8_t *src )
{
    PREDICT_4x4_LOAD_TOP			//A��B��C��D
    PREDICT_4x4_LOAD_TOP_RIGHT		//E��F��G��H

    src[0*FDEC_STRIDE+0] = ( t0 + 2*t1 + t2 + 2 ) >> 2;		//a=��A+2B+C+2��/4
															//+2��ʾ��������
    src[0*FDEC_STRIDE+1] =
    src[1*FDEC_STRIDE+0] = ( t1 + 2*t2 + t3 + 2 ) >> 2;		//b=e=��B+2C+D+2��/4

    src[0*FDEC_STRIDE+2] =
    src[1*FDEC_STRIDE+1] =
    src[2*FDEC_STRIDE+0] = ( t2 + 2*t3 + t4 + 2 ) >> 2;		//c=f=i=��C+2D+E+2��/4

    src[0*FDEC_STRIDE+3] =
    src[1*FDEC_STRIDE+2] =
    src[2*FDEC_STRIDE+1] =
    src[3*FDEC_STRIDE+0] = ( t3 + 2*t4 + t5 + 2 ) >> 2;		//d=g=j=m=��D+2E+F+2��/4

    src[1*FDEC_STRIDE+3] =
    src[2*FDEC_STRIDE+2] =
    src[3*FDEC_STRIDE+1] = ( t4 + 2*t5 + t6 + 2 ) >> 2;		//h=k=n=��E+2F+G+2��/4

    src[2*FDEC_STRIDE+3] =
    src[3*FDEC_STRIDE+2] = ( t5 + 2*t6 + t7 + 2 ) >> 2;		//i=o=��F+2G+H+2��/4

    src[3*FDEC_STRIDE+3] = ( t6 + 3*t7 + 2 ) >> 2;			//p=��G+3H+2��/4
}

/*
 * ģʽ4 ���¶Խ�Ԥ��
 * ���ܣ�45�����������¸���Ԥ��
 *
 *
 * 4 ()
 * *********************
 * M A B C D E F G H
 * I a b c d
 * J e f g h
 * K i j k l
 * L m n o p
 *
*/
static void predict_4x4_ddr( uint8_t *src )
{
    const int lt = src[-1-FDEC_STRIDE];		//lt=M
    PREDICT_4x4_LOAD_LEFT					//I��J��K��L
    PREDICT_4x4_LOAD_TOP					//A��B��C��D

    src[0*FDEC_STRIDE+0] =
    src[1*FDEC_STRIDE+1] =
    src[2*FDEC_STRIDE+2] =
    src[3*FDEC_STRIDE+3] = ( t0 + 2 * lt + l0 + 2 ) >> 2;	//a=f=k=p=��A+2M+I+2��/4

    src[0*FDEC_STRIDE+1] =
    src[1*FDEC_STRIDE+2] =
    src[2*FDEC_STRIDE+3] = ( lt + 2 * t0 + t1 + 2 ) >> 2;	//b=g=l=��M+2A+B+2��/4

    src[0*FDEC_STRIDE+2] =
    src[1*FDEC_STRIDE+3] = ( t0 + 2 * t1 + t2 + 2 ) >> 2;	//c=h=��A+2B+C+2��/4

    src[0*FDEC_STRIDE+3] = ( t1 + 2 * t2 + t3 + 2 ) >> 2;	//d=��B+2C+D+2��/4

    src[1*FDEC_STRIDE+0] =
    src[2*FDEC_STRIDE+1] =
    src[3*FDEC_STRIDE+2] = ( lt + 2 * l0 + l1 + 2 ) >> 2;	//e=j=o=��M+2I+J+2��/4

    src[2*FDEC_STRIDE+0] =
    src[3*FDEC_STRIDE+1] = ( l0 + 2 * l1 + l2 + 2 ) >> 2;	//i=n=��I+2J+K+2��/4

    src[3*FDEC_STRIDE+0] = ( l1 + 2 * l2 + l3 + 2 ) >> 2;	//m=��J+2K+L+2��/4
}

/*
 * ģʽ5 ��ֱ���½�
 * 
 * ���ܣ���y�н�26.6�����������¸���Ԥ�� ��û�õ�L��
 * 
 *
 *
 *
 *
 *
 *
*/
static void predict_4x4_vr( uint8_t *src )
{
    const int lt = src[-1-FDEC_STRIDE];		//M
    PREDICT_4x4_LOAD_LEFT					//I��J��K��L
    PREDICT_4x4_LOAD_TOP					//A��B��C��D

    src[0*FDEC_STRIDE+0]=
    src[2*FDEC_STRIDE+1]= ( lt + t0 + 1 ) >> 1;	//a=j=��M+A+1��/2

    src[0*FDEC_STRIDE+1]=
    src[2*FDEC_STRIDE+2]= ( t0 + t1 + 1 ) >> 1;	//e=k=��A+B+1��/2

    src[0*FDEC_STRIDE+2]=
    src[2*FDEC_STRIDE+3]= ( t1 + t2 + 1 ) >> 1;	//c=i=��B+C+1��/2

    src[0*FDEC_STRIDE+3]= ( t2 + t3 + 1 ) >> 1;	//d=��C+D+1��/2

    src[1*FDEC_STRIDE+0]=
    src[3*FDEC_STRIDE+1]= ( l0 + 2 * lt + t0 + 2 ) >> 2;	//e=n=��I+2M+A+2��/4

    src[1*FDEC_STRIDE+1]=
    src[3*FDEC_STRIDE+2]= ( lt + 2 * t0 + t1 + 2 ) >> 2;	//f=o=��M+2A+B+2��/4

    src[1*FDEC_STRIDE+2]=
    src[3*FDEC_STRIDE+3]= ( t0 + 2 * t1 + t2 + 2) >> 2;		//g=p=��A+2B+C+2��/4

    src[1*FDEC_STRIDE+3]= ( t1 + 2 * t2 + t3 + 2 ) >> 2;	//h=��B+2C+D+2��/4
    src[2*FDEC_STRIDE+0]= ( lt + 2 * l0 + l1 + 2 ) >> 2;	//i=��M+2I+J+2��/4
    src[3*FDEC_STRIDE+0]= ( l0 + 2 * l1 + l2 + 2 ) >> 2;	//m=��I+2J+K+2��/4
}

/*
 * ģʽ6 ˮƽб�½�
 * 
 * ���ܣ���x�н�26.6�����������¸���Ԥ�⣨û�õ�D��
 *
 * 
 * 
 * 
 * 
 * 
 * 
 * 
*/
static void predict_4x4_hd( uint8_t *src )
{
    const int lt= src[-1-1*FDEC_STRIDE];	//M
    PREDICT_4x4_LOAD_LEFT					//I��J��K��L
    PREDICT_4x4_LOAD_TOP					//A��B��C��D

    src[0*FDEC_STRIDE+0]=
    src[1*FDEC_STRIDE+2]= ( lt + l0 + 1 ) >> 1;		//a=g=��M+I+1��/2
    src[0*FDEC_STRIDE+1]=
    src[1*FDEC_STRIDE+3]= ( l0 + 2 * lt + t0 + 2 ) >> 2;	//b=h=��I+2M+A+2��/4
    src[0*FDEC_STRIDE+2]= ( lt + 2 * t0 + t1 + 2 ) >> 2;	//c=��M+2A+B+2��/4
    src[0*FDEC_STRIDE+3]= ( t0 + 2 * t1 + t2 + 2 ) >> 2;	//d=��A+2B+C+2��/4
    src[1*FDEC_STRIDE+0]=
    src[2*FDEC_STRIDE+2]= ( l0 + l1 + 1 ) >> 1;				//e=k=��I+J+1��/2
    src[1*FDEC_STRIDE+1]=
    src[2*FDEC_STRIDE+3]= ( lt + 2 * l0 + l1 + 2 ) >> 2;	//f=l=��M+2I+J+2��/4
    src[2*FDEC_STRIDE+0]=
    src[3*FDEC_STRIDE+2]= ( l1 + l2+ 1 ) >> 1;				//i=o=��J+K+1��/2
    src[2*FDEC_STRIDE+1]=
    src[3*FDEC_STRIDE+3]= ( l0 + 2 * l1 + l2 + 2 ) >> 2;	//j=p=��I+2J+K+2��/4
    src[3*FDEC_STRIDE+0]= ( l2 + l3 + 1 ) >> 1;				//m=��K+L+1��/2
    src[3*FDEC_STRIDE+1]= ( l1 + 2 * l2 + l3 + 2 ) >> 2;	//n=��J+2K+L+2��/4
}

/*
 * ģʽ7 ��ֱ���½�
 * ��y�н�26.6�����������¸���Ԥ�⣨û�õ�H��

*/
static void predict_4x4_vl( uint8_t *src )
{
    PREDICT_4x4_LOAD_TOP							//A��B��C��D
    PREDICT_4x4_LOAD_TOP_RIGHT						//E��F��G��H

    src[0*FDEC_STRIDE+0]= ( t0 + t1 + 1 ) >> 1;		//a=��A+B+1��/2
    src[0*FDEC_STRIDE+1]=
    src[2*FDEC_STRIDE+0]= ( t1 + t2 + 1 ) >> 1;		//b=i=��B+C+1��/2
    src[0*FDEC_STRIDE+2]=
    src[2*FDEC_STRIDE+1]= ( t2 + t3 + 1 ) >> 1;		//c=j=��C+D+1��/2
    src[0*FDEC_STRIDE+3]=
    src[2*FDEC_STRIDE+2]= ( t3 + t4 + 1 ) >> 1;		//d=k=��D+E+1��/2
    src[2*FDEC_STRIDE+3]= ( t4 + t5 + 1 ) >> 1;		//l=��E+F+1��/2
    src[1*FDEC_STRIDE+0]= ( t0 + 2 * t1 + t2 + 2 ) >> 2;	//e=��A+2B+C+2��/4
    src[1*FDEC_STRIDE+1]=
    src[3*FDEC_STRIDE+0]= ( t1 + 2 * t2 + t3 + 2 ) >> 2;	//f=m=��B+2C+D+2��/4
    src[1*FDEC_STRIDE+2]=
    src[3*FDEC_STRIDE+1]= ( t2 + 2 * t3 + t4 + 2 ) >> 2;	//g=n=��C+2D+E+2��/4
    src[1*FDEC_STRIDE+3]=
    src[3*FDEC_STRIDE+2]= ( t3 + 2 * t4 + t5 + 2 ) >> 2;	//h=o=��D+2E+F+2��/4
    src[3*FDEC_STRIDE+3]= ( t4 + 2 * t5 + t6 + 2 ) >> 2;	//p=��E+2F+G+2��/4
}

/*
 * ģʽ8  ˮƽб�Ͻ�
 * ��x�н�26.6�����������ϸ���Ԥ��
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
*/
static void predict_4x4_hu( uint8_t *src )
{
    PREDICT_4x4_LOAD_LEFT									//I��J��K��L

    src[0*FDEC_STRIDE+0]= ( l0 + l1 + 1 ) >> 1;				//a=��I+J+1��/2
    src[0*FDEC_STRIDE+1]= ( l0 + 2 * l1 + l2 + 2 ) >> 2;	//b=��I+2J+K+2��/4

    src[0*FDEC_STRIDE+2]=
    src[1*FDEC_STRIDE+0]= ( l1 + l2 + 1 ) >> 1;				//c=e=��J+K+1��/2

    src[0*FDEC_STRIDE+3]=
    src[1*FDEC_STRIDE+1]= ( l1 + 2*l2 + l3 + 2 ) >> 2;		//d=f=��J+2K+L+2��/4

    src[1*FDEC_STRIDE+2]=
    src[2*FDEC_STRIDE+0]= ( l2 + l3 + 1 ) >> 1;				//g=i=��K+L+1��/2

    src[1*FDEC_STRIDE+3]=
    src[2*FDEC_STRIDE+1]= ( l2 + 2 * l3 + l3 + 2 ) >> 2;	//h=j=��K+3L+2��/4

    src[2*FDEC_STRIDE+3]=
    src[3*FDEC_STRIDE+1]=
    src[3*FDEC_STRIDE+0]=
    src[2*FDEC_STRIDE+2]=
    src[3*FDEC_STRIDE+2]=
    src[3*FDEC_STRIDE+3]= l3;								//k=l=m=n=o=p=L
}

/****************************************************************************
 * 8x8 prediction for intra luma block
 * 8x8 Ԥ�� ֡�� ���ȿ�
 ****************************************************************************/

#define SRC(x,y) src[(x)+(y)*FDEC_STRIDE]
#define PL(y) \
    edge[14-y] = (SRC(-1,y-1) + 2*SRC(-1,y) + SRC(-1,y+1) + 2) >> 2;
#define PT(x) \
    edge[16+x] = (SRC(x-1,-1) + 2*SRC(x,-1) + SRC(x+1,-1) + 2) >> 2;

/*

*/
void x264_predict_8x8_filter( uint8_t *src, uint8_t edge[33], int i_neighbor, int i_filters )
{
    /* edge[7..14] = l7..l0
     * edge[15] = lt
     * edge[16..31] = t0 .. t15
     * edge[32] = t15 */

    int have_lt = i_neighbor & MB_TOPLEFT;
    if( i_filters & MB_LEFT )
    {
        edge[15] = (SRC(-1,0) + 2*SRC(-1,-1) + SRC(0,-1) + 2) >> 2;
        edge[14] = ((have_lt ? SRC(-1,-1) : SRC(-1,0))
                    + 2*SRC(-1,0) + SRC(-1,1) + 2) >> 2;
        PL(1) PL(2) PL(3) PL(4) PL(5) PL(6)
        edge[7] = (SRC(-1,6) + 3*SRC(-1,7) + 2) >> 2;
    }

    if( i_filters & MB_TOP )
    {
        int have_tr = i_neighbor & MB_TOPRIGHT;
        edge[16] = ((have_lt ? SRC(-1,-1) : SRC(0,-1))
                    + 2*SRC(0,-1) + SRC(1,-1) + 2) >> 2;
        PT(1) PT(2) PT(3) PT(4) PT(5) PT(6)
        edge[23] = ((have_tr ? SRC(8,-1) : SRC(7,-1))
                    + 2*SRC(7,-1) + SRC(6,-1) + 2) >> 2;

        if( i_filters & MB_TOPRIGHT )
        {
            if( have_tr )
            {
                PT(8) PT(9) PT(10) PT(11) PT(12) PT(13) PT(14)
                edge[31] =
                edge[32] = (SRC(14,-1) + 3*SRC(15,-1) + 2) >> 2;
            }
            else
            {
                //*(uint64_t*)(edge+24) = SRC(7,-1) * 0x0101010101010101ULL;
				*(uint64_t*)(edge+24) = SRC(7,-1) * 0x0101010101010101uI64; //lsp060515
                edge[32] = SRC(7,-1);
            }
        }
    }
}

#undef PL
#undef PT

#define PL(y) \
    UNUSED const int l##y = edge[14-y];
#define PT(x) \
    UNUSED const int t##x = edge[16+x];
#define PREDICT_8x8_LOAD_TOPLEFT \
    const int lt = edge[15];
#define PREDICT_8x8_LOAD_LEFT \
    PL(0) PL(1) PL(2) PL(3) PL(4) PL(5) PL(6) PL(7)
#define PREDICT_8x8_LOAD_TOP \
    PT(0) PT(1) PT(2) PT(3) PT(4) PT(5) PT(6) PT(7)
#define PREDICT_8x8_LOAD_TOPRIGHT \
    PT(8) PT(9) PT(10) PT(11) PT(12) PT(13) PT(14) PT(15)

#define PREDICT_8x8_DC(v) \
    int y; \
    for( y = 0; y < 8; y++ ) { \
        ((uint32_t*)src)[0] = \
        ((uint32_t*)src)[1] = v; \
        src += FDEC_STRIDE; \
    }

/*

*/
static void predict_8x8_dc_128( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_DC(0x80808080);
}

/*

*/
static void predict_8x8_dc_left( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_LEFT
    const uint32_t dc = ((l0+l1+l2+l3+l4+l5+l6+l7+4) >> 3) * 0x01010101;
    PREDICT_8x8_DC(dc);
}

/*

*/
static void predict_8x8_dc_top( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_TOP
    const uint32_t dc = ((t0+t1+t2+t3+t4+t5+t6+t7+4) >> 3) * 0x01010101;
    PREDICT_8x8_DC(dc);
}

/*

*/
static void predict_8x8_dc( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_LEFT
    PREDICT_8x8_LOAD_TOP
    const uint32_t dc = ((l0+l1+l2+l3+l4+l5+l6+l7
                         +t0+t1+t2+t3+t4+t5+t6+t7+8) >> 4) * 0x01010101;
    PREDICT_8x8_DC(dc);
}

/*

*/
static void predict_8x8_h( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_LEFT
#define ROW(y) ((uint32_t*)(src+y*FDEC_STRIDE))[0] =\
               ((uint32_t*)(src+y*FDEC_STRIDE))[1] = 0x01010101U * l##y
    ROW(0); ROW(1); ROW(2); ROW(3); ROW(4); ROW(5); ROW(6); ROW(7);
#undef ROW
}

/*

*/
static void predict_8x8_v( uint8_t *src, uint8_t edge[33] )
{
    const uint64_t top = *(uint64_t*)(edge+16);
    int y;
    for( y = 0; y < 8; y++ )
        *(uint64_t*)(src+y*FDEC_STRIDE) = top;
}

/*

*/
static void predict_8x8_ddl( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_TOP
    PREDICT_8x8_LOAD_TOPRIGHT
    SRC(0,0)= (t0 + 2*t1 + t2 + 2) >> 2;
    SRC(0,1)=SRC(1,0)= (t1 + 2*t2 + t3 + 2) >> 2;
    SRC(0,2)=SRC(1,1)=SRC(2,0)= (t2 + 2*t3 + t4 + 2) >> 2;
    SRC(0,3)=SRC(1,2)=SRC(2,1)=SRC(3,0)= (t3 + 2*t4 + t5 + 2) >> 2;
    SRC(0,4)=SRC(1,3)=SRC(2,2)=SRC(3,1)=SRC(4,0)= (t4 + 2*t5 + t6 + 2) >> 2;
    SRC(0,5)=SRC(1,4)=SRC(2,3)=SRC(3,2)=SRC(4,1)=SRC(5,0)= (t5 + 2*t6 + t7 + 2) >> 2;
    SRC(0,6)=SRC(1,5)=SRC(2,4)=SRC(3,3)=SRC(4,2)=SRC(5,1)=SRC(6,0)= (t6 + 2*t7 + t8 + 2) >> 2;
    SRC(0,7)=SRC(1,6)=SRC(2,5)=SRC(3,4)=SRC(4,3)=SRC(5,2)=SRC(6,1)=SRC(7,0)= (t7 + 2*t8 + t9 + 2) >> 2;
    SRC(1,7)=SRC(2,6)=SRC(3,5)=SRC(4,4)=SRC(5,3)=SRC(6,2)=SRC(7,1)= (t8 + 2*t9 + t10 + 2) >> 2;
    SRC(2,7)=SRC(3,6)=SRC(4,5)=SRC(5,4)=SRC(6,3)=SRC(7,2)= (t9 + 2*t10 + t11 + 2) >> 2;
    SRC(3,7)=SRC(4,6)=SRC(5,5)=SRC(6,4)=SRC(7,3)= (t10 + 2*t11 + t12 + 2) >> 2;
    SRC(4,7)=SRC(5,6)=SRC(6,5)=SRC(7,4)= (t11 + 2*t12 + t13 + 2) >> 2;
    SRC(5,7)=SRC(6,6)=SRC(7,5)= (t12 + 2*t13 + t14 + 2) >> 2;
    SRC(6,7)=SRC(7,6)= (t13 + 2*t14 + t15 + 2) >> 2;
    SRC(7,7)= (t14 + 3*t15 + 2) >> 2;
}

/*

*/
static void predict_8x8_ddr( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_TOP
    PREDICT_8x8_LOAD_LEFT
    PREDICT_8x8_LOAD_TOPLEFT
    SRC(0,7)= (l7 + 2*l6 + l5 + 2) >> 2;
    SRC(0,6)=SRC(1,7)= (l6 + 2*l5 + l4 + 2) >> 2;
    SRC(0,5)=SRC(1,6)=SRC(2,7)= (l5 + 2*l4 + l3 + 2) >> 2;
    SRC(0,4)=SRC(1,5)=SRC(2,6)=SRC(3,7)= (l4 + 2*l3 + l2 + 2) >> 2;
    SRC(0,3)=SRC(1,4)=SRC(2,5)=SRC(3,6)=SRC(4,7)= (l3 + 2*l2 + l1 + 2) >> 2;
    SRC(0,2)=SRC(1,3)=SRC(2,4)=SRC(3,5)=SRC(4,6)=SRC(5,7)= (l2 + 2*l1 + l0 + 2) >> 2;
    SRC(0,1)=SRC(1,2)=SRC(2,3)=SRC(3,4)=SRC(4,5)=SRC(5,6)=SRC(6,7)= (l1 + 2*l0 + lt + 2) >> 2;
    SRC(0,0)=SRC(1,1)=SRC(2,2)=SRC(3,3)=SRC(4,4)=SRC(5,5)=SRC(6,6)=SRC(7,7)= (l0 + 2*lt + t0 + 2) >> 2;
    SRC(1,0)=SRC(2,1)=SRC(3,2)=SRC(4,3)=SRC(5,4)=SRC(6,5)=SRC(7,6)= (lt + 2*t0 + t1 + 2) >> 2;
    SRC(2,0)=SRC(3,1)=SRC(4,2)=SRC(5,3)=SRC(6,4)=SRC(7,5)= (t0 + 2*t1 + t2 + 2) >> 2;
    SRC(3,0)=SRC(4,1)=SRC(5,2)=SRC(6,3)=SRC(7,4)= (t1 + 2*t2 + t3 + 2) >> 2;
    SRC(4,0)=SRC(5,1)=SRC(6,2)=SRC(7,3)= (t2 + 2*t3 + t4 + 2) >> 2;
    SRC(5,0)=SRC(6,1)=SRC(7,2)= (t3 + 2*t4 + t5 + 2) >> 2;
    SRC(6,0)=SRC(7,1)= (t4 + 2*t5 + t6 + 2) >> 2;
    SRC(7,0)= (t5 + 2*t6 + t7 + 2) >> 2;
  
}

/*

*/
static void predict_8x8_vr( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_TOP
    PREDICT_8x8_LOAD_LEFT
    PREDICT_8x8_LOAD_TOPLEFT
    SRC(0,6)= (l5 + 2*l4 + l3 + 2) >> 2;
    SRC(0,7)= (l6 + 2*l5 + l4 + 2) >> 2;
    SRC(0,4)=SRC(1,6)= (l3 + 2*l2 + l1 + 2) >> 2;
    SRC(0,5)=SRC(1,7)= (l4 + 2*l3 + l2 + 2) >> 2;
    SRC(0,2)=SRC(1,4)=SRC(2,6)= (l1 + 2*l0 + lt + 2) >> 2;
    SRC(0,3)=SRC(1,5)=SRC(2,7)= (l2 + 2*l1 + l0 + 2) >> 2;
    SRC(0,1)=SRC(1,3)=SRC(2,5)=SRC(3,7)= (l0 + 2*lt + t0 + 2) >> 2;
    SRC(0,0)=SRC(1,2)=SRC(2,4)=SRC(3,6)= (lt + t0 + 1) >> 1;
    SRC(1,1)=SRC(2,3)=SRC(3,5)=SRC(4,7)= (lt + 2*t0 + t1 + 2) >> 2;
    SRC(1,0)=SRC(2,2)=SRC(3,4)=SRC(4,6)= (t0 + t1 + 1) >> 1;
    SRC(2,1)=SRC(3,3)=SRC(4,5)=SRC(5,7)= (t0 + 2*t1 + t2 + 2) >> 2;
    SRC(2,0)=SRC(3,2)=SRC(4,4)=SRC(5,6)= (t1 + t2 + 1) >> 1;
    SRC(3,1)=SRC(4,3)=SRC(5,5)=SRC(6,7)= (t1 + 2*t2 + t3 + 2) >> 2;
    SRC(3,0)=SRC(4,2)=SRC(5,4)=SRC(6,6)= (t2 + t3 + 1) >> 1;
    SRC(4,1)=SRC(5,3)=SRC(6,5)=SRC(7,7)= (t2 + 2*t3 + t4 + 2) >> 2;
    SRC(4,0)=SRC(5,2)=SRC(6,4)=SRC(7,6)= (t3 + t4 + 1) >> 1;
    SRC(5,1)=SRC(6,3)=SRC(7,5)= (t3 + 2*t4 + t5 + 2) >> 2;
    SRC(5,0)=SRC(6,2)=SRC(7,4)= (t4 + t5 + 1) >> 1;
    SRC(6,1)=SRC(7,3)= (t4 + 2*t5 + t6 + 2) >> 2;
    SRC(6,0)=SRC(7,2)= (t5 + t6 + 1) >> 1;
    SRC(7,1)= (t5 + 2*t6 + t7 + 2) >> 2;
    SRC(7,0)= (t6 + t7 + 1) >> 1;
}

/*

*/
static void predict_8x8_hd( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_TOP
    PREDICT_8x8_LOAD_LEFT
    PREDICT_8x8_LOAD_TOPLEFT
    SRC(0,7)= (l6 + l7 + 1) >> 1;
    SRC(1,7)= (l5 + 2*l6 + l7 + 2) >> 2;
    SRC(0,6)=SRC(2,7)= (l5 + l6 + 1) >> 1;
    SRC(1,6)=SRC(3,7)= (l4 + 2*l5 + l6 + 2) >> 2;
    SRC(0,5)=SRC(2,6)=SRC(4,7)= (l4 + l5 + 1) >> 1;
    SRC(1,5)=SRC(3,6)=SRC(5,7)= (l3 + 2*l4 + l5 + 2) >> 2;
    SRC(0,4)=SRC(2,5)=SRC(4,6)=SRC(6,7)= (l3 + l4 + 1) >> 1;
    SRC(1,4)=SRC(3,5)=SRC(5,6)=SRC(7,7)= (l2 + 2*l3 + l4 + 2) >> 2;
    SRC(0,3)=SRC(2,4)=SRC(4,5)=SRC(6,6)= (l2 + l3 + 1) >> 1;
    SRC(1,3)=SRC(3,4)=SRC(5,5)=SRC(7,6)= (l1 + 2*l2 + l3 + 2) >> 2;
    SRC(0,2)=SRC(2,3)=SRC(4,4)=SRC(6,5)= (l1 + l2 + 1) >> 1;
    SRC(1,2)=SRC(3,3)=SRC(5,4)=SRC(7,5)= (l0 + 2*l1 + l2 + 2) >> 2;
    SRC(0,1)=SRC(2,2)=SRC(4,3)=SRC(6,4)= (l0 + l1 + 1) >> 1;
    SRC(1,1)=SRC(3,2)=SRC(5,3)=SRC(7,4)= (lt + 2*l0 + l1 + 2) >> 2;
    SRC(0,0)=SRC(2,1)=SRC(4,2)=SRC(6,3)= (lt + l0 + 1) >> 1;
    SRC(1,0)=SRC(3,1)=SRC(5,2)=SRC(7,3)= (l0 + 2*lt + t0 + 2) >> 2;
    SRC(2,0)=SRC(4,1)=SRC(6,2)= (t1 + 2*t0 + lt + 2) >> 2;
    SRC(3,0)=SRC(5,1)=SRC(7,2)= (t2 + 2*t1 + t0 + 2) >> 2;
    SRC(4,0)=SRC(6,1)= (t3 + 2*t2 + t1 + 2) >> 2;
    SRC(5,0)=SRC(7,1)= (t4 + 2*t3 + t2 + 2) >> 2;
    SRC(6,0)= (t5 + 2*t4 + t3 + 2) >> 2;
    SRC(7,0)= (t6 + 2*t5 + t4 + 2) >> 2;
}

/*

*/
static void predict_8x8_vl( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_TOP
    PREDICT_8x8_LOAD_TOPRIGHT
    SRC(0,0)= (t0 + t1 + 1) >> 1;
    SRC(0,1)= (t0 + 2*t1 + t2 + 2) >> 2;
    SRC(0,2)=SRC(1,0)= (t1 + t2 + 1) >> 1;
    SRC(0,3)=SRC(1,1)= (t1 + 2*t2 + t3 + 2) >> 2;
    SRC(0,4)=SRC(1,2)=SRC(2,0)= (t2 + t3 + 1) >> 1;
    SRC(0,5)=SRC(1,3)=SRC(2,1)= (t2 + 2*t3 + t4 + 2) >> 2;
    SRC(0,6)=SRC(1,4)=SRC(2,2)=SRC(3,0)= (t3 + t4 + 1) >> 1;
    SRC(0,7)=SRC(1,5)=SRC(2,3)=SRC(3,1)= (t3 + 2*t4 + t5 + 2) >> 2;
    SRC(1,6)=SRC(2,4)=SRC(3,2)=SRC(4,0)= (t4 + t5 + 1) >> 1;
    SRC(1,7)=SRC(2,5)=SRC(3,3)=SRC(4,1)= (t4 + 2*t5 + t6 + 2) >> 2;
    SRC(2,6)=SRC(3,4)=SRC(4,2)=SRC(5,0)= (t5 + t6 + 1) >> 1;
    SRC(2,7)=SRC(3,5)=SRC(4,3)=SRC(5,1)= (t5 + 2*t6 + t7 + 2) >> 2;
    SRC(3,6)=SRC(4,4)=SRC(5,2)=SRC(6,0)= (t6 + t7 + 1) >> 1;
    SRC(3,7)=SRC(4,5)=SRC(5,3)=SRC(6,1)= (t6 + 2*t7 + t8 + 2) >> 2;
    SRC(4,6)=SRC(5,4)=SRC(6,2)=SRC(7,0)= (t7 + t8 + 1) >> 1;
    SRC(4,7)=SRC(5,5)=SRC(6,3)=SRC(7,1)= (t7 + 2*t8 + t9 + 2) >> 2;
    SRC(5,6)=SRC(6,4)=SRC(7,2)= (t8 + t9 + 1) >> 1;
    SRC(5,7)=SRC(6,5)=SRC(7,3)= (t8 + 2*t9 + t10 + 2) >> 2;
    SRC(6,6)=SRC(7,4)= (t9 + t10 + 1) >> 1;
    SRC(6,7)=SRC(7,5)= (t9 + 2*t10 + t11 + 2) >> 2;
    SRC(7,6)= (t10 + t11 + 1) >> 1;
    SRC(7,7)= (t10 + 2*t11 + t12 + 2) >> 2;
}

/*

*/
static void predict_8x8_hu( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_LEFT
    SRC(0,0)= (l0 + l1 + 1) >> 1;
    SRC(1,0)= (l0 + 2*l1 + l2 + 2) >> 2;
    SRC(0,1)=SRC(2,0)= (l1 + l2 + 1) >> 1;
    SRC(1,1)=SRC(3,0)= (l1 + 2*l2 + l3 + 2) >> 2;
    SRC(0,2)=SRC(2,1)=SRC(4,0)= (l2 + l3 + 1) >> 1;
    SRC(1,2)=SRC(3,1)=SRC(5,0)= (l2 + 2*l3 + l4 + 2) >> 2;
    SRC(0,3)=SRC(2,2)=SRC(4,1)=SRC(6,0)= (l3 + l4 + 1) >> 1;
    SRC(1,3)=SRC(3,2)=SRC(5,1)=SRC(7,0)= (l3 + 2*l4 + l5 + 2) >> 2;
    SRC(0,4)=SRC(2,3)=SRC(4,2)=SRC(6,1)= (l4 + l5 + 1) >> 1;
    SRC(1,4)=SRC(3,3)=SRC(5,2)=SRC(7,1)= (l4 + 2*l5 + l6 + 2) >> 2;
    SRC(0,5)=SRC(2,4)=SRC(4,3)=SRC(6,2)= (l5 + l6 + 1) >> 1;
    SRC(1,5)=SRC(3,4)=SRC(5,3)=SRC(7,2)= (l5 + 2*l6 + l7 + 2) >> 2;
    SRC(0,6)=SRC(2,5)=SRC(4,4)=SRC(6,3)= (l6 + l7 + 1) >> 1;
    SRC(1,6)=SRC(3,5)=SRC(5,4)=SRC(7,3)= (l6 + 3*l7 + 2) >> 2;
    SRC(0,7)=SRC(1,7)=SRC(2,6)=SRC(2,7)=SRC(3,6)=
    SRC(3,7)=SRC(4,5)=SRC(4,6)=SRC(4,7)=SRC(5,5)=
    SRC(5,6)=SRC(5,7)=SRC(6,4)=SRC(6,5)=SRC(6,6)=
    SRC(6,7)=SRC(7,4)=SRC(7,5)=SRC(7,6)=SRC(7,7)= l7;
}

/****************************************************************************
 * Exported(���) functions:
 * predict:Ԥ��
 * 
 * 
 * 
 * 
 * 
 * 
 * ���ϣ��Ϻ��204ҳ��16x16���ȿ��֡��Ԥ�ⷽʽ
 *
 * Ԥ��ģʽ���			Ԥ��ģʽ				ע��
 *
 *		0			Intra_16x16_Vertical		��ֱ
 *		1			Intra_16x16_Horizontal		ˮƽ
 *		2			Intra_16x16_DC				 DC
 *		3			Intra_16x16_Plane			ƽ��
 * 
 * 
 ****************************************************************************/
void x264_predict_16x16_init( int cpu, x264_predict_t pf[7] )
{
    pf[I_PRED_16x16_V ]     = predict_16x16_v;
    pf[I_PRED_16x16_H ]     = predict_16x16_h;
    pf[I_PRED_16x16_DC]     = predict_16x16_dc;
    pf[I_PRED_16x16_P ]     = predict_16x16_p;
    pf[I_PRED_16x16_DC_LEFT]= predict_16x16_dc_left;
    pf[I_PRED_16x16_DC_TOP ]= predict_16x16_dc_top;
    pf[I_PRED_16x16_DC_128 ]= predict_16x16_dc_128;

#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMXEXT )
    {
        x264_predict_16x16_init_mmxext( pf );
    }
#endif
}

/*
 * 
 * 
 * ֡��8x8ɫ�ȿ��Ԥ�ⷽʽ
 *	Ԥ��ģʽ���		Ԥ��ģʽ					ע��
 * 
 *		0				Intra_Chroma_DC				 DC
 *		1				Intra_Chroma_Horizontal		ˮƽ
 *		2				Intra_Chroma_Vertical		��ֱ
 *		3				Intra_Chroma_Plane			ƽ��
*/

void x264_predict_8x8c_init( int cpu, x264_predict_t pf[7] )
{
    pf[I_PRED_CHROMA_V ]     = predict_8x8c_v;
    pf[I_PRED_CHROMA_H ]     = predict_8x8c_h;
    pf[I_PRED_CHROMA_DC]     = predict_8x8c_dc;
    pf[I_PRED_CHROMA_P ]     = predict_8x8c_p;
    pf[I_PRED_CHROMA_DC_LEFT]= predict_8x8c_dc_left;
    pf[I_PRED_CHROMA_DC_TOP ]= predict_8x8c_dc_top;
    pf[I_PRED_CHROMA_DC_128 ]= predict_8x8c_dc_128;

#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMXEXT )
    {
        x264_predict_8x8c_init_mmxext( pf );
    }
#endif
}

/*
 *
 * 
 *				֡��8x8���ȿ��Ԥ�ⷽʽ
 ****************************************************************
 * 
 *		Ԥ��ģʽ���			Ԥ��ģʽ				ע��
 *			0					Vertical	(ģʽ0)
 *			1					Horizontal(ģʽ1)
 *			2					DC(ģʽ2)
 *			3					Diagonal_Down_Left(ģʽ3)
 *			4					Diagonal_Down_Right(ģʽ4)
 *			5					Vertical_Right(ģʽ5)
 *			6					Horizontal_Down(ģʽ6)
 *			7					Vertical_Left(ģʽ7)
 *			8					Horizontal_Up(ģʽ8)
 *

*/
void x264_predict_8x8_init( int cpu, x264_predict8x8_t pf[12] )
{
    pf[I_PRED_8x8_V]      = predict_8x8_v;			//Vertical(ģʽ0)
    pf[I_PRED_8x8_H]      = predict_8x8_h;			//Horizontal(ģʽ1)
    pf[I_PRED_8x8_DC]     = predict_8x8_dc;			//DC(ģʽ2)
    pf[I_PRED_8x8_DDL]    = predict_8x8_ddl;		//Diagonal_Down_Left(ģʽ3)
    pf[I_PRED_8x8_DDR]    = predict_8x8_ddr;		//Diagonal_Down_Right(ģʽ4)
    pf[I_PRED_8x8_VR]     = predict_8x8_vr;			//Vertical_Right(ģʽ5)
    pf[I_PRED_8x8_HD]     = predict_8x8_hd;			//Horizontal_Down(ģʽ6)
    pf[I_PRED_8x8_VL]     = predict_8x8_vl;			//Vertical_Left(ģʽ7)
    pf[I_PRED_8x8_HU]     = predict_8x8_hu;			//Horizontal_Up(ģʽ8)
    pf[I_PRED_8x8_DC_LEFT]= predict_8x8_dc_left;	//	
    pf[I_PRED_8x8_DC_TOP] = predict_8x8_dc_top;		//
    pf[I_PRED_8x8_DC_128] = predict_8x8_dc_128;		//

#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMXEXT )
    {
        x264_predict_8x8_init_mmxext( pf );
    }
#endif
#ifdef HAVE_SSE2
    if( cpu&X264_CPU_SSE2 )
    {
        x264_predict_8x8_init_sse2( pf );
    }
#endif
}

/*
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * ���ϣ��Ϻ�ܣ���200ҳ��4x4���ȿ��֡��Ԥ����뷽ʽ
*/
void x264_predict_4x4_init( int cpu, x264_predict_t pf[12] )
{
    pf[I_PRED_4x4_V]      = predict_4x4_v;			//Vertical(ģʽ0)
    pf[I_PRED_4x4_H]      = predict_4x4_h;			//Horizontal(ģʽ1)
    pf[I_PRED_4x4_DC]     = predict_4x4_dc;			//DC(ģʽ2)
    pf[I_PRED_4x4_DDL]    = predict_4x4_ddl;		//Diagonal_Down_Left(ģʽ3)
    pf[I_PRED_4x4_DDR]    = predict_4x4_ddr;		//Diagonal_Down_Right(ģʽ4)
    pf[I_PRED_4x4_VR]     = predict_4x4_vr;			//Vertical_Right(ģʽ5)
    pf[I_PRED_4x4_HD]     = predict_4x4_hd;			//Horizontal_Down(ģʽ6)
    pf[I_PRED_4x4_VL]     = predict_4x4_vl;			//Vertical_Left(ģʽ7)
    pf[I_PRED_4x4_HU]     = predict_4x4_hu;			//Horizontal_Up(ģʽ8)
    pf[I_PRED_4x4_DC_LEFT]= predict_4x4_dc_left;
    pf[I_PRED_4x4_DC_TOP] = predict_4x4_dc_top;
    pf[I_PRED_4x4_DC_128] = predict_4x4_dc_128;

#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMXEXT )
    {
        x264_predict_4x4_init_mmxext( pf );
    }
#endif
}

