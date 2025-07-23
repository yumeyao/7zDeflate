#include <string.h>
#include <memory>
#include "7zDeflate.h"
#include "7z/CpuArch.h"
#include "7z/DeflateEncoder.h"

using namespace NCompress::NDeflate::NEncoder;

struct DeflateEncoder {
    CCoder coder;
    struct Deflate7zParams params;
};

struct DeflateEncoder* CreateDeflate7zEncoder(struct Deflate7zParams params, unsigned long (*hash)(unsigned long, const unsigned char*, unsigned int)) {
    struct DeflateEncoder* p = NULL;
    switch (params.Format) {
    case DEFLATE_7Z_GZIP: case DEFLATE_7Z_ZLIB:
        if (!hash) return p;  /* fall through */
    case DEFLATE_7Z_RAW:
        break;
    default:
        return p;  /* invalid input */
    }
    /* call it once, if it's not working, crash here */
    unsigned long x;
    if (hash) x = hash(0L, NULL, 0);
    p = (DeflateEncoder*)malloc(sizeof(*p));
    if (p) {
        new (&p->coder) NCompress::NDeflate::NEncoder::CCoder();
        p->params = params;
        CEncProps props;
        // props.Level = params.Level;
        props.algo = params.Algo;
        props.fb = params.FastBytes;
        props.btMode = params.MatchFinder;
        props.mc = params.MatchCycles;
        props.numPasses = params.Passes;
        p->coder.SetProps(&props);
        p->coder.m_Hash = hash;
        if (p->coder.Create() != SZ_OK) {
            free(p);
            p = NULL;
        }
    }
    return p;
}

void DestroyDeflate7zEncoder(struct DeflateEncoder* p) {
    if (p) {
        p->coder.~CCoder();
        free(p);
    }
}

struct MemoryStream : ISeqInStream_, ISeqOutStream_ {
    MemoryStream(void* d, size_t s) : ISeqInStream_((ISeqInStream_){_Read}), ISeqOutStream_((ISeqOutStream_){_Write}), data(d), offset(0), size(s) {}
    void* data;
    size_t offset;
    size_t size;
    SRes Read(void *buf, size_t *bufsize) {
        size_t sz = size - offset;
        sz = sz > *bufsize ? *bufsize : sz;
        memcpy(buf, (char*)data + offset, sz);
        offset += sz;
        *bufsize = sz;
        return SZ_OK;
    }
    size_t Write(const void *buf, size_t bufsize) {
        size_t sz = size - offset;
        if (sz < bufsize) bufsize = sz;
        memcpy((char*)data + offset, buf, bufsize);
        offset += bufsize;
        return bufsize;
    }
    static SRes _Read(ISeqInStreamPtr p, void *buf, size_t *size) {
        return const_cast<MemoryStream*>(static_cast<const MemoryStream*>(p))->Read(buf, size);
    }
    static size_t _Write(ISeqOutStreamPtr p, const void *buf, size_t size) {
        return const_cast<MemoryStream*>(static_cast<const MemoryStream*>(p))->Write(buf, size);
    }
};

struct GZipHeader {
    unsigned char _0;
    unsigned char _1;
    unsigned char _2;
    unsigned char _3;
    unsigned char _4[4];
    unsigned char _8;
    unsigned char _9;
};

struct GZipFooter {
    UInt32 crc;
    UInt32 size;
};

struct GZipHeader GetGZipHeader(struct Deflate7zParams params, struct Deflate7zMeta meta) {
    static const unsigned char kHostOS =
#ifdef _WIN32
        DEFLATE_7Z_GZIP_FS_FAT;
#else
        DEFLATE_7Z_GZIP_FS_UNIX;
#endif
    unsigned char gzheader[10] = {0x1F, 0x8B, 8, 0, 0, 0, 0, 0, 2, kHostOS};
    GZipHeader header;
    memcpy(&header, &gzheader, sizeof(header));
    switch (meta.CompressLevel) {
    case DEFLATE_7Z_GZIP_COMPRESSION_DEFAULT: case DEFLATE_7Z_GZIP_COMPRESSION_BEST: case DEFLATE_7Z_GZIP_COMPRESSION_FASTEST:
        header._8 = meta.CompressLevel;
        break;
    default:
        header._8 = params.Algo ? DEFLATE_7Z_GZIP_COMPRESSION_BEST : DEFLATE_7Z_GZIP_COMPRESSION_FASTEST;
    }
    if (meta.FileSystem >= 0) header._9 = meta.FileSystem;
    memcpy(&header._4, &meta.MTime, 4);
    return header;
}

struct ZLibHeader {
    unsigned char _0;
    unsigned char _1;
};

struct ZLibHeader GetZLibHeader(struct Deflate7zParams params, struct Deflate7zMeta meta) {
    ZLibHeader header;
    unsigned char flg;
    unsigned rm;
    header._0 = 0x78;
    switch (meta.CompressLevel) {
    case 0: case 1: case 2: case 3:
        flg = meta.CompressLevel;
        break;
    default:
        flg = !params.Algo ? DEFLATE_7Z_ZLIB_COMPRESSION_FASTEST  /* 7z level 1-4 */
              : params.FastBytes < 32 ? DEFLATE_7Z_ZLIB_COMPRESSION_FAST /* 7z level 5|6 with smaller window */
              : params.FastBytes == 32 ? DEFLATE_7Z_ZLIB_COMPRESSION_DEFAULT /* 7z level 5|6 */
              : DEFLATE_7Z_ZLIB_COMPRESSION_MAXIMUM; /* anything beyond 7z level 5|6 */
    }
    flg <<= 6;
    rm = ((unsigned)(0x78 * 256) + (unsigned)flg + (unsigned)30) % (unsigned)31;
    flg |= (unsigned char)(30 - rm);
    header._1 = flg;
    return header;
}

struct ZLibFooter {
    UInt32 adler;
};

int InMemoryDeflate7z(struct DeflateEncoder* p, struct Deflate7zMeta meta, const void* indata, size_t insize, void* outdata, size_t* outsize) {
    if (!p || !indata || !outdata) return SZ_ERROR_PARAM;
    MemoryStream inStream((void*)indata, insize);
    MemoryStream outStream(outdata, *outsize);
    UInt64 insize1, outsize1;

    if (p->params.Format == DEFLATE_7Z_GZIP) {
        GZipHeader header = GetGZipHeader(p->params, meta);
        outStream.Write(&header, sizeof(header));
    } else if (p->params.Format == DEFLATE_7Z_ZLIB) {
        ZLibHeader header = GetZLibHeader(p->params, meta);
        outStream.Write(&header, sizeof(header));
    }

    int ret = p->coder.BaseCode(&inStream, &outStream, &insize1, &outsize1);
    if (ret == 0) {
        if (p->params.Format == DEFLATE_7Z_GZIP) {
            GZipFooter footer = {(UInt32)p->coder.m_HashValue, (UInt32)insize};
            outStream.Write(&footer, sizeof(footer));
        } else if (p->params.Format == DEFLATE_7Z_ZLIB) {
            ZLibFooter footer = {Z7_BSWAP32((UInt32)p->coder.m_HashValue)};
            outStream.Write(&footer, sizeof(footer));
        }
        *outsize = outStream.offset;
    }

    return ret;
}