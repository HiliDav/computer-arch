/* 046267 Computer Architecture - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <vector>

using namespace std;

#define REG_CNT 32

typedef struct {
    int op_latency;
    int depth;          //in cycles
    int src1_dep_inst;  // the instruction idx that the reg in src1 depends on (RAW)
    int src2_dep_inst;     // the instruction idx that the reg in src2 depends on (RAW)
} analized_inst;


class CtxData {
    public:
    vector<analized_inst> analized_inst_list;
    unsigned int reg_usage[REG_CNT]; // reg_usage[i] is the idx of the last instruction that wrote to reg i.
    int max_depth;

    CtxData(unsigned int numOfInsts) {
        analized_inst_list = vector<analized_inst>(numOfInsts);
        max_depth = 0;
        for (int i = 0; i < REG_CNT; i++) {
            reg_usage[i] = -1;
        }
    }

};


CtxData *my_ctx;


ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    
    //create new data class
    my_ctx = new CtxData(numOfInsts);
    InstInfo current_inst;

    //for each instruction, build the data structure
    for (int i = 0; i < (int)numOfInsts; i++) {
        current_inst = progTrace[i];
        analized_inst &my_ana_inst = my_ctx->analized_inst_list[i];

        // set operation latency, dependency of src1 and src2 regs
        my_ana_inst.op_latency = opsLatency[current_inst.opcode];
        my_ana_inst.src1_dep_inst = my_ctx->reg_usage[current_inst.src1Idx];
        my_ana_inst.src2_dep_inst = my_ctx->reg_usage[current_inst.src2Idx];

        //set depth of the instruction
        if (my_ana_inst.src1_dep_inst == -1) {
            if (my_ana_inst.src2_dep_inst == -1) {
                //there is no dependencies
                my_ana_inst.depth = 0;
            }
            else {
                int src2_depth = my_ctx->analized_inst_list[my_ana_inst.src2_dep_inst].depth;
                int src2_latency = my_ctx->analized_inst_list[my_ana_inst.src2_dep_inst].op_latency;
                my_ana_inst.depth = src2_depth + src2_latency;
            }
        }
        else {
            if (my_ana_inst.src2_dep_inst == -1) {
                int src1_depth = my_ctx->analized_inst_list[my_ana_inst.src1_dep_inst].depth;
                int src1_latency = my_ctx->analized_inst_list[my_ana_inst.src1_dep_inst].op_latency;
                my_ana_inst.depth = src1_depth + src1_latency;            
            }
            else {
                int src1_depth = my_ctx->analized_inst_list[my_ana_inst.src1_dep_inst].depth;
                int src1_latency = my_ctx->analized_inst_list[my_ana_inst.src1_dep_inst].op_latency;
                int depth1 = src1_depth + src1_latency;

                int src2_depth = my_ctx->analized_inst_list[my_ana_inst.src2_dep_inst].depth;
                int src2_latency = my_ctx->analized_inst_list[my_ana_inst.src2_dep_inst].op_latency;
                int depth2 = src2_depth + src2_latency;

                my_ana_inst.depth = max(depth1, depth2);
            }
        }

        //update register usage
        my_ctx->reg_usage[current_inst.dstIdx] = i;

        //update max depth
        int current_inst_depth = my_ana_inst.depth + my_ana_inst.op_latency;
        my_ctx->max_depth = (current_inst_depth > my_ctx->max_depth) ? current_inst_depth : my_ctx->max_depth;        
    }

    return my_ctx;
}

void freeProgCtx(ProgCtx ctx) {
    CtxData *my_ctx = static_cast<CtxData*> (ctx);
    delete my_ctx;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    CtxData *my_ctx = static_cast<CtxData*> (ctx);
    return my_ctx->analized_inst_list[theInst].depth;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    CtxData *my_ctx = static_cast<CtxData*> (ctx);
    *src1DepInst = my_ctx->analized_inst_list[theInst].src1_dep_inst;
    *src2DepInst = my_ctx->analized_inst_list[theInst].src2_dep_inst;
    return 0;
}

int getProgDepth(ProgCtx ctx) {
    CtxData *my_ctx = static_cast<CtxData*> (ctx);
    return my_ctx->max_depth;
}


