
ulong TEA_hash(ulong v, uint rounds, uint* k) {
    uint v0=v & 0xFFFFFFFFul, v1=v >> 32;
    uint sum=0, i;           /* set up */
    uint delta=0x9e3779b9;                     /* a key schedule constant */
    uint k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */
    
    ulong res=1234567801234567ul;
    for (i=0; i < rounds; i++) {                       /* basic cycle start */
        sum += delta;
        v0 += ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
        v1 += ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
        res=((res << 7) ^ (res>>57)) + (v0&v1);
    }     /* end cycle */
    return res;
}

__kernel void mining_kernel(
        __global ulong* hashes, 
        __global ulong* inputs,
        ulong offset, ulong length,
        __local ulong* scratch,
        uint rounds, __global uint *k
    ){
    uint global_index = get_global_id(0);
    ulong global_size = get_global_size(0);

    ulong min = INFINITY;
    ulong best_input = 0;

    uint keys[4] = {k[0], k[1], k[2], k[3]};

    for(uint i = global_index; i < length; i += global_size){
        ulong v = i + offset;
        ulong hash = TEA_hash(v, rounds, keys);
        if(hash < min){
            min = hash;
            best_input = v;
        }
    }

    int local_index = get_local_id(0);
    int local_size = get_local_size(0);
    int group_index = get_group_id(0);

    //source: http://developer.amd.com/resources/articles-whitepapers/opencl-optimization-case-study-simple-reductions/
    scratch[local_index] = best_input;
    barrier(CLK_LOCAL_MEM_FENCE);

    for(uint offset = local_size / 2; offset > 0; offset /= 2) {
        if (local_index < offset) {
            ulong left = scratch[local_index];
            ulong right = scratch[local_index + offset];
            scratch[local_index] = (
                TEA_hash(left, rounds, keys) < TEA_hash(right, rounds, keys)
                ) ? left : right;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    if (local_index == 0) {
        inputs[group_index] = scratch[0];
        hashes[group_index] = TEA_hash(scratch[0], rounds, keys);
    }
}