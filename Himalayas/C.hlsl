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
//
// To create an interesting scene and to add some scale to the clouds, I render a 
// terrain using a simple heightmap, based on the work by Íñigo Quílez on value noise and its 
// analytical derivatives.[3]
//
// In fact, the heightmap of this shader is almost exactly the same as the heightmap that 
// is used in Íñigo Quílez' shader Elevated:
//
// https://www.shadertoy.com/view/MdX3Rr
//
// To reduce noise I use temporal reprojection (both for clouds (Buffer D) and the terrain 
// (Buffer C)) separatly. The temporal reprojection code is based on code from the shader
// "Rain Forest" (again by Íñigo Quílez):
//
// https://www.shadertoy.com/view/4ttSWf
// 
float3 noised( in float2 x ) {
    float2 f = fract(x);
    float2 u = f*f*(3.0-2.0*f);
    
    float2 p = v2(floor(x));
    float a = hash12( (p+v2(0,0)) );
	float b = hash12( (p+v2(1,0)) );
	float c = hash12( (p+v2(0,1)) );
	float d = hash12( (p+v2(1,1)) );
    
	return v3(a+(b-a)*u.x+(c-a)*u.y+(a-b-c+d)*u.x*u.y,
				6.0*f*(1.0-f)*(v2(b-a,c-a)+(a-b-c+d)*u.yx));
}

static const float2x2 m2 = float2x2(1.6,-1.2,1.2,1.6);

float terrainMap( in float2 x, const int OCTAVES ) {
	float2 p = x*(MOUNTAIN_HW_RATIO*SCENE_SCALE);
    float s = mix(1., smoothstep(.0,.4, abs(p.y)), .75);
    
    float a = 0.;
    float b = 1.;
	float2  d = v2(0.0);
    for( int i=0; i<OCTAVES; i++ ) {
        float3 n = noised(p);
        d += n.yz;
        a += b*n.x/(1.0+dot(d,d));
		b *= 0.5;
        p = mul(p,m2);
    }
	return s*a*(MOUNTAIN_HEIGHT*INV_SCENE_SCALE*.5);
}

float terrainMapB( in float2 x, const int OCTAVES ) {
	float2 p = x*(MOUNTAIN_HW_RATIO*SCENE_SCALE);
    float s = mix(1., smoothstep(.0,.4, abs(p.y)), .75);
    
    float a = 0.;
    float b = 1.;
	float2  d = v2(0.0);
    for( int i=0; i<OCTAVES; i++ ) {
        float3 n = noised(p);
        d += n.yz;
        a += b*n.x/(1.0+dot(d,d));
		b *= 0.5;
        p = mul(p,m2);
    }
	return s*a*(MOUNTAIN_HEIGHT*INV_SCENE_SCALE*.5);
}
float3 calcNormal(in float3 pos, float t, const int OCTAVES) {
    float2  eps = v2( (0.0015)*t, 0.0 );
    return normalize( v3( terrainMap(pos.xz-eps.xy, OCTAVES) - terrainMap(pos.xz+eps.xy, OCTAVES),
                            2.0*eps.x,
                            terrainMap(pos.xz-eps.yx, OCTAVES) - terrainMap(pos.xz+eps.yx, OCTAVES) ) );
}

float4 render( in float3 ro, in float3 rd ) {
	float3 col, bgcol;
    
    float tmax = 10000.;
    // bouding top plane
    float topd = ((MOUNTAIN_HEIGHT*INV_SCENE_SCALE)-ro.y)/rd.y;
    if( rd.y > 0.0 && topd > 0.0 ) {
        tmax = min(tmax, topd);
    }
    
    // intersect with heightmap
    float t = 1.;
	for( int i=0; i<128; i++ ) {
        float3 pos = ro + t*rd;
		float h = pos.y - terrainMap( pos.xz, 7 );
        if(abs(h)<(0.003*t) || t>tmax ) break; // use abs(h) to bounce back if under terrain
	    t += .9 * h;
	}
   	
    bgcol = col = getSkyColor(rd);
	if( t<tmax) {
		float3 pos = ro + t*rd;
        float3 nor = calcNormal( pos, t, 15);
           
        // terrain color - just back and white
        float s = smoothstep(0.5,0.9,dot(nor, v3(.3,1.,0.05)));
        col = mix( v3(.01), v3(0.5,0.52,0.6), smoothstep(.1,.7,s ));
		
        // lighting	
        // shadow is calculated based on the slope of a low frequency version of the heightmap
        float shadow = .5 + clamp( -8.+ 16.*dot(SUN_DIR, calcNormal(pos, t, 5)), 0.0, .5 );
        shadow *= smoothstep(20.,80.,pos.y);
        
        float ao = terrainMap(pos.xz, 10)-terrainMap(pos.xz,7);
        ao = clamp(.25 + ao / (MOUNTAIN_HEIGHT*INV_SCENE_SCALE) * 200., 0., 1.);

        float ambient  = max(0.5+0.5*nor.y,0.0);
		float diffuse  = max(dot(SUN_DIR, nor), 0.0);
		float backlight = max(0.5 + 0.5*dot( normalize( v3(-SUN_DIR.x, 0., SUN_DIR.z)), nor), 0.0);
	 	
        //
        // use a 3-light setup as described by Íñigo Quílez
        // https://iquilezles.org/articles/outdoorslighting
        //
		float3 lin = (diffuse*shadow*3.) * SUN_COLOR;
		lin += (ao*ambient)*v3(0.40,0.60,1.00);
        lin += (backlight)*v3(0.40,0.50,0.60);
		col *= lin;
        col *= (.6+.4*smoothstep(400.,100.,abs(pos.z))); // dark in the distance
    
        // height based fog, see https://iquilezles.org/articles/fog
        float fogAmount = heightFog(ro.y,t,rd.y);
        col = mix( col, bgcol, fogAmount);
    } else {
        t = 10000.;
    }

	return v4( col, t );
}


void mainImage( out float4 fragColor, in float2 fragCoord ) {
    if( letterBox(fragCoord, iResolution.xy, 2.25) ) {
        fragColor = v4( 0., 0., 0., 1. );
        return;
    } else {
        float3 ro, rd;
        float3 o = hash33( v3(fragCoord,iFrame) ) - 0.5; // dither
        getRay( fragCoord+o.xy, iResolution.xy, ro, rd);

        float4 res = render( ro + rd*o.z, rd );

        float2 spos = reprojectPos(ro+rd*res.w, iResolution.xy);
        spos -= o.xy/iResolution.xy; // undo dither
        
        float2 rpos = spos * iResolution.xy;
        
        if( !letterBox(rpos.xy, iResolution.xy, 2.3) 
            && historyValid()) {
            float4 ocol = sampleClamp(iChannel0, spos, 0.0 );
            res.rgb = mix(max(ocol.rgb,v3(0)), res.rgb, .125);
        }

        fragColor = res;
    }
}
[numthreads(8,8,1)] void CS_C(uint3 id:SV_DispatchThreadID) {
    if(any(id.xy >= (uint2)iResolution.xy)) return;
    float4 color; mainImage(color,(float2)id.xy+0.5);
    outputImage[id.xy]=color;
}
