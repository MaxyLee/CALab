#include "rrs.h"

RRS::RRS() {
    busy = false;
    Q = 0;
}

RRS::RRS(int Q) {
    busy = false;
    this->Q = Q;
}