// language: C++, file: deebsofting_full.cpp, target: Windows x64, MSVC
#define _USE_MATH_DEFINES
#include <windows.h>
#include <tlhelp32.h>
#include <gdiplus.h>
#include <cmath>
#include <string>
#include <thread>
#include <chrono>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// --- ОФФСЕТЫ (ЗАМЕНИТЬ НА АКТУАЛЬНЫЕ ИЗ CS2-DUMPER) ---
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
// --------------------------------------------------------

struct Vec3 { float x, y, z; };
struct Vec2 { float x, y; };
struct QAngle { float pitch, yaw, roll; };

static HANDLE g_proc = nullptr;
static uintptr_t g_client = 0;
static bool g_esp = true;
static bool g_menuOpen = true;
static bool g_running = true;

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

void DrawESP(HDC hdc, int sw, int sh) {
    uintptr_t localPawn = 0; 
    if (!Read(g_client + offsets::dwLocalPlayerPawn, localPawn) || !localPawn) return;
    
    int myTeam = 0; 
    if (!Read(localPawn + schema::C_BaseEntity::m_iTeamNum, myTeam)) return;
    
    uintptr_t myNode = 0; 
    if (!Read(localPawn + schema::C_BaseEntity::m_pGameSceneNode, myNode) || !myNode) return;
    
    Vec3 myPos{}; 
    if (!Read(myNode + schema::C_GameSceneNode::m_vecAbsOrigin, myPos)) return;

    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    FontFamily fontFamily(L"Consolas");
    Font textFont(&fontFamily, 14, FontStyleBold, UnitPixel);

    for (int i = 1; i < 64; i++) {
        Entity e{}; 
        if (!GetEntity(i, e)) continue;
        if (e.health <= 0 || e.health > 100 || e.pawn == localPawn || e.team == myTeam) continue;
        
        Vec2 head{}, feet{};
        Vec3 hp = e.origin; hp.z += 72.0f;
        if (!WorldToScreen(hp, head, sw, sh) || !WorldToScreen(e.origin, feet, sw, sh)) continue;
        
        int h = (int)(feet.y - head.y); 
        int w = h / 2; 
        int x = (int)head.x - w / 2; 
        int y = (int)head.y;

        // Расчет дистанции
        float dx = e.origin.x - myPos.x;
        float dy = e.origin.y - myPos.y;
        float dz = e.origin.z - myPos.z;
        float dist_m = sqrtf(dx*dx + dy*dy + dz*dz) / 52.0f; // 1 unit ~ 1.9cm

        // Обводка (Glow)
        Color glowColor(150, 255, 0, 0); 
        Color solidColor(255, 255, 0, 0); 
        
        Pen glowPen(glowColor, 4.0f);
        graphics.DrawRectangle(&glowPen, x, y, w, h);
        
        Pen solidPen(solidColor, 1.5f);
        graphics.DrawRectangle(&solidPen, x, y, w, h);

        // HP и Дистанция
        SolidBrush healthBrush(Color(255, 255, 255, 255));
        wchar_t hpBuf[16];
        swprintf_s(hpBuf, L"%d HP", e.health);
        graphics.DrawString(hpBuf, -1, &textFont, PointF((REAL)x, (REAL)(y - 20)), &healthBrush);

        SolidBrush distBrush(Color(255, 200, 200, 200));
        wchar_t distBuf[16];
        swprintf_s(distBuf, L"[%.0fm]", dist_m); 
        RectF bounds;
        graphics.MeasureString(distBuf, -1, &textFont, PointF(0,0), &bounds);
        graphics.DrawString(distBuf, -1, &textFont, PointF((REAL)(x + w - bounds.Width), (REAL)(y - 20)), &distBrush);
    }
}

void DrawMenu(HDC hdc, int sw, int sh) {
    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    
    int mw = 400, mh = 250;
    int mx = (sw - mw) / 2, my = (sh - mh) / 2;

    SolidBrush bgBrush(Color(240, 14, 14, 18));
    graphics.FillRectangle(&bgBrush, mx, my, mw, mh);

    Pen borderPen(Color(255, 255, 0, 0), 2.0f);
    graphics.DrawRectangle(&borderPen, mx, my, mw, mh);

    FontFamily fontFamily(L"Segoe UI");
    Font titleFont(&fontFamily, 20, FontStyleBold, UnitPixel);
    Font textFont(&fontFamily, 14, FontStyleRegular, UnitPixel);

    SolidBrush titleBrush(Color(255, 255, 0, 0));
    SolidBrush textBrush(Color(255, 230, 230, 235));

    graphics.DrawString(L"deebsofting", -1, &titleFont, PointF((REAL)(mx + 20), (REAL)(my + 15)), &titleBrush);
    
    std::wstring espState = L"ESP: " + std::wstring(g_esp ? L"ON" : L"OFF") + L" (F8)";
    graphics.DrawString(espState.c_str(), -1, &textFont, PointF((REAL)(mx + 20), (REAL)(my + 70)), &textBrush);

    graphics.DrawString(L"F12 - Exit", -1, &textFont, PointF((REAL)(mx + 20), (REAL)(my + mh - 30)), &textBrush);
}

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    // Инициализация GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    DWORD pid = FindPid(L"cs2.exe");
if (!pid) { MessageBoxW(0, L"Start CS2 first!", L"deebsofting", MB_ICONERROR); return 1; }
    g_proc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!g_proc) { MessageBoxW(0, L"Запусти от админа!", L"deebsofting", MB_ICONERROR); return 2; }
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

    while (g_running) {
        // Выход по F12, чтобы наверняка
        if (GetAsyncKeyState(VK_F12) & 1) g_running = false;
        if (GetAsyncKeyState(VK_DELETE) & 1) g_menuOpen = !g_menuOpen;
        if (GetAsyncKeyState(VK_F8) & 1) g_esp = !g_esp;

        // Очистка фона
        PatBlt(mem, 0, 0, sw, sh, BLACKNESS);
        
        // Рисуем
        if (g_esp) DrawESP(mem, sw, sh);
        if (g_menuOpen) DrawMenu(mem, sw, sh);
        
        // Выводим на экран
        BitBlt(screen, 0, 0, sw, sh, mem, 0, 0, SRCCOPY);

        // ВАЖНО: Слип, чтобы прога не жрала 100% ЦП и не лагала
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // Очистка (если вышли из цикла)
    SelectObject(mem, old); DeleteObject(bmp); DeleteDC(mem); ReleaseDC(0, screen);
    GdiplusShutdown(gdiplusToken);
    return 0;
}
