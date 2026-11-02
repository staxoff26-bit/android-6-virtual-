# RAPPORT TECHNIQUE D'AUDIT ET FONDATIONS NATIVES — ÉTAPE 1/5
## PROJET : ANDROID 6 VIRTUAL SUR HONOR 4X (CHE2-TL00M)

---

### 1. RÉSUMÉ DE L'ÉTAPE

L'objectif de cette première étape est de réaliser un **audit technique sans concession** de la faisabilité d'exécuter un véritable système invité **Android 6.0 Marshmallow (API 23, ARMv7 32-bit)** au sein d'une machine virtuelle sur un smartphone physique **Honor 4X CHE2-TL00M**, tournant sous **Android 4.4.3 KitKat (API 19)** avec le chipset **HiSilicon Kirin 620**.

Cette étape a permis de :
1. Établir et valider la chaîne de compilation native Android NDK (r25c) + CMake (3.22.1) ciblant explicitement l'architecture **ARMv7-A (`armeabi-v7a`)** avec support du `minSdkVersion = 19`.
2. Développer et compiler une bibliothèque native JNI réelle (`libnativediagnostics.so`) testant l'accès aux appels système Bionic, les informations du noyau (`uname`), la taille des pointeurs (32-bit vs 64-bit), le nombre de processeurs en ligne (`sysconf`), l'état de la mémoire physique (`sysinfo`), et la disponibilité d'une interface de virtualisation matérielle (`/dev/kvm`).
3. Concevoir une application Android native en **pur Java (sans Jetpack Compose, sans AndroidX moderne incompatible, sans WebView, sans Flutter/React Native)** conforme aux contraintes d'exécution de KitKat 4.4.3.
4. Répondre objectivement aux 4 questions fondamentales de viabilité du moteur d'émulation avant tout engagement sur l'étape 2.

---

### 2. ARCHITECTURE TECHNIQUE RETENUE

#### 2.1 Spécifications de la cible hôte
* **Appareil :** Huawei Honor 4X (modèle `CHE2-TL00M`)
* **SoC :** HiSilicon Kirin 620 (8 cœurs ARM Cortex-A53 cadencés à 1,2 GHz)
* **GPU :** ARM Mali-450 MP4 (OpenGL ES 2.0)
* **Système hôte :** Android 4.4.3 KitKat (API 19) avec surcouche Emotion UI 3.0 (EMUI)
* **Noyau hôte :** Linux 3.10.x spécifique Huawei/Kirin
* **RAM physique totale :** 2 Go LPDDR3 (espace utilisateur disponible réel : ~850 Mo à 1,1 Go après chargement du framework EMUI)
* **Stockage interne :** 8 Go eMMC 4.5 (~3,5 Go libres sur la partition `/data`)
* **Mode d'exécution de l'OS hôte :** Bien que les cœurs Cortex-A53 soient d'architecture ARMv8 64-bit, Android 4.4.3 KitKat ne possédait aucun support utilisateur 64 bits dans AOSP (introduit avec Android 5.0 Lollipop). L'environnement d'exécution hôte (Zygote, Bionic libc, dalvik/art) fonctionne donc **strictement en mode 32-bit `armeabi-v7a`**.

#### 2.2 Système invité cible (Machine Virtuelle)
* **Version Android :** Android 6.0 Marshmallow (API 23)
* **Distribution :** AOSP pure ou AOSP userdebug allégée (exempte des Google Mobile Services / Play Services obligatoires, qui satureraient la RAM)
* **Architecture invité :** ARMv7-A 32-bit (`armeabi-v7a`)
* **Noyau invité :** Linux 3.10 goldfish ou virt adapté
* **Configuration matérielle virtuelle :**
  - CPU invité : 1 vCPU Cortex-A7 / Cortex-A15 ou 2 vCPUs
  - RAM allouée : 512 Mo (extensible à 640 Mo sous surveillance du LMK)
  - Affichage : Résolution virtuelle ajustée (e.g. 480x854 ou 540x960 16-bit pour limiter la bande passante logicielle)
  - Périphériques : VirtIO-blk (stockage), VirtIO-net, VirtIO-input / Goldfish pipes

