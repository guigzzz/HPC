# HPC
Coursework repository for the [High Performance Computing](https://github.com/HPCE/hpce-2017) course.
This course contains a number of C++ based assignments, all oriented towards speeding up the execution of computational code using CPU and GPU (OpenCL) based multi-threading.

## Coursework 1: [Julia Set](https://en.wikipedia.org/wiki/Julia_set) Rendering
- Introduction to tbb::parallel_for

## Coursework 2: Fourier Transform Parallelisation
- Using tbb::parallel_for to parallelise a DFT
- Using tbb::task_group to parallelise an FFT

## Coursework 3: Finite Difference
- Introduction to OpenCL programming
- Introduction to GPU specific performance optimisations (reducing memory accesses, read/writes, etc)

## Coursework 4: Sparsely-connected neural network
- Demonstration of non-trivial parallelisation
- Problem reformulation in order to expose parallelism

## Coursework 5: Group project - _Puzzle_ Optimisation 
- Project consisted of 6 small algorithms
  1. Gaussian Blur: Compute a gaussian blur over an arbitrarily large input image.
  2. Mining (e.g. hashing): Search for _m_ such that _hash(m)_ satisfies a target condition.
  3. Edit Distance: Compute the edit distance betwen two string of bits.
  4. Vector projection: Compute the projection of a vector onto a randomly generated space.
  5. Finite Difference algorithm seen in Coursework 3.
  6. Hold time: Given a set of gates and flipflops, compute the minimum hold time.
  
- Objective was to get them to run as fast as possible using software or hardware acceleration
- Very large speedups over most of the puzzles, combination of:
  - Algorithmic Improvements (Reduced complexity)
  - Porting the algorithm to OpenCL

## Coursework 6: Group project - Optimise [TestU01](http://simul.iro.umontreal.ca/testu01/tu01.html)
- Real-world application of the knowledge gained from the course
- Unknown and complicated codebase (C with a C++ _wrapper_)
- Objective was to optimise three benchmarks
