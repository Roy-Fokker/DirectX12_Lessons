#pragma once

#include "dx_wrapped_types.h"

#include <Windows.h>
#include <memory>

namespace learning_dx12
{
	class directx_12;
	class cmd_queue;
	class gpu_resource;

	class draw_cube
	{
	public:
		draw_cube(HWND hWnd);
		draw_cube() = delete;
		~draw_cube();

		auto continue_draw() const -> bool;

		void update(double delta_time);
		void render();

		auto on_key_press(uintptr_t wParam, uintptr_t lParam) -> bool;
		auto on_mouse_move(uintptr_t wParam, uintptr_t lParam) -> bool;

	private:
		void create_vertex_buffer(dx_cmd_list cmd_list, dx_resource &copy_buffer);
		void create_index_buffer(dx_cmd_list cmd_list, dx_resource &copy_buffer);

		void create_root_signature();
		void create_pipeline_state(dx_cmd_list cmd_list);

	private:
		bool continue_to_draw { true };

		std::unique_ptr<directx_12> dx{};
		std::unique_ptr<cmd_queue> copy_queue{};

		dx_resource vertex_buffer{};
		D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{};
		dx_resource index_buffer{};
		D3D12_INDEX_BUFFER_VIEW index_buffer_view{};

		dx_root_signature root_signature{};
		dx_pipeline_state pipeline_state{};
	};
}