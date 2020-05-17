#include <stdexcept>
#include<cstdio>
#include <fstream>
#include <vector>
#include <cstring>
#include <regex>
#include "const.h"
#include "inst.h"
#include "rs.h"
#include "rrs.h"
#include "lb.h"

#include <iostream>

using namespace std;

int Clock = 0;
int currissue = 0;
int instcnt = 0;

RS Ars1(ADD, operandInit), Ars2(ADD, operandInit), Ars3(ADD, operandInit), Ars4(ADD, operandInit), Ars5(ADD, operandInit), Ars6(ADD, operandInit);
RS Mrs1(MUL, operandInit), Mrs2(MUL, operandInit), Mrs3(MUL, operandInit);

vector<Inst> InstList;
vector<RS> RSList = {
    Ars1, Ars2, Ars3,
    Ars4, Ars5, Ars6,
    Mrs1, Mrs2, Mrs3,
};
vector<LB> LBList(Num_LB, LB(lbInit));
vector<RRS> RRSList(Num_Reg, RRS(RegStatusEmpty));
vector<int> Register(Num_Reg, 0);
vector<int> Add(Num_Add, -1);
vector<int> Mult(Num_Mult, -1);
vector<int> Load(Num_Load, -1);

void getInst(const char* filename);

void issue();
void execute();
void writeback();

void print();
void Log(const char* filename);

int main(int argc, char* argv[]) {
    #ifdef DEBUG
    Inst I0(LD, 1, 0x2, 0);
    Inst I1(LD, 2, 0x1, 0);
    Inst I2(LD, 3, 0xffffffff, 0);
    Inst I3(SUB, 1, 1, 2);
    Inst I4(DIV, 4, 3, 1);

    InstList.push_back(I0);
    InstList.push_back(I1);
    InstList.push_back(I2);
    InstList.push_back(I3);
    InstList.push_back(I4);
    print();
    #else
    getInst(argv[1]);
    #endif

    // string fuck = "80000000";
    // int fk = stoi(fuck, 0, 16);
    // int fk = 0x7fffffff;
    // fk += 0x80000000;
    // printf("yo sup bro %d\n", fk);
    // return 0;

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
    Log(out);

    while(instcnt != InstList.size()) {
        Clock++;

        writeback();
        issue();
        execute();

        #ifdef DEBUG
        print();
        #else
        Log(out);
        #endif
    }
    
    printf("Used %d Cycles\n", Clock);

    return 0;
}

void getInst(const char* filename) {
    string buff;
    ifstream fin(filename);

    if(fin.is_open()) {
        while(!fin.eof()) {
            getline(fin, buff);
            regex p0("LD,R([0-9]+),0x([(0-9)(a-z)(A-Z)]+)");
            regex p1("([A-Z]+),R([0-9]+),R([0-9]+),R([0-9]+)");
            smatch m;
            if(regex_search(buff, m, p0)) {
                // cout << "fuck" << m.str(2) << "\n";
                int rd = stoi(m.str(1));
                int rs;
                try {
                    rs = stoi(m.str(2), 0, 16);
                } catch(out_of_range&) {
                    // rs = 
                    int h = stoi(m.str(2).substr(0, 1), 0, 16);
                    // printf("h: %x\n", h);
                    string ns = to_string(h - 8) + m.str(2).substr(1, 7);
                    // cout << "ns: " << ns << "\n";
                    rs = stoi(ns, 0, 16) + 0x80000000;
                    // printf("out of range: %d\n", rs);
                }
                Inst I(LD, rd, rs, 0);
                InstList.push_back(I);
            } else if(regex_search(buff, m, p1)) {
                operation op;
                if(m.str(1) == "ADD") {
                    op = ADD;
                } else if(m.str(1) == "MUL") {
                    op = MUL;
                } else if(m.str(1) == "SUB") {
                    op = SUB;
                } else if(m.str(1) == "DIV") {
                    op = DIV;
                } else {
                    break;
                }
                int rd = stoi(m.str(2));
                int rs = stoi(m.str(3));
                int rt = stoi(m.str(4));
                Inst I(op, rd, rs, rt);
                InstList.push_back(I);
            }
        }
    } else {
        printf("file %s open failed\n", filename);
    }
}

