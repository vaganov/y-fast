# yfast

**yfast** is a header-only C++20 template library that provides `yfast::fastmap` &mdash; a sorted associative container
based upon [Y-fast trie](https://en.wikipedia.org/wiki/Y-fast_trie)

Asymptotically all the basic operations (find exact match, find predecessor/successor, insert/delete) in the y-fast trie
data structure take amortized `O(ln H)` time (assuming `H` is the bit length of key). The main goal of this library is
to provide an essentially faster container than classic sorted associative containers (represented in benchmark tests by
`std::map`) in practice. This, however, may only be achieved under certain conditions. Namely, to profit from using
`yfast::fastmap`, one:
- **must** operate a large size container (over one million entries for faster lookups and over ten million entries for
faster inserts)
- **must** _either_ index the container with an integral type _or_ provide an effective bit extraction mechanism
- **should** run on _ARM64_ architecture (especially if faster inserts are desired)
- **should** use custom hash table implementation

See [Performance](#performance) for details and benchmark test results

## Usage
### Basic usage example

    #include <cassert>
    #include <cstdint>
    #include <iostream>
    #include <iterator>
    #include <string>
    
    #include <unistd.h>
    
    #include <yfast/fastmap.h>
    #include <yfast/iterator.h>
    
    int main() {
        yfast::fastmap<std::uint32_t, std::string, 32> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };

        fastmap[0] = "zero";
    
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
`yfast::internal::BitExtractor` for these types:
  - all integral types
  - `std::vector<std::byte>`
  - `std::string` (which is basically treated as `std::vector<std::byte>`)
- `Hash` &mdash; map from shifted keys to `std::uintptr_t`; must be compliant with
[yfast::internal::MapGeneric](include/yfast/internal/concepts.h) concept and _default-constructible_;
[tsl::hopscotch_map](https://github.com/Tessil/hopscotch-map) _with default allocator_ is used as default (unless
`YFAST_WITHOUT_HOPSCOTCH_MAP` macro is defined, in which case `std::unordered_map` _with default allocator_ is used)
- `Compare` &mdash; key comparator; must be _copyable_; the order provided by `Compare` must match the lexicographic
order provided by `BitExtractor`; `std::less` is used as default
- `ArbitraryAllocator` &mdash; allocator; this allocator will not be used directly but rather rebound via
[std::allocator_traits::rebind_alloc](https://en.cppreference.com/w/cpp/memory/allocator_traits.html) to allocate
internal structures; `Hash`, however, only uses _any_ allocator if explicitly specified; `std::allocator<Key>` is used
as default

### Iterators
`yfast::fastmap` is equipped with mutable and const bidirectional iterators, both forward and reverse. Apart from
`rbegin`/`rend`/`crbegin`/`crend`, `yfast::fastmap` methods return forward iterators (const for const methods and
mutable otherwise). A reverse iterator may be obtained from a forward iterator by calling `make_reverse()` method or
`yfast::make_reverse_iterator()` template function.

#### Iterator dereference
Please note that `iterator::value_type` is `Value`, not `std::pair<Key, Value>`. Entry value is available via `*i` and
`i->`, as well as `i.value()`; (immutable) entry key is available via `i.key()`

`Value = void` is considered a special case, effectively turning `yfast::fastmap` into a "fastset." In this case
`iterator::value_type` becomes `const Key`

- dereferencing a valid iterator which is not `end()`: returns `iterator::value_type` by (mutable or const) reference
- dereferencing `end()`: throws `std::out_of_range`
- dereferencing an invalidated iterator: **undefined behavior**

#### Iterator increment/decrement safety
- incrementing/decrementing an iterator which is neither `begin()` nor `end()`: always safe
- incrementing `begin()`: safe
- decrementing `begin()`: **undefined behavior**
- incrementing `end()`: safe, no-op
- decrementing `end()`: safe (unless empty), points to the last entry (with respect to the iterator direction)

#### Iterator reversion
- reverting an iterator `i` which is neither `begin()` nor `end()` gives a reverse iterator pointing to the same entry
as `--i`
- reverting `begin()` gives `rend()`
- reverting `end()` gives `rbegin()`

#### Iterator invalidation
Iterator **only** gets invalidated if:
- pointed entry is replaced via calling `insert()` with the equal key
- pointed entry is erased via `erase()`
- _in particular,_ all entries are erased by `clear()`
- _in particular,_ the container is destroyed
- the container is moved (in which case iterator still may be dereferenced but not incremented/decremented)

Iterator **does not** get invalidated and may be safely dereferenced and incremented/decremented if:
- other iterators pointing at the same entry are created
- the value of the pointed entry is modified
- new entries are inserted
- other entries are erased

### Auto-generated docs
See [yfast::fastmap](https://vaganov.github.io/y-fast/html/classyfast_1_1fastmap.html) class summary

## Performance
Every y-fast trie lookup operation (find match/predecessor/successor) performs `O(ln H)` key shifts and `O(ln H)` hash
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

Based on benchmark tests, [tsl::hopscotch_map](https://github.com/Tessil/hopscotch-map) has been picked as default

## Memory consumption
While maintaining linear memory use, `yfast::fastmap` consumes around 30% more RAM than `std::map` due to use of `H`
hash tables.

## Underlying data structures
While `yfast::fastmap` is merely a wrapper (mostly iterator paperwork), these classes implement underlying data
structures:
- `yfast::impl::BST` &mdash; [Binary search tree](https://en.wikipedia.org/wiki/Binary_search_tree)
- `yfast::impl::AVL` &mdash; [AVL tree](https://en.wikipedia.org/wiki/AVL_tree) subclassing `yfast::impl::BST`
- `yfast::impl::XFastTrie` &mdash; [X-fast trie](https://en.wikipedia.org/wiki/X-fast_trie)
- `yfast::impl::YFastTrie` &mdash; [Y-fast trie](https://en.wikipedia.org/wiki/Y-fast_trie) on top of
`yfast::impl::XFastTrie` and `yfast::impl::AVL`

Why bother implementing self-balancing binary trees: neither
[std::map](https://en.cppreference.com/w/cpp/container/map.html) nor
[boost::intrusive::avltree](https://www.boost.org/doc/libs/latest/doc/html/intrusive/avl_set_multiset.html) nor other
self-balancing tree implementations provide a way to split a tree into two subtrees of comparable size in linear (in
size) time or better, which is crucial for inserting an entry into a y-fast trie. `yfast::impl::AVL` comes with
`split()` method to split a tree by root in logarithmic (in size) time.

Although not optimized design-wise, `yfast::impl` classes have solid interfaces and may be used on their own. Here are
auto-generated docs:
- [yfast::impl::BST](https://vaganov.github.io/y-fast/html/classyfast_1_1impl_1_1_b_s_t.html)
- [yfast::impl::AVL](https://vaganov.github.io/y-fast/html/classyfast_1_1impl_1_1_a_v_l.html)
- [yfast::impl::XFastTrie](https://vaganov.github.io/y-fast/html/classyfast_1_1impl_1_1_x_fast_trie.html)
- [yfast::impl::YFastTrie](https://vaganov.github.io/y-fast/html/classyfast_1_1impl_1_1_y_fast_trie.html)
