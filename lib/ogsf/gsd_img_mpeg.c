#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <grass/ogsf_proto.h>
#include <grass/gstypes.h>

/* FFMPEG stuff */
#ifdef HAVE_FFMPEG
#include <avformat.h>
#include <swscale.h>

/* 5 seconds stream duration */
#define STREAM_DURATION   5.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_NB_FRAMES  ((int)(STREAM_DURATION * STREAM_FRAME_RATE))
#define STREAM_PIX_FMT PIX_FMT_YUV420P /* default pix_fmt */

static int sws_flags = SWS_BICUBIC;

AVFrame *picture, *tmp_picture;
uint8_t *video_outbuf;
int frame_count, video_outbuf_size;

AVOutputFormat *fmt;
AVFormatContext *oc;
AVStream *video_st;


/* add a video output stream */
static AVStream *add_video_stream(AVFormatContext *oc, int codec_id, int w, int h)
{
    AVCodecContext *c;
    AVStream *st;

    st = av_new_stream(oc, 0);
    if (!st) {
        fprintf(stderr, "Could not alloc stream\n");
	return;
    }

    c = st->codec;
    c->codec_id = codec_id;
    c->codec_type = CODEC_TYPE_VIDEO;

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = w;
    c->height = h;
    /* time base: this is the fundamental unit of time (in seconds) in terms
       of which frame timestamps are represented. for fixed-fps content,
       timebase should be 1/framerate and timestamp increments should be
       identically 1. */
    c->time_base.den = STREAM_FRAME_RATE;
    c->time_base.num = 1;
    c->gop_size = 12; /* emit one intra frame every twelve frames at most */
    c->pix_fmt = STREAM_PIX_FMT;
    if (c->codec_id == CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B frames */
        c->max_b_frames = 2;
    }
    if (c->codec_id == CODEC_ID_MPEG1VIDEO){
        /* Needed to avoid using macroblocks in which some coeffs overflow.
           This does not happen with normal video, it just happens here as
           the motion of the chroma plane does not match the luma plane. */
        c->mb_decision=2;
    }
    /* some formats want stream headers to be separate */
    if(!strcmp(oc->oformat->name, "mp4") || !strcmp(oc->oformat->name, "mov") || !strcmp(oc->oformat->name, "3gp"))
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    c->flags |= CODEC_FLAG_QSCALE;
    c->global_quality = st->quality = FF_QP2LAMBDA * 10;

    return st;
}

static AVFrame *alloc_picture(int pix_fmt, int width, int height)
{
    AVFrame *picture;
    uint8_t *picture_buf;
    int size;

    picture = avcodec_alloc_frame();
    if (!picture)
        return NULL;
    size = avpicture_get_size(pix_fmt, width, height);
    picture_buf = av_malloc(size);
    if (!picture_buf) {
        av_free(picture);
        return NULL;
    }
    avpicture_fill((AVPicture *)picture, picture_buf,
                   pix_fmt, width, height);
    return picture;
}

static void open_video(AVFormatContext *oc, AVStream *st)
{
    AVCodec *codec;
    AVCodecContext *c;

    c = st->codec;

    /* find the video encoder */
    codec = avcodec_find_encoder(c->codec_id);
    if (!codec) {
        fprintf(stderr, "video codec not found\n");
        return;
    }

    /* open the codec */
    if (avcodec_open(c, codec) < 0) {
        fprintf(stderr, "could not open codec\n");
        return;
    }

    video_outbuf = NULL;
    if (!(oc->oformat->flags & AVFMT_RAWPICTURE)) {
        /* allocate output buffer */
        /* XXX: API change will be done */
        /* buffers passed into lav* can be allocated any way you prefer,
           as long as they're aligned enough for the architecture, and
           they're freed appropriately (such as using av_free for buffers
           allocated with av_malloc) */
        video_outbuf_size = 200000;
        video_outbuf = av_malloc(video_outbuf_size);
    }

    /* allocate the encoded raw picture */
    picture = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!picture) {
        fprintf(stderr, "Could not allocate picture\n");
        return;
    }

    /* if the output format is not YUV420P, then a temporary YUV420P
       picture is needed too. It is then converted to the required
       output format */
    tmp_picture = NULL;
    if (c->pix_fmt != PIX_FMT_YUV420P) {
        tmp_picture = alloc_picture(PIX_FMT_YUV420P, c->width, c->height);
        if (!tmp_picture) {
            fprintf(stderr, "Could not allocate temporary picture\n");
            return;
        }
    }
}

static void write_video_frame(AVFormatContext *oc, AVStream *st)
{
    int out_size, ret;
    AVCodecContext *c;
    static struct SwsContext *img_convert_ctx;

    c = st->codec;

    if (oc->oformat->flags & AVFMT_RAWPICTURE) {
        /* raw video case. The API will change slightly in the near
           future for that */
        AVPacket pkt;
        av_init_packet(&pkt);

        pkt.flags |= PKT_FLAG_KEY;
        pkt.stream_index= st->index;
        pkt.data= (uint8_t *)picture;
        pkt.size= sizeof(AVPicture);

        ret = av_write_frame(oc, &pkt);
    } else {
        /* encode the image */
        out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, picture);
        /* if zero size, it means the image was buffered */
        if (out_size > 0) {
            AVPacket pkt;
            av_init_packet(&pkt);

            pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
            if(c->coded_frame->key_frame)
                pkt.flags |= PKT_FLAG_KEY;
            pkt.stream_index= st->index;
            pkt.data= video_outbuf;
            pkt.size= out_size;

            /* write the compressed frame in the media file */
            ret = av_write_frame(oc, &pkt);
        } else {
            ret = 0;
        }
    }
    if (ret != 0) {
        fprintf(stderr, "Error while writing video frame\n");
        return;
    }
    frame_count++;
}

