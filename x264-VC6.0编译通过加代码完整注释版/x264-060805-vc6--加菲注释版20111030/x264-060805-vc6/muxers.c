/*****************************************************************************
 * muxers.c: h264 file i/o plugins
 * muxer:ƥ��
 *****************************************************************************
 * Copyright (C) 2003-2006 x264 project
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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "common/common.h"
#include "x264.h"
#include "matroska.h"
#include "muxers.h"

#ifndef _MSC_VER
#include "config.h"
#endif

#ifdef AVIS_INPUT
#include <windows.h>
#include <vfw.h>
#endif

#ifdef MP4_OUTPUT
#include <gpac/isomedia.h>
#endif

typedef struct {
    FILE *fh;			//�����ļ����ļ�ָ��
    int width, height;	//��͸ߣ�Ӧ����ָ��Ƶ����ĳߴ�
    int next_frame;		//��һ֡
} yuv_input_t;

/* 
raw 420 yuv file operation(����) 
��һ���ļ����Ѿ��ļ�·��
*/
int open_file_yuv( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param )
{
    yuv_input_t *h = malloc(sizeof(yuv_input_t));//����һ���ڴ��yuv_input_t�ṹ���ã��ڴ��ַ�浽h
	/*�Խṹ���ֶθ�ֵ*/
	h->width = p_param->i_width;
    h->height = p_param->i_height;
    h->next_frame = 0;

	printf("\n(muxer.c) open_file_yuv(...) done");//zjh�����������к�׺Ϊ.yuvʱ��ʾ�˾�

    if( !strcmp(psz_filename, "-") )
        h->fh = stdin;//���Ӧ���Ǵ����ļ��Ѿ��򿪣����ֻ�����������/��ַ
    else
        h->fh = fopen(psz_filename, "rb");/* ��һ���ļ� �ļ�˳���򿪺�ָ��������ļ�ָ��ͻᱻ���ء�����ļ���ʧ���򷵻�NULL�����Ѵ���������errno �� */
											//rb ��ֻ����ʽ���ļ������ļ��������,����b �ַ��������ߺ�����򿪵��ļ�Ϊ�������ļ������Ǵ������ļ�
    if( h->fh == NULL )//�ļ���ʧ��
        return -1;

    *p_handle = (hnd_t)h;//�Ѵ˴��򿪵��ļ���ָ�봫��ȥ����ߵ�p_handle�ڵ�������ʱ�Բ�����ʽ�ṩ����

    //printf("�ļ�����%s\n",psz_filename);//zjh
	return 0;
}

/*
�õ�֡������

������Ĳ�������������open_file_yuv����ǰ�������Ǹ��ļ�ָ��
���ͨ���ļ����ܳ���/ÿ֡�ߴ�������õ���֡�������Կ������ļ�������yuv420�ģ������������ʽ���߰�����Ƶ�ģ��ǲ����ô˺��������
*/
int get_frame_total_yuv( hnd_t handle )
{
    yuv_input_t *h = handle;
    int i_frame_total = 0;	/* ����֡���� int get_frame_total_yuv( hnd_t handle ) { return i_frame_total }*/

    if( !fseek( h->fh, 0, SEEK_END ) )//�ض�λ��(������/�ļ�)�ϵ��ļ��ڲ�λ��ָ��;���������ļ�ָ��stream��λ�á����ִ�гɹ���stream��ָ����fromwhere��ƫ����ʼλ�ã��ļ�ͷ0����ǰλ��1���ļ�β2��Ϊ��׼��ƫ��offset��ָ��ƫ���������ֽڵ�λ�á����ִ��ʧ��(����offset�����ļ������С)���򲻸ı�streamָ���λ�á�
    {
        uint64_t i_size = ftell( h->fh );//���� ftell() ���ڵõ��ļ�λ��ָ�뵱ǰλ��������ļ��׵�ƫ���ֽ������������ʽ��ȡ�ļ�ʱ�������ļ�λ��Ƶ����ǰ���ƶ�����������ȷ���ļ��ĵ�ǰλ�á����ú���ftell()���ܷǳ����׵�ȷ���ļ��ĵ�ǰλ��
        fseek( h->fh, 0, SEEK_SET );//�ض�λ��(������/�ļ�)�ϵ��ļ��ڲ�λ��ָ��,ע�⣺���Ƕ�λ�ļ�ָ�룬�ļ�ָ��ָ���ļ�/����λ��ָ��ָ���ļ��ڲ����ֽ�λ�ã������ļ��Ķ�ȡ���ƶ����ļ�ָ����������¸�ֵ������ı�ָ�����ļ�
									//����fseek�е�һ������Ϊ�ļ�ָ�룬�ڶ�������Ϊƫ��������ʼλ�ã�SEEK_END ��ʾ�ļ�β��SEEK_SET ��ʾ�ļ�ͷ��
        i_frame_total = (int)(i_size / ( h->width * h->height * 3 / 2 ));//�����ܵ�֡��, �������1.5����Ϊһ�����뵥λ��һ�����ȿ��2��ɫ�ȿ飬��С�ϵ���1.5�����ȿ�
    }		/* i_frame_total:����֡���� */

    return i_frame_total;	/* i_frame_total */
}

/*
��ȡ֡��yuv��ʽ��

����֧�ֵ�yuv�洢��ʽΪ:�ȴ�һ֡��ȫ������ֵ���ٴ�һ֡��ȫ��Cbֵ���ٴ�һ֡��ȫ��Crֵ����һ֡Ҳ����ˣ������洢��ʽ�������֧��
*/
int read_frame_yuv( x264_picture_t *p_pic, hnd_t handle, int i_frame )
{
    yuv_input_t *h = handle;

    if( i_frame != h->next_frame )
        if( fseek( h->fh, (uint64_t)i_frame * h->width * h->height * 3 / 2, SEEK_SET ) )//��λ�ļ�
            return -1;

    if( fread( p_pic->img.plane[0], 1, h->width * h->height, h->fh ) <= 0	//fread:��һ�����ж�����//��Y����
            || fread( p_pic->img.plane[1], 1, h->width * h->height / 4, h->fh ) <= 0//��Cb
            || fread( p_pic->img.plane[2], 1, h->width * h->height / 4, h->fh ) <= 0 )//��Cr
        return -1;

    h->next_frame = i_frame+1;//��ס��һ֡�ǵڼ�֡���浽��yuv_input_t�ṹ�����һ���ֶ���

    return 0;
}

/*
	fread ����
	�� ��: ��һ�����ж�����
	����ԭ��: size_t fread( void *buffer, size_t size, size_t count, FILE *stream );
	�� ����
	1.���ڽ������ݵĵ�ַ��ָ�룩��buffer��
	2.����Ԫ�صĴ�С��size�� ����λ���ֽڶ�����λ�������ȡһ������������2���ֽ�
	3.Ԫ�ظ�����count��
	4.�ṩ���ݵ��ļ�ָ�루stream��
	����ֵ���ɹ���ȡ��Ԫ�ظ���
*/




/*

�ر�YUV�ļ�
*/
int close_file_yuv(hnd_t handle)
{
    yuv_input_t *h = handle;
    if( !h || !h->fh )
        return 0;
    return fclose(h->fh);
}

/* YUV4MPEG2 raw(ԭʼ��) 420 yuv file operation */
typedef struct {
    FILE *fh;			//�ļ�ָ�룬��������һ���Ѵ򿪵��ļ���ָ��
    int width, height;	//����
    int next_frame;		//��һ֡��������	
    int seq_header_len, frame_header_len;	/*  */
    int frame_size;		//֡�ߴ�
} y4m_input_t;			/*  */

#define Y4M_MAGIC "YUV4MPEG2"
#define MAX_YUV4_HEADER 80
#define Y4M_FRAME_MAGIC "FRAME"
#define MAX_FRAME_HEADER 80


