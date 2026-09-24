/* Row-major storage and row vectors: position * World * View * Projection. */
typedef struct Matrix4 { float m[4][4]; } Matrix4;
static Matrix4 mWorld = {{
    { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 }
}};
/* Left-handed camera at (0, 0, -2), looking toward +Z, with +Y up. */
static const Matrix4 mView = {{
    { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 2, 1 }
}};
/* Perspective: 60-degree vertical FOV, 800/600 aspect, near 0.1, far 10.
 * cot(FOV/2) = sqrt(3); Direct3D maps depth into [0, 1]. */
static const Matrix4 mProjection = {{
    { 1.7320508f * WINHEIGHT / WINWIDTH, 0, 0, 0 },
    { 0, 1.7320508f, 0, 0 },
    { 0, 0, 10.0f / (10.0f - 0.1f), 1 },
    { 0, 0, -0.1f * 10.0f / (10.0f - 0.1f), 0 }
}};
static Matrix4 mWorldViewProjection;

/* The destination must be distinct from both inputs. */
static void MultiplyMatrices(Matrix4 *result, const Matrix4 *left, const Matrix4 *right)
{
    UINT row, column, k;
    for (row = 0; row < 4; ++row) {
        for (column = 0; column < 4; ++column) {
            float value = 0;
            for (k = 0; k < 4; ++k) value += left->m[row][k] * right->m[k][column];
            result->m[row][column] = value;
        }
    }
}

static void UpdateTransform(UINT elapsed)
{
    Matrix4 worldView;
    float angle = (int)elapsed * 0.002f; /* Two radians per second around Y. */
    float s, c;
    /* x87 keeps this x86 demo independent of a CRT trigonometry library. */
    __asm {
        fld angle
        fsincos
        fstp c
        fstp s
    }
    mWorld.m[0][0] = mWorld.m[2][2] = c;
    mWorld.m[0][2] = -s;
    mWorld.m[2][0] = s;
    MultiplyMatrices(&worldView, &mWorld, &mView);
    MultiplyMatrices(&mWorldViewProjection, &worldView, &mProjection);
}

