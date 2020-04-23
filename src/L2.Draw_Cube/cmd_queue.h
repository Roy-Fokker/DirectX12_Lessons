#pragma once

#include "dx_wrapped_types.h"

#include <winrt/base.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include <vector>

struct CD3DX12_RESOURCE_BARRIER;

namespace learning_dx12
{
	enum class cmd_queue_type
	{
		direct,
		bundle,
		compute,
		copy,
	};

	class cmd_queue
	{
		struct frame_fence
		{
			dx_fence fence;
			uint64_t value;
		};

	public:
		cmd_queue(dx_device device, cmd_queue_type type);
		cmd_queue(dx_device device, cmd_queue_type type, size_t buffer_count);
		cmd_queue() = delete;
		~cmd_queue();

		void set_command_list_barrier(CD3DX12_RESOURCE_BARRIER &transition_barrier);
		auto get_command_list(uint8_t buffer_index ) -> dx_cmd_list;
		void execute_command_list(uint8_t buffer_index);

		void set_name(LPCWSTR name_prefix);

	private:
		void create_command_queue(dx_device device);
		void create_command_allocators(dx_device device);
		void create_command_list(dx_device device);
		void create_fences(dx_device device);
		void create_fence_event_handle();

		void open_command_list(uint8_t buffer_index);
		void close_command_list();
		void execute_command_list();

		void wait_for_previous_frame(uint8_t buffer_index);
		void set_frame_complete_signal(uint8_t buffer_index);

	private:
		const cmd_queue_type type{};
		dx_cmd_queue command_queue{};
		
		std::vector<dx_cmd_allocator> command_allocators{};
		dx_cmd_list command_list;

		std::vector<frame_fence> fences{};
		HANDLE fence_event;

	private:
		friend class directx_12;
	};
}