# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libffmpeg
LOCAL_SRC_FILES := libffmpeg.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libav
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libgl2jni
LOCAL_CFLAGS    := -Werror
LOCAL_CXXFLAGS  := -D__STDC_CONSTANT_MACROS
LOCAL_SRC_FILES := testFFmpeg.cpp
LOCAL_LDLIBS    := -llog -lGLESv2
LOCAL_SHARED_LIBRARIES := libffmpeg
include $(BUILD_SHARED_LIBRARY)
