// HLSL adaptation; see LICENSE.txt and Original/.
#ifndef HIMALAYAS_COMMON
#define HIMALAYAS_COMMON
// Himalayas. Created by Reinder Nijhoff 2018
// @reindernijhoff
//
// https://www.shadertoy.com/view/MdGfzh
//
// This is my first attempt to render volumetric clouds in a fragment shader.
//
// 1 unit correspondents to SCENE_SCALE meter.

#define SCENE_SCALE (10.)
#define INV_SCENE_SCALE (.1)

#define MOUNTAIN_HEIGHT (5000.)
#define MOUNTAIN_HW_RATIO (0.00016)

#define SUN_DIR normalize(v3(-.7,.5,.75))
#define SUN_COLOR (v3(1.,.9,.85)*1.4)

#define FLAG_POSITION (v3(3900.5,720.,-2516.)*INV_SCENE_SCALE)
#define HUMANOID_SCALE (2.)

#define CAMERA_FL 2.

#define HEIGHT_BASED_FOG_B 0.02
#define HEIGHT_BASED_FOG_C 0.05
// Integrate world-anchored height fog from any free-camera altitude. Expanding
// the two endpoint densities avoids overflow times zero on long downward rays.
float heightFog(float height,float distance,float vertical) {
    float density=exp(-(height-73.)*HEIGHT_BASED_FOG_B);
    float integral=abs(vertical)>.00001?
        (density-exp(-(height+distance*vertical-73.)*HEIGHT_BASED_FOG_B))/vertical:
        distance*HEIGHT_BASED_FOG_B*density;
    return saturate(HEIGHT_BASED_FOG_C*integral);
}


float3x3 getCamera( out float3 ro ) {
    ro = cameraPositionYaw.xyz;
    float yaw = cameraPositionYaw.w, pitch = cameraPitchHistory.x;
    float3 cw = v3(sin(yaw)*cos(pitch),sin(pitch),cos(yaw)*cos(pitch));
	float3 cp = v3(0.0,1.0, 0.0);
	float3 cu = normalize( cross(cw,cp) );
	float3 cv = normalize( cross(cu,cw) );
    return float3x3( cu, cv, cw );
}

void getRay( in float2 fragCoord, in float2 resolution, out float3 ro, out float3 rd) {
	float3x3 cam = getCamera( ro );
    float2 p = (-resolution.xy + 2.0*(fragCoord))/resolution.y;
    rd = mul(normalize(v3(p,CAMERA_FL)),cam);     
}

//
// To reduce noise I use temporal reprojection (both for clouds (Buffer D) and the terrain 
// (Buffer C) seperatly. The temporal repojection code is based on code from the shader
// "Rain Forest" (again by Íñigo Quílez):
//
// https://www.shadertoy.com/view/4ttSWf
// 
float4 saveCamera( in float2 fragCoord ) {
    float3 ro;
    float3x3 cam = getCamera( ro );
    float4 fragColor = 0;
    
    if( abs(fragCoord.x-4.5)<0.5 ) fragColor = v4( cam[2], -dot(cam[2],ro) );
    if( abs(fragCoord.x-3.5)<0.5 ) fragColor = v4( cam[1], -dot(cam[1],ro) );
    if( abs(fragCoord.x-2.5)<0.5 ) fragColor = v4( cam[0], -dot(cam[0],ro) );
    
    return fragColor;
}

float2 reprojectPos( in float3 pos, in float2 resolution ) {
    float4x4 oldCam = float4x4( texelFetch(iChannel1,i2(2,0),0),
                        texelFetch(iChannel1,i2(3,0),0),
                        texelFetch(iChannel1,i2(4,0),0),
                        0.0, 0.0, 0.0, 1.0 );

    float4 wpos = v4(pos,1.0);
    float3 cpos = (mul(oldCam,wpos)).xyz; 
    if(cpos.z <= .001) return v2(-10); // Previous camera looked away from this point.
    float2 npos = CAMERA_FL * cpos.xy / cpos.z;
    return 0.5 + 0.5*npos*v2(resolution.y/resolution.x,1.0);
}

bool historyValid() {
    float4 old = texelFetch(iChannel1,i2(0),0);
    float dt = iTime-old.z;
    return iFrame>0 && all(old.xy==iResolution.xy) && old.w==cameraPitchHistory.y && dt>=0. && dt<.25;
}

