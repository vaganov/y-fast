#include <cstdint>
#include <map>
#include <string>

#include <yfast/fastmap.h>
#include <yfast/iterator.h>

#include <gtest/gtest.h>

TEST(fastmap, empty) {
    yfast::fastmap<std::uint8_t, void, 8> fastmap;
    EXPECT_TRUE(fastmap.empty());
    EXPECT_EQ(fastmap.find(0), fastmap.end());
    EXPECT_EQ(fastmap.pred(0), fastmap.end());
    EXPECT_EQ(fastmap.succ(0), fastmap.end());
    EXPECT_EQ(fastmap.lower_bound(0), fastmap.end());
    EXPECT_EQ(fastmap.upper_bound(0), fastmap.end());
}

TEST(fastmap, empty_const) {
    const yfast::fastmap<std::uint8_t, void, 8> fastmap;
    EXPECT_TRUE(fastmap.empty());
    EXPECT_EQ(fastmap.find(0), fastmap.end());
    EXPECT_EQ(fastmap.pred(0), fastmap.end());
    EXPECT_EQ(fastmap.succ(0), fastmap.end());
    EXPECT_EQ(fastmap.lower_bound(0), fastmap.end());
    EXPECT_EQ(fastmap.upper_bound(0), fastmap.end());
    EXPECT_THROW(fastmap.at(0), std::out_of_range);
}

TEST(fastmap, rebuild) {
    yfast::fastmap<std::uint8_t, void, 8> fastmap;
    auto i = fastmap.insert(0);
    auto j = fastmap.insert(17);
    for (auto key = 1; key < 17; ++key) {
        fastmap.insert(key);
    }
    EXPECT_EQ(i--, fastmap.begin());
    EXPECT_EQ(++j, fastmap.end());
}

TEST(fastmap, empty_erase) {
    yfast::fastmap<std::uint8_t, void, 8> fastmap;
    yfast::fastmap<std::uint8_t, void, 8>::iterator i;
    EXPECT_THROW(fastmap.erase(i), std::invalid_argument);

    i = fastmap.erase(fastmap.end());
    EXPECT_EQ(i, fastmap.end());
}

TEST(fastmap, null_iterator) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap;
    auto i = fastmap.end();
    EXPECT_THROW(i.key(), std::out_of_range);
    EXPECT_THROW(i.value(), std::out_of_range);
    EXPECT_THROW(*i, std::out_of_range);
    EXPECT_THROW(i->size(), std::out_of_range);
}

TEST(fastmap, iterator_pre_inc) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    for (auto i = fastmap.begin(); i != fastmap.end(); ++i) {
        EXPECT_EQ(i.value(), map[i.key()]);
    }
}

TEST(fastmap, iterator_post_inc) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    for (auto i = fastmap.begin(); i != fastmap.end(); i++) {
        EXPECT_EQ(i.value(), map[i.key()]);
    }
}

TEST(fastmap, iterator_pre_dec) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    auto i = fastmap.end();
    while (true) {
        --i;
        EXPECT_EQ(i.value(), map[i.key()]);
        if (i == fastmap.begin()) {
            break;
        }
    }
}

TEST(fastmap, iterator_post_dec) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    auto i = fastmap.end();
    while (true) {
        i--;
        EXPECT_EQ(i.value(), map[i.key()]);
        if (i == fastmap.begin()) {
            break;
        }
    }
}

TEST(fastmap, const_iterator_pre_inc) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    for (auto i = fastmap.cbegin(); i != fastmap.cend(); ++i) {
        EXPECT_EQ(i.value(), map[i.key()]);
    }
}

TEST(fastmap, const_iterator_post_inc_1) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    for (auto i = fastmap.cbegin(); i != fastmap.cend(); i++) {
        EXPECT_EQ(i.value(), map[i.key()]);
    }
}

TEST(fastmap, const_iterator_post_inc_2) {
    const yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    for (auto i = fastmap.begin(); i != fastmap.end(); i++) {
        EXPECT_EQ(i.value(), map[i.key()]);
    }
}

TEST(fastmap, const_iterator_pre_dec) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    auto i = fastmap.cend();
    while (true) {
        --i;
        EXPECT_EQ(i.value(), map[i.key()]);
        if (i == fastmap.cbegin()) {
            break;
        }
    }
}

