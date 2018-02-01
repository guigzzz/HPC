#ifndef user_hold_time_dijkstra_hpp
#define user_hold_time_dijkstra_hpp

#include <algorithm>

#include "tbb/parallel_for.h"

#include "puzzler/puzzles/hold_time.hpp"


class HoldTimeDijkstraProvider : public puzzler::HoldTimePuzzle
{
private:
  unsigned currentGlobalMin = UINT_MAX;

public:
  HoldTimeDijkstraProvider() {}

  virtual std::string Name() const override {
    return "hold_time_dijkstra";
  }

  inline int get_parent(int i) const
  {
    return (i - 1) / 2;
  }

  // return left child of A[i]
  inline int get_left_child(int i) const
  {
    return (2 * i + 1);
  }

  // return right child of A[i]
  inline int get_right_child(int i) const
  {
    return (2 * i + 2);
  }

  // swaps 2 elements()
  inline void swap(std::vector<unsigned> *A, unsigned a, unsigned b) const
  {
    unsigned temp = (*A)[a];
    (*A)[a] = (*A)[b];
    (*A)[b] = temp;
  }

  inline unsigned popSrc(
    std::vector<unsigned> *A,
    unsigned srcNodesDistancesCount,
    std::vector<unsigned> &cache
  ) const {
    unsigned i = 0;

    // Saved top element to be returned
    unsigned top = (*A)[i];

    // Swap largest for smallest value
    (*A)[0] = (*A)[--srcNodesDistancesCount];

    auto getCache = [&](unsigned heapIndex){
      return cache[(*A)[heapIndex]];
    };

    // Reorder min-heap
    while (true)
    {
      // get left and right child of node at index i
      unsigned left = get_left_child(i);
      unsigned right = get_right_child(i);

      unsigned smallest = i;

      // compare A[i] with its left and right child
      // and find smallest value
      if (left < srcNodesDistancesCount && getCache(left) < getCache(i)) {
        smallest = left;
      }

      if (right < srcNodesDistancesCount && getCache(right) < getCache(smallest)) {
        smallest = right;
      }

      // swap with child having lesser value and repeat on the child
      if (smallest != i) {
        swap(A, i, smallest);
        i = smallest;
      } else {
        return top;
      }
    }
  }

  inline void insertNode(
    unsigned *src,
    std::vector<unsigned> *A,
    unsigned srcNodesDistancesCount,
    std::vector<unsigned> &cache
  ) const {
    unsigned i = srcNodesDistancesCount;

    // Append to array, if an element is given for insertion
    if (src != NULL) {
      // If vector is full, extend it
      if ((*A).size() == i) {
        (*A).push_back(*src);
      }
      else {
        (*A)[i] = *src;
      }
    }

    auto getCache = [&](unsigned heapIndex){
      return cache[(*A)[heapIndex]];
    };

    // check if node at index i and its parent violates the heap property
    while (i && getCache(get_parent(i)) > getCache(i))
    {
      // swap the two if heap property is violated
      unsigned parent = get_parent(i);
      swap(A, i, parent);
      i = parent;
    }
  }

  inline void printArray(
    std::vector<unsigned> &A,
    unsigned srcNodesDistancesCount,
    std::vector<unsigned> &cache,
    puzzler::ILog *log,
    unsigned srcNode
  ) const {
    std::stringstream ss;
    ss << "min-heap[#" << srcNodesDistancesCount << "]: ";
    for(unsigned i = 0; i < srcNodesDistancesCount; i++) {
      ss << A[i] << "[" << cache[A[i]] << "], ";
    }
    log->LogVerbose("[%u] %s", srcNode, ss.str().c_str());
  }