//
// Fast skycolor function by Íñigo Quílez
// https://www.shadertoy.com/view/MdX3Rr
//
float3 getSkyColor(float3 rd) {
    float sundot = clamp(dot(rd,SUN_DIR),0.0,1.0);
	float3 col = v3(0.2,0.5,0.85)*1.1 - max(rd.y,0.01)*max(rd.y,0.01)*0.5;
    col = mix( col, 0.85*v3(0.7,0.75,0.85), pow(1.0-max(rd.y,0.0), 6.0) );

    col += 0.25*v3(1.0,0.7,0.4)*pow( sundot,5.0 );
    col += 0.25*v3(1.0,0.8,0.6)*pow( sundot,64.0 );
    col += 0.20*v3(1.0,0.8,0.6)*pow( sundot,512.0 );
    
    col += clamp((0.1-rd.y)*10., 0., 1.) * v3(.0,.1,.2);
    col += 0.2*v3(1.0,0.8,0.6)*pow( sundot, 8.0 );
    return col;
}

bool letterBox(float2 fragCoord, const float2 resolution, const float aspect) { 
    if( fragCoord.x < 0. || fragCoord.x > resolution.x ||
        abs(2.*fragCoord.y-resolution.y) > resolution.x * (1./aspect) ) {
        return true;
    } else {
        return false;
    }
}

//
// Noise functions
//
// Hash without Sine by DaveHoskins 
//
// https://www.shadertoy.com/view/4djSRW
//
float hash12( float2 p ) {
    p  = 50.0*fract( p*0.3183099 );
    return fract( p.x*p.y*(p.x+p.y) );
}

float hash13(float3 p3) {
    p3  = fract(p3 * 1031.1031);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

float3 hash33(float3 p3) {
	p3 = fract(p3 * v3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+19.19);
    return fract((p3.xxy + p3.yxx)*p3.zyx);
}

float valueHash(float3 p3) {
    p3  = fract(p3 * 0.1031);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

//
// Noise functions used for cloud shapes
//
float valueNoise( in float3 x, float tile ) {
    float3 p = floor(x);
    float3 f = fract(x);
    f = f*f*(3.0-2.0*f);
	
    return mix(mix(mix( valueHash(mod(p+v3(0,0,0),tile)), 
                        valueHash(mod(p+v3(1,0,0),tile)),f.x),
                   mix( valueHash(mod(p+v3(0,1,0),tile)), 
                        valueHash(mod(p+v3(1,1,0),tile)),f.x),f.y),
               mix(mix( valueHash(mod(p+v3(0,0,1),tile)), 
                        valueHash(mod(p+v3(1,0,1),tile)),f.x),
                   mix( valueHash(mod(p+v3(0,1,1),tile)), 
                        valueHash(mod(p+v3(1,1,1),tile)),f.x),f.y),f.z);
}

float voronoi( float3 x, float tile ) {
    float3 p = floor(x);
    float3 f = fract(x);

    float res = 100.;
    for(int k=-1; k<=1; k++){
        for(int j=-1; j<=1; j++) {
            for(int i=-1; i<=1; i++) {
                float3 b = v3(i, j, k);
                float3 c = p + b;

                if( tile > 0. ) {
                    c = mod( c, v3(tile) );
                }

                float3 r = v3(b) - f + hash13( c );
                float d = dot(r, r);

                if(d < res) {
                    res = d;
                }
            }
        }
    }

    return 1.-res;
}

float tilableVoronoi( float3 p, const int octaves, float tile ) {
    float f = 1.;
    float a = 1.;
    float c = 0.;
    float w = 0.;

    if( tile > 0. ) f = tile;

    for( int i=0; i<octaves; i++ ) {
        c += a*voronoi( p * f, f );
        f *= 2.0;
        w += a;
        a *= 0.5;
    }

    return c / w;
}

float tilableFbm( float3 p, const int octaves, float tile ) {
    float f = 1.;
    float a = 1.;
    float c = 0.;
    float w = 0.;

    if( tile > 0. ) f = tile;

    for( int i=0; i<octaves; i++ ) {
        c += a*valueNoise( p * f, f );
        f *= 2.0;
        w += a;
        a *= 0.5;
    }

    return c / w;
}
#endif

