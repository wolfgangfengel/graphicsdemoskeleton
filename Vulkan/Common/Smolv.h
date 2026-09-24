/* SMOL-V v1 decoder for the trusted embedded shaders in these demos.
 * Adapted from Aras Pranckevicius's smolv::Decode (2016-2024), MIT license:
 * ../../Tools/smol-v/license.txt. The pinned upstream source is kept there.
 * C, no allocation or CRT; deliberately omits legacy version-0 support.
 * Every build verifies this decoder's output byte-for-byte against SPIR-V.
 */
#ifndef TINY_SMOLV_H
#define TINY_SMOLV_H
#include "SmolvOps.h"
typedef struct {
    const unsigned char *next,*end;
    unsigned int *out,*outEnd;
    int valid;
} SMOL_READER;
static unsigned int smolWord(const unsigned char *p)
{
    return (unsigned int)p[0]|((unsigned int)p[1]<<8)|((unsigned int)p[2]<<16)|((unsigned int)p[3]<<24);
}
static unsigned int smolRead(SMOL_READER *r)
{
    unsigned int value=0,shift;
    for(shift=0;shift<35;shift+=7) {
        unsigned int b;
        if(r->next==r->end) break;
        b=*r->next++;
        if(shift==28 && (b&240)) break;
        value|=(b&127)<<shift;
        if(!(b&128)) return value;
    }
    r->valid=0; return 0;
}
static unsigned int smolRaw(SMOL_READER *r)
{
    unsigned int value;
    if(r->end-r->next<4) {r->valid=0;return 0;}
    value=smolWord(r->next); r->next+=4; return value;
}
static void smolWrite(SMOL_READER *r,unsigned int value)
{
    if(r->out==r->outEnd) r->valid=0; else *r->out++=value;
}
static unsigned int smolZig(unsigned int value) { return (value>>1)^(0u-(value&1)); }
static unsigned int smolOpcode(unsigned int op)
{
    static const unsigned char map[16]={71,61,62,65,79,5,6,72,248,59,133,129,12,13,32,127};
    unsigned int n;
    if(op<16) return map[op];
    for(n=0;n<16;++n) if(op==map[n]) return n;
    return op;
}
static unsigned int smolSize(const void *input,unsigned int size)
{
    const unsigned char *p=(const unsigned char *)input;
    if(size<24 || smolWord(p)!=0x534d4f4c || p[7]!=1) return 0;
    return smolWord(p+20);
}
static int smolDecode(const void *input,unsigned int size,void *output,unsigned int capacity)
{
    const unsigned char *bytes=(const unsigned char *)input;
    unsigned int needed=smolSize(input,size),previousResult=0,previousDecoration=0,n;
    SMOL_READER r;
    if(!needed || needed>capacity || (needed&3) || !output) return 0;
    r.next=bytes+24; r.end=bytes+size; r.out=(unsigned int *)output; r.outEnd=r.out+needed/4; r.valid=1;
    smolWrite(&r,0x07230203);
    smolWrite(&r,smolWord(bytes+4)&0xffffff);
    for(n=8;n<20;n+=4) smolWrite(&r,smolWord(bytes+n));
    while(r.next<r.end && r.valid) {
        unsigned int value=smolRead(&r),length=((value>>20)<<4)|((value>>4)&15);
        unsigned int op=smolOpcode(((value>>4)&0xfff0)|(value&15)),offset=1,metadata,relative;
        int swizzle=op==13;
        ++length;
        if(op==79 || swizzle) length+=4;
        if(op==71) length+=2;
        if(op==61 || op==65) length+=3;
        if(swizzle) op=79;
        metadata=op<sizeof(smolOps)?smolOps[op]:0;
        smolWrite(&r,(length<<16)|op);
        if(metadata&2) {smolWrite(&r,smolRead(&r));++offset;}
        if(metadata&1) {previousResult+=smolZig(smolRead(&r));smolWrite(&r,previousResult);++offset;}
        if(op==71 || op==72) {previousDecoration+=smolZig(smolRead(&r));smolWrite(&r,previousDecoration);++offset;}
        if(op==72) {
            unsigned int count,index=0,previousOffset=0;
            if(r.next==r.end) return 0;
            count=*r.next++;
            for(n=0;n<count && r.valid;++n) {
                unsigned int decoration,memberLength=4,i;
                index+=smolRead(&r); decoration=smolRead(&r);
                if(decoration>=29 && decoration<=37) ++memberLength;
                else if(!(decoration==0 || (decoration>=2 && decoration<=5))) memberLength+=smolRead(&r);
                if(n) {smolWrite(&r,(memberLength<<16)|op);smolWrite(&r,previousDecoration);}
                smolWrite(&r,index); smolWrite(&r,decoration);
                if(decoration==35) {previousOffset+=smolRead(&r);smolWrite(&r,previousOffset);}
                else for(i=4;i<memberLength && r.valid;++i) smolWrite(&r,smolRead(&r));
            }
            continue;
        }
        relative=metadata>>3;
        for(n=0;n<relative && offset<length;++n,++offset) smolWrite(&r,previousResult-smolZig(smolRead(&r)));
        if(swizzle && length<=9) {
            if(r.next==r.end) return 0;
            value=*r.next++;
            for(n=5;n<length;++n) smolWrite(&r,(value>>((8-n)*2))&3);
        } else for(;offset<length && r.valid;++offset) smolWrite(&r,(metadata&4)?smolRead(&r):smolRaw(&r));
    }
    return r.valid && r.next==r.end && r.out==r.outEnd;
}
#endif
