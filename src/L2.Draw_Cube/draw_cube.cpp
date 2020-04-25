#include "draw_cube.h"

#include "d3dx12.h"

#include "directx12.h"
#include "cmd_queue.h"
#include "gpu_resource.h"
#include "clock.h"

#include <DirectXMath.h>
#include <array>
#include <vector>
#include <filesystem>
#include <fstream>

using namespace learning_dx12;
using namespace DirectX;

namespace {
	struct vertex_pos_color
	{
		XMFLOAT3 position;
		XMFLOAT3 color;
	};

	constexpr auto input_elements_desc = std::array{
		D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		D3D12_INPUT_ELEMENT_DESC{ "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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

	constexpr auto cube_indicies = std::array<uint32_t, 36>{
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 6, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		4, 0, 3, 4, 3, 7,
	};

	auto read_binary_file(const std::filesystem::path &file_path) -> std::vector<byte>
	{
		std::vector<byte> buffer;

		std::ifstream inFile(file_path, std::ios::in | std::ios::binary);
		inFile.unsetf(std::ios::skipws);

		assert(inFile.is_open());

		inFile.seekg(0, std::ios::end);
		buffer.reserve(inFile.tellg());
		inFile.seekg(0, std::ios::beg);

		std::copy(std::istream_iterator<byte>(inFile),
		          std::istream_iterator<byte>(),
		          std::back_inserter(buffer));

		return buffer;
	}

	auto create_buffer_and_upload(dx_device device, dx_cmd_list cmd_list,
								  size_t buffer_size, const void *buffer_data,
								  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
		-> std::pair<dx_resource, dx_resource>
	{
		auto buffer = dx_resource{};
		auto hr = device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		                                          D3D12_HEAP_FLAG_NONE,
		                                          &CD3DX12_RESOURCE_DESC::Buffer(buffer_size, flags),
		                                          D3D12_RESOURCE_STATE_COPY_DEST,
		                                          nullptr,
		                                          __uuidof(ID3D12Resource),
		                                          buffer.put_void());
		assert(SUCCEEDED(hr));
		//buffer->SetName(L"destination buffer");

		auto inter_buff = dx_resource{};
		hr = device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		                                     D3D12_HEAP_FLAG_NONE,
		                                     &CD3DX12_RESOURCE_DESC::Buffer(buffer_size),
		                                     D3D12_RESOURCE_STATE_GENERIC_READ,
		                                     nullptr,
		                                     __uuidof(ID3D12Resource),
		                                     inter_buff.put_void());
		assert(SUCCEEDED(hr));
		//inter_buff->SetName(L"intermediate buffer");

		auto sub_res_data = D3D12_SUBRESOURCE_DATA{};
		sub_res_data.pData = buffer_data;
		sub_res_data.RowPitch = buffer_size;
		sub_res_data.SlicePitch = buffer_size;

		auto result = UpdateSubresources(cmd_list.get(),
		                                 buffer.get(),
		                                 inter_buff.get(),
		                                 0, 0, 1,
		                                 &sub_res_data);
		assert(result == buffer_size);

		return
		{
			buffer,
			inter_buff
		};
	}
}

draw_cube::draw_cube(HWND hWnd)
{
	dx = std::make_unique<directx_12>(hWnd);

	auto copy_buffer_count = 1;
	copy_queue = std::make_unique<cmd_queue>(dx->get_device(), cmd_queue_type::copy, copy_buffer_count);

	auto cmd_list = copy_queue->get_command_list();

	auto copy_vertex_buffer = dx_resource{};
	create_vertex_buffer(cmd_list, copy_vertex_buffer);
	
	auto copy_index_buffer = dx_resource{};
	create_index_buffer(cmd_list, copy_index_buffer);
	
	create_root_signature();
	create_pipeline_state(cmd_list);

	copy_queue->execute_commands();
	copy_queue->wait_for_execute_finish();
}

draw_cube::~draw_cube() = default;

auto draw_cube::continue_draw() const -> bool
{
	return continue_to_draw;
}

void draw_cube::update(const game_clock &clk)
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

void draw_cube::create_vertex_buffer(dx_cmd_list cmd_list, dx_resource &copy_buffer)
{
	auto buffer_size = cube_vertices.size() * sizeof(vertex_pos_color);
	auto buffer_pair = create_buffer_and_upload(dx->get_device(),
	                                            cmd_list,
	                                            buffer_size,
	                                            static_cast<const void *>(cube_vertices.data()));
	vertex_buffer = buffer_pair.first;
	copy_buffer = buffer_pair.second;

	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.SizeInBytes = static_cast<uint32_t>(buffer_size);
	vertex_buffer_view.StrideInBytes = sizeof(vertex_pos_color);
}

void draw_cube::create_index_buffer(dx_cmd_list cmd_list, dx_resource &copy_buffer)
{
	auto buffer_size = cube_indicies.size() * sizeof(uint32_t);
	auto buffer_pair = create_buffer_and_upload(dx->get_device(),
	                                                            cmd_list,
	                                                            buffer_size,
	                                                            static_cast<const void *>(cube_indicies.data()));
	index_buffer = buffer_pair.first;
	copy_buffer = buffer_pair.second;

	index_buffer_view.BufferLocation = index_buffer->GetGPUVirtualAddress();
	index_buffer_view.Format = DXGI_FORMAT_R16_UINT;
	index_buffer_view.SizeInBytes = static_cast<uint32_t>(buffer_size);
}

void draw_cube::create_root_signature()
{
	auto parameters = std::array<D3D12_ROOT_PARAMETER1, 1>{};
	CD3DX12_ROOT_PARAMETER1::InitAsConstants(parameters[0], 
	                                         sizeof(XMMATRIX) / 4,
	                                         0, 0,
	                                         D3D12_SHADER_VISIBILITY_VERTEX);

	constexpr auto flags = D3D12_ROOT_SIGNATURE_FLAGS
	{
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
	};

	auto desc = D3D12_VERSIONED_ROOT_SIGNATURE_DESC{};
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC::Init_1_1(desc,
	                                                static_cast<uint32_t>(parameters.size()),
	                                                parameters.data(),
	                                                0,
	                                                nullptr,
	                                                flags);

	auto rs_blob = dx_blob{};
	auto err_blob = dx_blob{};
	auto hr = D3D12SerializeVersionedRootSignature(&desc,
	                                               rs_blob.put(),
	                                               err_blob.put());
	assert(SUCCEEDED(hr));

	hr = dx->get_device()->CreateRootSignature(0,
	                                           rs_blob->GetBufferPointer(),
	                                           rs_blob->GetBufferSize(),
	                                           __uuidof(ID3D12RootSignature),
	                                           root_signature.put_void());
	assert(SUCCEEDED(hr));
	root_signature->SetName(L"Root Signature");
}

void draw_cube::create_pipeline_state(dx_cmd_list cmd_list)
{
	auto vso = read_binary_file("vertex_shader.cso");
	auto vs = CD3DX12_SHADER_BYTECODE(vso.data(),
									  vso.size() * sizeof(byte));
	
	auto pso = read_binary_file("pixel_shader.cso");
	auto ps = CD3DX12_SHADER_BYTECODE(pso.data(),
									  pso.size() * sizeof(byte));

	auto il = D3D12_INPUT_LAYOUT_DESC{};
	il.NumElements = static_cast<uint32_t>(input_elements_desc.size());
	il.pInputElementDescs = input_elements_desc.data();

	auto desc = D3D12_GRAPHICS_PIPELINE_STATE_DESC{};
	desc.pRootSignature = root_signature.get();
	desc.InputLayout = il;
	desc.VS = vs;
	desc.PS = ps;
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.SampleDesc = { 1, 0 };
	desc.SampleMask = 0xffffffff;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	auto hr = dx->get_device()->CreateGraphicsPipelineState(&desc,
															__uuidof(ID3D12PipelineState),
															pipeline_state.put_void());
	assert(SUCCEEDED(hr));
}
