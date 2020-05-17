#pragma once
#include "const.h"

class RS {
    public:
        int time;
        bool busy;
        operation op;
        int Vj, Vk;
        int Qj, Qk;
        int lat;
        int res;
        int issue_lat;
        int wb_lat;
        int inst_id;
        bool get_res;

        RS();
        RS(operation op, int operand);
        void reset();
};