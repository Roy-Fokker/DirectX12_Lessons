#ifdef _DEBUG
// CRT Memory Leak detection
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#endif

#include "window.h"
#include "clock.h"
#include "draw_cube.h"

#ifdef _DEBUG
#include <dxgi1_3.h>
#include <dxgidebug.h>
#include <winrt\base.h>
#include <cstdlib>

void report_live_dx_objects()
{
	winrt::com_ptr<IDXGIDebug1> debug{};

	DXGIGetDebugInterface1(NULL,
						   __uuidof(IDXGIDebug1),
						   debug.put_void());

	debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
}
#endif

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

#ifdef _DEBUG
	std::atexit(&report_live_dx_objects);
#endif // _DEBUG


	return 0;
}
