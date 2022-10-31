#include <stdint.h>

void yuyv_to_yuv420P(uint8_t *in, uint8_t*out, int width, int height)  
{

	uint8_t *y, *u, *v;  
	int i, j, offset = 0, yoffset = 0;  

	y = out;                                    //yuv420的y放在前面  
	u = out + (width * height);                 //yuv420的u放在y后  
	v = out + (width * height * 5 / 4);         //yuv420的v放在u后  
	//总共size = width * height * 3 / 2  

	for(j = 0; j < height; j++)  
	{

		yoffset = 2 * width * j;  
		for(i = 0; i < width * 2; i = i + 4)  
		{

			offset = yoffset + i;  
			*(y ++) = *(in + offset);  
			*(y ++) = *(in + offset + 2);  
			if(j % 2 == 1)                      //抛弃奇数行的UV分量  
			{

				*(u ++) = *(in + offset + 1);  
				*(v ++) = *(in + offset + 3);  
			}  
		}  
	}  
}  
