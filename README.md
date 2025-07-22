# 7zDeflate

7zDeflate is a [deflate](https://en.wikipedia.org/wiki/Deflate) lib with better compression ratio (~5% better than zlib/gzip) and good speed.

The original implementation is written by Igor Pavlov (the author of [7-zip](https://7-zip.org)/[LZMA](https://en.wikipedia.org/wiki/LZMA)). While the original implemention is not published anywhere else except for being a component of 7-zip, **7zDeflate** is a port of it aiming at providing a basic SDK of this better deflate compression implementation.

## Notable Deflate Implementations

- [zlib](https://zlib.net) - The De facto standard and commonly used implementation, esp. in \*nix worlds. But it's poorly implemented in both compression speed and ratio (not to mention it's API design, etc.).
  - [zlib-ng](https://github.com/zlib-ng/zlib-ng) - zlib's sanity maintenance + CPU SIMD optimization version. The core algorithm is exactly same as zlib so only speed improvement user-perspective.
- [gzip](https://www.gnu.org/software/gzip/) - It has an individual implementation of deflate. **IT'S NOT SAME AS ZLIB** but has a very close compression ratio and a faster speed.  
  NOTE: if you really care compression speed and think `gzip -9` is fast, you should NOT compare `gzip -9` with `-9` of other compression implementation because `7z -tgzip` (default level 5) already can sometimes beat `gzip -9` in both compression speed and ratio.
- [7-zip](https://7-zip.org) - **As described above, 7-zip's built-in deflate support is totally a different implementation written by the author himself. It offers very good compression ratio with a reasonable cost.**
- [WinRAR](https://www.win-rar.com) - Similarily, the author of WinRAR also shows his strength in compression algorithm and offers another deflate implementaiton with good compression ratio and very good speed. However neither WinRAR nor this implementation is open source.
- [Zopfli](https://github.com/google/zopfli) - The tool google wrote to (re)compress PNGs and gzip/deflate streams to optimize the compressed size to smallest possible to save network bandwidths of static resources (PNGs and pre-gzip'ed other resources). It uses a so-called `iterations` parameter to try many many times, **theoratically it may output the smallest one** (but not always practically) **with very high cost (memory usage and time)**. [I personally only recommended `zonflipng`](https://github.com/yumeyao/pngoptim/wiki).
- [KZIP](https://advsys.net/ken/utils.htm) - [More favorably, PNGOUT is what worths a try](https://github.com/yumeyao/pngoptim/wiki). I list it here for the honor of [PNGOUT](https://en.wikipedia.org/wiki/PNGOUT). It has a unique feature that is randomizing the initial tables so this could sometimes yield very good result but consequently the compression result with this feature on is not reproducible.

### Comparison Chart
The following test is done by July, 2025, on Ubuntu 24.04 WSL @AMD 9950x, all Linux tools are the latest binary version from Ubuntu official repository (including 7-zip Linux port), WinRAR and kzip are the latest Windows versions (and they don't produce .gz, but the file format encapsulation overhead is negligible). All commands are running in single-thread mode (override the default MT behavior via command line switch if required).
| Command       | Compressed Size(Ratio) | Time  | Compressed Size(Ratio) | Time  |
| ------------- |-------------:| -----:|-------------:| -----:|
| Uncompressed Data | `629186560(100.0%)` | [Ubuntu 1604 wsl install.tar](https://aka.ms/wsl-ubuntu-1604) | `917544960(100.0%)` | [gcc-15.1.0.tar](https://ftp.gnu.org/gnu/gcc/gcc-15.1.0/gcc-15.1.0.tar.gz)
| `zlib -9` (perl wrapper stdin/stdout) | `207230122(32.94%)` |   69.06s | `171340208(18.67%)` | 78.62s |
| `gzip -9`| `206517627(32.82%)` |   49.61s | `171315772(18.67%)` | 28.71s |
| `7z -tgzip` (`-mx5`) | `200167688(31.81%)` | 31.72s | `167047766(18.21%)` | 46.09s |
| `7z -tgzip -mx9` | `195937640(31.14%)` | 182.88s | `163415549(17.81%)` | 309.18s |
| `7z -tgzip -mx9 -mfb258` | `195779873(31.12%)` | 383.01s | `162981178(17.76%)` | 679.82s |
| `7z -tgzip -mfb258 -mpass15` | `195729167(31.11%)` | 834.85s | `162948313(17.76%)` | 1650.36s |
| `zopfli --i1` | `198128222(31.49%)` | 585.28s | `163697780(17.84%)` | 564.55s |
| `zopfli` (`--i15`) | `197070543(31.32%)` | 1342.16s | `162743248(17.74%)` | 1869.04s |
| `kzip` | `202911422(32.25%)` | 2521.63s | `164797988(17.96%)` | 1956.54s |
| `WinRAR -afzip` (`-m3`) | `205881308(32.73%)` | 8.28s | `170248830(18.55%)` | 9.57s |
| `WinRAR -afzip -m5` | `205383413(32.64%)` | 10.91s | `169527701(18.48%)` | 11.70s |

### Other Deflate implementations/Tools
- AdvanceCOMP - a combination tool that supports 4 deflate implementations: zlib, libdeflate, 7-zip, zopfli.
  - Its 7-zip deflate implementation is very ancient and is slightly worse than an up-to-date 7-zip (though still much better than zlib).  
  - It can't even let you specify more compression parameters (for example, 7-zip's deflate implementation's supports a max fastbytes of `258`, but it sticks with `-mfb255`)
    - NOTE that a larger but close fastbytes doesn't gurantee better compression ratio, so if you want the max compression you may want to try compress it multiple times using something like `-mfb[250-258]` and select the best one.
- libdeflate - It's still being actively maintained but I don't see a noticeable compression ratio improvement as it advertises.

## Features

- C APIs. (But itself requires C++(without stdlib) to compile)
- Accepts compression parameters as 7-zip does. Parameters are levels/fastbytes/passes.
- Create raw-deflate-stream/gzip/zlib in memory or with C `FILE*`.
  - Requires crc/alder32 implmentation with zlib-compatible API to create gzip/zlib respectively.  
    You may already want to use zlib for decompression(inflate) so just taking crc/alder32 from it keeps the footprint minimal.
- NO PLAN for decompression(inflate). Any existing one is working.

### Spaces for improvements
- Convert to plain C for better portability.
- 7-zip's deflate implementation is not being actively maintained, it lacks a lot of modern optimizations for speed.