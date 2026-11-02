#include <jni.h>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <android/log.h>

#define TAG "NativeDiagnostics"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_example_NativeDiagnostics_isNativeLoaded(JNIEnv *env, jobject /* this */) {
    return JNI_TRUE;
}

JNIEXPORT jstring JNICALL
Java_com_example_NativeDiagnostics_getNativeArch(JNIEnv *env, jobject /* this */) {
    std::string arch = "Unknown";
#if defined(__arm__)
    arch = "ARMv7-A (32-bit armeabi-v7a)";
#if defined(__ARM_NEON)
    arch += " [NEON supported]";
#endif
#if defined(__VFP_FP__)
    arch += " [VFPv3/v4]";
#endif
#elif defined(__aarch64__)
    arch = "AArch64 (64-bit arm64-v8a)";
#elif defined(__i386__)
    arch = "x86 (32-bit)";
#elif defined(__x86_64__)
    arch = "x86_64 (64-bit)";
#endif
    return env->NewStringUTF(arch.c_str());
}

JNIEXPORT jint JNICALL
Java_com_example_NativeDiagnostics_getPointerSize(JNIEnv *env, jobject /* this */) {
    return static_cast<jint>(sizeof(void*) * 8);
}

JNIEXPORT jstring JNICALL
Java_com_example_NativeDiagnostics_getKernelRelease(JNIEnv *env, jobject /* this */) {
    struct utsname u;
    if (uname(&u) == 0) {
        std::stringstream ss;
        ss << u.sysname << " " << u.release << " (" << u.machine << ")";
        return env->NewStringUTF(ss.str().c_str());
    }
    return env->NewStringUTF("Unavailable");
}

JNIEXPORT jlong JNICALL
Java_com_example_NativeDiagnostics_getSysconfPagesize(JNIEnv *env, jobject /* this */) {
    long pageSize = sysconf(_SC_PAGESIZE);
    return static_cast<jlong>(pageSize);
}

JNIEXPORT jint JNICALL
Java_com_example_NativeDiagnostics_getOnlineCpuCores(JNIEnv *env, jobject /* this */) {
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    return static_cast<jint>(cores);
}

JNIEXPORT jboolean JNICALL
Java_com_example_NativeDiagnostics_isKvmAvailable(JNIEnv *env, jobject /* this */) {
    // Check if /dev/kvm device node is accessible for read/write
    int fd = open("/dev/kvm", O_RDWR);
    if (fd >= 0) {
        close(fd);
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_example_NativeDiagnostics_getMemorySysinfo(JNIEnv *env, jobject /* this */) {
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        std::stringstream ss;
        unsigned long long totalRamMb = (si.totalram * si.mem_unit) / (1024 * 1024);
        unsigned long long freeRamMb = (si.freeram * si.mem_unit) / (1024 * 1024);
        unsigned long long bufferRamMb = (si.bufferram * si.mem_unit) / (1024 * 1024);
        ss << "Total: " << totalRamMb << " MB | Libre: " << freeRamMb 
           << " MB | Buffers: " << bufferRamMb << " MB | Procs: " << si.procs;
        return env->NewStringUTF(ss.str().c_str());
    }
    return env->NewStringUTF("sysinfo non disponible");
}

JNIEXPORT jstring JNICALL
Java_com_example_NativeDiagnostics_getCompilerInfo(JNIEnv *env, jobject /* this */) {
    std::stringstream ss;
#if defined(__clang__)
    ss << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elif defined(__GNUC__)
    ss << "GCC " << __GNUC__ << "." << __GNUC_MINOR__;
#else
    ss << "Inconnu";
#endif

#if defined(__ANDROID_API__)
    ss << " (Android NDK Target API: " << __ANDROID_API__ << ")";
#endif
    return env->NewStringUTF(ss.str().c_str());
}

} // extern "C"