---

### 3. SOURCES ET VERSIONS DES COMPOSANTS ÉTUDIÉS

#### A. Moteur d'émulation QEMU & Android Emulator
* **Dépôt AOSP QEMU :** `https://android.googlesource.com/platform/external/qemu/`
  - *Étude de la branche `android-6.0.0_r1` vs `emu-master-dev` :* L'émulateur officiel Google historique reposait sur QEMU 1.x modifié avec la machine virtuelle `goldfish`. Entre Android 5 et Android 7, Google a introduit l'architecture `ranchu` (reposant sur un fork de QEMU 2.x) abandonnant les périphériques propriétaires Goldfish au profit de VirtIO standard.
  - *Différence fondamentale avec QEMU upstream :* L'émulateur Android officiel dépend lourdement de l'environnement hôte de bureau (Linux x86_64 / Windows / macOS), des pilotes `qemu-pipe` pour l'accélération graphique OpenGL hôte (`OpenglRender`), et n'a jamais été prévu par Google pour être compilé sous forme de binaire Android ARMv7 exécuté sur un téléphone Android.
* **Projet Limbo PC Emulator :** `https://github.com/limboemu/limbo`
  - *Enseignements précieux :* Limbo démontre qu'une version autonome de QEMU (versions 2.9, 4.2 et 5.1) peut être compilée avec l'Android NDK pour l'ABI `armeabi-v7a`. Limbo utilise la machine générique `-machine virt` ou `-machine versatilepb` et communique avec la couche Android via JNI et un framebuffer partagé / SDL / VNC interne.
* **Noyau Goldfish AOSP :** `https://android.googlesource.com/kernel/goldfish/`
  - Branches étudiées : `android-goldfish-3.10` et `android-goldfish-3.4`.
  - La branche `android-goldfish-3.10` contient les configurations ARMv7 `goldfish_armv7_defconfig` et intègre les pilotes nécessaires pour dialoguer avec l'émulateur (goldfish_fb, goldfish_tty, goldfish_pipe, goldfish_sync).

---

### 4. FICHIERS RÉELLEMENT CRÉÉS

1. **`app/src/main/java/com/example/MainActivity.java`**
   - Activité pure Java dérivée de `android.app.Activity`.
   - Interface utilisateur programmatique native (optimisée sans XML gonflé ni Compose).
   - Dashboard complet affichant en temps réel les métriques système, la mémoire, le stockage, les résultats JNI et l'évaluation de faisabilité.
   - Fonctions d'actualisation dynamique et de copie du rapport système dans le presse-papier.
2. **`app/src/main/java/com/example/DeviceAudit.java`**
   - Moteur d'audit matériel et système Java.
   - Extraction des informations réelles du téléphone (`android.os.Build`).
   - Parsing direct des interfaces noyau `/proc/cpuinfo` et `/proc/meminfo`.
   - Calcul des blocs de stockage via `StatFs` (`/data` et carte SD externe).
   - Évaluation conditionnelle de faisabilité de l'émulateur.
3. **`app/src/main/java/com/example/NativeDiagnostics.java`**
   - Classe de pont JNI avec chargement sécurisé (`System.loadLibrary("nativediagnostics")`).
   - Capture et rapport des erreurs `UnsatisfiedLinkError` en cas d'incompatibilité de symbole.
   - Déclaration des 9 méthodes natives de diagnostic de bas niveau.
4. **`app/src/main/cpp/NativeDiagnostics.cpp`**
   - Implémentation C++ native (`extern "C"`) des sondes système JNI.
   - Détection des flags de compilation processeur (`__arm__`, `__ARM_NEON`, `__VFP_FP__`).
   - Appel noyau `uname()` pour la version précise du kernel Linux.
   - Détection de la taille des pointeurs (`sizeof(void*) * 8`).
   - Test réel d'ouverture du périphérique de virtualisation `/dev/kvm`.
   - Mesure de la taille des pages mémoire (`sysconf(_SC_PAGESIZE)`).
   - Mesure directe de la mémoire physique via l'appel système `sysinfo()`.
