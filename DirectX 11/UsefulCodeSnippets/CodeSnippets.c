////////////////////////////////////////////////////////////////////////
//
// Skeleton Intro Coding
//
// by Wolfgang Engel 
//
///////////////////////////////////////////////////////////////////////

#define Distance(a,b)                           sqrtf((a-b) * (a-b))
#define CEIL_DIV(x,y) (((x) + (y) - 1) / (y))


// http://stackoverflow.com/questions/12279914/implement-ceil-in-c

static inline uint64_t toRep(double x) {
    uint64_t r;
    memcpy(&r, &x, sizeof x);
    return r;
}

static inline double fromRep(uint64_t r) {
    double x;
    memcpy(&x, &r, sizeof x);
    return x;
}

double ceil(double x) {

    const uint64_t signbitMask  = UINT64_C(0x8000000000000000);
    const uint64_t significandMask = UINT64_C(0x000fffffffffffff);

    const uint64_t xrep = toRep(x);
    const uint64_t xabs = xrep & signbitMask;

    // If |x| is larger than 2^52 or x is NaN, the result is just x.
    if (xabs >= toRep(0x1.0p52)) return x;

    if (xabs < toRep(1.0)) {
        // If x is in (1.0, 0.0], the result is copysign(0.0, x).
        // We can generate this value by clearing everything except the signbit.
        if (x <= 0.0) return fromRep(xrep & signbitMask);
        // Otherwise x is in (0.0, 1.0), and the result is 1.0.
        else return 1.0;
    }

    // Now we know that the exponent of x is strictly in the range [0, 51],
    // which means that x contains both integral and fractional bits.  We
    // generate a mask covering the fractional bits.
    const int exponent = xabs >> 52;
    const uint64_t fractionalBits = significandMask >> exponent;

    // If x is negative, we want to truncate, so we simply mask off the
    // fractional bits.
    if (xrep & signbitMask) return fromRep(xrep & ~fractionalBits);

    // x is positive; to force rounding to go away from zero, we first *add*
    // the fractionalBits to x, then truncate the result.  The add may
    // overflow the significand into the exponent, but this produces the
    // desired result (zero significand, incremented exponent), so we just
    // let it happen.
    return fromRep(xrep + fractionalBits & ~fractionalBits);
}


//
// simple high precision timer code 
//
LARGE_INTEGER tm0, tm1;
LARGE_INTEGER fq;
 
void StartTimer()
{
  QueryPerformanceFrequency(&fq);
  QueryPerformanceCounter(&tm0);
}

// return elapsed time in miliseconds since last start
float Elapsed()
{
  QueryPerformanceCounter(&tm1);		// get reference time
  float tm = (float)((tm1.QuadPart-tm0.QuadPart)/(float)fq.QuadPart)*1000;
  return tm;
}


/*
stupid idea 247: worlds smallest vector maths library
Here is a tiny 3d vector maths library:
typedef float vec3[3];
vec3 *vec(vec3 *v, float x) {v[0]=v[1]=v[2]=x;return v;}
float *mav(vec3 *v1,vec3 *v2,vec3 *v3,vec3 *v4,vec3 *v5){
  for (int i=0; i!=3; i++) v3[i]=v1[i]*v4[i]+v2[i]*v5[i];
 return v3[0]+v3[1]+v3[2];
}
vec3 VZERO={0,0,0};  vec3 VONE={1,1,1}; vec3 VMONE={-1,-1,-1};

Thats it. It can do zero, set, add, sub, length2, dotprod, negate, lerp, centroid. e.g add is :
mav(&v1, &v2, &v3, &VONE, &VONE ); // v3=v2+v1

Lerp means that centroid (the centre vertex of a quad) can be found without a division by using two lerps. For a moment it seems cool until you realise the calls to it dont compress all that well.
The challenges remaining are xprod, normalise and max/min. then I have everything for handling my 3d modeller qoob.If I can make minor adjustments to get those, this might be worth persuing.
*/

//
// Random number generator
// see http://www.codeproject.com/KB/recipes/SimpleRNG.aspx

// These values are not magical, just the default values Marsaglia used.
// Any pair of unsigned integers should be fine.
static unsigned int m_w = 521288629;
//static unsigned int m_z = 362436069;
#define MZ ((36969 * (362436069 & 65535) + (362436069 >> 16)) << 16)

static void SetSeed(unsigned int u)
{
	m_w = u;
}

// This is the heart of the generator.
// It uses George Marsaglia's MWC algorithm to produce an unsigned integer.
// See http://www.bobwheeler.com/statistics/Password/MarsagliaPost.txt
static unsigned int GetUint()
{
//	m_z = 36969 * (m_z & 65535) + (m_z >> 16);
	m_w = 18000 * (m_w & 65535) + (m_w >> 16);
	return (MZ) + m_w;
}


// Produce a uniform random sample from the open interval (0, 1).
// The method will not return either end point.
/*
static float GetUniform()
{
    // 0 <= u < 2^32
    unsigned int u = GetUint();
	// The magic number below is 1/(2^32 + 2).
    // The result is strictly between 0 and 1.
    return (u + 1.0) * 2.328306435454494e-10;
}
*/

// Produce a uniform random sample from the interval (-1, 1).
// The method will not return either end point.
static float GetUniform()
{
    // 0 <= u < 2^32
    unsigned int u = GetUint();
	// The magic number below is 1/(2^32 + 2).
    // The result is strictly between 0 and 1.
    return (u) * 2.328306435454494e-10 * 2.0;
}



// some simplified C lib functions:
// http://stackoverflow.com/questions/7824239/exp-function-using-c
double factorial(const int k)
{
    int prod = 1;
    for(int i=1; i<=k; i++)
        prod = i * prod;
    return prod;
}

double power(const double base, const int exponent)
{
    double result = 1;
    for(int i=1; i<=exponent; i++)
        result = result * base;
    return result;
}


double my_exp(double x)
{
    double sum = 1.0 + x;
    double term = x;                 // term for k = 1 is just x
    for (int k = 2; k < 50; k++)
    {
        term = term * x / (double)k; // term[k] = term[k-1] * x / k
        sum = sum + term;
    }
    return sum;
}
