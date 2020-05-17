#include "rs.h"

RS::RS() {
    time = -1;
    busy = false;
    op = operation(0);
    Vj = 0;
    Vk = 0;
    Qj = 1;
    Qk = 1;
    lat = 0;
    res = 0;
    issue_lat = 0;
    wb_lat = 0;
    inst_id = -1;
    get_res = false;
}

RS::RS(operation op, int operand) {
    time = -1;
    busy = false;
    this->op = op;
    Vj = 0;
    Vk = 0;
    Qj = operand;
    Qk = operand;
    lat = 0;
    res = 0;
    issue_lat = 0;
    wb_lat = 0;
    inst_id = -1;
    get_res = false;
}

void RS::reset() {
    busy = false;
    Vj = 0;
    Vk = 0;
    Qj = 1;
    Qk = 1;
    wb_lat = 0;
    get_res = false;
}