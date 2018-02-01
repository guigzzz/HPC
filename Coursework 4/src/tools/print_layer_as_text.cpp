#include "layer.hpp"
#include "set_binary_io.hpp"

#include <unordered_set>
#include <random>

int main(int argc, char *argv[])
{
    set_binary_stdin();
    // Leave stdout as text
    
    const char *srcFile="<STDIN>";
    
    uint16_t nIn, nOut;
    std::vector<synapse_t> synapses;
    load_layer(srcFile, nIn, nOut, synapses);
    
    fprintf(stdout, "nIn  = %6u\n", nIn);
    fprintf(stdout, "nOut = %6u\n", nOut);
    fprintf(stdout, "nSynapses = %u\n", (unsigned)synapses.size());
    fprintf(stdout, "density = %g\n", synapses.size() / (double(nIn)*double(nOut)) );
    
    for(unsigned i=0; i<synapses.size(); i++){
        fprintf(stdout, "  %u : %u <- %u (%f)\n", i, synapses[i].dst, synapses[i].src, synapses[i].weight/65536.0);
    }    
    
    fprintf(stdout, "nIn  = %6u\n", nIn);
    fprintf(stdout, "nOut = %6u\n", nOut);
    fprintf(stdout, "nSynapses = %u\n", (unsigned)synapses.size());
    fprintf(stdout, "density = %g\n", synapses.size() / (double(nIn)*double(nOut)) );
    
    
    return 0;
}