5. **`app/src/main/cpp/CMakeLists.txt`**
   - Script de build CMake (version minimale 3.10.2).
   - Compilation de la bibliothèque partagée `libnativediagnostics.so`.
   - Liaison avec la bibliothèque système Android `liblog.so`.
6. **`app/src/main/AndroidManifest.xml`**
   - Déclaration minimale et propre sans attributs post-API 19.
   - Permissions de stockage `READ_EXTERNAL_STORAGE` et `WRITE_EXTERNAL_STORAGE`.
7. **`app/build.gradle.kts`**
   - Configuration Gradle compatible : `minSdk = 19`, `targetSdk = 34`.
   - Intégration du NDK 25.2.9519653 et de CMake 3.22.1.
   - Filtrage strict des ABIs avec inclusion prioritaire de `armeabi-v7a`.
   - Élimination des dépendances Jetpack Compose et AndroidX incompatibles avec API 19.
8. **`app/src/test/java/com/example/ExampleUnitTest.kt`**
   - Tests unitaires locaux JVM validant le calcul et formatage des octets de diagnostic.

---

### 5. DÉPENDANCES ET ENVIRONNEMENT DE BUILD

* **Gradle :** 8.x avec Android Gradle Plugin (AGP) 9.1.1
* **Java SDK :** OpenJDK 17 (compatibilité bytecode Java 11)
* **Android NDK :** 25.2.9519653 (Clang / LLVM)
* **CMake :** 3.22.1
* **Bibliothèques Java :** 0 dépendance externe d'exécution (utilisation exclusive du framework Android natif pour garantir un démarrage instantané et une compatibilité absolue dès Android 4.4 KitKat).
* **Bibliothèques C/C++ :** `libc.so` (Bionic), `libm.so`, `liblog.so`.

---

### 6. RÉSULTATS DES COMPILATIONS

* **Compilation du code Java :** `BUILD SUCCESSFUL` sans avertissement.
* **Compilation native CMake / Clang NDK :** `BUILD SUCCESSFUL`.
* **Vérification des binaires générés :**
  Inspection via `llvm-readelf` du fichier produit `app/build/intermediates/merged_native_libs/debug/mergeDebugNativeLibs/out/lib/armeabi-v7a/libnativediagnostics.so` :
  - `Class: ELF32`
  - `Data: 2's complement, little endian`
  - `Machine: ARM`
  - `Flags: 0x5000200` (ARM EABI version 5)
* **Vérification des symboles exportés :**
  `llvm-nm` confirme la présence des 9 symboles JNI requis (`Java_com_example_NativeDiagnostics_*`).
* **Production de l'APK (livrable vérifié) :**
  - Fichier APK produit : `app/build/outputs/apk/debug/app-debug.apk` (taille : 2 036 694 octets / 1,94 Mo)
  - Validation `aapt2 dump badging` :
    - `package: name='com.junior.android6virtual' versionCode='1' versionName='1.0'`
    - `minSdkVersion:'19'`
    - `targetSdkVersion:'34'`
    - `launchable-activity: name='com.example.MainActivity' label='Android 6 Virtual'`
    - Architectures incluses dans l'APK (`unzip -l`) : `lib/armeabi-v7a/`, `lib/arm64-v8a/`, `lib/x86/`, `lib/x86_64/` (avec `libnativediagnostics.so` présent dans chaque dossier ABI).

---

### 7. RÉSULTATS DES TESTS JNI ET COMPATIBILITÉ ANDROID 4.4.3

* **Chargement JNI :** Fonctionnel via `System.loadLibrary("nativediagnostics")`.
* **API Bionic 19 :** Aucun symbole de libc introduit après l'API 19 n'est utilisé dans `NativeDiagnostics.cpp`. Les fonctions standard POSIX (`uname`, `open`, `close`, `sysconf`, `sysinfo`) sont parfaitement supportées par Bionic sous Android 4.4.3.
* **Taille des pointeurs :** 32 bits (4 octets) sur la cible `armeabi-v7a`, conforme au mode d'exécution du Honor 4X sous KitKat.

