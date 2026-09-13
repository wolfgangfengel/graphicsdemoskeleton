struct VOut
{
	// Keep color in the same register as the pixel shader's sole input.
	float4 color : C;
	float4 position : SV_POSITION;
};

cbuffer Transforms : register(b0)
{
	row_major float4x4 worldViewProjection;
};

[RootSignature("RootConstants(num32BitConstants=16, b0, visibility=SHADER_VISIBILITY_VERTEX)")]
VOut VShader(uint vertex : SV_VertexID)
{
	VOut output;
	// Model-space vertices in the XY plane, centered at their centroid.
	float4 position = float4(vertex == 0 ? 0 : (vertex == 1 ? 0.6 : -0.6),
	                         vertex == 0 ? 2.0 / 3.0 : -1.0 / 3.0, 0, 1);
	output.color = float4(vertex == uint3(0, 1, 2), 1);
	// CPU concatenates World * View * Projection; rasterization divides by W.
	output.position = mul(position, worldViewProjection);

	return output;
}
