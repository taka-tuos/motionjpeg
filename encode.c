#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/*
#include <libavcodec/avcodec.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#define INBUF_SIZE 4096*/

#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

FILE *arfp;

void *jpeg_buffer;
int jpeg_len;

void ar_createfile(char *name,int siz)
{
	fprintf(arfp,"%-16.16s1342943816  0     0     100644  %-10d`\x0a",name,siz);
}

void stbi_ar_write(void *context, void *data, int size)
{
	jpeg_buffer = realloc(jpeg_buffer,jpeg_len+size);
	memcpy(jpeg_buffer+jpeg_len,data,size);
	jpeg_len += size;
}

unsigned int *pic = NULL;

int aw=0,ah=0;
int pn=0;
int f=0;

static void frame_save(unsigned char *inp, int wrap, int w, int h, const char *q)
{
	//printf("%d,%d\n",w,h);
	if(!pic) {
		aw = w * 8;
		ah = h * 8;
	}
	if((f % 64) == 0 && f != 0) {
		char fn2[1024];
		jpeg_len=0;
		jpeg_buffer=0;
		sprintf(fn2,"movie_%d.jpg",pn);
		stbi_write_jpg_to_func(stbi_ar_write,0,aw,ah,4,pic,atoi(q));
		ar_createfile(fn2,jpeg_len);
		fwrite(jpeg_buffer,jpeg_len,1,arfp);
		if(jpeg_len & 1) fputc(0x0a,arfp);
		pn++;
	}
	pic=realloc(pic,aw*ah*4);
	for(int y=0;y<h;y++) {
		for(int x=0;x<w;x++) {
			unsigned char *np = (inp + y*wrap);
			int r=np[x*3+2];
			int g=np[x*3+1];
			int b=np[x*3+0];
			pic[(y*8+((f%64)/8))*aw+(x*8+(f%8))] = (r << 16) | (g << 8) | b;
		}
	}
	printf("saving frame %3d\n", f);
	f++;
}

void on_frame_decoded(AVFrame *frame, char *quality, int width, int height)
{
	frame_save((unsigned char *)frame->data[0], frame->linesize[0], width, height, quality);
}

int main(int argc, char *argv[])
{
	if (argc <= 3) {
		fprintf(stderr, "Usage: %s <input file> <output file> <quality>\n", argv[0]);
		exit(0);
	}
	const char *filename	= argv[1];
	const char *outfilename = argv[2];
	const char *quality		= argv[3];
	
	double video_framerate;
	
	av_register_all();

	const char *input_path = filename;
	AVFormatContext* format_context = NULL;
	if (avformat_open_input(&format_context, input_path, NULL, NULL) != 0) {
		printf("avformat_open_input failed\n");
	}

	if (avformat_find_stream_info(format_context, NULL) < 0) {
		printf("avformat_find_stream_info failed\n");
	}

	AVStream *video_stream = NULL;
	for (int i = 0; i < (int)format_context->nb_streams; ++i) {
		if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream = format_context->streams[i];
			break;
		}
	}
	if (video_stream == NULL) {
		printf("No video stream ...\n");
	}

	video_framerate = video_stream->r_frame_rate.num / (double)video_stream->r_frame_rate.den;

	AVCodec *codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
	if (codec == NULL) {
		printf("No supported decoder ...\n");
	}

	AVCodecContext *codec_context = avcodec_alloc_context3(codec);
	if (codec_context == NULL) {
		printf("avcodec_alloc_context3 failed\n");
	}

	if (avcodec_parameters_to_context(codec_context, video_stream->codecpar) < 0) {
		printf("avcodec_parameters_to_context failed\n");
	}
	
	if (avcodec_open2(codec_context, codec, NULL) != 0) {
		printf("avcodec_open2 failed\n");
	}
	
	arfp = fopen(outfilename,"wb");
	fprintf(arfp,"!<arch>\x0a");

	AVFrame *frame = av_frame_alloc();
	AVPacket packet;
	struct SwsContext *img_convert_ctx;
	
	AVFrame *rgb = av_frame_alloc();
	
	int w = codec_context->width;
    int h = codec_context->height;
	
	printf("size: %dx%d\n",w,h);
	
	int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codec_context->width, codec_context->height, 1);
	uint8_t *buffer = (uint8_t *)av_malloc(num_bytes*sizeof(uint8_t));

	avpicture_fill((AVPicture *)rgb, buffer, AV_PIX_FMT_RGB24, codec_context->width, codec_context->height);
	
	img_convert_ctx = sws_getContext(w, h, codec_context->pix_fmt, w, h, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
	
	printf("framerate: %dfps\n",(int)(video_framerate+0.5));

	while (av_read_frame(format_context, &packet) == 0) {
		if (packet.stream_index == video_stream->index) {
			if (avcodec_send_packet(codec_context, &packet) != 0) {
				printf("avcodec_send_packet failed\n");
			}
			while (avcodec_receive_frame(codec_context, frame) == 0) {
				sws_scale(img_convert_ctx, (const uint8_t * const *)frame->data, frame->linesize, 0, codec_context->height, rgb->data, rgb->linesize);
				on_frame_decoded(rgb, quality, w, h);
			}
		}
		av_packet_unref(&packet);
	}

	// flush decoder
	if (avcodec_send_packet(codec_context, NULL) != 0) {
		printf("avcodec_send_packet failed");
	}
	while (avcodec_receive_frame(codec_context, frame) == 0) {
		sws_scale(img_convert_ctx, (const uint8_t * const *)frame->data, frame->linesize, 0, codec_context->height, rgb->data, rgb->linesize);
		on_frame_decoded(rgb, quality, w, h);
	}

	av_frame_free(&frame);
	avcodec_free_context(&codec_context);
	avformat_close_input(&format_context);
	
	char info[1024];
	sprintf(info,"%d\n%d\n%d\n%d\n",aw/8,ah/8,(int)(video_framerate+0.5),pn);
	ar_createfile("movie.info", strlen(info));
	fprintf(arfp,info);
	
	fclose(arfp);
	
	return 0;
}