/*
 * ���ƣ�
 * ���ܣ����ļ�:"*.y4m"
 * �������ļ�����typedef void *hnd_t,x264_param_t
 * ע�⣺
 * ���ϣ�YUV4MPEG2�����ļ���ʽ��һ����ͷ�ļ��洢��Ƶ����δѹ����Ƶ���С�����˵����ԭʼ��yuv���е���ʼ��ÿһ֡��ͷ���������˴�������ʽ����Ƶ������Ϣ�������ֱ��ʡ�֡�ʡ�����/����ɨ�跽ʽ���߿��(aspect ratio)���Լ�ÿһ֡��ʼ�ġ�FRAME ����־λ��
 *		 y4m��yuv��ת�� �˽���y4m�ķ�װ��ʽ�����ǵĹ����ͱ���൱��е��ֻҪ��ͷ�ļ���ÿ֡�ı�־λȥ�����ɣ�ʣ�µ��������ԭ�ⲻ����yuv���ݣ������4:2:0Ҳ����Ҫ��һ����ת������������
*/
int open_file_y4m( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param )	/* typedef void *hnd_t; */
{
    int  i, n, d;
    int  interlaced;						/* ����ɨ��,���� */
    char header[MAX_YUV4_HEADER+10];		/* #define MAX_YUV4_HEADER 80 ����ͷ���ĳ��ȶ�������10���ֽڿռ�*/
    char *tokstart, *tokend, *header_end;	/*  */
    y4m_input_t *h = malloc(sizeof(y4m_input_t));	/* �ṹ���ֶΰ����ļ�ָ�롢���Ϳ���һ֡�� */

	printf("\n(muxer.c) open_file_y4m(...) done\n");//zjh����Դ�������к�׺��Ϊ.y4mʱ��ʾ
    h->next_frame = 0;		/*  */

    if( !strcmp(psz_filename, "-") )	/* !strcmp(psz_filename, "-") Ϊ�棬��strcmp(psz_filename, "-")���뷵��0���⼴psz_filename=="-" */	
	/*
	�÷���#include <string.h>
	���ܣ��Ƚ��ַ���s1��s2��
	һ����ʽ��strcmp(�ַ���1���ַ���2)
	˵����
	��s1<s2ʱ������ֵ<0 ������s1=s2ʱ������ֵ=0 ������s1>s2ʱ������ֵ>0 �������������ַ���������������ַ���ȣ���ASCIIֵ��С��Ƚϣ���ֱ�����ֲ�ͬ���ַ�����'\0'Ϊֹ��	
	*/
        h->fh = stdin;		/* ??? */
    else
        h->fh = fopen(psz_filename, "rb");	/* ִ�д��ļ����� */
    if( h->fh == NULL )	/* ���ļ�ʧ�� */
        return -1;

    h->frame_header_len = strlen(Y4M_FRAME_MAGIC)+1;	/* #define Y4M_FRAME_MAGIC "FRAME" ; MAGIC:ħ�� */
														/* strlen("FRAME") + 1 */
    /* ��ȡͷ�� Read header */
    for( i=0; i<MAX_YUV4_HEADER; i++ )	/* ѭ��80�Σ���80�ֽ� #define MAX_YUV4_HEADER 80 */
    {
        header[i] = fgetc(h->fh);		/* ÿ��ѭ����ȡһ���ַ�������'\n'ʱ�˳� */
		/*  
		��ʽ��int fgetc(FILE *stream);
		��Ϊ���ļ�ָ��streamָ����ļ��ж�ȡһ���ַ���
		��������ķ���ֵ���Ƿ��ض�ȡ��һ���ֽڡ���������ļ�ĩβ����EOF��		
		*/
        if( header[i] == '\n' )		/* \n ASCII������\r����0x0D \n 0x0A  ͷ����β�����ͷ�������ַ���������� C�ַ���ʵ���Ͼ���һ����null('\0')�ַ���β���ַ����飬null�ַ���ʾ�ַ����Ľ�������Ҫע����ǣ�ֻ����null�ַ���β���ַ��������C�ַ���������ֻ��һ���C�ַ����顣 */
        {
            /* Add a space after last option. Makes parsing(��[��]��,�ֽ�) "444" vs
               "444alpha" easier(�����׵�). */
            header[i+1] = 0x20;	/* 0010 0000 ?�ڴ�λ�÷�����ʲô��˼ */
            header[i+2] = 0;	/* ? */
            break;
        }
    }
	/* �������y4m��ʽ���˳����� */
    if( i == MAX_YUV4_HEADER || strncmp(header, Y4M_MAGIC, strlen(Y4M_MAGIC)) )	/* ������80 ���� header��"YUV4MPEG2" */
		/*
		�����ж�Ϊ�棬�� i == 80 ���� �ļ�ͷ��û��YUV4MPEG2����ִ��return (��Ϊstrncmp�ڱȽ����ʱ����0���������ʱ�����ط�0)
		#define MAX_YUV4_HEADER 80
		#define Y4M_MAGIC "YUV4MPEG2"
		*/
        return -1;	/* �˳����� */

    /* Scan properties(����) */
    header_end = &header[i+1]; /* ͷ��ĩβ������i+1��  Include space */
    h->seq_header_len = i+1;	/* Sequence:����,�й�����һ������, һ�������Ⱥ����, ˳��, ���� */
    for( tokstart = &header[strlen(Y4M_MAGIC)+1]; tokstart < header_end; tokstart++ )	/* ��YUV4MPEG2���濪ʼɨ�裬YUV4MPEG2���ļ����ʼ�� */
    {	/* ����for���������ڴ��ַ */
        if(*tokstart==0x20) continue;	
		/* 	
		 ����0x20����ΪYUV4MPEG2�������һ��0x20��continue:��ֹ����ѭ�������ſ�ʼ�´�ѭ�� 
		 �磺YUV4MPEG2 W176 H144 F15:1 Ip A128:117 FRAME,.1# $%$""$##..
		*/
        switch(*tokstart++)	/* http://wmnmtm.blog.163.com/blog/static/3824571420118424330628/ */
        {
        case 'W': /* (��W176) �����Ǳ���� Width. Required. W������ž�����ֵ*/
            h->width = p_param->i_width = strtol(tokstart, &tokend, 10);	/* ��tokstartת���ɳ�����,���һ����������tokstart�Ľ������ͣ��˴�����10������ */
			/*
			long int strtol(const char *nptr,char **endptr,int base);
			��������Ὣ����nptr�ַ������ݲ���base��ת���ɳ�������������base��Χ��2��36����0������base����ɵĽ��Ʒ�ʽ����baseֵΪ10�����10���ƣ���baseֵΪ16�����16���Ƶȡ���baseֵΪ0ʱ���ǲ���10������ת�����������硯0x��ǰ���ַ����ʹ��16������ת����������0��ǰ���ַ������ǡ�0x����ʱ���ʹ��8������ת����һ��ʼstrtol()��ɨ�����nptr�ַ���������ǰ��Ŀո��ַ���ֱ���������ֻ��������Ųſ�ʼ��ת���������������ֻ��ַ�������ʱ('\0')����ת��������������ء�������endptr��ΪNULL����Ὣ����������������ֹ��nptr�е��ַ�ָ����endptr���ء�
			strtol��atoi����ǿ��
			��Ҫ�������⼸���棺
			1.��������ʶ��ʮ����������������ʶ���������Ƶ�������ȡ����base����������strtol("0XDEADbeE~~", NULL, 16)����0xdeadbee��ֵ��strtol("0777~~", NULL, 8)����0777��ֵ��
		����2.endptr��һ��������������������ʱָ�����δ��ʶ��ĵ�һ���ַ�������char *pos; strtol("123abc", &pos, 10);��strtol����123��posָ���ַ����е���ĸa������ַ�����ͷû�п�ʶ�������������char *pos; strtol("ABCabc", &pos, 10);����strtol����0��posָ���ַ�����ͷ�����Ծݴ��ж����ֳ���������������atoi�����˵ġ�
		����3.����ַ����е�����ֵ����long int�ı�ʾ��Χ����������磩����strtol���������ܱ�ʾ����󣨻���С��������������errnoΪERANGE������strtol("0XDEADbeef~~", NULL, 16)����0x7fffffff������errnoΪERANGE
			*/
            tokstart=tokend;	/*  */
            break;
        case 'H': /* (�� H144) �����Ǳ���� Height. Required. */
            h->height = p_param->i_height = strtol(tokstart, &tokend, 10);	/*  */
            tokstart=tokend;	/*  */
            break;
        case 'C': /* ɫ�ʿռ� ����Ǳ���� Color space */
            if( strncmp("420", tokstart, 3) )	/*  */
            {
                fprintf(stderr, "Colorspace(ɫ�ʿռ�) unhandled(δ���������)\n");	/*  */
                return -1;
            }
            tokstart = strchr(tokstart, 0x20);	/*  */
			/*
			ԭ�ͣ�extern char *strchr(const char *s,char c); 
			const char *strchr(const char* _Str,int _Val)
			char *strchr(char* _Str,int _Ch)
			ͷ�ļ���#include <string.h>
			���ܣ������ַ���s���״γ����ַ�c��λ��
			˵���������״γ���c��λ�õ�ָ�룬���s�в�����c�򷵻�NULL��
			����ֵ��Returns the address of the first occurrence of the character in the string if successful, or NULL otherwise 			
			*/
            break;
        case 'I': /* (��Ip) Interlace(����;ʹ����;ʹ��֯) type */
            switch(*tokstart++)	/*  */
            {
            case 'p': interlaced = 0; break;	/* ���Ǹ��� */
            case '?':	/*  */
            case 't':	/*  */
            case 'b':	/*  */
            case 'm':	/*  */
            default: interlaced = 1;	/* ����ɨ�� */
                fprintf(stderr, "Warning, this sequence(����) might(����) be interlaced\n");	/*  */
            }
            break;
        case 'F': /* (�� F15:1)Frame rate(����, �ʣ�(�˶����仯�ȵ�)�ٶ�; ����) - 0:0 if unknown */
            if( sscanf(tokstart, "%d:%d", &n, &d) == 2 && n && d )	/*  */
            {
                x264_reduce_fraction( &n, &d );	/* x264_reduce_fraction:�������� (common.c�ж���) reduce:ת����������ԭ,��������[��]; fraction:���� */
                p_param->i_fps_num = n;	/* ���ļ��е�ֵ�浽�������Ľṹ���ֶ���ȥ */
                p_param->i_fps_den = d;	/*  */
            }
            tokstart = strchr(tokstart, 0x20);	/* �����ַ���s���״γ����ַ�c��λ�� */
            break;
        case 'A': /* (�� A128:117) Pixel aspect(����) - 0:0 if unknown */
            if( sscanf(tokstart, "%d:%d", &n, &d) == 2 && n && d )	/*  */
            {
                x264_reduce_fraction( &n, &d );	/*  */
                p_param->vui.i_sar_width = n;	/* ��׼313ҳ �䷨Ԫ��sar_height��ʾ����߿�ȵ�ˮƽ�ߴ�(�����ⵥλ)  */
                p_param->vui.i_sar_height = d;	/* ��׼313ҳ �䷨Ԫ��sar_height��ʾ����߿�ȵĴ�ֱ�ߴ�(����䷨Ԫ��sar_width��ͬ�����ⵥλ) */
            }
            tokstart = strchr(tokstart, 0x20);	/* �ҵ���������ո��0x20���������ĵ�ַ���´δ�������� */
            break;
        case 'X': /* Vendor(����;����) extensions(��չ) */
            if( !strncmp("YSCSS=",tokstart,6) )	/*  */
            {
                /* Older nonstandard pixel format representation */
                tokstart += 6;	/*  */
                if( strncmp("420JPEG",tokstart,7) &&	/*  */
                    strncmp("420MPEG2",tokstart,8) &&	/*  */
                    strncmp("420PALDV",tokstart,8) )	/*  */
                {
                    fprintf(stderr, "Unsupported extended colorspace\n");	/* ����֧�ֵ���չɫ�ʿռ� */
                    return -1;
                }
            }
            tokstart = strchr(tokstart, 0x20);	/* ���ܣ������ַ���s���״γ����ַ�c��λ�� */
            break;
        }
    }

    fprintf(stderr, "yuv4mpeg: %ix%i@%i/%ifps, %i:%i\n",
            h->width, h->height, p_param->i_fps_num, p_param->i_fps_den,
            p_param->vui.i_sar_width, p_param->vui.i_sar_height);	/*  */

    *p_handle = (hnd_t)h;	/*  */
    return 0;
}

