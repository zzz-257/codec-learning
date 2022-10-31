/**********************************************************************************************
* COPYRIGHT NOTICE
* Copyright (c) 2020
* All rights reserved
* @author :chenjp
* @file :/home/chenjianping/others/transplant/test\main.c
* @date :2022/10/28 05:58
* @description :
**********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "x264.h"
#include "x265.h"
#include "libavcodec/codec_id.h"

extern int x265_encode(const char* infile, int width, int height, int type, const char* outfile);
extern int x264_encode(const char* infile, int width, int height, int type, const char* outfile);
extern int ffmpeg_decode(const char* infile, int type, const char* outfile);
extern void yuyv_to_yuv420P(uint8_t *in, uint8_t*out, int width, int height);

int main(int argc,char *argv[]){

	char *in = "./yuv/640x360_yuv420p.yuv";
	char *out1 = "test.h264";
	char *out2 = "test.h265";

//	yuyv_to_yuv420P(buf1, buf2, 640, 360);

	x264_encode(in, 640, 360, X264_CSP_I420, out1);
	x265_encode(in, 640, 360, X265_CSP_I420, out2);

	ffmpeg_decode("test.h264", AV_CODEC_ID_H264, "test.yuv");

    return 0;
}
