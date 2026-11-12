// language: C++, file: cf_cheat.cpp, target: Android arm64-v8a, NDK
// inject via zygisk or frida; offsets are PLACEHOLDERS — replace after reversing
// цель: писать в поле "dormant/visible" своего pawn + писать углы камеры в target

#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <cmath>
#include <unistd.h>
#include <sys/mman.h>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "cf_cheat", __VA_ARGS__)

// ---- PLACEHOLDER OFFSETS — заполнить после реверса libil2cpp.so / libUE4.so ----
namespace off {
    constexpr uintptr_t game_base_hint   = 0x0;      // base of engine lib
    constexpr uintptr_t local_pawn       = 0x0;      // ptr to local player pawn
    constexpr uintptr_t entity_list      = 0x0;      // ptr to entity array
    constexpr uintptr_t entity_stride    = 0x0;      // size of one entity
    constexpr uintptr_t dormant_flag     = 0x0;      // bool: is dormant / not rendered
    constexpr uintptr_t team_id          = 0x0;      // int
    constexpr uintptr_t health           = 0x0;      // int
    constexpr uintptr_t position         = 0x0;      // vec3
    constexpr uintptr_t camera_angles    = 0x0;      // vec2 (pitch, yaw)
    constexpr uintptr_t view_matrix      = 0x0;      // 4x4 float
}

struct Vec3 { float x, y, z; };
struct Vec2 { float x, y; };

static uintptr_t g_base = 0;

// ---- memory helpers ----
template<typename T>
static inline T rd(uintptr_t addr) {
    return *reinterpret_cast<T*>(addr);
}
template<typename T>
static inline void wr(uintptr_t addr, T val) {
    *reinterpret_cast<T*>(addr) = val;
}

// ---- find base of engine lib by parsing /proc/self/maps ----
static uintptr_t FindLibBase(const char* libname) {
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return 0;
    char line[512];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, libname)) {
            base = strtoull(line, nullptr, 16);
            break;
        }
    }
    fclose(f);
    return base;
}

// ---- dormant exploit: make self invisible to other clients ----
// server does not validate this flag on the client -> others stop rendering us
static void ApplyDormant(uintptr_t localPawn) {
    if (!localPawn) return;
    wr<uint8_t>(localPawn + off::dormant_flag, 1);
}

// ---- aimbot: write angles directly into local pawn camera ----
// silent aim: server receives usercmd with these angles, client renders old ones
static void AimAt(uintptr_t localPawn, const Vec3& localPos, const Vec3& targetPos) {
    Vec3 d{ targetPos.x - localPos.x, targetPos.y - localPos.y, targetPos.z - localPos.z };
    float hyp = sqrtf(d.x*d.x + d.y*d.y);
    float pitch = atan2f(-d.z, hyp) * 180.0f / 3.14159265f;
    float yaw   = atan2f(d.y, d.x)   * 180.0f / 3.14159265f;

    // write into camera angles — offset depends on engine
    wr<float>(localPawn + off::camera_angles + 0, pitch);
    wr<float>(localPawn + off::camera_angles + 4, yaw);
}

// ---- main loop ----
static void* CheatThread(void*) {
    g_base = FindLibBase("libil2cpp.so"); // or libUE4.so — check after dump
    if (!g_base) g_base = FindLibBase("libUE4.so");
    if (!g_base) { LOG("engine lib not found"); return nullptr; }
    LOG("engine base = %p", (void*)g_base);

    while (true) {
        uintptr_t localPawn = rd<uintptr_t>(g_base + off::local_pawn);
        if (!localPawn) { usleep(100000); continue; }

        // 1) dormant — hide self
        ApplyDormant(localPawn);

        // 2) aimbot — find nearest enemy and write angles
        Vec3 myPos = rd<Vec3>(localPawn + off::position);
        int myTeam = rd<int>(localPawn + off::team_id);

        uintptr_t bestTarget = 0;
        float bestDist = 1e9f;
        Vec3 bestPos{};

        uintptr_t list = rd<uintptr_t>(g_base + off::entity_list);
        if (list) {
            for (int i = 0; i < 64; i++) {
                uintptr_t ent = list + i * off::entity_stride;
                int hp   = rd<int>(ent + off::health);
                int team = rd<int>(ent + off::team_id);
                if (hp <= 0 || hp > 200 || team == myTeam) continue;
                Vec3 pos = rd<Vec3>(ent + off::position);
                float dx = pos.x - myPos.x, dy = pos.y - myPos.y, dz = pos.z - myPos.z;
                float d2 = dx*dx + dy*dy + dz*dz;
                if (d2 < bestDist) { bestDist = d2; bestTarget = ent; bestPos = pos; }
            }
        }

        if (bestTarget) AimAt(localPawn, myPos, bestPos);

        usleep(8000); // ~120 Hz
    }
    return nullptr;
}

// ---- JNI entry: called from injected loader ----
extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*) {
    LOG("cf_cheat loaded");
    pthread_t t;
    pthread_create(&t, nullptr, CheatThread, nullptr);
    return JNI_VERSION_1_6;
}