/* Most common(������,���е�) case(����, ʵ��,���): frame_header = "FRAME" 
 * ȡ��֡����.y4m
 * 
 * file format:
 * seq_header
 * [frame_header +frame]
 * [frame_header +frame]
 * ... ...
 * [frame_header +frame]
*/
int get_frame_total_y4m( hnd_t handle )			/*  */
{
    y4m_input_t *h             = handle;		/* ����ṹ���ָ�� */	
    int          i_frame_total = 0;				/* ԭʼ֡���� */
    off_t        init_pos      = ftell(h->fh);	/* fh:�ṹ��y4m_input_t��fh�ֶ���FILE���͵�ָ�� */

    if( !fseek( h->fh, 0, SEEK_END ) )		/* �ض�λ��(������/�ļ�)�ϵ��ļ��ڲ�λ��ָ��;���������ļ�ָ��stream��λ�á����ִ�гɹ���stream��ָ����fromwhere��ƫ����ʼλ�ã��ļ�ͷ0����ǰλ��1���ļ�β2��Ϊ��׼��ƫ��offset��ָ��ƫ���������ֽڵ�λ�á����ִ��ʧ��(����offset�����ļ������С)���򲻸ı�streamָ���λ�á� */
    {
        uint64_t i_size = ftell( h->fh );	/* ���� ftell() ���ڵõ��ļ�λ��ָ�뵱ǰλ��������ļ��׵�ƫ���ֽ������������ʽ��ȡ�ļ�ʱ�������ļ�λ��Ƶ����ǰ���ƶ�����������ȷ���ļ��ĵ�ǰλ�á����ú���ftell()���ܷǳ����׵�ȷ���ļ��ĵ�ǰλ�� */
        fseek( h->fh, init_pos, SEEK_SET );	/* �ض�λ��(������/�ļ�)�ϵ��ļ��ڲ�λ��ָ��,ע�⣺���Ƕ�λ�ļ�ָ�룬�ļ�ָ��ָ���ļ�/����λ��ָ��ָ���ļ��ڲ����ֽ�λ�ã������ļ��Ķ�ȡ���ƶ����ļ�ָ����������¸�ֵ������ı�ָ�����ļ� */
        i_frame_total = (int)((i_size - h->seq_header_len) /
                              (3*(h->width * h->height)/2 + h->frame_header_len)); /* ԭʼ��Ƶ�ļ�����֡�� */
							/* ԭʼ֡�� = (�ļ��ܴ�С - ���е�ͷ����)/((��*��)*1.5 + ֡ͷ������) */
							/* ÿ֡ǰ����"FRAME" */
							/* һ������ռ1.5�ֽ�? */
    }

    return i_frame_total;	/* ԭʼ��Ƶ�ļ�����֡�� */
}


