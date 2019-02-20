
#include <iostream>
#include <fstream>
#include <unordered_map>

// For some reason, tuple canno 
//#include <tuple>

#include "predictor.h"

#include "pin.H"

struct BlockStatistics
{
    std::unordered_map<int, uint64_t> histogram;
    std::unordered_map<int, uint64_t> histogram_size;
};
int bb_count = 0;
uint64_t ins_count = 0;

KNOB<std::string> output_file_param(KNOB_MODE_WRITEONCE, "pintool",
    "o", "inscount.out", "specify output file name");

KNOB<std::string> filter_app_name(KNOB_MODE_WRITEONCE, "pintool",
    "image", "", "name of image to filter");

// This function is called before every block
//void count_blocks(void * stats, uint32_t count, uint32_t size)
//{
//    ++bb_count;
//    ins_count += count;
//    BlockStatistics * _stats = static_cast<BlockStatistics*>(stats);
//    assert(_stats == &block_stats);
//    auto it = block_stats.histogram.find(count);
//    if(it == block_stats.histogram.end())
//        block_stats.histogram[count] = 1;
//    else
//        (*it).second++;
//    block_stats.histogram_size[size]++;
//    //std::cout << "Block size: " << count << '\n';
//}

static std::string SYS_LIB_NAMES[] = {
    "ld-linux-x86-64.so.2",
    "libc.so.6"
};

const char * StripPath(const char * path)
{
    const char * file = strrchr(path,'/');
    if (file)
        return file+1;
    else
        return path;
}

struct BranchStatistics
{
    // TAGE predictor
    PREDICTOR predictor;
    // first field - counts, second fields - count mispredicted
    std::vector<int64_t> mispredicted_distances;
    std::unordered_map<ADDRINT, std::pair<int32_t, int32_t>> branches;
    uint64_t branch_count = 0;
    uint64_t branch_predicted_correctly = 0;
    // mispredictions where branchs was indeed taken
    uint64_t mispredicted_not_taken = 0;
};
// segfaults when allocated in main
BranchStatistics branch_statistics;

void before_branch(CONTEXT *ctx, ADDRINT inst_ptr, bool taken,
        ADDRINT target, void * _stats)
{
    BranchStatistics * stats = static_cast<BranchStatistics*>(_stats);
    // Instruction pointer
    ADDRINT BeforeIP = (ADDRINT)PIN_GetContextReg(ctx, REG_INST_PTR);
    auto branch_prediction = stats->predictor.GetPrediction(BeforeIP);
    // Update predictor: was it taken, what we predict and what is the branch target
    stats->predictor.UpdatePredictor(BeforeIP, taken, branch_prediction, target);
    stats->branch_count++;
    stats->branch_predicted_correctly += taken == branch_prediction;
    stats->mispredicted_not_taken += taken != branch_prediction && taken;
    // did tage mispredict?
    bool mispredicted = taken != branch_prediction;
    auto it = stats->branches.find(BeforeIP);
    if(it != stats->branches.end()) {
        it->second.first += 1;
        it->second.second += mispredicted;
    } else {
        stats->branches[ BeforeIP ] = std::make_pair(1, mispredicted);
    }
    if(taken != branch_prediction) {
        if(inst_ptr < target) {
            stats->mispredicted_distances.push_back(static_cast<int64_t>(target) - static_cast<int64_t>(inst_ptr));
        }
        else {
            stats->mispredicted_distances.push_back(static_cast<int64_t>(target) - static_cast<int64_t>(inst_ptr));
        }
    }
}

void taken(CONTEXT *ctx, ADDRINT inst_ptr, ADDRINT BranchTarget, bool BranchTaken)
{
    //ADDRINT BeforeIP = (ADDRINT)PIN_GetContextReg( ctx, REG_INST_PTR);
    //std::cout << "Taken: IP = " << std::hex << BeforeIP << " prediction: " << branch_prediction << " taken "
     //   << BranchTaken << std::dec << '\n';
}

void trace(TRACE trace, void * stats)
{
    RTN rtn = TRACE_Rtn(trace);
    // Exclude syslib calls
    if(RTN_Valid(rtn)) {
        const char * name = StripPath(IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str());
        for(std::string & lib_name : SYS_LIB_NAMES) {
            if(!strcmp(name, lib_name.c_str())) {
                return;
            }
        }
    }
    for (BBL bb = TRACE_BblHead(trace); BBL_Valid(bb); bb = BBL_Next(bb))
    {
        //BBL_InsertCall(bb, IPOINT_BEFORE,
        //        (AFUNPTR)count_blocks, IARG_PTR, stats,
        //        IARG_UINT32, BBL_NumIns(bb), IARG_UINT32,
        //        BBL_Size(bb),IARG_END
        //    );
        // For each branch instruction, insert a call to before_branch 
        for(INS ins = BBL_InsHead(bb); INS_Valid(ins); ins = INS_Next(ins) ) {
            // https://github.com/jingpu/pintools/blob/master/source/tools/ToolUnitTests/branch_target_addr.cpp
            if(INS_IsBranch(ins)) {
                std::cerr << stats << '\n';
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)before_branch,
                                   IARG_CONTEXT, IARG_INST_PTR, IARG_BRANCH_TAKEN,
                                   IARG_BRANCH_TARGET_ADDR,
                                   IARG_PTR, stats,
                                   IARG_END);
            }
        }
    }
}


// This function is called when the application exits
void finish(int32_t code, void * _stats)
{
    BranchStatistics * stats = reinterpret_cast<BranchStatistics*>(_stats);
    // Branches (old file format)
    // Occurence Count - Mispredicted Count - Predicted Count - 0 - 0
    std::ofstream out_file((output_file_param.Value() + "_branches").c_str(), std::ios::app);
    for(const auto & val : stats->branches)
        out_file << val.second.first << ' ' << val.second.second << ' ' << (val.second.first - val.second.second ) << ' ' << 0 << ' ' << 0 << '\n';
    out_file.close();
    out_file.open( (output_file_param.Value() + "_summary").c_str(), std::ios::app);
    int mispredicted = (stats->branch_count - stats->branch_predicted_correctly);
    out_file << ins_count << " " << stats->branch_count << " " << stats->branch_predicted_correctly << " " << mispredicted << " " << stats->mispredicted_not_taken << " " << (mispredicted - stats->mispredicted_not_taken) << '\n';
    out_file.close();
}

int main(int argc, char ** argv)
{
    if (PIN_Init(argc, argv))
        return 1;
    //BlockStatistics block_stats;
    //INS_AddInstrumentFunction(Instruction, 0);
    //BranchStatistics branch_statistics;
    //std::cerr << &branch_statistics << '\n';
    TRACE_AddInstrumentFunction(trace, static_cast<void*>(&branch_statistics));
    PIN_AddFiniFunction(finish, static_cast<void*>(&branch_statistics));
    PIN_StartProgram();

    return 0;
}
