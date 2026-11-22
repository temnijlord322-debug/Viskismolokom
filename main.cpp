// language: C++, file: cs2_cheat.cpp, target: Windows 11 x64, MSVC, C++20
// full build: ESP, Aimbot, Bhop, Autostrafe, GDI Menu (Delete)
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <cmath>
#include <cstdio>
#include <string>
#include <thread>
#include <chrono>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// --- ЗАГЛУШКИ ОФФСЕТОВ ---
// ВНИМАНИЕ: Это вымышленные оффсеты. Замени их на реальные из cs2-dumper.
// Без реальных оффсетов ESP и Aimbot будут читать мусор.
namespace offsets {
    constexpr uintptr_t dwEntityList = 0x18C7B28;
    constexpr uintptr_t dwLocalPlayerPawn = 0x1873A20;
    constexpr uintptr_t dwViewMatrix = 0x194A5C0;
    constexpr uintptr_t dwViewAngles = 0x194AAB8;
}
namespace schema {
    namespace C_BaseEntity {
        constexpr uintptr_t m_iTeamNum = 0x3C8;
        constexpr uintptr_t m_iHealth = 0x344;
        constexpr uintptr_t m_pGameSceneNode = 0x328;
        constexpr uintptr_t m_fFlags = 0x3D4;
    }
    namespace C_GameSceneNode {
        constexpr uintptr_t m_vecAbsOrigin = 0xC8;
        constexpr uintptr_t m_vecAbsVelocity = 0xCC; 
    }
    namespace CCSPlayerController {
        constexpr uintptr_t m_hPlayerPawn = 0x7BC;
    }
}
// -------------------------

struct Vec3 { float x, y, z; };
struct Vec2 { float x, y; };
struct QAngle { float pitch, yaw, roll; };

static HANDLE g_proc = nullptr;
static uintptr_t g_client = 0;

// Глобальные состояния (для меню и функций)
static bool g_esp = true;
static bool g_aim = false;
static bool g_bhop = false;
static bool g_autostrafe = false;
static bool g_menuOpen = true;

template<typename T> bool Read(uintptr_t addr, T& out) {
    return ReadProcessMemory(g_proc, (LPCVOID)addr, &out, sizeof(T), nullptr) != 0;
}

DWORD FindPid(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe{ sizeof(pe) }; DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) do {
        if (!_wcsicmp(pe.szExeFile, name)) { pid = pe.th32ProcessID; break; }
    } while (Process32NextW(snap, &pe));
    CloseHandle(snap); return pid;
}

uintptr_t ModuleBase(DWORD pid, const wchar_t* mod) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    MODULEENTRY32W me{ sizeof(me) }; uintptr_t base = 0;
    if (Module32FirstW(snap, &me)) do {
        if (!_wcsicmp(me.szModule, mod)) { base = (uintptr_t)me.modBaseAddr; break; }
    } while (Module32NextW(snap, &me));
    CloseHandle(snap); return base;
}

struct Entity {
    uintptr_t pawn = 0;
    int team = 0;
    int health = 0;
    Vec3 origin{};
};

bool GetEntity(int index, Entity& e) {
    uintptr_t list = 0;
    if (!Read(g_client + offsets::dwEntityList, list) || !list) return false;
    uintptr_t chunk = 0;
    if (!Read(list + 0x8 * (index >> 9) + 0x10, chunk) || !chunk) return false;
    uintptr_t identity = 0;
    if (!Read(chunk + 0x78 * (index & 0x1FF), identity) || !identity) return false;
    uintptr_t controller = 0;
    if (!Read(identity + 0x10, controller) || !controller) return false;
    if (!Read(controller + schema::CCSPlayerController::m_hPlayerPawn, e.pawn) || !e.pawn) return false;
    if (!Read(e.pawn + schema::C_BaseEntity::m_iTeamNum, e.team)) return false;
    if (!Read(e.pawn + schema::C_BaseEntity::m_iHealth, e.health)) return false;
    uintptr_t node = 0;
    if (!Read(e.pawn + schema::C_BaseEntity::m_pGameSceneNode, node) || !node) return false;
    if (!Read(node + schema::C_GameSceneNode::m_vecAbsOrigin, e.origin)) return false;
    return true;
}

