/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef COMMON_VIDEO_INCLUDE_VIDEO_FRAME_H_
#define COMMON_VIDEO_INCLUDE_VIDEO_FRAME_H_

// TODO(nisse): This header file should eventually be deleted. The
// EncodedImage class stays in this file until we have figured out how
// to refactor and clean up related interfaces, at which point it
// should be moved to somewhere under api/.

#include "absl/types/optional.h"
#include "api/video/video_content_type.h"
#include "api/video/video_rotation.h"
#include "api/video/video_timing.h"
#include "common_types.h"  // NOLINT(build/include)

namespace webrtc {

// TODO(pbos): Rename EncodedFrame and reformat this class' members.
class EncodedImage {
 public:
  static const size_t kBufferPaddingBytesH264;

  // Some decoders require encoded image buffers to be padded with a small
  // number of additional bytes (due to over-reading byte readers).
  static size_t GetBufferPaddingBytes(VideoCodecType codec_type);

  EncodedImage();
  EncodedImage(const EncodedImage&);
  EncodedImage(uint8_t* buffer, size_t length, size_t size);

  // TODO(nisse): Change style to timestamp(), set_timestamp(), for consistency
  // with the VideoFrame class.
  // Set frame timestamp (90kHz).
  void SetTimestamp(uint32_t timestamp) { _timeStamp = timestamp; }

  // Get frame timestamp (90kHz).
  uint32_t Timestamp() const { return _timeStamp; }

  void SetEncodeTime(int64_t encode_start_ms, int64_t encode_finish_ms);

  absl::optional<int> SpatialIndex() const {
    if (spatial_index_ < 0)
      return absl::nullopt;
    return spatial_index_;
  }
  void SetSpatialIndex(absl::optional<int> spatial_index) {
    RTC_DCHECK_GE(spatial_index.value_or(0), 0);
    RTC_DCHECK_LT(spatial_index.value_or(0), kMaxSpatialLayers);
    spatial_index_ = spatial_index.value_or(-1);
  }

  uint32_t _encodedWidth = 0;
  uint32_t _encodedHeight = 0;
  // TODO(nisse): Make private, once users have been updated
  // to use accessor methods.
  uint32_t _timeStamp = 0;
  // NTP time of the capture time in local timebase in milliseconds.
  int64_t ntp_time_ms_ = 0;
  int64_t capture_time_ms_ = 0;
  FrameType _frameType = kVideoFrameDelta;
  uint8_t* _buffer;
  size_t _length;
  size_t _size;
  VideoRotation rotation_ = kVideoRotation_0;
  VideoContentType content_type_ = VideoContentType::UNSPECIFIED;
  bool _completeFrame = false;
  int qp_ = -1;  // Quantizer value.

  // When an application indicates non-zero values here, it is taken as an
  // indication that all future frames will be constrained with those limits
  // until the application indicates a change again.
  PlayoutDelay playout_delay_ = {-1, -1};

  struct Timing {
    uint8_t flags = VideoSendTiming::kInvalid;
    int64_t encode_start_ms = 0;
    int64_t encode_finish_ms = 0;
    int64_t packetization_finish_ms = 0;
    int64_t pacer_exit_ms = 0;
    int64_t network_timestamp_ms = 0;
    int64_t network2_timestamp_ms = 0;
    int64_t receive_start_ms = 0;
    int64_t receive_finish_ms = 0;
  } timing_;

 private:
  // -1 means not set. Use a plain int rather than optional, to keep this class
  // copyable with memcpy.
  int spatial_index_ = -1;
};

}  // namespace webrtc

#endif  // COMMON_VIDEO_INCLUDE_VIDEO_FRAME_H_