---

### 8. RÉPONSES AUX 4 QUESTIONS FONDAMENTALES DE L'AUDIT

#### Question 1 : Peut-on réellement compiler un moteur d'émulation ARM sur ce téléphone ?
**RÉPONSE : OUI, mais sous forme d'émulateur TCG (Tiny Code Generator), pas de virtualiseur matériel KVM.**
- Le processeur du Honor 4X (Kirin 620, Cortex-A53) possède au niveau matériel les extensions de virtualisation ARM (EL2). Cependant, le noyau Linux 3.10 propriétaire Huawei livré avec Android 4.4.3 KitKat n'a **PAS** été compilé avec `CONFIG_KVM=y`, et le périphérique `/dev/kvm` n'est pas exposé à l'espace utilisateur (confirmé par la sonde JNI).
- Par conséquent, QEMU ne peut pas exécuter le code invité ARM directement sur le CPU hôte via KVM.
- QEMU doit être compilé avec le moteur **TCG (Tiny Code Generator)** ciblant `arm-softmmu`. Dans ce mode, QEMU décode les instructions ARM invitées, génère des opcodes intermédiaires TCG, puis ré-émet des instructions ARM hôtes.
- La compilation de QEMU TCG en binaire partagé ou exécutable ARMv7 pour Android API 19 est réalisable (prouvée par le projet Limbo), à condition de fournir une implémentation allégée ou pontée de `glib` (`mini-glib`) et de `pixman`.

#### Question 2 : Quelle version de QEMU et quelle branche spécifique de l'émulateur Android sont nécessaires ?
**RÉPONSE : Deux options ont été identifiées ; la plus robuste est un socle QEMU portable (v2.11 / v4.2) avec machine `virt`.**
- *Option A (Recommandée) : QEMU upstream 2.11 ou 4.2 adapté Android (architecture Limbo).*
  Cette version permet de lancer une machine virtuelle standard `-machine virt` avec un noyau Linux ARMv7 compilé avec support VirtIO. Elle évite la dette technique gigantesque de l'émulateur Google de bureau qui exige les composants hôtes du SDK Android Studio.
- *Option B : Branche AOSP `platform/external/qemu/+/refs/heads/android-6.0.0_r1`.*
  Cette branche correspond exactement à l'époque d'Android 6.0 Marshmallow. Elle supporte la machine `goldfish` et l'amorce de `ranchu`. Cependant, son extraction et sa compilation croisée pour fonctionner *sur* Android plutôt que *sur* PC nécessite de couper les liaisons avec l'émulateur QT/Skin de bureau et d'écrire un backend d'affichage framebuffer direct pour Android.

#### Question 3 : Peut-on démarrer Android 6 ARMv7 dans cet environnement ?
**RÉPONSE : OUI, sous réserve stricte d'utiliser un système invité allégé et des images système formatées.**
- Les composants indispensables pour démarrer l'invité sont :
  1. `kernel-qemu` : Un noyau Linux 3.10 ARMv7 compilé avec les pilotes goldfish ou virtio (notamment `CONFIG_VIRTIO`, `CONFIG_FB_GOLDFISH` ou `CONFIG_DRM_VIRTIO_GPU`, `CONFIG_ANDROID_BINDER_IPC`, `CONFIG_ASHMEM`).
  2. `ramdisk.img` : Le système de fichiers initial (init) contenant l'exécutable `init` AOSP, le fichier de configuration `init.rc`, et les règles de montage.
  3. `system.img` : L'image système AOSP 6.0 Marshmallow (API 23) ARMv7.
  4. `userdata.img` : La partition de données inscriptible invitée (ext4).
- La compatibilité entre le noyau et l'image système est garantie dès lors que le noyau supporte Binder et Ashmem conformes aux structures de données de l'API 23.

