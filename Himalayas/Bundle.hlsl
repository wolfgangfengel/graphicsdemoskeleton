// All five DirectX entry points share one source payload. See LICENSE.txt.
// Unique helper names keep each pass's texture bindings unchanged.
#define mainImage mainA
#include "A.hlsl"
#undef mainImage
#define mainImage mainB
#include "B.hlsl"
#undef mainImage
#define mainImage mainC
#include "C.hlsl"
#undef mainImage
#define mainImage mainD
#include "D.hlsl"
#undef mainImage
#define mainImage mainFinal
#include "Image.hlsl"
#undef mainImage
