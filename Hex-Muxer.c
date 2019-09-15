/******************************************************************************
 * Hex-Muxer: Coding and Muxing of hexagonal Data Streams
 ******************************************************************************
 * v1.0 - 07.11.2017
 *
 * Copyright (c) 2017 Tobias Schlosser
 *  (tobias.schlosser@informatik.tu-chemnitz.de)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_HMod/Hex-MuxerHMod.h"

#include <libavformat/avformat.h>


u32   HMod_order;
float HMod_scale  = 1.0f;
float HMod_radius = 1.0f;
u32   HMod_mode_i;
u32   HMod_mode_d;


void encode_mux_frame(
 AVCodecContext* codec_context_hex,
 AVFormatContext* format_context, AVFormatContext* format_context_hex,
 AVFrame* frame_hex, AVPacket* packet_hex, int cached_hex, int* got_packet,
 int stream_index) {
	*got_packet = 0;


	int ret = avcodec_encode_video2(codec_context_hex, packet_hex, frame_hex, got_packet); // deprecated (TODO?)
	if(ret < 0) {
		fputs("\n[Hex-Muxer] Error encoding hex. frame\n", stderr);
		exit(1);
	}


	if(*got_packet) {
		// = { 1, 30 } (e.g.)
		AVRational time_base     = format_context->streams[stream_index]->time_base;
		AVRational time_base_hex = format_context_hex->streams[stream_index]->time_base;


		// pts, dts = 33; duration = 0* (e.g.)
		packet_hex->stream_index = stream_index;
		packet_hex->pts          = av_rescale_q_rnd(packet_hex->pts,  time_base, time_base_hex, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
		packet_hex->dts          = av_rescale_q_rnd(packet_hex->dts,  time_base, time_base_hex, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
		packet_hex->duration     = av_rescale_q(packet_hex->duration, time_base, time_base_hex);
		packet_hex->pos          = -1;

		printf(
			"[Hex-Muxer] packet_hex->pts             = %8li (stream_index = %i, cached = %i)"
			" (encode_mux_frame: avcodec_encode_video2)\n",
			 packet_hex->pts, stream_index, cached_hex);


		ret = av_interleaved_write_frame(format_context_hex, packet_hex); // av_packet_unref (refcounted_frames)
		if(ret < 0) {
			fputs("\n[Hex-Muxer] Error muxing hex. packet\n", stderr);
			exit(1);
		}
	}
}

int code_mux_packet(
 AVCodecContext* codec_context, AVCodecContext* codec_context_hex,
 AVFormatContext* format_context, AVFormatContext* format_context_hex,
 AVFrame* frame, AVFrame* frame_hex, AVPacket* packet, AVPacket* packet_hex,
 int cached, int cached_hex, int* got_frame, int stream_index,
 u8* srcFrame, u8* destFrame, int width, int height) {
	int got_packet = 0;


	*got_frame = 0;


	int ret = avcodec_decode_video2(codec_context, frame, got_frame, packet); // deprecated (TODO?)
	if(ret < 0) {
		fputs("\n[Hex-Muxer] Error decoding frame\n", stderr);
		return ret;
	}


	if(*got_frame) {
		printf(
			"[Hex-Muxer] frame->coded_picture_number = %8i (stream_index = %i, cached = %i):"
			" frame->width x frame->height = %i x %i (code_mux_packet: avcodec_decode_video2)\n",
			 frame->coded_picture_number, stream_index, cached, frame->width, frame->height);


		// frame->data -> srcFrame

		// 4:4:4
		if(frame->linesize[0] == frame->linesize[1]) {
			for(unsigned int h = 0; h < frame->height; h++) {
				for(unsigned int w = 0; w < frame->width; w++) {
					const unsigned int p  = h * frame->linesize[0] + w;
					const unsigned int ps = 3 * (h * frame->width + w);

					srcFrame[ps]     = frame->data[0][p];
					srcFrame[ps + 1] = frame->data[1][p];
					srcFrame[ps + 2] = frame->data[2][p];
				}
			}
		// 4:2:0
		} else {
			for(unsigned int h = 0; h < frame->height; h++) {
				for(unsigned int w = 0; w < frame->width; w++) {
					const unsigned int p  = h * frame->linesize[0] + w;
					const unsigned int ps = 3 * (h * frame->width + w);

					srcFrame[ps] = frame->data[0][p];
				}
			}


			const unsigned int h_base = frame->height / 4;
			const unsigned int w_base = frame->width  / 4;

			for(unsigned int h = 0; h < frame->height / 2; h++) {
				for(unsigned int w = 0; w < frame->width / 2; w++) {
					const unsigned int p  = h * frame->linesize[1] + w;
					const unsigned int ps = 3 * ((h_base + h) * frame->width + (w_base + w));

					srcFrame[ps + 1] = frame->data[1][p];
					srcFrame[ps + 2] = frame->data[2][p];
				}
			}
		}


		HexMuxerHMod(
			srcFrame, destFrame,
			(u32)width, (u32)height, (u32)width, (u32)height,
			HMod_order, HMod_scale, HMod_radius, HMod_mode_i, 0);


		// destFrame -> frame_hex->data

		if(!av_frame_is_writable(frame_hex))
			av_frame_make_writable(frame_hex);

		if(!HMod_mode_d) {
			// 4:4:4
			if(frame->linesize[0] == frame->linesize[1]) {
				const unsigned int h_base = abs(frame->height - frame_hex->height) / 2;
				const unsigned int w_base = abs(frame->width  - frame_hex->width)  / 2;

				for(unsigned int h = 0; h < frame_hex->height; h++) {
					for(unsigned int w = 0; w < frame_hex->width; w++) {
						const unsigned int p  = h * frame_hex->linesize[0] + w;
						const unsigned int pd = 3 * ((h_base + h) * frame->width + (w_base + w));

						frame_hex->data[0][p] = destFrame[pd];
						frame_hex->data[1][p] = destFrame[pd + 1];
						frame_hex->data[2][p] = destFrame[pd + 2];
					}
				}
			// 4:2:0
			} else {
				unsigned int h_base = abs(frame->height - frame_hex->height) / 2;
				unsigned int w_base = abs(frame->width  - frame_hex->width)  / 2;

				for(unsigned int h = 0; h < frame_hex->height; h++) {
					for(unsigned int w = 0; w < frame_hex->width; w++) {
						const unsigned int p  = h * frame_hex->linesize[0] + w;
						const unsigned int pd = 3 * ((h_base + h) * frame->width + (w_base + w));

						frame_hex->data[0][p] = destFrame[pd];
					}
				}


				h_base = (frame->height + abs(frame->height - frame_hex->height)) / 4;
				w_base = (frame->width  + abs(frame->width  - frame_hex->width))  / 4;

				for(unsigned int h = 0; h < frame_hex->height / 2; h++) {
					for(unsigned int w = 0; w < frame_hex->width / 2; w++) {
						const unsigned int p  = h * frame_hex->linesize[1] + w;
						const unsigned int pd = 3 * ((h_base + h) * frame->width + (w_base + w));

						frame_hex->data[1][p] = destFrame[pd + 1];
						frame_hex->data[2][p] = destFrame[pd + 2];
					}
				}
			}
		} else {
			// 4:4:4
			if(frame->linesize[0] == frame->linesize[1]) {
				for(unsigned int h = 0; h < frame_hex->height; h++) {
					for(unsigned int w = 0; w < frame_hex->width; w++) {
						const unsigned int p  = h * frame_hex->linesize[0] + w;
						const unsigned int pd = 3 * (h * frame_hex->width + w);

						frame_hex->data[0][p] = destFrame[pd];
						frame_hex->data[1][p] = destFrame[pd + 1];
						frame_hex->data[2][p] = destFrame[pd + 2];
					}
				}
			// 4:2:0
			} else {
				for(unsigned int h = 0; h < frame_hex->height; h++) {
					for(unsigned int w = 0; w < frame_hex->width; w++) {
						const unsigned int p  = h * frame_hex->linesize[0] + w;
						const unsigned int pd = 3 * (h * frame_hex->width + w);

						frame_hex->data[0][p] = destFrame[pd];
					}
				}


				const unsigned int h_base = frame_hex->height / 4;
				const unsigned int w_base = frame_hex->width  / 4;

				for(unsigned int h = 0; h < frame_hex->height / 2; h++) {
					for(unsigned int w = 0; w < frame_hex->width / 2; w++) {
						const unsigned int p  = h * frame_hex->linesize[1] + w;
						const unsigned int pd = 3 * ((h_base + h) * frame_hex->width + (w_base + w));

						frame_hex->data[1][p] = destFrame[pd + 1];
						frame_hex->data[2][p] = destFrame[pd + 2];
					}
				}
			}
		}

		frame_hex->pts = frame->pts; // = 33 (e.g.)

		printf(
			"[Hex-Muxer] frame_hex->pts              = %8li (stream_index = %i, cached = %i)"
			" (code_mux_packet:  HexMuxerHMod)\n",
			 frame_hex->pts, stream_index, cached);


		encode_mux_frame(
			codec_context_hex,
			format_context, format_context_hex,
			frame_hex, packet_hex, 0, &got_packet, stream_index);


		av_frame_unref(frame); // refcounted_frames
	}


	if(cached_hex) {
		do {
			encode_mux_frame(
				codec_context_hex,
				format_context, format_context_hex,
				NULL, packet_hex, 1, &got_packet, stream_index);
		} while(got_packet);
	}


	return packet->size;
}


int main(int argc, char **argv)
{
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    const char *in_filename, *out_filename;
    int ret, i;
    int stream_index = 0;
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;








	// schlto 07.11.2017




	u32 HMod_width;
	u32 HMod_height;


	// x264-params

	char preset[16] = "medium";

	int qmin    = -1;
	int qmax    = -1;
	int trellis = -1;

	int mv_range = -2;
	int gop_size = -1;
	int bframes  = -1;

	int lookahead = -1;




	setvbuf(stdout, NULL, _IONBF, 0);




	if(argc < 8) {
		printf(
			"[Hex-Muxer] Usage: %s <input> <output>"
			" <width> <height> <order=(1-7)> <mode_i=(0-3)> <mode_d=(0|1)>"
			"[ (...) ]\n",
			 argv[0]);

		return 1;
	}


	HMod_width  = (u32)atoi(argv[3]);
	HMod_height = (u32)atoi(argv[4]);
	HMod_order  = (u32)atoi(argv[5]);
	HMod_mode_i = (u32)atoi(argv[6]);
	HMod_mode_d = (u32)atoi(argv[7]);

	printf(
		"[Hex-Muxer] width x height = %u x %u, order = %u, mode_i = %u, mode_d = %u\n",
		 HMod_width, HMod_height, HMod_order, HMod_mode_i, HMod_mode_d);


	for(unsigned int i = 8; i < argc; i++) {
		if(!strcmp(argv[i], "--preset")) {
			strcpy(preset, argv[++i]);
			printf("[Hex-Muxer]  -> preset    = %s\n", preset);
		} else if(!strcmp( argv[i], "--qmin"      )) {
			qmin      = atoi(argv[++i]);
			printf("[Hex-Muxer]  -> qmin      = %u\n", qmin);
		} else if(!strcmp( argv[i], "--qmax"      )) {
			qmax      = atoi(argv[++i]);
			printf("[Hex-Muxer]  -> qmax      = %u\n", qmax);
		} else if(!strcmp( argv[i], "--trellis"   )) {
			trellis   = atoi(argv[++i]);
			printf("[Hex-Muxer]  -> trellis   = %u\n", trellis);
		} else if(!strcmp( argv[i], "--mv_range"  )) {
			mv_range  = atoi(argv[++i]);
			printf("[Hex-Muxer]  -> mv_range  = %i\n", mv_range);
		} else if(!strcmp( argv[i], "--gop_size"  )) {
			gop_size  = atoi(argv[++i]);
			printf("[Hex-Muxer]  -> gop_size  = %u\n", gop_size);
		} else if(!strcmp( argv[i], "--bframes"   )) {
			bframes   = atoi(argv[++i]);
			printf("[Hex-Muxer]  -> bframes   = %u\n", bframes);
		} else if(!strcmp( argv[i], "--lookahead" )) {
			lookahead = atoi(argv[++i]);
			printf("[Hex-Muxer]  -> lookahead = %u\n", lookahead);
		}
	}


	puts("\n");




	puts("[Hex-Muxer] Initializing HMod...\n");

	HexMuxerHMod_init(
		HMod_width, HMod_height,
		HMod_order, HMod_scale, HMod_radius);








    in_filename  = argv[1];
    out_filename = argv[2];

    av_register_all();

    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", in_filename);
        // goto end;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        // goto end;
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        // goto end;
    }

    stream_mapping_size = ifmt_ctx->nb_streams;
    stream_mapping = av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
    if (!stream_mapping) {
        ret = AVERROR(ENOMEM);
        // goto end;
    }

    ofmt = ofmt_ctx->oformat;

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *out_stream;
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = stream_index++;

        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            // goto end;
        }


		// schlto 07.11.2017
		if(in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {




			// SPS and PPS (TODO)

			u8 SPS_PPS[] = {
				0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x41, 0xa2,
				0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80 };

			out_stream->codecpar->extradata      = (u8*)malloc(sizeof(SPS_PPS));
			out_stream->codecpar->extradata_size = sizeof(SPS_PPS);

			memcpy(out_stream->codecpar->extradata, SPS_PPS, sizeof(SPS_PPS));




			out_stream->avg_frame_rate = in_stream->avg_frame_rate;


			out_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
			out_stream->codecpar->codec_id   = AV_CODEC_ID_H264;
			out_stream->codecpar->format     = in_stream->codecpar->format;

			if(!HMod_mode_d) {
				out_stream->codecpar->width  = HMod_width  < size_out.x ? HMod_width  : size_out.x;
				out_stream->codecpar->height = HMod_height < size_out.y ? HMod_height : size_out.y;

				out_stream->codecpar->width  += out_stream->codecpar->width  % 2;
				out_stream->codecpar->height += out_stream->codecpar->height % 2;
			} else {
				out_stream->codecpar->width  = in_stream->codecpar->width;
				out_stream->codecpar->height = in_stream->codecpar->height;
			}
		} else {
			ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
			if(ret < 0) {
				fputs("\n[Hex-Muxer] Failed to copy codec parameters\n", stderr);
				return 1;
			}
		}


    }
    av_dump_format(ofmt_ctx, 0, out_filename, 1);

    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", out_filename);
            // goto end;
        }
    }

    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        // goto end;
    }








	// schlto 07.11.2017




	unsigned int n = ifmt_ctx->nb_streams;


	AVDictionary* options = NULL;


	AVStream* stream;

	AVCodecParameters* codec_parameters;
	AVCodecParameters* codec_parameters_hex;


	// TODO: # < n

	AVCodecContext** codec_context     = malloc(n * sizeof(AVCodecContext*));
	AVCodecContext** codec_context_hex = malloc(n * sizeof(AVCodecContext*));

	u8** srcFrame  = malloc(n * sizeof(u8*));
	u8** destFrame = malloc(n * sizeof(u8*));

	AVFrame** frame     = malloc(n * sizeof(AVFrame*));
	AVFrame** frame_hex = malloc(n * sizeof(AVFrame*));

	AVPacket  packet;
	AVPacket  packet_cached; // flush/draining packet
	AVPacket* packet_hex;


	bool get_packet       = false;
	bool get_first_packet = true;

	int  got_frame;




	av_dict_set(&options, "refcounted_frames", "1", 0);


	av_init_packet(&packet);
	av_init_packet(&packet_cached);

	packet_hex = av_packet_alloc();
	if(!packet_hex) {
		fputs("\n[Hex-Muxer] Could not allocate hex. packet\n", stderr);
		return 1;
	}

	// av_init_packet
	packet.data        = NULL;
	packet_cached.data = NULL;
	packet.size        = 0;
	packet_cached.size = 0;


	for(i = 0; i < n; i++) {
		stream = ifmt_ctx->streams[i];

		codec_parameters     = stream->codecpar;
		codec_parameters_hex = ofmt_ctx->streams[i]->codecpar;


		if(codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
			AVCodec* codec     = avcodec_find_decoder(codec_parameters->codec_id);
			if(!codec) {
				fputs("\n[Hex-Muxer] Failed to find codec\n",      stderr);
				return 1;
			}

			AVCodec* codec_hex = avcodec_find_encoder(codec_parameters_hex->codec_id);
			if(!codec_hex) {
				fputs("\n[Hex-Muxer] Failed to find hex. codec\n", stderr);
				return 1;
			}

			codec_context[i]     = avcodec_alloc_context3(codec);
			if(!codec_context[i]) {
				fputs("\n[Hex-Muxer] Failed to allocate codec context\n",      stderr);
				return 1;
			}

			codec_context_hex[i] = avcodec_alloc_context3(codec_hex);
			if(!codec_context_hex[i]) {
				fputs("\n[Hex-Muxer] Failed to allocate hex. codec context\n", stderr);
				return 1;
			}

			ret = avcodec_parameters_to_context(codec_context[i],     codec_parameters);
			if(ret < 0) {
				fputs("\n[Hex-Muxer] Failed to copy codec parameters to codec context\n",      stderr);
				return 1;
			}

			ret = avcodec_parameters_to_context(codec_context_hex[i], codec_parameters_hex);
			if(ret < 0) {
				fputs("\n[Hex-Muxer] Failed to copy codec parameters to hex. codec context\n", stderr);
				return 1;
			}

			codec_context_hex[i]->time_base = stream->time_base; // = { 1, 30 } (e.g.)




			// x264-params


			char x264_params[128] = "me=hex";
			int  x264_params_len  = strlen(x264_params);


			if( qmin      >=  0 ) x264_params_len += sprintf( x264_params + x264_params_len, ":qpmin=%i",        qmin      );
			if( qmax      >=  0 ) x264_params_len += sprintf( x264_params + x264_params_len, ":qpmax=%i",        qmax      );
			if( trellis   >=  0 ) x264_params_len += sprintf( x264_params + x264_params_len, ":trellis=%i",      trellis   );

			if( mv_range  >= -1 ) x264_params_len += sprintf( x264_params + x264_params_len, ":mvrange=%i",      mv_range  );
			if( gop_size  >=  0 ) x264_params_len += sprintf( x264_params + x264_params_len, ":keyint=%i",       gop_size  );
			if( bframes   >=  0 ) x264_params_len += sprintf( x264_params + x264_params_len, ":bframes=%i",      bframes   );

			if( lookahead >=  0 ) x264_params_len += sprintf( x264_params + x264_params_len, ":rc-lookahead=%i", lookahead );


			av_dict_set(&options, "preset",      preset,      0);
			av_dict_set(&options, "x264-params", x264_params, 0);




			ret = avcodec_open2(codec_context[i],     codec,     &options);
			if(ret < 0) {
				fputs("\n[Hex-Muxer] Failed to open codec context\n",      stderr);
				return 1;
			}

			ret = avcodec_open2(codec_context_hex[i], codec_hex, &options);
			if(ret < 0) {
				fputs("\n[Hex-Muxer] Failed to open hex. codec context\n", stderr);
				return 1;
			}


			int size = 3 * codec_context[i]->width * codec_context[i]->height;

			srcFrame[i]  = (u8*)calloc(size, sizeof(u8));
			destFrame[i] = (u8*)calloc(size, sizeof(u8));


			frame[i]     = av_frame_alloc();
			if(!frame[i]) {
				fputs("\n[Hex-Muxer] Could not allocate frame\n",      stderr);
				return 1;
			}

			frame_hex[i] = av_frame_alloc();
			if(!frame_hex[i]) {
				fputs("\n[Hex-Muxer] Could not allocate hex. frame\n", stderr);
				return 1;
			}


			frame_hex[i]->format = codec_context[i]->pix_fmt;

			if(!HMod_mode_d) {
				frame_hex[i]->width  = HMod_width  < size_out.x ? HMod_width  : size_out.x;
				frame_hex[i]->height = HMod_height < size_out.y ? HMod_height : size_out.y;

				frame_hex[i]->width  += frame_hex[i]->width  % 2;
				frame_hex[i]->height += frame_hex[i]->height % 2;
			} else {
				frame_hex[i]->width  = codec_context[i]->width;
				frame_hex[i]->height = codec_context[i]->height;
			}


			ret = av_frame_get_buffer(frame_hex[i], 32);
			if(ret < 0) {
				fputs("\n[Hex-Muxer] Could not allocate hex. frame data\n", stderr);
				return 1;
			}
		}
	}




	puts("\n");


	while(ret >= 0) {
		if(get_first_packet) {
			ret = av_read_frame(ifmt_ctx, &packet);
			i   = packet.stream_index;

			get_first_packet = false;
		}


		stream           = ifmt_ctx->streams[i];
		codec_parameters = stream->codecpar;


		if(codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
			while(true) {
				// New context, old packet?

				if(get_packet) {
					ret = av_read_frame(ifmt_ctx, &packet);
				} else {
					get_packet = true;
				}

				if(ret < 0 || packet.stream_index != i) {
					i = packet.stream_index;

					get_packet = false;
					break;
				}


				do {
					ret = code_mux_packet(
					          codec_context[i], codec_context_hex[i],
					          ifmt_ctx, ofmt_ctx,
					          frame[i], frame_hex[i], &packet, packet_hex,
					          0, 0, &got_frame, i,
					          srcFrame[i], destFrame[i],
					           codec_context[i]->width, codec_context[i]->height);

					if(ret < 0)
						break;

					packet.data += ret;
					packet.size -= ret;
				} while(packet.size > 0);

				av_packet_unref(&packet);
			}
		} else {
			while(true) {
				// New context, old packet?

				if(get_packet) {
					ret = av_read_frame(ifmt_ctx, &packet);
				} else {
					get_packet = true;
				}

				if(ret < 0 || packet.stream_index != i) {
					i = packet.stream_index;

					get_packet = false;
					break;
				}


				// = { 1, 30 } (e.g.)
				AVRational time_base     = ifmt_ctx->streams[i]->time_base;
				AVRational time_base_hex = ofmt_ctx->streams[i]->time_base;


				// pts, dts = 33; duration = 0* (e.g.)
				packet.stream_index = i;
				packet.pts          = av_rescale_q_rnd(packet.pts,  time_base, time_base_hex, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
				packet.dts          = av_rescale_q_rnd(packet.dts,  time_base, time_base_hex, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
				packet.duration     = av_rescale_q(packet.duration, time_base, time_base_hex);
				packet.pos          = -1;

				printf(
					"[Hex-Muxer] packet.pts                  = %8li (stream_index = %i, codec_type = %s)\n",
					 packet.pts, packet.stream_index, av_get_media_type_string(codec_parameters->codec_type));


				ret = av_interleaved_write_frame(ofmt_ctx, &packet); // av_packet_unref (refcounted_frames)
				if(ret < 0) {
					fputs("\n[Hex-Muxer] Error muxing packet\n", stderr);
					return 1;
				}
			}
		}
	}


	for(i = 0; i < n; i++) {
		stream           = ifmt_ctx->streams[i];
		codec_parameters = stream->codecpar;


		if(codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
			// Flush decoding (cached = 1, cached_hex = 0)
			do {
				code_mux_packet(
					codec_context[i], codec_context_hex[i],
					ifmt_ctx, ofmt_ctx,
					frame[i], frame_hex[i], &packet_cached, packet_hex,
					1, 0, &got_frame, i,
					srcFrame[i], destFrame[i],
					 codec_context[i]->width, codec_context[i]->height);
			} while(got_frame);

			// Flush encoding (cached = 1, cached_hex = 1)
			code_mux_packet(
				codec_context[i], codec_context_hex[i],
				ifmt_ctx, ofmt_ctx,
				frame[i], frame_hex[i], &packet_cached, packet_hex,
				1, 1, &got_frame, i,
				srcFrame[i], destFrame[i],
				 codec_context[i]->width, codec_context[i]->height);




			avcodec_free_context(&codec_context[i]);
			avcodec_free_context(&codec_context_hex[i]);

			free(srcFrame[i]);
			free(destFrame[i]);

			av_frame_free(&frame[i]);
			av_frame_free(&frame_hex[i]);
		}
	}


	free(codec_context);
	free(codec_context_hex);

	free(srcFrame);
	free(destFrame);

	free(frame);
	free(frame_hex);

	av_packet_free(&packet_hex); // av_packet_alloc








    av_write_trailer(ofmt_ctx);
// end:

    avformat_close_input(&ifmt_ctx);

    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

    av_freep(&stream_mapping);


	// schlto 07.11.2017

	puts("\n\n[Hex-Muxer] Freeing Memory (Precalculations)...");

	HexMuxerHMod_free();


    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}