bool WorldToScreen(const Vec3& w, Vec2& s, int ww, int wh) {
    float m[16];
    if (!Read(g_client + offsets::dwViewMatrix, m)) return false;
    float clipX = w.x*m[0] + w.y*m[1] + w.z*m[2]  + m[3];
    float clipY = w.x*m[4] + w.y*m[5] + w.z*m[6]  + m[7];
    float clipW = w.x*m[12]+ w.y*m[13]+ w.z*m[14] + m[15];
    if (clipW < 0.01f) return false;
    float ndcX = clipX / clipW, ndcY = clipY / clipW;
    s.x = (ww * 0.5f) * (ndcX + 1.0f);
    s.y = (wh * 0.5f) * (1.0f - ndcY);
    return true;
}

QAngle CalcAngle(const Vec3& src, const Vec3& dst) {
    Vec3 d{ dst.x - src.x, dst.y - src.y, dst.z - src.z };
    float hyp = sqrtf(d.x*d.x + d.y*d.y);
    QAngle a{};
    a.pitch = atan2f(-d.z, hyp) * 180.0f / 3.14159265f;
    a.yaw   = atan2f(d.y, d.x)   * 180.0f / 3.14159265f;
    return a;
}

void DoBhop() {
    uintptr_t localPawn = 0;
    if (!Read(g_client + offsets::dwLocalPlayerPawn, localPawn) || !localPawn) return;
    uint32_t flags = 0;
    Read(localPawn + schema::C_BaseEntity::m_fFlags, flags);
    if (flags & 0x1) { // FL_ONGROUND
        INPUT in{}; in.type = INPUT_KEYBOARD; in.ki.wVk = VK_SPACE;
        SendInput(1, &in, sizeof(in));
        in.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &in, sizeof(in));
    }
}

void DoAutoStrafe() {
    uintptr_t localPawn = 0;
    if (!Read(g_client + offsets::dwLocalPlayerPawn, localPawn) || !localPawn) return;
    uint32_t flags = 0; Read(localPawn + schema::C_BaseEntity::m_fFlags, flags);
    if (flags & 0x1) return; // not in air
    
    uintptr_t node = 0; Read(localPawn + schema::C_BaseEntity::m_pGameSceneNode, node);
    if (!node) return;
    Vec3 vel{}; Read(node + schema::C_GameSceneNode::m_vecAbsVelocity, vel);
    float hspeed = sqrtf(vel.x*vel.x + vel.y*vel.y);
    if (hspeed < 5.0f) return;

    QAngle cur{}; Read(g_client + offsets::dwViewAngles, cur);
    float velYaw = atan2f(vel.y, vel.x) * 180.0f / 3.14159265f;
    float delta = cur.yaw - velYaw;
    while (delta > 180.0f) delta -= 360.0f;
    while (delta < -180.0f) delta += 360.0f;

    float turnDir = (delta >= 0.0f) ? 1.0f : -1.0f;
    float turnAmount = fabsf(delta) - 30.0f;
    if (fabsf(turnAmount) > 0.5f) {
        INPUT mi{}; mi.type = INPUT_MOUSE; mi.mi.dx = (LONG)((turnDir * turnAmount) / 0.022f);
        mi.mi.dwFlags = MOUSEEVENTF_MOVE; SendInput(1, &mi, sizeof(mi));
    }
    WORD key = (delta > 0.0f) ? 'D' : 'A';
    INPUT ki{}; ki.type = INPUT_KEYBOARD; ki.ki.wVk = key; SendInput(1, &ki, sizeof(ki));
    ki.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &ki, sizeof(ki));
}

void DoAimbot() {
    uintptr_t localPawn = 0;
    if (!Read(g_client + offsets::dwLocalPlayerPawn, localPawn) || !localPawn) return;
    int myTeam = 0; Read(localPawn + schema::C_BaseEntity::m_iTeamNum, myTeam);
    uintptr_t node = 0; Read(localPawn + schema::C_BaseEntity::m_pGameSceneNode, node);
    if (!node) return;
    Vec3 myPos{}; Read(node + schema::C_GameSceneNode::m_vecAbsOrigin, myPos);

    uintptr_t best = 0; float bestD = 1e9f; Vec3 bestPos{};
    for (int i = 1; i < 64; i++) {
        Entity e{}; if (!GetEntity(i, e)) continue;
        if (e.health <= 0 || e.health > 100 || e.team == myTeam || e.pawn == localPawn) continue;
        Vec3 hpos = e.origin; hpos.z += 64.0f;
        float dx = hpos.x - myPos.x, dy = hpos.y - myPos.y, dz = hpos.z - myPos.z;
        float d2 = dx*dx + dy*dy + dz*dz;
        if (d2 < bestD) { bestD = d2; best = e.pawn; bestPos = hpos; }
    }
    if (!best) return;
    QAngle want = CalcAngle(myPos, bestPos);
    QAngle cur{}; Read(g_client + offsets::dwViewAngles, cur);
    float dPitch = want.pitch - cur.pitch, dYaw = want.yaw - cur.yaw;
    while (dYaw > 180.0f) dYaw -= 360.0f; while (dYaw < -180.0f) dYaw += 360.0f;
    
    INPUT in{}; in.type = INPUT_MOUSE;
    in.mi.dx = (LONG)((dYaw / 3.0f) / 0.022f); in.mi.dy = (LONG)((dPitch / 3.0f) / 0.022f);
    in.mi.dwFlags = MOUSEEVENTF_MOVE; SendInput(1, &in, sizeof(in));
}

