#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdarg.h>
#include "hev-main.h"
#include "hev-misc.h"

#define LOG_TAG "NatmapJNI"

// 全局上下文
static struct {
    JavaVM *jvm;
    jobject callback;  // 全局引用
    jmethodID onLogUpdate;
    jmethodID onResult;
    int initialized;
} callback_context = {0};

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
/**
 * 获取JNIEnv，返回值：
 *  0: 已附加
 *  1: 刚附加（需要Detach）
 * -1: 附加失败
 */
static int get_env(JNIEnv **env) {
    if ((*callback_context.jvm)->GetEnv(callback_context.jvm, (void **)env, JNI_VERSION_1_6) == JNI_OK) {
        return 0; // 已经附加
    }
    if ((*callback_context.jvm)->AttachCurrentThread(callback_context.jvm, env, NULL) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "AttachCurrentThread failed");
        *env = NULL;
        return -1;
    }
    return 1; // 新附加
}


static void jni_log_capture_impl(const char* format, va_list args) {
    char temp_buffer[512];
    vsnprintf(temp_buffer, sizeof(temp_buffer), format, args);

    if (callback_context.initialized && callback_context.callback && callback_context.onLogUpdate) {
        JNIEnv *env;
        int attach_state = get_env(&env);
        if (!env) return;

        if ((*env)->PushLocalFrame(env, 10) != 0) {
            if (attach_state == 1) {
                (*callback_context.jvm)->DetachCurrentThread(callback_context.jvm);
            }
            return;
        }

        LOGI("jni_log_capture_impl :%s", temp_buffer);

        (*env)->CallVoidMethod(env, callback_context.callback, callback_context.onLogUpdate, (*env)->NewStringUTF(env, temp_buffer));

        (*env)->PopLocalFrame(env, NULL);

        if (attach_state == 1) {
            (*callback_context.jvm)->DetachCurrentThread(callback_context.jvm);
        }
    }
}

static void jni_result_capture_impl(const char* format, va_list args) {
    char temp_buffer[512];
    vsnprintf(temp_buffer, sizeof(temp_buffer), format, args);

    if (callback_context.initialized && callback_context.callback && callback_context.onResult) {
        JNIEnv *env;
        int attach_state = get_env(&env);
        if (!env) return;

        if ((*env)->PushLocalFrame(env, 10) != 0) {
            if (attach_state == 1) {
                (*callback_context.jvm)->DetachCurrentThread(callback_context.jvm);
            }
            return;
        }
        LOGI("jni_result_capture_impl : %s", temp_buffer);

        (*env)->CallVoidMethod(env, callback_context.callback, callback_context.onResult, JNI_TRUE,(*env)->NewStringUTF(env, temp_buffer));

        (*env)->PopLocalFrame(env, NULL);

        if (attach_state == 1) {
            (*callback_context.jvm)->DetachCurrentThread(callback_context.jvm);
        }
    }
}

JNIEXPORT void JNICALL
Java_me_aside0_exposedadb_natmap_NatmapPlugin_nativeExecuteNatmap(JNIEnv *env, jobject thiz,
                                                      jobjectArray args,
                                                      jobject callback) {
    // 保存 JavaVM
    JavaVM *jvm;
    (*env)->GetJavaVM(env, &jvm);

    // 获取回调方法 ID
    jclass callbackClass = (*env)->GetObjectClass(env, callback);
    jmethodID onResult = (*env)->GetMethodID(env, callbackClass, "onResult", "(ZLjava/lang/String;)V");
    jmethodID onLogUpdate = (*env)->GetMethodID(env, callbackClass, "onLogUpdate", "(Ljava/lang/String;)V");

    if (!onResult || !onLogUpdate) {
        LOGE("Failed to get method IDs");
        return;
    }

    // 储存全局信息
    callback_context.jvm = jvm;
    callback_context.callback = callback;
    callback_context.onResult = onResult;
    callback_context.onLogUpdate = onLogUpdate;
    callback_context.initialized = 1;
    
    // 重置全局停止标志
    set_stop_execution(0);

    // 注册Native日志和结果回调
    set_jni_log_functions(jni_log_capture_impl, jni_result_capture_impl);

    // 转换参数
    int argc = (*env)->GetArrayLength(env, args);
    char **argv = (char**)malloc(argc * sizeof(char*));
    if (!argv) {
        LOGE("Failed to allocate argv array");
        return;
    }
    for (int i = 0; i < argc; i++) {
        jstring jstr = (jstring)(*env)->GetObjectArrayElement(env, args, i);
        const char *str = (*env)->GetStringUTFChars(env, jstr, NULL);
        argv[i] = strdup(str);
        (*env)->ReleaseStringUTFChars(env, jstr, str);
        (*env)->DeleteLocalRef(env, jstr);

        LOGI("argv[%d]: %s", i, argv[i]);
    }

    // 设置环境变量
    setenv("ANDROID_LOG_TAGS", "*:V", 1);

    // 执行 main
    LOGI("Starting natmap main function with %d arguments", argc);
    int result = main(argc, argv);
    LOGI("Main function completed with result: %d", result);

    // 发送最终结果（在调用线程，可以直接用局部引用）
    jboolean success = (result == 0) ? JNI_TRUE : JNI_FALSE;
    char result_msg[256];
    snprintf(result_msg, sizeof(result_msg), "natmap completed with exit code: %d", result);
    jstring output = (*env)->NewStringUTF(env, result_msg);
    (*env)->CallVoidMethod(env, callback, onResult, success, output);
    (*env)->DeleteLocalRef(env, output);

    callback_context.callback = NULL;
    callback_context.initialized = 0;

    for (int i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
}

JNIEXPORT void JNICALL
Java_me_aside0_exposedadb_natmap_NatmapPlugin_nativeStopExecution(JNIEnv *env, jobject thiz) {
    LOGI("Native stop execution called");
    set_stop_execution(1);
}