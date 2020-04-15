#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include "directx12.h"

#include <cppitertools/count.hpp>
#include <cppitertools/takewhile.hpp>
#include <cppitertools/filter.hpp>
#include <cppitertools/enumerate.hpp>
#include <numeric>
#include <tuple>
#include <chrono>

#include <cassert>
#include <cstdint>

using namespace terrain_render;

namespace
{
	using dx = directx_12;
	using dxgi_factory_5 = winrt::com_ptr<IDXGIFactory5>;
	using dxgi_adaptor_1 = winrt::com_ptr<IDXGIAdapter1>;

	constexpr auto vsync_enabled = TRUE;
	constexpr auto clear_color = std::array{ 0.4f, 0.6f, 0.9f, 1.0f };

	auto get_window_size(HWND hWnd) -> std::tuple<uint32_t, uint32_t>
	{
		RECT rect{};
		::GetClientRect(hWnd, &rect);

		return {
			rect.right - rect.left,
			rect.bottom - rect.top
		};
	}

	void enable_debug_layer()
	{
		winrt::com_ptr<ID3D12Debug> debugInterface;
		auto hr = D3D12GetDebugInterface(__uuidof(ID3D12Debug), debugInterface.put_void());
		assert(SUCCEEDED(hr));

		debugInterface->EnableDebugLayer();

		winrt::com_ptr<IDXGIInfoQueue> dxgiInfoQueue;
		hr = DXGIGetDebugInterface1(0, __uuidof(IDXGIInfoQueue), dxgiInfoQueue.put_void());
		assert(SUCCEEDED(hr));

		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	}

	auto get_dxgi_factory() -> dx::dxgi_factory
	{
		auto factory = dx::dxgi_factory{};
		uint32_t flags{};
#ifdef _DEBUG
		flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG

		auto hr = CreateDXGIFactory2(flags,
									 __uuidof(IDXGIFactory4),
									 factory.put_void());
		assert(SUCCEEDED(hr));

		return factory;
	}

	auto get_dxgi_adaptor(dx::dxgi_factory factory) -> dx::dxgi_adaptor
	{
		auto adaptor1 = dxgi_adaptor_1{};

		auto device_found = [&](const uint32_t index) -> bool
		{
			adaptor1 = nullptr;
			return (factory->EnumAdapters1(index, adaptor1.put()) not_eq DXGI_ERROR_NOT_FOUND);
		};

		auto is_not_warp = [&](const uint32_t index) -> bool
		{
			auto desc = DXGI_ADAPTER_DESC1{};
			adaptor1->GetDesc1(&desc);

			return not (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE);
		};

		auto can_create_device = [&](const uint32_t index) -> bool
		{
			auto p_adaptor1 = adaptor1.get();
			auto hr = D3D12CreateDevice(p_adaptor1,
										D3D_FEATURE_LEVEL_11_0,
										__uuidof(ID3D12Device),
										nullptr);

			return SUCCEEDED(hr);
		};

		auto adaptors = iter::count()
			| iter::takewhile(device_found)
			| iter::filter(is_not_warp)
			| iter::filter(can_create_device);

		auto max_dedicated_video_memory = size_t{};
		auto index_with_most_memory = [&](uint32_t prev_index, const uint32_t index) -> uint32_t
		{
			adaptor1 = nullptr;
			factory->EnumAdapters1(index, adaptor1.put());

			auto desc = DXGI_ADAPTER_DESC1{};
			adaptor1->GetDesc1(&desc);

			if (desc.DedicatedVideoMemory > max_dedicated_video_memory)
			{
				max_dedicated_video_memory = desc.DedicatedVideoMemory;
				return index;
			}

			return prev_index;
		};

		auto adapter_index = std::accumulate(adaptors.begin(), adaptors.end(),
											 uint32_t{},
											 index_with_most_memory);

		adaptor1 = nullptr;
		factory->EnumAdapters1(adapter_index, adaptor1.put());

		auto adaptor = dx::dxgi_adaptor{};
		auto hr = adaptor1->QueryInterface<IDXGIAdapter4>(adaptor.put());
		return adaptor;
	}

