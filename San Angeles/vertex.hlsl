// San Angeles Observation: fixed-function lighting and perspective in SM5.
// Scene data derived from Jetro Lauha's demo. See license-BSD.txt.
cbuffer Scene : register(b0) {
    float4 eye, cameraRight, cameraUp, cameraForward;
    float4 translateScale, rotateMirrorLight;
};
struct Output { float4 position : SV_POSITION; nointerpolation float4 color : COLOR; };
#ifdef SAN_ANGELES_DX12
[RootSignature("RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), RootConstants(num32BitConstants=24, b0, visibility=SHADER_VISIBILITY_VERTEX)")]
#endif
Output VShader(float3 position : POSITION, float3 normal : NORMAL, float4 color : COLOR)
{
    Output output;
    float2 rotation = rotateMirrorLight.xy;
    float3 p = position * translateScale.w;
    p.xy = float2(p.x*rotation.x-p.y*rotation.y, p.x*rotation.y+p.y*rotation.x);
    p += translateScale.xyz;
    p.z *= rotateMirrorLight.z;
    float3 q = p-eye.xyz;
    float depth = dot(q,cameraForward.xyz);
    output.position = float4(dot(q,cameraRight.xyz)*cameraRight.w,
        dot(q,cameraUp.xyz)*2.41421356237, depth*(150.0/149.5)-75.0/149.5, depth);
    float3 n = normal;
    n.xy = float2(n.x*rotation.x-n.y*rotation.y, n.x*rotation.y+n.y*rotation.x);
    n.z *= rotateMirrorLight.z;
    float3 light0 = normalize(float3(-4,1,1));
    float3 illumination = 0.2 + saturate(dot(n,light0))*float3(1,0.4,0)
        + saturate(dot(n,normalize(float3(1,-2,-1))))*float3(0.07,0.14,0.35)
        + saturate(dot(n,normalize(float3(-1,0,-4))))*float3(0.07,0.17,0.14);
    float specular = pow(saturate(dot(n,normalize(light0-cameraForward.xyz))),60);
    if (dot(n,light0) <= 0) specular = 0;
    // The floor multiplies the already-faded reflection. Fading only lit
    // geometry (and the clear color) equals the original final fade quad.
    output.color = float4(rotateMirrorLight.w ? saturate(color.rgb*illumination+specular)*eye.w : color.rgb,1);
    return output;
}
