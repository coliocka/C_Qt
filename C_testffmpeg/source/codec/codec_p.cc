#include "codec_p.h"

#include <QDateTime>
#include <qfile.h>
#include <utility>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavformat/avio.h"
#include "libavutil/samplefmt.h"
#include "libavutil/imgutils.h"
#include "libavcodec/codec_par.h"
#include "libswresample/swresample.h"
#include "libavutil/time.h"
#include "libavutil/pixfmt.h"
}

// CCalcPtsDur
inline CCalcPtsDur::CCalcPtsDur() {
	m_dTimeBase = 0.0;
	m_dFpsBen = 0.0;
	m_dFpsNum = 0.0;
	m_dFrameDur = 0.0;

	m_llAbsBaseTime = 0;
}

inline CCalcPtsDur::~CCalcPtsDur() = default;

inline void CCalcPtsDur::SetAbsBaseTime(const __int64& llAbsBaseTime) {
	m_llAbsBaseTime = llAbsBaseTime;
}

inline void CCalcPtsDur::SetTimeBase(const int iTimeBase, const int iFpsNum, const int iTimeBen) {
	m_dTimeBase = static_cast<double>(iTimeBase);
	m_dFpsBen = static_cast<double>(iTimeBen);
	m_dFpsNum = static_cast<double>(iFpsNum);

	m_dFrameDur = m_dTimeBase / (m_dFpsNum / m_dFpsBen);
}

inline __int64 CCalcPtsDur::GetVideoPts(const __int64 lFrameIndex) const {
	auto lPts = static_cast<__int64>(m_dFrameDur) * lFrameIndex;
	lPts += m_llAbsBaseTime;

	return lPts;
}

inline __int64 CCalcPtsDur::GetVideoDur(const __int64 lFrameIndex) const {
	const __int64 lPts0 = static_cast<__int64>(m_dFrameDur) * lFrameIndex;
	const __int64 lPts1 = static_cast<__int64>(m_dFrameDur) * (lFrameIndex + 1);
	return lPts1 - lPts0;
}

inline __int64 CCalcPtsDur::GetAudioPts(const __int64 lPaketIndex, const int iAudioSample) const {
	constexpr __int64 dAACSample = 1024.0;

	__int64 llPts = lPaketIndex * static_cast<__int64>(m_dTimeBase) * dAACSample / iAudioSample;
	llPts += m_llAbsBaseTime;
	return llPts;
}

__int64 CCalcPtsDur::GetAudioDur(const __int64 lPaketIndex, const int iAudioSample) const {
	const __int64 lPts0 = GetAudioPts(lPaketIndex, iAudioSample);
	const __int64 lPts1 = GetAudioPts(lPaketIndex + 1, iAudioSample);
	return lPts1 - lPts0;
}

// ���ݸ����Ĳ����ʺ�ͨ������Ϊ ADTS ��ʽ����Ƶ֡���� ADTS ͷ����Ϣ
void GetADTS(uint8_t* pBuf, const __int64 nSampleRate, const __int64 nChannels) {
	// ���ݲ�����ȷ�� ADTS ͷ���е� smap ֵ
	int iSmapIndex = 0;
	if (nSampleRate == 44100)
		iSmapIndex = 4;
	else if (nSampleRate == 48000)
		iSmapIndex = 3;
	else if (nSampleRate == 32000)
		iSmapIndex = 5;
	else if (nSampleRate == 64000)
		iSmapIndex = 2;

	// ���� ADTS ͷ���е� audio_specific_config �ֶ�
	uint16_t audio_specific_config = 0;
	audio_specific_config |= ((2 << 11) & 0xF800);
	audio_specific_config |= ((iSmapIndex << 7) & 0x0780);
	audio_specific_config |= ((nChannels << 3) & 0x78);
	audio_specific_config |= 0 & 0x07;

	// �� audio_specific_config д�� ADTS ͷ����ǰ�����ֽ�
	pBuf[0] = (audio_specific_config >> 8) & 0xFF;
	pBuf[1] = audio_specific_config & 0xFF;
	// ���������ֽ�����Ϊ����ֵ
	pBuf[2] = 0x56;
	pBuf[3] = 0xe5;
	pBuf[4] = 0x00;
}

// CCodecThread
FILE* CCodecThread::m_pLogFile = nullptr;

CCodecThread::CCodecThread(QString strFile, CPushStreamInfo stStreamInfo, const QSize szPlay,
                           const QPair<AVCodecParameters*, AVCodecParameters*> pairRecvCodecPara, const OpenMode mode, const bool bLoop,
                           const bool bPicture, QObject* parent)
: QThread(parent), m_pairRecvCodecPara(pairRecvCodecPara), m_strFile(std::move(strFile)), m_szPlay(szPlay),
  m_stPushStreamInfo(std::move(stStreamInfo)),
  m_eMode(mode), m_bLoop(bLoop), m_bPicture(bPicture) {
	// ������־����Ϊ����ϸ����־��Ϣ
	av_log_set_level(AV_LOG_TRACE);
	// ���ûص�������д����־��Ϣ
	av_log_set_callback(LogCallBack);
	// �߳̽������Զ�ɾ��
	setAutoDelete(true);
}

CCodecThread::CCodecThread(QObject* parent)
: QThread(parent) {
	av_log_set_level(AV_LOG_TRACE);
	av_log_set_callback(LogCallBack);
	setAutoDelete(true);
}

CCodecThread::~CCodecThread() {
	stop();
	clearMemory();
}

void CCodecThread::open(const QString& strFile, const QSize& szPlay, const CPushStreamInfo& stStreamInfo, const OpenMode mode, const bool bLoop,
                        const bool bPicture) {
	stop();
	m_strFile = strFile;
	m_stPushStreamInfo = stStreamInfo;
	m_eMode = mode;
	m_bLoop = bLoop;
	m_szPlay = szPlay;
	m_bPicture = bPicture;
	resume();
	start();
}

void CCodecThread::seek(const quint64& nDuration) {
	if (m_pFormatCtx) {
		// ʹ�� QMutexLocker ȷ���̰߳�ȫ�ز�����Ƶ����Ƶ�����Դ
		QMutexLocker videoLocker(&m_videoMutex);
		QMutexLocker audioLocker(&m_audioMutex);
		// ������Ƶ��λ
		if (m_pVideoThread) {
			// ��λ��Ƶ��
			if (const int nRet = avformat_seek_file(m_pFormatCtx, m_nVideoIndex, m_pFormatCtx->streams[m_nVideoIndex]->start_time,
			                                        static_cast<int64_t>(nDuration / static_cast<int64_t>(av_q2d(
				                                        m_pFormatCtx->streams[m_nVideoIndex]->time_base))),
			                                        m_pFormatCtx->streams[m_nVideoIndex]->duration,
			                                        AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME); nRet >= 0) {
				// ������Ƶ�̵߳�ǰ�� PTS
				m_pVideoThread->setCurrentPts(
					static_cast<int64_t>(nDuration / static_cast<int64_t>(av_q2d(m_pFormatCtx->streams[m_nVideoIndex]->time_base))));
				// �����Ƶ�����е�����
				while (!m_videoPacketQueue.isEmpty()) {
					AVPacket* pkt = m_videoPacketQueue.pop();
					av_packet_unref(pkt);
					av_packet_free(&pkt);
				}
				m_videoPacketQueue.clear();
				while (!m_videoPushPacketQueue.isEmpty()) {
					AVPacket* pkt = m_videoPushPacketQueue.pop();
					av_packet_unref(pkt);
					av_packet_free(&pkt);
				}
				m_videoPushPacketQueue.clear();
			} else {
				av_log(nullptr, AV_LOG_WARNING, "seek video fail, avformat_seek_file return %d\n", nRet);
			}
		}
		// ������Ƶ��λ
		if (m_pAudioThread) {
			if (m_nAudioIndex >= 0) {
				// ��λ��Ƶ��
				if (const int nRet = avformat_seek_file(m_pFormatCtx, m_nAudioIndex, m_pFormatCtx->streams[m_nAudioIndex]->start_time,
				                                        static_cast<int64_t>(nDuration / static_cast<int64_t>(av_q2d(
					                                        m_pFormatCtx->streams[m_nAudioIndex]->time_base))),
				                                        m_pFormatCtx->streams[m_nAudioIndex]->duration,
				                                        AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME); nRet >= 0) {
					m_pAudioThread->setCurrentPts(
						static_cast<int64_t>(nDuration / static_cast<int64_t>(av_q2d(m_pFormatCtx->streams[m_nAudioIndex]->time_base))));
					while (!m_audioPacketQueue.isEmpty()) {
						AVPacket* pkt = m_audioPacketQueue.pop();
						av_packet_unref(pkt);
						av_packet_free(&pkt);
					}
					m_audioPacketQueue.clear();
					while (!m_audioPushPacketQueue.isEmpty()) {
						AVPacket* pkt = m_audioPushPacketQueue.pop();
						av_packet_unref(pkt);
						av_packet_free(&pkt);
					}
					m_audioPushPacketQueue.clear();
				} else {
					av_log(nullptr, AV_LOG_WARNING, "seek audio fail, avformat_seek_file return %d\n", nRet);
				}
			} else if (m_pEncodeMuteThread) {
				while (!m_audioPacketQueue.isEmpty()) {
					AVPacket* pkt = m_audioPacketQueue.pop();
					av_packet_unref(pkt);
					av_packet_free(&pkt);
				}
				m_audioPacketQueue.clear();
				while (!m_audioPushPacketQueue.isEmpty()) {
					AVPacket* pkt = m_audioPushPacketQueue.pop();
					av_packet_unref(pkt);
					av_packet_free(&pkt);
				}
				m_audioPushPacketQueue.clear();
			}
		}
	}
}

void CCodecThread::pause() {
	m_bPause = true;
	if (m_pAudioThread) {
		m_pAudioThread->pause();
	}
	if (m_pVideoThread) {
		m_pVideoThread->pause();
	}
	if (m_pEncodeMuteThread) {
		m_pEncodeMuteThread->pause();
	}
}

void CCodecThread::resume() {
	// �̼߳���ִ��
	m_bRunning = true;
	// �̲߳�����ͣ
	m_bPause = false;
	// ������Ƶ�̺߳���Ƶ�̣߳�ʹ�����ͣ״̬�лָ�
	m_audioWaitCondition.wakeOne();
	m_videoWaitCondition.wakeOne();
	// ����̴߳��ڣ��ָ��̵߳�ִ��
	if (m_pAudioThread) {
		m_pAudioThread->resume();
	}
	if (m_pVideoThread) {
		m_pVideoThread->resume();
	}
	if (m_pEncodeMuteThread) {
		m_pEncodeMuteThread->resume();
	}
}

void CCodecThread::stop() {
	m_bLoop = false;
	m_bRunning = false;
	m_videoWaitCondition.wakeAll();
	m_audioWaitCondition.wakeAll();
	m_AVSyncWaitCondition.wakeAll();
	this->wait();
}

