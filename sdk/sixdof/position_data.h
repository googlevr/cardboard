/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Aryzon 6DoF

#ifndef position_data_h
#define position_data_h

#include <deque>

#include "util/vector.h"

namespace cardboard {

// This class holds a buffer of position data samples with corresponding timestamp samples
class PositionData {
 public:
  // Create a buffer to hold position data of size buffer_size.
  // @param buffer_size size of samples to buffer.
  explicit PositionData(size_t buffer_size);

  // Add sample to buffer_ if buffer_ is full it drop the oldest sample.
  void AddSample(const Vector3& sample, const int64_t timestamp_ns);

  // Returns true if buffer has buffer_size sample, false otherwise.
  bool IsValid() const;

  // Returns the latest value stored in the internal buffer.
  Vector3 GetLatestData() const;
    
  // Returns the latest timestamp value stored in the internal timestampbuffer.
  long long GetLatestTimestamp() const;
    
  // Returns the position extrapolated from data stored in the internal buffers.
  // A buffer size of 2 is required to work.
  // It returns a zero Vector3 when not fully initialised.
  // @param timestamp_ns the time in nanoseconds to get a position value for.
  Vector3 GetExtrapolatedForTimeStamp(const int64_t timestamp_ns);
  
  // Clear the internal buffers.
  void Reset();
 private:
  const size_t buffer_size_;
  std::deque<Vector3> buffer_;
  std::deque<int64_t> timestamp_buffer_;
};

}  // namespace cardboard


#endif /* rotation_data_h */
