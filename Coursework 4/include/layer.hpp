#ifndef layer_hpp
#define layer_hpp

#include <cmath>
#include <cstdint>
#include <vector>
#include <iostream>

#include <memory>

// Input is signed with 16 fractional bits
// Output is signed with 7 fractional bits
inline int8_t sigmoid(int32_t acc)
{
    float fAcc=acc / 65536.0f;
    float fSig = 1.0f / (1.0f + expf(-fAcc) );
    return (int8_t)(127*fSig);
}

//! This is designed to be exactly 64-bits
/*! On disk it will be stored in little-endian.
    Weights are stored as signed fixed-point with 16 fractional bits
*/
struct synapse_t{
    int32_t  weight; //! Varies how much it contributes
    uint16_t src;    //! Input neuron to take value from
    uint16_t dst;    //! Output neuron to contribute to
};


/*! Load a layer from the given file */
void load_layer(
    const char *srcFile,
    uint16_t &nIn,
    uint16_t &nOut,
    std::vector<synapse_t> &synapses
);

/*! Save a layer to the given file */
void save_layer(
    const char *dstFile,
    uint16_t nIn,
    uint16_t nOut,
    const std::vector<synapse_t> &synapses
);

/*! Read n bytes into array from stdin.
    Return true if successful, false on end of stream.
*/
bool read_input(unsigned n, int8_t *dest);

/*! Write n bytes to stdout */
void write_output(unsigned n, const int8_t *dest);



class Layer
{
public:
    virtual ~Layer()
    {}
    
    virtual unsigned input_size() const=0;
    virtual unsigned output_size() const=0;

    virtual void execute(
        const int8_t *pIn,  // Values of input neurons
        int8_t *pOut        // Values of output neurons
    ) const =0;
};

typedef std::shared_ptr<Layer> LayerPtr;

LayerPtr createLayer(
    const std::string &engine,
    unsigned nIn,
    unsigned nOut,
    const std::vector<synapse_t> &synapses
);

#endif
