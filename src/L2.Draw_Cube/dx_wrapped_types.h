#pragma once

#include <winrt/base.h>
#include <d3d12.h>
#include <dxgi1_6.h>

namespace learning_dx12
{
	using dxgi_factory_4 = winrt::com_ptr<IDXGIFactory4>;
	using dxgi_factory_5 = winrt::com_ptr<IDXGIFactory5>;
	
	using dxgi_adaptor_1 = winrt::com_ptr<IDXGIAdapter1>;
	using dxgi_adaptor_4 = winrt::com_ptr<IDXGIAdapter4>;

	using dx_device = winrt::com_ptr<ID3D12Device2>;
	using dx_swapchain = winrt::com_ptr<IDXGISwapChain4>;
	
	using dx_cmd_queue = winrt::com_ptr<ID3D12CommandQueue>;
	using dx_cmd_allocator = winrt::com_ptr<ID3D12CommandAllocator>;
	
	using dx_cmd_list = winrt::com_ptr<ID3D12GraphicsCommandList>;
	using dx_cmd_list_2 = winrt::com_ptr<ID3D12GraphicsCommandList2>;

	using dx_descriptor_heap = winrt::com_ptr<ID3D12DescriptorHeap>;
	using dx_resource = winrt::com_ptr<ID3D12Resource>;
	using dx_fence = winrt::com_ptr<ID3D12Fence>;

	using dx_blob = winrt::com_ptr<ID3DBlob>;

	using dx_pipeline_state = winrt::com_ptr<ID3D12PipelineState>;
	using dx_root_signature = winrt::com_ptr<ID3D12RootSignature>;

	constexpr auto frame_buffer_count = uint8_t{2};
}