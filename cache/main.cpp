#include <iostream>
#include <cstring>
#include "cache.h"

using std::cout;

int main(int argc, char* argv[]) {
    u64 cache_size = 1 << 17;
    const char* filename = argv[1];
    u64 cache_line_size = 0;
    u64 cache_mapping_ways = 8;
    replacement_policy rp = BT;
    write_policy wp = write_back_allocate;
    double miss_rate;
    Cache cache;

    for (int i = 0; i < strlen(argv[2]); i++) {
        cache_line_size = cache_line_size * 10 + (argv[2][i] - '0');
    }
    if (cache_line_size != 8 && cache_line_size != 32 && cache_line_size != 64) {
        printf("cache line size %s is illegal!\n", argv[2]);
        return 1;
    }

    if (argv[3][0] == 'f') {
        cache_mapping_ways = cache_size / cache_line_size;
    } else {
        cache_mapping_ways = argv[3][0] - '0';
    }
    if (cache_mapping_ways != 1 && cache_mapping_ways != 4 && cache_mapping_ways != 8 && argv[3][0] != 'f') {
        printf("cache mapping ways %s is illegal!\n", argv[3]);
        return 1;
    }

    for (int i = 0; i < 3; i++) {
        if (strcmp(argv[4], cache.repl_policy[i]) == 0) {
            rp = replacement_policy(i);
            break;
        }
    }

    for (int i = 0; i < 4; i++) {
        if (strcmp(argv[5], cache.w_policy[i]) == 0) {
            wp = write_policy(i);
            break;
        }
    }


    cache.init(cache_size, cache_line_size, cache_mapping_ways, rp, wp);
    cache.load_trace(filename);
    miss_rate = cache.get_missrate();
    printf("********************\nmiss rate: %llf%%\n********************\n", 100 * miss_rate);
    return 0;
}