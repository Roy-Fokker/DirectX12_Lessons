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
    using namespace Helpers;

    static constexpr uint8_t Num_Frames = 3;

    struct DirectX12
    {
        using Device_t = winrt::com_ptr<ID3D12Device2>;
        using CommandQueue_t = winrt::com_ptr<ID3D12CommandQueue>;
        using SwapChain_t = winrt::com_ptr<IDXGISwapChain4>;
        using Buffer_t = winrt::com_ptr<ID3D12Resource>;
        using CommandList_t = winrt::com_ptr<ID3D12CommandList>;
        using CommandAllocator_t = winrt::com_ptr<ID3D12CommandAllocator>;
        using DescriptorHeap_t = winrt::com_ptr<ID3D12DescriptorHeap>;
        using Fence_t = winrt::com_ptr<ID3D12Fence>;

        DirectX12() = delete;
        DirectX12(HWND hWnd) :
            m_hWnd(hWnd)
        {
            /* Enable Debug Layer */ {
            #ifdef _DEBUG
                winrt::com_ptr<ID3D12Debug> debugInterface;
                auto dihr = D3D12GetDebugInterface(__uuidof(ID3D12Debug), debugInterface.put_void());
                AssertIfFailed(dihr);

                debugInterface->EnableDebugLayer();
            #endif
            }

            winrt::com_ptr<IDXGIFactory4> dxgiFactory;
            /* Create DXGI Factory */ {
                uint32_t factoryFlags{};
            #ifdef _DEBUG
                factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            #endif
                auto dfhr = CreateDXGIFactory2(factoryFlags, 
                                            __uuidof(IDXGIFactory4),
                                            dxgiFactory.put_void());
                AssertIfFailed(dfhr);
            }

            winrt::com_ptr<IDXGIAdapter4> dxgiAdapter4;
            size_t maxDedicatedVideoMemory{};
            /* Get the adapter with Largest Video Memory */ {
                winrt::com_ptr<IDXGIAdapter1> dxgiAdapter1;
                for (uint32_t i = 0; 
                    dxgiFactory->EnumAdapters1(i, dxgiAdapter1.put()) not_eq DXGI_ERROR_NOT_FOUND;
                    ++i)
                {
                    DXGI_ADAPTER_DESC1 dxgiAdapterDesc1{};
                    dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

                    if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0
                        && dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory
                        && D3D12CreateDevice(dxgiAdapter1.get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr) == S_OK)
                    {
                        maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                        dxgiAdapter4 = dxgiAdapter1.as();
                    }
                }
            }

            /* Create Direct 3D 12 device */ {
                auto dhr = D3D12CreateDevice(dxgiAdapter4.get(), 
                                             D3D_FEATURE_LEVEL_11_0, 
                                             __uuidof(ID3D12Device2),
                                             m_Device.put_void());
                AssertIfFailed(dhr);

            #ifdef _DEBUG
                winrt::com_ptr<ID3D12InfoQueue> infoQueue = m_Device.as();
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,      TRUE);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,    TRUE);

                D3D12_MESSAGE_SEVERITY severities [] {
                    D3D12_MESSAGE_SEVERITY_INFO
                };

                D3D12_MESSAGE_ID denyIds[] {
                    D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                    D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                    D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                };

                D3D12_INFO_QUEUE_FILTER newFilter{};
                newFilter.DenyList.NumSeverities = std::size(severities);
                newFilter.DenyList.pSeverityList = severities;
                newFilter.DenyList.NumIDs = std::size(denyIds);
                newFilter.DenyList.pIDList = denyIds;
                auto iqhr = infoQueue->PushStorageFilter(&newFilter);
                AssertIfFailed(iqhr);
            #endif
            }

            /* Create Command Queue */ {
                D3D12_COMMAND_QUEUE_DESC desc{};
                desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
                desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
                desc.NodeMask = 0;

                auto cqhr = m_Device->CreateCommandQueue(&desc,
                                                         __uuidof(ID3D12CommandQueue),
                                                         m_CommandQueue.put_void());
                AssertIfFailed(cqhr);
            }
        }
        ~DirectX12() = default;

        HWND m_hWnd{};
        Device_t m_Device;
        CommandQueue_t m_CommandQueue;
        SwapChain_t m_SwapChain;
        std::array<Buffer_t, Num_Frames> m_BackBuffers;
        CommandList_t m_CommandList;
        std::array<CommandAllocator_t, Num_Frames> m_CommandAllocators;
        DescriptorHeap_t m_RTVDescriptorHeap;
        Fence_t m_Fence;
    };
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

    Lesson1::DirectX12 d3d(wnd.m_hWnd);

    wnd.Show();

    while(wnd.m_hWnd)
    {
        wnd.ProcessMessages();
    }

}