void CCodecThread::run() {
	int nRet = -1;
	char errBuf[ERRBUFSIZE]{};
	//������Ƶ������Ƶ��
	nRet = avformat_open_input(&m_pFormatCtx, m_strFile.toStdString().c_str(), nullptr, nullptr);
	if (nRet < 0) {
		av_strerror(nRet, errBuf, ERRBUFSIZE);
		av_log(nullptr, AV_LOG_ERROR, "Can't open input, %s\n", errBuf);
		return;
	}
	nRet = avformat_find_stream_info(m_pFormatCtx, nullptr);
	if (nRet < 0) {
		av_strerror(nRet, errBuf, ERRBUFSIZE);
		av_log(nullptr, AV_LOG_ERROR, "Can't find stream info, %s\n", errBuf);
		avformat_close_input(&m_pFormatCtx);
		return;
	}
	av_dump_format(m_pFormatCtx, 0, m_strFile.toStdString().c_str(), 0);
	m_nVideoIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	m_nAudioIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
	if (-1 == m_nVideoIndex && -1 == m_nAudioIndex) {
		av_log(nullptr, AV_LOG_ERROR, "Can't find video and audio stream, %s\n");
		avformat_close_input(&m_pFormatCtx);
		return;
	}
	//push
	int nVideoEncodeIndex = -1;
	int nAudioEncodeIndex = -1;
	QPair<AVFormatContext*, std::tuple<CCalcPtsDur, AVCodecContext*, AVCodecContext*>> pairPushFormat;
	if (CCodecThread::OpenMode::OpenMode_Push & m_eMode) {
		// ����������ַ�� scheme
		QUrl url(m_stPushStreamInfo.strAddress);
		AVFormatContext* pOutputFormatCtx = nullptr;
		CCalcPtsDur calPts;
		QString strFileFormat = url.scheme();
		// ���� scheme ���������ʽ
		int nTimeBase = 90000;
		if (0 == url.scheme().compare("udp", Qt::CaseInsensitive)) {
			strFileFormat = "mpegts";
		} else if (0 == url.scheme().compare("srt", Qt::CaseInsensitive)) {
			strFileFormat = "mpegts";
			if (url.query().contains("transtype=file", Qt::CaseInsensitive)) {
				nTimeBase = 1000;
			}
		} else if (0 == url.scheme().compare("rtmp", Qt::CaseInsensitive)) {
			strFileFormat = "flv";
			nTimeBase = 1000;
		}
		// ����ʱ�����������ʱ���׼��֡��
		calPts.SetTimeBase(nTimeBase, m_stPushStreamInfo.nFrameRateDen > 0 ? m_stPushStreamInfo.nFrameRateDen : 25,
		                   m_stPushStreamInfo.nFrameRateNum > 0 ? m_stPushStreamInfo.nFrameRateNum : 1);

		// ������������Ĳ�������ز���
		nRet = avformat_alloc_output_context2(&pOutputFormatCtx, nullptr, strFileFormat.toStdString().c_str(),
		                                      m_stPushStreamInfo.strAddress.toStdString().c_str());
		if (nRet < 0) {
			av_strerror(nRet, errBuf, ERRBUFSIZE);
			av_log(nullptr, AV_LOG_ERROR, "Can't alloc output ctx, %s\n", errBuf);
			avformat_close_input(&m_pFormatCtx);
			return;
		}

		//video
		AVCodecContext* pOutputVideoCodecCtx = nullptr;
		if (CPushStreamInfo::Video & m_stPushStreamInfo.eStream) {
			// ����H.264��Ƶ������
			const AVCodec* out_VideoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
			//const AVCodec* out_VideoCodec = avcodec_find_encoder_by_name("h264_nvenc");

			// ����ͳ�ʼ�������Ƶ������������
			pOutputVideoCodecCtx = avcodec_alloc_context3(nullptr);
			if (m_pairRecvCodecPara.first && m_pairRecvCodecPara.first->width > 0 && m_pairRecvCodecPara.first->height > 0) {
				// �������������������俽���������Ƶ����������������
				avcodec_parameters_to_context(pOutputVideoCodecCtx, m_pairRecvCodecPara.first);
				avcodec_parameters_free(&m_pairRecvCodecPara.first);
				//av_opt_set_dict(pOutputVideoCodecCtx, &m_pRectFormatCtx->streams[nVideoIndex]->metadata);
			} else {
				pOutputVideoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
				//������Ƶ���
				pOutputVideoCodecCtx->width = m_stPushStreamInfo.nWidth > 0 ? m_stPushStreamInfo.nWidth :
					                              m_pFormatCtx->streams[m_nVideoIndex]->codecpar->width;
				//������Ƶ�߶�
				pOutputVideoCodecCtx->height = m_stPushStreamInfo.nHeight > 0 ? m_stPushStreamInfo.nHeight :
					                               m_pFormatCtx->streams[m_nVideoIndex]->codecpar->height;
				//ƽ������
				pOutputVideoCodecCtx->bit_rate = m_stPushStreamInfo.fVideoBitRate > 0 ? static_cast<int64_t>(m_stPushStreamInfo.fVideoBitRate)
					                                 : 2000000; //��С���ʻ��ʲ�����
				//ָ��ͼ����ÿ�����ص���ɫ���ݵĸ�ʽ
				//I֡���  50֡һ��I֡
				pOutputVideoCodecCtx->gop_size = 50;

				pOutputVideoCodecCtx->thread_count = 4;
				//������b֮֡��b֡�����
				pOutputVideoCodecCtx->max_b_frames = 0;
			}

			if (pOutputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
				pOutputVideoCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			}
			//��С�������� ����ֵ 10~30
			pOutputVideoCodecCtx->qmin = 10; //���Խ������QMIN���߻�qmax���ͣ�������Ƶ�����ʹ�С��������Ӱ��
			//�����������
			pOutputVideoCodecCtx->qmax = 51; //���Խ������������qmaxֵ�������������������߻��ʣ��������������й�
			//�˶����Ƶ����������Χ�����˶������йأ�ֵԽ�󣬲����ο���ΧԽ�㣬Խ��ȷ������Ч���½���
			pOutputVideoCodecCtx->me_range = 16;
			//֡�������������
			pOutputVideoCodecCtx->max_qdiff = 4;
			//ѹ���仯�����׳̶ȡ�ֵԽ��Խ��ѹ���任��ѹ����Խ�ߣ�������ʧ�ϴ�
			pOutputVideoCodecCtx->qcompress = 0.4;
			//������Ƶ֡��  25
			pOutputVideoCodecCtx->time_base.num = m_stPushStreamInfo.nFrameRateNum > 0 ?
				                                      m_stPushStreamInfo.nFrameRateNum : m_pFormatCtx->streams[m_nVideoIndex]->r_frame_rate.num;
			pOutputVideoCodecCtx->time_base.den = m_stPushStreamInfo.nFrameRateDen > 0 ?
				                                      m_stPushStreamInfo.nFrameRateDen : m_pFormatCtx->streams[m_nVideoIndex]->r_frame_rate.den;
			/*
			preset��ultrafast��superfast��veryfast��faster��fast��medium��slow��slower��veryslow��placebo��10������ÿ�������preset��Ӧһ������������ͬ�����preset��Ӧ�ı����������һ�¡�preset����Խ�ߣ������ٶ�Խ��������������ҲԽ�ߣ�����Խ�ͣ��ٶ�ҲԽ�죬������ͼ������Ҳ��Խ������ң������ٶ�Խ��Խ������������Խ��Խ��
			*/
			AVDictionary* param = nullptr;
			av_dict_set(&param, "preset", "superfast", 0);
			av_dict_set(&param, "tune", "zerolatency", 0);
			//av_dict_set(&param, "profile", "baseline", 0);
			if (int inRet = avcodec_open2(pOutputVideoCodecCtx, out_VideoCodec, &param); inRet < 0) {
				av_strerror(inRet, errBuf, ERRBUFSIZE);
				av_log(nullptr, AV_LOG_ERROR, "Can't open video encoder\n");
				avcodec_free_context(&pOutputVideoCodecCtx);
				avformat_free_context(pOutputFormatCtx);
				avformat_close_input(&m_pFormatCtx);
				return;
			}
			//unsigned char sps_pps[23] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x0f, 0x00, 0x44, 0xbe, 0x8,
			//    0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80 };
			//pOutputVideoCodecCtx->extradata_size = 23;
			//pOutputVideoCodecCtx->extradata = (uint8_t*)av_malloc(23 + AV_INPUT_BUFFER_PADDING_SIZE);
			//if (pOutputVideoCodecCtx->extradata == NULL) {
			//    printf("could not av_malloc the video params extradata!\n");
			//    return;
			//}
			//memcpy(pOutputVideoCodecCtx->extradata, sps_pps, 23);

			// �����������Ƶ
			AVStream* out_VideoStream = avformat_new_stream(pOutputFormatCtx, nullptr);
			if (!out_VideoStream) {
				av_log(nullptr, AV_LOG_ERROR, "Can't create video streeam\n");
				avcodec_free_context(&pOutputVideoCodecCtx);
				avformat_free_context(pOutputFormatCtx);
				avformat_close_input(&m_pFormatCtx);
				return;
			}
			// �洢��Ƶ��������
			nVideoEncodeIndex = out_VideoStream->index;
			// ����Ƶ�����������ĵĲ��������������Ƶ���ı��������
			avcodec_parameters_from_context(out_VideoStream->codecpar, pOutputVideoCodecCtx);
			out_VideoStream->codecpar->codec_tag = 0;

			//out_VideoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
			//out_VideoStream->codecpar->codec_id = AV_CODEC_ID_H264;
			//out_VideoStream->codecpar->bit_rate = m_stPushStreamInfo.nAudioBitRate > 0 ? m_stPushStreamInfo.nAudioBitRate * 1024 : 1672320;
			//out_VideoStream->codecpar->bits_per_coded_sample = 24;
			//out_VideoStream->codecpar->bits_per_raw_sample = m_stPushStreamInfo.nColorDepth > 0 ? m_stPushStreamInfo.nColorDepth : 8;
			//out_VideoStream->codecpar->profile = 100;
			//out_VideoStream->codecpar->level = 41;
			//out_VideoStream->codecpar->width = m_stPushStreamInfo.nWidth > 0 ? m_stPushStreamInfo.nWidth : m_pFormatCtx->streams[m_nVideoIndex]->codecpar->width;
			//out_VideoStream->codecpar->height = m_stPushStreamInfo.nHeight > 0 ? m_stPushStreamInfo.nHeight : m_pFormatCtx->streams[m_nVideoIndex]->codecpar->height;
			//out_VideoStream->codecpar->field_order = AV_FIELD_PROGRESSIVE;
			//out_VideoStream->codecpar->chroma_location = AVCHROMA_LOC_LEFT;
			//out_VideoStream->codecpar->video_delay = 2;

			////avcodec_parameters_copy(out_VideoStream->codecpar, m_pFormatCtx->streams[m_nVideoIndex]->codecpar);
		}

		//audio
		AVCodecContext* pOutputAudioCodecCtx = nullptr;
		if (CPushStreamInfo::Audio & m_stPushStreamInfo.eStream) {
			// ���� AAC ��Ƶ������
			const AVCodec* out_AudioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
			pOutputAudioCodecCtx = avcodec_alloc_context3(out_AudioCodec);
			if (m_pairRecvCodecPara.second && m_pairRecvCodecPara.second->sample_rate > 0 && m_pairRecvCodecPara.second->channels > 0) {
				avcodec_parameters_to_context(pOutputAudioCodecCtx, m_pairRecvCodecPara.second);
				avcodec_parameters_free(&m_pairRecvCodecPara.second);
				//av_opt_set_dict(pOutputVideoCodecCtx, &m_pRectFormatCtx->streams[nVideoIndex]->metadata);
			} else {
				//avcodec_parameters_to_context(pOutputAudioCodecCtx, m_pFormatCtx->streams[m_nAudioIndex]->codecpar);
				//av_opt_set_dict(pOutputAudioCodecCtx, &m_pFormatCtx->streams[m_nAudioIndex]->metadata);
				pOutputAudioCodecCtx->profile = (strFileFormat == "mpegts") ? FF_PROFILE_MPEG2_AAC_HE : FF_PROFILE_AAC_MAIN; // ����Э��
				pOutputAudioCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO; // ��Ƶ����
				pOutputAudioCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP; // ָ����Ƶ������ʽΪ������
				pOutputAudioCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO; // ������Ƶͨ������Ϊ������
				pOutputAudioCodecCtx->channels = 2; // ˫ͨ��
				pOutputAudioCodecCtx->sample_rate = m_stPushStreamInfo.nAudioSampleRate > 0 ? m_stPushStreamInfo.nAudioSampleRate : 44100; // ���ò���Ƶ��
				pOutputAudioCodecCtx->bit_rate = m_stPushStreamInfo.nAudioBitRate > 0 ? m_stPushStreamInfo.nAudioBitRate * 1024 : 327680; // ���ò���������
				//             if (pOutputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
				//             {
				//                 pOutputAudioCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
				//             }

				pOutputAudioCodecCtx->extradata_size = 5;                               // ������Ƶ�������Ķ������ݴ�СΪ5���ֽ�
				pOutputAudioCodecCtx->extradata = static_cast<uint8_t*>(av_mallocz(5)); // ����5���ֽڶ���ռ�
				// ���� AAC �� ADTS ͷ��Ϣ
				GetADTS(pOutputAudioCodecCtx->extradata, pOutputAudioCodecCtx->sample_rate, pOutputAudioCodecCtx->channels);
			}
			// �����ֵ䣬�洢��Ƶ�����������ò���
			AVDictionary* param = nullptr;
			// ������Ƶ��������Ԥ��ֵ��Ԥ��ֵӰ������ٶȺ�����֮ǰ��Ȩ��
			av_dict_set(&param, "preset", "superfast", 0);
			// ������Ƶ�������ĵ����������Ż��������Լ�С�ӳ�
			av_dict_set(&param, "tune", "zerolatency", 0);
			// ����Ƶ������
			nRet = avcodec_open2(pOutputAudioCodecCtx, out_AudioCodec, &param);
			if (nRet < 0) {
				av_strerror(nRet, errBuf, ERRBUFSIZE);
				av_log(nullptr, AV_LOG_ERROR, "Can't open audio encoder\n");
				avcodec_free_context(&pOutputAudioCodecCtx);
				avformat_free_context(pOutputFormatCtx);
				avformat_close_input(&m_pFormatCtx);
				return;
			}
			// ������Ƶ������������ӵ������ʽ��������
			AVStream* out_AudioStream = avformat_new_stream(pOutputFormatCtx, nullptr);
			if (!out_AudioStream) {
				av_log(nullptr, AV_LOG_ERROR, "Can't create audio stream\n");
				avcodec_free_context(&pOutputAudioCodecCtx);
				avformat_free_context(pOutputFormatCtx);
				avformat_close_input(&m_pFormatCtx);
				return;
			}
			// �洢��Ƶ��������
			nAudioEncodeIndex = out_AudioStream->index;
			// ����Ƶ�������Ĳ������Ƶ���Ƶ���Ĳ�����
			nRet = avcodec_parameters_from_context(out_AudioStream->codecpar, pOutputAudioCodecCtx);
			// ��ʹ�ñ�ǩ
			out_AudioStream->codecpar->codec_tag = 0;

			//out_AudioStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
			//out_AudioStream->codecpar->codec_id = AV_CODEC_ID_AAC;
			//out_AudioStream->codecpar->bit_rate = m_stPushStreamInfo.nAudioBitRate > 0 ? m_stPushStreamInfo.nAudioBitRate * 1024 : 327680;
			//out_AudioStream->codecpar->bits_per_coded_sample = 16;
			////out_AudioStream->codecpar->bits_per_raw_sample = 24;
			//out_AudioStream->codecpar->profile = 1;
			//out_AudioStream->codecpar->channel_layout = AV_CH_LAYOUT_STEREO;
			//out_AudioStream->codecpar->channels = 2;
			//out_AudioStream->codecpar->sample_rate = m_stPushStreamInfo.nAudioSampleRate > 0 ? m_stPushStreamInfo.nAudioSampleRate : 44100;
			//avcodec_parameters_copy(out_AudioStream->codecpar, m_pFormatCtx->streams[m_nAudioIndex]->codecpar);
		}

		// �����ļ���ʽ�ж���������
		int nPushStreamType = 0;
		AVDictionary* avdic = nullptr;
		if (strFileFormat == "flv")
			nPushStreamType = 0;
		else if (strFileFormat == "mpegts")
			nPushStreamType = 1;
		else if (strFileFormat == "mp4") {
			nPushStreamType = 2;
		}
		if (nPushStreamType == 0) {
			//		av_dict_set(&avdic, "rtmp_live", "1", 0);
			//		av_dict_set(&avdic, "live", "1", 0);
		} else if (nPushStreamType == 1) {
			av_dict_set(&avdic, "pkt_size", "1316", 0);
			//av_dict_set(&avdic, "bitrate", pBitRate, 0);
			//			av_dict_set(&avdic, "burst_bits", pBurstBits, 0);
			//			av_dict_set(&avdic, "fifo_size", pFifoSize, 0);
		} else if (nPushStreamType == 2) {
			av_dict_set(&avdic, "mss", "1316", 0);
			//			av_dict_set(&avdic, "inputbw", ptmp, 0);
			av_dict_set(&avdic, "linger", "1", 0);
		}

		// ��������� I/O �����ģ�����֮ǰ���úõ���Ƶ���������ò���
		nRet = avio_open2(&pOutputFormatCtx->pb, m_stPushStreamInfo.strAddress.toStdString().c_str(), AVIO_FLAG_WRITE, nullptr, &avdic);
		if (nRet < 0) {
			av_strerror(nRet, errBuf, ERRBUFSIZE);
			av_log(nullptr, AV_LOG_ERROR, "Can't open io, %s\n", errBuf);
			if (pOutputVideoCodecCtx) {
				avcodec_free_context(&pOutputVideoCodecCtx);
			}
			if (pOutputAudioCodecCtx) {
				avcodec_free_context(&pOutputAudioCodecCtx);
			}
			avformat_free_context(pOutputFormatCtx);
			avformat_close_input(&m_pFormatCtx);
			return;
		}
		// д���������ͷ����Ϣ����ʼ���������д�������Ҫ��Ϣ��ͷ��
		nRet = avformat_write_header(pOutputFormatCtx, nullptr);
		if (nRet < 0) {
			avio_closep(&pOutputFormatCtx->pb);
			avformat_free_context(pOutputFormatCtx);
			avformat_close_input(&m_pFormatCtx);
			av_strerror(nRet, errBuf, ERRBUFSIZE);
			av_log(nullptr, AV_LOG_ERROR, "Can't write header, %s\n", errBuf);
			return;
		}
		// ����������ĸ�ʽ�����ģ�ʱ���׼���Լ���Ƶ����Ƶ�����������ı���
		pairPushFormat = qMakePair(pOutputFormatCtx, std::make_tuple(calPts, pOutputVideoCodecCtx, pOutputAudioCodecCtx));
		// ���������߳�
		m_pPushThread = new CPushThread(pOutputFormatCtx, m_videoPushPacketQueue, m_audioPushPacketQueue,
		                                m_pushVideoWaitCondition, m_pushVideoMutex, m_pushAudioWaitCondition, m_pushAudioMutex,
		                                CPushStreamInfo::Video & m_stPushStreamInfo.eStream, CPushStreamInfo::Audio & m_stPushStreamInfo.eStream);
		m_pPushThread->start();
	}
	// ���������Ƶ��
	if (m_nVideoIndex >= 0) {
		// ���ý���ģʽ
		m_eDecodeMode = m_bPicture ? eDecodeMode::eDecodeMode_CPU : eDecodeMode::eDecodeMode_GPU;
		// �������� AVFormatContext �������ϢԪ���pair����
		QPair<AVFormatContext*, std::tuple<CCalcPtsDur, AVCodecContext*>> pairVideoFormat;
		pairVideoFormat = qMakePair(pairPushFormat.first, std::make_tuple(std::get<0>(pairPushFormat.second), std::get<1>(pairPushFormat.second)));
		// ������Ƶ�߳�
		m_pVideoThread = new CVideoThread(m_videoPacketQueue, m_videoPushPacketQueue, m_videoWaitCondition,
		                                  m_videoMutex, m_pushVideoWaitCondition, m_pushVideoMutex, m_pFormatCtx, m_nVideoIndex, nVideoEncodeIndex,
		                                  pairVideoFormat,
		                                  m_eMode, m_nAudioIndex < 0, CPushStreamInfo::Video & m_stPushStreamInfo.eStream, m_szPlay, m_eDecodeMode);
		// �����ź�
		connect(m_pVideoThread, &CVideoThread::notifyImage, this, &CCodecThread::notifyImage);
		connect(m_pVideoThread, &CVideoThread::notifyCountDown, this, &CCodecThread::notifyCountDown);
		// �����߳�
		m_pVideoThread->start();
	}
	// ���������Ƶ������������ģʽ������Ƶ���ǿ�����
	if (m_nAudioIndex >= 0 || (CCodecThread::OpenMode::OpenMode_Push & m_eMode && CPushStreamInfo::Audio & m_stPushStreamInfo.eStream)) {
		// �����洢��������� AVCodecParameters ����
		AVCodecParameters decodePara;
		// ���û����Ƶ������������ģʽ������Ƶ���ǿ�����
		if (m_nAudioIndex < 0 && CCodecThread::OpenMode::OpenMode_Push & m_eMode && CPushStreamInfo::Audio & m_stPushStreamInfo.eStream) {
			// ����һ��������Ƶ���������Ԫ��
			std::tuple<CCalcPtsDur, AVCodecContext*> pairEncodeCtx = std::make_tuple(std::get<0>(pairPushFormat.second),
			                                                                         std::get<2>(pairPushFormat.second));
			// ����������Ƶ�����߳�
			m_pEncodeMuteThread = new CEncodeMuteAudioThread(m_audioPacketQueue, m_audioWaitCondition, m_audioMutex, m_AVSyncWaitCondition,
			                                                 m_AVSyncMutex, nAudioEncodeIndex, pairEncodeCtx);
			m_pEncodeMuteThread->start();
			// �����͸�ʽ����Ƶ�������л�ȡ���������������AAC��profile
			avcodec_parameters_from_context(&decodePara, std::get<2>(pairPushFormat.second));
			decodePara.profile = FF_PROFILE_AAC_LOW;
			memset(decodePara.extradata, 0, decodePara.extradata_size);
			decodePara.extradata_size = 0;
		}
		QPair<AVFormatContext*, std::tuple<CCalcPtsDur, AVCodecContext*>> pairAudioFormat;
		pairAudioFormat = qMakePair(pairPushFormat.first, std::make_tuple(std::get<0>(pairPushFormat.second), std::get<2>(pairPushFormat.second)));
		// ������Ƶ�߳�
		m_pAudioThread = new CAudioThread(m_audioPacketQueue, m_audioPushPacketQueue, m_audioWaitCondition,
		                                  m_audioMutex, m_pushAudioWaitCondition, m_pushAudioMutex, m_pFormatCtx, m_nAudioIndex, nAudioEncodeIndex,
		                                  pairAudioFormat,
		                                  m_eMode, CPushStreamInfo::Audio & m_stPushStreamInfo.eStream, decodePara);
		connect(m_pAudioThread, &CAudioThread::notifyAudio, this, &CCodecThread::notifyAudio);
		connect(m_pAudioThread, &CAudioThread::notifyCountDown, this, &CCodecThread::notifyCountDown);
		connect(m_pAudioThread, &CAudioThread::notifyAudioPara, this, &CCodecThread::notifyAudioPara);
		// ������Ƶ�߳�
		m_pAudioThread->start();
	}
	// ���ý���ļ���ʱ�����ڵ���0��ͨ���źŴ��ݵ�Ƭ��ʱ������λ���룬���򴫵�0
	if (m_pFormatCtx->duration >= 0) {
		emit notifyClipInfo(m_pFormatCtx->duration / AV_TIME_BASE);
	} else {
		emit notifyClipInfo(0);
	}
	//��Ƶ׼����ȡ
	AVPacket* packet = av_packet_alloc();

	//��ȡ���ݰ�
Loop:
	while (m_bRunning && av_read_frame(m_pFormatCtx, packet) >= 0) {
		// ���������ͣ״̬���ȴ������̻߳���
		if (m_bPause) {
			if (m_nVideoIndex >= 0) {
				QMutexLocker videoLocker(&m_videoMutex);
				m_videoWaitCondition.wait(&m_videoMutex);
			} else if (m_nAudioIndex >= 0) {
				QMutexLocker audioLocker(&m_audioMutex);
				m_audioWaitCondition.wait(&m_audioMutex);
			}
			av_packet_unref(packet);
			continue;
		}
		// ��ǰ���ݰ�������Ƶ��
		if (m_nVideoIndex == packet->stream_index) {
			// �����ͼ��ģʽ��ѭ������¡�����ݰ����͵���Ƶ�����У�����������߳�
			if (m_bPicture) {
				while (m_bRunning) {
					AVPacket* pkt = av_packet_clone(packet);
					QMutexLocker locker(&m_videoMutex);
					if (m_videoPacketQueue.isFull()) {
						m_videoWaitCondition.wait(&m_videoMutex);
					}
					m_videoPacketQueue.push(pkt);
					m_videoWaitCondition.wakeOne();
					m_AVSyncWaitCondition.wakeOne();
				}
			}
			// ���򽫿�¡�����ݰ����͵���Ƶ���У�����������߳�
			else {
				AVPacket* pkt = av_packet_clone(packet);
				QMutexLocker locker(&m_videoMutex);
				if (m_videoPacketQueue.isFull()) {
					m_videoWaitCondition.wait(&m_videoMutex);
				}
				m_videoPacketQueue.push(pkt);
				m_AVSyncWaitCondition.wakeOne();
				m_videoWaitCondition.wakeOne();
			}
		}
		// ��ǰ���ݰ�������Ƶ��
		else if (m_nAudioIndex == packet->stream_index) {
			// ����¡�����ݰ����͵���Ƶ���У�����������߳�
			AVPacket* pkt = av_packet_clone(packet);
			QMutexLocker locker(&m_audioMutex);
			if (m_audioPacketQueue.isFull()) {
				m_audioWaitCondition.wait(&m_audioMutex);
			}
			m_audioPacketQueue.push(pkt);
			m_audioWaitCondition.wakeOne();
		}
		// �ͷŵ�ǰ���ݰ�
		av_packet_unref(packet);
	}
	// �����������״̬����������ѭ������
	if (m_bRunning && m_bLoop) {
		// ��ͣ����λ����ʼʱ�䣬�ָ�����
		pause();
		seek(m_pFormatCtx->start_time);
		resume();
		goto Loop;
	}
	// �����ڴ�
	clearMemory();
	// ����������͸�ʽ�������ģ�д��β�����ر�IO���ͷ���Ƶ����Ƶ����������
	if (pairPushFormat.first) {
		av_write_trailer(pairPushFormat.first);
		avio_close(pairPushFormat.first->pb);
		if (std::get<1>(pairPushFormat.second)) {
			avcodec_close(std::get<1>(pairPushFormat.second));
			avcodec_free_context(&std::get<1>(pairPushFormat.second));
		}
		if (std::get<2>(pairPushFormat.second)) {
			avcodec_close(std::get<2>(pairPushFormat.second));
			avcodec_free_context(&std::get<2>(pairPushFormat.second));
		}
		avformat_free_context(pairPushFormat.first);
	}
	// �ͷ����ݰ���ý���ʽ������
	av_packet_free(&packet);
	avformat_close_input(&m_pFormatCtx);
}

