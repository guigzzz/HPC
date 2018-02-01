#ifndef user_hold_time_hpp
#define user_hold_time_hpp

#include <algorithm>
#include "tbb/parallel_for.h"

#include "puzzler/puzzles/hold_time.hpp"
#include "hold_time_dijkstra.hpp"

class HoldTimeProvider : public puzzler::HoldTimePuzzle
{
public:
  HoldTimeProvider()
  {
    dijkstra = std::make_shared<HoldTimeDijkstraProvider>();
  }

private:
  std::shared_ptr<HoldTimeDijkstraProvider> dijkstra;

protected:

  unsigned memoized_path_delay(
    unsigned ffCount,
    const std::vector<std::vector<unsigned>> &nodes,
    unsigned targetFF,
    unsigned currNode,
    std::vector<unsigned> &distanceCache
  ) const {
    // Check if result is cached
    unsigned cachedIdx = currNode*nodes.size()+targetFF;
    unsigned cachedDist = distanceCache[cachedIdx];
    if (cachedDist > 0) {
      return cachedDist;
    }

    // We use UINT_MAX as a sentinal value to indicate that no path was found
    unsigned minDist = UINT_MAX;

    // Don't look at the first element, as that is the delay
    for (unsigned i=1; i < nodes[currNode].size(); i++) {
      unsigned localDist;

      // Follow each incoming wire
      unsigned src = nodes[currNode][i];
      if (targetFF == src) {
        localDist = 0; // Found it!
      } else if (src < ffCount) {
        localDist = UINT_MAX; // Didn't find it, but still hit a flip-flop
      } else {
        localDist = memoized_path_delay(ffCount, nodes, targetFF, src, distanceCache);
      }

      if (localDist!=UINT_MAX) {
        minDist=std::min(minDist, localDist);
      }
    }

    if (minDist!=UINT_MAX) {
      minDist+=nodes[currNode][0]; // Add local delay
    }

    // Cache result
    // NB: no need to bother qbout concurrent writes, cache can be set twice
    distanceCache[cachedIdx] = minDist;

    return minDist;
  }

  virtual void MemoizedExecute(
    puzzler::ILog *log,
    const puzzler::HoldTimeInput *input,
    puzzler::HoldTimeOutput *output
  ) const {
    // minDelay for each outer loop iteration
    std::vector<unsigned> localDistances(input->flipFlopCount, 0);

    // Implement a memoized version of path_delay to cache direct paths dist
    size_t s = input->nodes.size();
    std::vector<unsigned> distanceCache(s*s, 0);

    tbb::parallel_for(0u, input->flipFlopCount, [&](unsigned srcFF){
      unsigned localMinDelay = UINT_MAX;

      for(unsigned dstFF=0; dstFF < input->flipFlopCount; dstFF++){
        unsigned localDistance = memoized_path_delay( input->flipFlopCount,
          input->nodes, dstFF, srcFF, distanceCache);

        log->LogVerbose("Checking path %u <- %u : %u", dstFF, srcFF, localDistance);

        if(localDistance < localMinDelay){
          log->LogInfo("New min distance = %u", localDistance);
          localMinDelay=localDistance;
        }
      }

      localDistances[srcFF] = localMinDelay;
    });

    unsigned minDelay = *std::min_element(localDistances.begin(), localDistances.end());
    log->LogInfo("Min delay = %u", minDelay);
    output->minDelay = minDelay;
  }

  virtual void Execute(
    puzzler::ILog *log,
    const puzzler::HoldTimeInput *input,
    puzzler::HoldTimeOutput *output
  ) const override {
    dijkstra->Execute(log, input, output);
  }
};

#endif