/*
 * ���ƣ�
 * ������
 * ע�⣺
 * ˼·��
 * ���ϣ�
*/
int read_frame_y4m( x264_picture_t *p_pic, hnd_t handle, int i_frame )
{
    int          slen = strlen(Y4M_FRAME_MAGIC);	/* #define Y4M_FRAME_MAGIC "FRAME" */
    int          i    = 0;
    char         header[16];
    y4m_input_t *h    = handle;

    if( i_frame != h->next_frame )
    {
        if (fseek(h->fh, (uint64_t)i_frame*(3*(h->width*h->height)/2+h->frame_header_len)
                  + h->seq_header_len, SEEK_SET))
            return -1;
    }

    /* Read frame header - without terminating(ʹ�ս�) '\n' */
    if (fread(header, 1, slen, h->fh) != slen)	/*  */
        return -1;
/*
	
������ ��: ��һ�����ж�����
	����ԭ��: size_t fread( void *buffer, size_t size, size_t count, FILE *stream );
	�� ����
	����1.���ڽ������ݵĵ�ַ(ָ��)(buffer)
	����2.����Ԫ�صĴ�С(size) ����λ���ֽڶ�����λ�������ȡһ������������2���ֽ�
	����3.Ԫ�ظ���(count)
	����4.�ṩ���ݵ��ļ�ָ��(stream)
	��������ֵ���ɹ���ȡ��Ԫ�ظ���
	������ ��: ��һ�����ж�����
	��������ԭ��: size_t fread( void *buffer, size_t size, size_t count, FILE *stream );
		�� ����
		����1.���ڽ������ݵĵ�ַ(ָ��)(buffer)
		����2.����Ԫ�صĴ�С(size) ����λ���ֽڶ�����λ�������ȡһ������������2���ֽ�
		����3.Ԫ�ظ���(count)
			4.�ṩ���ݵ��ļ�ָ��(stream)
		����ֵ���ɹ���ȡ��Ԫ�ظ���
*/    
    header[slen] = 0;
    if (strncmp(header, Y4M_FRAME_MAGIC, slen))	/* ����Ϊ�棬˵��strncmp�����˷�0ֵ,��"FRAME"����� */
		/* "FRAME"��ÿ֡ǰ�涼����� */
    {
        fprintf(stderr, "Bad header magic (%08X <=> %s)\n",
                *((uint32_t*)header), header);
        return -1;
    }
  
    /* 
	Skip most of it 

	*/
    while (i<MAX_FRAME_HEADER && fgetc(h->fh) != '\n')	/* ѭ������'\n'��ֹѭ�� */
        i++;
    if (i == MAX_FRAME_HEADER)	/* #define MAX_FRAME_HEADER 80 */
    {
        fprintf(stderr, "Bad frame header(�����֡ͷ��)!\n");
        return -1;
    }
    h->frame_header_len = i+slen+1;		/* i + strlen(Y4M_FRAME_MAGIC) + 1 */

    if( fread(p_pic->img.plane[0], 1, h->width*h->height, h->fh) <= 0				/* x264_picture_t.plane[0] */
        || fread(p_pic->img.plane[1], 1, h->width * h->height / 4, h->fh) <= 0		/* x264_picture_t.plane[1] */
        || fread(p_pic->img.plane[2], 1, h->width * h->height / 4, h->fh) <= 0)		/* x264_picture_t.plane[2] */
        return -1;

    h->next_frame = i_frame+1;

    return 0;
}

/*
 * ���ƣ�
 * ������
 * ���ܣ��ر�".y4m"�ļ�
 * ˼·��
 * ���ϣ�
*/
int close_file_y4m(hnd_t handle)
{
    y4m_input_t *h = handle;	/* �Ѵ����ָ�뱣����ṹ��y4m_input_t���ֶ� */
    if( !h || !h->fh )			/* hָ��Ϊ�� || y4m_input_h�����fh�ֶ�Ϊ�� */
        return 0;
    return fclose(h->fh);		/* FILE *fh; */
	/*
	�� ��: �ر�һ������
	ע�⣺ʹ��fclose()�����Ϳ��԰ѻ����������ʣ�����������������ļ��У����ͷ��ļ�ָ����йصĻ�������
	�� ��: int fclose(FILE *stream); 
	������ɹ��رգ�fclose ���� 0�����򷵻�EOF(-1)��
	�����ΪNULL�����ҳ�����Լ���ִ�У�fclose�趨error number��EINVAL��������EOF
	*/
}

/* avs/avi input file support(֧��) under cygwin(��װ) */

#ifdef AVIS_INPUT
typedef struct {
    PAVISTREAM p_avi;	/* P AVI STREAM */
    int width, height;	/* ��,�� */
} avis_input_t;

/* 
yuv_input_t 
avis_input_t
*/

/*
 * ���ƣ�
 * ������
 * ���ܣ������Լ��
 * ˼·��
 * ���ϣ�
*/
int gcd(int a, int b)
{
    int c;

    while (1)
    {
        c = a % b;
        if (!c)
            return b;
        a = b;
        b = c;
    }
}

