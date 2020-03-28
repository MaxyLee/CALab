#include "cache.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstring>

using namespace std;

u64 Cacheline::get_tag(int tag_bytes, u64 cache_t) {
    u64 tag = 0;
    for (int i = 0; i < tag_bytes - 1; i++) {
        tag += u64(tags[i]) << (i * 8);
    }
    tag += u64(tags[tag_bytes - 1] & 0b111111) << ((tag_bytes - 1) * 8);
    return tag;
}

bool Cacheline::get_valid(int tag_bytes) {
    return bool(tags[tag_bytes - 1] >> 7);
}

bool Cacheline::get_dirty(int tag_bytes) {
    return bool((tags[tag_bytes - 1] >> 6) & 1);
}

void Cacheline::set_tag(int tag_bytes, u64 cache_t, u64 tag) {
    for (int i = 0; i < tag_bytes - 1; i++) {
        tags[i] = u8(tag >> i * 8);
    }
    u8 mask = 1;
    for (int i = 0; i < cache_t - (tag_bytes - 1) * 8; i++) {
        mask = (mask << 1) + 1;
    }
    tags[tag_bytes - 1] = (tags[tag_bytes - 1] & (0b11 << 6)) + u8((tag >> ((tag_bytes - 1) * 8)) & mask);
}

void Cacheline::set_valid(int tag_bytes, bool valid) {
    if (valid != get_valid(tag_bytes)) {
        tags[tag_bytes - 1] += u8(1 << 7);
    }
}

void Cacheline::set_dirty(int tag_bytes, bool dirty) {
    if (dirty != get_dirty(tag_bytes)) {
        if(get_dirty(tag_bytes)) {
            tags[tag_bytes - 1] -= u8(1 << 6);
        } else {
            tags[tag_bytes - 1] += u8(1 << 6);
        }
    }
}

Cache::Cache() {}
Cache::~Cache() {}

void Cache::init(u64 cache_size, u64 cache_line_size, u64 cache_mapping_ways, replacement_policy rp, write_policy wp) {
    this->cache_size = cache_size;
    this->cache_line_size = cache_line_size;
    this->cache_line_num = cache_size / cache_line_size;
    this->cache_mapping_ways = cache_mapping_ways;
    this->rp = rp;
    this->wp = wp;
    this->cache_set_num = cache_size / (cache_line_size * cache_mapping_ways);
    this->cache_s = u64(log2(cache_set_num));
    this->cache_b = u64(log2(cache_line_size));
    this->cache_t = ADDRESS_SIZE - cache_b - cache_s;

    this->hit_count = 0;
    this->miss_count = 0;

    this->cachesets = new Cacheset[cache_set_num];
    this->tag_bytes = (cache_t + 1 + (u64(wp) >> 1) > 56) ? 8 : 7;
    u64 stack_size = u64(cache_mapping_ways * log2(cache_mapping_ways));
    for (int i = 0; i < cache_set_num; i++) {
        cachesets[i].cachelines = new Cacheline[cache_mapping_ways];
        for (int j = 0; j < cache_mapping_ways; j++) {
            cachesets[i].cachelines[j].tags = new u8[tag_bytes];
        }
        cachesets[i].stack = new u8[stack_size];
    }
    #ifdef DEBUG
        printf("init:\nt: %lld\ns: %lld\nb: %lld\nreplacement policy: %s\ntag bytes: %d\nstack size: %d\n", cache_t, cache_s, cache_b, repl_policy[int(rp)], tag_bytes, stack_size);
    #endif
}

int Cache::check_hit(u64 set_index, u64 tag) {
    for (int i = 0; i < cache_mapping_ways; i++) {
        if (cachesets[set_index].cachelines[i].get_tag(tag_bytes, cache_t) == tag && 
            cachesets[set_index].cachelines[i].get_valid(tag_bytes)) {

        }
    }
}

void Cache::read(u64 address) {
    #ifdef DEBUG
        printf("read 0x%llx\n", address);
    #endif
    u64 tag, index;
    int line_index;
    tag = address >> (cache_b + cache_s);
    index = (address >> cache_b) % cache_set_num;

}

void Cache::write(u64 address) {
    #ifdef DEBUG
        printf("write 0x%llx\n", address);
    #endif
}

void Cache::load_trace(const char* filename) {
    FILE* trace = fopen(filename, "r");

    if (!trace) {
        printf("failed to load %s ...\n", filename);
        return;
    }

    while(!feof(trace)) {
        char op;
        u64 addr;
        fscanf(trace, "%c %llx\n", &op, &addr);
        if (op == 'r' || op == 'l') {
            read(addr);
        } else if (op == 'w' || op == 's') {
            write(addr);
        }
    }
}

double Cache::get_missrate() {
    return double(miss_count) / (miss_count + hit_count);
}