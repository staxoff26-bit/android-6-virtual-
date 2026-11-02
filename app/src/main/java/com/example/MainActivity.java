package com.example;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity implements QemuEngine.StateListener, QemuEngine.LogListener {

    private LinearLayout contentContainer;
    private DeviceAudit.AuditReport lastReport;
    private QemuEngine qemuEngine;

    // QEMU Engine UI components
    private TextView engineStateBadge;
    private TextView engineDevicesView;
    private TextView engineLogsView;
    private Button btnInit;
    private Button btnStart;
    private Button btnPause;
    private Button btnResume;
    private Button btnStop;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        qemuEngine = QemuEngine.getInstance();
        qemuEngine.setStateListener(this);
        qemuEngine.setLogListener(this);

        // Main layout container: Clean Light Theme (Fond clair sobre et fluide)
        ScrollView scrollView = new ScrollView(this);
        scrollView.setBackgroundColor(Color.parseColor("#F8FAFC")); // Slate 50
        scrollView.setFillViewport(true);

        contentContainer = new LinearLayout(this);
        contentContainer.setOrientation(LinearLayout.VERTICAL);
        int pad = dpToPx(16);
        contentContainer.setPadding(pad, pad, pad, pad);
        scrollView.addView(contentContainer, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));

        setContentView(scrollView);

        buildUserInterface();
    }

    private void buildUserInterface() {
        contentContainer.removeAllViews();
        lastReport = DeviceAudit.runAudit(this);

        // Header Title
        TextView titleView = new TextView(this);
        titleView.setText("Android 6 Virtual");
        titleView.setTextColor(Color.parseColor("#0F172A")); // Slate 900
        titleView.setTextSize(22);
        titleView.setTypeface(Typeface.DEFAULT_BOLD);
        contentContainer.addView(titleView);

        // Header Subtitle
        TextView subView = new TextView(this);
        subView.setText("Étape 2/5 : Intégration du Moteur QEMU Natif ARMv7\nCible : Honor 4X CHE2-TL00M (Android 4.4.3 / Kirin 620)");
        subView.setTextColor(Color.parseColor("#64748B")); // Slate 500
        subView.setTextSize(12);
        subView.setPadding(0, dpToPx(4), 0, dpToPx(12));
        contentContainer.addView(subView);

        // ==========================================
        // SECTION 1: QEMU ENGINE PANEL (NOUVEAU ÉTAPE 2)
        // ==========================================
        buildQemuEngineSection();
        addSpacer(14);

        // ==========================================
        // SECTION 2: AUDIT SYSTÈME EXISTANT (ÉTAPE 1)
        // ==========================================
        buildSystemAuditSection();
        addSpacer(14);

        // Action Buttons Row
        LinearLayout buttonRow = new LinearLayout(this);
        buttonRow.setOrientation(LinearLayout.HORIZONTAL);
        buttonRow.setWeightSum(2f);

        Button refreshBtn = createStyledButton("Actualiser Diagnostic", Color.parseColor("#2563EB"), Color.WHITE);
        LinearLayout.LayoutParams p1 = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.0f);
        p1.setMargins(0, 0, dpToPx(6), 0);
        refreshBtn.setLayoutParams(p1);
        refreshBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                buildUserInterface();
                Toast.makeText(MainActivity.this, "Diagnostic actualisé", Toast.LENGTH_SHORT).show();
            }
        });
        buttonRow.addView(refreshBtn);

        Button copyBtn = createStyledButton("Copier Rapport", Color.parseColor("#475569"), Color.WHITE);
        LinearLayout.LayoutParams p2 = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.0f);
        p2.setMargins(dpToPx(6), 0, 0, 0);
        copyBtn.setLayoutParams(p2);
        copyBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                copyFullReportToClipboard();
            }
        });
        buttonRow.addView(copyBtn);

        contentContainer.addView(buttonRow);
        addSpacer(20);

        updateButtonStates(qemuEngine.getCurrentStateCode());
    }

    private void buildQemuEngineSection() {
        LinearLayout qemuCard = createCard(Color.WHITE, Color.parseColor("#CBD5E1"));
        addSectionHeader(qemuCard, "QEMU ENGINE (MOTEUR D'ÉMULATION ARMv7)", Color.parseColor("#1D4ED8"));

        // Status badge
        LinearLayout stateRow = new LinearLayout(this);
        stateRow.setOrientation(LinearLayout.HORIZONTAL);
        stateRow.setPadding(0, 0, 0, dpToPx(8));

        TextView stateLabel = new TextView(this);
        stateLabel.setText("État du moteur :");
        stateLabel.setTextColor(Color.parseColor("#475569"));
        stateLabel.setTextSize(13);
        stateLabel.setTypeface(Typeface.DEFAULT_BOLD);
        stateRow.addView(stateLabel);

        engineStateBadge = new TextView(this);
        engineStateBadge.setText(" " + qemuEngine.getCurrentStateString());
        engineStateBadge.setTextSize(13);
        engineStateBadge.setTypeface(Typeface.DEFAULT_BOLD);
        engineStateBadge.setTextColor(getStateColor(qemuEngine.getCurrentStateCode()));
        stateRow.addView(engineStateBadge);
        qemuCard.addView(stateRow);

        // Authenticity Status Card
        boolean isAuthentic = qemuEngine.isAuthenticQemuLoaded();
        LinearLayout authCard = createCard(isAuthentic ? Color.parseColor("#F0FDF4") : Color.parseColor("#FFFBEB"),
                isAuthentic ? Color.parseColor("#22C55E") : Color.parseColor("#F59E0B"));
        authCard.setPadding(dpToPx(8), dpToPx(8), dpToPx(8), dpToPx(8));
        TextView authTitle = new TextView(this);
        authTitle.setText(isAuthentic ? "AUTHENTICITÉ DU MOTEUR : VÉRITABLE QEMU INTÉGRÉ" : "AUDIT D'AUTHENTICITÉ : DÉTECTION EN COURS");
        authTitle.setTextColor(isAuthentic ? Color.parseColor("#15803D") : Color.parseColor("#B45309"));
        authTitle.setTextSize(12);
        authTitle.setTypeface(Typeface.DEFAULT_BOLD);
        authCard.addView(authTitle);

        TextView authDetails = new TextView(this);
        authDetails.setText(qemuEngine.getAuthenticQemuDetails());
        authDetails.setTextColor(Color.parseColor("#334155"));
        authDetails.setTextSize(10);
        authDetails.setTypeface(Typeface.MONOSPACE);
        authDetails.setPadding(0, dpToPx(4), 0, 0);
        authCard.addView(authDetails);
        qemuCard.addView(authCard);
        addSpacer(8);

        // Technical specs
        addRow(qemuCard, "Version QEMU :", qemuEngine.getVersion());
        addRow(qemuCard, "Architecture Hôte :", lastReport.primaryAbi + " (Cortex-A53 en 32 bits)");
        addRow(qemuCard, "Architecture Invitée :", "arm-softmmu (Cortex-A15 ARMv7-A)");
        addRow(qemuCard, "Moteur d'exécution :", "TCG v2 (Tiny Code Generator JIT, sans KVM)");
        addRow(qemuCard, "Bibliothèque native :", qemuEngine.isNativeLoaded() ? "libqemubridge.so (CHARGÉE)" : "ERREUR: " + qemuEngine.getNativeLoadError());
        addRow(qemuCard, "Machine virtuelle :", "ranchu (Android Marshmallow VirtIO)");
        addRow(qemuCard, "Machines supportées :", qemuEngine.getAvailableMachines().replace("\n", " | "));

        // Detected Peripherals summary
        TextView devTitle = new TextView(this);
        devTitle.setText("Périphériques virtuels Ranchu :");
        devTitle.setTextColor(Color.parseColor("#334155"));
        devTitle.setTextSize(12);
        devTitle.setTypeface(Typeface.DEFAULT_BOLD);
        devTitle.setPadding(0, dpToPx(6), 0, dpToPx(2));
        qemuCard.addView(devTitle);

        engineDevicesView = new TextView(this);
        engineDevicesView.setText(qemuEngine.getDetectedDevices());
        engineDevicesView.setTextColor(Color.parseColor("#475569"));
        engineDevicesView.setTextSize(11);
        engineDevicesView.setPadding(0, 0, 0, dpToPx(8));
        qemuCard.addView(engineDevicesView);

        // Engine Control Buttons
        TextView ctrlTitle = new TextView(this);
        ctrlTitle.setText("Commandes de contrôle du moteur :");
        ctrlTitle.setTextColor(Color.parseColor("#0F172A"));
        ctrlTitle.setTextSize(12);
        ctrlTitle.setTypeface(Typeface.DEFAULT_BOLD);
        ctrlTitle.setPadding(0, dpToPx(4), 0, dpToPx(6));
        qemuCard.addView(ctrlTitle);

        LinearLayout row1 = new LinearLayout(this);
        row1.setOrientation(LinearLayout.HORIZONTAL);
        row1.setWeightSum(2f);

        btnInit = createStyledButton("INITIALISER", Color.parseColor("#0284C7"), Color.WHITE);
        LinearLayout.LayoutParams pInit = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.0f);
        pInit.setMargins(0, 0, dpToPx(4), dpToPx(6));
        btnInit.setLayoutParams(pInit);
        btnInit.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                btnInit.setEnabled(false);
                qemuEngine.initializeEngine(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(MainActivity.this, "Moteur QEMU initialisé", Toast.LENGTH_SHORT).show();
                    }
                });
            }
        });
        row1.addView(btnInit);

        btnStart = createStyledButton("DÉMARRER", Color.parseColor("#16A34A"), Color.WHITE);
        LinearLayout.LayoutParams pStart = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.0f);
        pStart.setMargins(dpToPx(4), 0, 0, dpToPx(6));
        btnStart.setLayoutParams(pStart);
        btnStart.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                btnStart.setEnabled(false);
                qemuEngine.start(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(MainActivity.this, "Moteur QEMU démarré", Toast.LENGTH_SHORT).show();
                    }
                });
            }
        });
        row1.addView(btnStart);
        qemuCard.addView(row1);

        LinearLayout row2 = new LinearLayout(this);
        row2.setOrientation(LinearLayout.HORIZONTAL);
        row2.setWeightSum(3f);

        btnPause = createStyledButton("PAUSE", Color.parseColor("#D97706"), Color.WHITE);
        LinearLayout.LayoutParams pPause = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.0f);
        pPause.setMargins(0, 0, dpToPx(4), 0);
        btnPause.setLayoutParams(pPause);
        btnPause.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                qemuEngine.pause(null);
            }
        });
        row2.addView(btnPause);

        btnResume = createStyledButton("REPRENDRE", Color.parseColor("#0D9488"), Color.WHITE);
        LinearLayout.LayoutParams pResume = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.0f);
        pResume.setMargins(dpToPx(4), 0, dpToPx(4), 0);
        btnResume.setLayoutParams(pResume);
        btnResume.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                qemuEngine.resume(null);
            }
        });
        row2.addView(btnResume);

        btnStop = createStyledButton("ARRÊTER", Color.parseColor("#DC2626"), Color.WHITE);
        LinearLayout.LayoutParams pStop = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.0f);
        pStop.setMargins(dpToPx(4), 0, 0, 0);
        btnStop.setLayoutParams(pStop);
        btnStop.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                qemuEngine.stop(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(MainActivity.this, "Moteur QEMU arrêté proprement", Toast.LENGTH_SHORT).show();
                    }
                });
            }
        });
        row2.addView(btnStop);
        qemuCard.addView(row2);

        // Technical Logs Console
        TextView logTitle = new TextView(this);
        logTitle.setText("Journaux techniques en direct (TCG / JNI / Machine) :");
        logTitle.setTextColor(Color.parseColor("#0F172A"));
        logTitle.setTextSize(12);
        logTitle.setTypeface(Typeface.DEFAULT_BOLD);
        logTitle.setPadding(0, dpToPx(10), 0, dpToPx(4));
        qemuCard.addView(logTitle);

        ScrollView logScroll = new ScrollView(this);
        logScroll.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dpToPx(140)));
        logScroll.setBackgroundColor(Color.parseColor("#0F172A")); // Slate 900 terminal bg
        int logPad = dpToPx(8);
        logScroll.setPadding(logPad, logPad, logPad, logPad);

        engineLogsView = new TextView(this);
        engineLogsView.setText(qemuEngine.getLogs());
        engineLogsView.setTextColor(Color.parseColor("#38BDF8")); // Monospace Sky 400
        engineLogsView.setTextSize(10);
        engineLogsView.setTypeface(Typeface.MONOSPACE);
        logScroll.addView(engineLogsView);
        qemuCard.addView(logScroll);

        contentContainer.addView(qemuCard);
    }

    private void buildSystemAuditSection() {
        // Feasibility Decision Banner
        boolean isGo = "CONDITIONAL GO".equals(lastReport.feasibilityStatus) || "GO".equals(lastReport.feasibilityStatus);
        int bannerBg = isGo ? Color.parseColor("#ECFDF5") : Color.parseColor("#FEF2F2");
        int bannerBorder = isGo ? Color.parseColor("#10B981") : Color.parseColor("#EF4444");

        LinearLayout bannerCard = createCard(bannerBg, bannerBorder);
        TextView bannerTitle = new TextView(this);
        bannerTitle.setText("FAISABILITÉ GLOBALE : " + lastReport.feasibilityStatus);
        bannerTitle.setTextColor(isGo ? Color.parseColor("#065F46") : Color.parseColor("#991B1B"));
        bannerTitle.setTextSize(15);
        bannerTitle.setTypeface(Typeface.DEFAULT_BOLD);
        bannerCard.addView(bannerTitle);

        StringBuilder notesText = new StringBuilder();
        for (String note : lastReport.feasibilityNotes) {
            notesText.append("• ").append(note).append("\n");
        }
        TextView bannerDesc = new TextView(this);
        bannerDesc.setText(notesText.toString().trim());
        bannerDesc.setTextColor(Color.parseColor("#334155"));
        bannerDesc.setTextSize(11);
        bannerDesc.setPadding(0, dpToPx(6), 0, 0);
        bannerCard.addView(bannerDesc);

        contentContainer.addView(bannerCard);
        addSpacer(12);

        // Host System & Identity
        LinearLayout sysCard = createCard(Color.WHITE, Color.parseColor("#E2E8F0"));
        addSectionHeader(sysCard, "AUDIT MATÉRIEL & HÔTE KITKAT (HONOR 4X)", Color.parseColor("#0284C7"));
        addRow(sysCard, "Modèle / Fabricant :", lastReport.manufacturer + " " + lastReport.model + " (" + lastReport.device + ")");
        addRow(sysCard, "Matériel (Hardware) :", lastReport.hardware + " / Board: " + lastReport.board);
        addRow(sysCard, "Version Android hôte :", "Android " + lastReport.androidRelease + " (API " + lastReport.sdkInt + ")");
        addRow(sysCard, "Processeur hôte :", lastReport.cpuModel + " (" + lastReport.cpuCores + " cœurs actifs)");
        addRow(sysCard, "RAM Physique totale :", DeviceAudit.formatBytes(lastReport.totalRamBytes));
        addRow(sysCard, "RAM Disponible :", DeviceAudit.formatBytes(lastReport.availRamBytes));
        addRow(sysCard, "Stockage /data libre :", DeviceAudit.formatBytes(lastReport.internalAvailBytes));
        addRow(sysCard, "Carte SD Externe :", lastReport.externalStorageState != null ? lastReport.externalStorageState : "Non montée");
        contentContainer.addView(sysCard);
    }

    private int getStateColor(int code) {
        switch (code) {
            case 1: return Color.parseColor("#0284C7"); // INITIALIZED
            case 2: return Color.parseColor("#16A34A"); // RUNNING
            case 3: return Color.parseColor("#D97706"); // PAUSED
            case 4: return Color.parseColor("#64748B"); // STOPPED
            case 5: return Color.parseColor("#DC2626"); // ERROR
            default: return Color.parseColor("#94A3B8"); // UNINITIALIZED
        }
    }

    private void updateButtonStates(int stateCode) {
        boolean loaded = qemuEngine.isNativeLoaded();
        if (!loaded) {
            btnInit.setEnabled(false);
            btnStart.setEnabled(false);
            btnPause.setEnabled(false);
            btnResume.setEnabled(false);
            btnStop.setEnabled(false);
            return;
        }

        switch (stateCode) {
            case 0: // UNINITIALIZED
                btnInit.setEnabled(true);
                btnStart.setEnabled(true);
                btnPause.setEnabled(false);
                btnResume.setEnabled(false);
                btnStop.setEnabled(false);
                break;
            case 1: // INITIALIZED
                btnInit.setEnabled(true);
                btnStart.setEnabled(true);
                btnPause.setEnabled(false);
                btnResume.setEnabled(false);
                btnStop.setEnabled(false);
                break;
            case 2: // RUNNING
                btnInit.setEnabled(false);
                btnStart.setEnabled(false);
                btnPause.setEnabled(true);
                btnResume.setEnabled(false);
                btnStop.setEnabled(true);
                break;
            case 3: // PAUSED
                btnInit.setEnabled(false);
                btnStart.setEnabled(false);
                btnPause.setEnabled(false);
                btnResume.setEnabled(true);
                btnStop.setEnabled(true);
                break;
            case 4: // STOPPED
                btnInit.setEnabled(true);
                btnStart.setEnabled(true);
                btnPause.setEnabled(false);
                btnResume.setEnabled(false);
                btnStop.setEnabled(false);
                break;
            case 5: // ERROR
            default:
                btnInit.setEnabled(true);
                btnStart.setEnabled(true);
                btnPause.setEnabled(false);
                btnResume.setEnabled(false);
                btnStop.setEnabled(false);
                break;
        }
    }

    @Override
    public void onStateChanged(final int stateCode, final String stateDescription) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (engineStateBadge != null) {
                    engineStateBadge.setText(" " + stateDescription);
                    engineStateBadge.setTextColor(getStateColor(stateCode));
                }
                if (engineDevicesView != null) {
                    engineDevicesView.setText(qemuEngine.getDetectedDevices());
                }
                updateButtonStates(stateCode);
            }
        });
    }

    @Override
    public void onLogsUpdated(final String logs) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (engineLogsView != null) {
                    engineLogsView.setText(logs);
                }
            }
        });
    }

    private void copyFullReportToClipboard() {
        StringBuilder sb = new StringBuilder();
        sb.append("=== ANDROID 6 VIRTUAL - ÉTAPE 2/5 (QEMU ENGINE) ===\n");
        sb.append("Version QEMU : ").append(qemuEngine.getVersion()).append("\n");
        sb.append("État Moteur : ").append(qemuEngine.getCurrentStateString()).append("\n");
        sb.append("Machines : ").append(qemuEngine.getAvailableMachines()).append("\n");
        sb.append("Périphériques :\n").append(qemuEngine.getDetectedDevices()).append("\n");
        sb.append("Hôte : ").append(lastReport.manufacturer).append(" ").append(lastReport.model).append("\n");
        sb.append("Android hôte : ").append(lastReport.androidRelease).append(" (API ").append(lastReport.sdkInt).append(")\n");
        sb.append("RAM Totale : ").append(DeviceAudit.formatBytes(lastReport.totalRamBytes)).append("\n");
        sb.append("Stockage libre : ").append(DeviceAudit.formatBytes(lastReport.internalAvailBytes)).append("\n");
        sb.append("Journaux QEMU récents :\n").append(qemuEngine.getLogs()).append("\n");

        ClipboardManager clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData clip = ClipData.newPlainText("QEMU Engine Report", sb.toString());
        if (clipboard != null) {
            clipboard.setPrimaryClip(clip);
            Toast.makeText(this, "Rapport complet copié", Toast.LENGTH_SHORT).show();
        }
    }

    private LinearLayout createCard(int bgColor, int borderColor) {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        int p = dpToPx(14);
        card.setPadding(p, p, p, p);

        GradientDrawable bg = new GradientDrawable();
        bg.setColor(bgColor);
        bg.setCornerRadius(dpToPx(8));
        bg.setStroke(dpToPx(1), borderColor);
        card.setBackground(bg);

        return card;
    }

    private void addSectionHeader(LinearLayout card, String title, int color) {
        TextView tv = new TextView(this);
        tv.setText(title);
        tv.setTextColor(color);
        tv.setTextSize(13);
        tv.setTypeface(Typeface.DEFAULT_BOLD);
        tv.setPadding(0, 0, 0, dpToPx(8));
        card.addView(tv);
    }

    private void addRow(LinearLayout card, String label, String value) {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setPadding(0, dpToPx(2), 0, dpToPx(2));

        TextView tvLabel = new TextView(this);
        tvLabel.setText(label);
        tvLabel.setTextColor(Color.parseColor("#64748B")); // Slate 500
        tvLabel.setTextSize(11);
        tvLabel.setLayoutParams(new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.2f));

        TextView tvVal = new TextView(this);
        tvVal.setText(value);
        tvVal.setTextColor(Color.parseColor("#0F172A")); // Slate 900
        tvVal.setTextSize(11);
        tvVal.setTypeface(Typeface.DEFAULT_BOLD);
        tvVal.setLayoutParams(new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.8f));

        row.addView(tvLabel);
        row.addView(tvVal);
        card.addView(row);
    }

    private Button createStyledButton(String text, int bgColor, int textColor) {
        Button btn = new Button(this);
        btn.setText(text);
        btn.setTextColor(textColor);
        btn.setTextSize(12);
        btn.setTypeface(Typeface.DEFAULT_BOLD);

        GradientDrawable bg = new GradientDrawable();
        bg.setColor(bgColor);
        bg.setCornerRadius(dpToPx(6));
        btn.setBackground(bg);

        return btn;
    }

    private void addSpacer(int dp) {
        View v = new View(this);
        v.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dpToPx(dp)));
        contentContainer.addView(v);
    }

    private int dpToPx(int dp) {
        float density = getResources().getDisplayMetrics().density;
        return (int) (dp * density + 0.5f);
    }
}
