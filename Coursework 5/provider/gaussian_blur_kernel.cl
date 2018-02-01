#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void horizontal_kernel(
    __global uchar  *input,
    __global float *coef,
    __global float *temp,
    int kernel_size,
    int height,
    int width
){ 
    int yOut = get_global_id(0) / width;
    int xOut = get_global_id(0) % width;
    
    float acc=0.0;
    for(int i=max(xOut-kernel_size,0); i < min(xOut+kernel_size,width); i++){
        acc += coef[abs(i-xOut)] * input[yOut*width+i];
    }

    temp[ get_global_id(0) ] = acc;
};

__kernel void vertical_kernel(
    __global float *temp,
    __global float *coef,
    __global uchar *output,
    int kernel_size,
    int height,
    int width,
    float normalisor
){
    int yOut = get_global_id(0) / width;
    int xOut = get_global_id(0) % width;

    float acc=0.0;
    for(int i=max(yOut-kernel_size,0); i < min(yOut+kernel_size,height); i++){
        acc += coef[abs(i-yOut)] * temp[i*width+xOut];
    }

    acc /= normalisor; 

    if(acc<0){
        acc=0;
    }else if(acc>255){
        acc=255;
    }

    output[ get_global_id(0) ] = (uchar)acc;
};