void DrawESP(HDC dc, int sw, int sh) {
    uintptr_t localPawn = 0; Read(g_client + offsets::dwLocalPlayerPawn, localPawn);
    int myTeam = 0; if (localPawn) Read(localPawn + schema::C_BaseEntity::m_iTeamNum, myTeam);

    for (int i = 1; i < 64; i++) {
        Entity e{}; if (!GetEntity(i, e)) continue;
        if (e.health <= 0 || e.health > 100 || e.pawn == localPawn || e.team == myTeam) continue;
        Vec2 head{}, feet{};
        Vec3 hp = e.origin; hp.z += 72.0f;
        if (!WorldToScreen(hp, head, sw, sh) || !WorldToScreen(e.origin, feet, sw, sh)) continue;
        int h = (int)(feet.y - head.y); int w = h / 2; int x = (int)head.x - w / 2; int y = (int)head.y;

        HPEN pen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
        HGDIOBJ old = SelectObject(dc, pen);
        SelectObject(dc, GetStockObject(NULL_BRUSH));
        Rectangle(dc, x, y, x + w, y + h);
        SelectObject(dc, old); DeleteObject(pen);
    }
}

// === МЕНЮ ===
static constexpr COLORREF CLR_BG = RGB(14, 14, 18);
static constexpr COLORREF CLR_PANEL = RGB(22, 22, 28);
static constexpr COLORREF CLR_ACCENT = RGB(0, 200, 255);
static constexpr COLORREF CLR_TEXT = RGB(230, 230, 235);
static constexpr COLORREF CLR_SUB = RGB(120, 120, 130);
static constexpr COLORREF CLR_BORDER = RGB(40, 40, 50);

void DrawLogo(HDC dc, int cx, int cy, int r, COLORREF c) {
    HPEN pen = CreatePen(PS_SOLID, 2, c); HGDIOBJ old = SelectObject(dc, pen);
    SelectObject(dc, GetStockObject(NULL_BRUSH));
    POINT pts[6]; for (int i = 0; i < 6; i++) {
        double a = 3.14159265 / 3.0 * i - 3.14159265 / 6.0;
        pts[i].x = cx + (int)(r * cos(a)); pts[i].y = cy + (int)(r * sin(a));
    }
    Polygon(dc, pts, 6);
    Arc(dc, cx - r/2, cy - r/2, cx + r/2, cy + r/2, cx + r/2, cy, cx, cy - r/2);
    MoveToEx(dc, cx + r/3, cy - r/2, nullptr); LineTo(dc, cx + r/3, cy + r/2);
    SelectObject(dc, old); DeleteObject(pen);
}

