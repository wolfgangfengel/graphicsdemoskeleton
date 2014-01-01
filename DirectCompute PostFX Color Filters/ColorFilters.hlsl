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
	int c_height : packoffset(c0.x);
	int c_width : packoffset(c0.y);		// size view port
/*	float c_epsilon : packoffset(c0.z);	// julia detail  	
	int c_selfShadow : packoffset(c0.w);  // selfshadowing on or off  
	float4 c_diffuse : packoffset(c1);	// diffuse shading color
	float4 c_mu : packoffset(c2);		// julia quaternion parameter
	float4x4 rotation : packoffset(c3);
	float zoom : packoffset(c7.x);
*/
	float Saturation : packoffset(c7.y);
	float ColorCorrectRed : packoffset(c7.z);
	float ColorCorrectGreen : packoffset(c7.w);
	float ColorCorrectBlue : packoffset(c8.x);
	float ColorAddRed : packoffset(c8.y);
	float ColorAddGreen : packoffset(c8.z);
	float ColorAddBlue : packoffset(c8.w);
	float3 Contrast : packoffset(c9);
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
	float2 size = float2(c_width, c_height);

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
		color = lerp(Lum.xxx, color, Saturation);
	
		// Color Correction
		color = color * float3(ColorCorrectRed, ColorCorrectGreen, ColorCorrectBlue)  * float3(2.0f, 2.0f, 2.0f) + float3(ColorAddRed, ColorAddGreen, ColorAddBlue);
		
	}

    Result[DTid.xy] = float4(color, 0.0); 
}
