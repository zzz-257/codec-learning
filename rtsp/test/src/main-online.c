#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>

#include "comm.h"
#include "x265.h"
#include "x264.h"
#include "fifo.h"
#include "rtsp_demo.h"

static int flag_run = 1;

static void sig_proc(int signo)
{
	flag_run = 0;
	printf("%s[%d]\n",__func__,__LINE__);
}

pthread_t v4l2;
pthread_t x265;
pthread_t rtsp;

struct sfifo_des_s *gbuf_p2;
struct sfifo_des_s *gbuf_p1;

extern void v4l2_init(void);
extern void v4l2_exit(void);
extern void v4l2_get_yuv(struct sfifo_des_s *gbuf);
void *v4l2_thread(void *args)
{
	v4l2_init();
	while (flag_run) {

		v4l2_get_yuv(gbuf_p1);
	}
	v4l2_exit();
	printf("%s[%d]\n",__func__,__LINE__);
	return 0;
}


extern void yuyv_to_yuv420P(unsigned char *in, unsigned char *out, int width, int height);
extern int x265_venc(struct sfifo_des_s *gbuf_p1, struct sfifo_des_s *gbuf_p2);
extern int x264_venc(struct sfifo_des_s *gbuf_p1, struct sfifo_des_s *gbuf_p2);

void *x265_thread(void *args)
{
	while (flag_run) {

		//	x264_venc(gbuf_p1, gbuf_p2);
		x265_venc(gbuf_p1, gbuf_p2);
	}
	printf("%s[%d]\n",__func__,__LINE__);
	return 0;
}

void *rtsp_thread(void *args)
{
	rtsp_demo_handle demo;
	rtsp_session_handle session = NULL;
	uint8_t *vbuf = NULL;
	uint64_t ts = 0;
	int ret;

	demo = rtsp_new_demo(10554);//rtsp sever socket
	if (NULL == demo) {
		printf("rtsp_new_demo failed\n");
		//	return 0;
	}

	session = rtsp_new_session(demo, "/live");//对应rtsp session 
	if (NULL == session) {
		printf("rtsp_new_session failed\n");
		//	return 0;
	}

	int codec_id = RTSP_CODEC_ID_VIDEO_H265;
	rtsp_set_video(session, codec_id, NULL, 0);
	rtsp_sync_video_ts(session, rtsp_get_reltime(), rtsp_get_ntptime());

	printf("rtsp://192.168.2.10:10554/live\n");

	while (flag_run) {
		ts = rtsp_get_reltime();
		uint8_t type = 0;

		struct sfifo_s *bs;
read_video_again:
		/*读取nalu*/
		bs = sfifo_get_active_buf(gbuf_p2);
		vbuf = bs->buffer;
		if (vbuf != NULL) {

			/*传送nalu*/
			if (session)//1源session 存存
			{
				rtsp_tx_video(session, vbuf, bs->cur_size, ts);//2rtsp_client_connect存在
			}

			type = 0;
			if (vbuf[0] == 0 && vbuf[1] == 0 && vbuf[2] == 1) {

				if(codec_id==RTSP_CODEC_ID_VIDEO_H264)
					type = vbuf[3] & 0x1f;
				else if(codec_id==RTSP_CODEC_ID_VIDEO_H265)
					type = vbuf[3]>>1 & 0x3f;
			}

			if (vbuf[0] == 0 && vbuf[1] == 0 && vbuf[2] == 0 && vbuf[3] == 1) {

				if(codec_id==RTSP_CODEC_ID_VIDEO_H264)
					type = vbuf[4] & 0x1f;
				else if(codec_id==RTSP_CODEC_ID_VIDEO_H265)
					type = vbuf[4]>>1 & 0x3f;
			}

			printf("%s[%d] type: %d \n",__func__,__LINE__, type);

			if(codec_id==RTSP_CODEC_ID_VIDEO_H264)
			{
				if (type != 5 && type != 1)
				{
					sfifo_put_free_buf(bs, gbuf_p2);
					goto read_video_again;
				}
			}

			if(codec_id==RTSP_CODEC_ID_VIDEO_H264)
			{
				if (!(type>0 && type<22))
				{
					sfifo_put_free_buf(bs, gbuf_p2);
					goto read_video_again;
				}
			}
			sfifo_put_free_buf(bs, gbuf_p2);
		}
		else
		{
			printf("%s[%d] ####################################\n",__func__,__LINE__);
			sfifo_put_free_buf(bs, gbuf_p2);
			goto read_video_again;
		}

		do {
			ret = rtsp_do_event(demo);//
			if (ret > 0)
				continue;
			if (ret < 0)
				break;
			usleep(20000);
		} while (rtsp_get_reltime() - ts < 1000000 / 25);
		if (ret < 0)
			break;
		ts += 1000000 / 25;
	}

	free(vbuf);

	if (session)
		rtsp_del_session(session);

	rtsp_del_demo(demo);
	printf("%s[%d]\n",__func__,__LINE__);
	return 0;
}

int main(int argc, char *argv[])
{

	int sfifo_num = 10;
	int sfifo_buffer_size = 640*360*2;

	signal(SIGINT, sig_proc);

	/*fifo gbuf_p2 init*/
	gbuf_p2 = sfifo_init(sfifo_num, sfifo_buffer_size);
	/*fifo gbuf_p1 init*/
	gbuf_p1 = sfifo_init(sfifo_num, sfifo_buffer_size);

	/*pthread v4l2 init*/
	pthread_create(&v4l2,NULL,v4l2_thread,NULL);
	/*pthread x265 init*/
	pthread_create(&x265,NULL,x265_thread,NULL);
	/*pthread rtsp init*/
	pthread_create(&rtsp,NULL,rtsp_thread,NULL);

	pthread_join(rtsp,NULL);printf("%s[%d] rtsp exit\n",__func__,__LINE__);
	pthread_join(x265,NULL);printf("%s[%d] x265 exit\n",__func__,__LINE__);
	pthread_join(v4l2,NULL);printf("%s[%d] v4l2 exit\n",__func__,__LINE__);

	/*fifo gbuf_p2 exit*/
	free(gbuf_p2);
	/*fifo gbuf_p1 exit*/
	free(gbuf_p1);

	printf("%s[%d]\n",__func__,__LINE__);
	return 0;
}
