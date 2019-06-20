#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <fmt/format.h>
#include <string>
#include <io.h>
#include <fcntl.h>
#include <Windows.h>

#include <cstdint>
#include <cassert>
#include <algorithm>
#include <array>
#include <chrono>

#include <winrt/base.h>

#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <Initguid.h>
#include <dxgidebug.h>

static constexpr uint8_t Lesson_Num{ 1 };

#define AssertIfFailed(hr) assert(SUCCEEDED(hr))

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
			::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

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
		
		void Create(RECT windowRect, const std::wstring_view &title)
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
	static constexpr uint8_t Num_Frames = 3;
	static constexpr bool Enable_VSync = true;

	struct DirectX12
	{
		using DxgiFactory4 = winrt::com_ptr<IDXGIFactory4>;
		using Device_t = winrt::com_ptr<ID3D12Device2>;
		using CommandQueue_t = winrt::com_ptr<ID3D12CommandQueue>;
		using SwapChain_t = winrt::com_ptr<IDXGISwapChain4>;
		using Buffer_t = winrt::com_ptr<ID3D12Resource>;
		using CommandList_t = winrt::com_ptr<ID3D12GraphicsCommandList>;
		using CommandAllocator_t = winrt::com_ptr<ID3D12CommandAllocator>;
		using DescriptorHeap_t = winrt::com_ptr<ID3D12DescriptorHeap>;
		using Fence_t = winrt::com_ptr<ID3D12Fence>;

		DirectX12() = delete;
		DirectX12(HWND hWnd) :
			m_hWnd(hWnd)
		{
#ifdef _DEBUG
			EnableDebugLayer();
#endif
			auto dxgiFactory = GetDxgiFactory();

			auto dxgiAdapter = GetDxgiAdaptor(dxgiFactory);

			m_IsAllowTearingSupported = CheckTearingSupport(dxgiFactory);

			CreateD3DDevice(dxgiAdapter);

			CreateCommandQueue();


			RECT wndRect{};
			::GetClientRect(m_hWnd, &wndRect);
			uint32_t width = wndRect.right - wndRect.left;
			uint32_t height = wndRect.bottom - wndRect.top;

			CreateSwapChain(dxgiFactory, width, height, Num_Frames);

			CreateRTVDescriptorHeap();

			CreateBackBuffers();

			CreateCommandAllocators();

			CreateCommandList();

			CreateFence();

			CreateEventHandle();
		}

		~DirectX12()
		{
			Flush();
			::CloseHandle(m_FenceEvent);
		}
		
		void Render()
		{
			auto commandAllocator = m_CommandAllocators.at(m_CurrentBackBufferIndex);
			auto backBuffer = m_BackBuffers.at(m_CurrentBackBufferIndex);

			commandAllocator->Reset();
			m_CommandList->Reset(commandAllocator.get(), nullptr);

			/* Clear */
			{
				auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.get(), 
				                                                    D3D12_RESOURCE_STATE_PRESENT,
				                                                    D3D12_RESOURCE_STATE_RENDER_TARGET);

				m_CommandList->ResourceBarrier(1, &barrier);

				static float clearColor[]{ 0.4f, 0.6f, 0.9f, 1.0f };
				CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
				                                  m_CurrentBackBufferIndex,
				                                  m_RTVDescriptorSize);

				m_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
			}

			/* Present */
			{
				auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.get(),
				                                                    D3D12_RESOURCE_STATE_RENDER_TARGET,
				                                                    D3D12_RESOURCE_STATE_PRESENT);

				m_CommandList->ResourceBarrier(1, &barrier);

				auto clhr = m_CommandList->Close();
				AssertIfFailed(clhr);

				ID3D12CommandList *const commandLists[]{
					m_CommandList.get()
				};

				m_CommandQueue->ExecuteCommandLists(static_cast<UINT>(std::size(commandLists)),
				                                    commandLists);

				uint32_t syncInterval = Enable_VSync ? 1 : 0;
				uint32_t presentFlags = (m_IsAllowTearingSupported and not Enable_VSync) ? DXGI_PRESENT_ALLOW_TEARING : NULL;
				auto schr = m_SwapChain->Present(syncInterval, presentFlags);
				AssertIfFailed(schr);

				m_FrameFenceValues[m_CurrentBackBufferIndex] = Signal(m_CommandQueue, m_Fence, m_CurrentFenceValue);

				m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
				WaitForFenceValue(m_Fence,
				                  m_FrameFenceValues[m_CurrentBackBufferIndex],
				                  m_FenceEvent,
				                  std::chrono::milliseconds::max());
			}
		}

		HWND m_hWnd{};
		Device_t m_Device{};
		CommandQueue_t m_CommandQueue{};
		SwapChain_t m_SwapChain{};
		std::array<Buffer_t, Num_Frames> m_BackBuffers{};
		uint8_t m_CurrentBackBufferIndex = 0;
		CommandList_t m_CommandList{};
		std::array<CommandAllocator_t, Num_Frames> m_CommandAllocators{};
		DescriptorHeap_t m_RTVDescriptorHeap{};
		uint32_t m_RTVDescriptorSize = 0;
		Fence_t m_Fence{};
		HANDLE m_FenceEvent{};
		std::array<uint64_t, Num_Frames> m_FrameFenceValues{};
		uint64_t m_CurrentFenceValue = 0;
		bool m_IsAllowTearingSupported = false;

	private:
		void EnableDebugLayer()
		{
			winrt::com_ptr<ID3D12Debug> debugInterface;
			auto dihr = D3D12GetDebugInterface(__uuidof(ID3D12Debug), debugInterface.put_void());
			AssertIfFailed(dihr);

			debugInterface->EnableDebugLayer();

			winrt::com_ptr<IDXGIInfoQueue> dxgiInfoQueue;
			dihr = DXGIGetDebugInterface1(0, __uuidof(IDXGIInfoQueue), dxgiInfoQueue.put_void());
			AssertIfFailed(dihr);

			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		}

		DxgiFactory4 GetDxgiFactory()
		{
			DxgiFactory4 dxgiFactory{};
			
			uint32_t factoryFlags{};
#ifdef _DEBUG
			factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
			auto dfhr = CreateDXGIFactory2(factoryFlags,
			                               __uuidof(IDXGIFactory4),
			                               dxgiFactory.put_void());
			AssertIfFailed(dfhr);

			return dxgiFactory;
		}

		winrt::com_ptr<IDXGIAdapter4> GetDxgiAdaptor(DxgiFactory4 dxgiFactory)
		{
			winrt::com_ptr<IDXGIAdapter4> dxgiAdapter4{};
			
			size_t maxDedicatedVideoMemory{};
			winrt::com_ptr<IDXGIAdapter1> dxgiAdapter1{};
			for (uint32_t i = 0;
			     dxgiFactory->EnumAdapters1(i, dxgiAdapter1.put()) not_eq DXGI_ERROR_NOT_FOUND;
			     ++i)
			{
				DXGI_ADAPTER_DESC1 dxgiAdapterDesc1{};
				dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);
				
				IDXGIAdapter1 *da1 = dxgiAdapter1.get();
				auto dchr = D3D12CreateDevice(da1, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr);
				if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0
				    && dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory
				    && SUCCEEDED(dchr))
				{
					maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
					auto dahr = dxgiAdapter1->QueryInterface<IDXGIAdapter4>(dxgiAdapter4.put());
					AssertIfFailed(dahr);
					//dxgiAdapter4 = dxgiAdapter1.as<IDXGIAdapter4>();
				}
				
				dxgiAdapter1 = nullptr;
			}
			return dxgiAdapter4;
		}

		bool CheckTearingSupport(DxgiFactory4 dxgiFactory)
		{
			winrt::com_ptr<IDXGIFactory5> dxgiFactory5{};
			auto dfhr = dxgiFactory->QueryInterface<IDXGIFactory5>(dxgiFactory5.put());
			if (FAILED(dfhr))
			{
				return false;
			}

			BOOL allowTearing = FALSE;
			dfhr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(BOOL));
			if (FAILED(dfhr))
			{
				return false;
			}

			return (allowTearing == TRUE);
		}

		void CreateD3DDevice(winrt::com_ptr<IDXGIAdapter4> &dxgiAdaptor)
		{
			auto dhr = D3D12CreateDevice(dxgiAdaptor.get(),
			                             D3D_FEATURE_LEVEL_11_0,
			                             __uuidof(ID3D12Device2),
			                             m_Device.put_void());
			AssertIfFailed(dhr);

#ifdef _DEBUG
			winrt::com_ptr<ID3D12InfoQueue> infoQueue;
			auto iqhr = m_Device->QueryInterface<ID3D12InfoQueue>(infoQueue.put());
			AssertIfFailed(iqhr);
			//winrt::com_ptr<ID3D12InfoQueue> infoQueue = m_Device.as<ID3D12InfoQueue>();
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

			D3D12_MESSAGE_SEVERITY severities[]{
				D3D12_MESSAGE_SEVERITY_INFO
			};

			D3D12_MESSAGE_ID denyIds[]{
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
				D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
				D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
			};

			D3D12_INFO_QUEUE_FILTER newFilter{};
			newFilter.DenyList.NumSeverities = static_cast<UINT>(std::size(severities));
			newFilter.DenyList.pSeverityList = severities;
			newFilter.DenyList.NumIDs = static_cast<UINT>(std::size(denyIds));
			newFilter.DenyList.pIDList = denyIds;
			iqhr = infoQueue->PushStorageFilter(&newFilter);
			AssertIfFailed(iqhr);
#endif
		}

		void CreateCommandQueue()
		{
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
		
		void CreateSwapChain(DxgiFactory4 dxgiFactory, uint32_t width, uint32_t height, uint32_t bufferCount)
		{
			DXGI_SWAP_CHAIN_DESC1 desc{};
			desc.Width = width;
			desc.Height = height;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.Stereo = FALSE;
			desc.SampleDesc = { 1, 0 };
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.BufferCount = bufferCount;
			desc.Scaling = DXGI_SCALING_STRETCH;
			desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
			desc.Flags = m_IsAllowTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : NULL;

			winrt::com_ptr<IDXGISwapChain1> swapChain1{};
			auto schr = dxgiFactory->CreateSwapChainForHwnd(m_CommandQueue.get(),
			                                                m_hWnd,
			                                                &desc,
			                                                nullptr,
			                                                nullptr,
			                                                swapChain1.put());
			AssertIfFailed(schr);

			schr = dxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
			AssertIfFailed(schr);

			schr = swapChain1->QueryInterface<IDXGISwapChain4>(m_SwapChain.put());
			AssertIfFailed(schr);
		}

		void CreateRTVDescriptorHeap()
		{
			D3D12_DESCRIPTOR_HEAP_DESC desc{};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			desc.NumDescriptors = Num_Frames;

			auto dhhr = m_Device->CreateDescriptorHeap(&desc,
			                                           __uuidof(ID3D12DescriptorHeap),
			                                           m_RTVDescriptorHeap.put_void());
			AssertIfFailed(dhhr);
		}

		void CreateBackBuffers()
		{
			m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart() );
			
			int i = 0;
			for (auto &backBuffer : m_BackBuffers)
			{
				auto bbhr = m_SwapChain->GetBuffer(i,
				                                   __uuidof(ID3D12Resource),
				                                   backBuffer.put_void());
				AssertIfFailed(bbhr);

				m_Device->CreateRenderTargetView(backBuffer.get(),
				                                 nullptr,
				                                 rtvHandle);

				rtvHandle.Offset(m_RTVDescriptorSize);
				i++;
			}
		}

		void CreateCommandAllocators()
		{
			for (auto &commandAllocator : m_CommandAllocators)
			{
				m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
				                                 __uuidof(ID3D12CommandAllocator),
				                                 commandAllocator.put_void());
			}
		}

		void CreateCommandList()
		{
			auto clhr = m_Device->CreateCommandList(NULL,
			                                        D3D12_COMMAND_LIST_TYPE_DIRECT,
			                                        m_CommandAllocators.back().get(),
			                                        nullptr,
			                                        __uuidof(ID3D12GraphicsCommandList),
			                                        m_CommandList.put_void());
			AssertIfFailed(clhr);

			clhr = m_CommandList->Close();
			AssertIfFailed(clhr);
		}

		void CreateFence()
		{
			auto fhr = m_Device->CreateFence(0,
			                                 D3D12_FENCE_FLAG_NONE,
			                                 __uuidof(ID3D12Fence),
			                                 m_Fence.put_void());
			AssertIfFailed(fhr);
		}

		void CreateEventHandle()
		{
			m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
			assert(m_FenceEvent);
		}

		uint64_t Signal(CommandQueue_t commandQueue, Fence_t fence, uint64_t &fenceValue)
		{
			auto signalFenceValue = ++fenceValue;
			auto shr = commandQueue->Signal(fence.get(), signalFenceValue);
			AssertIfFailed(shr);

			return signalFenceValue;
		}

		void WaitForFenceValue(Fence_t fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration)
		{
			if (fence->GetCompletedValue() < fenceValue)
			{
				auto fhr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
				AssertIfFailed(fhr);

				::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
			}
		}

		void Flush()
		{
			auto signalFenceValue = Signal(m_CommandQueue, m_Fence, m_CurrentFenceValue);
			WaitForFenceValue(m_Fence, signalFenceValue, m_FenceEvent, std::chrono::milliseconds::max());
		}
	};

	void Update()
	{
		static uint64_t frameCounter{ 0 };
		static double elapsedSeconds{ 0.0 };
		static std::chrono::high_resolution_clock clock;
		static auto t_old = clock.now();

		frameCounter++;
		auto t_now = clock.now();
		auto delta_t = t_now - t_old;
		t_old = t_now;

		elapsedSeconds += std::chrono::duration<double>(delta_t).count();
		if (elapsedSeconds > 1.0)
		{
			auto fps = frameCounter / elapsedSeconds;
			fmt::print(L"\nFPS: {}", fps);

			frameCounter = 0;
			elapsedSeconds = 0.0;
		}
	}
}

int main()
{
	auto r = _setmode(_fileno(stdout), _O_U8TEXT);

	fmt::print(L"Direct X 12 - Lesson {}\n", Lesson_Num);

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
		Lesson1::Update();

		d3d.Render();

		wnd.ProcessMessages();
	}

	fmt::print(L"\nClosing Window\n");
}