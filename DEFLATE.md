# DEFLATE Internals and Their Impact on Compression Ratio

## TL;DR
- **DEFLATE =** *Find* (LZ77 candidates) → *Choose* (optimal parse) → *Code* (block & Huffman).
- Window 32 KiB; match length 3-258; distance 1-32768.
- Algorithms and parameters chosen for each stage strongly affect compression ratio and performance.

---

## How DEFLATE works

1) **Match finder (LZ77 side)**  
   Finds candidate back-references `(length, distance)` in the last **32 KiB**.  
   Stronger finders surface **longer** and **better-positioned** matches.

2) **Parse strategy**  
   Chooses a path through literals vs matches across the stream.
    - **Greedy/lazy (one-step, local):** only look ahead if the immediate next match is longer.
    - **DP/global (shortest-path):** choose the overall cheapest bit sequence.

3) **Block segmentation & Huffman coding**  
   Splits the stream into blocks and builds (fixed/dynamic) Huffman trees.  
   Better splitting and re-pricing can reduce bits; too many blocks wastes headers.

---

## Implementations comparison

|  | **Match finder** | **Parse strategy** | **Code (blocks & Huffman)** |
|---|---|---|---|
| **gzip / zlib** | **HC** (hash chain, bounded search) | **Lazy** lookahead | Heuristics; dynamic/fixed as needed |
| **7-Zip Deflate** | **BT** (binary tree, same family as in LZMA); **HC** at lower levels | **Optimal DP** (multi-pass pricing) | Dynamic trees; **extra pricing passes** refine symbol costs |
| **Zopfli** | **HC** (with optional match caching) | **Global optimal parsing** with **code-stage feedback** to re-price and re-parse over **iterations** | **Aggressive block-split search** that **rewinds and re-splits prior blocks boundaries** to minimize size |

- **7-Zip Deflate**  
  - **BT** finder at higher levels homes in on **longer/nearer** matches than HC for a given time budget.  
  - **Optimal DP** chooses a **globally cheaper** sequence than greedy/lazy.  
  - **Multi-pass pricing** refines symbol costs and revisits the DP to trim more bits.
- **Zopfli**  
  - Keeps **HC** but uses **global optimal parsing** with **code-stage feedback**, repeatedly repricing under the current Huffman model and reparsing across **iterations**.
  - Performs **explicit block-split search** with **rewind**, rebuilding trees and re-splitting earlier boundaries to minimize total size.  
   This feedback-and-rewind loop is extremely time-consuming.

With these more powerful algorithms, 7-Zip Deflate and Zopfli often beat the baseline implementation (zlib/gzip).  
[**ECT**](https://github.com/fhanau/Efficient-Compression-Tool) uses **Zopfli** but **swaps in 7-Zip's BT match finder** (replacing Zopfli's baseline HC finder) and often achieves a even higher compression ratio.

---

## 7-Zip Deflate parameters (the knobs that matter)

### Levels & finder mode
- **Levels 1-4:** **HC** (hash chain) finder; faster, lower ratio.  
- **Levels 5-9:** **BT** (binary tree) finder; slower, higher ratio.

### FastBytes (`fb`)
- Target **nice** match length. Larger values *can* help - **if** the finder has enough search budget to reach them. Extreme `fb` isn't always best on small or medium-sized corpora.

### MatchCycles (`mc`, API-level)
- Conceptually: the **per-position search budget** for the match finder - i.e., **how deep to search** (for **BT**, deeper in the tree).  
- **Default when unspecified (typical CLI behavior):** `mc = 16 + (fb / 2)`  
This coupling explains why **raising `fb`** often looks effective:
  - you implicitly grant **more search budget**, which can improve compression ratio.
  - by contrast, holding `mc` fixed while raising `fb` usually only yields negligible improvement.

### Passes (`numPasses` / CLI `-mpass=N`)
- Extra **pricing passes** re-estimate symbol costs and rerun the optimal parse. Gains are modest but real; cost is additional CPU. (Same spirit as Zopfli's iterations, but narrower in scope.)

### Practical tuning notes
- **Biggest lever:** **BT Finder + more MatchCycles**. This typically moves ratio more than cranking `fb` alone.  
- **Don't assume `fb = 258` always wins.** For tens-of-MB or smaller corpora, a sweet spot often lives around **128-240**, *with* adequate match budget.  
- **Passes:** Add a few if you can afford them; but the default is usually enough - more passes often make negligible or no difference while costing extra time.
