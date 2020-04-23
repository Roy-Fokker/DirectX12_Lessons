#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include "directx12.h"
#include "cmd_queue.h"
#include "gpu_resource.h"

#include "d3dx12.h"

#include <cppitertools/count.hpp>
#include <cppitertools/takewhile.hpp>
#include <cppitertools/filter.hpp>
#include <cppitertools/enumerate.hpp>
#include <numeric>
#include <tuple>
#include <chrono>

#include <cassert>
#include <cstdint>

using namespace learning_dx12;

namespace
{
	constexpr auto vsync_enabled = TRUE;
	constexpr auto clear_color = std::array{ 0.4f, 0.6f, 0.9f, 1.0f };
	constexpr auto dsv_buffer_count = 1;

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

	void debug_info_queue(dx_device &device)
	{
		winrt::com_ptr<ID3D12InfoQueue> infoQueue;
		auto hr = device->QueryInterface<ID3D12InfoQueue>(infoQueue.put());
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
	}

	auto get_dxgi_factory() -> dxgi_factory_4
	{
		auto factory = dxgi_factory_4{};
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

	auto get_dxgi_adaptor(dxgi_factory_4 factory) -> dxgi_adaptor_4
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

		auto adaptor = dxgi_adaptor_4{};
		auto hr = adaptor1->QueryInterface<IDXGIAdapter4>(adaptor.put());
		return adaptor;
	}

	auto is_tearing_allowed(dxgi_factory_4 factory) -> bool
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
	
	command_queue = std::make_unique<cmd_queue>(device, cmd_queue_type::direct);

	create_swapchain(factory);
	create_rendertarget_heap();
	create_back_buffers();

	create_depthstencil_heap();
	create_depthstencil_buffer();
}

directx_12::~directx_12() = default;

auto directx_12::get_cleared_cmd_list() -> dx_cmd_list
{
	auto cmd_list = command_queue->get_command_list(active_back_buffer_index);

	auto barrier = back_buffers.at(active_back_buffer_index)->transition_to(resource_state::render_target);
	command_queue->set_command_list_barrier(barrier);

	clear_rendertarget(cmd_list);
	clear_depthstencil(cmd_list);

	return cmd_list;
}

void directx_12::present()
{
	auto barrier = back_buffers.at(active_back_buffer_index)->transition_to(resource_state::present);
	command_queue->set_command_list_barrier(barrier);

	command_queue->execute_command_list(active_back_buffer_index);

	auto hr = swapchain->Present(vsync_enabled, present_flags);
	assert(SUCCEEDED(hr));

	active_back_buffer_index = swapchain->GetCurrentBackBufferIndex();
}

auto directx_12::get_device() const -> dx_device
{
	return device;
}

void directx_12::create_device(dxgi_adaptor_4 adaptor)
{
	auto hr = D3D12CreateDevice(adaptor.get(),
	                            D3D_FEATURE_LEVEL_11_0,
	                            __uuidof(ID3D12Device2),
	                            device.put_void());
	assert(SUCCEEDED(hr));

#ifdef _DEBUG
	debug_info_queue(device);
#endif
}

void directx_12::create_swapchain(dxgi_factory_4 factory)
{
	auto [width, height] = get_window_size(hWnd);

	auto desc = DXGI_SWAP_CHAIN_DESC1{};
	desc.Width = width;
	desc.Height = height;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Stereo = FALSE;
	desc.SampleDesc = { 1, 0 };
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = frame_buffer_count;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = present_flags;

	auto swapChain1 = winrt::com_ptr<IDXGISwapChain1>{};
	auto hr = factory->CreateSwapChainForHwnd(command_queue->command_queue.get(),
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

	active_back_buffer_index = swapchain->GetCurrentBackBufferIndex();
}

void directx_12::create_rendertarget_heap()
{
	auto desc = D3D12_DESCRIPTOR_HEAP_DESC{};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.NumDescriptors = frame_buffer_count;

	rendertarget_heap_size = device->GetDescriptorHandleIncrementSize(desc.Type);

	auto hr = device->CreateDescriptorHeap(&desc,
	                                       __uuidof(ID3D12DescriptorHeap),
	                                       rendertarget_heap.put_void());
	assert(SUCCEEDED(hr));
}

void directx_12::create_depthstencil_heap()
{
	auto desc = D3D12_DESCRIPTOR_HEAP_DESC{};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	desc.NumDescriptors = dsv_buffer_count;

	auto hr = device->CreateDescriptorHeap(&desc,
	                                       __uuidof(ID3D12DescriptorHeap),
	                                       depthstencil_heap.put_void());
	assert(SUCCEEDED(hr));
}

void directx_12::create_back_buffers()
{
	auto rendertarget_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
									rendertarget_heap->GetCPUDescriptorHandleForHeapStart());

	for (auto &&[i, back_buffer] : back_buffers | iter::enumerate)
	{
		auto buffer = dx_resource{};
		auto hr = swapchain->GetBuffer(static_cast<uint32_t>(i),
		                               __uuidof(ID3D12Resource),
		                               buffer.put_void());
		assert(SUCCEEDED(hr));

		device->CreateRenderTargetView(buffer.get(),
		                               nullptr,
		                               rendertarget_handle);

		rendertarget_handle.Offset(rendertarget_heap_size);

		back_buffer = std::make_unique<gpu_resource>(buffer, resource_state::present);
	}

}

void directx_12::create_depthstencil_buffer()
{
	auto [width, height] = get_window_size(hWnd);

	auto clear_value = D3D12_CLEAR_VALUE{};
	clear_value.Format = DXGI_FORMAT_D32_FLOAT;
	clear_value.DepthStencil = { 1.0f, 0 };

	auto buffer_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
	                                                width, height,
	                                                1, 0, 1, 0,
	                                                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	dx_resource buffer{};
	auto hr = device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
	                                          D3D12_HEAP_FLAG_NONE,
	                                          &buffer_desc,
	                                          D3D12_RESOURCE_STATE_DEPTH_WRITE,
	                                          &clear_value,
	                                          __uuidof(ID3D12Resource),
	                                          buffer.put_void());
	assert(SUCCEEDED(hr));

	auto desc = D3D12_DEPTH_STENCIL_VIEW_DESC{};
	desc.Format = DXGI_FORMAT_D32_FLOAT;
	desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;
	desc.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(buffer.get(),
	                               &desc,
	                               depthstencil_heap->GetCPUDescriptorHandleForHeapStart());

	depthstencil_buffer = std::make_unique<gpu_resource>(buffer, resource_state::present);
}

void directx_12::clear_rendertarget(dx_cmd_list cmd_list)
{
	auto rtv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rendertarget_heap->GetCPUDescriptorHandleForHeapStart(),
	                                                active_back_buffer_index,
	                                                rendertarget_heap_size);

	cmd_list->ClearRenderTargetView(rtv_handle,
	                                clear_color.data(),
	                                0,
	                                nullptr);
}

void directx_12::clear_depthstencil(dx_cmd_list cmd_list)
{
	auto dsv_handle = depthstencil_heap->GetCPUDescriptorHandleForHeapStart();

	cmd_list->ClearDepthStencilView(dsv_handle,
	                                D3D12_CLEAR_FLAG_DEPTH,
	                                1.0f,
	                                0, 0,
	                                nullptr);
}