void issue() {
    if(currissue == InstList.size())
        return;
    
    operation op = InstList[currissue].op;
    int id = -1;
    bool getRS = false;

    switch(op) {
        case ADD:
            for(int i = 0; i < Num_ARS; i++) {
                if(!RSList[i].busy) {
                    RSList[i].op = ADD;
                    id = i;
                    getRS = true;
                    break;
                }
            }
            break;
        case MUL:
            for(int i = Num_ARS; i < Num_ARS + Num_MRS; i++) {
                if(!RSList[i].busy) {
                    RSList[i].op = MUL;
                    id = i;
                    getRS = true;
                    break;
                }
            }
            break;
        case SUB:
            for(int i = 0; i < Num_ARS; i++) {
                if(!RSList[i].busy) {
                    RSList[i].op = SUB;
                    id = i;
                    getRS = true;
                    break;
                }
            }
            break;
        case DIV:
            for(int i = Num_ARS; i < Num_ARS + Num_MRS; i++) {
                if(!RSList[i].busy) {
                    RSList[i].op = DIV;
                    id = i;
                    getRS = true;
                    break;
                }
            }
            break;
        case LD:
            for(int i = 0; i < Num_LB; i++) {
                if(!LBList[i].busy) {
                    id = i;
                    getRS = true;
                    break;
                }
            }
            break;
        case JUMP:
            printf("JUMP not implemented!\n");
            break;
        default:
            printf("no such instruction!\n");
    }

    if(!getRS) {
        return;
    }

    if(op == LD) {
        LBList[id].address = InstList[currissue].rs;
        LBList[id].busy = true;
        LBList[id].issue_lat = 0;
        LBList[id].inst_id = currissue;

        InstList[currissue].isClock = Clock;

        // RRSList[InstList[currissue].rd].busy = true;
        RRSList[InstList[currissue].rd].Q = id + Num_ARS + Num_MRS;
    } else {
        if(RRSList[InstList[currissue].rs].Q == RegStatusEmpty) {
            RSList[id].Vj = Register[InstList[currissue].rs];
            RSList[id].Qj = operandAvailable;
        } else {
            RSList[id].Qj = RRSList[InstList[currissue].rs].Q;
            // RRSList[InstList[currissue].rd].busy = false;
        }

        if(RRSList[InstList[currissue].rt].Q == RegStatusEmpty) {
            RSList[id].Vk = Register[InstList[currissue].rt];
            RSList[id].Qk = operandAvailable;
        } else {
            RSList[id].Qk = RRSList[InstList[currissue].rt].Q;
        }
        
        RSList[id].busy = true;
        RSList[id].issue_lat = 0;
        RSList[id].inst_id = currissue;

        InstList[currissue].isClock = Clock;

        RRSList[InstList[currissue].rd].Q = id;
    }
    

    currissue++;
}

