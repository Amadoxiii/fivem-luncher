/* ============================================================
 *   FIVEM LAUNCHER - Modern Dark-Themed Win32 Application
 *   A sleek launcher for managing FiveM
 * ============================================================ */

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define _WIN32_WINNT 0x0600

#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <stdio.h>
#include <wchar.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "shell32.lib")

/* ---- Color Palette ---- */
#define CLR_BG          RGB(13, 17, 23)
#define CLR_HEADER      RGB(18, 22, 30)
#define CLR_PANEL       RGB(22, 27, 34)
#define CLR_TAB_BG      RGB(18, 22, 30)
#define CLR_TAB_ACTIVE  RGB(30, 37, 46)
#define CLR_TAB_HOVER   RGB(25, 31, 40)
#define CLR_ACCENT      RGB(249, 115, 22)
#define CLR_ACCENT_DIM  RGB(200, 90, 15)
#define CLR_TEXT        RGB(230, 237, 243)
#define CLR_TEXT_SEC    RGB(125, 133, 144)
#define CLR_SUCCESS     RGB(46, 204, 113)
#define CLR_DANGER      RGB(239, 68, 68)
#define CLR_INPUT_BG    RGB(13, 17, 23)
#define CLR_BORDER      RGB(48, 54, 61)
#define CLR_FOOTER      RGB(18, 22, 30)

/* ---- Layout Constants ---- */
#define WINDOW_W    800
#define WINDOW_H    650
#define HEADER_H    58
#define TAB_H       44
#define FOOTER_H    72
#define CONTENT_Y   (HEADER_H + TAB_H)
#define PADDING     28
#define TAB_COUNT   5

/* ---- Button Styles ---- */
enum { BTN_PRIMARY = 0, BTN_SUCCESS, BTN_DANGER, BTN_SECONDARY };

typedef struct {
    COLORREF bgNormal, bgHover, bgPressed, text;
} BtnColors;

static const BtnColors g_btnStyles[] = {
    { RGB(249,115,22),  RGB(251,146,60),  RGB(234,88,12),  RGB(255,255,255) },
    { RGB(34,197,94),   RGB(74,222,128),  RGB(22,163,74),  RGB(255,255,255) },
    { RGB(220,38,38),   RGB(248,113,113), RGB(185,28,28),  RGB(255,255,255) },
    { RGB(48,54,61),    RGB(63,70,78),    RGB(33,38,45),   RGB(210,217,224) },
};

/* ---- Control IDs ---- */
#define IDC_BTN_LAUNCH         1001
#define IDC_BTN_CACHE_NORMAL   1002
#define IDC_BTN_CACHE_EXTREME  1003
#define IDC_BTN_MODS_TOGGLE    1004
#define IDC_BTN_PLUGINS_TOGGLE 1005
#define IDC_COMBO_BUILD        1006
#define IDC_LIST_SERVERS       1007
#define IDC_EDIT_SRV_NAME      1008
#define IDC_EDIT_SRV_ADDR      1009
#define IDC_BTN_SRV_ADD        1010
#define IDC_BTN_SRV_CONNECT    1011
#define IDC_BTN_SRV_DELETE     1012
#define IDC_CMB_BUILD          1013
#define IDC_CMB_DELAY          1014

#define IDC_LBL_FIVEM_STATUS   1101
#define IDC_LBL_FIVEM_PATH     1102
#define IDC_LBL_CACHE_SIZE     1103
#define IDC_LBL_MODS_STATUS    1104
#define IDC_LBL_PLUGINS_STATUS 1105

/* Section titles and desc use ranges */
#define IDC_LBL_SECTION_BASE   1200
#define IDC_LBL_DESC_BASE      1300

/* ---- Data Structures ---- */
#define MAX_SERVERS 50

typedef struct {
    wchar_t name[128];
    wchar_t address[256];
} ServerEntry;

typedef enum { FOLDER_ENABLED, FOLDER_DISABLED, FOLDER_NOTFOUND } FolderStatus;

/* ---- Globals ---- */
static HINSTANCE g_hInstance;
static HWND g_hMainWnd;
static int  g_activeTab = 0;
static int  g_hoveredTab = -1;
static BOOL g_trackingMouse = FALSE;

static wchar_t g_fivemExePath[MAX_PATH];
static wchar_t g_fivemDataPath[MAX_PATH];
static wchar_t g_fivemAppDataPath[MAX_PATH];
static wchar_t g_citizenfxIniPath[MAX_PATH];
static wchar_t g_launcherDir[MAX_PATH];
static wchar_t g_serversIniPath[MAX_PATH];
static wchar_t g_configIniPath[MAX_PATH];
static BOOL g_fivemInstalled = FALSE;

static ServerEntry g_servers[MAX_SERVERS];
static int g_serverCount = 0;
static int g_selectedBuildIndex = 0;
static int g_connectDelaySeconds = 15;

static HFONT g_fontTitle, g_fontSection, g_fontBold, g_fontTab, g_fontNormal, g_fontButton, g_fontSmall;
static HBRUSH g_hBrushBg, g_hBrushInput, g_hBrushPanel;

/* Tab control management */
#define MAX_TAB_CTRLS 25
static HWND g_tabCtrls[TAB_COUNT][MAX_TAB_CTRLS];
static int  g_tabCtrlCount[TAB_COUNT] = {0};

/* Important control references */
static HWND g_hBtnLaunch;
static HWND g_hBtnModsToggle, g_hBtnPluginsToggle;
static HWND g_hModsStatus, g_hPluginsStatus;
static HWND g_hFivemStatus, g_hFivemPath;
static HWND g_hCacheSize;
static HWND g_hSrvNameEdit = NULL;
static HWND g_hSrvAddrEdit = NULL;
static HWND g_hServerList = NULL;
static HWND g_hBuildCombo = NULL;
static HWND g_hDelayCombo = NULL;
static HWND g_hBtnConnect = NULL;

static const wchar_t* g_tabNames[] = {
    L"Accueil", L"Cache", L"Mods & Plugins", L"Param\x00E8tres", L"Serveurs"
};

static const wchar_t* g_buildItems[] = {
    L"Automatique",
    L"1604  \x2014  Arena War",
    L"2060  \x2014  Los Santos Summer Special",
    L"2189  \x2014  Cayo Perico",
    L"2372  \x2014  Los Santos Tuners",
    L"2545  \x2014  The Contract",
    L"2612  \x2014  Expanded & Enhanced",
    L"2699  \x2014  Criminal Enterprises",
    L"2802  \x2014  Los Santos Drug Wars",
    L"2944  \x2014  San Andreas Mercenaries",
    L"3095  \x2014  The Chop Shop",
    L"3258  \x2014  Bottom Dollar Bounties",
    L"3407  \x2014  Agents of Sabotage",
    L"3570  \x2014  Money Fronts",
    L"3751  \x2014  A Safehouse in the Hills"
};
static const int g_buildNumbers[] = {
    0, 1604, 2060, 2189, 2372, 2545, 2612, 2699, 2802, 2944, 3095, 3258, 3407, 3570, 3751
};
#define BUILD_COUNT 15

/* ---- Forward Declarations ---- */
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK ButtonSubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
static void AddTabControl(int tab, HWND hwnd);
static void SwitchTab(int newTab);
static void DetectFiveMPaths(void);
static BOOL DeleteDirectoryRecursive(const wchar_t* path);
static ULONGLONG GetDirectorySize(const wchar_t* path);
static void LoadServers(void);
static void SaveServers(void);
static void LoadSettings(void);
static void SaveSettings(void);
static void RefreshServerList(void);
static void RefreshModsUI(void);
static void RefreshPluginsUI(void);
static void RefreshCacheSize(void);
static void CreateAllControls(HWND hWnd);
static void DrawCustomButton(LPDRAWITEMSTRUCT dis);
static void DrawServerItem(LPDRAWITEMSTRUCT dis);
static void DrawBuildComboItem(LPDRAWITEMSTRUCT dis);
static void ApplyBuildSetting(void);

/* ============================================================
 *   UTILITY: Path Detection
 * ============================================================ */

