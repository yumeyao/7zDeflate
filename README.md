# 7zDeflate

7zDeflate is a [deflate](https://en.wikipedia.org/wiki/Deflate) lib with better compression ratio (~5% over zlib/gzip) and good speed.

The original implementation was written by Igor Pavlov (the author of [7-zip](https://7-zip.org)/[LZMA](https://en.wikipedia.org/wiki/LZMA)). While the original implementation is only published as part of 7-zip, **7zDeflate** is a port of it aiming at providing a basic SDK of this better deflate compression implementation.

## Notable Deflate Implementations

- [zlib](https://zlib.net) - The de facto standard and commonly used implementation, esp. in \*nix worlds. But it's poorly implemented in both compression speed and ratio (not to mention it's API design, etc.).
  - [zlib-ng](https://github.com/zlib-ng/zlib-ng) - zlib's sanity maintenance + CPU SIMD optimization version. The core algorithm is exactly same as zlib so only speed improvement user-perspective.
- [gzip](https://www.gnu.org/software/gzip/) - It has an individual implementation of deflate. **IT'S NOT SAME AS ZLIB** but has a very close compression ratio and a faster speed.  
  **NOTE:** For fair speed comparisons, don't equate `gzip -9` with level 9 elsewhere. For example, `7z -tgzip` at its default level `-5` can sometimes beat `gzip -9` in both speed and ratio.
- [7-zip](https://7-zip.org) - **As described above, 7-zip's built-in deflate support is totally a different implementation written by the author himself. It offers very good compression ratio with a reasonable cost.**
- [WinRAR](https://www.win-rar.com) - Similarily, the author of WinRAR also shows his strength in compression algorithm and offers another deflate implementaiton with good ratio and very good speed. However neither WinRAR nor this implementation is open source.
- [Zopfli](https://github.com/google/zopfli) - Google's deflate optimizer for PNGs and gzip/zlib streams for saving network bandwidths of static resources. It performs optimal parsing over `iterations` globally (reblocking with feedbacks) for minimal sizes **at very high cost (memory usage and time)**.
  - [ECT](https://github.com/fhanau/Efficient-Compression-Tool) - ECT is a multi-format compression optimizer. Its deflate compressor is effectively **zopfli** with **7-zip's BT match finder** swapped in. **It often beats zopfli and 7-zip as it combines their strength into one**. See [internals](DEFLATE.md#implementations-comparison).
- [KZIP](https://advsys.net/ken/utils.htm) - listed here in honor of **PNGOUT** ([worth trying for PNGs](https://github.com/yumeyao/pngoptim/wiki)). PNGOUT can **randomize initial Huffman tables**, sometimes beating local minima (especially on logo/UI PNGs with few colors and repeated patterns), but consequently the output is not reproducible.

### Comparison Chart
The following tests are done by July, 2025, on Ubuntu 24.04 WSL @AMD 9950x, all Linux tools are the latest binary version from the Ubuntu repository (including the 7-zip port), WinRAR and KZIP are the latest Windows versions (they don't produce .gz, but container overhead is negligible).All commands ran **single-threaded** (overriding defaults where needed).

`7zgzip` is the streaming demo in `examples/` built on **7zDeflate**. It produces **bit-identical** output to official `7z` on the listed test cases. It's compiled with `g++ 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04)` `-O2` so its performance (speed) *should* align with the official `7z`. **To keep the table short, we show official times followed by the demo time in parentheses.**
| Command       | Compressed Size(Ratio) | Time  | Compressed Size(Ratio) | Time  |
| ------------- |-------------:| -----:|-------------:| -----:|
| Uncompressed Data | `629186560(100.0%)` | [Ubuntu 1604 wsl install.tar](https://aka.ms/wsl-ubuntu-1604) | `917544960(100.0%)` | [gcc-15.1.0.tar](https://ftp.gnu.org/gnu/gcc/gcc-15.1.0/gcc-15.1.0.tar.gz)
| `zlib -9` (perl wrapper stdin/stdout) | `207230122(32.94%)` |   69.06s | `171340208(18.67%)` | 78.62s |
| `gzip -9`| `206517627(32.82%)` |   49.61s | `171315772(18.67%)` | 28.71s |
| `7z -tgzip` (`-mx5`) | `200167688(31.81%)` | 31.72s (37.19s) | `167047766(18.21%)` | 46.09s (47.65s) |
| `7z -tgzip -mx9` | `195937640(31.14%)` | 182.88s (210.66s) | `163415549(17.81%)` | 309.18s (290.96s) |
| `7z -tgzip -mx9 -mfb258` | `195779873(31.12%)` | 383.01s (301.85s) | `162981178(17.76%)` | 679.82s (542.23s) |
| `7z -tgzip -mfb258 -mpass15` | `195729167(31.11%)` | 834.85s (781.03s) | `162948313(17.76%)` | 1650.36s (1574.61s) |
| `zopfli --i1` | `198128222(31.49%)` | 585.28s | `163697780(17.84%)` | 564.55s |
| `zopfli` (`--i15`) | `197070543(31.32%)` | 1342.16s | `162743248(17.74%)` | 1869.04s |
| `ect -gzip -9` | `193833758(30.81%)` | 1304.69s | `162143097(17.67%)` | 1610.46s |
| `kzip` | `202911422(32.25%)` | 2521.63s | `164797988(17.96%)` | 1956.54s |
| `WinRAR -afzip` (`-m3`) | `205881308(32.73%)` | 8.28s | `170248830(18.55%)` | 9.57s |
| `WinRAR -afzip -m5` | `205383413(32.64%)` | 10.91s | `169527701(18.48%)` | 11.70s |

### Other Deflate implementations/Tools
- AdvanceCOMP - a combination tool that supports 4 deflate implementations: zlib, libdeflate, 7-zip, zopfli.
  - Its 7-zip deflate implementation is very ancient and is slightly worse than an up-to-date 7-zip (though still much better than zlib).  
  - It can't even let you specify more compression parameters (for example, 7-zip's deflate implementation's supports a max fastbytes of `258`, but it sticks with `-mfb255`)
    - **Note:** a larger `FastBytes` does not guarantee a better ratio. If chasing the last tenths, try a band (`-mfb[128-258]`) and keep the best. See [practical notes](DEFLATE.md#practical-tuning-notes).
- libdeflate - It's still being actively maintained but I don't see a noticeable compression ratio improvement as it advertises.

## Features

- C APIs. (But itself requires C++(without stdlib) to compile)
  - In-memory and `FILE*` APIs.
  - No coroutine-style streaming API (feed by chunks) yet.
- Lightweight and Fast
  - Demo executable in `examples/` is only ~60KB.
  - Even faster than official `7z` sometimes (esp. at higher compression settings).
- **Tunable like 7-zip** - See [parameters](DEFLATE.md#7-zip-deflate-parameters-the-knobs-that-matter).
- Create raw-deflate-stream/gzip/zlib in memory or with C `FILE*`.
  - Requires crc/alder32 implmentation with zlib-compatible API to create gzip/zlib respectively.  
    You may already want to use zlib for decompression(inflate) so just taking crc/alder32 from it keeps the footprint minimal.
- NO PLAN for decompression(inflate). Any existing one is working.

### Spaces for improvements
- Convert to plain C for better portability.
- 7-zip's deflate implementation is not being actively maintained, it lacks a lot of modern optimizations for speed.  
  See even this simple demo is faster than 7z by just ripping out some C++ encapsulation or maybe better inline despite it's using a slower crc32 implementation and calling routines(not only calling a function pointer, but also each time with a very small chunk).
