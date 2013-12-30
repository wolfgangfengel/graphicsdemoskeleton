////////////////////////////////////////////////////////////////////////
//
// Instructional Post-Processing Color filters with DirectCompute
//
// by Wolfgang Engel 
//
// Last time modified: 12/29/2013 
//
///////////////////////////////////////////////////////////////////////
StructuredBuffer<float4> Input : register( t0 );
RWTexture2D<float4> Result : register (u0);

#define THREADX 16
#define THREADY 16

cbuffer cbCS : register(b0)
{
	float4 WidthHeightSaturation;
	float4 ColorCorrect;
	float4 ColorAdd;
	float4 Contrast;
};

#define groupthreads THREADX * THREADY
groupshared float4 sharedMem[groupthreads];

// SV_DispatchThreadID - index of the thread within the entire dispatch in each dimension: x - 0..x - 1; y - 0..y - 1; z - 0..z - 1
// SV_GroupID - index of a thread group in the dispatch — for example, calling Dispatch(2,1,1) results in possible values of 0,0,0 and 1,0,0, varying from 0 to (numthreadsX * numthreadsY * numThreadsZ) – 1
// SV_GroupThreadID - 3D version of SV_GroupIndex - if you specified numthreads(3,2,1), possible values for the SV_GroupThreadID input value have the range of values (0–2,0–1,0)
// SV_GroupIndex - index of a thread within a thread group
[numthreads(THREADX, THREADY, 1)]
void PostFX( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex  )
{
	float2 size = float2(WidthHeightSaturation.x, WidthHeightSaturation.y);

    // copy the number of values == groupthreads into the shared memory
	uint idx = DTid.x + DTid.y * size.x;
        
    float3 color = 0;
    
    // make sure we are not running out of bounds
	if (idx < (size.x * size.y - 1))
    {
		color = Input[idx].xyz;
		
		// Contrast
		color = color - Contrast * (color - 1.0f) * color * (color - 0.5f);

		// Saturation
		float Lum = dot(color, float3(0.2126, 0.7152, 0.0722));
		color = lerp(Lum.xxx, color, WidthHeightSaturation.z);
	
		// Color Correction
		color = color *ColorCorrect * float3(2.0f, 2.0f, 2.0f) + ColorAdd;
	}

    Result[DTid.xy] = float4(color, 0.0); 
}
