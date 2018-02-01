#include "layer.hpp"
#include <iostream>
LayerPtr CreateSimpleLayer(unsigned nIn, unsigned nOut, const std::vector<synapse_t> &synapses);
LayerPtr CreateParForNaiveLayer(unsigned nIn, unsigned nOut, const std::vector<synapse_t> &synapses);
LayerPtr CreateParForAtomicLayer(unsigned nIn, unsigned nOut, const std::vector<synapse_t> &synapses);
LayerPtr CreateClusteredLayer(unsigned nIn, unsigned nOut, const std::vector<synapse_t> &synapses);
LayerPtr CreateParForClusteredLayer(unsigned nIn, unsigned nOut, const std::vector<synapse_t> &synapses);
LayerPtr CreateGPUClusteredLayer(unsigned nIn, unsigned nOut, const std::vector<synapse_t> &synapses);

std::shared_ptr<Layer> createLayer(
    const std::string &layerEngine,  // If empty then use a default factory
    unsigned nIn,
    unsigned nOut,
    const std::vector<synapse_t> &synapses
){
    if(layerEngine=="simple"){
        return CreateSimpleLayer(nIn,nOut,synapses);
         
    }else if(layerEngine=="par_for_naive"){
        return CreateParForNaiveLayer(nIn,nOut,synapses);

    }else if(layerEngine=="par_for_atomic"){
        return CreateParForAtomicLayer(nIn,nOut,synapses);
        
    }else if(layerEngine=="clustered"){
        return CreateClusteredLayer(nIn,nOut,synapses);
    
    }else if(layerEngine=="par_for_clustered"){
        return CreateParForClusteredLayer(nIn,nOut,synapses);
    /*    
    }else if(layerEngine=="gpu_clustered"){
        return CreateGPUClusteredLayer(nIn,nOut,synapses);
    */
    }else if(layerEngine=="auto"){
        return CreateSimpleLayer(nIn,nOut,synapses);
    
    }else{
        fprintf(stderr, "ERROR : Unknown engine '%s'\n", layerEngine.c_str());
        exit(1);
    }
}
