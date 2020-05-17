#pragma once
#include "const.h"

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