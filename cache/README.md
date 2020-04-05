# Cache Lab

## 实验要求

使用 C/C++编写一个 Cache 替换策略模拟器。输入为存储器访问 trace， 输出为在不同的条件下(如 Cache 块大小，组织方式替换策略，不同的写策略等)，在给定 trace 上的缺失率，以及访问 log(记录命中和缺失的结果).观测不同的替换策略对于程序运行性能的影响.

## 算法实现

以下内容中所引用的代码均来自`cache.h`和`cache.cpp`

### cache实现

在实现过程中,为了方便使用了一些自定义类型,如下所示:

```C++
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
```

实现了`Cacheline`,`Cacheset`和`Cache`三个类,其中`Cacheset`的实现将在替换算法中详细介绍,以下主要介绍另外两个类的实现:

```C++
struct Cacheline {
    u8* tags;

    u64 get_tag(int tag_bytes, u64 cache_t);
    bool get_valid(int tag_bytes);
    bool get_dirty(int tag_bytes);
    void set_tag(int tag_bytes, u64 cache_t, u64 tag);
    void set_valid(int tag_bytes, bool valid);
    void set_dirty(int tag_bytes, bool dirty);
};
```

按照要求,为了节省空间,根据不同的cache布局,tags(其中包括tag、valid以及dirty)的长度可能是7或8,并实现了一些函数来对这些数据进行修改.

```C++
class Cache{
    public:
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
```

`Cache`类中保存了cache的一些关键信息,如布局、替换策略和写策略等,以及cacheline的必要信息,并实现了一些函数来进行读写等操作.

### 替换算法

实现了3种cache替换算法:LRU、BInary Tree和Random,具体实现过程中,对于不同的cache布局,实现方式也有所不同。

#### LRU

按照实验要求,采用了堆栈法实现了LRU,主要的数据结构如下:

```C++
struct Cacheset {
    Cacheline* cachelines;
    u8* stack;
    u16* dstack;

    u64 get_bottom(u64 cache_mapping_ways);
    u64 get(u64 cache_mapping_ways, u64 index);
    u64 find(u64 cache_mapping_ways, u64 line_index);
    void push(u64 cache_mapping_ways, u64 line_index);
};
```

`Cacheset`中用数组实现了一个栈,并实现了一些函数对栈进行维护,考虑到cache布局的不同,在不同情况下采用了两种方式来实现,即`u8* stack`和`u16* dstack`,前者为4、8路组映射,后者为全相连.当使用4路组映射时,只需要1个u8(8 bits)即可实现;当使用8路组映射时,需要3个u8(24 bits);当使用全相连时,由于行数过多,为了方便实现使用了u16的数组,每一个u16中保存一个行编号,考虑到是128KB的cache,以及8KB、32KB和64KB的block size,实际上每个u16中有几位是浪费的.

下面简略介绍一下这些函数的作用与实现:

1. `u64 get_bottom(u64 cache_mapping_ways)`

   取出栈底对应行的编号,并将其置于栈顶

   ```C++
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
       return bottom;
   }
   ```

   

2. ` u64 get(u64 cache_mapping_ways, u64 index)`

   获取栈中`index`位置中的行编号

   ```C++
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
   ```

   

3. `u64 find(u64 cache_mapping_ways, u64 line_index)`

   查找第`line_index`行在栈中的位置

   ```C++
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
   ```

   

4. `void push(u64 cache_mapping_ways, u64 line_index)`

   将第`line_index`行从栈中取出并置于栈顶

   ```C++
   void Cacheset::push(u64 cache_mapping_ways, u64 line_index) {
       u64 stack_index = find(cache_mapping_ways, line_index);
       u16 line = u16(get(cache_mapping_ways, stack_index));
   
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
   ```

#### Binary Tree

实现二叉树替换算法时,只需要在`Cacheset`中实现一个二叉树即可:

```C++
struct Cacheset {
    ......
    u8* btree;

    ......

    u64 tree_get_bottom(u64 cache_mapping_ways);
    u64 tree_get(u64 cache_mapping_ways, u64 index);
    void tree_set(u64 cache_mapping_ways, u64 index, u8 value);
    void tree_access(u64 cache_mapping_ways, u64 line_index);
};
```

使用u8数组实现了一个二叉树,在4、8路组映射时只用一个u8即可,全相连时则需要比较大规模的数组.

下面简略介绍一下这些函数的作用与实现:

1. `u64 tree_get_bottom(u64 cache_mapping_ways)`

   根据二叉树替换算法的规则,得到要被替换的行编号

   ```C++
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
   ```

   

2. `u64 tree_get(u64 cache_mapping_ways, u64 index)`

   获取第`index`个节点的值(0或1)

   ```C++
   u64 Cacheset::tree_get(u64 cache_mapping_ways, u64 index) {
       return u64((btree[index / 8] >> (7 - (index % 8))) & 1);
   }
   ```

3. `void tree_set(u64 cache_mapping_ways, u64 index, u8 value)`

   设置第`index`个节点的值(0或1)

   ```C++
   void Cacheset::tree_set(u64 cache_mapping_ways, u64 index, u8 value) {
       btree[index / 8] = (value << (7 - (index % 8))) + (btree[index / 8] & mask[index % 8]);
   }
   ```

   

4. `void tree_access(u64 cache_mapping_ways, u64 line_index)`

   当访问了cache中第`line_index`行时,对树进行修改

   ```C++
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
   ```

#### Random

这个比较简单,生成一个伪随机数来选择被替换的行

### 写策略

