#ifndef ZIP7_7Z_DEFLATE_H
#define ZIP7_7Z_DEFLATE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct DeflateEncoder;
/* level: 1-9 */
/* use -1 for fb and pass to use level default */
struct DeflateEncoder* CreateDeflate7zEncoder(int level, int fb, int pass);
void DestroyDeflate7zEncoder(struct DeflateEncoder* p);

/* return 0 for success, outsize is set to really compressed size */
int InMemoryDeflate7z(struct DeflateEncoder* p, const void* indata, size_t insize, void* outdata, size_t* outsize);

#ifdef __cplusplus
}
#endif

#endif