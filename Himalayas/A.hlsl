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

        float2 vUV = fragCoord / iResolution.xy;
        float3 coord = fract(v3(vUV + v2(.2,0.62), .5));
        
        float4 col = v4(1);
        
        float mfbm = 0.9;
        float mvor = 0.7;
        
        col.r = mix(1., tilableFbm( coord, 7, 4. ), mfbm) * 
            	mix(1., tilableVoronoi( coord, 8, 9. ), mvor);
        col.g = 0.625 * tilableVoronoi( coord + 0., 3, 15. ) +
        		0.250 * tilableVoronoi(  coord + 0., 3, 19. ) +
        		0.125 * tilableVoronoi( coord + 0., 3, 23. ) 
            	-1.;
        col.b = 1. - tilableVoronoi( coord + 0.5, 6, 9. );
        
	    fragColor = col;
}

[numthreads(8,8,1)] void CS_A(uint3 id:SV_DispatchThreadID) {
    if(any(id.xy >= (uint2)iResolution.xy)) return;
    float4 color; mainImage(color,(float2)id.xy+0.5);
    outputImage[id.xy]=color;
}