void execute() {
    for(int i = 0; i < LBList.size(); i++) {
        if(LBList[i].busy) {
            if(LBList[i].issue_lat >= IS_Lat) {
                int index = -1;
                for(int j = 0; j < Num_Load; j++) {
                    if(Load[j] == i) {
                        index = j;
                        break;
                    }
                }

                if(index == -1) {
                    for(int j = 0; j < Num_Load; j++) {
                        if(Load[j] == -1) {
                            index = j;
                            Load[j] = i;
                            break;
                        }
                    }
                }

                if(index >= 0) {
                    if(InstList[LBList[i].inst_id].exClockStart == 0) {
                        InstList[LBList[i].inst_id].exClockStart = Clock;
                    }

                    LBList[i].lat++;

                    if(LBList[i].lat == LD_Lat) {
                        LBList[i].get_res = true;
                        LBList[i].issue_lat = 0;
                        InstList[LBList[i].inst_id].exClockEnd = Clock;
                        // Load[index] = -1;
                    }
                }
                
            } else {
                LBList[i].issue_lat++;
            }
        }
    }

    for(int i = 0; i < Num_LB; i++) {
        int index = -1;
        for(int j = 0; j < Num_Load; j++) {
            if(Load[j] == i) {
                index = j;
                break;
            }
        }
        if(LBList[i].lat == LD_Lat) {
            LBList[i].lat = 0;
            Load[index] = -1;
        }
    }


    for(int i = 0; i < RSList.size(); i++) {
        if(RSList[i].busy) {
            if(RSList[i].issue_lat >= IS_Lat) {
                if(RSList[i].Qj == operandAvailable && RSList[i].Qk == operandAvailable) {
                    int index = -1;
                    if(RSList[i].op == ADD || RSList[i].op == SUB) {
                        for(int j = 0; j < Num_Add; j++) {
                            if(Add[j] == i) {
                                index = j;
                                break;
                            }
                        }

                        if(index == -1) {
                            for(int j = 0; j < Num_Add; j++) {
                                if(Add[j] == -1) {
                                    Add[j] = i;
                                    index = j;
                                    break;
                                }
                            }
                        }

                        if(index == -1) {
                            continue;
                        }
                    } else if(RSList[i].op == MUL || RSList[i].op == DIV) {
                        for(int j = 0; j < Num_Mult; j++) {
                            if(Mult[j] == i) {
                                index = j;
                                break;
                            }
                        }

                        if(index == -1) {
                            for(int j = 0; j < Num_Mult; j++) {
                                if(Mult[j] == -1) {
                                    Mult[j] = i;
                                    index = j;
                                    break;
                                }
                            }
                        }

                        if(index == -1) {
                            continue;
                        }
                    } else {
                        continue;
                    }

                    if(InstList[RSList[i].inst_id].exClockStart == 0) {
                        InstList[RSList[i].inst_id].exClockStart = Clock;
                    }

                    RSList[i].lat++;

                    switch(RSList[i].op) {
                        case ADD:
                            if(RSList[i].lat == ADD_Lat) {
                                RSList[i].res = RSList[i].Vj + RSList[i].Vk;
                                RSList[i].get_res = true;
                                RSList[i].lat = 0;
                                RSList[i].issue_lat = 0;
                                InstList[RSList[i].inst_id].exClockEnd = Clock;
                                Add[index] = -1;
                            }
                            break;
                        case MUL:
                            if(RSList[i].lat == MUL_Lat) {
                                RSList[i].res = RSList[i].Vj * RSList[i].Vk;
                                RSList[i].get_res = true;
                                RSList[i].lat = 0;
                                RSList[i].issue_lat = 0;
                                InstList[RSList[i].inst_id].exClockEnd = Clock;
                                Mult[index] = -1;
                            }
                            break;
                        case SUB:
                            if(RSList[i].lat == SUB_Lat) {
                                RSList[i].res = RSList[i].Vj - RSList[i].Vk;
                                RSList[i].get_res = true;
                                RSList[i].lat = 0;
                                RSList[i].issue_lat = 0;
                                InstList[RSList[i].inst_id].exClockEnd = Clock;
                                Add[index] = -1;
                            }
                            break;
                        case DIV:
                            if(RSList[i].Vk == 0 && RSList[i].lat == DIV0_Lat) {
                                RSList[i].res = RSList[i].Vj;
                                RSList[i].get_res = true;
                                RSList[i].lat = 0;
                                RSList[i].issue_lat = 0;
                                InstList[RSList[i].inst_id].exClockEnd = Clock;
                                Mult[index] = -1;
                            } else if(RSList[i].lat == DIV_Lat) {
                                RSList[i].res = RSList[i].Vj / RSList[i].Vk;
                                RSList[i].get_res = true;
                                RSList[i].lat = 0;
                                RSList[i].issue_lat = 0;
                                InstList[RSList[i].inst_id].exClockEnd = Clock;
                                Mult[index] = -1;
                            }
                            break;
                        default:
                            printf("execute: no such operation!\n");
                    }
                }
            } else {
                RSList[i].issue_lat++;
            }
        }
    }
}

