# Tomasulo

## 算法实现

在实现本算法过程中参考了一部分[开源的实现](https://github.com/Milleraj66/ECE585_TomasuloAlgorithm.git)，其中并没有实现`LD`指令，并且没有对运算器数量的限制，主要是参考了其中`Reservation Station`的实现，下面介绍一下我的实现。

首先我实现了几个类来模拟各种`Reservation Station`、`Load Buffer`、`Register`等部件，主要有如下部分：

```C++
//rs.h
class RS {
    public:
        int time;
        bool busy;
        operation op;
        int Vj, Vk;
        int Qj, Qk;
        int lat;//ex阶段时间
        int res;//运算结果
        int issue_lat;//is阶段时间
        int wb_lat;//wb阶段时间
        int inst_id;//指令的id
        bool get_res;//是否已得到结果

        RS();
        RS(operation op, int operand);
        void reset();
};
//lb.h
class LB {
    public:
        bool busy;
        int address;
        int lat;
        int issue_lat;
        int wb_lat;
        int inst_id;
        bool get_res;

        LB();
        LB(int address);
        void reset();
};
//rrs.h
class RRS {
    public:
        bool busy;
        int Q;

        RRS();
        RRS(int Q);
};
//inst.h
class Inst {
    public:
        operation op;
        int rd, rs, rt;
        int isClock, exClockStart, exClockEnd, wbClock;

        Inst();
        Inst(operation op, int rd, int rs, int rt);
};
```

接下来是`main.cpp`部分

```C++
int main(int argc, char* argv[]) {
  	//首先从指定nel文件中读取指令
    getInst(argv[1]);

  	//获取输出文件名
    string infile = string(argv[1]);
    smatch m;
    regex e("TestCase/([(0-9)(a-z)(A-Z)(\.)]*)\.nel");
    string outfile;
    char* out = new char[100];

    if(regex_search(infile, m, e)) {
        outfile = "Log/2017011455_" + m.str(1) + ".log";
        strcpy(out, outfile.c_str());
    } else {
        printf("match %s fail\n", argv[1]);
        return 0;
    }

    FILE* fout = fopen(out, "w");
    fclose(fout);
  	
  	//打印初始状态
    Log(out);

  	//流水线开始执行，并打印每一个周期的状态
    while(instcnt != InstList.size()) {
        Clock++;

        writeback();
        issue();
        execute();

        Log(out);
    }
    
    printf("Used %d Cycles\n", Clock);

    return 0;
}
```

其中最核心的三个函数就是`while`循环中的`issue()`，`execute()`和`writeback()`，分别实现了相应阶段的执行过程，其中`issue()`的主要工作是读取一条指令，并判断相应保留站中是否空闲，如果找到则将指令发射，否则等待到下一周期。`execute()`主要是遍历每一个保留站，并判断相应指令目前是否操作数已经可以得到且有空闲的运算器来执行该指令，如果有则该保留站占有运算器直到执行结束。运算器的数量小于相应保留站数量，因此我使用一个数组来维护，如果一个保留站占用了一个运算器，则在相应数组中中保存其id，表示已被占用，当执行结束后再释放。`writeback()`就是遍历每一个保留站找到是否有指令已经得到结果可以写回。我的实现在example中与示例有一点不同，就是在第五周期时，按照示例第三条LD指令还没有开始执行，但是实际上第四周期时第一条LD指令已经完成，因此第五周期正常应该已经开始执行第三条LD指令，我在实现过程中考虑到了示例中的不合理性，并进行了修改。另外，我的实现中并没有实现`JUMP`指令与分支预测。

## 实验结果

使用`make run`即可运行，并且可以设置变量`$(TESTCASE)`来指定`TestCase`文件夹下的测试文件名（默认为0.basic.nel），相应结果保存在`Log`文件夹中，例如：

```shell
[maxinyu@14:48:12]~/Desktop/mdlife/Courses/Third_Year/CA/CALab/Tomasulo$ make run TESTCASE=1.basic.nel
make
g++  -o main.o -c main.cpp
g++  -o rs.o -c rs.cpp
g++  -o rrs.o -c rrs.cpp
g++  -o inst.o -c inst.cpp
g++  -o lb.o -c lb.cpp
g++  -o tomasulo main.o rs.o rrs.o inst.o lb.o
./tomasulo TestCase/1.basic.nel
Used 59 Cycles
make clean
rm *.o tomasulo
```

则输出文件为`Log/2017011455_1.basic.log`，另外Terminal中还会给出总共的时间（周期数）

## 与记分牌算法的差异性

1. Tomasulo方法通过寄存器换名过程可以消除WAR和WAW竞争。记分牌方法能检测WAR和WAW竞争，一旦检测到存在WAR和WAW竞争，通过插入停顿周期来解决这一竞争。所以，记分牌方法不能消除WAR和WAW竞争。

2. 检测竞争和控制指令执行方式的不同：

   Tomasulo方法检测竞争和控制指令执行两方面功能是通过分布在每一功能单元的保留站来进行的，因此Tomasulo方法是一种分布式方法。

   记分牌方法的上述功能是通过统一的记分牌来实现的，因此记分牌方法是一种集中式方法。

3. 写结果的方法不同：

   Tomasulo方法通过CDB直接将功能单元输出的结果送往需要该结果的所有保留站，而不必经过寄存器这一中间环节。

   记分牌方法是将结果写入FP寄存器, 因而可能造成等待这一结果的指令都出现停顿现象，之后，所有相关指令的功能单元在读FP 寄存器时又可能出现竞争现象。

   Tomasulo方法的流水级功能与记分牌比较：

   Tomasulo方法中无检查WAW和WAR竞争功能，因为在指令发射过程中，由issue logic结合保留站完成了register operands的改名过程，即消除了这两种竞争。

   CDB起到广播结果的作用，不必通过register file直接结果送到所有需要该结果的保留站和buffers.

   Load和Store buffers相当于基本功能单元