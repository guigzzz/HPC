#define MAX_NODES_ROW_LENGTH 4  /* delay + 1..3 degrees */
#define NULL 4294967295         /* Using a sentinel instead of NULL pointer */

#define SWAP(A, a, b) do { \
    uint temp = A[a];      \
    A[a] = A[b];           \
    A[b] = temp;           \
  } while(0)

/*
 * Implementation of a min-heap in a fixed size array.
 */

// return parent of A[i]
uint get_parent(uint i)
{
  return (i - 1) / 2;
}

// return left child of A[i]
uint get_left_child(uint i)
{
  return (2 * i + 1);
}

// return right child of A[i]
uint get_right_child(uint i)
{
  return (2 * i + 2);
}

/*
uint popSrc(
  uint *A,
  uint srcNodesDistancesCount,
  uint *cache
) {
  uint i = 0;

  // Saved top element to be returned
  uint top = A[i];

  // Swap largest for smallest value
  A[0] = A[--srcNodesDistancesCount];

  // Reorder min-heap
  while (true)
  {
    // get left and right child of node at index i
    uint left = get_left_child(i);
    uint right = get_right_child(i);

    uint smallest = i;

    // compare A[i] with its left and right child
    // and find smallest value
    if (left < srcNodesDistancesCount && cache[A[left]] < cache[A[i]]) {
      smallest = left;
    }

    if (right < srcNodesDistancesCount && cache[A[right]] < cache[A[smallest]]) {
      smallest = right;
    }

    // swap with child having lesser value and
    // call heapify-down on the child
    if (smallest != i) {
      SWAP(A, i, smallest);
      i = smallest;
    } else {
      return top;
    }
  }
}

void insertNode(
  uint src,
  uint *A,
  uint i,
  uint *cache
) {
  // Append to array, if an element is given for insertion
  if (src != NULL) {
    A[i] = src;
  }

  // check if node at index i and its parent violates the heap property
  while (i && cache[A[get_parent(i)]] > cache[A[i]])
  {
    // swap the two if heap property is violated
    uint parent = get_parent(i);
    SWAP(A, i, parent);
    i = parent;
  }
}
*/

