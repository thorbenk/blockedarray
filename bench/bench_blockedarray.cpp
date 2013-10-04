#include <bw/array.h>

#include <valgrind/callgrind.h>

#include "test_utils.h"

//
// valgrind --tool=callgrind --instr-atstart=no --callgrind-out-file=log_bench_blockedarray.out  ./bench_blockedarray
//
// kcachegrind log_bench_blockedarray.out
//

int main(int argc, char** argv) {
    typedef BW::Array<3, float> BA;
    typedef BA::V V;

    vigra::MultiArray<3,float> theData(V(100,200,300));
    FillRandom<float, vigra::MultiArray<3,float>::iterator>::fillRandom(theData.begin(), theData.end());

    CALLGRIND_START_INSTRUMENTATION;
    BA blockedArray(V(50,50,50), theData);
    CALLGRIND_STOP_INSTRUMENTATION;
}