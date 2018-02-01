inline uint lcg(uint s)
{
    return s*1664525+1013904223;
}

__kernel void random_projection_kernel(
    __global ulong* acc,
    __global uint* v,
    __global uint* ini_seeds,
    uint p, uint n, uint max_size
    ){

    uint i = get_global_id(0) / n;
    uint y = get_global_id(0) % n;

    if(i < max_size && y < max_size){
        uint seed = ini_seeds[i] + y * max_size * 19937; // fast forward seed

        ulong a = 0;
        for(uint x=0; x<max_size; x++){
            uint r = lcg(seed);
            a += (r >> 8 < p) ? r & v[x] : 0;
            seed+=19937;
        }
        
        acc[i * max_size + y] = a;
    }
}