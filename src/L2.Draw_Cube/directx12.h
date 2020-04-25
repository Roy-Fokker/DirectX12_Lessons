#pragma once

#include "dx_wrapped_types.h"

#include <winrt/base.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include <array>
#include <memory>

#ifdef _DEBUG
#include <initguid.h>
#include <dxgidebug.h>
#endif // _DEBUG

struct CD3DX12_RESOURCE_BARRIER;

namespace learning_dx12
{
	class cmd_queue;
	class gpu_resource;

	class directx_12
	{
	public:
		directx_12() = delete;
		directx_12(HWND hWnd);
		~directx_12();

		auto get_cmd_list() -> dx_cmd_list;
		void present();

		auto get_device() const -> dx_device;
		auto get_rendertarget() const -> D3D12_CPU_DESCRIPTOR_HANDLE;
		auto get_depthstencil() const -> D3D12_CPU_DESCRIPTOR_HANDLE;

	private:
		void create_device(dxgi_adaptor_4 adaptor);
		void create_swapchain(dxgi_factory_4 factory);
		void create_rendertarget_heap();
		void create_depthstencil_heap();
		void create_back_buffers();
		void create_depthstencil_buffer();
		
	private:
		using gpu_resource_p = std::unique_ptr<gpu_resource>;
		using cmd_queue_p = std::unique_ptr<cmd_queue>;

		HWND hWnd{};
		dx_device device{};
		dx_swapchain swapchain{};
		
		dx_descriptor_heap rendertarget_heap{};
		uint32_t rendertarget_heap_size{};
		std::array<gpu_resource_p, frame_buffer_count> back_buffers{};
		uint8_t active_back_buffer_index{};

		dx_descriptor_heap depthstencil_heap{};
		gpu_resource_p depthstencil_buffer{};

		cmd_queue_p command_queue{}; // must be destroyed before all the buffers

		uint32_t present_flags = {};
	};
}