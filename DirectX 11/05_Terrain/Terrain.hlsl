// Mountains. By David Hoskins - 2013
// https://www.shadertoy.com/view/4slGD4
// Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported.
// https://creativecommons.org/licenses/by-nc-sa/3.0/
// HLSL / DirectX 11 adaptation for graphicsdemoskeleton, 2026.
// Changes: compute entry, explicit GLSL matrix orientation and descending
// smoothstep, shared noise, terrain-height and ray-march loops,
// procedural water-glint mask, bounded final output,
// and a lower constant cruise altitude with faster travel along each route.
// Original credits: Inigo Quilez for noise; Croqueteer for optional stereo
// code (the standalone demo uses the original default monoscopic view).
// See LICENSE-Mountains.txt for attribution and distribution terms.

RWTexture2D<float4> output : register(u0);
cbuffer FrameConstants : register(b0)
{
    float elapsedSeconds;
    float width;
    float height;
    float mouseX;
};

static float treeLine = 0.0;
static float treeCol = 0.0;
static float specular = 0.0;
static float ambient = 0.0;
static float3 cameraPos = 0.0;
static const float3 sunLight = normalize(float3(0.4, 0.4, 0.48));
static const float3 sunColour = float3(1.0, 0.9, 0.83);
groupshared float groupCruiseAltitude;
static const float cameraPhaseSpeed = 0.01125; // 25% faster than 0.009.

float Hash12(float2 p)
{
    // This fractional hash is sensitive to fused multiply/add contraction.
    precise float3 p3 = frac(p.xyx * 0.1031);
    p3 += dot(p3, p3.yzx + 19.19);
    precise float h = (p3.x + p3.y) * p3.z;
    return frac(h);
}

float2 Hash22(float2 p)
{
    precise float3 p3 = frac(p.xyx * float3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 19.19);
    precise float2 h = (p3.xx + p3.yz) * p3.zy;
    return frac(h);
}

float Noise(float2 x)
{
    float2 p = floor(x);
    float2 f = frac(x);
    f = f*f*(3.0-2.0*f);
    // Shift the four corners through one hash evaluator to limit bytecode.
    float4 n = 0.0;
    [loop] for (int corner = 0; corner < 4; ++corner)
        n = float4(n.yzw, Hash12(p+float2(corner & 1, corner >> 1)));
    return lerp(lerp(n.x,n.y,f.x),lerp(n.z,n.w,f.x),f.y);
}

float2 Noise2(float2 x)
{
    float2 p = floor(x);
    float2 f = frac(x);
    f = f*f*(3.0-2.0*f);
    float4 nx = 0.0, ny = 0.0;
    [loop] for (int corner = 0; corner < 4; ++corner) {
        float2 n = Hash22(p+float2(corner & 1, corner >> 1));
        nx = float4(nx.yzw,n.x);
        ny = float4(ny.yzw,n.y);
    }
    return lerp(lerp(float2(nx.x,ny.x),float2(nx.y,ny.y),f.x),
                lerp(float2(nx.z,ny.z),float2(nx.w,ny.w),f.x),f.y);
}

float2 RotateTerrain(float2 p)
{
    // GLSL mat2 constructors supply columns; HLSL constructors supply rows.
    return mul(float2x2(1.3623, -1.7131, 1.7531, 1.4623), p);
}

static const int HEIGHT_COARSE = 0;
static const int HEIGHT_MARCH = 1;
static const int HEIGHT_NORMAL = 2;

