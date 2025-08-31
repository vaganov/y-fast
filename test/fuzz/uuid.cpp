#include <cstdlib>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <yfast/fastmap.h>
#include <yfast/iterator.h>

#include <gtest/gtest.h>

typedef boost::uuids::uuid Key;

class BitExtractor {
public:
    typedef yfast::internal::BitExtractor<std::vector<std::byte>>::ShiftResult ShiftResult;

    static bool extract_bit(const Key& key, unsigned int n) {
        auto data = reinterpret_cast<const std::byte*>(key.data());
        return yfast::internal::BitExtractor<std::vector<std::byte>>::extract_bit(data, key.size(), n);
    }

    static ShiftResult shift(const Key& key, unsigned int n) {
        auto data = reinterpret_cast<const std::byte*>(key.data());
        return yfast::internal::BitExtractor<std::vector<std::byte>>::shift(data, key.size(), n);
    }
};

template <typename T>
class CountingAllocator {
public:
    typedef T value_type;

    template <typename U>
    struct rebind {
        typedef CountingAllocator<U> other;
    };

private:
    int _alloc_count = 0;
    int _dealloc_count = 0;
    int _ctor_count = 0;
    int _dtor_count = 0;

public:
    CountingAllocator() noexcept = default;

    template <typename U>
    CountingAllocator(const CountingAllocator<U>& other) noexcept {}

    ~CountingAllocator() noexcept(false) { check(); }

    T* allocate(std::size_t n) {
        ++_alloc_count;
        auto p = ::operator new(n * sizeof(T));
        return static_cast<T*>(p);
    }

    void deallocate(T* p, std::size_t n) {
        ++_dealloc_count;
        ::operator delete(p);
    }

    template <typename ... Args>
    void construct(T* p, Args&& ... args) {
        ++_ctor_count;
        new (p) T(std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U* p) {
        ++_dtor_count;
        p->~U();
    }

private:
    void check() const noexcept(false) {
        if (_alloc_count != _dealloc_count) {
            throw std::runtime_error("alloc/dealloc mismatch");
        }
        if (_ctor_count != _dtor_count) {
            throw std::runtime_error("ctor/dtor mismatch");
        }
        if (_alloc_count != _ctor_count) {
            throw std::runtime_error("alloc/ctor mismatch");
        }
    }
};

TEST(fuzz, uuid) {
    constexpr unsigned int H = 128;
    yfast::fastmap<Key, void, H, BitExtractor, yfast::internal::DefaultHash<BitExtractor::ShiftResult, std::uintptr_t>, std::less<Key>, CountingAllocator<Key>> fastmap;

    boost::uuids::random_generator uuid_gen;
    for (auto i = 0; i < 1'000'000; ++i) {
        auto key = uuid_gen();
        fastmap.insert(key);
        if (::rand() % 2 == 0) {
            fastmap.insert(key);
        }
    }
    EXPECT_EQ(fastmap.size(), 1'000'000);

    ::srand(time(nullptr));
    for (auto i = fastmap.cbegin(); i != fastmap.cend(); ) {
        if (::rand() % 2 == 0) {
            i = fastmap.erase(i);
        }
        else {
            ++i;
        }
    }
    // law of the iterated logarithm
    EXPECT_LT(fastmap.size(), 502'000);
    EXPECT_GT(fastmap.size(), 498'000);

    for (auto cnt = 0; cnt < 1'000; ++cnt) {
        auto key = uuid_gen();
        auto i = fastmap.succ(key);
        auto r = yfast::make_reverse_iterator(i);
        ASSERT_NE(i, fastmap.end());
        EXPECT_GT(*i, key);
        ASSERT_NE(r, fastmap.rend());
        EXPECT_LT(*r, key);
    }
}
