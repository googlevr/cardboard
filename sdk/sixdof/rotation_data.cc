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
#include "sixdof/rotation_data.h"

#include <string>
#include <sstream>

namespace cardboard {

RotationData::RotationData(size_t buffer_size) : buffer_size_(buffer_size) {}

void RotationData::AddSample(const Vector4& sample, const int64_t timestamp_ns) {
    buffer_.push_back(sample);
    if (buffer_.size() > buffer_size_) {
        buffer_.pop_front();
    }
    
    timestamp_buffer_.push_back(timestamp_ns);
    if (timestamp_buffer_.size() > buffer_size_) {
        timestamp_buffer_.pop_front();
    }
}

bool RotationData::IsValid() const { return buffer_.size() == buffer_size_; }

    Vector4 RotationData::GetLatestData() const {
        if (buffer_.size() > 0) {
            return buffer_[buffer_.size() -1];
        }
        return Vector4::Zero();
    }


Vector4 RotationData::GetInterpolatedForTimeStamp(const int64_t timestamp_ns) const{
  
    if (!IsValid()) {
        return {0.0,0.0,0.0,1.0};
    }
    int64_t smaller = -1;
    int64_t larger = -1;
    
    bool didPassLarger = false;
    int i=0;
    
    while (!didPassLarger && i < buffer_size_) {
        int64_t currentTs = timestamp_buffer_[i];
        if (currentTs <= timestamp_ns) {
            smaller = currentTs;
        } else {
            larger = currentTs;
            didPassLarger = true;
        }
        i++;
    }

    float interpolationValue = 0.5f;
    Vector4 q;
    Vector4 q0;
    Vector4 q1;
    
    if (smaller > 0 && larger > 0) {
        int64_t delta = larger - smaller;
        interpolationValue = (timestamp_ns - smaller) / (double)(delta);
        q0 = buffer_[i-1];
        q1 = buffer_[i];
        q = interpolationValue*(q1-q0) + q0;
    } else {
        q = buffer_[buffer_size_];
    }
    
    return q;
}

}  // namespace cardboard
