#include "layer.hpp"
#include <tbb/parallel_for.h>

class ParForAtomicLayer
    : public Layer
{
protected:
    unsigned m_nIn;
    unsigned m_nOut;

    std::vector<synapse_t> m_synapses;
public:
    ParForAtomicLayer(
        unsigned nIn,
        unsigned nOut,
        const std::vector<synapse_t> &synapses
    )
        : m_nIn(nIn)
        , m_nOut(nOut)
        , m_synapses(synapses)
    {}
    
    const char *name() const
    { return "par_for_atomic"; }
    
    virtual unsigned input_size() const
    { return m_nIn; }
    
    virtual unsigned output_size() const
    { return m_nOut; }
    
    void execute(
        const int8_t *pIn,  // Values of input neurons in -127..+127
        int8_t *pOut        // Values of output neurons in -127..+127
    ) const
    {        
        // std::vector<int32_t> acc(m_nOut, 0); // Create a working vector
        std::vector<tbb::atomic<int32_t> > acc(m_nOut, 0);
        
        // for(unsigned i=0; i<m_synapses.size(); i++){
        tbb::parallel_for(size_t(0), m_synapses.size(), [&](const unsigned i){
            // weight has 16 fractional bits, input has 7 fractional bits
            // contrib has 23 fractional bits
            int32_t contrib = m_synapses[i].weight * pIn[ m_synapses[i].src];
            
            // Add into acc array with 16 fractional bits
            acc[ m_synapses[i].dst ] += contrib >> (23-16);
        });
        
        tbb::parallel_for(0u, m_nOut, [&](const unsigned i){
        // for(unsigned i=0; i<m_nOut; i++){
            // Input to sigmoid has 16 fractional bits
            // Output maps to -127..+127 (7 fractional bits)
            pOut[i] = sigmoid( acc[i] ); // compress with sigmoid
        });
    }
};

LayerPtr CreateParForAtomicLayer(unsigned nIn, unsigned nOut, const std::vector<synapse_t> &synapses)
{
    return std::make_shared<ParForAtomicLayer>(nIn, nOut, synapses);
}
