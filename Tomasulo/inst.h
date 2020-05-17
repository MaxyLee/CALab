#pragma once
#include "const.h"

class Inst {
    public:
        operation op;
        int rd, rs, rt;
        int isClock, exClockStart, exClockEnd, wbClock;

        Inst();
        Inst(operation op, int rd, int rs, int rt);
};