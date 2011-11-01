////////////////////////////////////////////////////////////////////////
//
// Skeleton Intro Coding
//
// by Wolfgang Engel 
//
///////////////////////////////////////////////////////////////////////



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