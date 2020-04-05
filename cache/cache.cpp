#include "cache.h"
#include <cmath>
#include <fstream>
// #include <cstdio>
#include <cstring>
#include <ctime>


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
    tags[tag_bytes - 1] = (tags[tag_bytes - 1] & (0b11 << 6)) + u8(tag >> ((tag_bytes - 1) * 8));
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

u64 Cacheset::get_bottom(u64 cache_mapping_ways) {
    u64 bottom = get(cache_mapping_ways, cache_mapping_ways - 1);
    switch(cache_mapping_ways) {
        case 8:
            stack[2] = (stack[2] << 3) + (stack[1] >> 5);
            stack[1] = (stack[1] << 3) + (stack[0] >> 5);
            stack[0] = (stack[0] << 3) + u8(bottom);
            break;
        case 4:
            stack[0] = (stack[0] << 6) + u8(bottom);
            break;
        case 1 << 11:
        case 1 << 12:
        case 1 << 14:
            for (int i = cache_mapping_ways - 1; i > 0; i--) {
                dstack[i] = dstack[i - 1];
            }
            dstack[0] = u16(bottom);
            break;
        default:
            printf("%lld mapping ways is illegal!\n", cache_mapping_ways);
    }

    #ifdef DEBUG
        printf("get line %lld at bottom", bottom);
    #endif

    return bottom;
}

u64 Cacheset::get(u64 cache_mapping_ways, u64 index) {
    switch(cache_mapping_ways) {
        case 8:
            switch(index) {
                case 0:
                    return u64(stack[0] & 0b111);
                case 1:
                    return u64((stack[0] >> 3) & 0b111);
                case 2:
                    return u64(((stack[1] & 0b1) << 2) + (stack[0] >> 6));
                case 3:
                    return u64((stack[1] >> 1) & 0b111);
                case 4:
                    return u64((stack[1] >> 4) & 0b111);
                case 5:
                    return u64(((stack[2] & 0b11) << 1) + (stack[1] >> 7));
                case 6:
                    return u64((stack[2] >> 2) & 0b111);
                case 7:
                    return u64(stack[2] >> 5);
                default:
                    printf("index %lld is illegal!\n", index);
            }
            break;
        case 4:
            return u64((stack[0] >> (2 * index)) & 0b11);
        case 1 << 11:
        case 1 << 12:
        case 1 << 14:
            return u64(dstack[index]);
            break;
        default:
            printf("%lld mapping ways is illegal!\n", cache_mapping_ways);
    }
}

u64 Cacheset::find(u64 cache_mapping_ways, u64 line_index) {
    switch(cache_mapping_ways) {
        case 8:
            for (u64 i = 0; i < 8; i++) {
                if (line_index == get(8, i)) {
                    return i;
                }
            }
            break;
        case 4:
            for (u64 i = 0; i < 4; i++) {
                if (line_index == get(4, i)) {
                    return i;
                }
            }
            break;
        case 1 << 11:
        case 1 << 12:
        case 1 << 14:
            for (u64 i = 0; i < cache_mapping_ways; i++) {
                if (line_index == dstack[i]) {
                    return i;
                }
            }
            break;
        default:
            printf("%lld mapping ways is illegal!\n", cache_mapping_ways);
    }
}