void DrawMenu(HDC dc, int sw, int sh) {
    int mw = 520, mh = 380, mx = (sw - mw) / 2, my = (sh - mh) / 2;
    HBRUSH b;
    b = CreateSolidBrush(RGB(0,0,0)); RECT sr{mx-8,my-8,mx+mw+8,my+mh+8}; FillRect(dc,&sr,b); DeleteObject(b);
    b = CreateSolidBrush(CLR_BG); RECT mr{mx,my,mx+mw,my+mh}; FillRect(dc,&mr,b); DeleteObject(b);
    b = CreateSolidBrush(CLR_PANEL); RECT hr{mx,my,mx+mw,my+56}; FillRect(dc,&hr,b); DeleteObject(b);

    HPEN p = CreatePen(PS_SOLID, 1, CLR_BORDER); HGDIOBJ op = SelectObject(dc, p);
    SelectObject(dc, GetStockObject(NULL_BRUSH)); Rectangle(dc, mx, my, mx+mw, my+mh);
    SelectObject(dc, op); DeleteObject(p);

    DrawLogo(dc, mx+32, my+28, 16, CLR_ACCENT);
    SetBkMode(dc, TRANSPARENT);
    
    HFONT f1 = CreateFontW(22,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    HGDIOBJ of = SelectObject(dc, f1);
    SetTextColor(dc, CLR_ACCENT); TextOutW(dc, mx+58, my+16, L"deebsofting", 11);

    HFONT f2 = CreateFontW(14,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    SelectObject(dc, f2);
    SetTextColor(dc, CLR_SUB); TextOutW(dc, mx+58, my+40, L"cs2 external", 12);

    p = CreatePen(PS_SOLID, 2, CLR_ACCENT); SelectObject(dc, p);
    MoveToEx(dc, mx, my+56, nullptr); LineTo(dc, mx+mw, my+56); DeleteObject(p);

    SelectObject(dc, f2);
    const wchar_t* items[] = { L"ESP", L"Aimbot", L"Auto-strafe", L"Bhop" };
    bool* states[] = { &g_esp, &g_aim, &g_autostrafe, &g_bhop };

    int y = my + 80;
    for (int i=0; i<4; i++) {
        b = CreateSolidBrush(CLR_PANEL); RECT ir{mx+20, y, mx+mw-20, y+44}; FillRect(dc,&ir,b); DeleteObject(b);
        SetTextColor(dc, CLR_TEXT); TextOutW(dc, mx+36, y+14, items[i], (int)wcslen(items[i]));
        
        b = CreateSolidBrush(*states[i] ? CLR_ACCENT : CLR_BORDER);
        RECT tr{mx+mw-80, y+12, mx+mw-36, y+32}; FillRect(dc, &tr, b); DeleteObject(b);
        SetTextColor(dc, *states[i] ? CLR_BG : CLR_SUB);
        TextOutW(dc, mx+mw-72, y+14, *states[i] ? L"ON" : L"OFF", 3);
        y += 56;
    }
    SetTextColor(dc, CLR_SUB); TextOutW(dc, mx+20, my+mh-24, L"Delete — menu   F8 ESP   F9 Aim", 32);
    SelectObject(dc, of); DeleteObject(f1); DeleteObject(f2);
}

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    DWORD pid = FindPid(L"cs2.exe");
    if (!pid) { MessageBoxW(0, L"cs2.exe not found", L"deebsofting", MB_ICONERROR); return 1; }
    g_proc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!g_proc) { MessageBoxW(0, L"OpenProcess failed. Run as admin.", L"deebsofting", MB_ICONERROR); return 2; }
    g_client = ModuleBase(pid, L"client.dll");
    if (!g_client) return 3;

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = WndProc; wc.hInstance = hInst; wc.lpszClassName = L"cf_overlay";
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOPMOST|WS_EX_TOOLWINDOW, 
        L"cf_overlay", L"", WS_POPUP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0, 0, hInst, 0);
    
    SetLayeredWindowAttributes(hwnd, RGB(0,0,0), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_SHOW);

    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    HDC screen = GetDC(0), mem = CreateCompatibleDC(screen);
    HBITMAP bmp = CreateCompatibleBitmap(screen, sw, sh);
    HGDIOBJ old = SelectObject(mem, bmp);

    while (true) {
        if (GetAsyncKeyState(VK_DELETE) & 1) g_menuOpen = !g_menuOpen;
        if (GetAsyncKeyState(VK_F8) & 1) g_esp = !g_esp;
        if (GetAsyncKeyState(VK_F9) & 1) g_aim = !g_aim;
        if (GetAsyncKeyState(VK_END) & 1) break;

        if (g_bhop) DoBhop();
        if (g_autostrafe) DoAutoStrafe();
        if (g_aim && (GetAsyncKeyState(VK_XBUTTON2) & 0x8000)) DoAimbot();

        PatBlt(mem, 0, 0, sw, sh, BLACKNESS);
        if (g_esp) DrawESP(mem, sw, sh);
        if (g_menuOpen) DrawMenu(mem, sw, sh);
        BitBlt(screen, 0, 0, sw, sh, mem, 0, 0, SRCCOPY);

        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }

    SelectObject(mem, old); DeleteObject(bmp); DeleteDC(mem); ReleaseDC(0, screen);
    return 0;
}
