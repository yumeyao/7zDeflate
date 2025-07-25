#ifndef ZIP7_7Z_DEFLATE_H
#define ZIP7_7Z_DEFLATE_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Deflate7zEncoder Deflate7zEncoder;

typedef struct _Deflate7zParams {
    unsigned Format : 2;       /* 0 for raw-deflate, 1 for gzip, 2 for zlib */
    unsigned Algo : 1;         /* 0 for fast mode, 1 for normal */
    unsigned MatchFinder : 1;  /* 1 for Binary Tree Matcher, 0 for Hash Chain Matcher */
    unsigned Passes : 4;       /* valid input is 0-15 */
    unsigned MatchCycles : 8;  /* 0 for auto, otherwise valid input is 1-255 */
    unsigned FastBytes : 16;   /* valid input is 3-258, otherwise clamps to the range */
} Deflate7zParams;

enum Deflate7zFormat {
    DEFLATE_7Z_RAW,
    DEFLATE_7Z_GZIP,
    DEFLATE_7Z_ZLIB,
};

/*
 * level: 1-9, corresponding to that of 7-zip program.
 *        level <= 0 uses the default level (5), level > 9 clamps to 9.
 * fb, pass: use negative number (e.g. -1) to use the preset value of the level.
 */
static Deflate7zParams Deflate7zGetParams(int format, int level);
static Deflate7zParams Deflate7zGetParamsEx(int format, int level, int fb, int pass);

static Deflate7zParams Deflate7zGetParamsEx(int format, int level, int fb, int pass) {
    const Deflate7zParams DeflatePresets[5] = {
    /*
     * The deflate preset in 7-zip actually has 4 levels, mapping to [1-4],[5-6],[7-8],[9].
     * 9 is the number just to keep consistent with LZMA preset levels.
     */
    { 0, 0, 0,  1, 32,  32 },    /* 1-2 */
    { 0, 0, 0,  1, 32,  32 },    /* 3-4 */
    { 0, 1, 1,  1, 32,  32 },    /* 5-6 */
    { 0, 1, 1,  3, 48,  64 },    /* 7-8 */
    { 0, 1, 1, 10, 80, 128 },    /* 9 */
    };
    Deflate7zParams params;
    if (level > 9) level = 9;
    if (level <= 0) level = 5;
    params = DeflatePresets[((unsigned)level - 1) >> 1];
    if (fb >= 0) {
        params.FastBytes = (unsigned)fb;
        params.MatchCycles = (16 + ((unsigned)fb >> 1));
    }
    if (pass >= 0) params.Passes = (unsigned)pass;
    switch (format) {
    case DEFLATE_7Z_GZIP: case DEFLATE_7Z_ZLIB:
        params.Format = format;
    }
    return params;
};

static Deflate7zParams Deflate7zGetParams(int format, int level) {
    return Deflate7zGetParamsEx(format, level, -1, -1);
}

/*
 * params can be created from Deflate7zGetParams[Ex](), or a user-specific one.
 * hashfunc is a zlib-compatible crc32()/adler32() if format is gzip/zlib respectively, leave it NULL if format is raw.
 * may only fail if out-of-memory, otherwise check your params if a NULL is returned.
 */
Deflate7zEncoder* Deflate7zCreateEncoder(Deflate7zParams params, unsigned long (*hash)(unsigned long, const unsigned char*, unsigned int));
void Deflate7zDestroyEncoder(Deflate7zEncoder*);

typedef struct _Deflate7zMeta {
    int CompressLevel : 16; /* gzip/zlib level, not the 7-zip deflate level, see enum Deflate7zMetaLevel */
                            /* use invalid number (e.g. -1) to let the encoder select one */
    int FileSystem : 16;    /* gzip-only: file system, use negative number (e.g. -1) to use the fixed value upon compile-time */
    int MTime;              /* gzip-only: last modified time, in the format of traditional unix time */
} Deflate7zMeta;

enum Deflate7zMetaLevel {
    DEFLATE_7Z_GZIP_COMPRESSION_DEFAULT = 0,
    DEFLATE_7Z_GZIP_COMPRESSION_BEST = 2,
    DEFLATE_7Z_GZIP_COMPRESSION_FASTEST = 4,
    DEFLATE_7Z_ZLIB_COMPRESSION_FASTEST = 0,
    DEFLATE_7Z_ZLIB_COMPRESSION_FAST = 1,
    DEFLATE_7Z_ZLIB_COMPRESSION_DEFAULT = 2,
    DEFLATE_7Z_ZLIB_COMPRESSION_MAXIMUM = 3,
};

enum Deflate7zMetaFS {
    DEFLATE_7Z_GZIP_FS_FAT = 0,
    DEFLATE_7Z_GZIP_FS_UNIX = 3,
    /* don't want to type the others */
};

/* return 0 for success, outsize is set to really compressed size */
/* mtime is only needed if the format is gzip, otherwise it's ignored */
static int Deflate7zInMemory(Deflate7zEncoder*, int mtime, const void* indata, size_t insize, void* outdata, size_t* outsize);
/* The file */
static int Deflate7zFileStream(Deflate7zEncoder*, int mtime, FILE* infile, FILE* outfile);

/* Ex version to allow specifying more header info */
int Deflate7zInMemoryEx(Deflate7zEncoder*, Deflate7zMeta meta, const void* indata, size_t insize, void* outdata, size_t* outsize);
int Deflate7zFileStreamEx(Deflate7zEncoder*, Deflate7zMeta meta, FILE* infile, FILE* outfile);

static int Deflate7zInMemory(Deflate7zEncoder* p, int mtime, const void* indata, size_t insize, void* outdata, size_t* outsize) {
    return Deflate7zInMemoryEx(p, (Deflate7zMeta){-1, -1, mtime}, indata, insize, outdata, outsize);
}
static int Deflate7zFileStream(Deflate7zEncoder* p, int mtime, FILE* infile, FILE* outfile) {
    return Deflate7zFileStreamEx(p, (Deflate7zMeta){-1, -1, mtime}, infile, outfile);
}

#ifdef __cplusplus
}
#endif

#endif