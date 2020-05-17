#include "lb.h"

LB::LB() {
    busy = false;
    address = 0;
    lat = 0;
    issue_lat = 0;
    wb_lat = 0;
    inst_id = -1;
    get_res = false;
}

LB::LB(int address) {
    busy = false;
    this->address = address;
    lat = 0;
    issue_lat = 0;
    wb_lat = 0;
    inst_id = -1;
    get_res = false;
}

void LB::reset() {
    busy = false;
    address = 0;
    lat = 0;
    issue_lat = 0;
    wb_lat = 0;
    inst_id = -1;
    get_res = false;
}