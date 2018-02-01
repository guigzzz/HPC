#include "layer.hpp"
#include "set_binary_io.hpp"

#include <iostream>

int main(int argc, char *argv[])
{
    try{
        
        std::vector<LayerPtr> layers;
        
        set_binary_io();
        
        ////////////////////////////////////////////////////////
        // Load all the layers
        
        for(int i=1; i<argc; i++){
            std::string config=argv[i];
            std::string engine="auto";
            
            int colon=config.find(':');
            if(colon!=std::string::npos){
                engine=config.substr(colon+1);
                config=config.substr(0, colon);
            }
            
            fprintf(stderr," Loading layer %u from file %s using engine %s\n", i, config.c_str(), engine.c_str());
            
            uint16_t nIn, nOut;
            std::vector<synapse_t> synapses;
            load_layer(config.c_str(), nIn, nOut, synapses);
            
            layers.push_back( createLayer(engine, nIn, nOut, synapses) );
        }
        
        ////////////////////////////////////////////////////////
        // Run the network
        
        while(1){
            unsigned nIn=layers[0]->input_size();
            std::vector<int8_t> input(nIn);
            
            if(!read_input(input.size(), &input[0]))
                break;
            
            for(int i=0;i<layers.size();i++){
                std::vector<int8_t> output(layers[i]->output_size());
                
                layers[i]->execute(&input[0], &output[0]);
                
                std::swap(input, output); // Cheap version of input=output
            }
            
            write_output(input.size(), &input[0]);
            
        }
    }catch(std::exception &e){
        std::cerr<<"Exception : "<<e.what()<<"\n";
        exit(1);
    }catch(...){
        std::cerr<<"Unknonwn exception\n";
        exit(1);
    }
    
    return 0;
}