	auto is_tearing_allowed(dx::dxgi_factory factory) -> bool
	{
		auto factory5 = dxgi_factory_5{};
		auto hr = factory->QueryInterface<IDXGIFactory5>(factory5.put());
		assert(SUCCEEDED(hr));

		auto allow_tearing = FALSE;
		hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
										   &allow_tearing,
										   sizeof BOOL);

		return SUCCEEDED(hr) 
		   and (allow_tearing == TRUE);
	}

	auto signal_fence(dx::dx_cmd_queue queue, dx::dx_fence fence, uint64_t &fence_value) -> uint64_t
	{
		auto signal_fence_value = ++fence_value;
		auto hr = queue->Signal(fence.get(), signal_fence_value);
		assert(SUCCEEDED(hr));

		return signal_fence_value;
	}

	auto wait_for_fence_value(dx::dx_fence fence, uint64_t fence_value, HANDLE event_handle, std::chrono::milliseconds duration)
	{
		if (fence->GetCompletedValue() >= fence_value)
		{
			return;
		}

		auto hr = fence->SetEventOnCompletion(fence_value, event_handle);
		assert(SUCCEEDED(hr));

		::WaitForSingleObject(event_handle, static_cast<DWORD>(duration.count()));
	}
}

directx_12::directx_12(HWND hWnd) :
	hWnd(hWnd)
{
#ifdef _DEBUG
	enable_debug_layer();
#endif // _DEBUG

	auto factory = get_dxgi_factory();
	auto adaptor = get_dxgi_adaptor(factory);

	auto allow_tearing = is_tearing_allowed(factory);
	
	present_flags = (allow_tearing and not vsync_enabled) ? DXGI_PRESENT_ALLOW_TEARING : NULL;

	create_device(adaptor);
	
	create_command_queue();
	create_command_allocators();
	create_command_list();

	create_swapchain(factory);
	create_rendertarget_heap();
	create_back_buffers();

	create_fence();
	create_event_handle();
}

directx_12::~directx_12()
{
	auto fence_value = signal_fence(command_queue,
									fence,
									current_fence_value);
	wait_for_fence_value(fence,
						 fence_value,
						 fence_event_handle,
						 std::chrono::milliseconds::max());

	::CloseHandle(fence_event_handle);
}

void directx_12::clear()
{
	open_command_list();

	set_buffer_to_render();

	auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(rendertarget_heap->GetCPUDescriptorHandleForHeapStart(),
											 active_back_buffer_index,
											 rendertarget_heap_size);

	command_list->ClearRenderTargetView(rtv,
										clear_color.data(),
										0,
										nullptr);
}

void directx_12::present()
{
	set_buffer_to_present();

	close_execute_command_list();

	auto hr = swapchain->Present(vsync_enabled, present_flags);
	assert(SUCCEEDED(hr));

	next_fence_and_buffer();
}

void directx_12::create_device(dxgi_adaptor adaptor)
{
	auto hr = D3D12CreateDevice(adaptor.get(),
								D3D_FEATURE_LEVEL_11_0,
								__uuidof(ID3D12Device2),
								device.put_void());
	assert(SUCCEEDED(hr));


#ifdef _DEBUG
	winrt::com_ptr<ID3D12InfoQueue> infoQueue;
	hr = device->QueryInterface<ID3D12InfoQueue>(infoQueue.put());
	assert(SUCCEEDED(hr));

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
	hr = infoQueue->PushStorageFilter(&newFilter);
	assert(SUCCEEDED(hr));
#endif
}

void directx_12::create_command_queue()
{
	auto desc = D3D12_COMMAND_QUEUE_DESC{};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	auto hr = device->CreateCommandQueue(&desc,
										 __uuidof(ID3D12CommandQueue),
										 command_queue.put_void());
	assert(SUCCEEDED(hr));
}

void directx_12::create_command_allocators()
{
	for (auto &cmd_alloc : command_allocators)
	{
		auto hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
												 __uuidof(ID3D12CommandAllocator),
												 cmd_alloc.put_void());
		assert(SUCCEEDED(hr));
	}
}

