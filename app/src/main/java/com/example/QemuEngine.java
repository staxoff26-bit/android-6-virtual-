package com.example;

import android.os.Handler;
import android.os.Looper;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class QemuEngine {

    public interface StateListener {
        void onStateChanged(int stateCode, String stateDescription);
    }

    public interface LogListener {
        void onLogsUpdated(String logs);
    }

    private static QemuEngine instance;
    private final QemuBridge bridge;
    private final ExecutorService executor;
    private final Handler mainHandler;
    private StateListener stateListener;
    private LogListener logListener;
    private boolean isInitialized = false;

    private QemuEngine() {
        this.bridge = new QemuBridge();
        this.executor = Executors.newSingleThreadExecutor();
        this.mainHandler = new Handler(Looper.getMainLooper());
    }

    public static synchronized QemuEngine getInstance() {
        if (instance == null) {
            instance = new QemuEngine();
        }
        return instance;
    }

    public boolean isNativeLoaded() {
        return QemuBridge.isLibraryLoaded();
    }

    public String getNativeLoadError() {
        return QemuBridge.getLoadError();
    }

    public boolean isAuthenticQemuLoaded() {
        if (!isNativeLoaded()) return false;
        try {
            return bridge.isAuthenticQemuLoaded();
        } catch (Throwable t) {
            return false;
        }
    }

    public String getAuthenticQemuDetails() {
        if (!isNativeLoaded()) return "Non chargé";
        try {
            return bridge.getAuthenticQemuDetails();
        } catch (Throwable t) {
            return "Erreur: " + t.getMessage();
        }
    }

    public void setStateListener(StateListener listener) {
        this.stateListener = listener;
    }

    public void setLogListener(LogListener listener) {
        this.logListener = listener;
    }

    public String getVersion() {
        if (!isNativeLoaded()) return "Non chargé (" + getNativeLoadError() + ")";
        try {
            return bridge.getEngineVersion();
        } catch (Throwable t) {
            return "Erreur: " + t.getMessage();
        }
    }

    public String getAvailableMachines() {
        if (!isNativeLoaded()) return "Bibliothèque native non disponible";
        try {
            return bridge.getAvailableMachines();
        } catch (Throwable t) {
            return "Erreur: " + t.getMessage();
        }
    }

    public String getDetectedDevices() {
        if (!isNativeLoaded()) return "Bibliothèque native non disponible";
        try {
            return bridge.getDetectedDevices();
        } catch (Throwable t) {
            return "Erreur: " + t.getMessage();
        }
    }

    public String getCurrentStateString() {
        if (!isNativeLoaded()) return "NATIVE_UNAVAILABLE";
        try {
            return bridge.getEngineStateString();
        } catch (Throwable t) {
            return "ERROR";
        }
    }

    public int getCurrentStateCode() {
        if (!isNativeLoaded()) return -1;
        try {
            return bridge.getEngineState();
        } catch (Throwable t) {
            return -1;
        }
    }

    public String getLogs() {
        if (!isNativeLoaded()) return "QEMU logs non disponibles : bibliothèque native non chargée.";
        try {
            return bridge.getEngineLogs();
        } catch (Throwable t) {
            return "Erreur lors de la lecture des logs: " + t.getMessage();
        }
    }

    public void initializeEngine(final Runnable onComplete) {
        executor.execute(new Runnable() {
            @Override
            public void run() {
                if (isNativeLoaded()) {
                    bridge.initializeEngine();
                    bridge.initializeVirtualMachine("ranchu", 512);
                    isInitialized = true;
                }
                notifyStateChanged();
                if (onComplete != null) {
                    mainHandler.post(onComplete);
                }
            }
        });
    }

    public void start(final Runnable onComplete) {
        executor.execute(new Runnable() {
            @Override
            public void run() {
                if (isNativeLoaded()) {
                    if (!isInitialized) {
                        bridge.initializeEngine();
                        bridge.initializeVirtualMachine("ranchu", 512);
                        isInitialized = true;
                    }
                    bridge.startVirtualMachine();
                }
                notifyStateChanged();
                if (onComplete != null) {
                    mainHandler.post(onComplete);
                }
            }
        });
    }

    public void pause(final Runnable onComplete) {
        executor.execute(new Runnable() {
            @Override
            public void run() {
                if (isNativeLoaded()) {
                    bridge.pauseVirtualMachine();
                }
                notifyStateChanged();
                if (onComplete != null) {
                    mainHandler.post(onComplete);
                }
            }
        });
    }

    public void resume(final Runnable onComplete) {
        executor.execute(new Runnable() {
            @Override
            public void run() {
                if (isNativeLoaded()) {
                    bridge.resumeVirtualMachine();
                }
                notifyStateChanged();
                if (onComplete != null) {
                    mainHandler.post(onComplete);
                }
            }
        });
    }

    public void stop(final Runnable onComplete) {
        executor.execute(new Runnable() {
            @Override
            public void run() {
                if (isNativeLoaded()) {
                    bridge.stopVirtualMachine();
                }
                notifyStateChanged();
                if (onComplete != null) {
                    mainHandler.post(onComplete);
                }
            }
        });
    }

    private void notifyStateChanged() {
        final int code = getCurrentStateCode();
        final String desc = getCurrentStateString();
        final String logs = getLogs();
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                if (stateListener != null) {
                    stateListener.onStateChanged(code, desc);
                }
                if (logListener != null) {
                    logListener.onLogsUpdated(logs);
                }
            }
        });
    }
}
