// HLSL adaptation; see LICENSE.txt and Original/.
#include "Bindings.hlsl"
#include "Common.hlsl"
// Himalayas. Created by Reinder Nijhoff 2018
// @reindernijhoff
//
// https://www.shadertoy.com/view/MdGfzh
//
// This is my first attempt to render volumetric clouds in a fragment shader.
//
// Buffer A: The main look-up texture for the cloud shapes. 
// Buffer B: A 3D (32x32x32) look-up texture with Worley Noise used to add small details 
//           to the shapes of the clouds. I have packed this 3D texture into a 2D buffer.
// 
void mainImage(out float4 fragColor,in float2 fragCoord) {

        // pack 32x32x32 3d texture in 2d texture (with padding)
        float z = floor(fragCoord.x/34.) + 8.*floor(fragCoord.y/34.);
        float2 uv = mod(fragCoord.xy, 34.) - 1.;
        float3 coord = v3(uv, z) / 32.;

        float r = tilableVoronoi( coord, 16,  3. );
        float g = tilableVoronoi( coord,  4,  8. );
        float b = tilableVoronoi( coord,  4, 16. );

        float c = max(0., 1.-(r + g * .5 + b * .25) / 1.75);

        fragColor = v4(c,c,c,c);
}

[numthreads(8,8,1)] void CS_B(uint3 id:SV_DispatchThreadID) {
    if(any(id.xy >= (uint2)iResolution.xy)) return;
    float4 color; mainImage(color,(float2)id.xy+0.5);
    outputImage[id.xy]=color;
}
