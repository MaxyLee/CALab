#include <iostream>
#include "cache.h"

using std::cout;

int main(int argc, char* argv[]) {
    // for (int i = 0; i < argc; i++) {
    //     std::cout << argv[i] << std::endl;
    // }
    Cache c;
    u64 cache_size = 1 << 17;
    u64 cache_line_size = 64;
    u64 cache_mapping_ways = 8;
    replacement_policy rp = LRU;
    write_policy wp = write_back_allocate;
    double miss_rate;
    const char* filename = "trace/astar.trace";
    c.init(cache_size, cache_line_size, cache_mapping_ways, rp, wp);
    c.load_trace(filename);
    miss_rate = c.get_missrate();
    printf("********************\nmiss rate: %llf%%\n", 100 * miss_rate);
    return 0;
}