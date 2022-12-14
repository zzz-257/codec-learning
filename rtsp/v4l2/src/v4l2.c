#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>          /* for videodev2.h */

#include <linux/videodev2.h>

#include "fifo.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))

typedef enum {

	IO_METHOD_READ, IO_METHOD_MMAP, IO_METHOD_USERPTR,
} io_method;

struct buffer {

	void * start;
	size_t length;
};

static char * dev_name = NULL;
static io_method io = IO_METHOD_MMAP;
static int fd = -1;
struct buffer * buffers = NULL;
static unsigned int n_buffers = 0;

static void errno_exit(const char * s) {

	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));

	exit(EXIT_FAILURE);
}

int xioctl(int fd, int request, void * arg) {

	int r;

	do {

		r = ioctl(fd, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

void yuyv_to_yuv420P(unsigned char *in, unsigned char *out, int width, int height)  
{
	unsigned char *y, *u, *v;  
	int i, j, offset = 0, yoffset = 0;  

	y = out;                                    //yuv420的y放在前面  
	u = out + (width * height);                 //yuv420的u放在y后  
	v = out + (width * height * 5 / 4);         //yuv420的v放在u后  

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

int read_frame(struct sfifo_s *sfifo) {

	struct v4l2_buffer buf;

	CLEAR(buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {

		switch (errno) {

			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit("VIDIOC_DQBUF");
		}
	}

	assert(buf.index < n_buffers);

	memcpy(sfifo->buffer, buffers[buf.index].start, buf.length);
	sfifo->cur_size = buf.length;

	if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
		errno_exit("VIDIOC_QBUF");

	return 1;
}

void v4l2_capture(struct sfifo_des_s *gbuf) {

	struct sfifo_s *sfifo;

	fd_set fds;
	struct timeval tv;
	int r;
start:

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	/* Timeout. */
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	r = select(fd + 1, &fds, NULL, NULL, &tv);

	if (-1 == r) {

		if (EINTR == errno)
			goto start;

		errno_exit("select");
	}

	if (0 == r) {

		fprintf(stderr, "select timeout\n");
		exit(EXIT_FAILURE);
	}

	sfifo = sfifo_get_free_buf(gbuf);
	if (sfifo != NULL) {

		if (read_frame(sfifo))
		{
			sfifo_put_active_buf(sfifo, gbuf);
		}
	}
}

static void stop_capturing(void) {

	enum v4l2_buf_type type;

	switch (io) {

		case IO_METHOD_READ:
			/* Nothing to do. */
			break;

		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
				errno_exit("VIDIOC_STREAMOFF");

			break;
	}
}

static void start_capturing(void) {

	unsigned int i;
	enum v4l2_buf_type type;

	switch (io) {

		case IO_METHOD_READ:
			/* Nothing to do. */
			break;

		case IO_METHOD_MMAP:
			for (i = 0; i < n_buffers; ++i) {

				struct v4l2_buffer buf;

				CLEAR(buf);

				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_MMAP;
				buf.index = i;

				if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
					errno_exit("VIDIOC_QBUF");
			}

			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
				errno_exit("VIDIOC_STREAMON");

			break;

		case IO_METHOD_USERPTR:
			for (i = 0; i < n_buffers; ++i) {

				struct v4l2_buffer buf;

				CLEAR(buf);

				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_USERPTR;
				buf.index = i;
				buf.m.userptr = (unsigned long) buffers[i].start;
				buf.length = buffers[i].length;

				if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
					errno_exit("VIDIOC_QBUF");
			}

			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
				errno_exit("VIDIOC_STREAMON");

			break;
	}
}

static void uninit_device(void) {

	unsigned int i;

	switch (io) {

		case IO_METHOD_READ:
			free(buffers[0].start);
			break;

		case IO_METHOD_MMAP:
			for (i = 0; i < n_buffers; ++i)
				if (-1 == munmap(buffers[i].start, buffers[i].length))
					errno_exit("munmap");
			break;

		case IO_METHOD_USERPTR:
			for (i = 0; i < n_buffers; ++i)
				free(buffers[i].start);
			break;
	}

	free(buffers);
}

static void init_read(unsigned int buffer_size) {

	buffers = calloc(1, sizeof(*buffers));

	if (!buffers) {

		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	buffers[0].length = buffer_size;
	buffers[0].start = malloc(buffer_size);

	if (!buffers[0].start) {

		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
}

static void init_mmap(void) {

	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {

		if (EINVAL == errno) {

			fprintf(stderr, "%s does not support "
					"memory mapping\n", dev_name);
			exit(EXIT_FAILURE);
		} else {

			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {

		fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
		exit(EXIT_FAILURE);
	}

	buffers = calloc(req.count, sizeof(*buffers));

	if (!buffers) {

		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {

		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL /* start anywhere */, buf.length,
				PROT_READ | PROT_WRITE /* required */,
				MAP_SHARED /* recommended */, fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");
	}
}

static void init_userp(unsigned int buffer_size) {

	struct v4l2_requestbuffers req;
	unsigned int page_size;

	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {

		if (EINVAL == errno) {

			fprintf(stderr, "%s does not support "
					"user pointer i/o\n", dev_name);
			exit(EXIT_FAILURE);
		} else {

			errno_exit("VIDIOC_REQBUFS");
		}
	}

	buffers = calloc(4, sizeof(*buffers));

	if (!buffers) {

		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < 4; ++n_buffers) {

		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = memalign(/* boundary */page_size,
				buffer_size);

		if (!buffers[n_buffers].start) {

			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}

static void init_device(void) {

	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	unsigned int min;

	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {

		if (EINVAL == errno) {

			fprintf(stderr, "%s is no V4L2 device\n", dev_name);
			exit(EXIT_FAILURE);
		} else {

			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {

		fprintf(stderr, "%s is no video capture device\n", dev_name);
		exit(EXIT_FAILURE);
	}

	switch (io) {

		case IO_METHOD_READ:
			if (!(cap.capabilities & V4L2_CAP_READWRITE)) {

				fprintf(stderr, "%s does not support read i/o\n", dev_name);
				exit(EXIT_FAILURE);
			}

			break;

		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
			if (!(cap.capabilities & V4L2_CAP_STREAMING)) {

				fprintf(stderr, "%s does not support streaming i/o\n", dev_name);
				exit(EXIT_FAILURE);
			}

			break;
	}

	/* Select video input, video standard and tune here. */

	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {

		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {

			switch (errno) {

				case EINVAL:
					/* Cropping not supported. */
					break;
				default:
					/* Errors ignored. */
					break;
			}
		}
	} else {

		/* Errors ignored. */
	}

	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 640;
	fmt.fmt.pix.height = 480;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if(0)
	{
		struct v4l2_fmtdesc fmtdesc;
		fmtdesc.type = fmt.type; // 指定需要枚举的类型

		fmtdesc.index = 0;
		if (-1 == xioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc))	// 获取图片支持格式
			errno_exit("VIDIOC_ENUM_FMT");

		struct v4l2_frmsizeenum frmsize;
		frmsize.pixel_format = fmtdesc.pixelformat;

		frmsize.index = 0;
		if (-1 == xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize))	// 默认获取图片最大分辨率
			errno_exit("VIDIOC_ENUM_FRAMESIZES");

		/* 打印图片分辨率 */
		printf("width: %d height: %d\n", frmsize.discrete.width,frmsize.discrete.height);

		fmt.fmt.pix.width = frmsize.discrete.width;
		fmt.fmt.pix.height = frmsize.discrete.height;
	}

	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
		errno_exit("VIDIOC_S_FMT");

	/* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	switch (io) {

		case IO_METHOD_READ:
			init_read(fmt.fmt.pix.sizeimage);
			break;

		case IO_METHOD_MMAP:
			init_mmap();
			break;

		case IO_METHOD_USERPTR:
			init_userp(fmt.fmt.pix.sizeimage);
			break;
	}
}

static void close_device(void) {

	if (-1 == close(fd))
		errno_exit("close");

	fd = -1;
}

static int open_device(void) {

	struct stat st;

	if (-1 == stat(dev_name, &st)) {

		fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno,
				strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!S_ISCHR(st.st_mode)) {

		fprintf(stderr, "%s is no device\n", dev_name);
		exit(EXIT_FAILURE);
	}

	fd = open(dev_name, O_RDWR /* required */| O_NONBLOCK, 0);

	if (-1 == fd) {

		fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno,
				strerror(errno));
		exit(EXIT_FAILURE);
	}
	return fd;
}

void v4l2_init(void) {

	dev_name = "/dev/video0";

	open_device();

	init_device();

	start_capturing();
}

void v4l2_exit(void) {

	stop_capturing();

	uninit_device();

	close_device();
}