void Cacheset::push(u64 cache_mapping_ways, u64 line_index) {
    u64 stack_index = find(cache_mapping_ways, line_index);
    u16 line = u16(get(cache_mapping_ways, stack_index));
    #ifdef DEBUG
        printf("pushing line %lld at %lld\n", line_index, stack_index);
    #endif

    switch(cache_mapping_ways) {
        case 8:
            switch(stack_index) {
                case 0:
                    break;
                case 1:
                    stack[0] = (stack[0] & 0b11000000) + ((stack[0] & 0b111) << 3) + line;
                    break;
                case 2:
                    stack[1] = (stack[1] & 0b11111110) + ((stack[0] >> 5) & 0b1);
                    stack[0] = (stack[0] << 3) + line;
                    break;
                case 3:
                    stack[1] = (stack[1] & 0b11110000) + ((stack[1] & 0b1) << 3) + (stack[0] >> 5);
                    stack[0] = (stack[0] << 3) + line;
                    break;
                case 4:
                    stack[1] = (stack[1] & 0b10000000) + ((stack[1] & 0b1111) << 3) + (stack[0] >> 5);
                    stack[0] = (stack[0] << 3) + line;
                    break;
                case 5:
                    stack[2] = (stack[2] & 0b11111100) + ((stack[1] >> 5) & 0b11);
                    stack[1] = (stack[1] << 3) + (stack[0] >> 5);
                    stack[0] = (stack[0] << 3) + line;
                    break;
                case 6:
                    stack[2] = (stack[2] & 0b11100000) + ((stack[2] & 0b11) << 3) + (stack[1] >> 5);
                    stack[1] = (stack[1] << 3) + (stack[0] >> 5);
                    stack[0] = (stack[0] << 3) + line;
                    break;
                case 7:
                    stack[2] = (stack[2] << 3) + (stack[1] >> 5);
                    stack[1] = (stack[1] << 3) + (stack[0] >> 5);
                    stack[0] = (stack[0] << 3) + line;
                    break;
                default:
                    printf("stack index %lld is illegal!\n", stack_index);
            }
            break;
        case 4:
            switch(stack_index) {
                case 0:
                    break;
                case 1:
                    stack[0] = (stack[0] && 0b11110000) + ((stack[0] & 0b11) << 2) + line;
                    break;
                case 2:
                    stack[0] = (stack[0] && 0b11000000) + ((stack[0] & 0b1111) << 4) + line;
                    break;
                case 3:
                    stack[0] = (stack[0] << 6) + line;
                    break;
                default:
                    printf("stack index %lld is illegal!\n", stack_index);
            }
            break;
        case 1 << 11:
        case 1 << 12:
        case 1 << 14:
            for (int i = stack_index; i > 0; i--) {
                dstack[i] = dstack[i - 1];
            }
            dstack[0] = line;
            break;
        default:
            printf("%lld mapping ways is illegal!\n", cache_mapping_ways);
    }
}



u64 Cacheset::tree_get_bottom(u64 cache_mapping_ways) {
    u64 index = 0;
    u64 max = cache_mapping_ways  - 1;
    while(index < max) {
        if(tree_get(cache_mapping_ways, index) == 0) {
            index = 2 * index + 1;
        } else {
            index = 2 * index + 2;
        }
    }
    return index - max;
}

u64 Cacheset::tree_get(u64 cache_mapping_ways, u64 index) {
    return u64((btree[index / 8] >> (7 - (index % 8))) & 1);
}

void Cacheset::tree_set(u64 cache_mapping_ways, u64 index, u8 value) {
    btree[index / 8] = (value << (7 - (index % 8))) + (btree[index / 8] & mask[index % 8]);
}

void Cacheset::tree_access(u64 cache_mapping_ways, u64 line_index) {
    u64 index = 0;
    u64 left = 0;
    u64 right = cache_mapping_ways;
    while(right - left > 1) {
        u64 middle = (left + right) / 2;
        if (line_index < middle) {
            tree_set(cache_mapping_ways, index, 1);
            index = 2 * index + 1;
            right = middle;
        } else {
            tree_set(cache_mapping_ways, index, 0);
            index = 2 * index + 2;
            left = middle;
        }
    }
}

Cache::Cache() {}

Cache::~Cache() {

}

