
#include "FFMPEGPlugin.h"
#include "../../chipplayer.h"

#include <coreutils/utils.h>
#include <coreutils/file.h>
#include <coreutils/fifo.h>
#include <webutils/web.h>

#include <coreutils/thread.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include <set>
#include <unordered_map>

using namespace std;
using namespace utils;

namespace chipmachine {

class FFMPEGPlayer : public ChipPlayer {
public:
	FFMPEGPlayer() {
	}

	FFMPEGPlayer(const std::string &fileName) {

		if(!init) {
			av_register_all();
			avformat_network_init();
			init = true;
		}

		decodeThread = std::thread([=]() {
			
			string fn = fileName;
			
			LOGD("Open stream %s", fn);
			pFormatCtx = avformat_alloc_context();
			//pFormatCtx->max_analyze_duration2 = 5 * AV_TIME_BASE;
			if(avformat_open_input(&pFormatCtx, fn.c_str(), NULL, NULL) != 0)
				throw player_exception("avformat_open_input() failed");
	
			//VDictionary *options = nullptr;
			//int ok = avformat_find_stream_info(pFormatCtx, &options);
			//LOGD("OK %d COUNT %d", ok, av_dict_count(options));
	
			int audioStreamIndex = -1;
			AVCodecContext *pAudioCodecCtx;
			AVCodec *pAudioCodec;
			LOGD("%d STREAMS", pFormatCtx->nb_streams);
			for(unsigned int i = 0; i < pFormatCtx->nb_streams; i++) {
				auto &stream = pFormatCtx->streams[i];
				LOGD("STREAM %d is %x", i, stream->codec->codec_type);
				if(stream->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
					audioStreamIndex = i;
					LOGD("DURATION %d TIMEBASE %d/%d", stream->duration, stream->time_base.num, stream->time_base.den);
					pAudioCodecCtx = pFormatCtx->streams[i]->codec;
					// Find decoder
					LOGD("Looking for audio codec %x", pAudioCodecCtx->codec_id);
					pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);
					if(pAudioCodec) {
						// Open decoder
						int res = !(avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL) < 0);
						rate = pAudioCodecCtx->sample_rate;
						LOGD("AUDIO %s %d/%d", pAudioCodec->long_name, pAudioCodecCtx->sample_rate,
							 pAudioCodecCtx->channels);
					}
					// break;
				}
			}
			
			LOGD("Duration %d", pFormatCtx->duration);
			//length = pFormatCtx->duration / 1000000;
			//gotLength = true;
			//setMeta("length", length);
			
			
			//av_resample_init(44100, rate, 16, 10, 0, 1.0);
			SwrContext *swr = swr_alloc_set_opts(nullptr,  AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100, 
			                                     pAudioCodecCtx->channel_layout ,pAudioCodecCtx->sample_fmt, rate, 0, nullptr);
			swr_init(swr);
			AVPacket packet;
			av_init_packet(&packet);
			int16_t buffer[8192];

			AVFrame *audioFrame = av_frame_alloc();

			LOGD("LOOP");
			while(!quit && av_read_frame(pFormatCtx, &packet) >= 0) {
				//LOGD("STREAM %d", packet.stream_index);
				if(packet.stream_index == audioStreamIndex) {
					//LOGD("PACKET SIZE %d", packet.size);
					while(packet.size > 0) {
						int got_frame;
						int len =
						    avcodec_decode_audio4(pAudioCodecCtx, audioFrame, &got_frame, &packet);
						if(got_frame) {
							auto data_size = av_samples_get_buffer_size(
							    NULL, pAudioCodecCtx->channels, audioFrame->nb_samples,
							    pAudioCodecCtx->sample_fmt, 1);
						
							uint8_t* out[] = { (uint8_t*)buffer };
							//const uint8_t* in[] = { audioFrame->data[0], audioFrame->data[1] };
							int rc = swr_convert(swr, out, 8192, (const uint8_t**)audioFrame->data, audioFrame->nb_samples);
							fifo.put(buffer, rc*2);
						}
						packet.size -= len;
						packet.data += len;
					}
				}
			}
			av_frame_free(&audioFrame);
			swr_free(&swr);
			avcodec_close(pAudioCodecCtx);
			avformat_close_input(&pFormatCtx);
			LOGD("Thread ending");
		});
		// av_dump_format(pFormatCtx, 0, fn.c_str(), 01);
		// checkMeta();
	}

	~FFMPEGPlayer() override {
		fifo.clear();
		quit = true;
		decodeThread.join();
		fifo.clear();
	}

	virtual int getSamples(int16_t *target, int noSamples) override {
		return fifo.get(target, noSamples);
	}

	virtual bool seekTo(int song, int seconds) override { return false; }

private:
	std::thread decodeThread;
	AVFormatContext *pFormatCtx = nullptr;
	bool quit = false;
	long rate;
	mutex m;
	//bool gotLength = false;
	//int length;
	static bool init;
	utils::Fifo<int16_t> fifo{32768};
};

bool FFMPEGPlayer::init = false;

bool FFMPEGPlugin::canHandle(const std::string &name) {
	auto ext = utils::path_extension(name);
	return ext == "m4a" || ext == "aac";
}

ChipPlayer *FFMPEGPlugin::fromFile(const std::string &fileName) {
	return new FFMPEGPlayer{fileName};
};

ChipPlayer *FFMPEGPlugin::fromStream() {
	return new FFMPEGPlayer();
}
}