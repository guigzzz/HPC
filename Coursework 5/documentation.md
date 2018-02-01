Summary
=======

count-us-in : yes

Initial approach (Assessed)
---------------------------

_At most 100 words: How did you get started?_

Our initial approach to improving the performance of a puzzle was to copy-paste
the reference implementation, add a few log->LogVerbose and draw some
data / control flow graphs of the puzzle. It helped us discover how some of the algorithms
could be rewritten with a smaller complexity, and identify the pre-fetching /
caching and parallelisation opportunities.

Some of the puzzles being generic mathematical/image processing problems, online research made it easier to formulate the problem more explicitly and identify the opportunities mentioned above.

Regarding parallelisation, the first step was usually to test the corectness of the re-structured algorithm on CPU using TBB. Depending on the nature of the problem, the loops were then ported onto an OpenCL kernel, given that the g2.2xlarge is a GPU instance.

Approach to group work (Assessed)
---------------------------------

_At most 100 words: How did you split tasks up. How did you work together?_

First of all, as you are a group of 3, it seemed obvious for each individual to own two puzzles. Throughout the duration of the coursework,
we talked on a daily basis to discuss speed-up techniques and synchronised our
efforts to improve the performance of our weakest puzzles. Also, we met a few
times for some debugging / optimisation-focussed sessions.

Puzzle summary (Not assessed)
-----------------------------

_This section is not assessed for correctness, but must be filled in._

_Out of the set of selected puzzles what do you think were your group's strongest and weakest puzzle solutions?_

Strongest puzzle : edit_distance/hold_time

Weakest puzzle : mining

_Out of *all* puzzles what did you think were the hardest and easiest puzzles?_

Hardest puzzle : edit_distance/heat_world

Easiest puzzle : gaussian_blur/rand_proj



_Copy and fill-in the below template once for each puzzle (so six for triples, four for pairs)_

Puzzle : Heat World
============

Lead developer : lb3214

_Which techniques did you use for this puzzle? There are no marks for_
_ticking more boxes or adding lots of stuff, this is more to gather statistics._

- [x] GPU
- [x] tbb::parallel_for
- [ ] tbb::task_group
- [ ] Other TBB library
- [x] pre-calculation
- [ ] algorithmic re-structuring
- [ ] Other: [add brief description]

Chosen parallelisation strategy (Assessed)
------------------------------------------

Starting from where we left off at the end of CW3, we followed the same parallelisation strategy relying heavily on the use of GPU with some pre-computations.

Optimisations or transformations applied (Assessed)
---------------------------------------------------

We further optimised the pre-computations done before invoking the kernel by reducing the size of a packed pixel information from 6 down to 5 bits, and parallelising this step. We also removed the branching in the kernel function by pre-computing the if/else condition.

Opportunities identified but not exploited (Assessed)
-----------------------------------------------------

The biggest opportunity we could not get to work was to use shared memory. Following the `Further optimisations` section from coursework 3, we have a near-working implementation in `heat_world_v2.hpp` that groups threads into work groups and let them share state information faster. However, it still does not catch some scaling edge cases which we could not fix in time so we ended up using the enhanced original implementation.


Puzzle : Mining
============

Lead developer : gr1714

_Which techniques did you use for this puzzle? There are no marks for_
_ticking more boxes or adding lots of stuff, this is more to gather statistics._

- [x] GPU
- [ ] tbb::parallel_for
- [ ] tbb::task_group
- [ ] Other TBB library
- [ ] pre-calculation
- [ ] algorithmic re-structuring
- [ ] Other: [add brief description]

Chosen parallelisation strategy (Assessed)
------------------------------------------

This problem is embarassingly parallel as the iterations of the original while loop are completely independent. As a consequence, this problem scales perfectly and is a prime candidate for a OpenCL port.

Optimisations or transformations applied (Assessed)
---------------------------------------------------

- OpenCL: two stage map reduce. First a batch of hashes is computed. Each work group then reduces its computed hashes to find the minimum (in logN time using a divide and conquer approach, also in parallel) which is then output to a global buffer. The buffer is then read and reduced in software on the CPU. This runs in a while loop and runs until a suitable hash is found. For each iteration of the global while loop, the nonce is offset by the number of hashes calculated in the previous batch and passed to the kernel. The batch_size (number of hashes computed per iteration) along with the work group size and the number of hashes computed per thread can be tuned.

Opportunities identified but not exploited (Assessed)
-----------------------------------------------------

- Dynamic scaling of batch size as a function of problem size
- Simplification of kernel to remove local synchronisation constraint

Puzzle : Edit Distance
============

Lead developer : gr1714 (tcf14 figured out most of the loop skewing logic)

_Which techniques did you use for this puzzle? There are no marks for_
_ticking more boxes or adding lots of stuff, this is more to gather statistics._

- [ ] GPU
- [ ] tbb::parallel_for
- [x] tbb::task_group
- [ ] Other TBB library
- [ ] pre-calculation
- [ ] algorithmic re-structuring
- [ ] Other: [add brief description]

Chosen parallelisation strategy (Assessed)
------------------------------------------

Parallel processing of items parallel to the antidiagonal of the "work matrix".

Optimisations or transformations applied (Assessed)
---------------------------------------------------

Sequential Optimisations:

- Reduced memory complexity from O(n²) to O(n): It is only necessary to store the current and previous rows rather than the whole nxn d-matrix.

- Used boolean casting to integer to make the inner loop branchless
- Small simplifications to needed operations

Transformations for parallel implementation:

- Divided the work matrix into a predefined number of blocks.
- Blocks are iterated through in a skewed manner, allowing them to be computed in parallel. Each block is computed using the optimised sequential algorithm.
- Final implementation uses optimised sequential for small scales and the parallel version for large scales.

Opportunities identified but not exploited (Assessed)
-----------------------------------------------------

- None

Puzzle : Random Projection
============

Lead developer : gr1714

_Which techniques did you use for this puzzle? There are no marks for_
_ticking more boxes or adding lots of stuff, this is more to gather statistics._

- [x] GPU
- [x] tbb::parallel_for
- [ ] tbb::task_group
- [ ] Other TBB library
- [x] pre-calculation
- [x] algorithmic re-structuring
- [ ] Other: [add brief description]

Chosen parallelisation strategy (Assessed)
------------------------------------------

Parallel generation of all projection with sequential reduction at the end.

Optimisations or transformations applied (Assessed)
---------------------------------------------------

Optimisations:

- Combined the outer loop + the two function calls into a triple nested for loop. Removing:
    - a double for loop
    - Some temporary storage ("m")
- reorganised projection calculation to maximise usage of bitwise operations and pre-calculation opportunities

- Both TBB and OpenCL Implementations.
    - However only the OpenCL implementation is used as it is sufficiently efficient even for small scales.


Opportunities identified but not exploited (Assessed)
-----------------------------------------------------

- None



Puzzle : Hold Time
============

Lead developer : lb3214

_Which techniques did you use for this puzzle? There are no marks for_
_ticking more boxes or adding lots of stuff, this is more to gather statistics._

- [ ] GPU
- [x] tbb::parallel_for
- [ ] tbb::task_group
- [ ] Other TBB library
- [ ] pre-calculation
- [x] algorithmic re-structuring
- [ ] Other: [add brief description]

Chosen parallelisation strategy (Assessed)
------------------------------------------

Most of the time spent was spent on algorithmic re-structuring: hold time now
solves `ffCount` shortest path problems in parallel while the reference
implementation computed `ffCount`^2 recursive depth-first searches. This gives
a better time complexity but a worse memory complexity. The implemented
algorithm is heavily inspired by Dijkstra's algorithm.


Optimisations or transformations applied (Assessed)
---------------------------------------------------

This new design does need to search all paths anymore, and Dijkstra's algorithm
can provide a lower bound on the minimum distance at any time. This invariant
allows us to prune graph traversals that won't produce an optimal solution by
maintaining a global value of the shortest path computed so far.

Opportunities identified but not exploited (Assessed)
-----------------------------------------------------

A GPU approach was tried and failed due to the heavy memory usage that slows
down performance significantly (more memory access latency in GPU than in CPU).
It first seemed promising as each shortest-path problem could be solved
independently without any synchronisation between tasks.


Puzzle : Gaussian Blur
============

Lead developer : tcf14

_Which techniques did you use for this puzzle? There are no marks for_
_ticking more boxes or adding lots of stuff, this is more to gather statistics._

- [X] GPU
- [ ] tbb::parallel_for
- [ ] tbb::task_group
- [ ] Other TBB library
- [X] pre-calculation
- [X] algorithmic re-structuring
- [ ] Other: [add brief description]

Chosen parallelisation strategy (Assessed)
------------------------------------------

Main priority was to reduce the existing complexity of the algorithm before porting the problem onto GPU. The gaussian distribution function has numerous mathematical properties offering ways to vastly reduce the number of operations (symmetry, exponential...).

Optimisations or transformations applied (Assessed)
---------------------------------------------------

Major optimisations include:

- Only half of 1D gaussian coefficients computed using the symmetry of the gaussian distruibution function.
- Pre-compute all coefficients, prior to entering the iteration space.
- Take advantage of the separablility of the kernel to run two 1D blurs (one on columns, the other on rows), reducing the complexity from O(n^4) to O(n^3).
- Take advantage of exponential decay of the gaussian distribution function to reduce the kernel size (k), disregarding the influence of distant pixels, reducing the complexity from O(n^3) to O(n^2). k depending on the radius size.
- Port the convolution operations to GPU, using an OpenCL kernel.

Minor optimisations include:

- Operating with `float` within the OpenCL kernel (instead of `double`).


Opportunities identified but not exploited (Assessed)
-----------------------------------------------------

- Finding a tighter lower bound for the kernel size k.
- Investigate usage of FFT for fast 1D convolution algorithm to speed up the OpenCL kernel.
- Passing coefficients as direct arguments to the kernel to reduce memory access.
- Hard coding the kernel size and embedding the coefficients in the kernel itself could allow the compiler to optimise the convolution further (loop unrolling, etc).
- Using the GPU's local memory: instead of working on the entirety of our iteration space (full image), the GPU kernel input could be a section of the image (1 line for instance, depending on the size of the image). This data could be stored in the GPU's local memory, for faster memory access on that specific segment of pixels. This would probably be mostly beneficial for the vertical pass as this pass accesses pixels that are not continuous in memory (hence cache misses could be a problem).