static void close_video(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);
    av_free(picture->data[0]);
    av_free(picture);
    if (tmp_picture) {
        av_free(tmp_picture->data[0]);
        av_free(tmp_picture);
    }
    av_free(video_outbuf);
}

#endif

/******************************************
 * initialize FAME setup mpeg defaults and
 * open file for writing
******************************************/
int gsd_init_mpeg(char *filename)
{
#ifdef HAVE_FFMPEG
	GLuint l, r, b, t;
	GLint tmp[4];
	
        glGetIntegerv(GL_VIEWPORT, tmp);
        l = tmp[0];
        r = tmp[0] + tmp[2] - 1;
        b = tmp[1];
        t = tmp[1] + tmp[3] - 1;

	fprintf(stderr, "Opening MPEG stream <%s> ...\n", filename);

	/* initialize libavcodec, and register all codecs and formats */
    	av_register_all();

	/* auto detect the output format from the name. default is mpeg. */
    fmt = guess_format(NULL, filename, NULL);
    if (!fmt) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        fmt = guess_format("mpeg", NULL, NULL);
    }
    if (!fmt) {
        fprintf(stderr, "Could not find suitable output format\n");
        return (-1);
    }

    /* allocate the output media context */
    oc = av_alloc_format_context();
    if (!oc) {
        fprintf(stderr, "Memory error\n");
        return(-1);
    }
    oc->oformat = fmt;
    snprintf(oc->filename, sizeof(oc->filename), "%s", filename);

/* if you want to hardcode the codec (eg #ifdef USE_XVID)
   this may be the place to do it (?????) */
#ifdef USE_XVID
    fmt->video_codec = CODEC_ID_XVID;
#endif

    video_st = NULL;
    if (fmt->video_codec != CODEC_ID_NONE) {
        video_st = add_video_stream(oc, fmt->video_codec, (r - l + 1), (t - b + 1) );
    }

    /* set the output parameters (must be done even if no parameters). */
    if (av_set_parameters(oc, NULL) < 0) {
        fprintf(stderr, "Invalid output format parameters\n");
        return (-1);
    }

    dump_format(oc, 0, filename, 1);

    /* now that all the parameters are set, we can open the audio and
       video codecs and allocate the necessary encode buffers */
    if (video_st)
        open_video(oc, video_st);

        /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        if (url_fopen(&oc->pb, filename, URL_WRONLY) < 0) {
            fprintf(stderr, "Could not open '%s'\n", filename);
            return (-1);
        }
    }

    /* write the stream header, if any */
    av_write_header(oc);


#else
	fprintf(stderr, "NVIZ has not been built with MPEG output support\n");
	return(-1);
#endif

        return (0);

}

/*********************************************
 * get RGB pixbuf and convert to YUV 4:2:0
 * image and write to mpeg stream
*********************************************/
int gsd_write_mpegframe(void)
{
#ifdef HAVE_FFMPEG
        unsigned int xsize, ysize;
        int x, y, xy, xy_uv;
        int yy, uu, vv;
        unsigned char *pixbuf;

        gsd_getimage(&pixbuf, &xsize, &ysize);
        xy = xy_uv = 0;
        for (y = ysize - 1; y >= 0; y--) {
          for (x = 0; x < xsize; x++) {
	    unsigned char r = pixbuf[(y * xsize + x) * 4 + 0];
	    unsigned char g = pixbuf[(y * xsize + x) * 4 + 1];
	    unsigned char b = pixbuf[(y * xsize + x) * 4 + 2];

            yy = (0.257 * r) + (0.504 * g) + (0.098 * b) + 16;;
            vv = (0.439 * r) - (0.368 * g) - (0.071 * b) + 128;
            uu = -(0.148 * r) - (0.291 * g) + (0.439 * b) + 128;
            fflush(stdout);
            picture->data[0][xy] = yy;
            
            if( (x % 2) && (y % 2) )
            {
                    picture->data[1][xy_uv] = uu;
                    picture->data[2][xy_uv] = vv;
                    xy_uv++;
            }

            xy++;
          }

        }
        free(pixbuf);

	write_video_frame(oc, video_st);
	

#endif

        return (0);

}

/****************************************
 * close the mpeg, free buffer, and close file
****************************************/
int gsd_close_mpeg(void)
{
#ifdef HAVE_FFMPEG
int i;

	close_video(oc, video_st);

	/* write the trailer, if any */
    	av_write_trailer(oc);

	/* free the streams */
    	for(i = 0; i < oc->nb_streams; i++) {
        	av_freep(&oc->streams[i]->codec);
        	av_freep(&oc->streams[i]);
    	}

    if (!(fmt->flags & AVFMT_NOFILE)) {
        /* close the output file */
        url_fclose(&oc->pb);
    }

    /* free the stream */
    av_free(oc);


	fprintf(stderr, "Closed MPEG stream.\n");
#endif

        return (0);
}

