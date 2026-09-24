#define WINWIDTH 800
#define WINHEIGHT 600
#define DEMO_DURATION 3300
#define DEMO_TITLE L"Skeleton - Vulkan"
#define DEMO_SKELETON
#include "../Common/Vulkan.h"
static void demoInitialize(void) {}
static void demoFrame(UINT elapsed,UINT delta) { (void)elapsed; (void)delta; }
static void demoShutdown(void) {}
#include "../Common/Entry.h"