void CCodecThread::LogCallBack(void* avcl, int level, const char* fmt, va_list vl) {
	QString strLogPath = qApp->applicationDirPath() + "/log/ffmpeg.log";
	QFile logFile(strLogPath);
	if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
		// �ļ��򿪳ɹ�
		// ����ʱ�����ʽ
		QString timeFmt = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.z ");
		QTextStream out(&logFile);
		out << timeFmt << QString::fromUtf8(fmt).arg(vl) << Qt::endl;
		// �ر��ļ�
		logFile.close();
	}
}

void CCodecThread::clearMemory() {
	if (m_pPushThread) {
		m_pushVideoWaitCondition.wakeAll();
		m_pushAudioWaitCondition.wakeAll();
		m_pPushThread->stop();
		SAFE_DELETE(m_pPushThread);
	}
	if (m_pVideoThread) {
		m_videoWaitCondition.wakeAll();
		m_pVideoThread->stop();
		SAFE_DELETE(m_pVideoThread);
	}
	if (m_pAudioThread) {
		m_audioWaitCondition.wakeAll();
		m_pAudioThread->stop();
		SAFE_DELETE(m_pAudioThread);
	}
	if (m_pEncodeMuteThread) {
		m_AVSyncWaitCondition.wakeAll();
		m_pEncodeMuteThread->stop();
		SAFE_DELETE(m_pEncodeMuteThread);
	}
	while (!m_videoPacketQueue.isEmpty()) {
		AVPacket* pkt = m_videoPacketQueue.pop();
		av_packet_unref(pkt);
		av_packet_free(&pkt);
	}
	m_videoPacketQueue.clear();
	while (!m_audioPacketQueue.isEmpty()) {
		AVPacket* pkt = m_audioPacketQueue.pop();
		av_packet_unref(pkt);
		av_packet_free(&pkt);
	}
	m_audioPacketQueue.clear();
	while (!m_videoPushPacketQueue.isEmpty()) {
		AVPacket* pkt = m_videoPushPacketQueue.pop();
		av_packet_unref(pkt);
		av_packet_free(&pkt);
	}
	m_videoPushPacketQueue.clear();
	while (!m_audioPushPacketQueue.isEmpty()) {
		AVPacket* pkt = m_audioPushPacketQueue.pop();
		av_packet_unref(pkt);
		av_packet_free(&pkt);
	}
	m_audioPushPacketQueue.clear();
}

