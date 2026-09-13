#ifndef TINY_RUNTIME_H
#define TINY_RUNTIME_H

/* Compiler support only: no CRT startup, imports, allocation, or constructors.
 * /Gy and the linker's dead-code removal discard unused helpers. */
#if !defined(_M_IX86)
#error These size-focused demos and their custom entry point target Win32/x86.
#endif

int _fltused = 0;

void * __cdecl memcpy(void *destination, const void *source, unsigned int count)
{
    char *out = (char *)destination;
    const char *in = (const char *)source;
    while (count--) *out++ = *in++;
    return destination;
}

void * __cdecl memset(void *destination, int value, unsigned int count)
{
    unsigned char *out = (unsigned char *)destination;
    while (count--) *out++ = (unsigned char)value;
    return destination;
}

#endif