void writeback() {
    for(int i = 0; i < LBList.size(); i++) {
        if(LBList[i].get_res) {
            if(LBList[i].wb_lat == WB_Lat) {
                if(InstList[LBList[i].inst_id].wbClock == 0) {
                    InstList[LBList[i].inst_id].wbClock = Clock - 1;
                }
                for(int j = 0; j < Num_Reg; j++) {
                    if(RRSList[j].Q == i + Num_ARS + Num_MRS) {
                        Register[j] = LBList[i].address;
                        RRSList[j].Q = RegStatusEmpty;
                    }
                }
                for(int j = 0; j < RSList.size(); j++) {
                    if(RSList[j].Qj == i + Num_ARS + Num_MRS) {
                        RSList[j].Vj = LBList[i].address;
                        RSList[j].Qj = operandAvailable;
                    }
                    if(RSList[j].Qk == i + Num_ARS + Num_MRS) {
                        RSList[j].Vk = LBList[i].address;
                        RSList[j].Qk = operandAvailable;
                    }
                }
                LBList[i].reset();
                instcnt++;
            } else {
                LBList[i].wb_lat++;
            }
        }
    }

    for(int i = 0; i < RSList.size(); i++) {
        if(RSList[i].get_res) {
            if(RSList[i].wb_lat == WB_Lat) {
                if(InstList[RSList[i].inst_id].wbClock == 0) {
                    InstList[RSList[i].inst_id].wbClock = Clock - 1;
                }
                for(int j = 0; j < Num_Reg; j++) {
                    if(RRSList[j].Q == i) {
                        Register[j] = RSList[i].res;
                        RRSList[j].Q = RegStatusEmpty;
                        // break;
                    }
                }
                for(int j = 0; j < RSList.size(); j++) {
                    if(RSList[j].Qj == i) {
                        RSList[j].Vj = RSList[i].res;
                        RSList[j].Qj = operandAvailable;
                    }
                    if(RSList[j].Qk == i) {
                        RSList[j].Vk = RSList[i].res;
                        RSList[j].Qk = operandAvailable;
                    }
                }
                RSList[i].reset();
                instcnt++;
            } else {
                RSList[i].wb_lat++;
            }
        }
    }
}

void print() {
    printf("System Clock: %d\n", Clock);
    //Instructions
    printf("---------------------------------------------------------------------\nInstruction:\n");
    printf("                 ID      IS      EX      WB\n");
    for(int i = 0; i < InstList.size(); i++) {
        printf("%s,R%d,", opname[int(InstList[i].op)], InstList[i].rd);
        if(InstList[i].op == LD) {
            printf("0x%-8x", InstList[i].rs);
        } else {
            printf("R%d,R%-5d", InstList[i].rs, InstList[i].rt);
        }
        printf(" %-8d%-8d%-8d%d\n", i, InstList[i].isClock, InstList[i].exClockEnd, InstList[i].wbClock);
    }
    //Reservation Station
    printf("---------------------------------------------------------------------\nReservation Station\n");
    printf("      Busy    Vj          Vk          Qj        Qk\n");
    for(int i = 0; i < Num_ARS; i++) {
        printf("Ars%d  %s", i + 1, RSList[i].busy ? "Yes" : "No ");
        if(RSList[i].busy) {
            printf("     0x%-10x0x%-10x%-10d%-10d\n", RSList[i].Vj, RSList[i].Vk, RSList[i].Qj, RSList[i].Qk);
        } else {
            printf("\n");
        }
    }
    for(int i = Num_ARS; i < Num_ARS + Num_MRS; i++) {
        printf("Mrs%d  %s", i - Num_ARS + 1, RSList[i].busy ? "Yes" : "No ");
        if(RSList[i].busy) {
            printf("     0x%-10x0x%-10x%-10d%-10d\n", RSList[i].Vj, RSList[i].Vk, RSList[i].Qj, RSList[i].Qk);
        } else {
            printf("\n");
        }
    }
    //Load Buffer
    printf("---------------------------------------------------------------------\nLoad Buffer:\n");
    printf("     Busy     Address\n");
    for(int i = 0; i < LBList.size(); i++) {
        printf("LB%d  %s", i + 1, LBList[i].busy ? "Yes" : "No ");
        if(LBList[i].busy) {
            printf("      0x%x\n",  LBList[i].address);
        } else {
            printf("\n");
        }
    }
    //Register
    printf("---------------------------------------------------------------------\nRegister:\n");
    for(int i = 0; i < Register.size(); i++) {
        if(RRSList[i].Q != RegStatusEmpty) {
            if(RRSList[i].Q < Num_ARS) {
                printf("R%d: Ars%d ", i, RRSList[i].Q + 1);
            } else if(RRSList[i].Q < Num_ARS + Num_MRS) {
                printf("R%d: Mrs%d ", i, RRSList[i].Q - Num_ARS + 1);
            } else if(RRSList[i].Q < Num_ARS + Num_MRS + Num_LB) {
                printf("R%d: LB%d ", i, RRSList[i].Q - Num_ARS - Num_MRS + 1);
            }
            
        } else {
            printf("R%d: %d ", i, Register[i]);
        }
        if(i % 8 == 7) {
            printf("\n");
        }
    }
    // printf("\n");
    //Load
    // printf("Load:\n");
    // for(int i = 0; i < Num_Load; i++) {
    //     printf("Load%d: %d\n", i, Load[i]);
    // }
    // printf("currissue: %d\n", currissue);
    // for(int i = 0; i < Num_Reg; i++) {
    //     printf("RRS[%d].Q: %d\n", i, RRSList[i].Q);
    // }
    printf("************************************************************************\n");
}

