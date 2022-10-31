#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "x264.h"

#if 1
#include "x265.h"
int x265_encode(const char* infile, int width, int height, int type, const char* outfile)
{

	FILE *fp_src = NULL;
	FILE *fp_dst = NULL;
	unsigned int luma_size = 0;
	unsigned int chroma_size = 0;
	//int buff_size = 0;
	char *buff = NULL;
	uint32_t i_nal=0;
	int i_frame=0;
	int ret = 0;

	x265_param param;
	x265_nal *nal = NULL;
	x265_encoder *handle = NULL;
	x265_picture *pic_in = NULL;

	int csp = type; // X265_CSP_I420;

	fp_src = fopen(infile, "rb");
	fp_dst = fopen(outfile, "wb");
	if(fp_src==NULL || fp_dst==NULL)
	{

		perror("Error open yuv files:");
		return -1;
	}

	x265_param_default(&param);
	param.bRepeatHeaders = 1;//write sps,pps before keyframe
	param.internalCsp = csp;
	param.sourceWidth = width;
	param.sourceHeight = height;
	param.fpsNum = 25; // 帧率
	param.fpsDenom = 1; // 帧率

	handle = x265_encoder_open(&param);
	if(handle == NULL)
	{

		printf("x265_encoder_open err\n");
		goto out;
	}

	pic_in = x265_picture_alloc();
	if (pic_in == NULL)
	{

		goto out;
	}
	x265_picture_init(&param, pic_in);

	// Y分量大小
	luma_size = param.sourceWidth * param.sourceHeight;
	// 分量一帧YUV的空间
	switch (csp)
	{

		case X265_CSP_I444:
			buff = (char *)malloc(luma_size*3);
			pic_in->planes[0] = buff;
			pic_in->planes[1] = buff + luma_size;
			pic_in->planes[2] = buff + luma_size*2;
			pic_in->stride[0] = width;
			pic_in->stride[1] = width;
			pic_in->stride[2] = width;
			break;
		case X265_CSP_I420:
			buff = (char *)malloc(luma_size*3/2);
			pic_in->planes[0] = buff;
			pic_in->planes[1] = buff + luma_size;
			pic_in->planes[2] = buff + luma_size*5/4;
			pic_in->stride[0] = width;
			pic_in->stride[1] = width/2;
			pic_in->stride[2] = width/2;
			break;
		default:
			printf("Colorspace Not Support.\n");
			goto out;
	}

	// 计算总帧数
	fseek(fp_src, 0, SEEK_END);
	switch(csp)
	{

		case X265_CSP_I444:
			i_frame = ftell(fp_src) / (luma_size*3);
			chroma_size = luma_size;
			break;
		case X265_CSP_I420:
			i_frame = ftell(fp_src) / (luma_size*3/2);
			chroma_size = luma_size / 4;
			break;
		default:
			printf("Colorspace Not Support.\n");
			return -1;
	}
	fseek(fp_src,0,SEEK_SET);
	printf("framecnt: %d, y: %d u: %d\n", i_frame, luma_size, chroma_size);

	for(int i=0; i < i_frame; i++)
	{

		switch(csp)
		{

			case X265_CSP_I444:
			case X265_CSP_I420:
				if (fread(pic_in->planes[0], 1, luma_size, fp_src) != luma_size)
					break;
				if (fread(pic_in->planes[1], 1, chroma_size, fp_src) != chroma_size)
					break;
				if (fread(pic_in->planes[2], 1, chroma_size, fp_src) != chroma_size)
					break;
				break;
			default:
				printf("Colorspace Not Support.\n");
				goto out;
		}

		ret = x265_encoder_encode(handle, &nal, &i_nal, pic_in, NULL);	
		printf("encode frame: %5d framesize: %d nal: %d\n", i+1, ret, i_nal);
		if (ret < 0)
		{

			printf("Error encode frame: %d.\n", i+1);
			goto out;
		}

		for(uint32_t j = 0; j < i_nal; j++)
		{

			fwrite(nal[j].payload, 1, nal[j].sizeBytes, fp_dst);
		}
	}
	// Flush Decoder
	while((ret = x265_encoder_encode(handle, &nal, &i_nal, NULL, NULL)))
	{

		static int cnt = 1;
		printf("flush frame: %d\n", cnt++);
		for(uint32_t j=0; j < i_nal; j++)
		{

			fwrite(nal[j].payload, 1, nal[j].sizeBytes, fp_dst);
		}
	}

out:
	x265_encoder_close(handle);
	x265_picture_free(pic_in);
	if (buff)
		free(buff);

	fclose(fp_src);
	fclose(fp_dst);

	return 0;
}
#endif

