#include "layer.hpp"
#include "set_binary_io.hpp"

#include <unordered_set>
#include <random>

int main(int argc, char *argv[])
{
    set_binary_stdout();
    
    unsigned nIn=32;
    unsigned nOut=32;
    double sparsity=0.1;
    const char *dstFile="<STDOUT>";
    
    if(argc>1){
        nIn=atoi(argv[1]);
    }
    if(argc>2){
        nOut=atoi(argv[2]);
    }
    if(argc>3){
        sparsity=strtod(argv[3],0);
    }
    
    // generate_dense was taken out to simplify things.
    if(sparsity > 0.25){
        fprintf(stderr, "Warning : sparsity of more than > 0.25 may be slow to generate.\n");
    }

    unsigned nSynapses=sparsity * nIn * unsigned(nOut);
    
    std::mt19937 urng(1);
    
    // Build a set of (src<<32)|dst pairs
    std::unordered_set<uint64_t> connections;
    
    // Keep adding random pairs until we have hit the limit
    while( connections.size() < nSynapses ){
        uint64_t src= urng()%nIn;
        uint64_t dst= urng()%nOut;
        
        connections.insert( (src<<32)|dst );
    }
    
    // Turn the set into synapses
    std::vector<synapse_t> synapses;
    synapses.reserve(connections.size());
    for(auto c : connections){
        uint32_t r=urng();
        synapse_t n;
        n.src = (uint32_t)(c>>32),
        n.dst = (uint32_t)(c&0xFFFFFFFFul),
        n.weight = ( (r&0x80000000) ? -1 : +1) * ((int32_t)(r&0xFFFF));
        synapses.push_back(n);
    }
    
    save_layer(dstFile, nIn, nOut, synapses);
    
    return 0;
}
