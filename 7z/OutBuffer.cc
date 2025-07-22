// OutBuffer.cc

#include <stdlib.h>
#include "OutBuffer.h"

bool COutBuffer::Create(UInt32 bufSize) throw()
{
  const UInt32 kMinBlockSize = 1;
  if (bufSize < kMinBlockSize)
    bufSize = kMinBlockSize;
  if (_buf && _bufSize == bufSize)
    return true;
  Free();
  _bufSize = bufSize;
  _buf = (Byte *)::malloc(bufSize);
  return (_buf != NULL);
}

void COutBuffer::Free() throw()
{
  ::free(_buf);
  _buf = NULL;
}

void COutBuffer::Init() throw()
{
  _streamPos = 0;
  _limitPos = _bufSize;
  _pos = 0;
  _processedSize = 0;
  _overDict = false;
  ErrorCode = SZ_OK;
}

UInt64 COutBuffer::GetProcessedSize() const throw()
{
  UInt64 res = _processedSize + _pos - _streamPos;
  if (_streamPos > _pos)
    res += _bufSize;
  return res;
}


SRes COutBuffer::FlushPart() throw()
{
  // _streamPos < _bufSize
  UInt32 size = (_streamPos >= _pos) ? (_bufSize - _streamPos) : (_pos - _streamPos);
  SRes result = SZ_OK;
  result = ErrorCode;
  if (_buf2)
  {
    memcpy(_buf2, _buf + _streamPos, size);
    _buf2 += size;
  }

  if (_stream && (ErrorCode == SZ_OK))
  {
    UInt32 processedSize = ISeqOutStream_Write(_stream, _buf + _streamPos, size);
    result = processedSize == size ? SZ_OK : SZ_ERROR_WRITE;
    size = processedSize;
  }
  _streamPos += size;
  if (_streamPos == _bufSize)
    _streamPos = 0;
  if (_pos == _bufSize)
  {
    _overDict = true;
    _pos = 0;
  }
  _limitPos = (_streamPos > _pos) ? _streamPos : _bufSize;
  _processedSize += size;
  return result;
}

SRes COutBuffer::Flush() throw()
{
  if (ErrorCode != SZ_OK)
    return ErrorCode;

  while (_streamPos != _pos)
  {
    const SRes result = FlushPart();
    if (result != SZ_OK)
      return result;
  }
  return SZ_OK;
}

void COutBuffer::FlushWithCheck()
{
  const SRes result = Flush();
  ErrorCode = result;
}
