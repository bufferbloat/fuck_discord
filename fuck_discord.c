#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>
#include <psapi.h>
#include <time.h>
#include <shellapi.h>

#define WM_TRAYICON (WM_USER + 1)
#define HOTKEY_ID 1
#define IDM_SHOW 100
#define IDM_EXIT 101

const char *priorityToString(DWORD p)
{
    switch (p)
    {
    case NORMAL_PRIORITY_CLASS:
        return "Normal";
    case IDLE_PRIORITY_CLASS:
        return "Idle";
    case HIGH_PRIORITY_CLASS:
        return "High";
    case REALTIME_PRIORITY_CLASS:
        return "Realtime";
    case BELOW_NORMAL_PRIORITY_CLASS:
        return "Below normal";
    case ABOVE_NORMAL_PRIORITY_CLASS:
        return "Above normal";

    default:
        return "unknown";
    }
}

const DWORD AboveNormalPriorities = ABOVE_NORMAL_PRIORITY_CLASS | HIGH_PRIORITY_CLASS | REALTIME_PRIORITY_CLASS;

int no_console = 0;
HWND consoleWindow = NULL;
int consoleVisible = 0;
HINSTANCE hInstanceGlobal = NULL;
char** argv = NULL;
int argc = 0;

DWORD WINAPI MonitorThread(LPVOID lpParam)
{
    (void)lpParam;
    for (;;)
    {
        // Get processes
        DWORD processes[1024], cbNeeded, cProcesses;
        if (!EnumProcesses(processes, sizeof(processes), &cbNeeded))
        {
            if (!no_console) printf("Failed to enumerate processes\n");
            return 1;
        }

        cProcesses = cbNeeded / sizeof(DWORD);

        for (DWORD i = 0; i < cProcesses; i++)
        {
            // Get handle
            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processes[i]);
            if (hProcess != NULL)
            {
                // Get the process name
                char szProcessName[MAX_PATH] = "";
                HMODULE hMod;
                DWORD cbNeeded;
                if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
                {
                    GetModuleBaseNameA(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(char));
                }

                // Check if the process name matches "Discord.exe"
                if (strcmp(szProcessName, "Discord.exe") == 0)
                {
                    DWORD priority = GetPriorityClass(hProcess);
                    if (!(priority & AboveNormalPriorities))
                    {
                        CloseHandle(hProcess);
                        continue;
                    }

                    // Set the process priority to normal
                    if (SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS))
                    {
                        if (!no_console)
                        {
                            time_t t = time(NULL);
                            struct tm tm = *localtime(&t);
                            printf("[%02d:%02d:%02d] ", tm.tm_hour, tm.tm_min, tm.tm_sec);
                            // printf("Successfully set priority of Discord.exe (%ld) to normal\n", processes[i]);
                            printf("Discord.exe (%ld) %s -> %s\n", processes[i], priorityToString(priority), priorityToString(NORMAL_PRIORITY_CLASS));
                        }
                    }
                    else
                    {
                        if (!no_console)
                        {
                            printf("Failed to set priority of Discord.exe (%ld) to normal\n", processes[i]);
                        }
                    }
                }

                // Close the process handle
                CloseHandle(hProcess);
            }
        }

        // Sleep for 5 seconds
        Sleep(5000);
    }
    return 0;
}

void ToggleConsole(void)
{
    if (consoleWindow)
    {
        if (consoleVisible)
        {
            ShowWindow(consoleWindow, SW_HIDE);
            consoleVisible = 0;
        }
        else
        {
            ShowWindow(consoleWindow, SW_SHOW);
            SetForegroundWindow(consoleWindow);
            consoleVisible = 1;
        }
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_HOTKEY:
        if (wParam == HOTKEY_ID)
        {
            ToggleConsole();
        }
        break;
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONDOWN)
        {
            // Menu
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuA(hMenu, MF_STRING, IDM_SHOW, "Show");
            AppendMenuA(hMenu, MF_STRING, IDM_EXIT, "Exit");
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        else if (lParam == WM_LBUTTONDBLCLK)
        {
            ToggleConsole();
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDM_SHOW)
        {
            ToggleConsole();
        }
        else if (LOWORD(wParam) == IDM_EXIT)
        {
            PostQuitMessage(0);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    hInstanceGlobal = hInstance;
    
    // Parse args
    LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (szArglist)
    {
        // Alloc
        argv = (char**)malloc(argc * sizeof(char*));
        
        for (int i = 0; i < argc; i++)
        {
            size_t len = WideCharToMultiByte(CP_UTF8, 0, szArglist[i], -1, NULL, 0, NULL, NULL);
            argv[i] = (char*)malloc(len);
            WideCharToMultiByte(CP_UTF8, 0, szArglist[i], -1, argv[i], len, NULL, NULL);
        }
        
        LocalFree(szArglist);
        
        // Check for -no-console flag
        for (int i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], "-no-console") == 0)
            {
                no_console = 1;
            }
        }
    }

    if (!no_console)
    {
        // Create console
        AllocConsole();
        
        SetConsoleTitleA("fuck discord");
        
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        
        printf("Fuck Discord is monitoring process priority..\n");
        printf("Press CTRL + ALT + M to toggle console\n");
        

        consoleWindow = GetConsoleWindow();
        if (consoleWindow)
        {
            ShowWindow(consoleWindow, SW_HIDE);
            consoleVisible = 0;
        }

        // Create window
        WNDCLASS wc = {0};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = "FuckDiscord";
        RegisterClass(&wc);

        HWND hwnd = CreateWindowEx(0, "FuckDiscord", "Fuck Discord", 
                                   0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

        // Tray icon
        NOTIFYICONDATA nid = {0};
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        strcpy(nid.szTip, "Fuck Discord");
        Shell_NotifyIcon(NIM_ADD, &nid);

        // Hotkey
        RegisterHotKey(hwnd, HOTKEY_ID, MOD_CONTROL | MOD_ALT, 'M');

        HANDLE hThread = CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);
        CloseHandle(hThread);

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Cleanup
        Shell_NotifyIcon(NIM_DELETE, &nid);
        
        // Free console
        FreeConsole();
        
        // Free allocated argv memory
        if (argv)
        {
            for (int i = 0; i < argc; i++)
            {
                if (argv[i])
                {
                    free(argv[i]);
                }
            }
            free(argv);
        }
    }
    else
    {
        // No console
        MonitorThread(NULL);
    }

    return 0;
}