TEST(fastmap, const_iterator_post_dec) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    auto i = fastmap.cend();
    while (true) {
        i--;
        EXPECT_EQ(i.value(), map[i.key()]);
        if (i == fastmap.cbegin()) {
            break;
        }
    }
}

TEST(fastmap, reverse_iterator_pre_inc) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    for (auto r = fastmap.rbegin(); r != fastmap.rend(); ++r) {
        EXPECT_EQ(r.value(), map[r.key()]);
    }
}

TEST(fastmap, reverse_iterator_post_inc) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    for (auto r = fastmap.rbegin(); r != fastmap.rend(); r++) {
        EXPECT_EQ(r.value(), map[r.key()]);
    }
}

TEST(fastmap, reverse_iterator_pre_dec) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    auto i = fastmap.rend();
    while (true) {
        --i;
        EXPECT_EQ(i.value(), map[i.key()]);
        if (i == fastmap.rbegin()) {
            break;
        }
    }
}

TEST(fastmap, reverse_iterator_post_dec) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    auto i = fastmap.rend();
    while (true) {
        i--;
        EXPECT_EQ(i.value(), map[i.key()]);
        if (i == fastmap.rbegin()) {
            break;
        }
    }
}

TEST(fastmap, const_reverse_iterator_pre_inc) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    for (auto r = fastmap.crbegin(); r != fastmap.crend(); ++r) {
        EXPECT_EQ(r.value(), map[r.key()]);
    }
}

TEST(fastmap, const_reverse_iterator_post_inc_1) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    for (auto r = fastmap.crbegin(); r != fastmap.crend(); r++) {
        EXPECT_EQ(r.value(), map[r.key()]);
    }
}

TEST(fastmap, const_reverse_iterator_post_inc_2) {
    const yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    for (auto r = fastmap.rbegin(); r != fastmap.rend(); r++) {
        EXPECT_EQ(r.value(), map[r.key()]);
    }
}

TEST(fastmap, const_reverse_iterator_pre_dec) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    auto i = fastmap.crend();
    while (true) {
        --i;
        EXPECT_EQ(i.value(), map[i.key()]);
        if (i == fastmap.crbegin()) {
            break;
        }
    }
}

TEST(fastmap, const_reverse_iterator_post_dec) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap { {1, "one"}, {2, "two"}, {3, "three"} };
    std::map<std::uint8_t, std::string> map { {1, "one"}, {2, "two"}, {3, "three"} };

    auto i = fastmap.crend();
    while (true) {
        i--;
        EXPECT_EQ(i.value(), map[i.key()]);
        if (i == fastmap.crbegin()) {
            break;
        }
    }
}

TEST(fastmap, loc) {
    yfast::fastmap<std::uint8_t, std::string, 8> fastmap;

    EXPECT_THROW(fastmap.at(0), std::out_of_range);

    EXPECT_EQ(fastmap[0], "");

    fastmap[0] = "zero";
    EXPECT_EQ(fastmap[0], "zero");
    EXPECT_EQ(fastmap.at(0), "zero");
}

TEST(fastmap, erase_by_key) {
    yfast::fastmap<std::uint8_t, void, 8> fastmap;

    EXPECT_FALSE(fastmap.erase(0));

    fastmap.insert(0);
    EXPECT_NE(fastmap.find(0), fastmap.end());

    EXPECT_TRUE(fastmap.erase(0));
    EXPECT_EQ(fastmap.find(0), fastmap.end());
}

TEST(fastmap, make_reverse) {
    yfast::fastmap<std::uint8_t, void, 8> fastmap;
    auto i = fastmap.insert(0);
    EXPECT_EQ(yfast::make_reverse_iterator(fastmap.begin()), fastmap.rend());
    EXPECT_EQ(yfast::make_reverse_iterator(fastmap.end()), fastmap.rbegin());
    EXPECT_EQ(yfast::make_reverse_iterator(fastmap.cbegin()), fastmap.crend());
    EXPECT_EQ(yfast::make_reverse_iterator(fastmap.cend()), fastmap.crbegin());
}
