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
    
    fprintf(stdout, "digraph layer{\n");
        
    for(unsigned i=0; i<synapses.size(); i++){
        fprintf(stdout, "  i%u -> o%u [ label=\"%d\" ]\n", synapses[i].src, synapses[i].dst, synapses[i].weight);
    }    
    
    fprintf(stdout, "}\n");
    
    
    
    return 0;
}
