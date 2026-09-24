/* Julia animation is shared by the Vulkan Julia and PostFX examples. */
typedef struct MainConstantBuffer {
    float diffuse[4], mu[4], epsilon;
    int width, height, selfShadow;
    float orientation[16], zoom, padding[3];
} MainConstantBuffer;

static MainConstantBuffer mConstants = {
    { 0 }, { 0 }, 0.003f, WINWIDTH, WINHEIGHT, TRUE,
    { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }, 1, { 0 }
};
static float mMuA[4] = { -0.278f, -0.479f, 0, 0 };
static float mMuB[4] = { 0.278f, 0.479f, 0, 0 };
static float mColorA[4] = { 0.25f, 0.45f, 1, 1 };
static float mColorB[4] = { 0.25f, 0.45f, 1, 1 };
static float mPhase;
static UINT mRandom;

/* Same LCG and 24-bit samples in [-1, 1) as the DX11 intro. */
static float GetUniform(void)
{
    mRandom = 1664525u * mRandom + 1013904223u;
    return (float)(mRandom >> 8) * (1.0f / 8388608.0f) - 1.0f;
}

static void Interpolate(float *result, const float *a, const float *b)
{
    UINT n;
    for (n = 0; n < 4; ++n) result[n] = (1.0f - mPhase) * a[n] + mPhase * b[n];
}

static void UpdateAnimation(UINT delta)
{
    UINT n;
    /* Mu and color share the same approximately 20-second interval. */
    mPhase += 0.01f * ((int)delta * (1.0f / 200.0f));
    if (mPhase >= 1.0f) {
        mPhase = 0;
        for (n = 0; n < 4; ++n) {
            mMuA[n] = mMuB[n];
            mMuB[n] = GetUniform();
            mColorA[n] = mColorB[n];
        }
        do {
            for (n = 0; n < 3; ++n) mColorB[n] = GetUniform();
        } while (mColorB[0] < 0 && mColorB[1] < 0 && mColorB[2] < 0);
    }
    Interpolate(mConstants.mu, mMuA, mMuB);
    Interpolate(mConstants.diffuse, mColorA, mColorB);
}

