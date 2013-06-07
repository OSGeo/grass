/*!
   \file gsd_img_mpeg.c

   \brief OGSF library - FFMPEG stuff

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008, 2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Bill Brown USACERL, GMSL/University of Illinois
   \author Doxygenized (May 2008) and update for FFMPEG >= 0.7
   (November 2012) by Martin Landa <landa.martin gmail.com>
 */

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/ogsf.h>

/* FFMPEG stuff */
#ifdef HAVE_FFMPEG
#include <avformat.h>
#include <avio.h>
#if LIBAVUTIL_VERSION_MAJOR < 51
#include <avutil.h>
#else
/* libavutil 51.22.1's avutil.h doesn't include libavutil/mathematics.h */
#include <mathematics.h>
#endif

/* 5 seconds stream duration */
#define STREAM_DURATION   5.0
#define STREAM_FRAME_RATE 25	/* 25 images/s */
#define STREAM_NB_FRAMES  ((int)(STREAM_DURATION * STREAM_FRAME_RATE))
#define STREAM_PIX_FMT PIX_FMT_YUV420P	/* default pix_fmt */

AVFrame *picture, *tmp_picture;
uint8_t *video_outbuf;
int frame_count, video_outbuf_size;

AVOutputFormat *fmt;
AVFormatContext *oc;
AVStream *video_st;

/*!
   \brief Add a video output stream

   \param oc
   \param codec_id
   \param w
   \param h

   \return 
 */
static AVStream *add_video_stream(AVFormatContext * oc, int codec_id, int w,
				  int h)
{
    AVCodecContext *c;
    AVStream *st;

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 112, 0)
    st = av_new_stream(oc, 0);
#else
    st = avformat_new_stream(oc, NULL);
#endif
    if (!st) {
	G_warning(_("Unable to allocate stream"));
	return NULL;
    }

    c = st->codec;
    c->codec_id = codec_id;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(52, 123, 0)
    c->codec_type = CODEC_TYPE_VIDEO;
#else
    c->codec_type = AVMEDIA_TYPE_VIDEO;
#endif

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
    c->gop_size = 12;		/* emit one intra frame every twelve frames at most */
    c->pix_fmt = STREAM_PIX_FMT;
    if (c->codec_id == CODEC_ID_MPEG2VIDEO) {
	/* just for testing, we also add B frames */
	c->max_b_frames = 2;
    }
    if (c->codec_id == CODEC_ID_MPEG1VIDEO) {
	/* Needed to avoid using macroblocks in which some coeffs overflow.
	   This does not happen with normal video, it just happens here as
	   the motion of the chroma plane does not match the luma plane. */
	c->mb_decision = 2;
    }
    /* some formats want stream headers to be separate */
    if (!strcmp(oc->oformat->name, "mp4") || !strcmp(oc->oformat->name, "mov")
	|| !strcmp(oc->oformat->name, "3gp"))
	c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    c->flags |= CODEC_FLAG_QSCALE;
    
    /* Quality, as it has been removed from AVCodecContext and put in AVVideoFrame. */
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 100, 0)
    c->global_quality = st->quality = FF_QP2LAMBDA * 10;
#else
    c->global_quality = FF_QP2LAMBDA * 10;
#endif

    return st;
}

/*!
   \brief Allocate picture

   \param pix_fmt
   \param width picture width
   \param height picture height

   \return pointer to AVFrame struct
   \return NULL on failure
 */
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

    avpicture_fill((AVPicture *) picture, picture_buf,
		   pix_fmt, width, height);

    return picture;
}

/*!
   \brief Open video

   \param oc
   \param st
 */
static void open_video(AVFormatContext * oc, AVStream * st)
{
    AVCodec *codec;
    AVCodecContext *c;

    c = st->codec;

    /* find the video encoder */
    codec = avcodec_find_encoder(c->codec_id);
    if (!codec) {
	G_warning(_("Video codec not found"));
	return;
    }

    /* open the codec */
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 100, 0)
    if (avcodec_open(c, codec) < 0) { 
#else
    if (avcodec_open2(c, codec, NULL) < 0) {
#endif
	G_warning(_("Unable to open codec"));
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
	G_warning(_("Unable to allocate picture"));
	return;
    }

    /* if the output format is not YUV420P, then a temporary YUV420P
       picture is needed too. It is then converted to the required
       output format */
    tmp_picture = NULL;
    if (c->pix_fmt != PIX_FMT_YUV420P) {
	tmp_picture = alloc_picture(PIX_FMT_YUV420P, c->width, c->height);
	if (!tmp_picture) {
	    G_warning(_("Unable to allocate temporary picture"));
	    return;
	}
    }
}

/*!
   \brief Write video frame

   \param oc
   \param st
 */
