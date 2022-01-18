
Texture2D RenderInput;
SamplerState sRenderInput;
Texture2D DepthInput;
SamplerState sDepthInput;

// Vertex Shader
void VS_MAIN(in uint id : SV_VertexID, out float4 position : SV_Position, out float2 texcoord : TEXCOORD, in float3 icol : DIFFUSE, out float3 ocol : DIFFUSE)
{
	texcoord.x = (id == 2) ? 2.0 : 0.0;
	texcoord.y = (id == 1) ? 2.0 : 0.0;
	position = float4(texcoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
	ocol = icol;
}

// Pixel Shader
void PS_MAIN(in float4 position : SV_Position, in float2 texcoord : TEXCOORD, out float3 color : SV_Target, in float3 icol : DIFFUSE)
{
	if (texcoord.x + texcoord.y > 1.0)
		color = RenderInput.Sample(sRenderInput, texcoord) * (icol.rgb * 3);
	else
		color = DepthInput.Sample(sDepthInput, texcoord);	
	
	if (texcoord.x + texcoord.y > 1.0) color =  icol.rgb * 3;
	else color =  1.0f - icol.rgb;
}
