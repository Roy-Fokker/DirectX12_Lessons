struct projection
{
	matrix data;
};

ConstantBuffer<projection> projection_cb : register(b0);

struct vertex_pos_color
{
	float3 position: POSITION;
	float3 color: COLOR;
};

struct vertex_shader_output
{
	float4 position: SV_POSITION;
	float4 color: COLOR;
};

vertex_shader_output main(vertex_pos_color in_v)
{
	vertex_shader_output out_v;

	out_v.position = mul(projection_cb.data, float4(in_v.position, 1.0f));
	out_v.color = float4(in_v.color, 1.0f);

	return out_v;
}