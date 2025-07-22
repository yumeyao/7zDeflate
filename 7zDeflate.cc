#include <string.h>
#include <memory>
#include "7zDeflate.h"
#include "7z/DeflateEncoder.h"

using namespace NCompress::NDeflate::NEncoder;

struct DeflateEncoder {
    CCoder coder;
};

struct DeflateEncoder* CreateDeflate7zEncoder(int level, int fb, int pass) {
    DeflateEncoder* p = (DeflateEncoder*)malloc(sizeof(*p));
    if (p) {
        new (&p->coder) NCompress::NDeflate::NEncoder::CCoder();
        CEncProps props;
        props.Level = level;
        props.fb = fb;
        props.numPasses = pass;
        p->coder.SetProps(&props);
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
    MemoryStream(void* d, size_t s) : ISeqInStream_({_Read}), ISeqOutStream_({_Write}), data(d), offset(0), size(s) {}
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

int InMemoryDeflate7z(struct DeflateEncoder* p, const void* indata, size_t insize, void* outdata, size_t* outsize) {
    if (!p || !indata || !outdata) return SZ_ERROR_PARAM;
    MemoryStream inStream((void*)indata, insize);
    MemoryStream outStream(outdata, *outsize);
    UInt64 insize1, outsize1;
    int ret = p->coder.BaseCode(&inStream, &outStream, &insize1, &outsize1);
    if (ret == 0) *outsize = outStream.offset;
    return ret;
}