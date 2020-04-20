#include "gpu_resource.h"

#include "d3dx12.h"

#include <d3dx12.h>

#include <cassert>

using namespace learning_dx12;

namespace
{
	constexpr auto map_to_resource_state(resource_state state) -> D3D12_RESOURCE_STATES
	{
		switch (state)
		{
			case resource_state::present:
				return D3D12_RESOURCE_STATE_PRESENT;
			case resource_state::render_target:
				return D3D12_RESOURCE_STATE_RENDER_TARGET;
			case resource_state::copy_dest:
				return D3D12_RESOURCE_STATE_COPY_DEST;
		}
	}
}

gpu_resource::gpu_resource(dx_resource res, resource_state state) :
	resource{ res }
{
	current_state = map_to_resource_state(state);
}

gpu_resource::~gpu_resource() = default;

auto gpu_resource::transition_to(resource_state state) -> CD3DX12_RESOURCE_BARRIER
{
	auto prev_state = current_state;
	current_state = map_to_resource_state(state);

	assert(prev_state != current_state);
	
	return CD3DX12_RESOURCE_BARRIER::Transition(resource.get(),
												prev_state,
												current_state);
}
