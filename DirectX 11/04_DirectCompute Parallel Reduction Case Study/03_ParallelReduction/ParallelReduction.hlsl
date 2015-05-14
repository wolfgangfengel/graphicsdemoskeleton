////////////////////////////////////////////////////////////////////////
//
// Instructional Post-Processing Parallel Reduction with DirectCompute
//
// by Wolfgang Engel 
//
// Last time modified: 08/01/2015
//
///////////////////////////////////////////////////////////////////////


StructuredBuffer<float4> Input : register( t0 );
RWTexture2D<float4> Result : register (u0);

#define THREADX 8
#define THREADY 16

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
};

//
// the following shader applies parallel reduction to an image and converts it to luminance
//
#define groupthreads THREADX * THREADY
groupshared float sharedMem[groupthreads];

// SV_DispatchThreadID - index of the thread within the entire dispatch in each dimension: x - 0..x - 1; y - 0..y - 1; z - 0..z - 1
// SV_GroupID - index of a thread group in the dispatch — for example, calling Dispatch(2,1,1) results in possible values of 0,0,0 and 1,0,0, varying from 0 to (numthreadsX * numthreadsY * numThreadsZ) – 1
// SV_GroupThreadID - 3D version of SV_GroupIndex - if you specified numthreads(3,2,1), possible values for the SV_GroupThreadID input value have the range of values (0–2,0–1,0)
// SV_GroupIndex - index of a thread within a thread group
[numthreads(THREADX, THREADY, 1)]
void PostFX( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex  )
{
	const float4 LumVector = float4(0.2125f, 0.7154f, 0.0721f, 0.0f);

	// thread groups in x is 1920 / 16 = 120
	// thread groups in y is 1080 / 16 = 68
	// index in x (1920) goes from 0 to 119 | 120 (thread groups) * 8 (threads) = 960 indices in x
	// index in y (1080) goes from 0 to 67 | 68 (thread groups) * 16 (threads) = 1080 indices in y
	uint idx = ((DTid.x * 2) + DTid.y * c_width);

	// store in shared memory 
	float temp = (dot(Input[idx], LumVector) + dot(Input[idx + 1], LumVector));
	sharedMem[GI] = temp;
	
	// wait until everything is transfered from device memory to shared memory
	GroupMemoryBarrierWithGroupSync();

	// hard-coded for 128 threads
	if (GI < 64)
		sharedMem[GI] += sharedMem[GI + 64];
	GroupMemoryBarrierWithGroupSync();

	if (GI < 32) sharedMem[GI] += sharedMem[GI + 32];
	if (GI < 16) sharedMem[GI] += sharedMem[GI + 16];
	if (GI < 8) sharedMem[GI] += sharedMem[GI + 8];
	if (GI < 4) sharedMem[GI] += sharedMem[GI + 4];
	if (GI < 2)	sharedMem[GI] += sharedMem[GI + 2];
	if (GI < 1)	sharedMem[GI] += sharedMem[GI + 1];

	// Have the first thread write out to the output
	if (GI == 0)
	{
		// write out the result for each thread group
		Result[Gid.xy] = sharedMem[0] / (THREADX * THREADY * 2);
	}
}