/*
 * ���ƣ�
 * ������
 * ���ܣ���.avi/.avs�ļ�
 * ˼·��
 * ���ϣ�typedef void *hnd_t
 *		 http://wmnmtm.blog.163.com/blog/static/3824571420118494618377/
 *		 Ϊ�˶�avi���ж�д��΢���ṩ��һ��API���ܹ�50�����������ǵ���;��Ҫ�����࣬һ����avi�ļ��Ĳ�����һ����������streams�Ĳ�����
 *		1���򿪺͹ر��ļ�
		AVIFileOpen ��AVIFileAddRef�� AVIFileRelease

		2�����ļ��ж�ȡ�ļ���Ϣ
		ͨ��AVIFileInfo���Ի�ȡavi�ļ���һЩ��Ϣ�������������һ��AVIFILEINFO�ṹ��ͨ��AVIFileReadData�������� ��ȡAVIFileInfo�����ò�������Ϣ����Щ��ϢҲ���������ļ���ͷ��������ӵ��file�Ĺ�˾�͸��˵����ơ�

		3��д���ļ���Ϣ
		����ͨ��AVIFileWriteData������д���ļ���һЩ������Ϣ��

	����4���򿪺͹ر�һ����

	������һ���������͸����ļ�һ���������ͨ�� AVIFileGetStream��������һ�����������������������һ�����Ľӿڣ�Ȼ���ڸýӿ��б�����һ�������

	���������������ļ���ĳһ����������������Բ���AVIStreamOpenFromFile��������������ۺ���AVIFileOpen��AVIFileGetStream������

	���������������ļ��еĶ�������������Ҫ����AVIFileOpen��Ȼ��AVIFileGetStream��

	��������ͨ��AVIStreamAddRef������stream�ӿڵ����á�

	����ͨ��AVIStreamRelease�������ر������������������������streams�����ü���������������Ϊ0ʱ��ɾ����

	����5�������ж�ȡ���ݺ���Ϣ

	����AVIStreamInfo�������Ի�ȡ���ݵ�һЩ��Ϣ���ú�������һ��AVISTREAMINFO�ṹ���ýṹ���������ݵ�����ѹ�������������buffersize���طŵ�rate���Լ�һЩdescription��

	�����������������һЩ�����Ķ������Ϣ�������ͨ��AVIStreamReadData��������ȡ��Ӧ�ó������һ���ڴ棬���ݸ����������Ȼ�����������ͨ������ڴ淵������������Ϣ���������Ϣ���ܰ�����������ѹ���ͽ�ѹ���ķ����������ͨ��AVIStreamDataSize������ȥ��Ҫ�����ڴ��Ĵ�С��

		����ͨ��AVIStreamReadFormat������ȡ�������ĸ�ʽ��Ϣ���������ͨ��ָ�����ڴ淵���������ĸ�ʽ��Ϣ�����������Ƶ������� buffer������һ��BIMAPINFO�ṹ��������Ƶ�����ڴ�������WAVEFORMATEX����PCMAVEFORMAT�ṹ�������ͨ���� AVIStreamReadFormat����һ����buffer�Ϳ��Ի�ȡbuffer�Ĵ�С��Ҳ����ͨ��AVIStreamFormatSize�ꡣ

		����ͨ��AVIStreamRead���������ض�ý������ݡ�������������ݸ��Ƶ�Ӧ�ó����ṩ���ڴ��У�������Ƶ���������������ͼ������������Ƶ �����������������Ƶ��sample���ݡ�����ͨ����AVIStreamRead����һ��NULL��buffer����ȡ��Ҫ��buffer�Ĵ�С��Ҳ���� ͨ��AVIStreamSampleSize������ȡbuffer�Ĵ�С��

		��ЩAVI���������������Ҫ��������������ǰҪ��һ��׼ ����������ʱ�����ǿ��Ե���AVIStreamBeginStreaming��������֪AVI������handle�������������Ҫ��һЩ��Դ������Ϻ� ����AVIStreamEndStreamming�������ͷ���Դ��

	����6������ѹ������Ƶ����

		7�������Ѵ��ڵ������������ļ�

		8�����ļ�д��һ��������

		9���������е�����λ��

*/
int open_file_avis( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param )
{
    avis_input_t *h = malloc(sizeof(avis_input_t));		/* ��̬�ڴ����룬�������avis_input_t */
    AVISTREAMINFO info;		/*  */
    int i;

    *p_handle = (hnd_t)h;

    AVIFileInit();		/* ��ʼ��AVI�� */
    if( AVIStreamOpenFromFile( &h->p_avi, psz_filename, streamtypeVIDEO, 0, OF_READ, NULL ) )	/* �����������ļ���ĳһ����������������Բ���AVIStreamOpenFromFile��������������ۺ���AVIFileOpen��AVIFileGetStream���� */
    {
        AVIFileExit();	/* �ͷ�AVI�� */
        return -1;
    }

    if( AVIStreamInfo(h->p_avi, &info, sizeof(AVISTREAMINFO)) )
    {
        AVIStreamRelease(h->p_avi);		
		/* �ر������� ͨ��AVIStreamRelease�������ر������������������������streams�����ü���������������Ϊ0ʱ��ɾ�� 
		����ͨ��AVIStreamAddRef������stream�ӿڵ�����
		*/
        AVIFileExit();	/* �ͷ�AVI�� */
        return -1;
    }

    // check input format
    if (info.fccHandler != MAKEFOURCC('Y', 'V', '1', '2'))
    {	/* AVISTREAMINFO.fccHandler */
        fprintf( stderr, "avis [error]: unsupported input format (%c%c%c%c)\n",
            (char)(info.fccHandler & 0xff), (char)((info.fccHandler >> 8) & 0xff),
            (char)((info.fccHandler >> 16) & 0xff), (char)((info.fccHandler >> 24)) );

        AVIStreamRelease(h->p_avi);
        AVIFileExit();

        return -1;
    }

	/*
	Ϊ�˼�RIFF�ļ��е�4�ַ���ʶ�Ķ�д��Ƚϣ�Windows SDK�ڶ�ý��ͷ�ļ�mmsystem.h�ж���������FOURCC(Four-Character Code���ַ�����)��
	typedef DWORD FOURCC;
	���乹���(���ڽ�4���ַ�ת����һ��FOURCC����)
	FOURCC mmioFOURCC(CHAR ch0, CHAR ch1, CHAR ch2, CHAR ch3);
	�䶨��ΪMAKEFOURCC�꣺
	#define mmioFOURCC(ch0, ch1, ch2, ch3) MAKEFOURCC(ch0, ch1, ch2, ch3);
	��MAKEFOURCC�궨��Ϊ��
	#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 )); 
	*/
    h->width  = p_param->i_width = info.rcFrame.right - info.rcFrame.left;	/* RECT rcFrame */
    h->height = p_param->i_height = info.rcFrame.bottom - info.rcFrame.top;	
	/* avis_input_t *h */
	/* x264_param_t *p_param */
    i = gcd(info.dwRate, info.dwScale);		/* �����Լ�� */
    p_param->i_fps_den = info.dwScale / i;	/* AVISTREAMINFO.dwScale */
    p_param->i_fps_num = info.dwRate / i;	/* AVISTREAMINFO.dwRate */

    fprintf( stderr, "avis [info]: %dx%d @ %.2f fps (%d frames)\n",	/*  */
        p_param->i_width, p_param->i_height,						/*  */
        (double)p_param->i_fps_num / (double)p_param->i_fps_den,	/*  */
        (int)info.dwLength );

    return 0;
}

/*
 * ���ƣ�ȡ֡������ .avi/avs
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int get_frame_total_avis( hnd_t handle )
{
    avis_input_t *h = handle;
    AVISTREAMINFO info;

    if( AVIStreamInfo(h->p_avi, &info, sizeof(AVISTREAMINFO)) )
        return -1;

    return info.dwLength;
}

/*
 * ���ƣ�
 * ������
 * ���ܣ���ȡ֡ .avi/.avs
 * ˼·��
 * ���ϣ�
*/
int read_frame_avis( x264_picture_t *p_pic, hnd_t handle, int i_frame )
{
    avis_input_t *h = handle;

    p_pic->img.i_csp = X264_CSP_YV12;

    if( AVIStreamRead(h->p_avi, i_frame, 1, p_pic->img.plane[0], h->width * h->height * 3 / 2, NULL, NULL ) )
        return -1;

    return 0;
}

/*
 * ���ƣ�
 * ������
 * ���ܣ��ر��ļ� .avi/.avs
 * ˼·��
 * ���ϣ�
*/
int close_file_avis( hnd_t handle )
{
    avis_input_t *h = handle;
    AVIStreamRelease(h->p_avi);
    AVIFileExit();
    free(h);
    return 0;
}
#endif


#ifdef HAVE_PTHREAD
typedef struct {
    int (*p_read_frame)( x264_picture_t *p_pic, hnd_t handle, int i_frame );
    int (*p_close_infile)( hnd_t handle );
    hnd_t p_handle;
    x264_picture_t pic;
    pthread_t tid;			/* typedef unsigned long int pthread_t; */
    int next_frame;
    int frame_total;
    struct thread_input_arg_t *next_args;
} thread_input_t;

typedef struct thread_input_arg_t {
    thread_input_t *h;
    x264_picture_t *pic;
    int i_frame;
    int status;
} thread_input_arg_t;

/*
 * ���ƣ�
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int open_file_thread( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param )
{
    thread_input_t *h = malloc(sizeof(thread_input_t));
    x264_picture_alloc( &h->pic, X264_CSP_I420, p_param->i_width, p_param->i_height );
    h->p_read_frame = p_read_frame;
    h->p_close_infile = p_close_infile;
    h->p_handle = *p_handle;
    h->next_frame = -1;
    h->next_args = malloc(sizeof(thread_input_arg_t));
    h->next_args->h = h;
    h->next_args->status = 0;
    h->frame_total = p_get_frame_total( h->p_handle );

    *p_handle = (hnd_t)h;
    return 0;
}

/*
 * ���ƣ�
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int get_frame_total_thread( hnd_t handle )
{
    thread_input_t *h = handle;
    return h->frame_total;
}

/*
 * ���ƣ�
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
void read_frame_thread_int( thread_input_arg_t *i )
{
    i->status = i->h->p_read_frame( i->pic, i->h->p_handle, i->i_frame );
}	/* status:״�� */

