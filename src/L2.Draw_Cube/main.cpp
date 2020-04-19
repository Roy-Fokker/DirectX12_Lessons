#ifdef _DEBUG
// CRT Memory Leak detection
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#endif

#include "window.h"
#include "clock.h"
#include "draw_cube.h"

auto main() -> int
{
#ifdef _DEBUG
	// Detects memory leaks upon program exit
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	using namespace learning_dx12;

	constexpr int wnd_width{ 1280 },
	              wnd_height{ wnd_width * 10 / 16 };

	auto wnd = window(L"Learning DirectX 12: Draw Cube",
					  { wnd_width, wnd_height });

	auto cube = draw_cube(wnd.handle());

	using msg = window::message_type;
	wnd.set_message_callback(msg::keypress, [&](uintptr_t wParam, uintptr_t lParam) -> bool
	{
		return cube.on_key_press(wParam, lParam);
	});

	wnd.set_message_callback(msg::mousemove, [&](uintptr_t wParam, uintptr_t lParam) -> bool
	{
		return cube.on_mouse_move(wParam, lParam);
	});

	wnd.show();

	auto clk = game_clock();
	while (wnd.handle() and cube.continue_draw())
	{
		wnd.process_messages();

		clk.tick();
		cube.update(clk.get_delta_s());

		cube.render();
	}

	return 0;
}
