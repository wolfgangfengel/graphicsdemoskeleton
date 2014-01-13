////////////////////////////////////////////////////////////////////////
//
// Instructional Post-Processing Color filters with DirectCompute
//
// by Wolfgang Engel 
//
// Last time modified: 12/31/2013
//
///////////////////////////////////////////////////////////////////////

#define OPTIMIZATION 5

StructuredBuffer<float4> Input : register( t0 );
RWTexture2D<float4> Result : register (u0);

#if OPTIMIZATION == 4 || 5
#define THREADX 16 / 2
#define THREADY 16 / 2
#else
#define THREADX 16
#define THREADY 16
#endif

cbuffer cbCS : register(b0)
{
	int c_height : packoffset(c0.x);
	int c_width : packoffset(c0.y);		// size view port
/*	
	This is in the constant buffer as well but not used in this shader, so I just keep it in here as a comment
	
	float c_epsilon : packoffset(c0.z);	// julia detail  	
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

//
// the following shader applies parallel reduction to an image and converts it to luminance
//
#define groupthreads THREADX * THREADY
groupshared float4 sharedMem[groupthreads];

// SV_DispatchThreadID - index of the thread within the entire dispatch in each dimension: x - 0..x - 1; y - 0..y - 1; z - 0..z - 1
// SV_GroupID - index of a thread group in the dispatch — for example, calling Dispatch(2,1,1) results in possible values of 0,0,0 and 1,0,0, varying from 0 to (numthreadsX * numthreadsY * numThreadsZ) – 1
// SV_GroupThreadID - 3D version of SV_GroupIndex - if you specified numthreads(3,2,1), possible values for the SV_GroupThreadID input value have the range of values (0–2,0–1,0)
// SV_GroupIndex - index of a thread within a thread group
[numthreads(THREADX, THREADY, 1)]
void PostFX( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex  )
{
	const float4 LumVector = float4(0.2125f, 0.7154f, 0.0721f, 0.0f);

	// read from structured buffer
	uint idx = DTid.x + DTid.y * c_width;

#if OPTIMIZATION == 4 || 5
	// store in shared memory    
	sharedMem[GI] = dot(Input[idx * 2], LumVector) + dot(Input[idx * 2 + 1], LumVector);
#else
	// store in shared memory    
	sharedMem[GI] = dot(Input[idx], LumVector);
#endif

	// Parallel reduction algorithm follows 
	GroupMemoryBarrierWithGroupSync();

	// Parallel reduction
	[unroll(groupthreads)]

#if OPTIMIZATION == 1
	// 
	for (uint s = 1; s < groupthreads; s *= 2)
	{
		if (GI % (2 * s) == 0)
			sharedMem[GI] += sharedMem[GI + s];

		GroupMemoryBarrierWithGroupSync();
	}
#elif OPTIMIZATION == 2

	for (uint s = 1; s < groupthreads; s *= 2)
	{
		int index = 2 * s * GI;

		if (index < groupthreads)
			sharedMem[index] += sharedMem[index + s];

		GroupMemoryBarrierWithGroupSync();
	}
#elif OPTIMIZATION == 3 || 4

for (uint s = groupthreads / 2; s > 0; s >>= 1)
{
	if (GI < s)
		sharedMem[GI] += sharedMem[GI + s];

	GroupMemoryBarrierWithGroupSync();
}
#elif OPTIMIZATION == 5
	
	for (uint s = groupthreads / 2; s > 32; s >>= 1)
	{
		// store in shared memory    
		sharedMem[GI] += sharedMem[GI + s];
	}

	if (GI < 32)
	{
		sharedMem[GI] += sharedMem[GI + 32];
		sharedMem[GI] += sharedMem[GI + 16];
		sharedMem[GI] += sharedMem[GI + 8];
		sharedMem[GI] += sharedMem[GI + 2];
		sharedMem[GI] += sharedMem[GI + 1];
	}

#endif
	/*
	// Parallel reduction algorithm follows
	// add up 64..127 to 0..63
	if ( GI < 64 )
	accum[GI] += accum[64+GI];
	GroupMemoryBarrierWithGroupSync();

	if ( GI < 32 )
	// add up 32..63 to 0..31
	accum[GI] += accum[32+GI];
	GroupMemoryBarrierWithGroupSync();

	if ( GI < 16 )
	// add up 16..31 to 0..15
	accum[GI] += accum[16+GI];
	GroupMemoryBarrierWithGroupSync();

	if ( GI < 8 )
	// add up 8..15 to 0..7
	accum[GI] += accum[8+GI];
	GroupMemoryBarrierWithGroupSync();

	if ( GI < 4 )
	// add up 4..7 to 0..3
	accum[GI] += accum[4+GI];
	GroupMemoryBarrierWithGroupSync();

	if ( GI < 2 )
	// add up 2..3 to 0..1
	accum[GI] += accum[2+GI];
	GroupMemoryBarrierWithGroupSync();

	if ( GI < 1 )
	// add up 1 to 0
	accum[GI] += accum[1+GI];
	GroupMemoryBarrierWithGroupSync();

	*/

	// Have the first thread write out to the output
	if (GI == 0)
	{
		// write out the result for each thread group
		Result[Gid.xy] = sharedMem[0] / (THREADX * THREADY);
	}
}