// CVideoThread
CVideoThread::CVideoThread(CircularQueue<AVPacket*>& packetQueue, CircularQueue<AVPacket*>& pushPacketQueue, QWaitCondition& waitCondition,
                           QMutex& mutex, QWaitCondition& videoWaitCondition, QMutex& videoMutex, AVFormatContext* pFormatCtx,
                           const int nStreamIndex, const int nEncodeStreamIndex,
                           QPair<AVFormatContext*, std::tuple<CCalcPtsDur, AVCodecContext*>> pairOutputCtx,
                           const CCodecThread::OpenMode eMode, const bool bSendCountDown, const bool bPush, const QSize szPlay,
                           CCodecThread::eDecodeMode eDecodeMode,
                           QObject* parent)
: QThread(parent), m_pFormatCtx(pFormatCtx), m_szPlay(szPlay), m_pairOutputCtx(std::move(pairOutputCtx)), m_nStreamIndex(nStreamIndex),
  m_nEncodeStreamIndex(nEncodeStreamIndex), m_bSendCountDown(bSendCountDown), m_bPush(bPush), m_packetQueue(packetQueue),
  m_pushPacketQueue(pushPacketQueue), m_decodeWaitCondition(waitCondition), m_decodeMutex(mutex), m_pushWaitCondition(videoWaitCondition),
  m_pushMutex(videoMutex), m_eMode(eMode), m_eDecodeMode(eDecodeMode) {
}

CVideoThread::~CVideoThread() {
	this->stop();
}

void CVideoThread::pause() {
	QMutexLocker locker(&m_decodeMutex);
	m_bPause = true;
	m_encodeWaitCondition.wakeAll();
	m_playWaitCondition.wakeAll();
	// if (m_pEncodeThread) {
	// 	m_pEncodeThread->pause();
	// }
	// if (m_pPlayThread) {
	// 	m_pPlayThread->pause();
	// }
}

void CVideoThread::resume() {
	QMutexLocker locker(&m_decodeMutex);
	m_bPause = false;
	// if (m_pEncodeThread) {
	// 	m_pEncodeThread->resume();
	// }
	// if (m_pPlayThread) {
	// 	m_pPlayThread->resume();
	// }
}

void CVideoThread::stop() {
	m_bRunning = false;
	// ���������߳�
	m_encodeWaitCondition.wakeAll();
	m_playWaitCondition.wakeAll();
	// if (m_pEncodeThread) {
	// 	m_encodeWaitCondition.wakeAll();
	// 	m_pEncodeThread->stop();
	// 	SAFE_DELETE(m_pEncodeThread);
	// }
	// if (m_pPlayThread) {
	// 	m_playWaitCondition.wakeAll();
	// 	m_pPlayThread->stop();
	// 	SAFE_DELETE(m_pPlayThread);
	// }
	// �ȴ��߳�ִ�����
	this->wait();
	clearQueue(m_decodeFrameQueue);
	clearQueue(m_playFrameQueue);
}

void CVideoThread::setCurrentPts(int64_t nPts) {
	// if (m_pPlayThread) {
	// 	m_pPlayThread->setCurrentPts(nPts);
	// }
	clearQueue(m_decodeFrameQueue);
	clearQueue(m_playFrameQueue);
}

enum AVPixelFormat CVideoThread::hw_pix_fmt = AV_PIX_FMT_NONE;

// ȷ��������������Ӧʹ�õ�Ӳ����������ģʽ
AVPixelFormat CVideoThread::get_hw_format([[maybe_unused]] AVCodecContext* ctx, const AVPixelFormat* pix_fmts) {
	for (const enum AVPixelFormat* p = pix_fmts; *p != -1; ++p) {
		if (*p == hw_pix_fmt) {
			return *p;
		}
	}
	fprintf(stderr, "Failed to get HW surface format.\n");
	return AV_PIX_FMT_NONE;
}

void CVideoThread::clearQueue(CircularQueue<AVFrame*>& queue) {
	while (!queue.isEmpty()) {
		auto* frame = queue.pop();
		av_frame_unref(frame);
		av_frame_free(&frame);
	}
	queue.clear();
}

void CVideoThread::run() {
	int nRet = -1;
	char errBuf[ERRBUFSIZE]{};
	m_pCodecCtx = avcodec_alloc_context3(nullptr);
	const AVCodec* pVideoCodec{ nullptr };
	AVBufferRef* hw_device_ctx{ nullptr };

	auto logErr = [nRet, &errBuf](AVPacket* packet, const char* log) {
		av_strerror(nRet, errBuf, ERRBUFSIZE);
		av_log(nullptr, AV_LOG_ERROR, log, errBuf);
		av_packet_unref(packet);
		av_packet_free(&packet);
	};

	auto codec = [&]() {
		// ����������������������������а�
		avcodec_parameters_to_context(m_pCodecCtx, m_pFormatCtx->streams[m_nStreamIndex]->codecpar);
		// ���ݽ������� id �ҵ���Ӧ����Ƶ������
		pVideoCodec = avcodec_find_decoder(m_pCodecCtx->codec_id);
	};

	// GPU ����
	if (m_eDecodeMode == CCodecThread::eDecodeMode::eDecodeMode_GPU) {
#ifdef Q_OS_WIN
		// ѡ����Ƶ������ѵĽ�����
		av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &pVideoCodec, 0);
		// ������������Ӳ�����ã��ҵ���ָ��Ӳ���豸����ƥ��Ľ������豸
		constexpr enum AVHWDeviceType type = AV_HWDEVICE_TYPE_DXVA2;
		for (int i = 0;; i++) {
			const AVCodecHWConfig* config = avcodec_get_hw_config(pVideoCodec, i);
			if (!config) {
				fprintf(stderr, "Decoder %s doesn't support device type %s.\n", pVideoCodec->name, av_hwdevice_get_type_name(type));
				return;
			}
			if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type) {
				// ���ý����������ظ�ʽ
				hw_pix_fmt = config->pix_fmt;
				break;
			}
		}
		// ����Ƶ���Ĳ���������������������
		avcodec_parameters_to_context(m_pCodecCtx, m_pFormatCtx->streams[m_nStreamIndex]->codecpar);
		// ���ý������ص�����
		m_pCodecCtx->get_format = get_hw_format;
		if (av_hwdevice_ctx_create(&hw_device_ctx, type, nullptr, nullptr, 0) < 0) {
			fprintf(stderr, "Failed to create specified HW device.\n");
			return;
		}
		// ��Ӳ�����豸�����İ󶨵�������������
		m_pCodecCtx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
#elif defined Q_OS_UNIX
		codec();