void directx_12::create_command_list()
{
	auto hr = device->CreateCommandList(NULL,
										D3D12_COMMAND_LIST_TYPE_DIRECT,
										command_allocators.front().get(),
										nullptr,
										__uuidof(ID3D12GraphicsCommandList),
										command_list.put_void());
	assert(SUCCEEDED(hr));

	hr = command_list->Close();
	assert(SUCCEEDED(hr));
}

void directx_12::create_swapchain(dxgi_factory factory)
{
	auto [width, height] = get_window_size(hWnd);

	auto desc = DXGI_SWAP_CHAIN_DESC1{};
	desc.Width = width;
	desc.Height = height;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Stereo = FALSE;
	desc.SampleDesc = { 1, 0 };
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = num_frame_buffers;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = present_flags;

	auto swapChain1 = winrt::com_ptr<IDXGISwapChain1>{};
	auto hr = factory->CreateSwapChainForHwnd(command_queue.get(),
											  hWnd,
											  &desc,
											  nullptr,
											  nullptr,
											  swapChain1.put());
	assert(SUCCEEDED(hr));

	hr = factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	assert(SUCCEEDED(hr));

	hr = swapChain1->QueryInterface<IDXGISwapChain4>(swapchain.put());
	assert(SUCCEEDED(hr));

}

void directx_12::create_rendertarget_heap()
{
	auto desc = D3D12_DESCRIPTOR_HEAP_DESC{};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.NumDescriptors = num_frame_buffers;

	rendertarget_heap_size = device->GetDescriptorHandleIncrementSize(desc.Type);

	auto hr = device->CreateDescriptorHeap(&desc,
										   __uuidof(ID3D12DescriptorHeap),
										   rendertarget_heap.put_void());
	assert(SUCCEEDED(hr));
}

void directx_12::create_back_buffers()
{
	auto rendertarget_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
									rendertarget_heap->GetCPUDescriptorHandleForHeapStart());

	for (auto &&[i, back_buffer] : back_buffers | iter::enumerate)
	{
		auto hr = swapchain->GetBuffer(static_cast<uint32_t>(i),
									   __uuidof(ID3D12Resource),
									   back_buffer.put_void());
		assert(SUCCEEDED(hr));

		device->CreateRenderTargetView(back_buffer.get(),
									   nullptr,
									   rendertarget_handle);

		rendertarget_handle.Offset(rendertarget_heap_size);
	}
}

void directx_12::create_fence()
{
	auto hr = device->CreateFence(0,
								  D3D12_FENCE_FLAG_NONE,
								  __uuidof(ID3D12Fence),
								  fence.put_void());
	assert(SUCCEEDED(hr));
}

void directx_12::create_event_handle()
{
	fence_event_handle = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fence_event_handle);
}

void directx_12::open_command_list()
{
	auto command_allocator = command_allocators.at(active_back_buffer_index);
	command_allocator->Reset();
	command_list->Reset(command_allocator.get(), nullptr);
}

void directx_12::close_execute_command_list()
{
	auto hr = command_list->Close();
	assert(SUCCEEDED(hr));

	auto command_lists = std::array { (ID3D12CommandList *const)
		command_list.get()
	};

	command_queue->ExecuteCommandLists(static_cast<uint32_t>(command_lists.size()),
									   command_lists.data());
}

void directx_12::set_buffer_to_render()
{
	auto back_buffer = back_buffers.at(active_back_buffer_index);

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(back_buffer.get(),
														D3D12_RESOURCE_STATE_PRESENT,
														D3D12_RESOURCE_STATE_RENDER_TARGET);

	command_list->ResourceBarrier(1, &barrier);
}

void directx_12::set_buffer_to_present()
{
	auto back_buffer = back_buffers.at(active_back_buffer_index);

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(back_buffer.get(),
														D3D12_RESOURCE_STATE_RENDER_TARGET,
														D3D12_RESOURCE_STATE_PRESENT);

	command_list->ResourceBarrier(1, &barrier);
}

void directx_12::next_fence_and_buffer()
{
	frame_fence_values[active_back_buffer_index] = signal_fence(command_queue,
																fence,
																current_fence_value);

	active_back_buffer_index = swapchain->GetCurrentBackBufferIndex();

	wait_for_fence_value(fence, 
						 frame_fence_values[active_back_buffer_index],
						 fence_event_handle,
						 std::chrono::milliseconds::max());
}

