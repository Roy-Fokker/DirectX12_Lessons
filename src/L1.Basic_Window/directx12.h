#pragma once

#include <winrt/base.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include "d3dx12.h"

#include <array>

#ifdef _DEBUG
#include <initguid.h>
#include <dxgidebug.h>
#endif // _DEBUG


namespace terrain_render
{
	class directx_12
	{
	public:
		using dxgi_factory = winrt::com_ptr<IDXGIFactory4>;
		using dxgi_adaptor = winrt::com_ptr<IDXGIAdapter4>;
		using dx_device = winrt::com_ptr<ID3D12Device2>;
		using dx_swapchain = winrt::com_ptr<IDXGISwapChain4>;
		using dx_cmd_queue = winrt::com_ptr<ID3D12CommandQueue>;
		using dx_cmd_list = winrt::com_ptr<ID3D12GraphicsCommandList>;
		using dx_cmd_allocator = winrt::com_ptr<ID3D12CommandAllocator>;
		using dx_descriptor_heap = winrt::com_ptr<ID3D12DescriptorHeap>;
		using dx_resource = winrt::com_ptr<ID3D12Resource>;
		using dx_fence = winrt::com_ptr<ID3D12Fence>;

	public:
		directx_12() = delete;
		directx_12(HWND hWnd);
		~directx_12();

		void clear();
		void present();

	private:
		void create_device(dxgi_adaptor adaptor);
		void create_command_queue();
		void create_command_allocators();
		void create_command_list();
		void create_swapchain(dxgi_factory factory);
		void create_rendertarget_heap();
		void create_back_buffers();
		void create_fence();
		void create_event_handle();

		void open_command_list();
		void close_execute_command_list();
		
		void set_buffer_to_render();
		void set_buffer_to_present();
		void next_fence_and_buffer();

	private:
		static constexpr uint8_t num_frame_buffers = { 2 };

		HWND hWnd{};
		dx_device device{};

		dx_cmd_queue command_queue{};
		std::array<dx_cmd_allocator, num_frame_buffers> command_allocators{};
		dx_cmd_list command_list{};
		
		dx_swapchain swapchain{};
		dx_descriptor_heap rendertarget_heap{};
		uint32_t rendertarget_heap_size{};
		std::array<dx_resource, num_frame_buffers> back_buffers{};
		uint8_t active_back_buffer_index{};

		dx_fence fence{};
		HANDLE fence_event_handle{};
		std::array<uint64_t, num_frame_buffers> frame_fence_values{};
		uint64_t current_fence_value{};

		uint32_t present_flags = {};
	};
}