// Computes the shortest path from srcNode to any flip-flop.
__kernel void get_dijkstra_shortest_path(
  __global const uint *nodes,
  __global uint *res
) {
  uint srcNode = get_global_id(0);
  const uint ffCount = get_global_size(0);
  // Needs to be set to 0
  uint distanceCache[NODES_COUNT] = {0};
  uint srcNodesDistances[NODES_COUNT];
  uint srcNodesDistancesCount = 0;

  //printf("\n[%u] nodes ==> %u - %u - %u", srcNode, nodes[srcNode*MAX_NODES_ROW_LENGTH], nodes[srcNode*MAX_NODES_ROW_LENGTH+1], nodes[srcNode*MAX_NODES_ROW_LENGTH+2]);

  // Initialize with root
  srcNodesDistances[srcNodesDistancesCount++] = srcNode;
  //printf("\n[%u] l=%u - srcNodesDistances[0] = %u", srcNode, srcNodesDistancesCount, srcNodesDistances[0]);

  while (true)
  {
    // if empty, return UINT_MAX i.e. no path found
    if (srcNodesDistancesCount == 0) {
      *res = UINT_MAX;
      return;
    }

    // Remove smallest distance from heap
    uint currNode;
    //uint currNode = popSrc(srcNodesDistances, srcNodesDistancesCount--, distanceCache);
    /*** BEGIN POP MIN-HEAP HEAD ***/
    do {
      uint *A = srcNodesDistances;
      uint *cache = distanceCache;
      uint i = 0;

      // Saved top element to be returned
      //uint top = A[i];
      currNode = A[i];

      // Swap largest for smallest value
      A[0] = A[--srcNodesDistancesCount];

      // Reorder min-heap
      while (true)
      {
        // get left and right child of node at index i
        uint left = get_left_child(i);
        uint right = get_right_child(i);

        uint smallest = i;

        // compare A[i] with its left and right child
        // and find smallest value
        if (left < srcNodesDistancesCount && cache[A[left]] < cache[A[i]]) {
          smallest = left;
        }

        if (right < srcNodesDistancesCount && cache[A[right]] < cache[A[smallest]]) {
          smallest = right;
        }

        // swap with child having lesser value and
        // call heapify-down on the child
        if (smallest != i) {
          SWAP(A, i, smallest);
          i = smallest;
        } else {
          break;
          //return top;
        }
      }
    } while(0);
    /*** END POP MIN-HEAP HEAD ***/
    uint currPathLength = distanceCache[currNode];

    //printf("\n[%u] Extracted min: %u <- %u : %u", srcNode, currNode, srcNode, currPathLength);
    //printf("\n[%u] l=%u - srcNodesDistances[0] = %u", srcNode, srcNodesDistancesCount, srcNodesDistances[0]);

    // If flip-flop is hit, return
    if (currNode < ffCount && currPathLength > 0) {
        *res = currPathLength;
        return;
    }

    uint localDelay = nodes[currNode * MAX_NODES_ROW_LENGTH];

    for (uint i = 1; i < MAX_NODES_ROW_LENGTH; i++) {
      uint idx = currNode * MAX_NODES_ROW_LENGTH + i;

      // If we hit an undefined sentinel
      if (nodes[idx] == UINT_MAX) {
        break;
      }

      uint childSrc = nodes[idx];

      // If cache unset, cache and add element to heap
      uint newDistance = currPathLength + localDelay;
      //printf("\n[%u] Check child %u : cache = %u | newDistance = %u", srcNode, childSrc, distanceCache[childSrc], newDistance);
      if (distanceCache[childSrc] <= 0) {
        distanceCache[childSrc] = newDistance;
        //printf("\n[%u] insertNode: %u : %u", srcNode, childSrc, distanceCache[childSrc]);

        //insertNode(childSrc, srcNodesDistances, srcNodesDistancesCount++, distanceCache);
        /*** BEGIN INSERT NODE IN MIN-HEAP ***/
        do {
          uint src = childSrc;
          uint *A = srcNodesDistances;
          uint i = srcNodesDistancesCount++;
          uint *cache = distanceCache;

          // Append to array
          A[i] = src;

          // check if node at index i and its parent violates the heap property
          while (i && cache[A[get_parent(i)]] > cache[A[i]])
          {
            // swap the two if heap property is violated
            uint parent = get_parent(i);
            SWAP(A, i, parent);
            i = parent;
          }
        } while(0);
        /*** END INSERT NODE IN MIN-HEAP ***/
      }
      // Else if we found a faster path, update cache (node alreay)
      else if (distanceCache[childSrc] > newDistance) {
        // Element must be in heap, otherwise it would have been already removed
        distanceCache[childSrc] = newDistance;

        // Reorder element in heap - (happens rarely, tolerates silly impl.)
        for (uint j = 0; j < srcNodesDistancesCount; j++) {
          if (srcNodesDistances[j] == childSrc) {
              //insertNode(NULL, &srcNodesDistances, j, distanceCache);
              /*** BEGIN INSERT NODE IN MIN-HEAP ***/
              do {
                uint *A = srcNodesDistances;
                uint i = j;
                uint *cache = distanceCache;

                // check if node at index i and its parent violates the heap property
                while (i && cache[A[get_parent(i)]] > cache[A[i]])
                {
                  // swap the two if heap property is violated
                  uint parent = get_parent(i);
                  SWAP(A, i, parent);
                  i = parent;
                }
              } while(0);
              /*** END INSERT NODE IN MIN-HEAP ***/
              break;
          }
        }
      }
      // Otherwise, skip
    }
  }
}