#endif
	} else {
		codec();
	}
	// �򿪽����� m_pCodecCtx, ���������������������
	nRet = avcodec_open2(m_pCodecCtx, pVideoCodec, nullptr);
	if (nRet < 0) {
		av_strerror(nRet, errBuf, ERRBUFSIZE);
		av_log(nullptr, AV_LOG_ERROR, "Can't open video decoder: %s\n", errBuf);
		avcodec_free_context(&m_pCodecCtx);
		return;
	}
	// ���� pFrame �ṹ
	auto pFrame = av_frame_alloc();
	// ���� & ��Ҫ����
	if (CCodecThread::OpenMode::OpenMode_Push & m_eMode && m_bPush) {
		auto pairEncodeCtx = qMakePair(m_pCodecCtx, m_pairOutputCtx.second);
		// ���������߳�, ���ڽ�������֡���͵��������������������������ݰ�
		// m_pEncodeThread = new CEncodeVideoThread(m_decodeFrameQueue, m_pushPacketQueue, m_encodeWaitCondition, m_pushWaitCondition, m_pushMutex,
		//                                          m_nEncodeStreamIndex, pairEncodeCtx, m_eDecodeMode);
		// m_pEncodeThread->start();
	}
	// ����
	if (CCodecThread::OpenMode::OpenMode_Play & m_eMode) {
		// ���������̣߳����߳����ڽ�������֡���в��Ż���ʾ
		// m_pPlayThread = new CLXVideoPlayThread(m_playFrameQueue, m_playWaitCondition, m_playMutex, m_pCodecCtx,
		// 	m_pFormatCtx->streams[m_nStreamIndex]->time_base, m_bSendCountDown, this, m_szPlay, m_eDecodeMode);
		// m_pPlayThread->start();
	}
	// sw_frame �洢�� GPU �� CPU ������
	auto sw_frame = av_frame_alloc();
	// tmp_frame ������ GPU ����ʱ�ж��Ƿ���Ҫ��������Ǩ��
	AVFrame* tmp_frame{ nullptr };
	while (m_bRunning) {
		if (m_bPause) {
			continue;
		}
		// ��¼��ǰʱ��, �������ͳ�ƽ����ʱ
		QElapsedTimer ti{};
		ti.start();
		// ����
		m_decodeMutex.lock();
		// �жϽ�������Ƿ�Ϊ��, ���ѵȴ�����, ���ȴ��µ����ݰ�
		if (m_packetQueue.isEmpty()) {
			m_decodeWaitCondition.wakeOne();
			m_decodeWaitCondition.wait(&m_decodeMutex);
		}
		// �ӽ������ȡ�����ݰ�
		auto* packet = m_packetQueue.pop();
		if (Q_LIKELY(packet)) {
			// ���������
			nRet = avcodec_send_packet(m_pCodecCtx, packet);
			if (nRet < 0) {
				logErr(packet, "Can't send packet for decoding: %s\n");
				m_decodeMutex.unlock();
				continue;
			}
			// ��ȡѭ����������Ƶ֡
			while (avcodec_receive_frame(m_pCodecCtx, pFrame) >= 0) {
				// ���֡��ʽ�� GPU ����ʽ, ���Խ����ݴ� GPU ת�Ƶ� CPU
				if (pFrame->format == hw_pix_fmt) {
					if (av_hwframe_transfer_data(sw_frame, pFrame, 0) < 0) {
						logErr(packet, "Error transferring the data to system memory, %s\n");
						m_decodeMutex.unlock();
						continue;
					}
					// �� tmp_frame ָ�� sw_frame
					tmp_frame = sw_frame;
					tmp_frame->pts = pFrame->pts;
					tmp_frame->pkt_dts = pFrame->pkt_dts;
				} else {
					tmp_frame = pFrame;
				}
				// �Ƿ�����
				if (CCodecThread::OpenMode::OpenMode_Push & m_eMode && m_bPush) {
					// ����, �ڶ�����ʱ���ѵȴ�����
					m_encodeMutex.lock();
					if (m_decodeFrameQueue.isFull()) {
						m_encodeWaitCondition.wait(&m_encodeMutex);
					}
					if (const auto pCopyFrame = av_frame_clone(tmp_frame)) {
						m_decodeFrameQueue.push(pCopyFrame);
						m_encodeWaitCondition.wakeOne();
					}
					m_encodeMutex.unlock();
				}
				// �Ƿ񲥷�
				if (CCodecThread::OpenMode::OpenMode_Play & m_eMode) {
					// ����, �ڶ�����ʱ���ѵȴ�����
					m_playMutex.lock();
					if (m_playFrameQueue.isFull()) {
						m_playWaitCondition.wait(&m_playMutex);
					}
					if (const auto pCopyFrame = av_frame_clone(tmp_frame)) {
						m_playFrameQueue.push(pCopyFrame);
						m_playWaitCondition.wakeOne();
					}
					m_playMutex.unlock();
				}
			}
			// �ͷŽ���ʹ����Դ
			av_packet_unref(packet);
			av_packet_free(&packet);
			av_frame_unref(pFrame);
		}
		m_decodeMutex.unlock();
	}
	// �ͷ�ʣ����Դ, ���رս�����
	av_frame_free(&pFrame);
	av_frame_free(&sw_frame);
	av_frame_free(&tmp_frame);
	if (hw_device_ctx) {
		av_buffer_unref(&hw_device_ctx);
	}
	avcodec_close(m_pCodecCtx);
	avcodec_free_context(&m_pCodecCtx);
}

// CAudioThread
CAudioThread::CAudioThread(CircularQueue<AVPacket*>& packetQueue, CircularQueue<AVPacket*>& pushPacketQueue, QWaitCondition& waitCondition,
                           QMutex& mutex, QWaitCondition& audioWaitCondition, QMutex& audioMutex, AVFormatContext* pFormatCtx,
                           const int nStreamIndex, const int nEncodeStreamIndex,
                           QPair<AVFormatContext*, std::tuple<CCalcPtsDur, AVCodecContext*>> pairOutputCtx,
                           const CCodecThread::OpenMode eMode, const bool bPush, const AVCodecParameters& decodePara, QObject* parent)
: QThread(parent), m_pFormatCtx(pFormatCtx), m_decodePara(decodePara), m_pairOutputCtx(std::move(pairOutputCtx)), m_nStreamIndex(nStreamIndex),
  m_nEncodeStreamIndex(nEncodeStreamIndex), m_bPush(bPush), m_packetQueue(packetQueue), m_pushPacketQueue(pushPacketQueue),
  m_decodeWaitCondition(waitCondition), m_decodeMutex(mutex), m_pushWaitCondition(audioWaitCondition), m_pushMutex(audioMutex), m_eMode(eMode) {
}

CAudioThread::~CAudioThread() {
	stop();
}

void CAudioThread::pause() {
	QMutexLocker loacker(&m_decodeMutex);
	m_encodeWaitCondition.wakeAll();
	m_playWaitCondition.wakeAll();
	if (m_pEncodeThread) {
		m_pEncodeThread->pause();
	}
	if (m_pPlayThread) {
		m_pPlayThread->pause();
	}
	m_bPause = true;
}

void CAudioThread::resume() {
	QMutexLocker locker(&m_decodeMutex);
	m_bPause = false;
	if (m_pEncodeThread) {
		m_pEncodeThread->resume();
	}
	if (m_pPlayThread) {
		m_pPlayThread->resume();
	}
}

void CAudioThread::stop() {
	m_bRunning = false;
	m_encodeWaitCondition.wakeAll();
	m_playWaitCondition.wakeAll();
	if (m_pEncodeThread) {
		m_encodeWaitCondition.wakeAll();
		m_pEncodeThread->stop();
		SAFE_DELETE(m_pEncodeThread);
	}
	if (m_pPlayThread) {
		m_playWaitCondition.wakeAll();
		m_pPlayThread->stop();
		SAFE_DELETE(m_pPlayThread);
	}
	this->wait();
	clearQueue(m_decodeFrameQueue);
	clearQueue(m_playFrameQueue);
}

void CAudioThread::setCurrentPts(const int64_t nPts) {
	if (m_pPlayThread) {
		m_pPlayThread->setCurrentPts(nPts);
	}
	clearQueue(m_decodeFrameQueue);
	clearQueue(m_playFrameQueue);
}

void CAudioThread::clearQueue(CircularQueue<AVFrame*>& queue) {
	while (!queue.isEmpty()) {
		auto* frame = queue.pop();
		av_frame_unref(frame);
		av_frame_free(&frame);
	}
	queue.clear();
}

void CAudioThread::run() {
	int nRet{ -1 };
	char errBuf[ERRBUFSIZE]{};
	// ���䲢��ʼ����Ƶ������������
	m_pCodecCtx = avcodec_alloc_context3(nullptr);
	auto errlog = [nRet, &errBuf](AVPacket* packet, const char* log) {
		av_strerror(nRet, errBuf, ERRBUFSIZE);
		av_log(nullptr, AV_LOG_ERROR, log, errBuf);
		av_packet_unref(packet);
		av_packet_free(&packet);
	};
	// ���� & ����������
	if (m_bPush && m_nStreamIndex < 0) {
		// ʹ�ô���Ľ������
		avcodec_parameters_to_context(m_pCodecCtx, &m_decodePara);
	} else {
		// ʹ�����Ľ������
		avcodec_parameters_to_context(m_pCodecCtx, m_pFormatCtx->streams[m_nStreamIndex]->codecpar);
	}
	// ͨ�� id �ҵ���Ƶ������
	const auto* pAudioCodec = avcodec_find_decoder(m_pCodecCtx->codec_id);
	// ����Ƶ������
	nRet = avcodec_open2(m_pCodecCtx, pAudioCodec, nullptr);
	if (nRet < 0) {
		av_strerror(nRet, errBuf, ERRBUFSIZE);
		av_log(nullptr, AV_LOG_ERROR, "Can't open audio decoder, %s\n", errBuf);
		avcodec_free_context(&m_pCodecCtx);
		return;
	}
	// ������Ƶ����֪ͨ
	emit notifyAudioPara(m_pCodecCtx->sample_rate, m_pCodecCtx->channels);
	// ���� | ����, ������Ӧ�߳�
	if (CCodecThread::OpenMode::OpenMode_Push & m_eMode && m_bPush) {
		const auto pairEncodeCtx = qMakePair(m_pCodecCtx, m_pairOutputCtx.second);
		m_pEncodeThread = new CEncodeAudioThread(m_decodeFrameQueue, m_pushPacketQueue, m_encodeWaitCondition, m_encodeMutex, m_pushWaitCondition,
		                                         m_pushMutex, m_nEncodeStreamIndex, pairEncodeCtx);
		m_pEncodeThread->start();
	}
	if (CCodecThread::OpenMode::OpenMode_Play & m_eMode) {
		m_pPlayThread = new CAudioPlayThread(m_playFrameQueue, m_playWaitCondition, m_playMutex, m_pCodecCtx,
		                                     m_nStreamIndex >= 0 ? m_pFormatCtx->streams[m_nStreamIndex]->time_base : m_pCodecCtx->time_base, this);
		m_pPlayThread->start();
	}

	auto* pFrame = av_frame_alloc();
	while (m_bRunning) {
		if (m_bPause) {
			continue;
		}
		m_decodeMutex.lock();
		// �����Ƶ������Ϊ��, ���Ѳ��ȴ�
		if (m_packetQueue.isEmpty()) {
			m_decodeWaitCondition.wakeOne();
			m_decodeWaitCondition.wait(&m_decodeMutex);
		}
		// ȡ����Ƶ��
		auto* packet = m_packetQueue.pop();
		if (Q_LIKELY(packet)) {
			// ����
			nRet = avcodec_send_packet(m_pCodecCtx, packet);
			if (nRet < 0) {
				errlog(packet, "Can't send audio packet, %s\n");
				m_decodeMutex.unlock();
				continue;
			}
			// ���ս�������Ƶ֡
			while (avcodec_receive_frame(m_pCodecCtx, pFrame) >= 0) {
				// ���� | ����, ����������Ƶ֡������Ӧ�Ķ���
				if (CCodecThread::OpenMode::OpenMode_Push & m_eMode && m_bPush) {
					m_encodeMutex.lock();
					if (m_decodeFrameQueue.isFull()) {
						m_encodeWaitCondition.wait(&m_encodeMutex);
					}
					if (const auto pCopyFrame = av_frame_clone(pFrame)) {
						m_decodeFrameQueue.push(pCopyFrame);
						m_encodeWaitCondition.wakeOne();
					}
					m_encodeMutex.unlock();
				}
				if (CCodecThread::OpenMode::OpenMode_Play & m_eMode) {
					m_playMutex.lock();
					if (m_playFrameQueue.isFull()) {
						m_playWaitCondition.wait(&m_playMutex);
					}
					if (const auto pCopyFrame = av_frame_clone(pFrame)) {
						m_playFrameQueue.push(pCopyFrame);
						m_encodeWaitCondition.wakeOne();
					}
					m_playMutex.unlock();
				}
			}
			// �ͷ���Դ
			av_packet_unref(packet);
			av_packet_free(&packet);
			av_frame_unref(pFrame);
		}
		m_decodeMutex.unlock();
	}
	// �ͷ�ʣ����Դ, ���رս�����
	av_frame_free(&pFrame);
	avcodec_close(m_pCodecCtx);
	avcodec_free_context(&m_pCodecCtx);
}

