#pragma once
#define DEBUG

typedef unsigned long long u64;
typedef unsigned short u16;
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
    u16* dstack;

    u64 get_bottom(u64 cache_mapping_ways);
    u64 get(u64 cache_mapping_ways, u64 index);
    u64 find(u64 cache_mapping_ways, u64 line_index);
    void push(u64 cache_mapping_ways, u64 line_index);
};

class Cache{
    public:
        const char* w_policy[4] = {
            "write through allocate",
            "write through not allocate",
            "write back allocate", 
            "write back not allocate", 
        };
        const char* repl_policy[3] = {
            "LRU",
            "RAND",
            "BT",
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
        u64 read_hit, read_miss;
        u64 write_hit, write_miss;

        // Cacheline* cachelines;
        Cacheset* cachesets;

        Cache();
        ~Cache();

        void init(u64 cache_size, u64 cache_line_size, u64 cache_mapping_ways, replacement_policy rp, write_policy wp);
        int check_hit(u64 set_index, u64 tag);
        int get_free_line(u64 set_index);
        int choose_victim(u64 set_index, u64 tag);
        void replace(u64 set_index, u64 line_index, u64 tag);
        void read(u64 address);
        void write(u64 address);
        void load_trace(const char* filename);
        double get_missrate();
};