  // Computes the shortest path from srcNode to any flip-flop.
  unsigned get_dijkstra_shortest_path(
    unsigned srcNode,
    unsigned ffCount,
    const std::vector<std::vector<unsigned>> &nodes,
    puzzler::ILog* /*log*/
  ) const {
    unsigned nodesCount = nodes.size();

    // Implement a memoized version of path_delay to cache direct paths dist
    std::vector<unsigned> distanceCache(nodesCount, 0);

    // Init min-heap of src nodes, ordered by currNode-src distances (recorded in distanceCache)
    // scale / 2000
    unsigned divBy = 100;
    if(getenv("HPCE_HOLD_TIME_DIV_BY")){
      divBy = (unsigned) atoi(getenv("HPCE_HOLD_TIME_DIV_BY"));
    }
    unsigned heuristicSize = (nodesCount - ffCount) / divBy;
    unsigned srcNodesDistancesCount = 0;
    std::vector<unsigned> srcNodesDistances(heuristicSize, 0);

    // Initialize with root
    srcNodesDistances[srcNodesDistancesCount++] = srcNode;
    //log->LogVerbose("[%u] [After init]", srcNode);
    //printArray(srcNodesDistances, srcNodesDistancesCount, distanceCache, log, srcNode);

    //unsigned maxSrcNodes = 0;
    while (true)
    {
      //maxSrcNodes = MAX(maxSrcNodes, srcNodesDistancesCount);
      // Remove smallest distance from heap
      // if empty, return UINT_MAX i.e. no path found

      unsigned currNode = popSrc(&srcNodesDistances, srcNodesDistancesCount--, distanceCache);
      //log->LogVerbose("[%u] [After extraction]", srcNode);
      //printArray(srcNodesDistances, srcNodesDistancesCount, distanceCache, log, srcNode);
      unsigned currPathLength = distanceCache[currNode];

      //log->LogVerbose("[%u] Extracted min: %u <- %u : %u", srcNode, currNode, srcNode, currPathLength);

      // If local min > global min, abort
      if (currPathLength >= currentGlobalMin) {
        return UINT_MAX;
      }

      // If flip-flop is hit, return
      if (currNode < ffCount && currPathLength > 0) {
        //std::cerr << "MAX = " << (nodesCount - ffCount) / maxSrcNodes << std::endl;
        return currPathLength;
      }

      unsigned localDelay = nodes[currNode][0];

      for (unsigned i = 1; i < nodes[currNode].size(); i++) {
        unsigned childSrc = nodes[currNode][i];

        // If cache unset, cache and add element to heap
        unsigned newDistance = currPathLength + localDelay;
        //log->LogVerbose("[%u] Check child %u : cache = %u | newDistance = %u", srcNode, childSrc, distanceCache[childSrc], newDistance);
        if (distanceCache[childSrc] <= 0) {
          distanceCache[childSrc] = newDistance;
          //log->LogVerbose("[%u] insertNode: %u : %u", srcNode, childSrc, distanceCache[childSrc]);
          insertNode(&childSrc, &srcNodesDistances, srcNodesDistancesCount++, distanceCache);
          //log->LogVerbose("[%u] [After insert]", srcNode);
          //printArray(srcNodesDistances, srcNodesDistancesCount, distanceCache, log, srcNode);
        }
        // Else if we found a faster path, update cache (node alreay)
        else if (distanceCache[childSrc] > newDistance) {
          // Element must be in heap, otherwise it would have been already removed
          distanceCache[childSrc] = newDistance;

          // Reorder element in heap - (happens rarely, tolerates silly impl.)
          for (unsigned j = 0; j < srcNodesDistancesCount; j++) {
            if (srcNodesDistances[j] == childSrc) {
                insertNode(NULL, &srcNodesDistances, j, distanceCache);
                break;
            }
          }
        }
        // Otherwise, skip
      }
    }
  }

  virtual void Execute(
    puzzler::ILog *log,
    const puzzler::HoldTimeInput *input,
    puzzler::HoldTimeOutput *output
  ) const override {
    unsigned ffCount = input->flipFlopCount;

    // minDelay for each outer loop iteration
    std::vector<unsigned> localDistances(ffCount, 0);

    tbb::parallel_for(0u, ffCount, [&](unsigned srcFF){
      unsigned localDistance = get_dijkstra_shortest_path(srcFF,
        ffCount, input->nodes, log);

      //log->LogInfo("New min distance = %u", localDistance);
      localDistances[srcFF] = localDistance;
    });

    unsigned minDelay = *std::min_element(localDistances.begin(), localDistances.end());
    log->LogInfo("Min delay = %u", minDelay);
    output->minDelay = minDelay;
  }
};

#endif
