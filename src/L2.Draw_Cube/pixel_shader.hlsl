struct pixel_shader_input
{
	float4 color: COLOR;
};

float4 main(pixel_shader_input in_p) : SV_TARGET
{
	return in_p.color;
}