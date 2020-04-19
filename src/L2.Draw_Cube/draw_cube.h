#pragma once

#include <Windows.h>
#include <memory>

namespace learning_dx12
{
	class directx_12;
	class cmd_queue;

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
		bool continue_to_draw { true };

		std::unique_ptr<directx_12> dx;
	};
}