void Cache::init(u64 cache_size, u64 cache_line_size, u64 cache_mapping_ways, replacement_policy rp, write_policy wp) {
    this->cache_size = cache_size;
    this->cache_line_size = cache_line_size;
    this->cache_line_num = cache_size / cache_line_size;
    this->cache_mapping_ways = cache_mapping_ways;
    this->rp = rp;
    this->wp = wp;
    this->cache_set_num = cache_line_num / cache_mapping_ways;
    this->cache_s = u64(log2(cache_set_num));
    this->cache_b = u64(log2(cache_line_size));
    this->cache_t = ADDRESS_SIZE - cache_b - cache_s;

    this->hit_count = 0;
    this->miss_count = 0;

    this->tag_bytes = ((cache_t + 1 + (u64(wp) >> 1)) > 56) ? 8 : 7;
    this->cachesets = new Cacheset[cache_set_num];

    for (int i = 0; i < cache_set_num; i++) {
        cachesets[i].cachelines = new Cacheline[cache_mapping_ways];
            for (int j = 0; j < cache_mapping_ways; j++) {
                cachesets[i].cachelines[j].tags = new u8[tag_bytes];
                memset(cachesets[i].cachelines[j].tags, 0, tag_bytes);
            }
            if (rp == LRU && cache_mapping_ways > 1) {
                switch(cache_mapping_ways) {
                    case 8:
                        cachesets[i].stack = new u8[3];
                        cachesets[i].stack[0] = 0b10001000;
                        cachesets[i].stack[1] = 0b11000110;
                        cachesets[i].stack[2] = 0b11111010;
                        break;
                    case 4:
                        cachesets[i].stack = new u8[1];
                        cachesets[i].stack[0] = 0b11100100;
                        break;
                    case 1 << 11:
                    case 1 << 12:
                    case 1 << 14:
                        cachesets[i].dstack = new u16[cache_mapping_ways];
                        for (int j = 0; j < cache_mapping_ways; j++) {
                            cachesets[i].dstack[j] = u16(j);
                        }
                        break;
                    default:
                        printf("%lld mapping ways is illegal!\n", cache_mapping_ways);
                }
            } else if (rp == BT && cache_mapping_ways > 1) {
                switch(cache_mapping_ways) {
                    case 8:
                    case 4:
                        cachesets[i].btree = new u8[1];
                        cachesets[i].btree[0] = 0;
                        break;
                    case 1 << 11:
                    case 1 << 12:
                    case 1 << 14:
                        cachesets[i].btree = new u8[cache_mapping_ways >> 3];
                        memset(cachesets[i].btree, 0, cache_mapping_ways >> 3);
                    default:
                        printf("%lld mapping ways is illegal!\n", cache_mapping_ways);
                }
            }
    }

    if (rp == RAND) {
        srand(int(time(0)));
    }

    #ifdef DEBUG
        printf("init:\nreplacement policy: %s\nwrite policy: %s\ntag bytes: %d\nmapping ways: %lld\nline size: %lld\n", repl_policy[int(rp)], w_policy[int(wp)], tag_bytes, cache_mapping_ways, cache_line_size);
        printf("cache line num: %lld\n", cache_line_num);
        printf("cache set num: %lld\n", cache_set_num);
        printf("t: %lld\ns: %lld\nb: %lld\n", cache_t, cache_s, cache_b);
    #endif
}

int Cache::check_hit(u64 set_index, u64 tag) {
    #ifdef DEBUG
        printf("checking set %lld\n", set_index);
    #endif

    for (int i = 0; i < cache_mapping_ways; i++) {
        #ifdef DEBUG
            printf("line %d tag : %lld valid %d\n", i, cachesets[set_index].cachelines[i].get_tag(tag_bytes, cache_t), int(cachesets[set_index].cachelines[i].get_valid(tag_bytes)));
        #endif

        if (cachesets[set_index].cachelines[i].get_tag(tag_bytes, cache_t) == tag && 
            cachesets[set_index].cachelines[i].get_valid(tag_bytes)) {
                hit_count++;
                return i;
        }
    }
    miss_count++;
    return -1;
}

int Cache::get_free_line(u64 set_index) {
    for (int i = 0; i < cache_mapping_ways; i++) {
        if (!cachesets[set_index].cachelines[i].get_valid(tag_bytes)) {
            return i;
        }
    }
    #ifdef DEBUG
        printf("++++++++++++++++++++valids of set %lld\n", set_index);
        for (int i = 0; i < cache_mapping_ways; i++) {
            printf("%d: %d ", i, int(cachesets[set_index].cachelines[i].get_valid(tag_bytes)));
        }
        printf("\n++++++++++++++++++++\n");
    #endif
    return -1;
}

