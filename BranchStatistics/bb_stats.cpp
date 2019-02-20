
#include <iostream>
#include <fstream>
#include <unordered_map>

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
BlockStatistics block_stats;
int bb_count = 0;
int ins_count = 0;

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
}
   
const char * StripPath(const char * path)
{
    const char * file = strrchr(path,'/');
    if (file)
        return file+1;
    else
        return path;
}

void trace(TRACE trace, void * stats)
{
    RTN rtn = TRACE_Rtn(trace);
    if(RTN_Valid(rtn)) {
        const char * name = StripPath(IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str());
        if(strcmp(name, filter_app_name.Value().c_str()))
            return;
    }
    //else
     //   return;
    for (BBL bb = TRACE_BblHead(trace); BBL_Valid(bb); bb = BBL_Next(bb))
    {
        BBL_InsertCall(bb, IPOINT_BEFORE, (AFUNPTR)count_blocks, IARG_PTR, stats, IARG_UINT32, BBL_NumIns(bb), IARG_UINT32, BBL_Size(bb),IARG_END);
    }
}


// This function is called when the application exits
void finish(int32_t code, void * stats)
{
    //BlockStatistics * block_stats = static_cast<BlockStatistics*>(stats);
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