static void write_video_frame(AVFormatContext * oc, AVStream * st)
{
    int out_size, ret;
    AVCodecContext *c;

    c = st->codec;

    if (oc->oformat->flags & AVFMT_RAWPICTURE) {
	/* raw video case. The API will change slightly in the near
	   future for that */
	AVPacket pkt;

	av_init_packet(&pkt);
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 32, 0)
	pkt.flags |= PKT_FLAG_KEY;
#else
	pkt.flags |= AV_PKT_FLAG_KEY;
#endif
	pkt.stream_index = st->index;
	pkt.data = (uint8_t *) picture;
	pkt.size = sizeof(AVPicture);

	ret = av_write_frame(oc, &pkt);
    }
    else {
	/* encode the image */
	out_size =
	    avcodec_encode_video(c, video_outbuf, video_outbuf_size, picture);
	/* if zero size, it means the image was buffered */
	if (out_size > 0) {
	    AVPacket pkt;

	    av_init_packet(&pkt);

	    pkt.pts =
		av_rescale_q(c->coded_frame->pts, c->time_base,
			     st->time_base);
	    if (c->coded_frame->key_frame)
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 32, 0)
		pkt.flags |= PKT_FLAG_KEY;
#else
		pkt.flags |= AV_PKT_FLAG_KEY;
#endif
	    pkt.stream_index = st->index;
	    pkt.data = video_outbuf;
	    pkt.size = out_size;

	    /* write the compressed frame in the media file */
	    ret = av_write_frame(oc, &pkt);
	}
	else {
	    ret = 0;
	}
    }
    if (ret != 0) {
	G_warning(_("Error while writing video frame"));
	return;
    }
    frame_count++;
}

/*!
   \brief Close video

   \param oc [unused]
   \param st
 */
static void close_video(AVStream * st)
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

/*!
   \brief Initialize FAME setup mpeg defaults and open file for writing

   \param filename file name

   \return -1 on failure
   \return 0 on success
 */
int gsd_init_mpeg(const char *filename)
{
#ifdef HAVE_FFMPEG
    GLuint l, r, b, t;
    GLint tmp[4];

    glGetIntegerv(GL_VIEWPORT, tmp);
    l = tmp[0];
    r = tmp[0] + tmp[2] - 1;
    b = tmp[1];
    t = tmp[1] + tmp[3] - 1;

    G_verbose_message(_("Opening MPEG stream <%s>..."), filename);

    /* initialize libavcodec, and register all codecs and formats */
    av_register_all();

    /* auto detect the output format from the name. default is mpeg. */
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 32, 0)
    fmt = guess_format(NULL, filename, NULL);
#else
    fmt = av_guess_format(NULL, filename, NULL);
#endif
    if (!fmt) {
	G_warning(_("Unable to deduce output format from file extension: using MPEG"));
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 32, 0)
	fmt = guess_format("mpeg", NULL, NULL);
#else
	fmt = av_guess_format("mpeg", NULL, NULL);
#endif
    }
    if (!fmt) {
	G_warning(_("Unable to find suitable output format"));
	return (-1);
    }

    /* allocate the output media context */
    oc = avformat_alloc_context();
    if (!oc) {
	G_warning(_("Out of memory"));
	return (-1);
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
	video_st =
	    add_video_stream(oc, fmt->video_codec, (r - l + 1), (t - b + 1));
    }

    /* set the output parameters (must be done even if no parameters). */
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 100, 0)
    if (av_set_parameters(oc, NULL) < 0) { 
#else
    if (avformat_write_header(oc, NULL) < 0) {
#endif
	G_warning(_("Invalid output format parameters"));
	return -1;
    }

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 100, 0)
    dump_format(oc, 0, filename, 1); 
#else
    av_dump_format(oc, 0, filename, 1);
#endif

    /* now that all the parameters are set, we can open the audio and
       video codecs and allocate the necessary encode buffers */
    if (video_st)
	open_video(oc, video_st);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 112, 0)
        if (url_fopen(&oc->pb, filename, URL_WRONLY) < 0) { 
#else
	if (avio_open(&oc->pb, filename, AVIO_FLAG_WRITE) < 0) {
#endif
	    G_warning(_("Unable to open <%s>"), filename);
	    return (-1);
	}
    }

    /* write the stream header, if any */
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 100, 0)
    av_write_header(oc);
#else
    avformat_write_header(oc, NULL);
#endif

#else
    G_warning(_("OGSF library has not been built with MPEG output support"));
    return -1;
#endif
    return 0;
}

/*!
   \brief Get RGB pixbuf and convert to YUV 4:2:0

   Image and write to mpeg stream

   \return 0
 */
int gsd_write_mpegframe(void)
{
#ifdef HAVE_FFMPEG
    unsigned int xsize, ysize, x;
    int y, xy, xy_uv;
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

	    if ((x % 2) && (y % 2)) {
		picture->data[1][xy_uv] = uu;
		picture->data[2][xy_uv] = vv;
		xy_uv++;
	    }

	    xy++;
	}

    }
    G_free(pixbuf);

    write_video_frame(oc, video_st);


#endif

    return (0);
}

/*!
   \brief Close the mpeg, free buffer, and close file

   \return 0
 */
int gsd_close_mpeg(void)
{
#ifdef HAVE_FFMPEG
    unsigned int i;

    close_video(video_st);

    /* write the trailer, if any */
    av_write_trailer(oc);

    /* free the streams */
    for (i = 0; i < oc->nb_streams; i++) {
	av_freep(&oc->streams[i]->codec);
	av_freep(&oc->streams[i]);
    }

    if (!(fmt->flags & AVFMT_NOFILE)) {
	/* close the output file */
#if (LIBAVFORMAT_VERSION_INT>>16) < 52
      url_fclose(&oc->pb);
#elif LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 100, 0)
      url_fclose(oc->pb);
#else
      avio_close(oc->pb);
#endif 
    }

    /* free the stream */
    av_free(oc);


    G_debug(3, "Closed MPEG stream");
#endif

    return 0;
}
