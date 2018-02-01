HPCE CW6 Group Assessment
=========================

Group name : EddyMalou

Group members (Imperial login) : tcf14, lb3214, gr1714

Use-case   |  Machine (g3.4xlarge or c4.8xlarge)
-----------|------------------------------------
Stress     | c4.8xlarge
Certify    | c4.8xlarge
Search     | c4.8xlarge

When asked for who is responsible for things, these are
to gently steer marks where appropriate, e.g. for a
particularly good idea. The change in marks is not
great (at most 10% of this component), but is to at
least try to assign some kind of individual weighting.
Most ideas can genuinely be attributed to someone (even
if they are then developed as a group), and even if
you are pair programming, there is someone who takes
more of a lead.


Identify two (pairs) or three (triples) key improvements you made
=================================================================

Improvement : Task scheduling in certify
--------------------------------------

Initial idea : tcf14

Feasibility analysis : tcf14

Implementation : tcf14

### What was the big idea?:

Parallelising tests was initially done using tbb::parallel_for, giving full responsibility to tbb to dynamically schedule when each test has to be done. On the other hand, taskgroups seem to follow the order in which tasks are enqueued using "run()".

### Why was it expected to improve things?:

The idea was to tackle the issue of load balancing, the large differences in test execution times gave room for improvement. I had to make sure that the long individual tests (e.g. SerialOver) were scheduled at the very beginning of certify in order for all threads to finish execution reasonably close to each other in time.

### How well did it work?:

This improvement had a strong impact on the over execution time, by not only making it less volatile from one test to the next, but also lead to obtaining a new lower bound (in terms of execution speed), equal to the longest test execution time.

Improvement : Task scheduling in stress
--------------------------------------

Initial idea : lb3214

Feasibility analysis : lb3214

Implementation : lb3214

### What was the big idea?:

The idea, similarly to certify, was to run the various tests contained in the Rabbit benchmark in parallel. In order to do this, we would isolate each test in its own lambda function.

### Why was it expected to improve things?:

The tests can be made completely independent by cloning the generator in order to have it run on continuous blocks of random numbers. While we knew already that we would be limited by certain tests (multinomial for example), this was a good starting point.

### How well did it work?:

As expected, the speedup is minimal as the biggest chunk of work in stress is multinomial.


Improvement : Multinomial Parallelisation
--------------------------------------

Initial idea : gr1714

Feasibility analysis : gr1714

Implementation : gr1714

### What was the big idea?:

[Initial work by lb3214, but very broken, see [this](https://github.com/HPCE/hpce-2017-cw6-EddyMalou/pull/1)]
In this task, the objective was to parallelise the multinomial test in order to reduce its dominance in the rabbit benchmark. We quickly realised that when the input scale gets sufficiently large, the multinomial test is simply ran a certain number of times. These iterations can be made independent, similarly in idea to that of the benchmark parallelisation.

### Why was it expected to improve things?:

Multinomial represents a big chunk of the stress execution time. This combined with the large number of threads on the c4.8xlarge instance makes it an obvious speedup opportunity.

### How well did it work?:

As expected it worked very well, the speedup is basically linear in the number of threads for large problem size (ie N > 1 when the stress scale is > 10,000,000.


Improvement : Search parallelisation at benchmark level
--------------------------------------

Initial idea : gr1714

Feasibility analysis : gr1714

Implementation : gr1714-tcf14

### What was the big idea?:

The search driver runs a small benchmark (SmallCrush) repeatedly, on the same generator type. Each iteration, _workload_Next_ is called in order to reseed the generator. The big idea was to turn the infinite while loop into a parallel\_do loop. This type of loop generates work on the fly. In this case we unconditionally add more work and the parallel_do will also run forever

### Why was it expected to improve things?:

The benchmark runs in search are completely independent, it is natural that the speedup is going to be very good.

### How well did it work?:

The speedup is as expected and scales perfectly linearly in the number of threads.

Identify two (pairs) or three (triples) further changes that should be made
==========================================================================

These can include API changes, restructuring, moving to a
different platform, ...

Further Change : Lempel Ziv Optimisation
----------------------------------------

Initial idea : gr1714

### What is the big idea?:

Benchmarking the new implementation of the stress benchmark, revealed that the limitation from the multinomial test had been lightened. This meant that the limiting factor actually shifted over to the Lempel Ziv Complexity test. This test already uses Ukkonen's algorithm which seems to be the best known algorithm for syntax trie building, in O(n log n) time. This ruled out complexity improvement, leaving parallelisation. It is also possible that TestU01's implementation is bad memory wise, as the access patterns for the tree will likely be quite chaotic.

My main idea is to split the tree into a certain number of sub-trees, all of which would be independent, hence enabling them to parse bit strings in parallel. These subtrees would be connected by an implicit _super-tree_ stored in a vector of length number of subtrees. Each thread would have its own queue, and the main thread, responsible for querying the RNG for random numbers, would check the left most log(number of subtrees) bits from each bit string and pass the rest to the appropriate thread. These threads would loop forever until either killed by _poisoned pill_ or by checking the value a global variable.

### How difficult would it be, what would the impact be on users?

This rewrite is likely to be quite difficult to get right, especially in terms of thread communication. This test quickly becomes a bottleneck in the rabbit benchmark. However, it is clear that this improvement is not however going to be the difference between running the rabbit benchmark over and over and getting coffee between runs. As if this test is parallelised, it is likely that multinomial will become the limiting factor once again.

### How much of an improvement would you expect?

If It turns out my hypothesis is false and inter thread communication isn't an issue and the worker threads aren't restricted by how fast random numbers can be sampled from the generator, then the speedup should be close to linear.

Further Change : Multinomial test rewrite
----------------------------------------

Initial idea : gr1714

### What is the big idea?:

Before attempting an optimisation of this test, it would probably be beneficial to rewrite the test using modern C++ rather than C. I had big issues understanding the dataflow as the original code modifies global (function) variables in place _literally everywhere_. From profiling, it seems that this function spends most of its time in the _infinite_ for loop responsible for finding a spot for the current _Pos_.

Optimisation possibilities consists of:
- manually inlining the update count hash function, removing unnecessary logic
- Using a faster hash table implementation
- Splitting the computation of a single block of _n_ numbers into multiple blocks, in a map reduce fashion.

### How difficult would it be, what would the impact be on users?

Optimising this function would probably be quite challenging, partially due to the fact that it is a very large piece of code, but also because it is already somewhat optimised by default.

### How much of an improvement would you expect?

Spending some time on this test is likely to bring large speedups to a number of tests spread over the different benchmarks, as it is used relatively often.


Further Change : Reuse of randomly generated numbers
----------------------------------------

Initial idea : tcf14/gr1714

### What is the big idea?:

Rather than regenerating the random numbers in every test, pre-generate numbers and store them in an vector that could be reused from test to test. Such RNG can also be done on GPU.

### How difficult would it be, what would the impact be on users?

Such an operation would require adding some code to create a vector full of random numbers early on, but also to modify all test functions in order to read from that vector instead of regenerating numbers (which would be exhaustive). Also, the amount of numbers generated has to be carefully chosen, depending on the needs of some large tests.

### How much of an improvement would you expect?

Some significant improvement is expected, especially for larger amounts of numbers generated. However, using an OpenCL kernel could potentially lead to even higher speedups, assuming the GPU memory is able to store all of the required data.
