# Copyright (C) 2022 The Android Open Source Project
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

TOP_PATH := $(call my-dir)

ifeq ($(filter $(modules-get-list),hev-task-system),)
    include $(TOP_PATH)/third-part/hev-task-system/Android.mk
endif

LOCAL_PATH = $(TOP_PATH)
SRCDIR := $(LOCAL_PATH)/src

# Common configuration
COMMON_C_INCLUDES := \
	$(LOCAL_PATH)/third-part/hev-task-system/include \
	$(LOCAL_PATH)/third-part/hev-task-system/src/lib/rbtree

COMMON_CFLAGS := $(VERSION_CFLAGS) -DANDROID
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
COMMON_CFLAGS += -mfpu=neon
endif

COMMON_STATIC_LIBRARIES := hev-task-system
COMMON_LDLIBS := -llog

# Build natmap JNI shared library (main library for Android app)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/build.mk
LOCAL_MODULE := natmap-jni
LOCAL_SRC_FILES := $(patsubst $(SRCDIR)/%,src/%,$(SRCFILES))
LOCAL_C_INCLUDES := $(COMMON_C_INCLUDES)
LOCAL_CFLAGS += $(COMMON_CFLAGS) -DJNI_BUILD
LOCAL_STATIC_LIBRARIES := $(COMMON_STATIC_LIBRARIES)
LOCAL_LDLIBS := $(COMMON_LDLIBS)
include $(BUILD_SHARED_LIBRARY)
