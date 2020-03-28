#pragma once
#define DEBUG
// #include <cstring>

// using namespace std;

typedef unsigned long long u64;
typedef unsigned char u8;

enum replacement_policy: u8 {
    LRU,
    RAND,
    BT
};

enum write_policy: u8 { 
    write_through_allocate,
    write_through_not_allocate,
    write_back_allocate, 
    write_back_not_allocate, 
};

const u64 ADDRESS_SIZE = 64;

// tags: |valid(1)|dirty(1)|blankspace|tag(t)|
struct Cacheline {
    u8* tags;

    u64 get_tag(int tag_bytes, u64 cache_t);
    bool get_valid(int tag_bytes);
    bool get_dirty(int tag_bytes);
    void set_tag(int tag_bytes, u64 cache_t, u64 tag);
    void set_valid(int tag_bytes, bool valid);
    void set_dirty(int tag_bytes, bool dirty);
};

struct Cacheset {
    Cacheline* cachelines;
    u8* stack;
};

class Cache{
    public:
        const char* repl_policy[4] = {
            "write through allocate",
            "write through not allocate",
            "write back allocate", 
            "write back not allocate", 
        };

        u64 cache_size;
        u64 cache_line_size;
        u64 cache_line_num;
        u64 cache_mapping_ways;
        u64 cache_set_num;
        // 2^s = set num
        u64 cache_s;
        // 2^b = line size
        u64 cache_b;
        u64 cache_t;
        int tag_bytes;

        replacement_policy rp;
        write_policy wp;

        u64 hit_count, miss_count;
        // u64 read_count, write_count;

        // Cacheline* cachelines;
        Cacheset* cachesets;

        Cache();
        ~Cache();

        void init(u64 cache_size, u64 cache_line_size, u64 cache_mapping_ways, replacement_policy rp, write_policy wp);
        int check_hit(u64 set_index, u64 tag);
        void read(u64 address);
        void write(u64 address);
        void load_trace(const char* filename);
        double get_missrate();
};