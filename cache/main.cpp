#include <iostream>
#include "cache.h"

using std::cout;

int main(int argc, char* argv[]) {
    // for (int i = 0; i < argc; i++) {
    //     std::cout << argv[i] << std::endl;
    // }
    Cache c;
    u64 cache_size = 1 << 17;
    u64 cache_line_size = 8;
    u64 cache_mapping_ways = 8;
    replacement_policy rp = LRU;
    write_policy wp = write_back_allocate;
    const char* filename = "trace/astar.trace";
    // cout << (u64(wp) >> 1) << "\n";
    c.init(cache_size, cache_line_size, cache_mapping_ways, rp, wp);
    c.load_trace(filename);
    return 0;
}