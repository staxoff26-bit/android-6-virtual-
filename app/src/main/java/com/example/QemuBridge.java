package com.example;

public class QemuBridge {

    private static boolean isLoaded = false;
    private static String loadError = null;

    static {
        try {
            System.loadLibrary("qemubridge");
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

    // Native JNI functions
    public native boolean isNativeLoaded();
    public native boolean isAuthenticQemuLoaded();
    public native String getAuthenticQemuDetails();
    public native int initializeEngine();
    public native String getEngineVersion();
    public native String getAvailableMachines();
    public native int initializeVirtualMachine(String machineName, int ramMb);
    public native int startVirtualMachine();
    public native int pauseVirtualMachine();
    public native int resumeVirtualMachine();
    public native int stopVirtualMachine();
    public native int getEngineState();
    public native String getEngineStateString();
    public native String getEngineLogs();
    public native String getDetectedDevices();
}
