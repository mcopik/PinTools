
#include <iostream>
#include <fstream>
#include <unordered_map>

#include "predictor.h"
//#include "TAGE.h"

#include "pin.H"

KNOB<std::string> output_file_param(KNOB_MODE_WRITEONCE, "pintool",
    "o", "inscount.out", "specify output file name");

KNOB<std::string> filter_app_name(KNOB_MODE_WRITEONCE, "pintool",
    "image", "", "name of image to filter");

struct BlockStatistics
{
    // block_size (instr_count) -> count
    std::unordered_map<int, uint64_t> histogram;
    // block_size (byte_size) -> count
    std::unordered_map<int, uint64_t> histogram_size;
};
int bb_count = 0;
uint64_t ins_count = 0;

PREDICTOR predictor;

// This function is called before every block
void count_blocks(void * stats, uint32_t count, uint32_t size)
{
    ++bb_count;
    ins_count += count;
    BlockStatistics * _stats = static_cast<BlockStatistics*>(stats);
    assert(_stats == &block_stats);
    auto it = block_stats.histogram.find(count);
    if(it == block_stats.histogram.end())
        block_stats.histogram[count] = 1;
    else
        (*it).second++;
    block_stats.histogram_size[size]++;
    //std::cout << "Block size: " << count << '\n';
}

const char * StripPath(const char * path)
{
    const char * file = strrchr(path,'/');
    if (file)
        return file+1;
    else
        return path;
}
int branch_prediction;

std::vector<int64_t> mispredicted_distances;
uint64_t branch_count = 0;
uint64_t branch_predicted_correctly = 0;
uint64_t mispredicted_not_taken = 0;

void before_branch(CONTEXT *ctx, ADDRINT inst_ptr, bool taken, ADDRINT target)
{
    ADDRINT BeforeIP = (ADDRINT)PIN_GetContextReg( ctx, REG_INST_PTR);
    branch_prediction = predictor.GetPrediction(BeforeIP);
    predictor.UpdatePredictor(BeforeIP, taken, branch_prediction, target);
    //std::cout << "Before: IP = " << std::hex << BeforeIP << " ptr " << inst_ptr << " target: " << target << " taken: " << taken << std::dec << " prediction " << branch_prediction << '\n';
    branch_count++;
    branch_predicted_correctly += taken == branch_prediction;
    mispredicted_not_taken += taken != branch_prediction && taken;
    if(taken != branch_prediction) {
        if(inst_ptr < target) {
            //mispredicted_distances.push_back(static_cast<int64_t>(target));
            //mispredicted_distances.push_back(static_cast<int64_t>(inst_ptr));
            mispredicted_distances.push_back(static_cast<int64_t>(target) - static_cast<int64_t>(inst_ptr));
        }
        else {
            //mispredicted_distances.push_back(static_cast<int64_t>(target));
            //mispredicted_distances.push_back(static_cast<int64_t>(inst_ptr));
            mispredicted_distances.push_back(static_cast<int64_t>(target) - static_cast<int64_t>(inst_ptr));
        }
    }
}

void taken(CONTEXT *ctx, ADDRINT inst_ptr, ADDRINT BranchTarget, bool BranchTaken)
{
    //ADDRINT BeforeIP = (ADDRINT)PIN_GetContextReg( ctx, REG_INST_PTR);
    //std::cout << "Taken: IP = " << std::hex << BeforeIP << " prediction: " << branch_prediction << " taken "
     //   << BranchTaken << std::dec << '\n';
}

static std::string SYS_LIB_NAMES[] = {
    "ld-linux-x86-64.so.2",
    "libc.so.6"
};

void trace(TRACE trace, void * stats)
{
    RTN rtn = TRACE_Rtn(trace);
    if(RTN_Valid(rtn)) {
        const char * name = StripPath(IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str());
        for(std::string & lib_name : SYS_LIB_NAMES) {
            if(!strcmp(name, lib_name.c_str())) {
                return;
            }
        }
    }
    //else
     //   return;
    for (BBL bb = TRACE_BblHead(trace); BBL_Valid(bb); bb = BBL_Next(bb))
    {
        BBL_InsertCall(bb, IPOINT_BEFORE, (AFUNPTR)count_blocks, IARG_PTR, stats, IARG_UINT32, BBL_NumIns(bb), IARG_UINT32, BBL_Size(bb),IARG_END);
        for(INS ins = BBL_InsHead(bb); INS_Valid(ins); ins = INS_Next(ins) ) {
            // https://github.com/jingpu/pintools/blob/master/source/tools/ToolUnitTests/branch_target_addr.cpp
            if(INS_IsBranch(ins)) {
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)before_branch,
                                   IARG_CONTEXT, IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_BRANCH_TARGET_ADDR, IARG_END);
                /*if(INS_HasFallThrough(ins))
                    INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)taken,
                                       IARG_CONTEXT, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
                else
                    INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)taken,
                                       IARG_CONTEXT, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);*/
            }
        }
    }
}

VOID Instruction(INS ins, VOID *v)
{
    if(INS_IsBranch(ins)) {
        //INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)before_branch2,
         //                  IARG_CONTEXT, IARG_END);
    }
}

// This function is called when the application exits
void finish(int32_t code, void * stats)
{
    BlockStatistics * block_stats = static_cast<BlockStatistics*>(stats);
    std::ofstream out_file(output_file_param.Value().c_str(), std::ios::app);
    //block_size block_freq succ_count mean_ilp crit_path
    for(const auto & val : block_stats.histogram)
        out_file << val.first << ' ' << val.second << ' ' << 0 << ' ' << 0 << ' ' << 0 << '\n';
    out_file.close();
    out_file.open( (output_file_param.Value() + "_size").c_str(), std::ios::app);
    for(const auto & val : block_stats.histogram_size)
        out_file << val.first << ' ' << val.second << ' ' << 0 << ' ' << 0 << ' ' << 0 << '\n';
    out_file.close();
    out_file.open((output_file_param.Value() + "_log").c_str(), std::ios::app);
    out_file << bb_count << ' ' << ins_count << '\n';
    out_file.close();
    out_file.open( (output_file_param.Value() + "_summary").c_str(), std::ios::app);
    int mispredicted = (branch_count - branch_predicted_correctly);
    out_file << ins_count << " " << branch_count << " " << branch_predicted_correctly << " " << mispredicted << " " << mispredicted_not_taken << " " << (mispredicted - mispredicted_not_taken) << '\n';
    out_file.close();
    out_file.open( (output_file_param.Value() + "_branches").c_str(), std::ios::app);
    for(auto v : mispredicted_distances)
        out_file << v << '\n';
    out_file.close();
}

int main(int argc, char ** argv)
{
    if (PIN_Init(argc, argv))
        return 1;
    TRACE_AddInstrumentFunction(trace, static_cast<void*>(&block_stats));
    PIN_AddFiniFunction(finish, static_cast<void*>(&block_stats));
    PIN_StartProgram();

    return 0;
}