int Cache::choose_victim(u64 set_index, u64 tag) {
    switch(rp) {
        case LRU:
            if (cache_mapping_ways > 1) {
                return int(cachesets[set_index].get_bottom(cache_mapping_ways));
            } else {
                return 0;
            }
            break;
        case RAND:
            return rand() % cache_mapping_ways;
            break;
        case BT:
            if (cache_mapping_ways > 1) {
                return int(cachesets[set_index].tree_get_bottom(cache_mapping_ways));
            } else {
                return 0;
            }
        default:
            printf("error! no such replacement policy!\n");
    }
}

void Cache::replace(u64 set_index, u64 line_index, u64 tag) {
    cachesets[set_index].cachelines[line_index].set_tag(tag_bytes, cache_t, tag);
    cachesets[set_index].cachelines[line_index].set_valid(tag_bytes, true);
    if (wp == write_back_allocate || wp == write_back_not_allocate) {
        cachesets[set_index].cachelines[line_index].set_dirty(tag_bytes, false);
    }
}

void Cache::read(u64 address) {
    #ifdef DEBUG
        printf("*************************\nread 0x%llx\n", address);
    #endif

    u64 tag, set_index;
    int line_index, repl_line;
    tag = address >> (cache_b + cache_s);
    set_index = (address >> cache_b) % cache_set_num;
    line_index = check_hit(set_index, tag);

    #ifdef DEBUG
        printf("tag: %lld\n", tag);
        printf("set: %lld\n", set_index);
    #endif

    if (line_index >= 0) {
        #ifdef LOG
            printf("Hit\n");
        #endif

        read_hit++;
        if (rp == LRU && cache_mapping_ways > 1) {
            cachesets[set_index].push(cache_mapping_ways, line_index);
        } else if (rp == BT && cache_mapping_ways > 1) {
            cachesets[set_index].tree_access(cache_mapping_ways, line_index);
        }
    } else {
        #ifdef LOG
            printf("Miss\n");
        #endif

        read_miss++;
        repl_line = get_free_line(set_index);
        if (repl_line < 0) {
            #ifdef DEBUG
                printf("choose victim\n");
            #endif

            repl_line = choose_victim(set_index, tag);
        } else {
            #ifdef DEBUG
                printf("get free line\n");
            #endif

            if (rp == LRU && cache_mapping_ways > 1) {
                cachesets[set_index].push(cache_mapping_ways, repl_line);
            } else if (rp == BT && cache_mapping_ways > 1) {
                cachesets[set_index].tree_access(cache_mapping_ways, repl_line);
            }
        }
        replace(set_index, repl_line, tag);
    }

    #ifdef DEBUG
        if(line_index >= 0) {
            printf("hit line %d\n", line_index);
        } else {
            printf("miss, replace line %d\n", repl_line);
        }
        if (rp == LRU && cache_mapping_ways > 1) {
            printf("LRU stack: ");
            for (u64 i = 0; i < cache_mapping_ways; i++) {
                printf("%lld ", cachesets[set_index].get(cache_mapping_ways, i));
            }
            printf("\n");
        } else if (rp == BT && cache_mapping_ways > 1) {
            printf("--------------------\nBinary Tree:\n");
            switch(cache_mapping_ways) {
                case 4:
                    printf("%lld\n", cachesets[set_index].tree_get(cache_mapping_ways, 0));
                    printf("%lld %lld\n", cachesets[set_index].tree_get(cache_mapping_ways, 1), cachesets[set_index].tree_get(cache_mapping_ways, 2));
                    break;
                case 8:
                    printf("%lld\n", cachesets[set_index].tree_get(cache_mapping_ways, 0));
                    printf("%lld %lld\n", cachesets[set_index].tree_get(cache_mapping_ways, 1), cachesets[set_index].tree_get(cache_mapping_ways, 2));
                    printf("%lld %lld %lld %lld\n", cachesets[set_index].tree_get(cache_mapping_ways, 3), cachesets[set_index].tree_get(cache_mapping_ways, 4), cachesets[set_index].tree_get(cache_mapping_ways, 5), cachesets[set_index].tree_get(cache_mapping_ways, 6));
                    break;
                default:
                    break;
            }
            printf("--------------------\n");
        }
    #endif
}

