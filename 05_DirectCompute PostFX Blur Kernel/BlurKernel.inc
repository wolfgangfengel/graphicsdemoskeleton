////////////////////////////////////////////////////////////////////////
//
// Instructional Post-Processing Parallel Reduction with DirectCompute
//
// Original Author: Jon Story
// Taken from the example
// http://developer.amd.com/tools-and-sdks/graphics-development/amd-radeon-sdk/
// 
// Copyright © AMD Corporation. All rights reserved.
//
// Changes by Igor Lobanchkikov and Wolfgang Engel 
//
// Last time modified: 01/19/2014
//
///////////////////////////////////////////////////////////////////////

#define	DOF_BLUR_KERNEL_RADIUS_MAX 16

// this needs to be the same as on the application level 
#define DOF_BLUR_KERNEL_RADIUS 4

#if	DOF_BLUR_KERNEL_RADIUS > DOF_BLUR_KERNEL_RADIUS_MAX
Error.
#endif

Texture2D Input : register(t0);
RWTexture2D<float4> Output : register(u0);


// constants that can change per frame
cbuffer cbCS : register( b0 )
{ 
	int c_height : packoffset(c0.x);
	int c_width : packoffset(c0.y);		// size view port
	float c_epsilon : packoffset(c0.z);	// julia detail  	
	int c_selfShadow : packoffset(c0.w);  // selfshadowing on or off  
	float4 c_diffuse : packoffset(c1);	// diffuse shading color
	float4 c_mu : packoffset(c2);		// julia quaternion parameter
	float4x4 rotation : packoffset(c3);
	float4 zoom : packoffset(c7); // zoom with dummy offset 
	
	// when you use
	//	float KernelWeights[DOF_BLUR_KERNEL_RADIUS_MAX + 1];
	// it indexes into the x channels of float4's
	float4 KernelWeights[DOF_BLUR_KERNEL_RADIUS_MAX + 1] : packoffset(c8);
};





#define KERNEL_RADIUS DOF_BLUR_KERNEL_RADIUS

//-----------------------------------------------------------------------------------------
// Defines that control the CS logic of the kernel 
//-----------------------------------------------------------------------------------------
#define KERNEL_DIAMETER				(2*KERNEL_RADIUS+1)
#define KERNEL_DIAMETER_MINUS_ONE	( KERNEL_DIAMETER - 1 )
#define RUN_SIZE_PLUS_KERNEL	    ( RUN_SIZE + KERNEL_DIAMETER_MINUS_ONE )

// the following two #defines need to be the same in the application code, when you invoke the kernel
#define RUN_SIZE					( 128 )	//	Pixels to process per line per kernel invocation
#define RUN_LINES					( 2 )	//	Lines to process per kernel invocation

#define PIXELS_PER_THREAD			( 4 )	//	Pixels to process per thread	
#define NUM_THREADS_PER_LINE		( RUN_SIZE / PIXELS_PER_THREAD )
#define SAMPLES_PER_THREAD          ( RUN_SIZE_PLUS_KERNEL / NUM_THREADS_PER_LINE )
#define EXTRA_SAMPLES               ( RUN_SIZE_PLUS_KERNEL - ( NUM_THREADS_PER_LINE * SAMPLES_PER_THREAD ) )

// 16 or 32. 16 is faster.
#define PRECISION					( 16 )

//-----------------------------------------------------------------------------------------
// LDS definition
//-----------------------------------------------------------------------------------------
#if ( PRECISION == 32 )
groupshared float4  g_f4LDS[RUN_LINES][RUN_SIZE_PLUS_KERNEL];
#else //( PRECISION == 16 )
groupshared uint2    g_u2LDS[RUN_LINES][RUN_SIZE_PLUS_KERNEL];
#endif

//-----------------------------------------------------------------------------------------
// Data packing
//-----------------------------------------------------------------------------------------
// Packs a float2 to a unit
uint Float2ToUint(float2 f2Value)
{
	return (f32tof16(f2Value.x) + (f32tof16(f2Value.y) << 16));
}

// Unpacks a uint to a float2
float2 UintToFloat2(uint uValue)
{
	return float2(f16tof32(uValue), f16tof32(uValue >> 16));
}

//--------------------------------------------------------------------------------------
// LDS data access
//--------------------------------------------------------------------------------------
void WriteToLDS(float4 f4Value, int iLine, int iPixel)
{
#if ( PRECISION == 32 )
	g_f4LDS[iLine][iPixel] = f4Value;
#else //( PRECISION == 16 ) 
	g_u2LDS[iLine][iPixel] = uint2(Float2ToUint(f4Value.xy), Float2ToUint(f4Value.zw));
#endif
}

float4 ReadFromLDS(int iLine, int iPixel)
{
#if ( PRECISION == 32 )
	return float4(g_f4LDS[iLine][iPixel]);
#else // ( PRECISION == 16 ) 
	return float4(UintToFloat2(g_u2LDS[iLine][iPixel].x), UintToFloat2(g_u2LDS[iLine][iPixel].y));
#endif
}

struct CS_Output
{
	float4 fOutput[PIXELS_PER_THREAD];
};

//--------------------------------------------------------------------------------------
// Defines the filter kernel logic. Computes filter kernel on PIXELS_PER_THREAD pixels per thread.
//--------------------------------------------------------------------------------------
CS_Output ComputeFilterKernel(int iPixelOffset, int iLineOffset)
{
/*
	float KernelWeights[5];
	KernelWeights[4] = 0.00854818523;
	KernelWeights[3] = 0.0232363734;
	KernelWeights[2] = 0.0631630123;
	KernelWeights[1] = 0.171694890;
	KernelWeights[0] = 0.466715068;
*/
	CS_Output O;
	int i, j;
	float fWeightSum[PIXELS_PER_THREAD];
	float4 f4Temp[PIXELS_PER_THREAD];

	// Prime the GPRs and clean accumulators
	[unroll]
	for (i = 0; i < PIXELS_PER_THREAD; ++i)
	{
		f4Temp[i] = ReadFromLDS(iLineOffset, iPixelOffset + i);
		O.fOutput[i] = float4(0.0f, 0.0f, 0.0f, 0.0f);
		fWeightSum[i] = 0.0f;
	}

	// Increment the LDS offset by PIXELS_PER_THREAD
	iPixelOffset += PIXELS_PER_THREAD;

	// Run through the kernel
	[unroll]
	// 0..8
	for (j = 0; j < KERNEL_DIAMETER; ++j)
	{
		// Perform kernel step for PIXELS_PER_THREAD pixels
		[unroll]
		for (i = 0; i < PIXELS_PER_THREAD; ++i)
		{
			// indices into constant buffer weights
			// first run abs(0 - 4) = 4
			// first run abs(1 - 4) = 3
			// first run abs(2 - 4) = 2
			// first run abs(3 - 4) = 1
			// first run abs(4 - 4) = 0
			// first run abs(5 - 4) = 1
			// first run abs(6 - 4) = 2
			// first run abs(7 - 4) = 3
			// first run abs(8 - 4) = 4
			float w = KernelWeights[abs(j - KERNEL_RADIUS)];
			O.fOutput[i] += w * f4Temp[i];
			fWeightSum[i] += w;
		}

		// Shift data in the temp registers (due to [unroll] of this and main loop
		// this is essentially register renaming).
		[unroll]
		for (i = 0; i < PIXELS_PER_THREAD - 1; ++i)
		{
			f4Temp[i] = f4Temp[i + 1];
		}

		f4Temp[i] = ReadFromLDS(iLineOffset, iPixelOffset + j);
	}

	return O;
}
