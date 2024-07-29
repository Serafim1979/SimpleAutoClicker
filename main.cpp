/*********************************************************************
* Description: It is an application for automatic mouse clicking on 
* Windows with a graphical interface implemented using WinAPI.
* The user can enter the number of clicks, the interval between clicks, 
* and the coordinates where clicks should occur. 
* The application also displays the current mouse coordinates.
*
* Author: Ivan Korolkov
* Date of creation: July 29, 2024
*
* License: MIT License
*
*********************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <iostream>

// Global variables
HWND g_hWnd;
HWND g_hStatusLabel;
HWND g_hClicksEdit;
HWND g_hIntervalEdit;
HWND g_hMousePosLabel;
HWND g_hXCoordEdit;
HWND g_hYCoordEdit;
HHOOK g_hMouseHook = NULL;
int g_remainingClicks = 0;

// Function declarations
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UpdateStatus(int totalClicks, int remainingClicks);
void AutoClicker(int totalClicks, int intervalMillis, int x, int y);
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
void Click();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSA wc = { 0 };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ClickerApp";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassA(&wc);

    // Create main window
    g_hWnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        "Auto Clicker",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, hInstance, NULL
    );

    // Create UI elements
    g_hStatusLabel = CreateWindowA("STATIC", "Clicks: 0", WS_VISIBLE | WS_CHILD, 10, 10, 200, 20, g_hWnd, NULL, hInstance, NULL);
    CreateWindowA("STATIC", "Total Clicks:", WS_VISIBLE | WS_CHILD, 170, 40, 100, 20, g_hWnd, NULL, hInstance, NULL);
    g_hClicksEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 270, 40, 100, 20, g_hWnd, NULL, hInstance, NULL);
    CreateWindowA("STATIC", "Interval (ms):", WS_VISIBLE | WS_CHILD, 170, 70, 100, 20, g_hWnd, NULL, hInstance, NULL);
    g_hIntervalEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 270, 70, 100, 20, g_hWnd, NULL, hInstance, NULL);
    CreateWindowA("STATIC", "X Coord:", WS_VISIBLE | WS_CHILD, 170, 130, 100, 20, g_hWnd, NULL, hInstance, NULL);
    g_hXCoordEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 270, 130, 100, 20, g_hWnd, NULL, hInstance, NULL);
    CreateWindowA("STATIC", "Y Coord:", WS_VISIBLE | WS_CHILD, 170, 160, 100, 20, g_hWnd, NULL, hInstance, NULL);
    g_hYCoordEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 270, 160, 100, 20, g_hWnd, NULL, hInstance, NULL);
    CreateWindowA("BUTTON", "Start", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 200, 100, 30, g_hWnd, (HMENU)1, hInstance, NULL);
    g_hMousePosLabel = CreateWindowA("STATIC", "", WS_VISIBLE | WS_CHILD, 10, 130, 150, 40, g_hWnd, NULL, hInstance, NULL);

    ShowWindow(g_hWnd, nCmdShow);

    // Set mouse hook to track mouse position
    g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, hInstance, 0);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(g_hMouseHook);

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == 1)
        {
            char clicksBuffer[256];
            GetWindowTextA(g_hClicksEdit, clicksBuffer, 256);
            int TotalClicks = 0;
            try {
                TotalClicks = std::stoi(clicksBuffer);
            }
            catch (...) {
                MessageBoxA(g_hWnd, "Invalid number of clicks", "Error", MB_OK | MB_ICONERROR);
                break;
            }

            char intervalBuffer[256];
            GetWindowTextA(g_hIntervalEdit, intervalBuffer, 256);
            int intervalMillis = 0;
            try {
                intervalMillis = std::stoi(intervalBuffer);
            }
            catch (...) {
                MessageBoxA(g_hWnd, "Invalid interval", "Error", MB_OK | MB_ICONERROR);
                break;
            }

            char xCoordBuffer[256];
            GetWindowTextA(g_hXCoordEdit, xCoordBuffer, 256);
            int xCoord = 0;
            try {
                xCoord = std::stoi(xCoordBuffer);
            }
            catch (...) {
                MessageBoxA(g_hWnd, "Invalid X coordinate", "Error", MB_OK | MB_ICONERROR);
                break;
            }

            char yCoordBuffer[256];
            GetWindowTextA(g_hYCoordEdit, yCoordBuffer, 256);
            int yCoord = 0;
            try {
                yCoord = std::stoi(yCoordBuffer);
            }
            catch (...) {
                MessageBoxA(g_hWnd, "Invalid Y coordinate", "Error", MB_OK | MB_ICONERROR);
                break;
            }

            // Start AutoClicker in a separate thread
            std::thread autoClickerThread(AutoClicker, TotalClicks, intervalMillis, xCoord, yCoord);
            autoClickerThread.detach();
        }
    }
    break;

    case WM_CLOSE:
    {
        DestroyWindow(hWnd);
    }
    break;

    case WM_DESTROY:
    {
        PostQuitMessage(0);
    }
    break;

    default:
    {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    }
    return 0;
}

void AutoClicker(int totalClicks, int intervalMillis, int x, int y)
{
    g_remainingClicks = totalClicks;

    POINT cursorPos;
    GetCursorPos(&cursorPos);

    SetCursorPos(x, y);

    for (int i = 0; i < totalClicks; ++i)
    {
        g_remainingClicks--;
        UpdateStatus(totalClicks, g_remainingClicks);
        Click();
        Sleep(intervalMillis);  // Correct sleep interval
    }

    SetCursorPos(cursorPos.x, cursorPos.y);

    g_remainingClicks = 0;
    UpdateStatus(totalClicks, g_remainingClicks);
}

void Click()
{
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));

    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void UpdateStatus(int totalClicks, int remainingClicks)
{
    std::ostringstream oss;
    oss << "Clicks: " << totalClicks << " (Remaining: " << remainingClicks << ")";
    SetWindowTextA(g_hStatusLabel, oss.str().c_str());
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && wParam == WM_MOUSEMOVE)
    {
        MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
        int xPos = pMouseStruct->pt.x;
        int yPos = pMouseStruct->pt.y;

        std::ostringstream oss;
        oss << "Mouse Pos: (" << xPos << ", " << yPos << ")";
        SetWindowTextA(g_hMousePosLabel, oss.str().c_str());
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}