void Cache::write(u64 address) {
    #ifdef DEBUG
        printf("*************************\nwrite 0x%llx\n", address);
    #endif

    u64 tag, set_index;
    int line_index, repl_line;
    tag = address >> (cache_b + cache_s);
    set_index = (address >> cache_b) % cache_set_num;
    line_index = check_hit(set_index, tag);

    #ifdef DEBUG
        printf("tag: %lld\n", tag);
        printf("set: %lld\n", set_index);
    #endif

    if(line_index >= 0) {
        #ifdef LOG
            printf("hit\n");
        #endif

        write_hit++;
        if(rp == LRU && cache_mapping_ways > 1) {
            cachesets[set_index].push(cache_mapping_ways, line_index);
        } else if (rp == BT && cache_mapping_ways > 1) {
            cachesets[set_index].tree_access(cache_mapping_ways, line_index);
        }
    } else {
        #ifdef LOG
            printf("Miss\n");
        #endif

        write_miss++;
        switch(wp) {
            case write_through_allocate:
            case write_back_allocate:
                repl_line = get_free_line(set_index);
                if (repl_line < 0) {
                    #ifdef DEBUG
                        printf("choose victim\n");
                    #endif

                    repl_line = choose_victim(set_index, tag);
                } else {
                    #ifdef DEBUG
                        printf("get free line\n");
                    #endif

                    if (rp == LRU && cache_mapping_ways > 1) {
                        cachesets[set_index].push(cache_mapping_ways, repl_line);
                    } else if (rp == BT && cache_mapping_ways > 1) {
                        cachesets[set_index].tree_access(cache_mapping_ways, repl_line);
                    }
                }
                replace(set_index, repl_line, tag);

                #ifdef DEBUG
                    printf("replaced line %d\n", repl_line);
                #endif

                break;
            case write_through_not_allocate:
            case write_back_not_allocate:
                #ifdef DEBUG
                    printf("%s: not doing anything\n", w_policy[int(wp)]);
                #endif
                break;
            default:
                printf("illegal write policy: %d\n", int(wp));
        }
    }
    #ifdef DEBUG
        if (rp == LRU && cache_mapping_ways > 1) {
            printf("LRU stack: ");
            for (u64 i = 0; i < cache_mapping_ways; i++) {
                printf("%lld ", cachesets[set_index].get(cache_mapping_ways, i));
            }
            printf("\n");
        } else if (rp == BT && cache_mapping_ways > 1) {
            printf("--------------------\nBinary Tree:\n");
            switch(cache_mapping_ways) {
                case 4:
                    printf("%lld\n", cachesets[set_index].tree_get(cache_mapping_ways, 0));
                    printf("%lld %lld\n", cachesets[set_index].tree_get(cache_mapping_ways, 1), cachesets[set_index].tree_get(cache_mapping_ways, 2));
                    break;
                case 8:
                    printf("%lld\n", cachesets[set_index].tree_get(cache_mapping_ways, 0));
                    printf("%lld %lld\n", cachesets[set_index].tree_get(cache_mapping_ways, 1), cachesets[set_index].tree_get(cache_mapping_ways, 2));
                    printf("%lld %lld %lld %lld\n", cachesets[set_index].tree_get(cache_mapping_ways, 3), cachesets[set_index].tree_get(cache_mapping_ways, 4), cachesets[set_index].tree_get(cache_mapping_ways, 5), cachesets[set_index].tree_get(cache_mapping_ways, 6));
                    break;
                default:
                    break;
            }
            printf("--------------------\n");
        }
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