#pragma once

enum operation {
    ADD, MUL, SUB, DIV, LD, JUMP
};

static const int Num_ARS = 6;
static const int Num_MRS = 3;
static const int Num_LB = 3;
static const int Num_Reg = 32;
static const int Num_Add = 3;
static const int Num_Mult = 2;
static const int Num_Load = 2;

// latency
static const int IS_Lat = 1;
static const int ADD_Lat = 3;
static const int SUB_Lat = 3;
static const int MUL_Lat = 4;
static const int DIV_Lat = 4;
static const int DIV0_Lat = 1;
static const int LD_Lat = 3;
static const int WB_Lat = 1;

// initial values
static const int lbInit = 0;
static const int operandAvailable = -1;
static const int RegStatusEmpty = -1;
static const int operandInit = -2;

//strings
static const char* opname[5] = {"ADD", "MUL", "SUB", "DIV", "LD "};