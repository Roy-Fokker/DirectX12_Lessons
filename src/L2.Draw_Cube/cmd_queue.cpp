#include "cmd_queue.h"

#include "d3dx12.h"

#include <cppitertools/enumerate.hpp>

using namespace learning_dx12;

namespace
{
	constexpr auto map_to_cmd_list_type(cmd_queue_type type) -> D3D12_COMMAND_LIST_TYPE
	{
		using cqt = cmd_queue_type;

		switch (type)
		{
		case cqt::direct:
			return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case cqt::bundle:
			return D3D12_COMMAND_LIST_TYPE_BUNDLE;
		case cqt::compute:
			return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case cqt::copy:
			return D3D12_COMMAND_LIST_TYPE_COPY;
		}
		assert(false);
		return {};
	}
}

cmd_queue::cmd_queue(dx_device device, cmd_queue_type type_) :
	type{type_}
{
	create_command_queue(device);
	create_command_allocators(device);
	create_command_list(device);
	create_fences(device);
	create_fence_event_handle();
}

cmd_queue::~cmd_queue()
{
	for (int i = 0; i < fences.size(); i++)
	{
		set_frame_complete_signal(i);
		wait_for_previous_frame(i);
	}

	::CloseHandle(fence_event);
}

auto cmd_queue::get_command_list(uint8_t buffer_index, CD3DX12_RESOURCE_BARRIER &barrier) -> dx_cmd_list
{
	wait_for_previous_frame(buffer_index);

	open_command_list(buffer_index);

	command_list->ResourceBarrier(1, &barrier);

	return command_list;
}

void cmd_queue::execute_command_list(uint8_t buffer_index, CD3DX12_RESOURCE_BARRIER &barrier)
{
	command_list->ResourceBarrier(1, &barrier);

	close_command_list();

	execute_command_list();

	set_frame_complete_signal(buffer_index);
}

void cmd_queue::create_command_queue(dx_device device)
{
	auto desc = D3D12_COMMAND_QUEUE_DESC{};
	desc.Type = map_to_cmd_list_type(type);
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = NULL;

	auto hr = device->CreateCommandQueue(&desc,
										 __uuidof(ID3D12CommandQueue),
										 command_queue.put_void());
	assert(SUCCEEDED(hr));
}

void cmd_queue::create_command_allocators(dx_device device)
{
	for (auto &allocator : command_allocators)
	{
		auto hr = device->CreateCommandAllocator(map_to_cmd_list_type(type),
												 __uuidof(ID3D12CommandAllocator),
												 allocator.put_void());
		assert(SUCCEEDED(hr));
	}
}

void cmd_queue::create_command_list(dx_device device)
{
	auto hr = device->CreateCommandList(NULL,
										map_to_cmd_list_type(type),
										command_allocators.front().get(),
										nullptr,
										__uuidof(ID3D12CommandList),
										command_list.put_void());
	assert(SUCCEEDED(hr));

	close_command_list();
}

void cmd_queue::create_fences(dx_device device)
{
	auto create_fence = [&](dx_fence &fence)
	{
		auto hr = device->CreateFence(0,
									  D3D12_FENCE_FLAG_NONE,
									  __uuidof(ID3D12Fence),
									  fence.put_void());
		assert(SUCCEEDED(hr));
	};

	for (auto &fence : fences)
	{
		create_fence(fence.fence);
		fence.value = {};
	}
}

void cmd_queue::create_fence_event_handle()
{
	fence_event = ::CreateEvent(NULL,
								FALSE,
								FALSE,
								NULL);
	assert(fence_event);
}

void cmd_queue::open_command_list(uint8_t buffer_index)
{
	auto allocator = command_allocators.at(buffer_index);
	allocator->Reset();
	command_list->Reset(allocator.get(), nullptr);
}

void cmd_queue::close_command_list()
{
	auto hr = command_list->Close();
	assert(SUCCEEDED(hr));
}

void cmd_queue::execute_command_list()
{
	auto cmd_lists = std::array{ (ID3D12CommandList *const)
		command_list.get()
	};

	command_queue->ExecuteCommandLists(static_cast<uint32_t>(cmd_lists.size()),
									   cmd_lists.data());
}

void cmd_queue::wait_for_previous_frame(uint8_t buffer_index)
{
	auto& [fence, signal_value] = fences.at(buffer_index);

	if (fence->GetCompletedValue() >= signal_value)
	{
		return;
	}

	auto hr = fence->SetEventOnCompletion(signal_value, fence_event);
	assert(SUCCEEDED(hr));

	::WaitForSingleObject(fence_event, INFINITE);
}

void cmd_queue::set_frame_complete_signal(uint8_t buffer_index)
{
	auto& [fence, signal_value] = fences.at(buffer_index);

	signal_value++;

	auto hr = command_queue->Signal(fence.get(),
									signal_value);
	assert(SUCCEEDED(hr));
}