/*
 * ���ƣ�
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int read_frame_thread( x264_picture_t *p_pic, hnd_t handle, int i_frame )
{
    thread_input_t *h = handle;
    UNUSED void *stuff;
    int ret = 0;

    if( h->next_frame >= 0 )
    {
        pthread_join( h->tid, &stuff );
        ret |= h->next_args->status;
    }

    if( h->next_frame == i_frame )
    {
        XCHG( x264_picture_t, *p_pic, h->pic );
    }
    else
    {
        ret |= h->p_read_frame( p_pic, h->p_handle, i_frame );
    }

    if( !h->frame_total || i_frame+1 < h->frame_total )
    {
        h->next_frame =
        h->next_args->i_frame = i_frame+1;
        h->next_args->pic = &h->pic;
        pthread_create( &h->tid, NULL, (void*)read_frame_thread_int, h->next_args );
    }
    else
        h->next_frame = -1;

    return ret;
}

/*
 * ���ƣ�
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int close_file_thread( hnd_t handle )
{
    thread_input_t *h = handle;
    h->p_close_infile( h->p_handle );
    x264_picture_clean( &h->pic );
    free( h );
    return 0;
}
#endif

/*
���ܣ����ļ�
��ʽ��w+ 
˵��������b �ַ��������ߺ�����򿪵��ļ�Ϊ�������ļ������Ǵ������ļ�
ע�⣺�򿪿ɶ�д�ļ������ļ��������ļ�������Ϊ�㣬�����ļ����ݻ���ʧ�����ļ��������������ļ�w+ �򿪿ɶ�д�ļ������ļ��������ļ�������Ϊ�㣬�����ļ����ݻ���ʧ�����ļ��������������ļ�
*/
int open_file_bsf( char *psz_filename, hnd_t *p_handle )
{
    if ((*p_handle = fopen(psz_filename, "w+b")) == NULL)//+ �򿪿ɶ�д�ļ������ļ��������ļ�������Ϊ�㣬�����ļ����ݻ���ʧ�����ļ��������������ļ�����������̬�ַ����������ټ�һ��b�ַ�����rb��w+b��ab������ϣ�����b �ַ��������ߺ�����򿪵��ļ�Ϊ�������ļ������Ǵ������ļ���
        return -1;

	printf("\n (muxers.c) open_file_bsf \n");//zjh ����ִ�У�����ļ���׺��Ϊ.264 .yuv .avc .mp3 .mp6 .mkv 
    return 0;								//����ļ���׺Ϊ.mp4ʱ����� ��ʾ��not compiled with MP4 output support ���ܱ���MP4 http://wmnmtm.blog.163.com/blog/static/38245714201181824344687/
}

/*
 * ���ƣ�
 * ����������_����_bsf
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int set_param_bsf( hnd_t handle, x264_param_t *p_param )
{
    return 0;
}

/*
 * ���ƣ�
 * ������hnd_t handle����һ������ʵ���Ǹ����
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/

int write_nalu_bsf( hnd_t handle, uint8_t *p_nalu, int i_size )
{
   if (fwrite(p_nalu, i_size, 1, (FILE *)handle) > 0)
	{
		//system("pause");//��ͣ�����������
		//printf("muxers.cд�ļ���fwrite i_size=%d\n",i_size);
		//exit(0);
	//	if (i_size > 5) 
	//		exit(0);
		return i_size;
	}
    return -1;
}

/*
size_t fwrite(const void*buffer,size_t size,size_t count,FILE*stream); ����
ע�⣺��������Զ�������ʽ���ļ����в��������������ı��ļ� ����
����ֵ������ʵ��д������ݿ���Ŀ
(1) buffer����һ��ָ�룬��fwrite��˵����Ҫ������ݵĵ�ַ��
(2) size��Ҫд�����ݵĵ��ֽ�����
(3) count:Ҫ����д��size�ֽڵ�������ĸ�����
(4) stream:Ŀ���ļ�ָ�롣
(5) �ɹ����� 1 �����򷵻� 0
˵����д�뵽�ļ������ ������ļ��Ĵ�ģʽ�йأ������w+�����Ǵ�file pointerָ��ĵ�ַ��ʼд���滻��֮������ݣ��ļ��ĳ��ȿ��Բ��䣬stream��λ���ƶ�count�����������a+������ļ���ĩβ��ʼ��ӣ��ļ����ȼӴ󣬶�����fseek�����Դ˺���û�����á�
*/


/*
 * ���ƣ�
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int set_eop_bsf( hnd_t handle,  x264_picture_t *p_picture )
{
    return 0;
}

/*
 * ���ƣ�
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int close_file_bsf( hnd_t handle )
{
    if ((handle == NULL) || (handle == stdout))
        return 0;

    return fclose((FILE *)handle);	/* �ر�һ���� */
}
/* 
������: fclose()
����: �ر�һ����
ע�⣺ʹ��fclose()�����Ϳ��԰ѻ����������ʣ�����������������ļ��У����ͷ��ļ�ָ����йصĻ�������
*/

/* -- mp4 muxing support ------------------------------------------------- */
#ifdef MP4_OUTPUT

typedef struct
{
    GF_ISOFile *p_file;
    GF_AVCConfig *p_config;
    GF_ISOSample *p_sample;
    int i_track;
    uint32_t i_descidx;
    int i_time_inc;
    int i_time_res;
    int i_numframe;
    int i_init_delay;
    uint8_t b_sps;
    uint8_t b_pps;
} mp4_t;

/*
 * ���ƣ�
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
void recompute_bitrate_mp4(GF_ISOFile *p_file, int i_track)		/* recompute:�ټ���,���� */
{
    u32 i, count, di, timescale, time_wnd, rate;
    u64 offset;
    Double br;
    GF_ESD *esd;

    esd = gf_isom_get_esd(p_file, i_track, 1);
    if (!esd) return;

    esd->decoderConfig->avgBitrate = 0;
    esd->decoderConfig->maxBitrate = 0;
    rate = time_wnd = 0;

    timescale = gf_isom_get_media_timescale(p_file, i_track);
    count = gf_isom_get_sample_count(p_file, i_track);
    for (i=0; i<count; i++) {
        GF_ISOSample *samp = gf_isom_get_sample_info(p_file, i_track, i+1, &di, &offset);

        if (samp->dataLength>esd->decoderConfig->bufferSizeDB) esd->decoderConfig->bufferSizeDB = samp->dataLength;

        if (esd->decoderConfig->bufferSizeDB < samp->dataLength) esd->decoderConfig->bufferSizeDB = samp->dataLength;
        esd->decoderConfig->avgBitrate += samp->dataLength;
        rate += samp->dataLength;
        if (samp->DTS > time_wnd + timescale) {
            if (rate > esd->decoderConfig->maxBitrate) esd->decoderConfig->maxBitrate = rate;
            time_wnd = samp->DTS;
            rate = 0;
        }

        gf_isom_sample_del(&samp);
    }

    br = (Double) (s64) gf_isom_get_media_duration(p_file, i_track);
    br /= timescale;
    esd->decoderConfig->avgBitrate = (u32) (esd->decoderConfig->avgBitrate / br);
    /*move to bps*/
    esd->decoderConfig->avgBitrate *= 8;
    esd->decoderConfig->maxBitrate *= 8;

    gf_isom_change_mpeg4_description(p_file, i_track, 1, esd);
    gf_odf_desc_del((GF_Descriptor *) esd);			/*  */
}

