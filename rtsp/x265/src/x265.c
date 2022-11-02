#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "x265.h"
#include "gx265.h"

#include "fifo.h"

extern void yuyv_to_yuv420P(unsigned char *in, unsigned char *out, int width, int height);

int x265_venc_init(struct codec_x265 *c)
{
	c->pNals = NULL;
	c->iNal = 0;

	c->pParam = NULL;
	c->pHandle = NULL;
	c->pPic_in = NULL;

	c->frame_num = 0;
	c->csp = X265_CSP_I420;
	c->width = 640;
	c->height = 360;

	c->pParam = x265_param_alloc();
	x265_param_default(c->pParam);
	c->pParam->bRepeatHeaders = 1;//write sps,pps before keyframe
	c->pParam->internalCsp = c->csp;
	c->pParam->sourceWidth = c->width;
	c->pParam->sourceHeight = c->height;
	c->pParam->fpsNum = 25;
	c->pParam->fpsDenom = 1;
	{
		/*log close*/
		c->pParam->logLevel = 0;
		/*sei close*/
		c->pParam->bEmitInfoSEI = 0;
		/*gop set*/
		c->pParam->keyframeMax = 8;
		/*b frame close*/
		c->pParam->bframes = 0;
		/*first gop slice IDR set*/
		c->pParam->bOpenGOP = 0;
		/*sao*/
		c->pParam->bEnableSAO = 1;
		/*qp*/
		c->pParam->bOptQpPPS = 25;
		/*rc*/
		c->pParam->rc.qp = 25;
		c->pParam->rc.qpMin = 15;
		c->pParam->rc.qpMax = 35;
	}

	c->pHandle = x265_encoder_open(c->pParam);

	if(c->pHandle == NULL)
	{
		printf("x265_encoder_open err\n");
		return 0;
	}

	c->y_size = c->width * c->height;

	c->pPic_in = x265_picture_alloc();

	x265_picture_init(c->pParam, c->pPic_in);

	c->pPic_in->stride[0] = c->width;
	c->pPic_in->stride[1] = c->width/2;
	c->pPic_in->stride[2] = c->width/2;

	c->xbuf = (unsigned char *)malloc(c->y_size*3/2);

	return 0;
}

int x265_venc_exit(struct codec_x265 *c)
{
	free(c->xbuf);

	x265_encoder_close(c->pHandle);
	x265_picture_free(c->pPic_in);
	x265_param_free(c->pParam);

	return 0;
}

int x265_venc(struct codec_x265 *c, struct sfifo_des_s *gbuf_p1, struct sfifo_des_s *gbuf_p2)
{
	static unsigned int cnt=0;
	int j;
	struct sfifo_s *yuv;
	struct sfifo_s *bs;
	uint32_t iNal = 0;
	yuv = sfifo_get_active_buf(gbuf_p1);
	bs = sfifo_get_free_buf(gbuf_p2);
	bs->cur_size = 0;
	if (yuv != NULL) {

		yuyv_to_yuv420P(yuv->buffer, c->xbuf, 640, 360);
		sfifo_put_free_buf(yuv, gbuf_p1);
	}

	/*yuv420p*/
	c->pPic_in->planes[0] = &c->xbuf[c->y_size*3/2];						//Y
	c->pPic_in->planes[1] = &c->xbuf[c->y_size*3/2+c->y_size];				//U
	c->pPic_in->planes[2] = &c->xbuf[c->y_size*3/2+c->y_size+c->y_size/4];	//V

	x265_encoder_encode(c->pHandle, &c->pNals, &iNal, c->pPic_in, NULL);	

	for(j=0;j<iNal;j++)
	{
		memcpy(&bs->buffer[bs->cur_size], c->pNals[j].payload, c->pNals[j].sizeBytes);
		bs->cur_size+=c->pNals[j].sizeBytes;
	}	
	//Flush Decoder
	while(1)
	{
		if((x265_encoder_encode(c->pHandle, &c->pNals, &iNal, NULL, NULL)) == 0){
			break;
		}
		for(j=0;j<iNal;j++)
		{
			memcpy(&bs->buffer[bs->cur_size], c->pNals[j].payload, c->pNals[j].sizeBytes);
			bs->cur_size += c->pNals[j].sizeBytes;
		}
	}
	sfifo_put_active_buf(bs, gbuf_p2);
	printf("[%s] [%d] encode : %8d size : 0x%8x \n", __func__,__LINE__, cnt++, bs->cur_size);
	return 0;
}
