#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <fmt/format.h>
#include <string>
#include <Windows.h>

#include <cstdint>
#include <cassert>
#include <algorithm>
#include <array>

#include <winrt/base.h>

#include <d3d12.h>
#include <dxgi1_6.h>

namespace Helpers
{
    inline void AssertIfFailed(HRESULT hr)
    {
        assert(FAILED(hr));
    }
}

namespace Window
{
    struct Window
    {
        static constexpr std::wstring_view CLASSNAME = L"Dx12Lesson1";
        static constexpr DWORD window_style = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,
                               window_style_ex = WS_EX_OVERLAPPEDWINDOW;

        Window() = delete;

        Window(int width, int height, const std::wstring_view &title)
        {
            Register();
            
            RECT rect {0, 0, width, height};
            AdjustWindowRectEx(&rect, window_style, NULL, window_style_ex);

            Create(rect, title);
        }

        ~Window()
        {
            Destroy();
            UnRegister();
        }

        static LRESULT CALLBACK window_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            if (msg == WM_NCCREATE)
            {
                uint64_t windowPtr = reinterpret_cast<uint64_t>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
                SetWindowLongPtr(hWnd,
                                    GWLP_USERDATA,
                                    windowPtr);
            }

            auto wnd = reinterpret_cast<Window *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            if (wnd)
            {
                return wnd->message_handler(hWnd, msg, wParam, lParam);
            }
            else
            {
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
        }

        void Register()
        {
            WNDCLASSEX wc{};
            wc.cbSize = sizeof(WNDCLASSEX);
            wc.lpszClassName = CLASSNAME.data();
            wc.hInstance = GetModuleHandle(nullptr);
            wc.lpfnWndProc = Window::window_proc;
            wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));

            RegisterClassEx(&wc);
        }

        void UnRegister()
        {
            HINSTANCE hInstance = GetModuleHandle(nullptr);
            UnregisterClass(CLASSNAME.data(), hInstance);
        }

        void Create(RECT windowRect, const std::wstring &title)
        {
            auto[x, y, w, h] = windowRect;
            w = w - x;
            h = h - y;
            x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
            y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

            m_hWnd = CreateWindowEx(window_style_ex,
                                    CLASSNAME.data(),
                                    title.data(),
                                    window_style,
                                    x, y, w, h,
                                    nullptr,
                                    nullptr,
                                    GetModuleHandle(nullptr),
                                    static_cast<LPVOID>(this));
        }

        void Destroy()
        {
            if (m_hWnd)
            {
                DestroyWindow(m_hWnd);
                m_hWnd = nullptr;
            }
        }

        LRESULT CALLBACK message_handler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            switch (msg)
            {
                case WM_DESTROY:
                {
                    Destroy();
                    PostQuitMessage(NULL);
                    return 0;
                }
            }

            return DefWindowProc(hWnd, msg, wParam, lParam);
        }

        void Show()
        {
            ShowWindow(m_hWnd, SW_SHOWNORMAL);
            SetFocus(m_hWnd);
        }

        void ProcessMessages()
        {
            BOOL has_more_messages = TRUE;
            while (has_more_messages)
            {
                MSG msg{};

                // Parameter two here has to be nullptr, putting hWnd here will
                // not retrive WM_QUIT messages, as those are posted to the thread
                // and not the window
                has_more_messages = PeekMessage(&msg, nullptr, NULL, NULL, PM_REMOVE);
                if (msg.message == WM_QUIT)
                {
                    return;
                }
                else
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }

        HWND m_hWnd{};
    };
}

namespace Lesson1
{

}

int main()
{
    fmt::print("Direct X 12 - Lesson 1\n");

    constexpr std::wstring_view wnd_title { L"Test Window" };
    constexpr int wnd_width { 1280 },
                  wnd_height { wnd_width * 10 / 16 };

    fmt::print(L"Window: \n\t Title: {} \n\t Width: {} \n\t Height: {} \n",
                wnd_title, wnd_width, wnd_height);

    Window::Window wnd(wnd_width, wnd_height, wnd_title);

    Window::Window wnd(600, 400, L"Test Window");

    wnd.Show();

    while(wnd.m_hWnd)
    {
        wnd.ProcessMessages();
    }

}