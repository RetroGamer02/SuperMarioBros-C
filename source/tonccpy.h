//# Stuff you may not have yet.

#ifndef TONCCPY_H
#define TONCCPY_H


#ifdef __cplusplus
extern "C" {
#endif

#include <3ds.h>

typedef unsigned int uint;
#define BIT_MASK(len)       ( (1<<(len))-1 )
static inline u32 quad8(u8 x)   {   x |= x<<8; return x | x<<16;    }


//# Declarations and inlines.

void tonccpya(void *dst, const void *src, uint size);
void tonccpy(void *dst, const void *src, uint size);

#ifdef __cplusplus
}
#endif
#endif