float TerrainHeight(float2 p, int mode)
{
    float2 pos = p*0.05;
    float w = 0.0, f = 0.0, ff = 0.0;
    int stages = mode == HEIGHT_COARSE ? 7 : mode == HEIGHT_MARCH ? 10 : 14;
    // Stages: 0 amplitude, 1-5 coarse octaves, 6 broad mountain relief.
    // Marching adds tree-line noise at 7-8 and tree height at 9.
    // Normals add tree height at 7, then finer octaves at 8-13 if needed.
    // One noise evaluator serves every stage, preserving evaluation order.
    [loop] for (int stage = 0; stage < stages; ++stage) {
        float2 samplePos = stage == 0 ? pos*0.25 : stage == 6 ? pos*0.002 : pos;
        if (mode == HEIGHT_MARCH && stage >= 7)
            samplePos = p*(stage == 7 ? 0.3 : stage == 8 ? 3.3 : 13.0);
        if (mode == HEIGHT_NORMAL && stage == 7) samplePos = p*13.0;
        float n = Noise(samplePos);
        if (stage == 0) {
            w = n*0.75+0.15;
            w = 66.0*w*w;
        } else if (stage == 6) {
            f = f+pow(abs(n),5.0)*275.0-5.0;
        } else if (mode == HEIGHT_MARCH && stage >= 7) {
            if (stage == 7) ff = n;
            else if (stage == 8) {
                ff += n*0.5;
                treeLine = smoothstep(ff,ff*2.0,f)*(1.0-smoothstep(0.4+ff,1.0+ff*3.0,f));
            } else treeCol = n*treeLine;
        } else if (mode == HEIGHT_NORMAL && stage == 7) {
            treeCol = n*treeLine;
            f += treeCol;
            if (treeCol > 0.0) break;
        } else {
            f += w*n;
            w *= -0.4;
            pos = RotateTerrain(pos);
        }
    }
    return f;
}

float Terrain(float2 p)
{
    return TerrainHeight(p,HEIGHT_COARSE);
}

float Map(float3 p)
{
    float h = TerrainHeight(p.xz,HEIGHT_MARCH);
    return p.y-h-treeCol;
}

float Terrain2(float2 p)
{
    return TerrainHeight(p,HEIGHT_NORMAL);
}

float FractalNoise(float2 xy)
{
    float w = 0.7;
    float f = 0.0;
    [loop] for (int i = 0; i < 4; ++i) {
        f += Noise(xy)*w;
        w *= 0.5;
        xy *= 2.7;
    }
    return f;
}

float3 GetClouds(float3 sky, float3 rd)
{
    if (rd.y < 0.01) return sky;
    float v = (200.0-cameraPos.y)/rd.y;
    rd.xz = (rd.xz*v+cameraPos.xz)*0.010;
    float f = (FractalNoise(rd.xz)-0.55)*5.0;
    return lerp(sky,float3(0.55,0.55,0.52),saturate(f*rd.y-0.1));
}

float3 GetSky(float3 rd)
{
    float sunAmount = max(dot(rd,sunLight),0.0);
    float v = pow(1.0-max(rd.y,0.0),5.0)*0.5;
    float3 sky = v*sunColour*0.4+float3(0.11,0.22,0.5);
    sky += sunColour*pow(sunAmount,6.5)*0.32;
    sky += sunColour*min(pow(sunAmount,1150.0),0.3)*0.65;
    return sky;
}

float3 ApplyFog(float3 rgb, float dis, float3 dir)
{
    return lerp(GetSky(dir),rgb,exp(-dis*0.00005));
}

void DoLighting(inout float3 mat, float3 normal, float3 eyeDir)
{
    float h = dot(sunLight,normal);
    mat *= sunColour*(max(h,0.0)+ambient);
    if (h > 0.0) {
        float3 r = reflect(sunLight,normal);
        float amount = pow(max(dot(r,normalize(eyeDir)),0.0),3.0)*specular;
        mat = lerp(mat,sunColour,amount);
    }
}

