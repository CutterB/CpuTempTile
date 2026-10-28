#include <windows.h>
#include <stdio.h>
#include <wchar.h>

#include "coretemp.h"

typedef long (*SetContentString_fn)(void *el, const wchar_t *content);
typedef long (*SetClass_fn)(void *el, const wchar_t *cls);
typedef long (*Add_fn)(void *parent, void *child);
typedef long (*Insert_fn)(void *parent, void *child, unsigned int index);
typedef long (*ElementCreate_fn)(unsigned int dType, void *parent, void *cookie, void **out);
typedef const wchar_t *(*GetClass_fn)(void *el, void **outValue);
typedef void *(*GetParent_fn)(void *el);
typedef long (*SetForegroundColor_fn)(void *el, unsigned int color);
typedef long (*SetLayout_fn)(void *el, void *layout);
typedef long (*FillLayoutCreate_fn)(void **outLayout);
typedef long (*SetPadding_fn)(void *el, int l, int t, int r, int b);
typedef int (*GetFontSize_fn)(void *el);
typedef long (*SetFontSize_fn)(void *el, int size);

static SetContentString_fn   g_origSetContent = NULL;
static SetClass_fn           g_SetClass = NULL;
static Add_fn                g_Add = NULL;
static Insert_fn             g_Insert = NULL;
static ElementCreate_fn      g_ElementCreate = NULL;
static GetClass_fn           g_GetClass = NULL;
static GetParent_fn          g_GetParent = NULL;
static SetForegroundColor_fn g_SetForegroundColor = NULL;
static SetLayout_fn        g_SetLayout = NULL;
static FillLayoutCreate_fn g_FillLayoutCreate = NULL;
static SetPadding_fn       g_SetPadding = NULL;
static GetFontSize_fn      g_GetFontSize = NULL;
static SetFontSize_fn      g_SetFontSize = NULL;

static void *g_valueTile = NULL;
static volatile LONG g_tileInjected = 0;
static DWORD g_lastUpdate = 0;

static void log_write(const char *s)
{
    FILE *f = fopen("C:\\Users\\cutter\\Documents\\Projects\\taskmgr add CPU temp\\reverse\\tile.log", "a");
    if (f) { fputs(s, f); fputs("\r\n", f); fclose(f); }
}

static void update_value(void)
{
    if (!g_valueTile || !g_origSetContent) return;
    float t = 0.0f;
    if (coretemp_read_average(&t) != 0) return;
    wchar_t buf[64];
    swprintf(buf, sizeof(buf) / sizeof(buf[0]), L"%.0f \u00B0C", t);
    g_origSetContent(g_valueTile, buf);
}

static void inject_tile(void *baseSpeedValue)
{
    void *tile = g_GetParent(baseSpeedValue);       /* base-speed tile (label+value) */
    void *grid = tile ? g_GetParent(tile) : NULL;   /* the static-tile container */
    char line[256];
    snprintf(line, sizeof(line), "inject: val=%p tile=%p grid=%p", baseSpeedValue, tile, grid);
    log_write(line);

    void *cell = NULL, *lbl = NULL, *val = NULL, *layout = NULL;
    long hc = g_ElementCreate(0, NULL, NULL, &cell);
    long hl = g_ElementCreate(0, NULL, NULL, &lbl);
    long hv = g_ElementCreate(0, NULL, NULL, &val);
    long hLay = g_FillLayoutCreate ? g_FillLayoutCreate(&layout) : -1;

    if (cell && layout && g_SetLayout) g_SetLayout(cell, layout);

    if (lbl && g_SetClass) g_SetClass(lbl, L"staticData");
    if (val && g_SetClass) g_SetClass(val, L"dynamicData");
    if (val && g_SetForegroundColor) g_SetForegroundColor(val, 0x00FFFFFF);

    if (lbl && g_origSetContent) g_origSetContent(lbl, L"Temperature:");
    if (val && g_origSetContent) g_origSetContent(val, L"0 \u00B0C");

    if (cell && lbl && g_Add) g_Add(cell, lbl);
    if (cell && val && g_Add) g_Add(cell, val);

    if (grid && cell && g_Insert) g_Insert(grid, cell, 1);   /* right after base speed */

    /* Apply font size + padding AFTER hosting so the class theme doesn't override */
    if (val && g_GetFontSize && g_SetFontSize) {
        int sz = g_GetFontSize(val);
        g_SetFontSize(val, -28);
        snprintf(line, sizeof(line), "  fontsize was %d -> -28", sz);
        log_write(line);
    }
    if (val && g_SetPadding) g_SetPadding(val, 110, -10, 0, 0);

    g_valueTile = val;
    snprintf(line, sizeof(line), "  cell=%p lbl=%p val=%p layout=%p hr=%x/%x/%x/%x",
             cell, lbl, val, layout, (unsigned)hc, (unsigned)hl, (unsigned)hv, (unsigned)hLay);
    log_write(line);
}

