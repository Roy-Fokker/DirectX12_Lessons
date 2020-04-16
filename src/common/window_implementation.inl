#include "window.h"

#include <atlbase.h>
#include <atlwin.h>
#include <array>

using namespace learning_dx12;

struct window::window_implementation : public CWindowImpl<window::window_implementation>
{
	window_implementation() = default;

	~window_implementation()
	{
		if (m_hWnd)
		{
			DestroyWindow();
		}
	}

	BEGIN_MSG_MAP(atl_window)
		MESSAGE_HANDLER(WM_DESTROY, on_wnd_destroy)
		MESSAGE_HANDLER(WM_PAINT, on_wnd_paint)

		MESSAGE_HANDLER(WM_ACTIVATEAPP, on_wnd_activate)
		MESSAGE_HANDLER(WM_SIZE, on_wnd_resize)
		MESSAGE_HANDLER(WM_KEYUP, on_wnd_keypress)
		MESSAGE_HANDLER(WM_MOUSEMOVE, on_wnd_mousemove)
	END_MSG_MAP()

	LRESULT on_wnd_destroy(UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
	{
		PostQuitMessage(NULL);
		bHandled = TRUE;
		return 0;
	}

	LRESULT on_wnd_paint(UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
	{
		PAINTSTRUCT ps{ 0 };
		HDC hdc = BeginPaint(&ps);
		EndPaint(&ps);

		bHandled = TRUE;
		return 0;
	}

	LRESULT on_wnd_activate(UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
	{
		if (invoke_callback(message_type::activate, wParam, lParam))
		{
			bHandled = TRUE;
			return 0;
		}

		return DefWindowProc(msg, wParam, lParam);
	}

	LRESULT on_wnd_resize(UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
	{
		if (invoke_callback(message_type::resize, wParam, lParam))
		{
			bHandled = TRUE;
			return 0;
		}

		return DefWindowProc(msg, wParam, lParam);
	}

	LRESULT on_wnd_keypress(UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
	{
		if (invoke_callback(message_type::keypress, wParam, lParam))
		{
			bHandled = TRUE;
			return 0;
		}

		return DefWindowProc(msg, wParam, lParam);
	}

	LRESULT on_wnd_mousemove(UINT msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
	{
		if (invoke_callback(message_type::mousemove, wParam, lParam))
		{
			bHandled = TRUE;
			return 0;
		}

		return DefWindowProc(msg, wParam, lParam);
	}

	bool invoke_callback(message_type msg, WPARAM wParam, LPARAM lParam)
	{
		uint16_t idx = static_cast<uint16_t>(msg);
		auto call = callback_methods.at(idx);
		if (call)
		{
			return call(wParam, lParam);
		}

		return false;
	}

	std::array<callback_method, max_message_types> callback_methods{ nullptr };
};