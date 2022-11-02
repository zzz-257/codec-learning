#ifndef __GX265_H__
#define __GX265_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "x265.h"

struct codec_x265 {

	x265_encoder* pHandle;
	x265_param* pParam;
	x265_picture *pPic_in;
	x265_nal *pNals;

	int y_size;
	int iNal;

	int frame_num;
	int csp;
	int width;
	int height;

	unsigned char *xbuf;
};
#endif
