#include "layer.hpp"
#include "set_binary_io.hpp"

#include <unordered_set>
#include <random>

struct layer{
    uint16_t nIn;
    uint16_t nOut;
    std::vector<synapse_t> synapses;
};

int main(int argc, char *argv[])
{
    std::vector<layer> layers;
    
    for(int i=1;i<argc;i++){
        layer curr;
        load_layer(argv[i], curr.nIn, curr.nOut, curr.synapses);
        layers.push_back(std::move(curr));
    }
    
    if(layers.size()==0){
        fprintf(stderr, "ERROR : No layers.\n");
        exit(1);
    }

    fprintf(stdout, "digraph layer{\n");
    
    fprintf(stdout, "  subgraph cluster_in {\n");
    for(unsigned i=0; i<layers.front().nIn; i++){
        fprintf(stdout, "  i%u;\n", i);
    }
    fprintf(stdout, "  }\n");
    
    for(int i=1;i<layers.size();i++){
        if(layers[i-1].nOut != layers[i].nIn){
            fprintf(stderr, "ERROR : Sizes of layers %u and %u don't match.\n", i-1, i);
            exit(1);
        }
        fprintf(stdout, "  subgraph cluster_hidden_%u {\n", i);
        for(unsigned j=0; j<layers.back().nIn; j++){
            fprintf(stdout, "  h_%i_%u;\n", i, j);
        }
        fprintf(stdout, "  }\n");
    }
    
    fprintf(stdout, "  subgraph cluster_out {\n");
    for(unsigned i=0; i<layers.back().nOut; i++){
        fprintf(stdout, "  o%u;\n", i);
    }
    fprintf(stdout, "  }\n");
    
    
    for(int i=0; i<layers.size(); i++){    
        const auto &synapses=layers[i].synapses;
        for(unsigned j=0; j<synapses.size(); j++){
            if(i==0 && layers.size()==1){
                fprintf(stdout, "  i%u -> o%u;\n", synapses[j].src, synapses[j].dst);
            }else if(i==0 && layers.size()>1){
                fprintf(stdout, "  i%u -> h_%u_%u;\n", synapses[j].src, i+1, synapses[j].dst);
            }else if(i==layers.size()-1){
                fprintf(stdout, "  h_%u_%u -> o%u;\n", i, synapses[j].src, synapses[j].dst);
            }else{
                fprintf(stdout, "  h_%u_%u -> h_%u_%u;\n", i, synapses[j].src, i+1, synapses[j].dst);
            }
        }    
    }
    
    fprintf(stdout, "}\n");
    
    
    
    return 0;
}
