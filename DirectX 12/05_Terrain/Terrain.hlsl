// Mountains by David Hoskins, 2013. See LICENSE-Mountains.txt.
// Add DX12 resource bindings to the unchanged DX11 entry point.
[RootSignature("RootConstants(num32BitConstants=4, b0), DescriptorTable(UAV(u0))")]
[numthreads(8,8,1)] void CS_Terrain(uint3 id:SV_DispatchThreadID,uint groupIndex:SV_GroupIndex);
#include "../../DirectX 11/05_Terrain/Terrain.hlsl"

