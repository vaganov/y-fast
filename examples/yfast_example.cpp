#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

#include <unistd.h>

#include <yfast/fastmap.h>

int main() {
    yfast::fastmap<std::uint32_t, std::string, 32> fastmap;

    std::uint32_t key = 1;
    std::string value = "one";
    auto i = fastmap.insert(key, value);
    auto j = fastmap.find(key);
    assert(i == j);
    assert(*j == value);
    assert(fastmap[key] == value);

    for (const auto& v: fastmap) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

    i = fastmap.erase(i);
    assert(i == fastmap.end());
    j = fastmap.find(key);
    assert(j == fastmap.end());

    return EXIT_SUCCESS;
}
