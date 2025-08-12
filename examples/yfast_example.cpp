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