//CVideoPlayThread
CVideoPlayThread::CVideoPlayThread(CircularQueue<AVFrame*>& decodeFrameQueue, QWaitCondition& waitCondition, QMutex& mutex,
                                   AVCodecContext* pCodecCtx, const AVRational& timeBase, const bool bSendCountDown, CVideoThread* videoThread,
                                   const QSize szPlay, CCodecThread::eDecodeMode eDecodeMode, QObject* parent)
: QThread(parent), m_pCodecCtx(pCodecCtx), m_bSendCountDown(bSendCountDown), m_timeBase(timeBase), m_videoThread(videoThread),
  m_playFrameQueue(decodeFrameQueue), m_playWaitCondition(waitCondition), m_playMutex(mutex), m_szPlay(szPlay), m_eDecodeMode(eDecodeMode) {
}

CVideoPlayThread::~CVideoPlayThread() = default;

void CVideoPlayThread::pause() {
	QMutexLocker locker(&m_playMutex);
	m_bPause = true;
}

void CVideoPlayThread::resume() {
	QMutexLocker locaer(&m_playMutex);
	m_bPause = false;
	m_nLastTime = av_gettime();
}

void CVideoPlayThread::stop() {
	m_bRunning = false;
	m_playWaitCondition.wakeOne();
	this->wait();
}

void CVideoPlayThread::setCurrentPts(const int64_t nPts) {
	if (m_pCodecCtx) {
		avcodec_flush_buffers(m_pCodecCtx);
		m_nLastPts = nPts;
	}
}

void CVideoPlayThread::run() {
	// �������ڴ洢 RGB ��ʽ֡�� AVFrame
	auto pFrameRGB = av_frame_alloc();
	if (!pFrameRGB) {
		return;
	}
	// ���� RGB ͼ�����ݻ�����
	[[maybe_unused]] int iSize = av_image_alloc(pFrameRGB->data, pFrameRGB->linesize, m_szPlay.width(), m_szPlay.height(), AV_PIX_FMT_RGB32, 1);
	// ����ͼ��ת����
	const auto srcFormat = m_eDecodeMode == CCodecThread::eDecodeMode::eDecodeMode_CPU ? m_pCodecCtx->pix_fmt : AV_PIX_FMT_NV12;
	struct SwsContext* img_decode_convert_ctx = sws_getContext(m_pCodecCtx->width, m_pCodecCtx->height, srcFormat
	                                                           , m_szPlay.width(), m_szPlay.height(),
	                                                           AV_PIX_FMT_RGB32, SWS_BICUBIC, nullptr, nullptr, nullptr);
	// ��ʼ������ʱ���
	m_nLastTime = av_gettime();
	m_nLastPts = 0;
	while (m_bRunning) {
		if (m_bPause) {
			continue;
		}
		m_playMutex.lock();
		if (m_playFrameQueue.isEmpty()) {
			m_playWaitCondition.wakeOne();
			m_playWaitCondition.wait(&m_playMutex);
		}
		auto* pFrame = m_playFrameQueue.pop();
		if (Q_LIKELY(pFrame)) {
			// ���͵���ʱ֪ͨ
			if (m_bSendCountDown) {
				const int64_t nCurTimeStamp = pFrame->pts * static_cast<int64_t>(av_q2d(m_timeBase));
				emit m_videoThread->notifyCountDown(nCurTimeStamp);
			}
			// ����ʱ��ƫ��
			const int64_t nTimeStampOffset = (pFrame->pts - m_nLastPts) * AV_TIME_BASE * static_cast<int64_t>(av_q2d(m_timeBase));
			// ������֡ת��Ϊ RGB ��ʽ
			[[maybe_unused]] int nH = sws_scale(img_decode_convert_ctx, pFrame->data, pFrame->linesize, 0, m_pCodecCtx->height, pFrameRGB->data,
			                                    pFrameRGB->linesize);
			QImage tmpImg(const_cast<const uchar*>(pFrameRGB->data[0]), m_szPlay.width(), m_szPlay.height(), QImage::Format_RGB32);
			tmpImg.detach();
			// this -> CVideoThread -> CPlaybackWidget
			emit m_videoThread->notifyImage(QPixmap::fromImage(tmpImg));
			// ����ʱ��ƫ�Ʋ�����
			if (const int64_t nTimeOffset = nTimeStampOffset - av_gettime() + m_nLastTime; nTimeOffset > 0) {
				av_usleep(nTimeOffset);
			}
			// �ͷ�֡����
			av_frame_unref(pFrame);
			av_frame_free(&pFrame);
		}
		m_playMutex.unlock();
	}
	// �ͷ� RGB ͼ�����ݻ�����, RGB ֡, ͼ��ת����
	av_freep(&pFrameRGB->data[0]);
	av_frame_free(&pFrameRGB);
	sws_freeContext(img_decode_convert_ctx);
}

// CAudioPlayThread
CAudioPlayThread::CAudioPlayThread(CircularQueue<AVFrame*>& decodeFrameQueue, QWaitCondition& waitCondition,
                                   QMutex& mutex, AVCodecContext* pCodecCtx, const AVRational& timeBase,
                                   CAudioThread* audioThread, QObject* parent)
: QThread(parent), m_pCodecCtx(pCodecCtx), m_timeBase(timeBase), m_audioThread(audioThread),
  m_playFrameQueue(decodeFrameQueue), m_playWaitCondition(waitCondition), m_playMutex(mutex) {
}

CAudioPlayThread::~CAudioPlayThread() {
	stop();
}

void CAudioPlayThread::pause() {
	QMutexLocker locker(&m_playMutex);
	m_bPause = true;
}

void CAudioPlayThread::resume() {
	QMutexLocker locker(&m_playMutex);
	m_nLastTime = av_gettime();
	m_bPause = false;
}

void CAudioPlayThread::stop() {
	m_bRunning = false;
	m_playWaitCondition.wakeOne();
	this->wait();
}

void CAudioPlayThread::setCurrentPts(const int64_t nPts) {
	if (m_pCodecCtx) {
		avcodec_flush_buffers(m_pCodecCtx);
		m_nLastPts = nPts;
	}
}

void CAudioPlayThread::run() {
	const int out_channel_nb = av_get_channel_layout_nb_channels(m_pCodecCtx->channel_layout);
	// �������ڴ洢ת������Ƶ���ݵĻ�����
	auto* audio_decode_buffer = static_cast<uint8_t*>(av_malloc(m_pCodecCtx->channels * m_pCodecCtx->sample_rate));
	// ������Ƶת����
	auto audio_decode_swrCtx = swr_alloc_set_opts(nullptr, static_cast<int64_t>(m_pCodecCtx->channel_layout), AV_SAMPLE_FMT_S16,
	                                              m_pCodecCtx->sample_rate, static_cast<int64_t>(m_pCodecCtx->channel_layout),
	                                              m_pCodecCtx->sample_fmt, m_pCodecCtx->sample_rate, 0, nullptr);
	swr_init(audio_decode_swrCtx);
	// ��ʼ������ʱ���
	m_nLastTime = av_gettime();
	m_nLastPts = 0;
	while (m_bRunning) {
		if (m_bPause) {
			continue;
		}
		m_playMutex.lock();
		if (m_playFrameQueue.isEmpty()) {
			m_playWaitCondition.wakeOne();
			m_playWaitCondition.wait(&m_playMutex);
		}
		int64_t nTimeStampOffset{ 0 };
		auto pFrame = m_playFrameQueue.pop();
		if (Q_LIKELY(pFrame)) {
			// ���͵���ʱ֪ͨ
			const int64_t nCurTimeStamp = pFrame->pts * static_cast<int64_t>(av_q2d(m_timeBase));
			emit m_audioThread->notifyCountDown(nCurTimeStamp);
			// ����ʱ��ƫ��
			nTimeStampOffset = (pFrame->pts - m_nLastPts) * AV_TIME_BASE * static_cast<int64_t>(av_q2d(m_timeBase));
			// ִ����Ƶת��
			swr_convert(audio_decode_swrCtx, &audio_decode_buffer, m_pCodecCtx->channels * m_pCodecCtx->sample_rate,
			            const_cast<const uint8_t**>(pFrame->data), pFrame->nb_samples);
			// ���������Ƶ���ݴ�С
			const int out_audio_buffer_size = av_samples_get_buffer_size(nullptr, out_channel_nb, pFrame->nb_samples, AV_SAMPLE_FMT_S16, 1);
			// this -> CAudioThread -> CAudioPlayWidget
			// data -> m_audioByteBuffer -> m_pAudioDevice -> m_pAudioDevice = m_pAudioOutput->start()
			QByteArray data(reinterpret_cast<const char*>(audio_decode_buffer), out_audio_buffer_size);
			emit m_audioThread->notifyAudio(data);
			// �ͷ�֡����
			av_frame_unref(pFrame);
			av_frame_free(&pFrame);
		}
		m_playMutex.unlock();
		// ����ʱ��ƫ�Ʋ�����, ȷ��������Ƶ��ʱ������в���
		if (const int64_t nTimeOffset = nTimeStampOffset - av_gettime() + m_nLastTime; nTimeOffset > 0) {
			av_usleep(nTimeOffset);
		}
	}
	// �ͷ�ת������Ƶ���ݻ�����, �ͷ���Ƶת����
	av_free(&audio_decode_buffer);
	swr_free(&audio_decode_swrCtx);
}

CEncodeVideoThread::CEncodeVideoThread(CircularQueue<AVFrame*>& encodeFrameQueue, CircularQueue<AVPacket*>& pushPacketQueue,
                                       QWaitCondition& encodeWaitCondition, QMutex& encodeMutex, QWaitCondition& pushWaitCondition, QMutex& pushMutex,
                                       const int nEncodeStreamIndex, QPair<AVCodecContext*,
                                                                           std::tuple<CCalcPtsDur, AVCodecContext*>> pairEncodeCtx,
                                       CCodecThread::eDecodeMode eDecodeMode, QObject* parent)
: QThread(parent), m_encodeFrameQueue(encodeFrameQueue), m_pushPacketQueue(pushPacketQueue), m_nEncodeStreamIndex(nEncodeStreamIndex),
  m_encodeWaitCondition(encodeWaitCondition), m_encodeMutex(encodeMutex), m_pushWaitCondition(pushWaitCondition), m_pushMutex(pushMutex),
  m_pairEncodeCtx(std::move(pairEncodeCtx)), m_eEncodeMode(eDecodeMode) {
}

CEncodeVideoThread::~CEncodeVideoThread() {
	stop();
}

void CEncodeVideoThread::pause() {
	QMutexLocker locker(&m_encodeMutex);
	m_bPause = true;
}

void CEncodeVideoThread::resume() {
	QMutexLocker locker(&m_encodeMutex);
	m_bPause = false;
}

void CEncodeVideoThread::stop() {
	m_bRunning = false;
	m_encodeWaitCondition.wakeOne();
	this->wait();
}

