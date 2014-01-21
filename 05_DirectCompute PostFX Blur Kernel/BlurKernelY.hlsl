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

#include "BlurKernel.inc"

[numthreads(NUM_THREADS_PER_LINE, RUN_LINES, 1)]
void CSFilterY(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	// Temp data
	int i, j;
	float4 f4LDSValue;

	// Group coords from group IDs
	int2 i2GroupCoord = int2((Gid.y * RUN_LINES), (Gid.x * RUN_SIZE) - KERNEL_RADIUS);

	{
		// Sampling and line offsets from group thread IDs
		int iPixelOffset = GTid.x;
		int iLineOffset = GTid.y;

		// Sixel coords from group IDs
		int2 i2Coord = int2(i2GroupCoord.x, i2GroupCoord.y + iPixelOffset);

			// Load PIXELS_PER_THREAD texels from input texture for RUN_LINES
			[unroll]
		for (i = 0; i < SAMPLES_PER_THREAD; ++i)
		{
			// Fetch source data
			f4LDSValue = Input[i2Coord + int2(iLineOffset, i * NUM_THREADS_PER_LINE)];

			// Write data to LDS
			WriteToLDS(f4LDSValue, iLineOffset, iPixelOffset + i * NUM_THREADS_PER_LINE);
		}

		// Optionally load some extra texels as required by the exact kernel size 
		if (GTid.x < KERNEL_DIAMETER_MINUS_ONE)
		{
			// Fetch source data
			f4LDSValue = Input[(i2GroupCoord + int2(iLineOffset, RUN_SIZE + GTid.x))];

			// Write data to LDS
			WriteToLDS(f4LDSValue, iLineOffset, RUN_SIZE + GTid.x);
		}
	}

	// Sync threads
	GroupMemoryBarrierWithGroupSync();

	{
		// Sampling and line offsets from group thread IDs
		int iPixelOffset = GTid.x * PIXELS_PER_THREAD;
		int iLineOffset = GTid.y;

		// Pixel coords from group IDs
		int2 i2Coord = int2(i2GroupCoord.x, i2GroupCoord.y + iPixelOffset);

		// Since we start with the first thread position, we need to increment the coord by KERNEL_RADIUS 
		i2Coord.y += KERNEL_RADIUS;

		// Ensure we don't compute pixels off screen
		if (i2Coord.y < c_height)
		{
			// Compute the bilateral dilate for this threads pixels
			CS_Output O = ComputeFilterKernel(iPixelOffset, iLineOffset);

			// Write the PIXELS_PER_THREAD results out
			[unroll]
			for (i = 0; i < PIXELS_PER_THREAD; ++i)
			{
				Output[i2Coord + int2(GTid.y, i)] = O.fOutput[i];
			}
		}
	}
}
