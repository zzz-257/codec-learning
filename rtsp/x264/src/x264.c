#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <x264.h>

#include "fifo.h"

extern void yuyv_to_yuv420P(unsigned char *in, unsigned char *out, int width, int height);

int x264_venc(struct sfifo_des_s *gbuf_p1, struct sfifo_des_s *gbuf_p2)
{
	int width  = 640;
	int height = 480;

	x264_param_t param;

	x264_picture_t pic;
	x264_picture_t pic_out;
	x264_t *h;
	int i_frame = 0;
	int i_frame_size;

	x264_nal_t *nal;
	int i_nal;

	x264_param_default_preset( &param, "medium", NULL );

	param.i_bitdepth = 8;
	param.i_csp = X264_CSP_I420;
	param.i_width  = width;
	param.i_height = height;
	param.b_vfr_input = 0;
	param.b_repeat_headers = 1;
	param.b_annexb = 1;
	x264_param_apply_profile( &param, "main" );
	x264_picture_alloc( &pic, param.i_csp, param.i_width, param.i_height );

	//打开编码器
	h = x264_encoder_open( &param );

	int luma_size = width * height;
//	int chroma_size = luma_size / 4;

	struct sfifo_s *yuv;
	struct sfifo_s *bs;
	unsigned char *xbuf = (unsigned char*)malloc(luma_size*3/2);
	assert(xbuf);

	/* Encode frames */
	int i,j;
	static int cnt=0;
	while(1)
	{
		i_nal=0;
		for( i=0; i < 4; i++, i_frame++)
		{
			yuv = sfifo_get_active_buf(gbuf_p1);
			bs = sfifo_get_free_buf(gbuf_p2);
			bs->cur_size = 0;
			assert(yuv);
			assert(bs);
			if (yuv != NULL) {

				yuyv_to_yuv420P(yuv->buffer, xbuf, 640, 480);
				sfifo_put_free_buf(yuv, gbuf_p1);
			}
#if 0
			memcpy( pic.img.plane[0], &xbuf[0], luma_size);								//Y
			memcpy( pic.img.plane[1], &xbuf[luma_size], chroma_size);					//U
			memcpy( pic.img.plane[2], &xbuf[luma_size+luma_size/4], chroma_size );		//V
#else

			assert(xbuf);
			pic.img.plane[0] = &xbuf[0];						//Y
			pic.img.plane[1] = &xbuf[luma_size];				//U
			pic.img.plane[2] = &xbuf[luma_size+luma_size/4];	//V
#endif

			pic.i_pts = i_frame;
			//编码一帧图像
			i_frame_size = x264_encoder_encode( h, &nal, &i_nal, &pic, &pic_out );
			printf("%s[%d] i_nal: %d i_frame_size 0x%x \n",__func__,__LINE__,i_nal, i_frame_size);
			if( i_frame_size )
			{

				for(j=0;j<i_nal;j++)
				{
					printf("%s[%d]\n",__func__,__LINE__);
					memcpy(&bs->buffer[bs->cur_size], nal[j].p_payload, i_frame_size);
					bs->cur_size+=i_frame_size;
				}
			}
			sfifo_put_active_buf(bs, gbuf_p2);
		}
#if 1
		while( x264_encoder_delayed_frames( h ) )	//返回当前延迟的 (缓冲的)帧的数量
		{
			bs = sfifo_get_free_buf(gbuf_p2);
			//编码一帧图像
			i_frame_size = x264_encoder_encode( h, &nal, &i_nal, NULL, &pic_out );
			printf("%s[%d] i_nal: %d i_frame_size 0x%x \n",__func__,__LINE__,i_nal, i_frame_size);
			if( i_frame_size )
			{
				for(j=0;j<i_nal;j++)
				{
					printf("%s[%d]\n",__func__,__LINE__);
					memcpy(&bs->buffer[bs->cur_size], nal[j].p_payload, i_frame_size);
					bs->cur_size+=i_frame_size;
				}
			}
			sfifo_put_active_buf(bs, gbuf_p2);
		}
#endif
		printf("[%s] [%d] ########Succeed encode frame : %d cur_size : 0x%x ######## \n", __func__,__LINE__, cnt++, bs->cur_size);
	}
	free(xbuf);
	//关闭编码器
	x264_encoder_close( h );
	//释放x264_picture_alloc()申请的资源
	x264_picture_clean( &pic );
	return 0;
}