void CEncodeVideoThread::run() {
	char errBuf[ERRBUFSIZE]{};
	int nRet{ 0 };
	int frame_index{ 0 };

	auto logWaring = [nRet, &errBuf](AVPacket* packet, const char* log) {
		av_strerror(nRet, errBuf, ERRBUFSIZE);
		av_log(nullptr, AV_LOG_WARNING, log, errBuf);
		av_packet_unref(packet);
	};
	// ��ȡ������
	if (const auto pEncodeCtx = std::get<1>(m_pairEncodeCtx.second)) {
		// �������ڴ洢 YUV420P ��ʽ֡�� AVFrame
		auto pFrameYUV420P = av_frame_alloc();
		auto* video_convert_buffer = static_cast<uint8_t*>(av_malloc(
			av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pEncodeCtx->width, pEncodeCtx->height, 1)));
		av_image_fill_arrays(pFrameYUV420P->data, pFrameYUV420P->linesize, video_convert_buffer, AV_PIX_FMT_YUV420P,
		                     pEncodeCtx->width, pEncodeCtx->height, 1);
		// ����ͼ��ת����
#ifdef Q_OS_WIN
		auto srcFormat = m_eEncodeMode == CCodecThread::eDecodeMode::eDecodeMode_CPU ? m_pairEncodeCtx.first->pix_fmt : AV_PIX_FMT_NV12;
		auto flags = SWS_BICUBIC;
#else
		auto srcFormat = m_pairEncodeCtx.first->pix_fmt;
		auto flags = SWS_ACCURATE_RND | SWS_FAST_BILINEAR;
#endif
		const auto img_encode_convert_ctx = sws_getContext(m_pairEncodeCtx.first->width, m_pairEncodeCtx.first->height, srcFormat, pEncodeCtx->width,
		                                                   pEncodeCtx->height, AV_PIX_FMT_YUV420P, flags, nullptr, nullptr, nullptr);
		// ���� AVFrame �ĸ�ʽΪ YUV420P
		pFrameYUV420P->format = AV_PIX_FMT_YUV420P;
		// �������ڴ洢��������ݵ� AVPacket
		auto pPushPacket = av_packet_alloc();
		while (m_bRunning) {
			if (m_bPause) {
				continue;
			}
			m_encodeMutex.lock();
			if (m_encodeFrameQueue.isEmpty()) {
				m_encodeWaitCondition.wakeOne();
				m_encodeWaitCondition.wait(&m_encodeMutex);
			}
			if (const auto pFrame = m_encodeFrameQueue.pop(); Q_LIKELY(pFrame)) {
				// ��ȡ���� PTS ����
				const auto& calPts = std::get<0>(m_pairEncodeCtx.second);
				// ʹ��ԭʼ֡���б���
				auto pEncodeFrame = pFrame;
				// �����������֧�� NV12 ��ʽ����֡�ķֱ��ʲ�����Ҫ��, ���и�ʽת��
				if (AV_PIX_FMT_YUV420P != m_pairEncodeCtx.first->pix_fmt || m_pairEncodeCtx.first->width != pEncodeCtx->width ||
					m_pairEncodeCtx.first->height != pEncodeCtx->height) {
					nRet = sws_scale(img_encode_convert_ctx, pFrame->data, pFrame->linesize, 0, m_pairEncodeCtx.first->height,
					                 pFrameYUV420P->data, pFrameYUV420P->linesize);
					if (nRet < 0) {
						logWaring(pPushPacket, "video sws_scale yuv420p fail, %s\n");
						m_encodeMutex.unlock();
						continue;
					}
					// ����֡���
					pFrameYUV420P->width = pEncodeCtx->width;
					pFrameYUV420P->height = pEncodeCtx->height;
					// ʹ��ת�����֡���н���
					pEncodeFrame = av_frame_clone(pFrameYUV420P);
				}
				// ����֡�� PTS �����͸�������
				pEncodeFrame->pts = calPts.GetVideoPts(frame_index);
				nRet = avcodec_send_frame(pEncodeCtx, pEncodeFrame);
				if (nRet < 0) {
					logWaring(pPushPacket, "video avcodec_send_frame fail, %s\n");
					m_encodeMutex.unlock();
					continue;
				}
				// ���ձ��������ݰ�
				nRet = avcodec_receive_packet(pEncodeCtx, pPushPacket);
				if (nRet < 0) {
					logWaring(pPushPacket, "video avcodec_receive_packet fail, %s\n");
					m_encodeMutex.unlock();
					continue;
				}
				// �������Ͱ�������
				pPushPacket->stream_index = m_nEncodeStreamIndex;
				m_pushMutex.lock();
				if (m_pushPacketQueue.isFull()) {
					m_pushWaitCondition.wait(&m_pushMutex);
				}
				// �����ݰ���¡һ�ݲ�����
				m_pushPacketQueue.push(av_packet_clone(pPushPacket));
				m_pushWaitCondition.wakeOne();
				m_pushMutex.unlock();
				// �ͷ���Դ
				av_frame_unref(pEncodeFrame);
				av_frame_free(&pEncodeFrame);
				av_frame_unref(pFrame);
				av_packet_unref(pPushPacket);
				frame_index++;
			}
			m_encodeMutex.unlock();
		}
		av_packet_free(&pPushPacket);
		av_frame_free(&pFrameYUV420P);
		av_free(&video_convert_buffer);
		sws_freeContext(img_encode_convert_ctx);
	}
}

// CEncodeAudioThread
CEncodeAudioThread::CEncodeAudioThread(CircularQueue<AVFrame*>& encodeFrameQueue, CircularQueue<AVPacket*>& pushPacketQueue,
                                       QWaitCondition& encodeWaitCondition, QMutex& encodeMutex, QWaitCondition& pushWaitCondition, QMutex& pushMutex,
                                       const int nEncodeStreamIndex, QPair<AVCodecContext*, std::tuple<CCalcPtsDur, AVCodecContext*>> pairEncodeCtx,
                                       QObject* parent)
: QThread(parent), m_encodeFrameQueue(encodeFrameQueue), m_pushPacketQueue(pushPacketQueue), m_nEncodeStreamIndex(nEncodeStreamIndex),
  m_encodeWaitCondition(encodeWaitCondition), m_encodeMutex(encodeMutex), m_pushWaitCondition(pushWaitCondition), m_pushMutex(pushMutex),
  m_pairEncodeCtx(std::move(pairEncodeCtx)) {
}

CEncodeAudioThread::~CEncodeAudioThread() {
	stop();
}

void CEncodeAudioThread::pause() {
	QMutexLocker locker(&m_encodeMutex);
	m_bPause = true;
}

void CEncodeAudioThread::resume() {
	QMutexLocker locker(&m_encodeMutex);
	m_bPause = false;
}

void CEncodeAudioThread::stop() {
	m_bRunning = false;
	m_encodeWaitCondition.wakeOne();
	this->wait();
}

void CEncodeAudioThread::run() {
	char errBuf[ERRBUFSIZE]{};
	int nRet{ 0 };
	int frame_index{ 0 };

	auto logWaring = [nRet, &errBuf](AVPacket* packet, const char* log) {
		av_strerror(nRet, errBuf, ERRBUFSIZE);
		av_log(nullptr, AV_LOG_WARNING, log, errBuf);
		av_packet_unref(packet);
	};
	// ��ȡ������
	if (const auto pEncodeCtx = std::get<1>(m_pairEncodeCtx.second)) {
		// �������ڴ洢����ǰ��Ƶ֡�Ŀռ�
		auto pFrameAAC = av_frame_alloc();
		// ������Ƶ�ز��������Ĳ����ò���
		auto audio_encode_swrCtx = swr_alloc_set_opts(nullptr, static_cast<int64_t>(pEncodeCtx->channel_layout), AV_SAMPLE_FMT_FLT,
		                                              pEncodeCtx->sample_rate, static_cast<int64_t>(m_pairEncodeCtx.first->channel_layout),
		                                              m_pairEncodeCtx.first->sample_fmt, m_pairEncodeCtx.first->sample_rate, 0, nullptr);
		swr_init(audio_encode_swrCtx);
		// ���䲢��ʼ���������ݰ�
		auto pPushPacket = av_packet_alloc();
		while (m_bRunning) {
			if (m_bPause) {
				continue;
			}
			m_encodeMutex.lock();
			if (m_encodeFrameQueue.isEmpty()) {
				m_encodeWaitCondition.wakeOne();
				m_encodeWaitCondition.wait(&m_encodeMutex);
			}
			if (const auto pFrame = m_encodeFrameQueue.pop(); Q_LIKELY(pFrame)) {
				// ������Ƶ֡
				const auto& calPts = std::get<0>(m_pairEncodeCtx.second);
				auto pEncodeFrame = pFrame;
				// �����Ƶ��ʽ���� AV_SAMPLE_FMT_FLTP, �������Ƶ�ز���
				if (AV_SAMPLE_FMT_FLTP != m_pairEncodeCtx.first->sample_fmt) {
					nRet = swr_convert_frame(audio_encode_swrCtx, pFrameAAC, pFrame);
					if (nRet < 0) {
						logWaring(pPushPacket, "audio swr_convert_frame fail, %s\n");
						m_encodeMutex.unlock();
						continue;
					}
					// ��¡�ز��������Ƶ֡
					pEncodeFrame = av_frame_clone(pFrameAAC);
				}
				// ������Ƶ֡��ʱ���
				pEncodeFrame->pts = calPts.GetAudioPts(frame_index, pEncodeCtx->sample_rate);
				// ����Ƶ֡���͸�������
				nRet = avcodec_send_frame(pEncodeCtx, pEncodeFrame);
				if (nRet < 0) {
					logWaring(pPushPacket, "audio avcodec_send_frame fail, %s\n");
					m_encodeMutex.unlock();
					continue;
				}
				// ���ձ�������Ƶ���ݰ�
				nRet = avcodec_receive_packet(pEncodeCtx, pPushPacket);
				if (nRet < 0) {
					logWaring(pPushPacket, "audio avcodec_receive_packet fail, %s\n");
					m_encodeMutex.unlock();
					continue;
				}
				// �������Ͱ�������
				pPushPacket->stream_index = m_nEncodeStreamIndex;
				m_pushMutex.lock();
				if (m_pushPacketQueue.isFull()) {
					m_pushWaitCondition.wait(&m_pushMutex);
				}
				// ����¡������ݰ����͵�����
				m_pushPacketQueue.push(av_packet_clone(pPushPacket));
				m_pushWaitCondition.wakeOne();
				m_pushMutex.unlock();
				// �ͷ�֡�����ݰ�
				av_frame_unref(pFrame);
				av_frame_unref(pEncodeFrame);
				av_frame_free(&pEncodeFrame);
				av_packet_unref(pPushPacket);
				// ����֡����
				frame_index++;
			}
			m_encodeMutex.unlock();
		}
		// �ͷ���Դ
		av_frame_free(&pFrameAAC);
		av_packet_free(&pPushPacket);
		swr_free(&audio_encode_swrCtx);
	}
}
// CEncodeMuteAudioThread
CEncodeMuteAudioThread::CEncodeMuteAudioThread(CircularQueue<AVPacket*>& pushPacketQueue, QWaitCondition& encodeWaitCondition, QMutex& encodeMutex,
                                               QWaitCondition& syncWaitCondition, QMutex& syncMutex, const int nEncodeStreamIndex,
                                               std::tuple<CCalcPtsDur, AVCodecContext*> pairEncodeCtx, QObject* parent)
: QThread(parent), m_encodePacketQueue(pushPacketQueue), m_nEncodeStreamIndex(nEncodeStreamIndex), m_encodeWaitCondition(encodeWaitCondition),
  m_encodeMutex(encodeMutex), m_syncWaitCondition(syncWaitCondition), m_syncMutex(syncMutex), m_pairEncodeCtx(std::move(pairEncodeCtx)) {
}

CEncodeMuteAudioThread::~CEncodeMuteAudioThread() {
	stop();
}