float3 TerrainColour(float3 pos, float3 normal, float dis)
{
    specular = 0.0;
    ambient = 0.1;
    float3 dir = normalize(pos-cameraPos);
    float3 matPos = pos*2.0;
    float disSqrd = dis*dis;

    float f = saturate(Noise(matPos.xz*0.05));
    f += Noise(matPos.xz*0.1+normal.yz*1.08)*0.85;
    f *= 0.55;
    float3 m = lerp(float3(0.63*f+0.2,0.7*f+0.1,0.7*f+0.1),
                    float3(f*0.43+0.1,f*0.3+0.2,f*0.35+0.1),f*0.65);
    float3 mat = m*(f*m+float3(0.36,0.30,0.28));
    if (normal.y < 0.5) {
        float c = (0.5-normal.y)*4.0;
        c = clamp(c*c,0.1,1.0);
        f = Noise(float2(matPos.x*0.09,matPos.z*0.095+matPos.y*0.15));
        f += Noise(float2(matPos.x*2.233,matPos.z*2.23))*0.5;
        mat = lerp(mat,(0.4*f).xxx,c);
        specular += 0.1;
    }
    if (matPos.y < 45.35 && normal.y > 0.65) {
        m = float3(Noise(matPos.xz*0.023)*0.5+0.15,Noise(matPos.xz*0.03)*0.6+0.25,0.0);
        m *= (normal.y-0.65)*0.6;
        mat = lerp(mat,m,saturate((normal.y-0.65)*1.3*(45.35-matPos.y)*0.1));
    }
    if (treeCol > 0.0) {
        mat = float3(0.02+Noise(matPos.xz*5.0)*0.03,0.05,0.0);
        float perturb = Noise(matPos.xz*33.0)-0.5;
        normal = normalize(normal+float3(perturb,0.0,perturb));
        specular = 0.0;
    }
    if (matPos.y > 80.0 && normal.y > 0.42) {
        float snow = saturate((matPos.y-80.0-Noise(matPos.xz*0.1)*28.0)*0.035);
        mat = lerp(mat,float3(0.7,0.7,0.8),snow);
        specular += snow;
        ambient += snow*0.3;
    }
    if (matPos.y < 1.45) {
        if (normal.y > 0.4) {
            f = Noise(matPos.xz*0.084)*1.5;
            f = clamp((1.45-f-matPos.y)*1.34,0.0,0.67);
            float t = normal.y-0.4;
            t *= t;
            mat = lerp(mat,float3(0.09+t,0.07+t,0.03+t),f);
        }
        if (matPos.y < 0.0) mat *= 0.2;
    }
    DoLighting(mat,normal,dir);

    if (matPos.y < 0.0) {
        float time = elapsedSeconds*0.03;
        float3 watPos = matPos-dir*(matPos.y/dir.y);
        float tx = cos(watPos.x*0.052)*4.5;
        float tz = sin(watPos.z*0.072)*4.5;
        float2 co = Noise2(float2(watPos.x*4.7+1.3+tz,watPos.z*4.69+time*35.0-tx));
        co += Noise2(float2(watPos.z*8.6+time*13.0-tx,watPos.x*8.712+tz))*0.4;
        float3 nor = normalize(float3(co.x,20.0,co.y));
        nor = normalize(reflect(dir,nor));
        tx = watPos.y-matPos.y;
        mat = lerp(mat,GetClouds(GetSky(nor)*float3(0.3,0.3,0.5),nor)*0.1+float3(0.0,0.02,0.03),clamp(tx*0.4,0.6,1.0));
        // Replace the external pebble texture's glint mask with local noise.
        float glintMask = Noise(watPos.xz*8.0);
        mat += 0.1*saturate(1.0-pow(tx+0.5,3.0)*glintMask);
        mat += sunColour*pow(max(dot(nor,sunLight),0.0),228.5)*0.6;
        float3 waterDistance = (watPos-cameraPos*2.0)*0.5;
        disSqrd = dot(waterDistance,waterDistance);
    }
    return ApplyFog(mat,disSqrd,dir);
}

bool Scene(float3 ro, float3 rd, out float resT, float2 fragCoord)
{
    float t = 1.0+Hash12(fragCoord);
    float oldT = 0.0;
    float2 interval = 0.0;
    int refinement = 0;
    resT = 0.0;
    // Reuse one Map evaluation for marching and the five refinement steps.
    [loop] for (int j = 0; j < 155; ++j) {
        if (refinement == 0 && (j >= 150 || t > 240.0)) break;
        float h = Map(ro+t*rd);
        if (refinement != 0) {
            if (refinement == 5) {
                resT = t;
                return true;
            }
            interval = h >= 0.5 ? float2(t,interval.y) : float2(interval.x,t);
            ++refinement;
        } else {
            if (h >= 0.5) {
                oldT = t;
                t += max(0.01,0.3*h)+t*0.0065;
                continue;
            }
            interval = float2(oldT,t);
            refinement = 1;
        }
        t = dot(interval,float2(0.5,0.5));
    }
    return false;
}