static void DetectFiveMPaths(void) {
    wchar_t localAppData[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData) != S_OK) return;

    swprintf(g_fivemExePath, MAX_PATH, L"%s\\FiveM\\FiveM.exe", localAppData);
    swprintf(g_fivemDataPath, MAX_PATH, L"%s\\FiveM\\FiveM.app\\data", localAppData);
    swprintf(g_fivemAppDataPath, MAX_PATH, L"%s\\FiveM\\FiveM.app", localAppData);
    swprintf(g_citizenfxIniPath, MAX_PATH, L"%s\\FiveM\\FiveM.app\\CitizenFX.ini", localAppData);

    g_fivemInstalled = (GetFileAttributesW(g_fivemExePath) != INVALID_FILE_ATTRIBUTES);

    /* Launcher config paths (next to exe) */
    GetModuleFileNameW(NULL, g_launcherDir, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(g_launcherDir, L'\\');
    if (lastSlash) *lastSlash = L'\0';

    swprintf(g_serversIniPath, MAX_PATH, L"%s\\launcher_servers.ini", g_launcherDir);
    swprintf(g_configIniPath, MAX_PATH, L"%s\\launcher_config.ini", g_launcherDir);
}

/* ============================================================
 *   UTILITY: File Operations
 * ============================================================ */