int x264_encode(const char* infile, int width, int height, int type, const char* outfile)
{

	FILE *fp_src = NULL;
	FILE *fp_dst = NULL;
	unsigned int luma_size = 0;
	unsigned int chroma_size = 0;
	int i_nal = 0;
	int i_frame = 0;
	unsigned int i_frame_size = 0;

	int csp = type;//X264_CSP_I420;
	x264_nal_t* nal = NULL;
	x264_t* handle = NULL;
	x264_picture_t* pic_in = NULL;
	x264_picture_t* pic_out = NULL;
	x264_param_t param;

	fp_src = fopen(infile, "rb");
	fp_dst = fopen(outfile, "wb");
	if(fp_src==NULL || fp_dst==NULL)
	{

		perror("Error open yuv files:");
		return -1;
	}

	x264_param_default(&param);

	param.i_width  = width;
	param.i_height = height;
	/*
	 *     //Param
	 *         param.i_log_level  = X264_LOG_DEBUG;
	 *             param.i_threads  = X264_SYNC_LOOKAHEAD_AUTO;
	 *                 param.i_frame_total = 0;
	 *                     param.i_keyint_max = 10;
	 *                         param.i_bframe  = 5;
	 *                             param.b_open_gop  = 0;
	 *                                 param.i_bframe_pyramid = 0;
	 *                                     param.rc.i_qp_constant=0;
	 *                                         param.rc.i_qp_max=0;
	 *                                             param.rc.i_qp_min=0;
	 *                                                 param.i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
	 *                                                     param.i_fps_den  = 1;
	 *                                                         param.i_fps_num  = 25;
	 *                                                             param.i_timebase_den = param.i_fps_num;
	 *                                                                 param.i_timebase_num = param.i_fps_den;
	 *                                                                     */
	param.i_csp = csp;
	param.b_repeat_headers = 1;
	param.b_annexb = 1;

	// profile: high x264_profile_names定义见x264.h
	x264_param_apply_profile(&param, x264_profile_names[2]);

	pic_in = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	if (pic_in == NULL)
	{

		goto out;
	}
	pic_out = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	if (pic_out == NULL)
	{

		goto out;
	}
	x264_picture_init(pic_out);
	x264_picture_alloc(pic_in, csp, param.i_width, param.i_height);

	handle = x264_encoder_open(&param);

	// 计算有多少帧
	luma_size = param.i_width * param.i_height;
	fseek(fp_src, 0, SEEK_END);
	switch (csp)
	{

		case X264_CSP_I444:
			i_frame = ftell(fp_src) / (luma_size*3);
			chroma_size = luma_size;
			break;
		case X264_CSP_I420:
			i_frame = ftell(fp_src) / (luma_size*3/2);
			chroma_size = luma_size / 4;
			break;
		default:
			printf("Colorspace Not Support.\n");
			goto out;
	}
	fseek(fp_src, 0, SEEK_SET);

	printf("framecnt: %d, y: %d u: %d\n", i_frame, luma_size, chroma_size);

	// 编码
	for (int i = 0; i < i_frame; i++)
	{

		switch(csp)
		{

			case X264_CSP_I444:
			case X264_CSP_I420:
				i_frame_size = fread(pic_in->img.plane[0], 1, luma_size, fp_src);
				if (i_frame_size != luma_size)
				{

					printf("read luma failed %d.\n", i_frame_size);
					break;
				}
				i_frame_size = fread(pic_in->img.plane[1], 1, chroma_size, fp_src);
				if (i_frame_size != chroma_size)
				{

					printf("read chroma1 failed %d.\n", i_frame_size);
					break;
				}
				i_frame_size = fread(pic_in->img.plane[2], 1, chroma_size, fp_src);
				if (i_frame_size != chroma_size)
				{

					printf("read chrome2 failed %d.\n", i_frame_size);
					break;
				}
				break;
			default:
				printf("Colorspace Not Support.\n");
				return -1;
		}
		pic_in->i_pts = i;

		i_frame_size = x264_encoder_encode(handle, &nal, &i_nal, pic_in, pic_out);
		printf("encode frame: %5d framesize: %d nal: %d\n", i+1, i_frame_size, i_nal);
		if (i_frame_size < 0)
		{

			printf("Error encode frame: %d.\n", i+1);
			goto out;
		}
#if 01
		else if (i_frame_size)
		{

			if(!fwrite(nal->p_payload, 1, i_frame_size, fp_dst))
				goto out;
		}
#endif
		// 另一种做法
#if 0
		// i_nal有可能为0，有可能多于1
		for (int j = 0; j < i_nal; j++)
		{

			fwrite(nal[j].p_payload, 1, nal[j].i_payload, fp_dst);
		}
#endif
	}

	//flush encoder
	while(x264_encoder_delayed_frames(handle))
		//while(1)
	{

		static int cnt = 1;
		i_frame_size = x264_encoder_encode(handle, &nal, &i_nal, NULL, pic_out);
		printf("flush frame: %d framesize: %d nalsize: %d\n", cnt++, i_frame_size, i_nal);
		if(i_frame_size == 0)
		{

			break;
			//goto out;
		}
#if 01
		else if(i_frame_size)
		{

			if(!fwrite(nal->p_payload, 1, i_frame_size, fp_dst))
				goto out;
		}
#endif
		// 另一种做法
#if 0
		// i_nal有可能为0，有可能多于1
		for (int j = 0; j < i_nal; j++)
		{

			fwrite(nal[j].p_payload, 1, nal[j].i_payload, fp_dst);
		}
#endif
	}

out:
	x264_picture_clean(pic_in);
	if (handle)
	{

		x264_encoder_close(handle);
		handle = NULL;
	}

	if (pic_in)
		free(pic_in);
	if (pic_out)
		free(pic_out);

	fclose(fp_src);
	fclose(fp_dst);

	return 0;
}
