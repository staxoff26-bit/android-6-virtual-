package com.example;

import android.app.ActivityManager;
import android.content.Context;
import android.os.Build;
import android.os.Environment;
import android.os.StatFs;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class DeviceAudit {

    public static class AuditReport {
        // System & Hardware
        public String manufacturer;
        public String model;
        public String device;
        public String product;
        public String hardware;
        public String board;
        public String androidRelease;
        public int sdkInt;
        public String buildFingerprint;

        // ABIs
        public String primaryAbi;
        public String secondaryAbi;
        public List<String> supportedAbis = new ArrayList<>();

        // CPU
        public int cpuCores;
        public String cpuModel;
        public String cpuHardware;
        public String cpuFeatures;
        public boolean hasNeon;
        public boolean hasVfp;

        // Memory
        public long totalRamBytes;
        public long availRamBytes;
        public long thresholdBytes;
        public boolean isLowMemory;
        public long procMemTotalKb;
        public long procMemFreeKb;
        public long procMemCachedKb;

        // Storage
        public long internalTotalBytes;
        public long internalAvailBytes;
        public String externalStorageState;
        public long externalTotalBytes;
        public long externalAvailBytes;

        // Native JNI probe
        public boolean nativeLoaded;
        public String nativeError;
        public String nativeArch;
        public int pointerSizeBits;
        public String kernelRelease;
        public long pageSizeBytes;
        public int nativeOnlineCores;
        public boolean kvmAvailable;
        public String nativeMemorySysinfo;
        public String nativeCompilerInfo;

        // Feasibility Decision
        public String feasibilityStatus; // GO, CONDITIONAL GO, NO GO
        public List<String> feasibilityNotes = new ArrayList<>();
    }

    public static AuditReport runAudit(Context context) {
        AuditReport report = new AuditReport();

        // 1. Device identity
        report.manufacturer = Build.MANUFACTURER;
        report.model = Build.MODEL;
        report.device = Build.DEVICE;
        report.product = Build.PRODUCT;
        report.hardware = Build.HARDWARE;
        report.board = Build.BOARD;
        report.androidRelease = Build.VERSION.RELEASE;
        report.sdkInt = Build.VERSION.SDK_INT;
        report.buildFingerprint = Build.FINGERPRINT;

        // 2. ABIs
        report.primaryAbi = Build.CPU_ABI;
        report.secondaryAbi = Build.CPU_ABI2;
        if (Build.CPU_ABI != null && !Build.CPU_ABI.isEmpty()) {
            report.supportedAbis.add(Build.CPU_ABI);
        }
        if (Build.CPU_ABI2 != null && !Build.CPU_ABI2.isEmpty()) {
            report.supportedAbis.add(Build.CPU_ABI2);
        }
        if (Build.VERSION.SDK_INT >= 21) {
            try {
                String[] abis = (String[]) Build.class.getField("SUPPORTED_ABIS").get(null);
                if (abis != null) {
                    report.supportedAbis.clear();
                    for (String a : abis) {
                        report.supportedAbis.add(a);
                    }
                }
            } catch (Throwable ignored) {
            }
        }

        // 3. CPU info from /proc/cpuinfo
        parseCpuInfo(report);

        // 4. Memory info
        ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        if (am != null) {
            ActivityManager.MemoryInfo mi = new ActivityManager.MemoryInfo();
            am.getMemoryInfo(mi);
            report.totalRamBytes = mi.totalMem;
            report.availRamBytes = mi.availMem;
            report.thresholdBytes = mi.threshold;
            report.isLowMemory = mi.lowMemory;
        }
        parseMemInfo(report);

        // 5. Storage info
        try {
            File dataDir = Environment.getDataDirectory();
            StatFs dataStat = new StatFs(dataDir.getPath());
            long blockSize = dataStat.getBlockSizeLong();
            long blockCount = dataStat.getBlockCountLong();
            long availBlocks = dataStat.getAvailableBlocksLong();
            report.internalTotalBytes = blockCount * blockSize;
            report.internalAvailBytes = availBlocks * blockSize;
        } catch (Throwable t) {
            report.internalTotalBytes = -1;
            report.internalAvailBytes = -1;
        }

        try {
            report.externalStorageState = Environment.getExternalStorageState();
            if (Environment.MEDIA_MOUNTED.equals(report.externalStorageState)) {
                File extDir = Environment.getExternalStorageDirectory();
                StatFs extStat = new StatFs(extDir.getPath());
                long extBlockSize = extStat.getBlockSizeLong();
                report.externalTotalBytes = extStat.getBlockCountLong() * extBlockSize;
                report.externalAvailBytes = extStat.getAvailableBlocksLong() * extBlockSize;
            }
        } catch (Throwable t) {
            report.externalTotalBytes = -1;
            report.externalAvailBytes = -1;
        }

        // 6. Native Diagnostics via JNI
        report.nativeLoaded = NativeDiagnostics.isLibraryLoaded();
        report.nativeError = NativeDiagnostics.getLoadError();
        if (report.nativeLoaded) {
            try {
                NativeDiagnostics nd = new NativeDiagnostics();
                report.nativeArch = nd.getNativeArch();
                report.pointerSizeBits = nd.getPointerSize();
                report.kernelRelease = nd.getKernelRelease();
                report.pageSizeBytes = nd.getSysconfPagesize();
                report.nativeOnlineCores = nd.getOnlineCpuCores();
                report.kvmAvailable = nd.isKvmAvailable();
                report.nativeMemorySysinfo = nd.getMemorySysinfo();
                report.nativeCompilerInfo = nd.getCompilerInfo();
            } catch (Throwable t) {
                report.nativeError = "Execution error: " + t.getMessage();
            }
        }

        // 7. Feasibility Evaluation
        evaluateFeasibility(report);

        return report;
    }

    private static void parseCpuInfo(AuditReport report) {
        int coreCount = 0;
        String model = "Unknown";
        String hardware = "Unknown";
        String features = "";

        File cpuinfo = new File("/proc/cpuinfo");
        if (cpuinfo.exists()) {
            try (BufferedReader br = new BufferedReader(new FileReader(cpuinfo))) {
                String line;
                while ((line = br.readLine()) != null) {
                    line = line.trim();
                    if (line.startsWith("processor")) {
                        coreCount++;
                    } else if (line.startsWith("model name") || line.startsWith("Processor")) {
                        String[] parts = line.split(":", 2);
                        if (parts.length > 1) model = parts[1].trim();
                    } else if (line.startsWith("Hardware")) {
                        String[] parts = line.split(":", 2);
                        if (parts.length > 1) hardware = parts[1].trim();
                    } else if (line.startsWith("Features")) {
                        String[] parts = line.split(":", 2);
                        if (parts.length > 1) features = parts[1].trim();
                    }
                }
            } catch (Throwable ignored) {
            }
        }

        if (coreCount == 0) {
            coreCount = Runtime.getRuntime().availableProcessors();
        }

        report.cpuCores = coreCount;
        report.cpuModel = model;
        report.cpuHardware = hardware;
        report.cpuFeatures = features;
        report.hasNeon = features.toLowerCase(Locale.US).contains("neon");
        report.hasVfp = features.toLowerCase(Locale.US).contains("vfp");
    }

    private static void parseMemInfo(AuditReport report) {
        File meminfo = new File("/proc/meminfo");
        if (meminfo.exists()) {
            try (BufferedReader br = new BufferedReader(new FileReader(meminfo))) {
                String line;
                while ((line = br.readLine()) != null) {
                    if (line.startsWith("MemTotal:")) {
                        report.procMemTotalKb = extractKb(line);
                    } else if (line.startsWith("MemFree:")) {
                        report.procMemFreeKb = extractKb(line);
                    } else if (line.startsWith("Cached:")) {
                        report.procMemCachedKb = extractKb(line);
                    }
                }
            } catch (Throwable ignored) {
            }
        }
    }

    private static long extractKb(String line) {
        try {
            String[] parts = line.split("\\s+");
            if (parts.length >= 2) {
                return Long.parseLong(parts[1]);
            }
        } catch (Throwable ignored) {
        }
        return 0;
    }

    private static void evaluateFeasibility(AuditReport report) {
        // We assess the feasibility of running an Android 6.0 Marshmallow ARMv7 guest
        // on Honor 4X CHE2-TL00M (Android 4.4.3 KitKat, Kirin 620).
        List<String> notes = report.feasibilityNotes;
        boolean canProceed = true;

        // 1. ABI Check
        boolean isArm = false;
        for (String abi : report.supportedAbis) {
            if (abi.contains("armeabi-v7a") || abi.contains("arm64-v8a")) {
                isArm = true;
                break;
            }
        }
        if (isArm) {
            notes.add("Architecture ARM détectée : compatible avec les binaires QEMU ARMv7 et le noyau invité Goldfish 3.10 ARM.");
        } else {
            notes.add("Architecture non-ARM (" + report.primaryAbi + ") : exécution possible via cross-émulation ou binaire x86/x86_64.");
        }

        // 2. Hardware Virtualization (KVM)
        if (report.kvmAvailable) {
            notes.add("Accélération KVM (/dev/kvm) : DISPONIBLE ! Performances quasi-natives possibles.");
        } else {
            notes.add("Accélération KVM (/dev/kvm) : ABSENTE (noyau stock KitKat). Émulation TCG (Tiny Code Generator) obligatoire.");
        }

        // 3. RAM Requirements
        long freeMb = report.availRamBytes / (1024 * 1024);
        if (freeMb >= 512) {
            notes.add("RAM disponible (" + freeMb + " Mo) : SUFFISANTE pour allouer 512 Mo à la machine virtuelle Android 6.0.");
        } else if (freeMb >= 384) {
            notes.add("RAM disponible (" + freeMb + " Mo) : LIMITE. Allocation recommandée de 384 Mo avec zRAM pour éviter le LowMemoryKiller.");
        } else {
            notes.add("RAM disponible (" + freeMb + " Mo) : CRITIQUE. Fermer les applications d'arrière-plan avant le démarrage de la VM.");
        }

        // 4. Storage Requirements
        long freeStorageMb = report.internalAvailBytes / (1024 * 1024);
        long requiredStorageMb = 2500; // ~2.5 GB for system.img + userdata.img + cache
        if (freeStorageMb >= requiredStorageMb) {
            notes.add("Stockage interne libre (" + (freeStorageMb / 1024) + " Go) : SUFFISANT pour l'image Android 6 AOSP (~2.5 Go).");
        } else {
            notes.add("Stockage interne libre (" + freeStorageMb + " Mo < 2.5 Go) : INSUFFISANT sur la mémoire interne. Une carte MicroSD externe montée est indispensable.");
        }

        // 5. Native JNI & Toolchain
        if (report.nativeLoaded) {
            notes.add("Fondation native JNI : FONCTIONNELLE (" + report.nativeArch + ", pointeurs " + report.pointerSizeBits + " bits).");
        } else {
            notes.add("Fondation native JNI : ERREUR (" + report.nativeError + ").");
            canProceed = false;
        }

        // 6. Conclusion
        if (canProceed) {
            report.feasibilityStatus = "CONDITIONAL GO";
            notes.add("DÉCISION ÉTAPE 1 : CONDITIONAL GO. Faisable avec QEMU TCG ARMv7, allocation 512 Mo RAM, et stockage sur carte SD.");
        } else {
            report.feasibilityStatus = "NO GO";
            notes.add("DÉCISION ÉTAPE 1 : NO GO. Résoudre les erreurs natives avant de poursuivre.");
        }
    }

    public static String formatBytes(long bytes) {
        if (bytes < 0) return "Non disponible";
        if (bytes < 1024) return bytes + " B";
        int z = (63 - Long.numberOfLeadingZeros(bytes)) / 10;
        return String.format(Locale.US, "%.2f %sB", (double) bytes / (1L << (z * 10)), " KMGTPE".charAt(z));
    }
}