float3 PathPosition(float phase)
{
    float2 p = 476.0*float2(sin(3.5*phase),cos(1.5*phase));
    return float3(35.0-p.x,0.6,4108.0+p.y);
}

float RoutePhase(float routeMouseX)
{
    float m = 1.0+(routeMouseX/width)*300.0;
    // Start earlier on the same path so the faster flight keeps its endpoint.
    return (m+657.0)*0.006-30.0*(cameraPhaseSpeed-0.009);
}

float3 CameraPath(float t)
{
    return PathPosition(t+elapsedSeconds*cameraPhaseSpeed+RoutePhase(mouseX));
}

float CruiseAltitude(float routeMouseX)
{
    // Survey the entire 30-second route, independently of the current time.
    // A fixed mouse offset therefore produces a fixed world-space altitude.
    float phase = RoutePhase(routeMouseX);
    float summit = 0.0;
    [loop] for (int sample = 0; sample <= 128; ++sample) {
        // Match Window.c's 30-second playback interval and CameraPath's speed.
        float2 p = PathPosition(phase+sample*(30.0*cameraPhaseSpeed/128.0)).xz;
        summit = max(summit,Terrain(p));
    }
    return max(21.0,summit+9.0);
}

float3 RenderMountains(float2 fragCoord, float cruiseAltitude)
{
    float2 xy = -1.0+2.0*fragCoord/float2(width,height);
    float2 uv = xy*float2(width/height,1.0);
    cameraPos.xz = CameraPath(0.0).xz;
    float3 target = CameraPath(0.1);
    cameraPos.y = cruiseAltitude;
    // Keep a shallow, fixed downward pitch instead of tipping with altitude.
    target.y = cameraPos.y-0.08*length(target.xz-cameraPos.xz);
    float roll = 0.15*sin(elapsedSeconds*0.2);
    float3 cw = normalize(target-cameraPos);
    float3 cp = float3(sin(roll),cos(roll),0.0);
    float3 cu = normalize(cross(cw,cp));
    float3 cv = normalize(cross(cu,cw));
    float3 rd = normalize(uv.x*cu+uv.y*cv+1.5*cw);

    float3 col;
    float distance;
    if (!Scene(cameraPos,rd,distance,fragCoord)) col = GetClouds(GetSky(rd),rd);
    else {
        float3 pos = cameraPos+distance*rd;
        float p = 0.02+0.00005*distance*distance;
        float3 heights = 0.0;
        // Preserve sample order and treeCol side effects, with one evaluator.
        [loop] for (int sample = 0; sample < 3; ++sample) {
            float2 offset = sample == 1 ? float2(p,0) : sample == 2 ? float2(0,-p) : float2(0,0);
            float sampleHeight = Terrain2(pos.xz+offset);
            if (sample == 0) heights.x = sampleHeight;
            else if (sample == 1) heights.y = sampleHeight;
            else heights.z = sampleHeight;
        }
        float3 v2 = float3(-p,heights.x-heights.y,0);
        float3 v3 = float3(0,heights.x-heights.z,p);
        float3 normal = normalize(cross(v2,v3));
        col = TerrainColour(pos,normal,distance);
    }
    // Match the source's post curve and the UNORM display's clipping.
    return saturate((1.0-exp(-col*6.0))*1.0024);
}

[numthreads(8,8,1)]
void CS_Terrain(uint3 id : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    // The route is identical for every pixel; survey it once per thread group.
    if (groupIndex == 0) groupCruiseAltitude = CruiseAltitude(mouseX);
    GroupMemoryBarrierWithGroupSync();
    // Keep the bounds check after the barrier so edge groups cannot deadlock.
    if (id.x >= (uint)width || id.y >= (uint)height) return;
    float2 pixel = float2(id.x+0.5,height-id.y-0.5);
    output[id.xy] = float4(RenderMountains(pixel,groupCruiseAltitude),1.0);
}
