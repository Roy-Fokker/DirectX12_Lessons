#pragma once

#include "dx_wrapped_types.h"

struct CD3DX12_RESOURCE_BARRIER;

namespace learning_dx12
{
	enum class resource_state
	{
		present,
		render_target,
		copy_dest,
	};

	class gpu_resource
	{
	public:
		gpu_resource() = delete;
		gpu_resource(dx_resource res, resource_state state);
		~gpu_resource();

		auto transition_to(resource_state state) -> CD3DX12_RESOURCE_BARRIER;

	private:
		dx_resource resource{};
		D3D12_RESOURCE_STATES current_state{};
	};


}