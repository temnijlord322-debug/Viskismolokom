// language: C++, file: menu.cpp — вставить в проект cs2_cheat
// GDI menu, toggled by VK_DELETE, dark theme, logo drawn procedurally

static bool g_menuOpen = false;

// ---- palette ----
static constexpr COLORREF CLR_BG      = RGB(14, 14, 18);
static constexpr COLORREF CLR_PANEL   = RGB(22, 22, 28);
static constexpr COLORREF CLR_ACCENT  = RGB(0, 200, 255);
static constexpr COLORREF CLR_TEXT    = RGB(230, 230, 235);
static constexpr COLORREF CLR_SUB     = RGB(120, 120, 130);
static constexpr COLORREF CLR_BORDER  = RGB(40, 40, 50);

// ---- logo: "d" in hexagon, drawn with lines ----
static void DrawLogo(HDC dc, int cx, int cy, int r, COLORREF c) {
    HPEN pen = CreatePen(PS_SOLID, 2, c);
    HGDIOBJ old = SelectObject(dc, pen);
    HBRUSH nullb = (HBRUSH)GetStockObject(NULL_BRUSH);
    SelectObject(dc, nullb);

    // hexagon
    POINT pts[6];
    for (int i = 0; i < 6; i++) {
        double a = 3.14159265 / 3.0 * i - 3.14159265 / 6.0;
        pts[i].x = cx + (int)(r * cos(a));
        pts[i].y = cy + (int)(r * sin(a));
    }
    Polygon(dc, pts, 6);

    // inner "d" — vertical bar + bowl
    Arc(dc, cx - r/2, cy - r/2, cx + r/2, cy + r/2, cx + r/2, cy, cx, cy - r/2);
    MoveToEx(dc, cx + r/3, cy - r/2, nullptr);
    LineTo(dc, cx + r/3, cy + r/2);

    SelectObject(dc, old);
    DeleteObject(pen);
}

// ---- menu draw ----
static void DrawMenu(HDC dc, int sw, int sh) {
    const int mw = 520, mh = 380;
    const int mx = (sw - mw) / 2, my = (sh - mh) / 2;

    // shadow
    HBRUSH shadow = CreateSolidBrush(RGB(0, 0, 0));
    RECT sr{ mx - 8, my - 8, mx + mw + 8, my + mh + 8 };
    FillRect(dc, &sr, shadow);
    DeleteObject(shadow);

    // main bg
    HBRUSH bg = CreateSolidBrush(CLR_BG);
    RECT mr{ mx, my, mx + mw, my + mh };
    FillRect(dc, &mr, bg);
    DeleteObject(bg);

    // border
    HPEN bp = CreatePen(PS_SOLID, 1, CLR_BORDER);
    HGDIOBJ oldp = SelectObject(dc, bp);
    HBRUSH nullb = (HBRUSH)GetStockObject(NULL_BRUSH);
    SelectObject(dc, nullb);
    Rectangle(dc, mx, my, mx + mw, my + mh);
    SelectObject(dc, oldp);
    DeleteObject(bp);

    // header
    HBRUSH hb = CreateSolidBrush(CLR_PANEL);
    RECT hr{ mx, my, mx + mw, my + 56 };
    FillRect(dc, &hr, hb);
    DeleteObject(hb);

    // logo
    DrawLogo(dc, mx + 32, my + 28, 16, CLR_ACCENT);

    // title
    SetBkMode(dc, TRANSPARENT);
    HFONT fnt = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HGDIOBJ oldf = SelectObject(dc, fnt);
    SetTextColor(dc, CLR_ACCENT);
    TextOutW(dc, mx + 58, my + 16, L"deebsofting", 11);

    // subtitle
    HFONT fnt2 = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(dc, fnt2);
    SetTextColor(dc, CLR_SUB);
    TextOutW(dc, mx + 58, my + 40, L"cs2 external", 12);

    // accent line under header
    HPEN ap = CreatePen(PS_SOLID, 2, CLR_ACCENT);
    SelectObject(dc, ap);
    MoveToEx(dc, mx, my + 56, nullptr);
    LineTo(dc, mx + mw, my + 56);

    // ---- menu items ----
    SelectObject(dc, fnt2);
    const wchar_t* items[] = {
        L"ESP",
        L"Aimbot",
        L"Auto-strafe",
        L"Bhop",
        L"Anti-aim",
    };
    const bool* states[] = { &g_esp, &g_aim, &g_autostrafe, &g_bhop, nullptr };

    int y = my + 80;
    for (int i = 0; i < 5; i++) {
        // item bg
        HBRUSH ib = CreateSolidBrush(CLR_PANEL);
        RECT ir{ mx + 20, y, mx + mw - 20, y + 44 };
        FillRect(dc, &ir, ib);
        DeleteObject(ib);

        // label
        SetTextColor(dc, CLR_TEXT);
        TextOutW(dc, mx + 36, y + 14, items[i], (int)wcslen(items[i]));

        // toggle
        if (states[i]) {
            HBRUSH tb = CreateSolidBrush(*states[i] ? CLR_ACCENT : CLR_BORDER);
            RECT tr{ mx + mw - 80, y + 12, mx + mw - 36, y + 32 };
            FillRect(dc, &tr, tb);
            DeleteObject(tb);
            SetTextColor(dc, CLR_BG);
            TextOutW(dc, mx + mw - 72, y + 14, *states[i] ? L"ON" : L"OFF", 3);
        } else {
            SetTextColor(dc, CLR_SUB);
            TextOutW(dc, mx + mw - 80, y + 14, L"N/A", 3);
        }

        y += 56;
    }

    // footer
    SetTextColor(dc, CLR_SUB);
    TextOutW(dc, mx + 20, my + mh - 24, L"Delete — close   F8 ESP   F9 Aim", 34);

    SelectObject(dc, oldf);
    DeleteObject(fnt);
    DeleteObject(fnt2);
    DeleteObject(ap);
}