long my_SetContentString(void *el, const wchar_t *content)
{
    if (content) {
        size_t len = wcslen(content);

        /* 1) Sidebar temp: append to the CPU sidebar speed (sidebarData " GHz") */
        if (len >= 4 && wcscmp(content + len - 4, L" GHz") == 0) {
            if (g_GetClass) {
                void *v = NULL;
                const wchar_t *cls = g_GetClass(el, &v);
                if (cls && wcscmp(cls, L"sidebarData") == 0) {
                    float t = 0.0f;
                    if (coretemp_read_average(&t) == 0) {
                        static wchar_t buf[128];
                        swprintf(buf, sizeof(buf) / sizeof(buf[0]),
                                 L"%ls (%.0f\u00B0C)", content, t);
                        return g_origSetContent(el, buf);
                    }
                }
            }
        }

        /* 2) Tile injection: on first staticData (base speed), insert Temperature tile */
        if (!g_tileInjected && g_GetClass && g_ElementCreate && g_GetParent) {
            void *v = NULL;
            const wchar_t *cls = g_GetClass(el, &v);
            if (cls && wcscmp(cls, L"staticData") == 0) {
                if (InterlockedCompareExchange(&g_tileInjected, 1, 0) == 0)
                    inject_tile(el);
            }
        }
    }

    if (g_valueTile && g_origSetContent) {
        DWORD now = GetTickCount();
        if (now - g_lastUpdate >= 1000) {
            g_lastUpdate = now;
            update_value();
        }
    }

    return g_origSetContent(el, content);
}

static DWORD WINAPI worker(void *p)
{
    (void)p;
    Sleep(300);

    HMODULE dui = GetModuleHandleW(L"dui70.dll");
    if (!dui) dui = LoadLibraryW(L"dui70.dll");
    g_SetClass = (SetClass_fn)GetProcAddress(dui, "?SetClass@Element@DirectUI@@QEAAJPEBG@Z");
    g_Add = (Add_fn)GetProcAddress(dui, "?Add@Element@DirectUI@@QEAAJPEAV12@@Z");
    g_Insert = (Insert_fn)GetProcAddress(dui, "?Insert@Element@DirectUI@@QEAAJPEAV12@I@Z");
    g_ElementCreate = (ElementCreate_fn)GetProcAddress(dui, "?Create@Element@DirectUI@@SAJIPEAV12@PEAKPEAPEAV12@@Z");
    g_GetClass = (GetClass_fn)GetProcAddress(dui, "?GetClass@Element@DirectUI@@QEAAPEBGPEAPEAVValue@2@@Z");
    g_GetParent = (GetParent_fn)GetProcAddress(dui, "?GetParent@Element@DirectUI@@QEAAPEAV12@XZ");
    g_SetForegroundColor = (SetForegroundColor_fn)GetProcAddress(dui, "?SetForegroundColor@Element@DirectUI@@QEAAJK@Z");
    g_SetLayout = (SetLayout_fn)GetProcAddress(dui, "?SetLayout@Element@DirectUI@@QEAAJPEAVLayout@2@@Z");
    g_FillLayoutCreate = (FillLayoutCreate_fn)GetProcAddress(dui, "?Create@FillLayout@DirectUI@@SAJPEAPEAVLayout@2@@Z");
    g_SetPadding = (SetPadding_fn)GetProcAddress(dui, "?SetPadding@Element@DirectUI@@QEAAJHHHH@Z");
    g_GetFontSize = (GetFontSize_fn)GetProcAddress(dui, "?GetFontSize@Element@DirectUI@@QEAAHXZ");
    g_SetFontSize = (SetFontSize_fn)GetProcAddress(dui, "?SetFontSize@Element@DirectUI@@QEAAJH@Z");

    BYTE *base = (BYTE *)GetModuleHandleW(NULL);
    void **iat = (void **)(base + 0x2D5F50);
    g_origSetContent = (SetContentString_fn)*iat;
    DWORD old;
    VirtualProtect(iat, sizeof(void *), PAGE_READWRITE, &old);
    *iat = (void *)my_SetContentString;
    VirtualProtect(iat, sizeof(void *), old, &old);
    FlushInstructionCache(GetCurrentProcess(), iat, sizeof(void *));

    log_write("hooks installed");
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        DeleteFileW(L"C:\\Users\\cutter\\Documents\\Projects\\taskmgr add CPU temp\\reverse\\tile.log");
        HANDLE t = CreateThread(NULL, 0, worker, NULL, 0, NULL);
        if (t) CloseHandle(t);
    }
    return TRUE;
}
