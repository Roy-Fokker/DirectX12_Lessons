#include "draw_cube.h"

#include "directx12.h"

#include <DirectXMath.h>
#include <array>

using namespace learning_dx12;
using namespace DirectX;

namespace {
	struct vertex_pos_color
	{
		XMFLOAT3 position;
		XMFLOAT3 color;
	};

	constexpr auto cube_vertices = std::array{
		vertex_pos_color{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f } },
		vertex_pos_color{ { -1.0f, +1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f } },
		vertex_pos_color{ { +1.0f, +1.0f, -1.0f }, { 1.0f, 1.0f, 0.0f } },
		vertex_pos_color{ { +1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },
		vertex_pos_color{ { -1.0f, -1.0f, +1.0f }, { 0.0f, 0.0f, 1.0f } },
		vertex_pos_color{ { -1.0f, +1.0f, +1.0f }, { 0.0f, 1.0f, 1.0f } },
		vertex_pos_color{ { +1.0f, +1.0f, +1.0f }, { 1.0f, 1.0f, 1.0f } },
		vertex_pos_color{ { +1.0f, -1.0f, +1.0f }, { 1.0f, 0.0f, 1.0f } },
	}; 

	constexpr auto cube_indicies = std::array{
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 6, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		4, 0, 3, 4, 3, 7,
	};
}

draw_cube::draw_cube(HWND hWnd)
{
	dx = std::make_unique<directx_12>(hWnd);
}

draw_cube::~draw_cube() = default;

auto draw_cube::continue_draw() const -> bool
{
	return continue_to_draw;
}

void draw_cube::update(double dt)
{
	int i = 0;
}

void draw_cube::render()
{
	auto cmd_list = dx->get_cleared_cmd_list();

	dx->present();
}

auto draw_cube::on_key_press(uintptr_t wParam, uintptr_t lParam) -> bool
{
	auto &key = wParam;

	switch (key)
	{
	case VK_ESCAPE:
		continue_to_draw = false;
		break;	
	}

	return true;
}

auto draw_cube::on_mouse_move(uintptr_t wParam, uintptr_t lParam) -> bool
{
	return true;
}