static BOOL DeleteDirectoryRecursive(const wchar_t* path) {
    WIN32_FIND_DATAW fd;
    wchar_t search[MAX_PATH];
    swprintf(search, MAX_PATH, L"%s\\*", path);

    HANDLE hFind = FindFirstFileW(search, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return FALSE;

    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;

        wchar_t fullPath[MAX_PATH];
        swprintf(fullPath, MAX_PATH, L"%s\\%s", path, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            DeleteDirectoryRecursive(fullPath);
        } else {
            SetFileAttributesW(fullPath, FILE_ATTRIBUTE_NORMAL);
            DeleteFileW(fullPath);
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    return RemoveDirectoryW(path);
}

static ULONGLONG GetDirectorySize(const wchar_t* path) {
    ULONGLONG size = 0;
    WIN32_FIND_DATAW fd;
    wchar_t search[MAX_PATH];
    swprintf(search, MAX_PATH, L"%s\\*", path);

    HANDLE hFind = FindFirstFileW(search, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return 0;

    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;

        wchar_t fullPath[MAX_PATH];
        swprintf(fullPath, MAX_PATH, L"%s\\%s", path, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            size += GetDirectorySize(fullPath);
        } else {
            size += ((ULONGLONG)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
    return size;
}

static FolderStatus GetFolderStatus(const wchar_t* enabledName, const wchar_t* disabledName) {
    wchar_t ePath[MAX_PATH], dPath[MAX_PATH];
    swprintf(ePath, MAX_PATH, L"%s\\%s", g_fivemAppDataPath, enabledName);
    swprintf(dPath, MAX_PATH, L"%s\\%s", g_fivemAppDataPath, disabledName);

    if (GetFileAttributesW(ePath) != INVALID_FILE_ATTRIBUTES) return FOLDER_ENABLED;
    if (GetFileAttributesW(dPath) != INVALID_FILE_ATTRIBUTES) return FOLDER_DISABLED;
    return FOLDER_NOTFOUND;
}

/* ============================================================
 *   UTILITY: Settings & Server I/O
 * ============================================================ */

static void LoadSettings(void) {
    g_selectedBuildIndex = GetPrivateProfileIntW(L"Settings", L"GameBuildIndex", 0, g_configIniPath);
    g_connectDelaySeconds = GetPrivateProfileIntW(L"Settings", L"ConnectDelay", 15, g_configIniPath);
    if (g_selectedBuildIndex < 0 || g_selectedBuildIndex >= BUILD_COUNT)
        g_selectedBuildIndex = 0;
}

static void SaveSettings(void) {
    wchar_t val[16];
    swprintf(val, 16, L"%d", g_selectedBuildIndex);
    WritePrivateProfileStringW(L"Settings", L"GameBuildIndex", val, g_configIniPath);
    swprintf(val, 16, L"%d", g_connectDelaySeconds);
    WritePrivateProfileStringW(L"Settings", L"ConnectDelay", val, g_configIniPath);
}

static void LoadServers(void) {
    g_serverCount = GetPrivateProfileIntW(L"Servers", L"Count", 0, g_serversIniPath);
    if (g_serverCount > MAX_SERVERS) g_serverCount = MAX_SERVERS;

    for (int i = 0; i < g_serverCount; i++) {
        wchar_t keyName[32], keyAddr[32];
        swprintf(keyName, 32, L"Name%d", i);
        swprintf(keyAddr, 32, L"Addr%d", i);
        GetPrivateProfileStringW(L"Servers", keyName, L"", g_servers[i].name, 128, g_serversIniPath);
        GetPrivateProfileStringW(L"Servers", keyAddr, L"", g_servers[i].address, 256, g_serversIniPath);
    }
}

static void SaveServers(void) {
    /* Clear the section first */
    WritePrivateProfileStringW(L"Servers", NULL, NULL, g_serversIniPath);

    wchar_t val[16];
    swprintf(val, 16, L"%d", g_serverCount);
    WritePrivateProfileStringW(L"Servers", L"Count", val, g_serversIniPath);

    for (int i = 0; i < g_serverCount; i++) {
        wchar_t keyName[32], keyAddr[32];
        swprintf(keyName, 32, L"Name%d", i);
        swprintf(keyAddr, 32, L"Addr%d", i);
        WritePrivateProfileStringW(L"Servers", keyName, g_servers[i].name, g_serversIniPath);
        WritePrivateProfileStringW(L"Servers", keyAddr, g_servers[i].address, g_serversIniPath);
    }
}

static void ApplyBuildSetting(void) {
    int buildNumber = g_buildNumbers[g_selectedBuildIndex];
    if (buildNumber == 0) {
        WritePrivateProfileStringW(L"Game", L"SavedBuildNumber", NULL, g_citizenfxIniPath);
    } else {
        wchar_t val[16];
        swprintf(val, 16, L"%d", buildNumber);
        WritePrivateProfileStringW(L"Game", L"SavedBuildNumber", val, g_citizenfxIniPath);
    }
}

/* ============================================================
 *   UTILITY: Connection Helpers
 * ============================================================ */

static BOOL IsCfxAddress(const wchar_t* addr) {
    if (wcsstr(addr, L"cfx.re")) return TRUE;
    if (wcschr(addr, L'.') && wcschr(addr, L':')) return FALSE;
    return TRUE;
}

static void LaunchFiveM(void) {
    if (!g_fivemInstalled) {
        MessageBoxW(g_hMainWnd,
            L"FiveM n'est pas install\x00E9 ou n'a pas \x00E9t\x00E9 trouv\x00E9.\n\n"
            L"Chemin attendu :\n%localappdata%\\FiveM\\FiveM.exe",
            L"FiveM Launcher", MB_OK | MB_ICONWARNING);
        return;
    }
    
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    wchar_t vbsPath[MAX_PATH];
    swprintf(vbsPath, MAX_PATH, L"%s\\fivem_launch.vbs", tempPath);

    FILE* f = _wfopen(vbsPath, L"wt, ccs=UTF-16LE");
    if (f) {
        fwprintf(f, L"Set objShell = CreateObject(\"Shell.Application\")\n");
        fwprintf(f, L"objShell.ShellExecute \"%s\", \"\", \"\", \"open\", 1\n", g_fivemExePath);
        fclose(f);
    }

    ShellExecuteW(NULL, L"open", vbsPath, NULL, NULL, SW_HIDE);
}

static BOOL IsFiveMRunning(void) {
    BOOL exists = FALSE;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(hSnap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, L"FiveM.exe") == 0) {
                    exists = TRUE;
                    break;
                }
            } while (Process32NextW(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
    return exists;
}

struct ConnectArgs {
    wchar_t url[512];
};

static DWORD WINAPI AutoConnectThread(LPVOID lpParam) {
    struct ConnectArgs* args = (struct ConnectArgs*)lpParam;
    
    int waitCount = 0;
    while (!IsFiveMRunning() && waitCount < 30) {
        Sleep(1000);
        waitCount++;
    }
    
    Sleep(g_connectDelaySeconds * 1000);

    char urlA[512];
    WideCharToMultiByte(CP_UTF8, 0, args->url, -1, urlA, 512, NULL, NULL);

    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    wchar_t urlFile[MAX_PATH];
    swprintf(urlFile, MAX_PATH, L"%s\\fivem_connect.url", tempPath);

    FILE* f = _wfopen(urlFile, L"wt");
    if (f) {
        fprintf(f, "[InternetShortcut]\nURL=%s\n", urlA);
        fclose(f);
    }
    
    ShellExecuteW(NULL, L"open", urlFile, NULL, NULL, SW_SHOWNORMAL);
    
    if (g_hBtnConnect) {
        EnableWindow(g_hBtnConnect, TRUE);
        SetWindowTextW(g_hBtnConnect, L"  Connecter");
        InvalidateRect(g_hBtnConnect, NULL, TRUE);
    }

    free(args);
    return 0;
}

static void ConnectToServer(const wchar_t* address) {
    if (!g_fivemInstalled) {
        MessageBoxW(g_hMainWnd,
            L"FiveM n'est pas install\x00E9.",
            L"FiveM Launcher", MB_OK | MB_ICONWARNING);
        return;
    }

    wchar_t url[512];
    if (IsCfxAddress(address)) {
        if (wcsstr(address, L"cfx.re/join/")) {
            swprintf(url, 512, L"fivem://connect/%s", address);
        } else {
            swprintf(url, 512, L"fivem://connect/cfx.re/join/%s", address);
        }
    } else {
        swprintf(url, 512, L"fivem://connect/%s", address);
    }

    if (!IsFiveMRunning()) {
        if (g_hBtnConnect) {
            EnableWindow(g_hBtnConnect, FALSE);
            SetWindowTextW(g_hBtnConnect, L"  Lancement...");
            InvalidateRect(g_hBtnConnect, NULL, TRUE);
        }

        wchar_t tempPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);
        wchar_t vbsPath[MAX_PATH];
        swprintf(vbsPath, MAX_PATH, L"%s\\fivem_launch.vbs", tempPath);

        FILE* f = _wfopen(vbsPath, L"wt, ccs=UTF-16LE");
        if (f) {
            fwprintf(f, L"Set objShell = CreateObject(\"Shell.Application\")\n");
            fwprintf(f, L"objShell.ShellExecute \"%s\", \"\", \"\", \"open\", 1\n", g_fivemExePath);
            fclose(f);
        }
        ShellExecuteW(NULL, L"open", vbsPath, NULL, NULL, SW_HIDE);

        struct ConnectArgs* args = malloc(sizeof(struct ConnectArgs));
        wcscpy(args->url, url);
        
        CreateThread(NULL, 0, AutoConnectThread, args, 0, NULL);
        return;
    }
    
    char urlA[512];
    WideCharToMultiByte(CP_UTF8, 0, url, -1, urlA, 512, NULL, NULL);

    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    wchar_t urlFile[MAX_PATH];
    swprintf(urlFile, MAX_PATH, L"%s\\fivem_connect.url", tempPath);

    FILE* f = _wfopen(urlFile, L"wt");
    if (f) {
        fprintf(f, "[InternetShortcut]\nURL=%s\n", urlA);
        fclose(f);
    }
    
    ShellExecuteW(NULL, L"open", urlFile, NULL, NULL, SW_SHOWNORMAL);
}

/* ============================================================
 *   UI: Tab Control Management
 * ============================================================ */

static void AddTabControl(int tab, HWND hwnd) {
    if (tab >= 0 && tab < TAB_COUNT && g_tabCtrlCount[tab] < MAX_TAB_CTRLS) {
        g_tabCtrls[tab][g_tabCtrlCount[tab]++] = hwnd;
    }
}

static void SwitchTab(int newTab) {
    for (int t = 0; t < TAB_COUNT; t++) {
        int show = (t == newTab) ? SW_SHOW : SW_HIDE;
        for (int c = 0; c < g_tabCtrlCount[t]; c++) {
            ShowWindow(g_tabCtrls[t][c], show);
        }
    }
    g_activeTab = newTab;

    if (newTab == 1) RefreshCacheSize();
    if (newTab == 2) { RefreshModsUI(); RefreshPluginsUI(); }

    InvalidateRect(g_hMainWnd, NULL, TRUE);
}

/* ============================================================
 *   UI: Button Subclass (Hover Tracking)
 * ============================================================ */

static LRESULT CALLBACK ButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                            LPARAM lParam, UINT_PTR uIdSubclass,
                                            DWORD_PTR dwRefData) {
    (void)uIdSubclass; (void)dwRefData;
    switch (uMsg) {
        case WM_MOUSEMOVE:
            if (!GetProp(hWnd, L"Hover")) {
                SetProp(hWnd, L"Hover", (HANDLE)1);
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
                TrackMouseEvent(&tme);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        case WM_MOUSELEAVE:
            RemoveProp(hWnd, L"Hover");
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, ButtonSubclassProc, uIdSubclass);
            RemoveProp(hWnd, L"Hover");
            RemoveProp(hWnd, L"Style");
            RemoveProp(hWnd, L"FG");
            break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/* ============================================================
 *   UI: Styled Control Creation Helpers
 * ============================================================ */

static HWND CreateStyledButton(HWND parent, int id, const wchar_t* text,
                                int x, int y, int w, int h, int style, int tab) {
    HWND btn = CreateWindowExW(0, L"BUTTON", text,
        WS_CHILD | BS_OWNERDRAW, x, y, w, h,
        parent, (HMENU)(INT_PTR)id, g_hInstance, NULL);
    SetProp(btn, L"Style", (HANDLE)(INT_PTR)style);
    SetWindowSubclass(btn, ButtonSubclassProc, 0, 0);
    SendMessageW(btn, WM_SETFONT, (WPARAM)g_fontButton, TRUE);
    if (tab >= 0) AddTabControl(tab, btn);
    return btn;
}

static HWND CreateLabel(HWND parent, int id, const wchar_t* text,
                         int x, int y, int w, int h,
                         COLORREF color, HFONT font, int tab) {
    HWND lbl = CreateWindowExW(0, L"STATIC", text,
        WS_CHILD | SS_LEFT, x, y, w, h,
        parent, (HMENU)(INT_PTR)id, g_hInstance, NULL);
    SetProp(lbl, L"FG", (HANDLE)(ULONG_PTR)color);
    SendMessageW(lbl, WM_SETFONT, (WPARAM)(font ? font : g_fontNormal), TRUE);
    if (tab >= 0) AddTabControl(tab, lbl);
    return lbl;
}

/* ============================================================
 *   UI: Refresh Functions
 * ============================================================ */

static void RefreshCacheSize(void) {
    if (GetFileAttributesW(g_fivemDataPath) == INVALID_FILE_ATTRIBUTES) {
        SetWindowTextW(g_hCacheSize, L"Dossier data non trouv\x00E9");
        return;
    }
    ULONGLONG size = GetDirectorySize(g_fivemDataPath);
    wchar_t text[128];
    if (size > 1073741824ULL)
        swprintf(text, 128, L"Taille du cache :  %.2f Go", (double)size / 1073741824.0);
    else
        swprintf(text, 128, L"Taille du cache :  %.1f Mo", (double)size / 1048576.0);
    SetWindowTextW(g_hCacheSize, text);
}

static void RefreshModsUI(void) {
    FolderStatus status = GetFolderStatus(L"mods", L"disablemods");
    switch (status) {
        case FOLDER_ENABLED:
            SetWindowTextW(g_hBtnModsToggle, L"  D\x00E9sactiver les mods");
            SetProp(g_hBtnModsToggle, L"Style", (HANDLE)BTN_DANGER);
            SetWindowTextW(g_hModsStatus, L"\x25CF  Mods activ\x00E9s");
            SetProp(g_hModsStatus, L"FG", (HANDLE)(ULONG_PTR)CLR_SUCCESS);
            EnableWindow(g_hBtnModsToggle, TRUE);
            break;
        case FOLDER_DISABLED:
            SetWindowTextW(g_hBtnModsToggle, L"  Activer les mods");
            SetProp(g_hBtnModsToggle, L"Style", (HANDLE)BTN_SUCCESS);
            SetWindowTextW(g_hModsStatus, L"\x25CF  Mods d\x00E9sactiv\x00E9s");
            SetProp(g_hModsStatus, L"FG", (HANDLE)(ULONG_PTR)CLR_DANGER);
            EnableWindow(g_hBtnModsToggle, TRUE);
            break;
        case FOLDER_NOTFOUND:
            SetWindowTextW(g_hBtnModsToggle, L"  Dossier non trouv\x00E9");
            SetProp(g_hBtnModsToggle, L"Style", (HANDLE)BTN_SECONDARY);
            SetWindowTextW(g_hModsStatus, L"\x25CF  Dossier mods non trouv\x00E9");
            SetProp(g_hModsStatus, L"FG", (HANDLE)(ULONG_PTR)CLR_TEXT_SEC);
            EnableWindow(g_hBtnModsToggle, FALSE);
            break;
    }
    InvalidateRect(g_hBtnModsToggle, NULL, TRUE);
    InvalidateRect(g_hModsStatus, NULL, TRUE);
}

static void RefreshPluginsUI(void) {
    FolderStatus status = GetFolderStatus(L"plugins", L"disableplugins");
    switch (status) {
        case FOLDER_ENABLED:
            SetWindowTextW(g_hBtnPluginsToggle, L"  D\x00E9sactiver les plugins");
            SetProp(g_hBtnPluginsToggle, L"Style", (HANDLE)BTN_DANGER);
            SetWindowTextW(g_hPluginsStatus, L"\x25CF  Plugins activ\x00E9s");
            SetProp(g_hPluginsStatus, L"FG", (HANDLE)(ULONG_PTR)CLR_SUCCESS);
            EnableWindow(g_hBtnPluginsToggle, TRUE);
            break;
        case FOLDER_DISABLED:
            SetWindowTextW(g_hBtnPluginsToggle, L"  Activer les plugins");
            SetProp(g_hBtnPluginsToggle, L"Style", (HANDLE)BTN_SUCCESS);
            SetWindowTextW(g_hPluginsStatus, L"\x25CF  Plugins d\x00E9sactiv\x00E9s");
            SetProp(g_hPluginsStatus, L"FG", (HANDLE)(ULONG_PTR)CLR_DANGER);
            EnableWindow(g_hBtnPluginsToggle, TRUE);
            break;
        case FOLDER_NOTFOUND:
            SetWindowTextW(g_hBtnPluginsToggle, L"  Dossier non trouv\x00E9");
            SetProp(g_hBtnPluginsToggle, L"Style", (HANDLE)BTN_SECONDARY);
            SetWindowTextW(g_hPluginsStatus, L"\x25CF  Dossier plugins non trouv\x00E9");
            SetProp(g_hPluginsStatus, L"FG", (HANDLE)(ULONG_PTR)CLR_TEXT_SEC);
            EnableWindow(g_hBtnPluginsToggle, FALSE);
            break;
    }
    InvalidateRect(g_hBtnPluginsToggle, NULL, TRUE);
    InvalidateRect(g_hPluginsStatus, NULL, TRUE);
}

static void RefreshServerList(void) {
    SendMessageW(g_hServerList, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < g_serverCount; i++) {
        int idx = (int)SendMessageW(g_hServerList, LB_ADDSTRING, 0, (LPARAM)g_servers[i].name);
        SendMessageW(g_hServerList, LB_SETITEMDATA, idx, (LPARAM)i);
    }
}

/* ============================================================
 *   DRAWING: Custom Button
 * ============================================================ */

static void DrawCustomButton(LPDRAWITEMSTRUCT dis) {
    BOOL disabled = (dis->itemState & ODS_DISABLED);
    BOOL pressed  = (dis->itemState & ODS_SELECTED);
    BOOL hovered  = (BOOL)(ULONG_PTR)GetProp(dis->hwndItem, L"Hover");

    int style = (int)(INT_PTR)GetProp(dis->hwndItem, L"Style");
    if (style < 0 || style > BTN_SECONDARY) style = BTN_SECONDARY;
    BtnColors colors = g_btnStyles[style];

    COLORREF bgColor;
    if (disabled)    bgColor = RGB(28, 32, 38);
    else if (pressed) bgColor = colors.bgPressed;
    else if (hovered) bgColor = colors.bgHover;
    else              bgColor = colors.bgNormal;

    /* Fill background first to avoid artifacts */
    HBRUSH bgBrush = CreateSolidBrush(CLR_BG);
    FillRect(dis->hDC, &dis->rcItem, bgBrush);
    DeleteObject(bgBrush);

    /* Draw rounded button */
    HBRUSH brush = CreateSolidBrush(bgColor);
    HPEN pen = CreatePen(PS_SOLID, 1, bgColor);
    HBRUSH oldBrush = (HBRUSH)SelectObject(dis->hDC, brush);
    HPEN oldPen = (HPEN)SelectObject(dis->hDC, pen);
    RoundRect(dis->hDC, dis->rcItem.left + 1, dis->rcItem.top + 1,
              dis->rcItem.right - 1, dis->rcItem.bottom - 1, 10, 10);
    SelectObject(dis->hDC, oldBrush);
    SelectObject(dis->hDC, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    /* Draw text */
    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, disabled ? RGB(80, 85, 92) : colors.text);
    HFONT oldFont = (HFONT)SelectObject(dis->hDC, g_fontButton);
    wchar_t text[256];
    GetWindowTextW(dis->hwndItem, text, 256);
    DrawTextW(dis->hDC, text, -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dis->hDC, oldFont);
}

/* ============================================================
 *   DRAWING: Server List Item
 * ============================================================ */

static void DrawServerItem(LPDRAWITEMSTRUCT dis) {
    if (dis->itemID == (UINT)-1) {
        HBRUSH brush = CreateSolidBrush(CLR_BG);
        FillRect(dis->hDC, &dis->rcItem, brush);
        DeleteObject(brush);
        return;
    }

    int idx = (int)dis->itemData;
    if (idx < 0 || idx >= g_serverCount) return;

    BOOL selected = (dis->itemState & ODS_SELECTED);
    RECT rc = dis->rcItem;

    /* Background */
    COLORREF bgColor = selected ? RGB(30, 37, 46) : CLR_BG;
    HBRUSH brush = CreateSolidBrush(bgColor);
    FillRect(dis->hDC, &rc, brush);
    DeleteObject(brush);

    /* Selected accent bar on the left */
    if (selected) {
        RECT bar = { rc.left, rc.top + 4, rc.left + 3, rc.bottom - 4 };
        HBRUSH accentBrush = CreateSolidBrush(CLR_ACCENT);
        FillRect(dis->hDC, &bar, accentBrush);
        DeleteObject(accentBrush);
    }

    SetBkMode(dis->hDC, TRANSPARENT);

    /* Server name */
    HFONT oldFont = (HFONT)SelectObject(dis->hDC, g_fontNormal);
    SetTextColor(dis->hDC, CLR_TEXT);
    RECT nameRect = { rc.left + 16, rc.top + 6, rc.right - 10, rc.top + 26 };
    DrawTextW(dis->hDC, g_servers[idx].name, -1, &nameRect, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

    /* Server address */
    SelectObject(dis->hDC, g_fontSmall);
    SetTextColor(dis->hDC, CLR_TEXT_SEC);
    RECT addrRect = { rc.left + 16, rc.top + 28, rc.right - 10, rc.bottom - 4 };
    DrawTextW(dis->hDC, g_servers[idx].address, -1, &addrRect, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

    SelectObject(dis->hDC, oldFont);

    /* Bottom separator */
    HPEN pen = CreatePen(PS_SOLID, 1, CLR_BORDER);
    HPEN oldPen = (HPEN)SelectObject(dis->hDC, pen);
    MoveToEx(dis->hDC, rc.left + 12, rc.bottom - 1, NULL);
    LineTo(dis->hDC, rc.right - 12, rc.bottom - 1);
    SelectObject(dis->hDC, oldPen);
    DeleteObject(pen);
}

/* ============================================================
 *   DRAWING: Build Combo Item
 * ============================================================ */

static void DrawBuildComboItem(LPDRAWITEMSTRUCT dis) {
    if (dis->itemID == (UINT)-1) {
        HBRUSH brush = CreateSolidBrush(CLR_INPUT_BG);
        FillRect(dis->hDC, &dis->rcItem, brush);
        DeleteObject(brush);
        return;
    }

    BOOL isDisplay = (dis->itemState & ODS_COMBOBOXEDIT);
    BOOL selected  = (dis->itemState & ODS_SELECTED) && !isDisplay;

    COLORREF bg = selected ? CLR_ACCENT : (isDisplay ? CLR_INPUT_BG : RGB(22, 27, 34));
    COLORREF fg = (selected || isDisplay) ? CLR_TEXT : CLR_TEXT;

    HBRUSH brush = CreateSolidBrush(bg);
    FillRect(dis->hDC, &dis->rcItem, brush);
    DeleteObject(brush);

    wchar_t text[128];
    SendMessageW(dis->hwndItem, CB_GETLBTEXT, dis->itemID, (LPARAM)text);

    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, fg);
    HFONT oldFont = (HFONT)SelectObject(dis->hDC, g_fontNormal);
    RECT textRect = dis->rcItem;
    textRect.left += 12;
    DrawTextW(dis->hDC, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dis->hDC, oldFont);
}

/* ============================================================
 *   UI: Create All Controls
 * ============================================================ */

static void CreateAllControls(HWND hWnd) {
    int cx = PADDING;
    int cw = WINDOW_W - 2 * PADDING;
    int cy;

    /* ====== TAB 0: Accueil ====== */
    cy = CONTENT_Y + 20;
    CreateLabel(hWnd, IDC_LBL_SECTION_BASE, L"BIENVENUE", cx, cy, cw, 24, CLR_ACCENT, g_fontSection, 0);
    cy += 38;
    CreateLabel(hWnd, IDC_LBL_DESC_BASE, L"Votre outil tout-en-un pour g\x00E9rer FiveM.", cx, cy, cw, 20, CLR_TEXT_SEC, g_fontNormal, 0);
    cy += 45;
    CreateLabel(hWnd, IDC_LBL_SECTION_BASE+1, L"STATUT FIVEM", cx, cy, cw, 24, CLR_ACCENT, g_fontSection, 0);
    cy += 34;
    g_hFivemStatus = CreateLabel(hWnd, IDC_LBL_FIVEM_STATUS, L"", cx, cy, cw, 20, CLR_SUCCESS, g_fontNormal, 0);
    cy += 26;
    g_hFivemPath = CreateLabel(hWnd, IDC_LBL_FIVEM_PATH, L"", cx, cy, cw, 18, CLR_TEXT_SEC, g_fontSmall, 0);
    cy += 45;
    CreateLabel(hWnd, IDC_LBL_SECTION_BASE+2, L"FONCTIONNALIT\x00C9S", cx, cy, cw, 24, CLR_ACCENT, g_fontSection, 0);
    cy += 34;
    CreateLabel(hWnd, IDC_LBL_DESC_BASE+1,
        L"\x2022  G\x00E9rer le cache FiveM (normal et extr\x00EAme)\r\n"
        L"\x2022  Activer / d\x00E9sactiver les mods et plugins\r\n"
        L"\x2022  Choisir le game build\r\n"
        L"\x2022  G\x00E9rer vos serveurs favoris (IP:Port ou cfx.re)",
        cx, cy, cw, 90, CLR_TEXT_SEC, g_fontNormal, 0);

    /* Set FiveM status */
    if (g_fivemInstalled) {
        SetWindowTextW(g_hFivemStatus, L"\x25CF  FiveM d\x00E9tect\x00E9");
        SetProp(g_hFivemStatus, L"FG", (HANDLE)(ULONG_PTR)CLR_SUCCESS);
        SetWindowTextW(g_hFivemPath, g_fivemExePath);
    } else {
        SetWindowTextW(g_hFivemStatus, L"\x25CF  FiveM non trouv\x00E9");
        SetProp(g_hFivemStatus, L"FG", (HANDLE)(ULONG_PTR)CLR_DANGER);
        SetWindowTextW(g_hFivemPath, L"Chemin attendu : %localappdata%\\FiveM\\FiveM.exe");
    }

    /* ====== TAB 1: Cache ====== */
    cy = CONTENT_Y + 20;
    CreateLabel(hWnd, IDC_LBL_SECTION_BASE+3, L"GESTION DU CACHE", cx, cy, cw, 24, CLR_ACCENT, g_fontSection, 1);
    cy += 38;
    g_hCacheSize = CreateLabel(hWnd, IDC_LBL_CACHE_SIZE, L"Calcul en cours...", cx, cy, cw, 20, CLR_TEXT, g_fontNormal, 1);
    cy += 40;
    CreateLabel(hWnd, IDC_LBL_DESC_BASE+2,
        L"Supprime les donn\x00E9es temporaires (cache, server-cache, nui-storage...).\n"
        L"Le dossier game-storage sera pr\x00E9serv\x00E9.",
        cx, cy, cw, 40, CLR_TEXT_SEC, g_fontSmall, 1);
    cy += 50;
    CreateStyledButton(hWnd, IDC_BTN_CACHE_NORMAL,
        L"  Vider le cache  \x2014  Normal", cx, cy, 340, 44, BTN_SUCCESS, 1);
    cy += 65;
    CreateLabel(hWnd, IDC_LBL_DESC_BASE+3,
        L"\x26A0  ATTENTION : Supprime TOUT y compris game-storage.\n"
        L"Vous pourriez perdre des configurations.",
        cx, cy, cw, 40, CLR_DANGER, g_fontSmall, 1);
    cy += 48;
    CreateStyledButton(hWnd, IDC_BTN_CACHE_EXTREME,
        L"  \x26A0  Vider le cache  \x2014  Extr\x00EAme", cx, cy, 340, 44, BTN_DANGER, 1);

    /* ====== TAB 2: Mods & Plugins ====== */
    cy = CONTENT_Y + 20;
    CreateLabel(hWnd, IDC_LBL_SECTION_BASE+4, L"MODS", cx, cy, cw, 24, CLR_ACCENT, g_fontSection, 2);
    cy += 34;
    CreateLabel(hWnd, IDC_LBL_DESC_BASE+4,
        L"Dossier : FiveM Application Data\\mods",
        cx, cy, cw, 18, CLR_TEXT_SEC, g_fontSmall, 2);
    cy += 28;
    g_hModsStatus = CreateLabel(hWnd, IDC_LBL_MODS_STATUS, L"", cx, cy, cw, 20, CLR_TEXT, g_fontNormal, 2);
    cy += 32;
    g_hBtnModsToggle = CreateStyledButton(hWnd, IDC_BTN_MODS_TOGGLE, L"", cx, cy, 280, 42, BTN_SECONDARY, 2);

    cy += 70;
    CreateLabel(hWnd, IDC_LBL_SECTION_BASE+5, L"PLUGINS", cx, cy, cw, 24, CLR_ACCENT, g_fontSection, 2);
    cy += 34;
    CreateLabel(hWnd, IDC_LBL_DESC_BASE+5,
        L"Dossier : FiveM Application Data\\plugins",
        cx, cy, cw, 18, CLR_TEXT_SEC, g_fontSmall, 2);
    cy += 28;
    g_hPluginsStatus = CreateLabel(hWnd, IDC_LBL_PLUGINS_STATUS, L"", cx, cy, cw, 20, CLR_TEXT, g_fontNormal, 2);
    cy += 32;
    g_hBtnPluginsToggle = CreateStyledButton(hWnd, IDC_BTN_PLUGINS_TOGGLE, L"", cx, cy, 280, 42, BTN_SECONDARY, 2);

    RefreshModsUI();
    RefreshPluginsUI();

    /* ====== TAB 3: Param\x00E8tres ====== */
    cy = CONTENT_Y + 20;
    CreateLabel(hWnd, IDC_LBL_SECTION_BASE+6, L"PARAM\x00C8TRES", cx, cy, cw, 24, CLR_ACCENT, g_fontSection, 3);
    cy += 38;
    CreateLabel(hWnd, IDC_LBL_DESC_BASE+6, L"S\x00E9lectionnez le build avec lequel lancer FiveM :", cx, cy, cw, 20, CLR_TEXT, g_fontNormal, 3);
    cy += 36;
    g_hBuildCombo = CreateWindowExW(0, L"COMBOBOX", NULL,
        WS_CHILD | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL,
        cx, cy, 380, 400,
        hWnd, (HMENU)IDC_CMB_BUILD, g_hInstance, NULL);
    SendMessageW(g_hBuildCombo, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);
    for (int i = 0; i < BUILD_COUNT; i++) SendMessageW(g_hBuildCombo, CB_ADDSTRING, 0, (LPARAM)g_buildItems[i]);
    SendMessageW(g_hBuildCombo, CB_SETCURSEL, g_selectedBuildIndex, 0);
    AddTabControl(3, g_hBuildCombo);

    cy += 60;
    CreateLabel(hWnd, IDC_LBL_DESC_BASE+7, L"D\x00E9lai d'attente avant connexion auto (s) :", cx, cy, cw, 20, CLR_TEXT, g_fontNormal, 3);
    cy += 30;
    g_hDelayCombo = CreateWindowExW(0, L"COMBOBOX", NULL,
        WS_CHILD | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL,
        cx, cy, 380, 200, hWnd, (HMENU)IDC_CMB_DELAY, g_hInstance, NULL);
    SendMessageW(g_hDelayCombo, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);
    const wchar_t* delays[] = { L"5 s", L"10 s", L"15 s", L"20 s", L"25 s", L"30 s", L"45 s", L"60 s" };
    const int vals[] = { 5, 10, 15, 20, 25, 30, 45, 60 };
    for (int i = 0; i < 8; i++) {
        SendMessageW(g_hDelayCombo, CB_ADDSTRING, 0, (LPARAM)delays[i]);
        if (g_connectDelaySeconds == vals[i]) SendMessageW(g_hDelayCombo, CB_SETCURSEL, i, 0);
    }
    AddTabControl(3, g_hDelayCombo);

    /* ====== TAB 4: Serveurs ====== */
    cy = CONTENT_Y + 20;
    CreateLabel(hWnd, IDC_LBL_SECTION_BASE+7, L"SERVEURS FAVORIS", cx, cy, cw, 24, CLR_ACCENT, g_fontSection, 4);
    cy += 34;

    g_hServerList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOTIFY | WS_VSCROLL,
        cx, cy, cw, 168,
        hWnd, (HMENU)IDC_LIST_SERVERS, g_hInstance, NULL);
    SendMessageW(g_hServerList, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);
    AddTabControl(4, g_hServerList);
    RefreshServerList();

    cy += 178;
    g_hBtnConnect = CreateStyledButton(hWnd, IDC_BTN_SRV_CONNECT, L"  Connecter", cx, cy, 160, 38, BTN_PRIMARY, 4);
    CreateStyledButton(hWnd, IDC_BTN_SRV_DELETE, L"  Supprimer", cx + 175, cy, 160, 38, BTN_DANGER, 4);

    cy += 58;
    CreateLabel(hWnd, IDC_LBL_SECTION_BASE+8, L"AJOUTER UN SERVEUR", cx, cy, cw, 24, CLR_ACCENT, g_fontSection, 4);
    cy += 34;

    CreateLabel(hWnd, IDC_LBL_DESC_BASE+8, L"Nom :", cx, cy + 3, 60, 20, CLR_TEXT_SEC, g_fontNormal, 4);
    g_hSrvNameEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
        WS_CHILD | ES_AUTOHSCROLL, cx + 72, cy, 300, 26,
        hWnd, (HMENU)IDC_EDIT_SRV_NAME, g_hInstance, NULL);
    SendMessageW(g_hSrvNameEdit, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);
    AddTabControl(4, g_hSrvNameEdit);

    cy += 34;
    CreateLabel(hWnd, IDC_LBL_DESC_BASE+9, L"Adresse :", cx, cy + 3, 70, 20, CLR_TEXT_SEC, g_fontNormal, 4);
    g_hSrvAddrEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
        WS_CHILD | ES_AUTOHSCROLL, cx + 72, cy, 300, 26,
        hWnd, (HMENU)IDC_EDIT_SRV_ADDR, g_hInstance, NULL);
    SendMessageW(g_hSrvAddrEdit, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);
    SendMessageW(g_hSrvAddrEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"IP:Port ou code cfx.re");
    AddTabControl(4, g_hSrvAddrEdit);

    cy += 38;
    CreateStyledButton(hWnd, IDC_BTN_SRV_ADD, L"  + Ajouter", cx, cy, 160, 38, BTN_SECONDARY, 4);

    /* ====== FOOTER: Launch Button (always visible) ====== */
    g_hBtnLaunch = CreateStyledButton(hWnd, IDC_BTN_LAUNCH,
        L"\x25B6   LANCER FIVEM", cx, WINDOW_H - FOOTER_H + 14, cw, 48, BTN_PRIMARY, -1);
    ShowWindow(g_hBtnLaunch, SW_SHOW);

    /* Show only the active tab's controls */
    SwitchTab(0);
}

/* ============================================================
 *   DRAWING: WM_PAINT (Header, Tab Bar, Footer, Separators)
 * ============================================================ */

static void PaintMainWindow(HWND hWnd, HDC hdc) {
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int clientW = clientRect.right;
    int clientH = clientRect.bottom;

    /* Create memory DC for double buffering */
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, clientW, clientH);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    /* ---- Background ---- */
    HBRUSH bgBrush = CreateSolidBrush(CLR_BG);
    FillRect(memDC, &clientRect, bgBrush);
    DeleteObject(bgBrush);

    /* ---- Header (gradient) ---- */
    TRIVERTEX vert[2];
    GRADIENT_RECT gRect = { 0, 1 };
    vert[0].x = 0; vert[0].y = 0;
    vert[0].Red = 0x1800; vert[0].Green = 0x1C00; vert[0].Blue = 0x2600;
    vert[0].Alpha = 0xFF00;
    vert[1].x = clientW; vert[1].y = HEADER_H;
    vert[1].Red = 0x0D00; vert[1].Green = 0x1100; vert[1].Blue = 0x1700;
    vert[1].Alpha = 0xFF00;
    GradientFill(memDC, vert, 2, &gRect, 1, GRADIENT_FILL_RECT_V);

    /* Header title */
    SetBkMode(memDC, TRANSPARENT);
    HFONT oldFont = (HFONT)SelectObject(memDC, g_fontTitle);
    SetTextColor(memDC, CLR_TEXT);
    RECT titleRect = { PADDING, 0, clientW - 120, HEADER_H };
    DrawTextW(memDC, L"FIVEM LAUNCHER", -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    /* Version text */
    SelectObject(memDC, g_fontSmall);
    SetTextColor(memDC, CLR_TEXT_SEC);
    RECT verRect = { clientW - 110, 0, clientW - PADDING, HEADER_H };
    DrawTextW(memDC, L"v1.0", -1, &verRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    /* Header accent line */
    HPEN accentPen = CreatePen(PS_SOLID, 2, CLR_ACCENT);
    HPEN oldPen = (HPEN)SelectObject(memDC, accentPen);
    MoveToEx(memDC, 0, HEADER_H - 1, NULL);
    LineTo(memDC, clientW, HEADER_H - 1);
    SelectObject(memDC, oldPen);
    DeleteObject(accentPen);

    /* ---- Tab Bar ---- */
    RECT tabBarRect = { 0, HEADER_H, clientW, HEADER_H + TAB_H };
    HBRUSH tabBarBrush = CreateSolidBrush(CLR_TAB_BG);
    FillRect(memDC, &tabBarRect, tabBarBrush);
    DeleteObject(tabBarBrush);

    int tabW = clientW / TAB_COUNT;
    for (int i = 0; i < TAB_COUNT; i++) {
        RECT trc = { i * tabW, HEADER_H, (i + 1) * tabW, HEADER_H + TAB_H };

        /* Tab background on hover/active */
        if (i == g_activeTab) {
            HBRUSH ab = CreateSolidBrush(CLR_TAB_ACTIVE);
            FillRect(memDC, &trc, ab);
            DeleteObject(ab);
        } else if (i == g_hoveredTab) {
            HBRUSH hb = CreateSolidBrush(CLR_TAB_HOVER);
            FillRect(memDC, &trc, hb);
            DeleteObject(hb);
        }

        /* Tab text */
        SelectObject(memDC, g_fontTab);
        SetTextColor(memDC, (i == g_activeTab) ? CLR_TEXT : CLR_TEXT_SEC);
        DrawTextW(memDC, g_tabNames[i], -1, &trc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        /* Active indicator (orange bottom line) */
        if (i == g_activeTab) {
            RECT indicator = { trc.left + 12, trc.bottom - 3, trc.right - 12, trc.bottom };
            HBRUSH indBrush = CreateSolidBrush(CLR_ACCENT);
            FillRect(memDC, &indicator, indBrush);
            DeleteObject(indBrush);
        }
    }

    /* Tab bar bottom border */
    HPEN borderPen = CreatePen(PS_SOLID, 1, CLR_BORDER);
    oldPen = (HPEN)SelectObject(memDC, borderPen);
    MoveToEx(memDC, 0, HEADER_H + TAB_H, NULL);
    LineTo(memDC, clientW, HEADER_H + TAB_H);
    SelectObject(memDC, oldPen);
    DeleteObject(borderPen);

    /* ---- Content area separators per tab ---- */
    if (g_activeTab == 2) {
        int sepY = CONTENT_Y + 20 + 34 + 28 + 20 + 32 + 42 + 20;
        HPEN sPen = CreatePen(PS_SOLID, 1, CLR_BORDER);
        oldPen = (HPEN)SelectObject(memDC, sPen);
        MoveToEx(memDC, PADDING, sepY, NULL);
        LineTo(memDC, clientW - PADDING, sepY);
        SelectObject(memDC, oldPen);
        DeleteObject(sPen);
    }

    /* ---- Footer ---- */
    RECT footerRect = { 0, clientH - FOOTER_H, clientW, clientH };
    HBRUSH footerBrush = CreateSolidBrush(CLR_FOOTER);
    FillRect(memDC, &footerRect, footerBrush);
    DeleteObject(footerBrush);

    /* Footer top border */
    HPEN fPen = CreatePen(PS_SOLID, 1, CLR_BORDER);
    oldPen = (HPEN)SelectObject(memDC, fPen);
    MoveToEx(memDC, 0, clientH - FOOTER_H, NULL);
    LineTo(memDC, clientW, clientH - FOOTER_H);
    SelectObject(memDC, oldPen);
    DeleteObject(fPen);

    SelectObject(memDC, oldFont);

    /* Blit to screen */
    BitBlt(hdc, 0, 0, clientW, clientH, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

/* ============================================================
 *   WINDOW PROCEDURE
 * ============================================================ */

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_CREATE: {
        /* Create GDI objects */
        g_fontTitle   = CreateFontW(-24, 0, 0, 0, FW_BOLD,   0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        g_fontSection = CreateFontW(-16, 0, 0, 0, FW_BOLD,   0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        g_fontBold    = CreateFontW(-14, 0, 0, 0, FW_BOLD,   0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        g_fontTab     = CreateFontW(-13, 0, 0, 0, FW_MEDIUM, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        g_fontNormal  = CreateFontW(-14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        g_fontButton  = CreateFontW(-14, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI Semibold");
        g_fontSmall   = CreateFontW(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");

        g_hBrushBg    = CreateSolidBrush(CLR_BG);
        g_hBrushInput = CreateSolidBrush(CLR_INPUT_BG);
        g_hBrushPanel = CreateSolidBrush(CLR_PANEL);

        CreateAllControls(hWnd);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; /* Handled in WM_PAINT */

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        PaintMainWindow(hWnd, hdc);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;
        ULONG_PTR fg = (ULONG_PTR)GetProp(hCtrl, L"FG");
        SetTextColor(hdcStatic, fg ? (COLORREF)fg : CLR_TEXT);
        SetBkColor(hdcStatic, CLR_BG);
        return (LRESULT)g_hBrushBg;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = (HDC)wParam;
        SetTextColor(hdcEdit, CLR_TEXT);
        SetBkColor(hdcEdit, CLR_INPUT_BG);
        return (LRESULT)g_hBrushInput;
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdcList = (HDC)wParam;
        SetTextColor(hdcList, CLR_TEXT);
        SetBkColor(hdcList, CLR_BG);
        return (LRESULT)g_hBrushBg;
    }

    case WM_MEASUREITEM: {
        LPMEASUREITEMSTRUCT mis = (LPMEASUREITEMSTRUCT)lParam;
        if (mis->CtlID == IDC_LIST_SERVERS) {
            mis->itemHeight = 48;
        } else if (mis->CtlID == IDC_CMB_BUILD || mis->CtlID == IDC_CMB_DELAY) {
            mis->itemHeight = 32;
        }
        return TRUE;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        switch (dis->CtlType) {
            case ODT_BUTTON:
                DrawCustomButton(dis);
                return TRUE;
            case ODT_LISTBOX:
                if (dis->CtlID == IDC_LIST_SERVERS)
                    DrawServerItem(dis);
                return TRUE;
            case ODT_COMBOBOX:
                if (dis->CtlID == IDC_CMB_BUILD || dis->CtlID == IDC_CMB_DELAY)
                    DrawBuildComboItem(dis);
                return TRUE;
        }
        break;
    }

    case WM_MOUSEMOVE: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        if (!g_trackingMouse) {
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
            TrackMouseEvent(&tme);
            g_trackingMouse = TRUE;
        }

        /* Tab hover */
        int newHover = -1;
        if (y >= HEADER_H && y < HEADER_H + TAB_H) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            int tabW = rc.right / TAB_COUNT;
            newHover = x / tabW;
            if (newHover >= TAB_COUNT) newHover = TAB_COUNT - 1;
        }

        if (newHover != g_hoveredTab) {
            g_hoveredTab = newHover;
            RECT rc;
            GetClientRect(hWnd, &rc);
            RECT tabBarRect = { 0, HEADER_H, rc.right, HEADER_H + TAB_H };
            InvalidateRect(hWnd, &tabBarRect, FALSE);
        }

        /* Set cursor */
        if (newHover >= 0) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        g_trackingMouse = FALSE;
        if (g_hoveredTab != -1) {
            g_hoveredTab = -1;
            RECT rc;
            GetClientRect(hWnd, &rc);
            RECT tabBarRect = { 0, HEADER_H, rc.right, HEADER_H + TAB_H };
            InvalidateRect(hWnd, &tabBarRect, FALSE);
        }
        return 0;

    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        if (y >= HEADER_H && y < HEADER_H + TAB_H) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            int tabW = rc.right / TAB_COUNT;
            int clickedTab = x / tabW;
            if (clickedTab >= TAB_COUNT) clickedTab = TAB_COUNT - 1;
            if (clickedTab != g_activeTab) {
                SwitchTab(clickedTab);
            }
        }
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int notif = HIWORD(wParam);

        switch (id) {
            case IDC_BTN_LAUNCH:
                LaunchFiveM();
                break;

            case IDC_BTN_CACHE_NORMAL: {
                int res = MessageBoxW(hWnd,
                    L"Voulez-vous vider le cache (mode normal) ?\n\n"
                    L"Le dossier game-storage sera pr\x00E9serv\x00E9.",
                    L"Confirmer", MB_YESNO | MB_ICONQUESTION);
                if (res == IDYES) {
                    if (GetFileAttributesW(g_fivemDataPath) == INVALID_FILE_ATTRIBUTES) {
                        MessageBoxW(hWnd, L"Dossier data non trouv\x00E9.", L"Erreur", MB_OK | MB_ICONERROR);
                        break;
                    }
                    WIN32_FIND_DATAW fd;
                    wchar_t search[MAX_PATH];
                    swprintf(search, MAX_PATH, L"%s\\*", g_fivemDataPath);
                    HANDLE hFind = FindFirstFileW(search, &fd);
                    if (hFind != INVALID_HANDLE_VALUE) {
                        do {
                            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
                            if (_wcsicmp(fd.cFileName, L"game-storage") == 0) continue;
                            wchar_t fullPath[MAX_PATH];
                            swprintf(fullPath, MAX_PATH, L"%s\\%s", g_fivemDataPath, fd.cFileName);
                            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                DeleteDirectoryRecursive(fullPath);
                            else {
                                SetFileAttributesW(fullPath, FILE_ATTRIBUTE_NORMAL);
                                DeleteFileW(fullPath);
                            }
                        } while (FindNextFileW(hFind, &fd));
                        FindClose(hFind);
                    }
                    RefreshCacheSize();
                    MessageBoxW(hWnd, L"Cache vid\x00E9 avec succ\x00E8s !\n(game-storage pr\x00E9serv\x00E9)",
                        L"Succ\x00E8s", MB_OK | MB_ICONINFORMATION);
                }
                break;
            }

            case IDC_BTN_CACHE_EXTREME: {
                int res = MessageBoxW(hWnd,
                    L"\x26A0 ATTENTION \x26A0\n\n"
                    L"Ceci va supprimer TOUT le contenu du dossier data,\n"
                    L"y compris game-storage !\n\n"
                    L"Vous pourriez perdre des configurations.\n\n"
                    L"Continuer ?",
                    L"Confirmer - Mode Extr\x00EAme", MB_YESNO | MB_ICONWARNING);
                if (res == IDYES) {
                    if (GetFileAttributesW(g_fivemDataPath) == INVALID_FILE_ATTRIBUTES) {
                        MessageBoxW(hWnd, L"Dossier data non trouv\x00E9.", L"Erreur", MB_OK | MB_ICONERROR);
                        break;
                    }
                    WIN32_FIND_DATAW fd;
                    wchar_t search[MAX_PATH];
                    swprintf(search, MAX_PATH, L"%s\\*", g_fivemDataPath);
                    HANDLE hFind = FindFirstFileW(search, &fd);
                    if (hFind != INVALID_HANDLE_VALUE) {
                        do {
                            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
                            wchar_t fullPath[MAX_PATH];
                            swprintf(fullPath, MAX_PATH, L"%s\\%s", g_fivemDataPath, fd.cFileName);
                            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                DeleteDirectoryRecursive(fullPath);
                            else {
                                SetFileAttributesW(fullPath, FILE_ATTRIBUTE_NORMAL);
                                DeleteFileW(fullPath);
                            }
                        } while (FindNextFileW(hFind, &fd));
                        FindClose(hFind);
                    }
                    RefreshCacheSize();
                    MessageBoxW(hWnd, L"Cache vid\x00E9 compl\x00E8tement (mode extr\x00EAme).",
                        L"Succ\x00E8s", MB_OK | MB_ICONINFORMATION);
                }
                break;
            }

            case IDC_BTN_MODS_TOGGLE: {
                FolderStatus status = GetFolderStatus(L"mods", L"disablemods");
                wchar_t src[MAX_PATH], dst[MAX_PATH];
                if (status == FOLDER_ENABLED) {
                    swprintf(src, MAX_PATH, L"%s\\mods", g_fivemAppDataPath);
                    swprintf(dst, MAX_PATH, L"%s\\disablemods", g_fivemAppDataPath);
                    if (MoveFileW(src, dst))
                        MessageBoxW(hWnd, L"Mods d\x00E9sactiv\x00E9s.", L"Succ\x00E8s", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBoxW(hWnd, L"Erreur lors du renommage.", L"Erreur", MB_OK | MB_ICONERROR);
                } else if (status == FOLDER_DISABLED) {
                    swprintf(src, MAX_PATH, L"%s\\disablemods", g_fivemAppDataPath);
                    swprintf(dst, MAX_PATH, L"%s\\mods", g_fivemAppDataPath);
                    if (MoveFileW(src, dst))
                        MessageBoxW(hWnd, L"Mods activ\x00E9s.", L"Succ\x00E8s", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBoxW(hWnd, L"Erreur lors du renommage.", L"Erreur", MB_OK | MB_ICONERROR);
                }
                RefreshModsUI();
                break;
            }

            case IDC_BTN_PLUGINS_TOGGLE: {
                FolderStatus status = GetFolderStatus(L"plugins", L"disableplugins");
                wchar_t src[MAX_PATH], dst[MAX_PATH];
                if (status == FOLDER_ENABLED) {
                    swprintf(src, MAX_PATH, L"%s\\plugins", g_fivemAppDataPath);
                    swprintf(dst, MAX_PATH, L"%s\\disableplugins", g_fivemAppDataPath);
                    if (MoveFileW(src, dst))
                        MessageBoxW(hWnd, L"Plugins d\x00E9sactiv\x00E9s.", L"Succ\x00E8s", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBoxW(hWnd, L"Erreur lors du renommage.", L"Erreur", MB_OK | MB_ICONERROR);
                } else if (status == FOLDER_DISABLED) {
                    swprintf(src, MAX_PATH, L"%s\\disableplugins", g_fivemAppDataPath);
                    swprintf(dst, MAX_PATH, L"%s\\plugins", g_fivemAppDataPath);
                    if (MoveFileW(src, dst))
                        MessageBoxW(hWnd, L"Plugins activ\x00E9s.", L"Succ\x00E8s", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBoxW(hWnd, L"Erreur lors du renommage.", L"Erreur", MB_OK | MB_ICONERROR);
                }
                RefreshPluginsUI();
                break;
            }

            case IDC_CMB_BUILD:
                if (notif == CBN_SELCHANGE) {
                    g_selectedBuildIndex = (int)SendMessageW(g_hBuildCombo, CB_GETCURSEL, 0, 0);
                    ApplyBuildSetting();
                    SaveSettings();
                }
                break;

            case IDC_CMB_DELAY:
                if (notif == CBN_SELCHANGE) {
                    int idx = (int)SendMessageW(g_hDelayCombo, CB_GETCURSEL, 0, 0);
                    const int vals[] = { 5, 10, 15, 20, 25, 30, 45, 60 };
                    if (idx >= 0 && idx < 8) {
                        g_connectDelaySeconds = vals[idx];
                        SaveSettings();
                    }
                }
                break;

            case IDC_BTN_SRV_CONNECT: {
                int sel = (int)SendMessageW(g_hServerList, LB_GETCURSEL, 0, 0);
                if (sel == LB_ERR) {
                    MessageBoxW(hWnd, L"S\x00E9lectionnez un serveur.", L"Info", MB_OK | MB_ICONINFORMATION);
                    break;
                }
                int idx = (int)SendMessageW(g_hServerList, LB_GETITEMDATA, sel, 0);
                if (idx >= 0 && idx < g_serverCount) {
                    ConnectToServer(g_servers[idx].address);
                }
                break;
            }

            case IDC_BTN_SRV_DELETE: {
                int sel = (int)SendMessageW(g_hServerList, LB_GETCURSEL, 0, 0);
                if (sel == LB_ERR) {
                    MessageBoxW(hWnd, L"S\x00E9lectionnez un serveur \x00E0 supprimer.", L"Info", MB_OK | MB_ICONINFORMATION);
                    break;
                }
                int idx = (int)SendMessageW(g_hServerList, LB_GETITEMDATA, sel, 0);
                if (idx >= 0 && idx < g_serverCount) {
                    wchar_t msg[512];
                    swprintf(msg, 512, L"Supprimer \"%s\" ?", g_servers[idx].name);
                    if (MessageBoxW(hWnd, msg, L"Confirmer", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        /* Remove from array */
                        for (int i = idx; i < g_serverCount - 1; i++) {
                            g_servers[i] = g_servers[i + 1];
                        }
                        g_serverCount--;
                        SaveServers();
                        RefreshServerList();
                    }
                }
                break;
            }

            case IDC_BTN_SRV_ADD: {
                wchar_t name[128], addr[256];
                GetWindowTextW(g_hSrvNameEdit, name, 128);
                GetWindowTextW(g_hSrvAddrEdit, addr, 256);

                if (wcslen(name) == 0 || wcslen(addr) == 0) {
                    MessageBoxW(hWnd, L"Remplissez le nom et l'adresse.", L"Info", MB_OK | MB_ICONINFORMATION);
                    break;
                }
                if (g_serverCount >= MAX_SERVERS) {
                    MessageBoxW(hWnd, L"Nombre maximum de serveurs atteint.", L"Erreur", MB_OK | MB_ICONWARNING);
                    break;
                }
                wcscpy(g_servers[g_serverCount].name, name);
                wcscpy(g_servers[g_serverCount].address, addr);
                g_serverCount++;
                SaveServers();
                RefreshServerList();
                SetWindowTextW(g_hSrvNameEdit, L"");
                SetWindowTextW(g_hSrvAddrEdit, L"");
                break;
            }
        }
        return 0;
    }

    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            if (pt.y >= HEADER_H && pt.y < HEADER_H + TAB_H) {
                SetCursor(LoadCursor(NULL, IDC_HAND));
                return TRUE;
            }
        }
        break;
    }

    case WM_DESTROY:
        DeleteObject(g_fontTitle);
        DeleteObject(g_fontSection);
        DeleteObject(g_fontTab);
        DeleteObject(g_fontNormal);
        DeleteObject(g_fontButton);
        DeleteObject(g_fontSmall);
        DeleteObject(g_hBrushBg);
        DeleteObject(g_hBrushInput);
        DeleteObject(g_hBrushPanel);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

/* ============================================================
 *   ENTRY POINT
 * ============================================================ */

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance; (void)lpCmdLine;
    g_hInstance = hInstance;

    /* Initialize common controls */
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icc);

    /* Detect FiveM and load config */
    DetectFiveMPaths();
    LoadSettings();
    LoadServers();

    /* Register window class */
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(CLR_BG);
    wc.lpszClassName = L"FiveMlauncher";
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    /* Calculate window rect for exact client area */
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT wr = { 0, 0, WINDOW_W, WINDOW_H };
    AdjustWindowRect(&wr, style, FALSE);

    /* Create main window */
    g_hMainWnd = CreateWindowExW(
        0, L"FiveMlauncher", L"FiveM Launcher",
        style, CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL, NULL, hInstance, NULL);

    if (!g_hMainWnd) return 1;

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    /* Message loop */
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
