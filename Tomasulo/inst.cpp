#include "inst.h"

Inst::Inst() {
    op = operation(0);
    rd = 0;
    rs = 0;
    rt = 0;
    isClock = 0;
    exClockStart = 0;
    exClockEnd = 0;
    wbClock = 0;
}

Inst::Inst(operation op, int rd, int rs, int rt) {
    this->op = op;
    this->rd = rd;
    this->rs = rs;
    this->rt = rt;
    isClock = 0;
    exClockStart = 0;
    exClockEnd = 0;
    wbClock = 0;
}