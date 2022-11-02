#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "x265.h"

#include "fifo.h"

extern void yuyv_to_yuv420P(unsigned char *in, unsigned char *out, int width, int height);

int x265_venc(struct sfifo_des_s *gbuf_p1, struct sfifo_des_s *gbuf_p2)
{
	static unsigned int cnt=0;
	int i,j;
	int y_size;
	char *buff=NULL;
	int ret;
	x265_nal *pNals=NULL;
	uint32_t iNal=0;

	x265_param* pParam=NULL;
	x265_encoder* pHandle=NULL;
	x265_picture *pPic_in=NULL;

	//Encode 50 frame
	//if set 0, encode all frame
	int frame_num=0;
	int csp=X265_CSP_I420;
	int width=640,height=360;

	pParam=x265_param_alloc();
	x265_param_default(pParam);
	pParam->bRepeatHeaders=1;//write sps,pps before keyframe
	pParam->internalCsp=csp;
	pParam->sourceWidth=width;
	pParam->sourceHeight=height;
	pParam->fpsNum=25;
	pParam->fpsDenom=1;
	{
		/*log close*/
		pParam->logLevel=0;
		/*sei close*/
		pParam->bEmitInfoSEI=0;
		/*gop set*/
		pParam->keyframeMax=10;
		/*b frame close*/
		pParam->bframes=0;
		/*first gop slice IDR set*/
		pParam->bOpenGOP=0;
#if 1
		/*sao*/
		pParam->bEnableSAO=1;
		/*qp*/
		pParam->bOptQpPPS=25;

		/*rc*/
		pParam->rc.qp=25;
		pParam->rc.qpMin=15;
		pParam->rc.qpMax=35;
#endif
	}

	//Init
	pHandle=x265_encoder_open(pParam);

	if(pHandle==NULL)
	{
		printf("x265_encoder_open err\n");
		return 0;
	}

	y_size = pParam->sourceWidth * pParam->sourceHeight;

	pPic_in = x265_picture_alloc();
	x265_picture_init(pParam,pPic_in);

	pPic_in->stride[0]=width;
	pPic_in->stride[1]=width/2;
	pPic_in->stride[2]=width/2;

	struct sfifo_s *yuv;
	struct sfifo_s *bs;
	unsigned char *xbuf = (unsigned char *)malloc(640*360*3/2);
	assert(xbuf);

//	while(1)
	{
		frame_num = 10;
		//Loop to Encode
		for( i=0;i<frame_num;i++)
		{
			yuv = sfifo_get_active_buf(gbuf_p1);
			bs = sfifo_get_free_buf(gbuf_p2);
			bs->cur_size = 0;
			assert(yuv);
			if (yuv != NULL) {

				yuyv_to_yuv420P(yuv->buffer, xbuf, 640, 360);
				sfifo_put_free_buf(yuv, gbuf_p1);
			}

			/*yuv420p*/
			pPic_in->planes[0] = &xbuf[y_size*3/2*0+0];		//Y
			pPic_in->planes[1] = &xbuf[y_size*3/2*0+y_size];	//U
			pPic_in->planes[2] = &xbuf[y_size*3/2*0+y_size+y_size/4];	//V

			ret = x265_encoder_encode(pHandle,&pNals,&iNal,pPic_in,NULL);	
			if(ret<0)
				printf("%s[%d] ret:%d \n",__func__,__LINE__,ret);

			for(j=0;j<iNal;j++)
			{
				memcpy(&bs->buffer[bs->cur_size], pNals[j].payload, pNals[j].sizeBytes);
				bs->cur_size+=pNals[j].sizeBytes;
			}	
			//Flush Decoder
			while(1)
			{
				ret=x265_encoder_encode(pHandle,&pNals,&iNal,NULL,NULL);
				if(ret==0){
					break;
				}
				//	printf("Flush 1 frame.\n");

				for(j=0;j<iNal;j++)
				{
					memcpy(&bs->buffer[bs->cur_size], pNals[j].payload, pNals[j].sizeBytes);
					bs->cur_size+=pNals[j].sizeBytes;
				}
			}
			sfifo_put_active_buf(bs, gbuf_p2);
			printf("[%s] [%d] ########Succeed encode frame : %d cur_size : 0x%x ######## \n", __func__,__LINE__, cnt++, bs->cur_size);
		}
	}
	free(xbuf);

	x265_encoder_close(pHandle);
	x265_picture_free(pPic_in);
	x265_param_free(pParam);
	free(buff);

	printf("%s[%d]\n",__func__,__LINE__);

	return 0;
}