/*
 * ���ƣ�
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int close_file_mp4( hnd_t handle )
{
    mp4_t *p_mp4 = (mp4_t *)handle;

    if (p_mp4 == NULL)
        return 0;

    if (p_mp4->p_config)
        gf_odf_avc_cfg_del(p_mp4->p_config);

    if (p_mp4->p_sample)
    {
        if (p_mp4->p_sample->data)
            free(p_mp4->p_sample->data);

        gf_isom_sample_del(&p_mp4->p_sample);
    }

    if (p_mp4->p_file)
    {
        recompute_bitrate_mp4(p_mp4->p_file, p_mp4->i_track);
        gf_isom_set_pl_indication(p_mp4->p_file, GF_ISOM_PL_VISUAL, 0x15);
        gf_isom_set_storage_mode(p_mp4->p_file, GF_ISOM_STORE_FLAT);
        gf_isom_close(p_mp4->p_file);
    }

    free(p_mp4);

    return 0;
}

/*
 * ���ƣ�
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int open_file_mp4( char *psz_filename, hnd_t *p_handle )
{
    mp4_t *p_mp4;

    *p_handle = NULL;

    if ((p_mp4 = (mp4_t *)malloc(sizeof(mp4_t))) == NULL)
        return -1;

    memset(p_mp4, 0, sizeof(mp4_t));
    p_mp4->p_file = gf_isom_open(psz_filename, GF_ISOM_OPEN_WRITE, NULL);

    if ((p_mp4->p_sample = gf_isom_sample_new()) == NULL)
    {
        close_file_mp4( p_mp4 );
        return -1;
    }

    gf_isom_set_brand_info(p_mp4->p_file, GF_ISOM_BRAND_AVC1, 0);

    *p_handle = p_mp4;

    return 0;
}

/*
 * ���ƣ�
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int set_param_mp4( hnd_t handle, x264_param_t *p_param )
{
    mp4_t *p_mp4 = (mp4_t *)handle;

    p_mp4->i_track = gf_isom_new_track(p_mp4->p_file, 0, GF_ISOM_MEDIA_VISUAL,
        p_param->i_fps_num);

    p_mp4->p_config = gf_odf_avc_cfg_new();
    gf_isom_avc_config_new(p_mp4->p_file, p_mp4->i_track, p_mp4->p_config,
        NULL, NULL, &p_mp4->i_descidx);

    gf_isom_set_track_enabled(p_mp4->p_file, p_mp4->i_track, 1);

    gf_isom_set_visual_info(p_mp4->p_file, p_mp4->i_track, p_mp4->i_descidx,
        p_param->i_width, p_param->i_height);

    p_mp4->p_sample->data = (char *)malloc(p_param->i_width * p_param->i_height * 3 / 2);
    if (p_mp4->p_sample->data == NULL)
        return -1;

    p_mp4->i_time_res = p_param->i_fps_num;
    p_mp4->i_time_inc = p_param->i_fps_den;
    p_mp4->i_init_delay = p_param->i_bframe ? (p_param->b_bframe_pyramid ? 2 : 1) : 0;
    p_mp4->i_init_delay *= p_mp4->i_time_inc;
    fprintf(stderr, "mp4 [info]: initial delay %d (scale %d)\n",
        p_mp4->i_init_delay, p_mp4->i_time_res);

    return 0;
}

/*
 * ���ƣ�
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int write_nalu_mp4( hnd_t handle, uint8_t *p_nalu, int i_size )
{
    mp4_t *p_mp4 = (mp4_t *)handle;
    GF_AVCConfigSlot *p_slot;
    uint8_t type = p_nalu[4] & 0x1f;
    int psize;

    switch(type)
    {
    // sps
    case 0x07:
        if (!p_mp4->b_sps)
        {
            p_mp4->p_config->configurationVersion = 1;
            p_mp4->p_config->AVCProfileIndication = p_nalu[5];
            p_mp4->p_config->profile_compatibility = p_nalu[6];
            p_mp4->p_config->AVCLevelIndication = p_nalu[7];
            p_slot = (GF_AVCConfigSlot *)malloc(sizeof(GF_AVCConfigSlot));
            p_slot->size = i_size - 4;
            p_slot->data = (char *)malloc(p_slot->size);
            memcpy(p_slot->data, p_nalu + 4, i_size - 4);
            gf_list_add(p_mp4->p_config->sequenceParameterSets, p_slot);
            p_slot = NULL;
            p_mp4->b_sps = 1;
        }
        break;

    // pps
    case 0x08:
        if (!p_mp4->b_pps)
        {
            p_slot = (GF_AVCConfigSlot *)malloc(sizeof(GF_AVCConfigSlot));
            p_slot->size = i_size - 4;
            p_slot->data = (char *)malloc(p_slot->size);
            memcpy(p_slot->data, p_nalu + 4, i_size - 4);
            gf_list_add(p_mp4->p_config->pictureParameterSets, p_slot);
            p_slot = NULL;
            p_mp4->b_pps = 1;
            if (p_mp4->b_sps)
                gf_isom_avc_config_update(p_mp4->p_file, p_mp4->i_track, 1, p_mp4->p_config);
        }
        break;

    // slice, sei
    case 0x1:
    case 0x5:
    case 0x6:
        psize = i_size - 4 ;
        memcpy(p_mp4->p_sample->data + p_mp4->p_sample->dataLength, p_nalu, i_size);
        p_mp4->p_sample->data[p_mp4->p_sample->dataLength + 0] = (psize >> 24) & 0xff;
        p_mp4->p_sample->data[p_mp4->p_sample->dataLength + 1] = (psize >> 16) & 0xff;
        p_mp4->p_sample->data[p_mp4->p_sample->dataLength + 2] = (psize >> 8) & 0xff;
        p_mp4->p_sample->data[p_mp4->p_sample->dataLength + 3] = (psize >> 0) & 0xff;
        p_mp4->p_sample->dataLength += i_size;
        break;
    }

    return i_size;
}

/*
 * ���ƣ�
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int set_eop_mp4( hnd_t handle, x264_picture_t *p_picture )
{
    mp4_t *p_mp4 = (mp4_t *)handle;
    uint64_t dts = (uint64_t)p_mp4->i_numframe * p_mp4->i_time_inc;
    uint64_t pts = (uint64_t)p_picture->i_pts;
    int32_t offset = p_mp4->i_init_delay + pts - dts;

    p_mp4->p_sample->IsRAP = p_picture->i_type == X264_TYPE_IDR ? 1 : 0;
    p_mp4->p_sample->DTS = dts;
    p_mp4->p_sample->CTS_Offset = offset;
    gf_isom_add_sample(p_mp4->p_file, p_mp4->i_track, p_mp4->i_descidx, p_mp4->p_sample);

    p_mp4->p_sample->dataLength = 0;
    p_mp4->i_numframe++;

    return 0;
}

#endif


/* -- mkv muxing support ------------------------------------------------- */
typedef struct
{
    mk_Writer *w;

    uint8_t   *sps, *pps;
    int       sps_len, pps_len;//���в�������ͼ��������ĳ���

    int       width, height, d_width, d_height;

    int64_t   frame_duration;//uration:������ʱ��
    int       fps_num;

    int       b_header_written;
    char      b_writing_frame;
} mkv_t;


