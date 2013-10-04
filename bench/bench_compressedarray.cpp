#include <bw/compressedarray.h>

#include <valgrind/callgrind.h>

#include "test_utils.h"

//
// valgrind --tool=callgrind --instr-atstart=no --callgrind-out-file=log_bench_compressedarray.out  ./bench_compressedarray
//
// kcachegrind log_bench_compressedarray.out
//

int main(int argc, char** argv) {
    typedef BW::CompressedArray<3, float> CA;
    typedef CA::V V;

    vigra::MultiArray<3,float> theData(V(100,200,300));
    FillRandom<float, vigra::MultiArray<3,float>::iterator>::fillRandom(theData.begin(), theData.end());

    CALLGRIND_START_INSTRUMENTATION;
    CA ca(theData);
    CALLGRIND_STOP_INSTRUMENTATION;
}