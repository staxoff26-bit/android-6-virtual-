package com.example;

public class NativeDiagnostics {

    private static boolean isLoaded = false;
    private static String loadError = null;

    static {
        try {
            System.loadLibrary("nativediagnostics");
            isLoaded = true;
        } catch (Throwable t) {
            isLoaded = false;
            loadError = t.getClass().getSimpleName() + ": " + t.getMessage();
        }
    }

    public static boolean isLibraryLoaded() {
        return isLoaded;
    }

    public static String getLoadError() {
        return loadError;
    }

    // Native JNI methods
    public native boolean isNativeLoaded();
    public native String getNativeArch();
    public native int getPointerSize();
    public native String getKernelRelease();
    public native long getSysconfPagesize();
    public native int getOnlineCpuCores();
    public native boolean isKvmAvailable();
    public native String getMemorySysinfo();
    public native String getCompilerInfo();
}