/*
 * ���ƣ�дͷ��
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int write_header_mkv( mkv_t *p_mkv )
{
    int       ret;
    uint8_t   *avcC;
    int  avcC_len;

    if( p_mkv->sps == NULL || p_mkv->pps == NULL ||
        p_mkv->width == 0 || p_mkv->height == 0 ||
        p_mkv->d_width == 0 || p_mkv->d_height == 0)
        return -1;

    avcC_len = 5 + 1 + 2 + p_mkv->sps_len + 1 + 2 + p_mkv->pps_len;
    avcC = malloc(avcC_len);
    if (avcC == NULL)
        return -1;

    avcC[0] = 1;
    avcC[1] = p_mkv->sps[1];
    avcC[2] = p_mkv->sps[2];
    avcC[3] = p_mkv->sps[3];
    avcC[4] = 0xff; // nalu size length is four bytes
    avcC[5] = 0xe1; // one sps

    avcC[6] = p_mkv->sps_len >> 8;
    avcC[7] = p_mkv->sps_len;

    memcpy(avcC+8, p_mkv->sps, p_mkv->sps_len);

    avcC[8+p_mkv->sps_len] = 1; // one pps
    avcC[9+p_mkv->sps_len] = p_mkv->pps_len >> 8;
    avcC[10+p_mkv->sps_len] = p_mkv->pps_len;

    memcpy( avcC+11+p_mkv->sps_len, p_mkv->pps, p_mkv->pps_len );

    ret = mk_writeHeader( p_mkv->w, "x264", "V_MPEG4/ISO/AVC",
                          avcC, avcC_len, p_mkv->frame_duration, 50000,
                          p_mkv->width, p_mkv->height,
                          p_mkv->d_width, p_mkv->d_height );

    free( avcC );

    p_mkv->b_header_written = 1;

    return ret;
}

/*
 * ���ƣ�дͷ��
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int open_file_mkv( char *psz_filename, hnd_t *p_handle )
{
    mkv_t *p_mkv;

    *p_handle = NULL;
	printf("\n(muxer.c) open_file_mkv(...) done");//zjh ����ļ�Ϊ.mkvʱ ���������--crf 22 -o test.mkv hall_cif.yuv 352x288

    p_mkv  = malloc(sizeof(*p_mkv));//��̬�ڴ����룬���ڽṹ��mkv_t
    if (p_mkv == NULL)//�ڴ�����ʧ���˳�
        return -1;

    memset(p_mkv, 0, sizeof(*p_mkv));//��ʼ��Ϊ00000...

    p_mkv->w = mk_createWriter(psz_filename);//���ڲ���һ��w->fp = fopen(filename, "wb");��ʵ��������ļ���д�뷽ʽ�򿪵Ķ���
    if (p_mkv->w == NULL)
    {
        free(p_mkv);//ʧ�ܵĻ������ͷŶ�̬������ڴ�
        return -1;
    }

    *p_handle = p_mkv;//��mkv_t�ṹ������ָ�봫��ȥ��������������

    return 0;
}


/*
 * ���ƣ�
 * ������
 * ���ܣ�����mkv��������Ե���x264_param_t
 * ˼·��
 * ���ϣ�
*/
int set_param_mkv( hnd_t handle, x264_param_t *p_param )
{
    mkv_t   *p_mkv = handle;
    int64_t dw, dh;

    if( p_param->i_fps_num > 0 )
    {
        p_mkv->frame_duration = (int64_t)p_param->i_fps_den *
                                (int64_t)1000000000 / p_param->i_fps_num;
        p_mkv->fps_num = p_param->i_fps_num;
    }
    else
    {
        p_mkv->frame_duration = 0;
        p_mkv->fps_num = 1;
    }

    p_mkv->width = p_param->i_width;
    p_mkv->height = p_param->i_height;

    if( p_param->vui.i_sar_width && p_param->vui.i_sar_height )
    {
        dw = (int64_t)p_param->i_width  * p_param->vui.i_sar_width;
        dh = (int64_t)p_param->i_height * p_param->vui.i_sar_height;
    }
    else
    {
        dw = p_param->i_width;
        dh = p_param->i_height;
    }

    if( dw > 0 && dh > 0 )
    {
        int64_t a = dw, b = dh;

        for (;;)
        {
            int64_t c = a % b;
            if( c == 0 )
              break;
            a = b;
            b = c;
        }

        dw /= b;
        dh /= b;
    }

    p_mkv->d_width = (int)dw;
    p_mkv->d_height = (int)dh;

    return 0;
}


/*
 * ���ƣ�
 * ������
 * ���ܣ�дNAL��Ԫ����Nalд�뵽һ���ļ�
 * ˼·��
 * ���ϣ�
*/
int write_nalu_mkv( hnd_t handle, uint8_t *p_nalu, int i_size )
{
    mkv_t *p_mkv = handle;
    uint8_t type = p_nalu[4] & 0x1f;//typedef unsigned char   uint8_t;
    uint8_t dsize[4];
    int psize;

    switch( type )
    {
    // sps//SPS(���в�����)
    case 0x07:		//[�Ϻ�ܣ�Page159 nal_uint_type�����] nal_uint_type=7�������в�����
        if( !p_mkv->sps )
        {
            p_mkv->sps = malloc(i_size - 4);
            if (p_mkv->sps == NULL)
                return -1;
            p_mkv->sps_len = i_size - 4;
            memcpy(p_mkv->sps, p_nalu + 4, i_size - 4);
        }
        break;

    // pps//PPS(ͼ�������)
    case 0x08:		/* [�Ϻ�ܣ�Page159 nal_uint_type�����] nal_uint_type=8����ͼ������� */
        if( !p_mkv->pps )
        {
            p_mkv->pps = malloc(i_size - 4);
            if (p_mkv->pps == NULL)
                return -1;
            p_mkv->pps_len = i_size - 4;
            memcpy(p_mkv->pps, p_nalu + 4, i_size - 4);
        }
        break;

    // slice, sei
    case 0x1:	//[�Ϻ�ܣ�Page159 nal_uint_type�����] nal_uint_type=1���ǲ���������IDRͼ���Ƭ
    case 0x5:	//[�Ϻ�ܣ�Page159 nal_uint_type�����] nal_uint_type=5����IDRͼ���е�Ƭ
    case 0x6:	//[�Ϻ�ܣ�Page159 nal_uint_type�����] nal_uint_type=5���ǲ�����ǿ��Ϣ��Ԫ(SEI)
        if( !p_mkv->b_writing_frame )
        {
            if( mk_startFrame(p_mkv->w) < 0 )
                return -1;
            p_mkv->b_writing_frame = 1;
        }
        psize = i_size - 4 ;
        dsize[0] = psize >> 24;
        dsize[1] = psize >> 16;
        dsize[2] = psize >> 8;
        dsize[3] = psize;
        if( mk_addFrameData(p_mkv->w, dsize, 4) < 0 ||
            mk_addFrameData(p_mkv->w, p_nalu + 4, i_size - 4) < 0 )
            return -1;
        break;

    default:
        break;
    }

    if( !p_mkv->b_header_written && p_mkv->pps && p_mkv->sps &&
        write_header_mkv(p_mkv) < 0 )
        return -1;

    return i_size;
}

/*
 * ���ƣ�
 * ������
 * ���ܣ�
 * ˼·��
 * ���ϣ�
*/
int set_eop_mkv( hnd_t handle, x264_picture_t *p_picture )
{
    mkv_t *p_mkv = handle;		/*  */
    int64_t i_stamp = (int64_t)(p_picture->i_pts * 1e9 / p_mkv->fps_num);	/*  */

    p_mkv->b_writing_frame = 0;	/*  */

    return mk_setFrameFlags( p_mkv->w, i_stamp,
                             p_picture->i_type == X264_TYPE_IDR );	/*  */
}


/*
 * ���ƣ�
 * ������
 * ���ܣ��ر�mkv�ļ�
 * ˼·��
 * ���ϣ�
*/
int close_file_mkv( hnd_t handle )
{
    mkv_t *p_mkv = handle;
    int   ret;

    if( p_mkv->sps )
        free( p_mkv->sps );
    if( p_mkv->pps )
        free( p_mkv->pps );

    ret = mk_close(p_mkv->w);

    free( p_mkv );

    return ret;
}

/*
typedef struct {

    DWORD fccType;    

    DWORD fccHandler; 

    DWORD dwFlags; 

    DWORD dwCaps; 

    WORD  wPriority; 

    WORD  wLanguage; 

    DWORD dwScale; 

    DWORD dwRate; 

    DWORD dwStart; 

    DWORD dwLength; 

    DWORD dwInitialFrames; 

    DWORD dwSuggestedBufferSize; 

    DWORD dwQuality; 

    DWORD dwSampleSize; 

    RECT  rcFrame; 

    DWORD dwEditCount; 

    DWORD dwFormatChangeCount; 

    TCHAR szName[64]; 

} AVISTREAMINFO; 



*/

int tst()
{
return 0;
}