float update_pixel_heat(
  uint myProps,
  float inner,
  float outer,
  __local float *localState,
  uint localIndex,
  uint n
) {
    float contrib = inner;
    float acc = 0;
    uint localStateWidth = PIXEL_SIDE_PER_THREAD * get_local_size(1) + 2;

    // Cell above
    if(myProps & 0x2) {
        contrib += outer;
        acc += localState[localIndex-localStateWidth];
    }

    // Cell below
    if(myProps & 0x4) {
        contrib += outer;
        acc += localState[localIndex+localStateWidth];
    }

    // Cell left
    if(myProps & 0x8) {
        contrib += outer;
        acc += localState[localIndex-1];
    }

    // Cell right
    if(myProps & 0x10) {
        contrib += outer;
        acc += localState[localIndex+1];
    }

    // Scale the accumulate value by the number of places contributing to it
    float res = (outer*acc + inner*localState[localIndex]) / contrib;
    // Then clamp to the range [-1,1]
    return min(1.0f, max(-1.0f, res));
}


uint get_global_index(int y, int x, uint n) {
  return (get_global_id(0) * PIXEL_SIDE_PER_THREAD + y) * n
    + get_global_id(1) * PIXEL_SIDE_PER_THREAD + x;
}

// uint coord_y_global(uint n) {
//   return
// }
//
// uint coord_x_global() {
//   return get_global_id(1) * PIXEL_SIDE_PER_THREAD^2;
// }

int get_local_state(int y, int x) {
  int width = PIXEL_SIDE_PER_THREAD * get_local_size(1) + 2;
  return width * (1 + get_local_id(0) * PIXEL_SIDE_PER_THREAD + y)
    + (1 + get_local_id(1) * PIXEL_SIDE_PER_THREAD + x);
}

/*
[g_y=0][g_x=1] startingCopyY = 0, startingCopyX = 0
[g_y=0][g_x=1][0][0] localState(31) = get_global_index(16) = 0.000000
[g_y=0][g_x=1][0][1] localState(32) = get_global_index(17) = 0.000000
[g_y=0][g_x=1][1][0] localState(41) = get_global_index(24) = 0.000000
[g_y=0][g_x=1][1][1] localState(42) = get_global_index(25) = -0.141780
*/

__kernel void kernel_xy(
      __global float *state,
      __global uchar *properties,
      __global float *buffer,
      float inner, float outer,
      uint n,
      __local float *localState // (side_thread_count*PIXEL_SIDE_PER_THREAD+2)^2
    ){
    uint y_block = get_global_id(0);
    uint x_block = get_global_id(1);
    uint side_block_count = get_global_size(0);

    uint y_thread = get_local_id(0);
    uint x_thread = get_local_id(1);
    uint side_thread_count = get_local_size(0);

    // STEP 1: copy from global to local/private memory
    int startingCopyY = (y_block > 0 && y_thread == 0) ? -1 : 0;
    int startingCopyX = (x_block > 0 && x_thread == 0) ? -1 : 0;
    int limitCopyY = PIXEL_SIDE_PER_THREAD + ( ((y_block + 1) < side_block_count && (y_thread + 1) == side_thread_count) ? 1 : 0);
    int limitCopyX = PIXEL_SIDE_PER_THREAD + ( ((x_block + 1) < side_block_count && (x_thread + 1) == side_thread_count) ? 1 : 0);

    // Copy state --> localState (kernel state + outer square)
    //printf("\n[g_y=%u][g_x=%x] startingCopyY = %i, startingCopyX = %i", y_block, x_block, startingCopyY, startingCopyX);
    for (int y = startingCopyY; y < limitCopyY; y++) {
      for (int x = startingCopyX; x < limitCopyX; x++) {
        localState[get_local_state(y, x)] = state[get_global_index(y, x, n)];
        //printf("\n[g_y=%u][g_x=%x][l_y=%u][l_x=%x][%i][%i] localState(%u) = get_global_index(%u) = %f", y_block, x_block, y_thread, x_thread, y, x, get_local_state(y, x), get_global_index(y, x, n), state[get_global_index(y, x, n)]);
      }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    // Copy properties --> private
    // uchar privateProperties[PIXEL_SIDE_PER_THREAD];
    // for (unsigned y = startingY; y < limitY; y++) {
    //   for (unsigned x = startingX; x < limitX; x++) {
    //       uint index = y * n + x;
    //       privateProperties[index] = properties[index];
    //   }
    // }

    // STEP 2: perform computations

    // Global coordinates of kernel
    int startingY = y_block * PIXEL_SIDE_PER_THREAD;
    int startingX = x_block * PIXEL_SIDE_PER_THREAD;
    // handle boundary kernels
    uint limitY = min(n, (y_thread+1) * PIXEL_SIDE_PER_THREAD) - y_thread * PIXEL_SIDE_PER_THREAD;
    uint limitX = min(n, (x_thread+1) * PIXEL_SIDE_PER_THREAD) - x_thread * PIXEL_SIDE_PER_THREAD;

    for (uint y = 0; y < limitY; y++) {
      for (uint x = 0; x < limitX; x++) {
          uint index = get_global_index(y, x, n);
          uint myProps = properties[index];
          if (myProps & 0x1) {
            // STEP 3: copy back to global memory
            uint localIndex = get_local_state(y, x);
            buffer[index] = update_pixel_heat(myProps, inner, outer, localState, localIndex, n);
            //printf("\n[g_y=%u][g_x=%x][l_y=%u][l_x=%x][%i][%i] global_index=%u, local_index=%u", y_block, x_block, y_thread, x_thread, y, x, index, localIndex);
          }
          else {
            //printf("\n[g_y=%u][g_x=%x][l_y=%u][l_x=%x][%i][%i] global_index=%u", y_block, x_block, y_thread, x_thread, y, x, index);
          }
          // Else do nothing, this cell never changes (e.g. a boundary, or an interior fixed-value heat-source)
      }
    }
}