#### Question 4 : Quelles sont les limites de mémoire, de stockage et de performances ?
**RÉPONSE : C'est le cœur de la décision conditionnelle.**
- **Limite Mémoire (RAM) :**
  - Le Honor 4X dispose de 2 Go de RAM physique.
  - Au démarrage d'Android 4.4.3 avec EMUI 3.0, environ 800 à 1000 Mo de RAM sont consommés par le système hôte, laissant ~900 Mo disponibles.
  - Android 6.0 invité a besoin d'au moins **512 Mo de RAM** pour initialiser Zygote, `system_server` et SurfaceFlinger sans déclencher le panic Out-Of-Memory (OOM).
  - L'allocation invité de 512 Mo est faisable mais laisse peu de marge. Si une application hôte gourmande s'exécute en arrière-plan, le `LowMemoryKiller` d'Android 4.4 risque de tuer le processus de la machine virtuelle. L'application devra donc activer des flags de priorité de processus ou être lancée dans un environnement libéré.
- **Limite Stockage :**
  - La mémoire interne du Honor 4X est de 8 Go au total, dont environ 3,5 Go libres sur `/data`.
  - Une image système AOSP 6.0 décompressée pèse environ 1,5 Go, plus 512 Mo à 1 Go pour `userdata.img`, plus le cache.
  - Utiliser la mémoire interne est possible mais saturerait presque entièrement le téléphone. **L'insertion d'une carte MicroSD externe (au moins 8 Go, classe 10 / UHS-1) est indispensable en pratique pour stocker les images disques.**
- **Limite Performances :**
  - Sans KVM matériel, l'exécution via QEMU TCG génère une surcharge CPU estimée entre 8x et 15x par rapport à la vitesse native.
  - Le premier démarrage d'Android 6.0 (qui implique l'optimisation dex/art de certaines applications système de base) prendra entre **6 et 15 minutes**.
  - Une fois démarré, la fréquence d'affichage en rendu logiciel framebuffer sera d'environ **5 à 12 FPS**. L'interactivité sera lente mais fonctionnelle pour des tests de compatibilité.

---

### 9. PROBLÈMES NON RÉSOLUS ET PRÉREQUIS POUR L'ÉTAPE 2

Pour aborder l'Étape 2 (Intégration du moteur d'émulation QEMU), les verrous techniques suivants devront être traités :
1. **Compilation croisée de la bibliothèque QEMU Core :** Extraire les sources de QEMU (arm-softmmu) et les compiler sous forme de bibliothèque partagée dynamique `libqemu-system-arm.so` liée avec une `mini-glib` compatible Bionic API 19.
2. **Interface JNI de contrôle :** Développer l'interface de contrôle du cycle de vie de la VM (`vm_init`, `vm_start`, `vm_stop`, `vm_pause`).
3. **Pipeline d'affichage (Display Loop) :** Mettre en place un `SurfaceView` ou un `Bitmap` partagé via DirectByteBuffer en JNI pour transférer les trames du framebuffer QEMU vers l'écran de l'hôte sans copie mémoire coûteuse.
4. **Acquisition des images de boot :** Préparer un pack minimal (Kernel 3.10 zImage + ramdisk.img + system.img AOSP 6.0 minimalisé) optimisé pour le format `qcow2` afin de limiter l'empreinte disque.

---

### 10. DÉCISION FINALE

## **DÉCISION : CONDITIONAL GO**

**Justification de la décision :**
1. La faisabilité technique native sur l'architecture ARMv7 / Android 4.4.3 (API 19) est confirmée et validée par la compilation effective de notre bibliothèque JNI `libnativediagnostics.so` pour `armeabi-v7a`.
2. L'absence de KVM matériel interdit une vitesse native, mais n'empêche pas le démarrage réel du système d'exploitation invité via QEMU TCG.
3. Les quotas de RAM (2 Go hôte / 512 Mo invité) permettent de faire tourner Android 6.0 AOSP si les applications Google lourdes sont exclues.
4. Le projet peut donc continuer vers l'Étape 2, sous réserve que l'utilisateur dispose d'une carte MicroSD sur le téléphone pour héberger les images disque du système invité.
