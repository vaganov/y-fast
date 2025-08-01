# yfast

**yfast** is a header-only C++20 template library that provides `yfast::fastmap` &mdash; a sorted associative container
based upon [Y-fast trie](https://en.wikipedia.org/wiki/Y-fast_trie)

Asymptotically all the basic operations (find exact match, find predecessor/successor, insert/delete) in the y-fast trie
data structure take amortized `O(ln H)` time (assuming `H` is the bit length of key). The main goal of this library is
to provide a faster container than classic sorted associative containers (represented in benchmark tests by `std::map`)
in practice. This, however, may only be achieved under certain conditions (see [Performance](#performance)).

## Current state
The library is in pre-release mode; basic operations are supported:
- insert an entry
- find an entry by key if present
- remove an entry
- operator []
- iteration

Yet to come:
- custom allocator
- proper documentation
- test coverage

## Usage
### Template parameters
- `Key` &mdash; key type for the map to be indexed with; must be:
  - _copyable_ and
  - _bit-representable_ (see `BitExtractor`) and
  - _comparable_ (see `Compare`)
- `Value` &mdash; value type to be stored in the map; must be either
  - _copyable_ or
  - _movable_ or
  - `void` (in which case no value is stored in the map and iterator dereference policy alters)
- `H` &mdash; key length; doesn't have to be a power of two or match `sizeof(Key)`; cannot be less than `8` though
- `BitExtractor` &mdash; helper type to provide key shifts and bit extractions; must be compliant with
[yfast::internal::BitExtractorGeneric](include/yfast/internal/concepts.h) concept; whatever type is returned by
`shift()` must be hashable (see `Hash`); `yfast::fastmap` comes with a default implementation
`yfast::impl::BitExtractor` for these types:
  - all integral types
  - `std::vector<std::byte>`
  - `std::string` (which is basically treated as `std::vector<std::byte>`)
- `Hash` &mdash; map from shifted keys to `std::uintptr_t`; must be compliant with
[yfast::internal::MapGeneric](include/yfast/internal/concepts.h) concept and _default-constructible_;
[tsl::hopscotch_map](https://github.com/Tessil/hopscotch-map) is set as a default (unless `WITHOUT_HOPSCOTCH_MAP` macro
is defined, in which case `std::unordered_map` is used)
- `Compare` &mdash; key comparator; must be _copyable_; the order provided by `Compare` must match the lexicographic
order provided by `BitExtractor`; `std::less` is set as a default

### Usage example

    #include <cassert>
    #include <cstdint>
    #include <iostream>
    #include <iterator>
    #include <string>
    
    #include <unistd.h>
    
    #include <yfast/fastmap.h>
    #include <yfast/iterator.h>
    
    int main() {
    yfast::fastmap<std::uint32_t, std::string, 32> fastmap;
    
        fastmap[3] = "three";
        fastmap[2] = "two";
        fastmap[1] = "one";
    
        std::cout << "values: ";
        for (const auto& v: fastmap) {
            std::cout << v << ' ';
        }
        std::cout << std::endl;
    
        assert(fastmap.size() == std::distance(fastmap.begin(), fastmap.end()));
    
        auto i = fastmap.find(2);
        auto r = yfast::make_reverse_iterator(i);
    
        std::cout << "erasing onward" << std::endl;
        while (i != fastmap.end()) {
            std::cout << i.key() << ' ' << i.value() << std::endl;
            i = fastmap.erase(i);
        }
    
        std::cout << "erasing backward" << std::endl;
        while (r != fastmap.rend()) {
            std::cout << r.key() << ' ' << r.value() << std::endl;
            r = fastmap.erase(r);
        }
    
        assert(fastmap.empty());
    
        return EXIT_SUCCESS;
    }

## Performance
Every y-fast trie lookup operation (find match/predecessor/successor) performs `ln H` key shifts and `ln H` hash
lookups. Besides, insert/delete operations may perform up to `H` key bit extractions and up to `H` hash
insertions/deletions respectively. This said, to outperform `std::map`, these factors shall be taken into account:

### large size
According to tests (see below), the break-even point occurs **between one million and one hundred million** entries
depending on the operation. As a rule of thumb, if your map operates less than one million records, you are unlikely to
profit from using `yfast::fastmap`

### CPU architecture
Tests have been run on _x86-64_ and _ARM64_ CPUs. Whilst lookups are faster in `yfast::fastmap` on both architectures
for large enough containers (with break-even point below ten million entries), inserts are only faster on _ARM64_ (with
break-event point over ten million entries) but never on _x86-64_ (presumably due to different cache miss handling). On
the contrary, deletions do not differ significantly.

### effective bit operations
Only integral key types with single-cycle shifts and bit extractions proved to be effective enough. In other words,
indexing a map with, say, `std::uint64_t` would make it faster than `std::map` but indexing with `std::string` probably
would not. However, everything is possible if a fine-tuned `BitExtractor` is provided.

### custom hash map
These hash maps have been tested as underlying hash table:
- [std::unordered_map](https://en.cppreference.com/w/cpp/container/unordered_map.html)
- [tsl::hopscotch_map](https://github.com/Tessil/hopscotch-map)
- [absl::flat_hash_map](https://github.com/abseil/abseil-cpp) **NB:** Apache-2.0 license
- [ankerl::unordered_dense::map](https://github.com/martinus/unordered_dense)

Benchmark tests have been conducted in the following manner:

for a sample size `M`:
- populate container with `M` random keys
- generate random sample of another `M` keys
- **insert** sample keys one by one in a loop (thus doubling the size)
- measure clock time of the previous action
- **find** sample keys one by one in a loop
- measure clock time of the previous action
- **find and delete** sample keys one by one in a loop (thus halving the size)
- measure clock time of the previous action
- divide the clock times by sample size

Test source file may be found in [test/benchmark.cpp](test/benchmark.cpp)

Tests have been run on AWS _r6a.8xlarge_ and _m6g.16xlarge_ instances. Sample size on the x-axis, time in nanoseconds on
the y-axis

#### ARM64, find
<picture>
<source media="(prefers-color-scheme: dark)" srcset="plots/benchmark-uint32-arm64-find-dark.png">
<img alt="uint32-arm64-find" src="plots/benchmark-uint32-arm64-find.png">
</picture>

#### ARM64, insert
<picture>
<source media="(prefers-color-scheme: dark)" srcset="plots/benchmark-uint32-arm64-insert-dark.png">
<img alt="uint32-arm64-insert" src="plots/benchmark-uint32-arm64-insert.png">
</picture>

#### ARM64, find+erase
<picture>
<source media="(prefers-color-scheme: dark)" srcset="plots/benchmark-uint32-arm64-erase-dark.png">
<img alt="uint32-arm64-erase" src="plots/benchmark-uint32-arm64-erase.png">
</picture>

#### x86-64, find
<picture>
<source media="(prefers-color-scheme: dark)" srcset="plots/benchmark-uint32-x64-find-dark.png">
<img alt="uint32-x64-find" src="plots/benchmark-uint32-x64-find.png">
</picture>

#### x86-64, insert
<picture>
<source media="(prefers-color-scheme: dark)" srcset="plots/benchmark-uint32-x64-insert-dark.png">
<img alt="uint32-x64-insert" src="plots/benchmark-uint32-x64-insert.png">
</picture>

#### x86-64, find+erase
<picture>
<source media="(prefers-color-scheme: dark)" srcset="plots/benchmark-uint32-x64-erase-dark.png">
<img alt="uint32-x64-erase" src="plots/benchmark-uint32-x64-erase.png">
</picture>
