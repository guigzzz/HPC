#include "layer.hpp"
#include <tbb/parallel_for.h>

class ClusteredLayer
    : public Layer
{
protected:
    unsigned m_nIn;
    unsigned m_nOut;

    std::vector< std::vector<synapse_t> > twod_synapses;
public:
    ClusteredLayer(
        unsigned nIn,
        unsigned nOut,
        const std::vector<synapse_t> &synapses
    )
        : m_nIn(nIn)
        , m_nOut(nOut)
    {
        twod_synapses.resize(m_nOut);
        for(unsigned i=0; i < synapses.size(); i++){
                twod_synapses[ synapses[i].dst ].push_back( synapses[i] );
        }
    }
    
    const char *name() const
    { return "clustered"; }
    
    virtual unsigned input_size() const
    { return m_nIn; }
    
    virtual unsigned output_size() const
    { return m_nOut; }    
    
    void execute(
        const int8_t *pIn,  // Values of input neurons in -127..+127
        int8_t *pOut        // Values of output neurons in -127..+127
    ) const
    {        
        for(unsigned i=0; i<twod_synapses.size(); i++){
            // weight has 16 fractional bits, input has 7 fractional bits
            // contrib has 23 fractional bits
            int32_t acc(0);
            for(unsigned j = 0; j < twod_synapses[i].size(); j++){
                int32_t contrib = twod_synapses[i][j].weight * pIn[ twod_synapses[i][j].src];
                acc += contrib >> (23-16);
            }

            pOut[i] = sigmoid(acc);
            
            // Add into acc array with 16 fractional bits
        }
    }
};

LayerPtr CreateClusteredLayer(unsigned nIn, unsigned nOut, const std::vector<synapse_t> &synapses)
{
    return std::make_shared<ClusteredLayer>(nIn, nOut, synapses);
}
