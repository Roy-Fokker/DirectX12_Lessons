#ifdef _DEBUG
// CRT Memory Leak detection
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#endif

#include "window.h"
#include "directx12.h"

auto main() -> int
{
#ifdef _DEBUG
	// Detects memory leaks upon program exit
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	using namespace learning_dx12;

	constexpr int wnd_width{ 1280 },
	              wnd_height{ wnd_width * 10 / 16 };

	auto wnd = window(L"Learning DirectX 12: Basic Window",
					  { wnd_width, wnd_height });

	auto dx = directx_12(wnd.handle());

	wnd.show();

	while (wnd.handle())
	{
		wnd.process_messages();

		dx.clear();

		dx.present();
	}

	return 0;
}
