// Native API bindings for Reinder Nijhoff's Himalayas (2018).
// HLSL adaptation: see LICENSE.txt. All image intermediates retain 32-bit floats.
#ifndef HIMALAYAS_BINDINGS
#define HIMALAYAS_BINDINGS
#ifdef HIMALAYAS_VULKAN
#define BINDING(n) [[vk::binding(n,0)]]
#ifdef HIMALAYAS_IMAGE
[[vk::image_format("rgba8")]]
#else
[[vk::image_format("rgba32f")]]
#endif
#else
#define BINDING(n)
#endif
BINDING(0) RWTexture2D<float4> outputImage : register(u0);
BINDING(1) Texture2D<float4> iChannel0 : register(t0);
BINDING(2) Texture2D<float4> iChannel1 : register(t1);
BINDING(3) Texture2D<float4> iChannel2 : register(t2);
BINDING(4) Texture2D<float4> iChannel3 : register(t3);
BINDING(5) SamplerState clampSampler : register(s0);
BINDING(6) SamplerState repeatSampler : register(s1);
BINDING(7) cbuffer Parameters : register(b0) {
    float4 resolutionTimeFrame;
    float4 cameraPositionYaw;
    float4 cameraPitchHistory;
};
#define iResolution resolutionTimeFrame.xyx
#define iTime resolutionTimeFrame.z
#define iFrame ((int)resolutionTimeFrame.w)
#define mix lerp
#define fract frac
#define mod(x,y) ((x)-(y)*floor((x)/(y)))
float2 v2(float a) { return float2(a,a); }
float2 v2(float2 a) { return a; }
float2 v2(float a,float b) { return float2(a,b); }
float3 v3(float a) { return float3(a,a,a); }
float3 v3(float3 a) { return a; }
float3 v3(float a,float b,float c) { return float3(a,b,c); }
float3 v3(float2 a,float b) { return float3(a,b); }
float3 v3(float a,float2 b) { return float3(a,b); }
float4 v4(float a) { return float4(a,a,a,a); }
float4 v4(float4 a) { return a; }
float4 v4(float3 a,float b) { return float4(a,b); }
float4 v4(float2 a,float b,float c) { return float4(a,b,c); }
float4 v4(float a,float b,float c,float d) { return float4(a,b,c,d); }
int2 i2(int a) { return int2(a,a); }
int2 i2(float2 a) { return (int2)a; }
int2 i2(int a,int b) { return int2(a,b); }
// Direct accesses avoid embedding these helper functions in the HLSL bundle.
#define texelFetch(image,xy,lod) image.Load(int3(xy,lod))
#define sampleClamp(image,uv,lod) image.SampleLevel(clampSampler,uv,lod)
#define sampleRepeat(image,uv) image.SampleLevel(repeatSampler,uv,0)
#endif
