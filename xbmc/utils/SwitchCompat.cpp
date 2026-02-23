#ifdef TARGET_SWITCH

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <iconv.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
#include <libswresample/swresample.h>
#include "lib/cpluff/libcpluff/cpluff.h"
}

extern "C" {

int fchown(int, unsigned int, unsigned int)
{
  errno = ENOSYS;
  return -1;
}

unsigned int geteuid(void)
{
  return 0;
}

int av_get_channel_layout_nb_channels(uint64_t channel_layout)
{
  int count = 0;
  while (channel_layout)
  {
    count += static_cast<int>(channel_layout & 1U);
    channel_layout >>= 1U;
  }
  return count;
}

int64_t av_get_default_channel_layout(int nb_channels)
{
  switch (nb_channels)
  {
    case 1: return AV_CH_LAYOUT_MONO;
    case 2: return AV_CH_LAYOUT_STEREO;
    case 3: return AV_CH_LAYOUT_SURROUND;
    case 4: return AV_CH_LAYOUT_QUAD;
    case 5: return AV_CH_LAYOUT_5POINT0;
    case 6: return AV_CH_LAYOUT_5POINT1;
    case 7: return AV_CH_LAYOUT_6POINT1;
    case 8: return AV_CH_LAYOUT_7POINT1;
    default: return 0;
  }
}

int av_get_channel_layout_channel_index(uint64_t channel_layout, uint64_t channel)
{
  if ((channel_layout & channel) == 0)
    return -1;

  int index = 0;
  for (int bit = 0; bit < 64; ++bit)
  {
    const uint64_t mask = (1ULL << bit);
    if ((channel_layout & mask) == 0)
      continue;
    if (mask == channel)
      return index;
    ++index;
  }
  return -1;
}

uint64_t av_channel_layout_extract_channel(uint64_t channel_layout, int index)
{
  if (index < 0)
    return 0;

  for (int bit = 0; bit < 64; ++bit)
  {
    const uint64_t mask = (1ULL << bit);
    if ((channel_layout & mask) == 0)
      continue;
    if (index == 0)
      return mask;
    --index;
  }
  return 0;
}

SwrContext* swr_alloc_set_opts(SwrContext* s,
                               int64_t out_ch_layout,
                               enum AVSampleFormat out_sample_fmt,
                               int out_sample_rate,
                               int64_t in_ch_layout,
                               enum AVSampleFormat in_sample_fmt,
                               int in_sample_rate,
                               int log_offset,
                               void* log_ctx)
{
  (void)log_offset;
  (void)log_ctx;
  SwrContext* ctx = s ? s : swr_alloc();
  if (!ctx)
    return nullptr;

  if (av_opt_set_int(ctx, "out_channel_layout", out_ch_layout, 0) < 0 ||
      av_opt_set_int(ctx, "out_sample_fmt", out_sample_fmt, 0) < 0 ||
      av_opt_set_int(ctx, "out_sample_rate", out_sample_rate, 0) < 0 ||
      av_opt_set_int(ctx, "in_channel_layout", in_ch_layout, 0) < 0 ||
      av_opt_set_int(ctx, "in_sample_fmt", in_sample_fmt, 0) < 0 ||
      av_opt_set_int(ctx, "in_sample_rate", in_sample_rate, 0) < 0)
  {
    if (!s)
      swr_free(&ctx);
    return nullptr;
  }

  if (swr_init(ctx) < 0)
  {
    if (!s)
      swr_free(&ctx);
    return nullptr;
  }

  return ctx;
}

int avcodec_encode_audio2(AVCodecContext* avctx, AVPacket* avpkt, const AVFrame* frame, int* got_packet_ptr)
{
  if (!got_packet_ptr)
    return AVERROR(EINVAL);

  *got_packet_ptr = 0;

  int ret = avcodec_send_frame(avctx, const_cast<AVFrame*>(frame));
  if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
    return ret;

  ret = avcodec_receive_packet(avctx, avpkt);
  if (ret == 0)
  {
    *got_packet_ptr = 1;
    return 0;
  }
  if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    return 0;

  return ret;
}

void av_get_channel_layout_string(char* buf, int buf_size, int nb_channels, uint64_t channel_layout)
{
  if (!buf || buf_size <= 0)
    return;
  std::snprintf(buf, static_cast<std::size_t>(buf_size), "0x%llx/%d",
                static_cast<unsigned long long>(channel_layout), nb_channels);
}

int8_t* av_frame_get_qp_table(AVFrame*, int* stride, int* type)
{
  if (stride)
    *stride = 0;
  if (type)
    *type = 0;
  return nullptr;
}

iconv_t iconv_open(const char*, const char*)
{
  return reinterpret_cast<iconv_t>(1);
}

std::size_t iconv(iconv_t cd, char** inbuf, std::size_t* inbytesleft, char** outbuf, std::size_t* outbytesleft)
{
  if (cd == reinterpret_cast<iconv_t>(-1))
  {
    errno = EINVAL;
    return static_cast<std::size_t>(-1);
  }

  // POSIX allows state-reset calls with null buffers.
  if (!inbuf || !inbytesleft || !outbuf || !outbytesleft)
    return 0;

  const std::size_t n = (*inbytesleft < *outbytesleft) ? *inbytesleft : *outbytesleft;
  if (n > 0)
    std::memcpy(*outbuf, *inbuf, n);
  *inbuf += n;
  *outbuf += n;
  *inbytesleft -= n;
  *outbytesleft -= n;
  return 0;
}

int iconv_close(iconv_t)
{
  return 0;
}

void init_emu_environ()
{
}

void update_emu_environ()
{
}

void cleanup_emu_environ()
{
}

} // extern "C"

#endif // TARGET_SWITCH
