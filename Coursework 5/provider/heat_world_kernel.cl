__kernel void kernel_xy(
      __global float *state,
      __global uchar *properties,
      __global float *buffer,
      float inner, float outer, uint t
    ){

    uint x = get_global_id(0);
    uint y = get_global_id(1);
    uint n = get_global_size(0);

    uint index = y * n + x;
    uchar myProps = properties[index];

    if (myProps & 0x1) {
        float contrib = inner;
        float acc = 0;

        // Cell above
        if(myProps & 0x2) {
            contrib += outer;
            acc += state[index-n];
        }

        // Cell below
        if(myProps & 0x4) {
            contrib += outer;
            acc += state[index+n];
        }

        // Cell left
        if(myProps & 0x8) {
            contrib += outer;
            acc += state[index-1];
        }

        // Cell right
        if(myProps & 0x10) {
            contrib += outer;
            acc += state[index+1];
        }

        // Scale the accumulate value by the number of places contributing to it
        float res = (outer*acc + inner*state[index]) / contrib;
        // Then clamp to the range [-1,1]
        buffer[index] = min(1.0f, max(-1.0f, res));
    }
    // Else do nothing, this cell never changes (e.g. a boundary, or an interior fixed-value heat-source)
}