void Log(const char* filename) {
    FILE* fout = fopen(filename, "a");
    if(fout == NULL) {
        printf("Log: file %s open failed!\n", filename);
        return;
    }

    fprintf(fout, "System Clock: %d\n", Clock);
    //Instructions
    fprintf(fout, "---------------------------------------------------------------------\nInstruction:\n");
    fprintf(fout, "                    ID      IS      EX      WB\n");
    for(int i = 0; i < InstList.size(); i++) {
        fprintf(fout, "%s,R%-2d,", opname[int(InstList[i].op)], InstList[i].rd);
        if(InstList[i].op == LD) {
            fprintf(fout, "0x%-8x", InstList[i].rs);
        } else {
            fprintf(fout, "R%-2d,R%-5d", InstList[i].rs, InstList[i].rt);
        }
        fprintf(fout, "  %-8d%-8d%-8d%d\n", i, InstList[i].isClock, InstList[i].exClockEnd, InstList[i].wbClock);
    }
    //Reservation Station
    fprintf(fout, "---------------------------------------------------------------------\nReservation Station\n");
    fprintf(fout, "      Busy    Vj          Vk          Qj        Qk\n");
    for(int i = 0; i < Num_ARS; i++) {
        fprintf(fout, "Ars%d  %s", i + 1, RSList[i].busy ? "Yes" : "No ");
        if(RSList[i].busy) {
            fprintf(fout, "     0x%-10x0x%-10x%-10d%-10d\n", RSList[i].Vj, RSList[i].Vk, RSList[i].Qj, RSList[i].Qk);
        } else {
            fprintf(fout, "\n");
        }
    }
    for(int i = Num_ARS; i < Num_ARS + Num_MRS; i++) {
        fprintf(fout, "Mrs%d  %s", i - Num_ARS + 1, RSList[i].busy ? "Yes" : "No ");
        if(RSList[i].busy) {
            fprintf(fout, "     0x%-10x0x%-10x%-10d%-10d\n", RSList[i].Vj, RSList[i].Vk, RSList[i].Qj, RSList[i].Qk);
        } else {
            fprintf(fout, "\n");
        }
    }
    //Load Buffer
    fprintf(fout, "---------------------------------------------------------------------\nLoad Buffer:\n");
    fprintf(fout, "     Busy     Address\n");
    for(int i = 0; i < LBList.size(); i++) {
        fprintf(fout, "LB%d  %s", i + 1, LBList[i].busy ? "Yes" : "No ");
        if(LBList[i].busy) {
            fprintf(fout, "      0x%x\n",  LBList[i].address);
        } else {
            fprintf(fout, "\n");
        }
    }
    //Register
    fprintf(fout, "---------------------------------------------------------------------\nRegister:\n");
    for(int i = 0; i < Register.size(); i++) {
        if(RRSList[i].Q != RegStatusEmpty) {
            if(RRSList[i].Q < Num_ARS) {
                fprintf(fout, "R%d: Ars%d ", i, RRSList[i].Q + 1);
            } else if(RRSList[i].Q < Num_ARS + Num_MRS) {
                fprintf(fout, "R%d: Mrs%d ", i, RRSList[i].Q - Num_ARS + 1);
            } else if(RRSList[i].Q < Num_ARS + Num_MRS + Num_LB) {
                fprintf(fout, "R%d: LB%d ", i, RRSList[i].Q - Num_ARS - Num_MRS + 1);
            }
            
        } else {
            fprintf(fout, "R%d: %d ", i, Register[i]);
        }
        if(i % 8 == 7) {
            fprintf(fout, "\n");
        }
    }
    fprintf(fout, "************************************************************************\n");
    fclose(fout);
}