void CEncodeMuteAudioThread::pause() {
	QMutexLocker locker(&m_encodeMutex);
	m_bPause = true;
}

void CEncodeMuteAudioThread::resume() {
	QMutexLocker locker(&m_encodeMutex);
	m_bPause = false;
}

void CEncodeMuteAudioThread::stop() {
	m_bRunning = false;
	this->wait();
}

void CEncodeMuteAudioThread::run() {
	char errBuf[ERRBUFSIZE]{};
	int nRet{ 0 };
	int frame_index{ 0 };

	auto logWaring = [nRet, &errBuf](AVFrame* frame, AVPacket* packet, const char* log) {
		av_strerror(nRet, errBuf, ERRBUFSIZE);
		av_log(nullptr, AV_LOG_WARNING, log, errBuf);
		av_frame_unref(frame);
		av_packet_unref(packet);
	};
	if (const auto pEncodeCtx = std::get<1>(m_pairEncodeCtx)) {
		// ���� AAC ������
		const auto out_AudioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
		// ���������Ƶ������������
		auto* pOutputAudioCodecCtx = avcodec_alloc_context3(nullptr);
		// ���������Ƶ�����������Ĳ���
		pOutputAudioCodecCtx->profile = FF_PROFILE_AAC_LOW;    // ����Э��
		pOutputAudioCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO; // ��Ƶ����
		pOutputAudioCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
		pOutputAudioCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
		pOutputAudioCodecCtx->channels = 2;
		pOutputAudioCodecCtx->sample_rate = pEncodeCtx->sample_rate;
		pOutputAudioCodecCtx->bit_rate = pEncodeCtx->bit_rate;

		// ���ñ���������
		AVDictionary* param{ nullptr };
		av_dict_set(&param, "preset", "superfast", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);
		// �������Ƶ������
		nRet = avcodec_open2(pOutputAudioCodecCtx, out_AudioCodec, &param);
		if (nRet < 0) {
			av_strerror(nRet, errBuf, ERRBUFSIZE);
			av_log(nullptr, AV_LOG_ERROR, "Can't open audio encoder %s\n", errBuf);
			avcodec_free_context(&pOutputAudioCodecCtx);
			return;
		}
		if (pOutputAudioCodecCtx) {
			// ���������Ƶ�����İ�
			auto* pEncodePacket = av_packet_alloc();
			// �������ھ�������Ƶ֡
			auto* pMuteFrame = av_frame_alloc();
			pMuteFrame->sample_rate = pEncodeCtx->sample_rate;
			pMuteFrame->channel_layout = pEncodeCtx->channel_layout;
			pMuteFrame->nb_samples = 1024; // Ĭ�ϵ�sample��С
			pMuteFrame->channels = 2;
			pMuteFrame->format = pEncodeCtx->sample_fmt;
			// ��ȡ������Ƶ֡�����ݻ�����
			nRet = av_frame_get_buffer(pMuteFrame, 0);
			if (nRet < 0) {
				av_frame_free(&pMuteFrame);
				return;
			}
			// ��������Ƶ֡����������Ϊ����
			av_samples_set_silence(pMuteFrame->data, 0, pMuteFrame->nb_samples, pMuteFrame->channels, pEncodeCtx->sample_fmt);
			const auto& calPts = std::get<0>(m_pairEncodeCtx);
			while (m_bRunning) {
				if (m_bPause) {
					continue;
				}
				QMutexLocker locker(&m_syncMutex);
				// �ȴ������̵߳��źţ����ȴ���Ƶ�����߳�֪ͨ���Կ�ʼ������һ֡������Ƶ
				m_syncWaitCondition.wait(&m_syncMutex);
				// ѭ��������֡��Ƶ��Ƶ
				for (int i = 0; i < 2; i++) {
					// ��¡������Ƶ֡
					auto* pCopyFrame = av_frame_clone(pMuteFrame);
					pCopyFrame->pts = calPts.GetAudioPts(frame_index, pEncodeCtx->sample_rate);
					// ���;�����Ƶ֡���б���
					nRet = avcodec_send_frame(pOutputAudioCodecCtx, pCopyFrame);
					if (nRet < 0) {
						logWaring(pCopyFrame, pEncodePacket, "audio avcodec_send_frame fail, %s\n");
						continue;
					}
					nRet = avcodec_receive_packet(pOutputAudioCodecCtx, pEncodePacket);
					if (nRet < 0) {
						logWaring(pCopyFrame, pEncodePacket, "audio avcodec_receive_packet fail, %s\n");
						continue;
					}
					// ������Ƶ����������
					pEncodePacket->stream_index = m_nEncodeStreamIndex;
					m_encodeMutex.lock();
					if (m_encodePacketQueue.isFull()) {
						m_encodeWaitCondition.wait(&m_encodeMutex);
					}
					// ����¡������ݰ����͵�����
					m_encodePacketQueue.push(av_packet_clone(pEncodePacket));
					m_encodeWaitCondition.wakeOne();
					// �ͷ���Դ
					av_frame_unref(pCopyFrame);
					av_frame_free(&pCopyFrame);
					av_packet_unref(pEncodePacket);
					frame_index++;
					m_encodeMutex.unlock();
				}
			}
			// �ͷ�ʣ����Դ
			av_frame_free(&pMuteFrame);
			av_packet_free(&pEncodePacket);
			avcodec_close(pOutputAudioCodecCtx);
			avcodec_free_context(&pOutputAudioCodecCtx);
		}
	}
}

// CPushThread
CPushThread::CPushThread(AVFormatContext* pOutputFormatCtx, CircularQueue<AVPacket*>& videoPacketQueue, CircularQueue<AVPacket*>& audioPacketQueue,
                         QWaitCondition& videoWaitCondition, QMutex& videoMutex, QWaitCondition& audioWaitCondition, QMutex& audioMutex,
                         const bool bPushVideo, const bool bPushAudio, QObject* parent)
: QThread(parent), m_videoPacketQueue(videoPacketQueue), m_audioPacketQueue(audioPacketQueue), m_bPushVideo(bPushVideo),
  m_bPushAudio(bPushAudio), m_videoWaitCondition(videoWaitCondition), m_videoMutex(videoMutex), m_audioWaitCondition(audioWaitCondition),
  m_audioMutex(audioMutex), m_pOutputFormatCtx(pOutputFormatCtx) {
}

CPushThread::~CPushThread() {
	stop();
}

void CPushThread::pause() {
	QMutexLocker videoLocker(&m_videoMutex);
	QMutexLocker audioLocker(&m_audioMutex);
	m_bPause = true;
}

void CPushThread::resume() {
	QMutexLocker videoLocker(&m_videoMutex);
	QMutexLocker audioLocker(&m_audioMutex);
	m_bPause = false;
}

void CPushThread::stop() {
	m_bRunning = false;
	m_videoWaitCondition.wakeAll();
	m_audioWaitCondition.wakeAll();
	this->wait();
}

void CPushThread::run() {
	char errBuf[ERRBUFSIZE]{};
	auto logWaring = [&errBuf](const int nRet, AVPacket* packet, const char* log) {
		av_strerror(nRet, errBuf, ERRBUFSIZE);
		av_log(nullptr, AV_LOG_WARNING, log, errBuf);
		av_packet_unref(packet);
		av_packet_free(&packet);
	};
	while (m_bRunning) {
		if (m_bPause) {
			continue;
		}
		int nVideoPts{ 0 };
		if (m_bPushVideo) {
			QMutexLocker locker(&m_videoMutex);
			if (m_videoPacketQueue.isEmpty()) {
				m_videoWaitCondition.wakeOne();
				m_videoWaitCondition.wait(&m_videoMutex);
			}
			auto pVideoPacket = m_videoPacketQueue.pop();
			if (pVideoPacket && pVideoPacket->pts >= 0) {
				nVideoPts = static_cast<int>(pVideoPacket->pts);
				if (const int nRet = av_interleaved_write_frame(m_pOutputFormatCtx, pVideoPacket); nRet < 0) {
					logWaring(nRet, pVideoPacket, "video push error, %s\n");
					continue;
				}
				av_packet_unref(pVideoPacket);
				av_packet_free(&pVideoPacket);
			}
		}
		if (m_bPushAudio) {
			QMutexLocker locker(&m_audioMutex);
			if (m_audioPacketQueue.isEmpty()) {
				m_audioWaitCondition.wakeOne();
				m_audioWaitCondition.wait(&m_audioMutex);
			}
			auto pAudioPacket = m_audioPacketQueue.pop();
			while (pAudioPacket && pAudioPacket->buf && pAudioPacket->pts >= 0 && (!m_bPushVideo || pAudioPacket->pts <= nVideoPts)) {
				if (const int nRet = av_interleaved_write_frame(m_pOutputFormatCtx, pAudioPacket); nRet < 0) {
					logWaring(nRet, pAudioPacket, "audio push error, %s\n");
					if (!m_audioPacketQueue.isEmpty()) {
						if (m_audioPacketQueue.first()->pts < nVideoPts) {
							pAudioPacket = m_audioPacketQueue.pop();
						}
					}
					continue;
				}
				av_packet_unref(pAudioPacket);
				av_packet_free(&pAudioPacket);
				if (!m_audioPacketQueue.isEmpty()) {
					if (!m_bPushVideo || m_audioPacketQueue.first()->pts, nVideoPts) {
						pAudioPacket = m_audioPacketQueue.pop();
					}
				}
			}
		}
	}
}

// CRecvThread
CRecvThread::CRecvThread(QString strPath, QObject* parent)
: QThread(parent), m_strPath(std::move(strPath)) {
	setAutoDelete(false);
	m_bRunning = true;
}

CRecvThread::~CRecvThread() = default;

void CRecvThread::stop() {
	m_bRunning = false;
	this->wait();
}

QString CRecvThread::path() const {
	return m_strPath;
}

QPair<AVCodecParameters*, AVCodecParameters*> CRecvThread::getContext() const {
	return qMakePair(m_pVideoCodecPara, m_pAudioCodecPara);
}

void CRecvThread::run() {
	int nRet{ -1 };
	char errBuf[ERRBUFSIZE];
	// ������Ƶ������Ƶ��
	// ����ý���ļ��ĸ�ʽ�����Ϣ
	nRet = avformat_open_input(&m_pFormatCtx, m_strPath.toStdString().c_str(), nullptr, nullptr);
	if (nRet < 0) {
		av_strerror(nRet, errBuf, ERRBUFSIZE);
		av_log(nullptr, AV_LOG_ERROR, "Can't open input, %s\n", errBuf);
		return;
	}
	// ����ý���ļ��е�����Ϣ
	nRet = avformat_find_stream_info(m_pFormatCtx, nullptr);
	if (nRet < 0) {
		av_strerror(nRet, errBuf, ERRBUFSIZE);
		av_log(nullptr, AV_LOG_ERROR, "Can't find stream info, %s\n", errBuf);
		avformat_close_input(&m_pFormatCtx);
		return;
	}
	// ��ӡ��ʽ��Ϣ
	av_dump_format(m_pFormatCtx, 0, m_strPath.toStdString().c_str(), 0);
	// ������Ƶ����Ƶ������
	const int nVideoIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	const int nAudioIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
	if (nVideoIndex == -1 && nAudioIndex == -1) {
		av_log(nullptr, AV_LOG_ERROR, "Can't find video and audio stream.\n");
		avformat_close_input(&m_pFormatCtx);
		return;
	}
	// ������Ƶ����Ƶ��������
	if (AVERROR_STREAM_NOT_FOUND != nVideoIndex) {
		m_pVideoCodecPara = avcodec_parameters_alloc();
		avcodec_parameters_copy(m_pVideoCodecPara, m_pFormatCtx->streams[nVideoIndex]->codecpar);
	}
	if (AVERROR_STREAM_NOT_FOUND != nAudioIndex) {
		m_pAudioCodecPara = avcodec_parameters_alloc();
		avcodec_parameters_copy(m_pAudioCodecPara, m_pFormatCtx->streams[nAudioIndex]->codecpar);
	}
	avformat_close_input(&m_pFormatCtx);
}