在本次试验中,采用write back和write through对于缺失率并没有影响,因此在实现中的区别仅仅是是否需要设置dirty位.对于缺失率有影响的是是否分配

## 实验结果

### 测试方法

使用`make run`即可,同时可以对cache的参数进行设定,具体参数如下:

`TRACE`:trace文件的路径

`LINE_SIZE`:cache line的大小,只实现了8、32和64

`WAYS`:组相连的路数,实现了4和8,直接连接则为1,全相连为f

`RP`:替换策略,支持LRU、RAND和BT

`WP`:写策略,包括四种写策略

另外分别可以用`CFLAGS=-DDEBUG`和`CFLAGS=-DLOG`来输出debug信息以及log,例如:

```
make run CFLASG=-DLOG TRACE=trace/astar.trace LINE_SIZE=64 WAYS=8 RP=LRU WP=write_back_allocate
```

若没有赋值则为默认值

### 测试结果

#### 1 不同的Cache布局

在固定替换策略(LRU)，固定写策略(写分配+写回)的前提下，不同的 Cache布局的结果如下:

Astar.trace:

| 块大小 | 直接连接   | 4路组相连  | 8路组相连  | 全连接     |
| :----- | :--------- | ---------- | ---------- | ---------- |
| 8      | 23.396109% | 23.275663% | 23.284836% | 23.259709% |
| 32     | 9.837716%  | 9.740801%  | 9.627534%  | 9.594032%  |
| 64     | 5.268133%  | 5.420485%  | 5.000120%  | 4.967017%  |

Bzip2.trace:

| 块大小 | 直接连接  | 4路组相连 | 8路组相连 | 全连接    |
| :----- | :-------- | --------- | --------- | --------- |
| 8      | 2.061471% | 1.217049% | 1.217049% | 1.217049% |
| 32     | 1.331095% | 0.306328% | 0.306328% | 0.306328% |
| 64     | 1.589674% | 0.154450% | 0.154450% | 0.154450% |

Mcf.trace:

| 块大小 | 直接连接  | 4路组相连 | 8路组相连 | 全连接    |
| :----- | :-------- | --------- | --------- | --------- |
| 8      | 4.944652% | 4.586173% | 4.575931% | 4.575931% |
| 32     | 2.196770% | 1.845381% | 1.824503% | 1.824503% |
| 64     | 1.459523% | 1.102423% | 1.083711% | 1.083711% |

Perlbench.trace:

| 块大小 | 直接连接  | 4路组相连 | 8路组相连 | 全连接    |
| :----- | :-------- | --------- | --------- | --------- |
| 8      | 3.666830% | 2.055214% | 1.790159% | 1.754096% |
| 32     | 2.313767% | 1.110277% | 0.822165% | 0.660175% |
| 64     | 1.894013% | 0.991248% | 0.624506% | 0.387434% |

不同trace的缺失率虽然不尽相同,但是在不同的Cache布局下都呈现了相同的变化趋势,当路数不变时,块大小越大则缺失率越低,且变化十分显著,这一结果与理论相符合,块大小越大则命中的可能性越高,而在块大小不变时,路数越多则缺失率越低,这也是与理论相符的,但是变化幅度相对较小,当路数达到最大时即是全连接,有最低的缺失率,但是性能也相对较低.

在前面替换算法中对数据的开销已经进行了简单的分析,在使用8路组相连时,每组只需三个u8(24 bits),当块大小为64B时,则堆栈占用的空间为$3B * 2^{17}B/64B = 3*2^{11}B$;而4路组相连则需要$2^{11}B$,全连接需要$2^{12}B$.

#### 2 不同的Cache替换策略

| 替换策略 | Astar      | Bzip2     | Mcf       | Perlbench |
| -------- | ---------- | --------- | --------- | --------- |
| LRU      | 23.284836% | 1.217049% | 4.575931% | 1.790159% |
| RAND     | 23.220026% | 1.217049% | 4.596415% | 1.793706% |
| BT       | 23.285633% | 1.217049% | 4.576916% | 1.784247% |

从上表中可以看出,LRU与二叉树相比缺失率低一些,这是符合理论预期的,因为二叉树是对LRU的近似.在LRU中选择的是最近最少用的一行,而二叉树是根据子节点的值来找出相对少用的一行,在一些情况下并不是最少的,而随机替换缺失率相对是最高的.但是不同替换策略之间的差距并不是很大,在Bzip2.trace中甚至完全相同,这可能与trace中访问的情况有关.

在空间开销方面,假设Cache布局为64B块大小,8路组相连.随机替换是最节省空间的,因为它不需要记录任何信息,而二叉树需要$7*2^8 B$的空间,LRU需要$3*2^{11}B$的空间.

#### 3 不同的Cache写策略

| 写策略          | Astar      | Bzip2     | Mcf        | Perlbench |
| --------------- | ---------- | --------- | ---------- | --------- |
| 写回+写分配     | 23.284836% | 1.217049% | 4.575931%  | 1.790159% |
| 写回+写不分配   | 34.498911% | 8.669933% | 11.146937% | 4.663399% |
| 写直达+写分配   | 23.284836% | 1.217049% | 4.575931%  | 1.790159% |
| 写直达+写不分配 | 34.498911% | 8.669933% | 11.146937% | 4.663399% |

从上表可以看出,写回与写直达对缺失率是没有影响的,这一点与预测相符,因为写回与写直达只影响缓存中数据在被修改时是否同步修改内存,而与缺失率无关.对缺失率有影响的是写分配与否,采用写分配的缺失率是明显降低的,这说明这几个trace中空间局部醒得到了很好的体现.