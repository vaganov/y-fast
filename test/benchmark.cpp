#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#define WITH_HOPSCOTCH 1
#define WITH_FLAT_HASH 1
#define WITH_DENSE_MAP 1

#ifdef WITH_FLAT_HASH
#include <absl/container/flat_hash_map.h>
#endif
#ifdef WITH_DENSE_MAP
#include <ankerl/unordered_dense.h>
#endif
#ifdef WITH_HOPSCOTCH
#include <tsl/hopscotch_map.h>
#endif

#include <yfast/fastmap.h>

int main() {
    auto constexpr N0 = 10;
    auto constexpr N1 = 26;
    auto constexpr M0 = 1 << N0;
    auto constexpr M1 = 1 << N1;

    std::vector<std::uint32_t> shuffle(M1);
    for (auto i = 0; i < M1; ++i) {
        shuffle[i] = i;
    }

    ::srand(time(nullptr));
    auto unshuffled = M1;
    while (unshuffled > 1) {
        auto index = ::rand() % unshuffled;
        std::swap(shuffle[index], shuffle[--unshuffled]);
    }

    std::chrono::time_point<std::chrono::high_resolution_clock> start, stop;
    std::chrono::high_resolution_clock::duration duration;

    std::ofstream stats("benchmark.csv");
    stats << "implementation,operation,sample,duration" << std::endl;

    std::map<std::uint32_t, std::uint32_t> map;

    for (auto i = 0; i < M0; ++i) {
        auto key = shuffle[i];
        map.insert(std::make_pair(key, key));
    }

    for (auto M = M0; M < M1; M <<= 1) {
        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            map.insert(std::make_pair(key, key));
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " std::map insert: " << duration.count() << std::endl;
        stats << "std::map,insert," << M << "," << duration.count() << std::endl;

        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            auto pos = map.find(key);
            if (pos == map.end()) {
                std::cerr << "std::map key=" << key << " not found" << std::endl;
            }
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " std::map find: " << duration.count() << std::endl;
        stats << "std::map,find," << M << "," << duration.count() << std::endl;
    }

    for (auto M = M1 >> 1; M >= M0; M >>= 1) {
        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            auto pos = map.find(key);
            map.erase(pos);
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " std::map find+erase: " << duration.count() << std::endl;
        stats << "std::map,find+erase," << M << "," << duration.count() << std::endl;
    }

    map.clear();

    yfast::fastmap<std::uint32_t, void, N1, yfast::impl::BitExtractor<std::uint32_t>, std::unordered_map<std::uint32_t, std::uintptr_t>> fastmap;

    for (auto i = 0; i < M0; ++i) {
        auto key = shuffle[i];
        fastmap.insert(key);
    }

    for (auto M = M0; M < M1; M <<= 1) {
        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            fastmap.insert(key);
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " yfast::fastmap+std::unordered_map insert: " << duration.count() << std::endl;
        stats << "yfast::fastmap+std::unordered_map,insert," << M << "," << duration.count() << std::endl;

        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            auto pos = fastmap.find(key);
            if (pos == fastmap.end()) {
                std::cerr << "yfast::fastmap+std::unordered_map key=" << key << " not found" << std::endl;
            }
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " yfast::fastmap+std::unordered_map find: " << duration.count() << std::endl;
        stats << "yfast::fastmap+std::unordered_map,find," << M << "," << duration.count() << std::endl;
    }

    for (auto M = M1 >> 1; M >= M0; M >>= 1) {
        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            auto pos = fastmap.find(key);
            fastmap.erase(pos);
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " yfast::fastmap+std::unordered_map find+erase: " << duration.count() << std::endl;
        stats << "yfast::fastmap+std::unordered_map,find+erase," << M << "," << duration.count() << std::endl;
    }

    fastmap.clear();

#if WITH_HOPSCOTCH
    yfast::fastmap<std::uint32_t, void, N1, yfast::impl::BitExtractor<std::uint32_t>, tsl::hopscotch_map<std::uint32_t, std::uintptr_t>> fastmap_hopscotch;

    for (auto i = 0; i < M0; ++i) {
        auto key = shuffle[i];
        fastmap_hopscotch.insert(key);
    }

    for (auto M = M0; M < M1; M <<= 1) {
        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            fastmap_hopscotch.insert(key);
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " yfast::fastmap+tsl::hopscotch insert: " << duration.count() << std::endl;
        stats << "yfast::fastmap+tsl::hopscotch,insert," << M << "," << duration.count() << std::endl;

        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            auto pos = fastmap_hopscotch.find(key);
            if (pos == fastmap_hopscotch.end()) {
                std::cerr << "yfast::fastmap+tsl::hopscotch key=" << key << " not found" << std::endl;
            }
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " yfast::fastmap+tsl::hopscotch find: " << duration.count() << std::endl;
        stats << "yfast::fastmap+tsl::hopscotch,find," << M << "," << duration.count() << std::endl;
    }

    for (auto M = M1 >> 1; M >= M0; M >>= 1) {
        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            auto pos = fastmap_hopscotch.find(key);
            fastmap_hopscotch.erase(pos);
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " yfast::fastmap+tsl::hopscotch find+erase: " << duration.count() << std::endl;
        stats << "yfast::fastmap+tsl::hopscotch,find+erase," << M << "," << duration.count() << std::endl;
    }

    fastmap_hopscotch.clear();
#endif

#if WITH_FLAT_HASH
    yfast::fastmap<std::uint32_t, void, N1, yfast::impl::BitExtractor<std::uint32_t>, absl::flat_hash_map<std::uint32_t, std::uintptr_t>> fastmap_flat_hash;

    for (auto i = 0; i < M0; ++i) {
        auto key = shuffle[i];
        fastmap_flat_hash.insert(key);
    }

    for (auto M = M0; M < M1; M <<= 1) {
        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            fastmap_flat_hash.insert(key);
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " yfast::fastmap+absl::flat_hash_map insert: " << duration.count() << std::endl;
        stats << "yfast::fastmap+absl::flat_hash_map,insert," << M << "," << duration.count() << std::endl;

        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            auto pos = fastmap_flat_hash.find(key);
            if (pos == fastmap_flat_hash.end()) {
                std::cerr << "yfast::fastmap+absl::flat_hash_map key=" << key << " not found" << std::endl;
            }
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " yfast::fastmap+absl::flat_hash_map find: " << duration.count() << std::endl;
        stats << "yfast::fastmap+absl::flat_hash_map,find," << M << "," << duration.count() << std::endl;
    }

    for (auto M = M1 >> 1; M >= M0; M >>= 1) {
        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            auto pos = fastmap_flat_hash.find(key);
            fastmap_flat_hash.erase(pos);
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " yfast::fastmap+absl::flat_hash_map find+erase: " << duration.count() << std::endl;
        stats << "yfast::fastmap+absl::flat_hash_map,find+erase," << M << "," << duration.count() << std::endl;
    }

    fastmap_flat_hash.clear();
#endif

#if WITH_DENSE_MAP
    yfast::fastmap<std::uint32_t, void, N1, yfast::impl::BitExtractor<std::uint32_t>, ankerl::unordered_dense::map<std::uint32_t, std::uintptr_t>> fastmap_dense_map;

    for (auto i = 0; i < M0; ++i) {
        auto key = shuffle[i];
        fastmap_dense_map.insert(key);
    }

    for (auto M = M0; M < M1; M <<= 1) {
        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            fastmap_dense_map.insert(key);
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " yfast::fastmap+ankerl::unordered_dense::map insert: " << duration.count() << std::endl;
        stats << "yfast::fastmap+ankerl::unordered_dense::map,insert," << M << "," << duration.count() << std::endl;

        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            auto pos = fastmap_dense_map.find(key);
            if (pos == fastmap_dense_map.end()) {
                std::cerr << "yfast::fastmap+ankerl::unordered_dense::map key=" << key << " not found" << std::endl;
            }
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " yfast::fastmap+ankerl::unordered_dense::map find: " << duration.count() << std::endl;
        stats << "yfast::fastmap+ankerl::unordered_dense::map,find," << M << "," << duration.count() << std::endl;
    }

    for (auto M = M1 >> 1; M >= M0; M >>= 1) {
        start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < M; ++i) {
            auto key = shuffle[M + i];
            auto pos = fastmap_dense_map.find(key);
            fastmap_dense_map.erase(pos);
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = stop - start;
        std::cout << "M=" << M << " yfast::fastmap+ankerl::unordered_dense::map find+erase: " << duration.count() << std::endl;
        stats << "yfast::fastmap+ankerl::unordered_dense::map,find+erase," << M << "," << duration.count() << std::endl;
    }

    fastmap_dense_map.clear();
#endif

    stats.close();

    return EXIT_SUCCESS;
}
