/*
 * Copyright 2019 Google LLC
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

#ifndef rotation_data_h
#define rotation_data_h

#include <deque>

#include "util/vector.h"

namespace cardboard {

// Fixed window FIFO mean filter for vectors of the given dimension.
class RotationData {
 public:
  // Create a buffer to hold rotation data of size buffer_size.
  // @param buffer_size size of samples to buffer.
  explicit RotationData(size_t buffer_size);

  // Add sample to buffer_ if buffer_ is full it drop the oldest sample.
  void AddSample(const Vector4& sample, const int64_t timestamp_ns);

  // Returns true if buffer has buffer_size sample, false otherwise.
  bool IsValid() const;

  // Returns the latest value stored in the internal buffer.
  Vector4 GetLatestData() const;

  Vector4 GetInterpolatedForTimeStamp(const int64_t timestamp_ns) const;
    
 private:
  const size_t buffer_size_;
  std::deque<Vector4> buffer_;
  std::deque<int64_t> timestamp_buffer_;
};

}  // namespace cardboard


#endif /* rotation_data_h */
