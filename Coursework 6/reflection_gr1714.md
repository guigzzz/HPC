HPCE CW6 Individual Reflection
==============================

Imperial login : gr1714

Github login : guigzzz

CW5 group : EddyMalou

CW6 group : EddyMalou

Cohort : EEE4 

Connections with existing courses and knowledge
===============================================

_Which courses that you took in previous years did you think this course was most related to,
and why? These could be courses in any department or any university._

If I'll be honest, most of what was done here wasn't related much to any course I have taken in the past. Apart from the software engineering course in first year which was my first introduction to coding and C++ (and data structures course in 2nd year), parallel programming and concurrency isn't a concept we see very often (if at all) in the earlier years of the EEE degree.

_What relevant pre-existing skills and existing knowledge did you bring to the course?_

I had an introduction to parallel programming this summer, where I had to speed up a face detector. I have also coded in C++ on my own time for personal projects and contributions to github projects. I've had a lot of experience in the past with Object Oriented python, so while I'd never done OO in C++, OO wasn't a new concept to me, and my pre-existing knowledge helped me in a number of situations (CW4 with the preprocessing in the constructor, etc.). I was also quite comfortable with usage of git and github, and had had some previous experience with shell (but not to the extent of writing scripts, I was just comfortable with the terminal).


What have you learnt in the course? : practise
==============================================

_What has been the most useful practical skill you have acquired in this course?_

The answer to this question is mixed between 2 things. Skills that make one a better software engineer, and the understanding of TBB and parallelism provided by this course.

Better software engineer:
- A far better knowledge of C++ and good practices when optimising a piece of code
  - for example: making sure reference implementations are still present for correctness 
- writing shell scripts for easy testing
- using _make_ to make compilation seemless throughout the team.
- deeper understanding of git functionalities 
  - learning to rebase rather than straight pull to prevent useless merges
  - learning to use PRs in github
- Learning to use AWS

Knowledge of TBB and parallel programming:
- awareness of the different functionalities present in TBB 
  - parallel for
  - parallel_do + feeder
  - task groups
- How to look at a serial for loop and know (basically) instantly if it is safe to parallelise 
- Ability to gauge what kind of improvement we should be expecting by parallelising.

_When would it have been useful in your own past activities?_

An understanding of parallelism and the inherent trade-off between communication costs and the scaling of a parallelised piece of software would most definitely have been useful during this summer's internship.

_When do you anticipate it might be useful in the future?_

TBB's ease of use and my now relatively good knowledge of it, means that if I need to make something faster (without taking forever to do so), I can get a parallel for loop up and running in a short amount of time. This somewhat relies on the assumption that I will coding in C++ in the future. However knowledge of parallelism can be useful regardless of the language used, especially seeing how hardware is becoming more and more parallel.

_What practical aspect (e.g. TBB, OpenCL, AWS, shell, ...) do you think is
 the *least* likely to be useful, and why?_

Even though I absolutely loved learning to code OpenCL and applying it in the various courseworks. The fact that it is quite low-level, and very difficult to get right (it's very easy to write slow OpenCL code!), means that unless I work for a company with big interests in getting a piece of code running as fast as possible with a large dev time budget, I will probably never code in OpenCL again. Like a lot of people right now, I have a growing interest in the field of Deep Learning, and taking an example from that, one would never write a convolution kernel from scratch, instead relying on library functions from manufacturers (CudNN for example).  
 
What have you learnt in the course? : theory/concepts
=====================================================

_How has the course changed your understanding of how to accelerate
or improve program performance? _

I'm confident in saying that I could probably do at least a decent job in speeding up most pieces of code. Whether this is by restructuring the algorithm to simplify or in order to expose parallelism, this course has taught me to try to look at a problem at a different angle. 

_What is a _specific_ example of tuning agglomeration *you* encountered or resolved in CW5 or CW6? Where
 possible, refer to a commit and source file (e.g. embed a link)._

[Edit Distance](https://github.com/HPCE/hpce-2017-cw5-EddyMalou/blob/master/provider/edit_distance.hpp) is the best example of this that I have worked on. In this puzzle, my solution used a combination of loop skewing and task agglomeration, along with some optimisation of the reference (i.e. making the inner loop branchless and reducing the memory complexity from O(n^2) to O(n)). A tunable parameter in this case the size of each sub-block of the iteration space. In the end, I set it to be ten times the number of threads available, as this seemed to offer a good compromise between parallelisation and the speed of the sequential version. The reason why this was done was that the simple skewed version I implemented at first was far slower than I expected. My hypothesis is that it could have to do with the very bad memory access patterns. Using this hypothesis, I aimed to generalise the skewing logic to use blocks of the iteration space rather than a single point. In the end, this generalisation made parallelisation actually run faster than the sequential version.

_What is a _specific_ example of managing critical path that *you* encountered or resolved in CW5 or CW6?
 Where possible, refer to a commit and source file._

There are two examples of this in coursework 5. 
In [Random Projection](https://github.com/HPCE/hpce-2017-cw5-EddyMalou/blob/master/provider/random_projection.hpp), the original code first generates the projection and then applies it. The first optimisation I made (also simplifying the code as a result, before parallelisation) was to manually inline the two function calls and combine their for loops. As a consequence, this removed the need for the temporary matrix _m_ that used to store the projection before application, while also making the constant in the complexity term smaller, due to less work needed to be done in the outer for loop.

Another example of this is what Theo (tcf14) and I did in [Gaussian Blur](https://github.com/HPCE/hpce-2017-cw5-EddyMalou/blob/master/provider/gaussian_blur.hpp). The original algorithm was extremely naive, implementing the blur in O(n^4) with a massive constant term due to the coefficients being calculated every time. While easy, naively parallelising the outer loop, without algorithm restructuring, would still have resulted in a very slow implementation. By consequence, we:

- split the kernel into X and Y to go from O(n^4) to O(n^3)
- Limited the size of the kernel to a constant to go from O(n^3) to O(kn^2) with k relatively large
- precomputed the kernel coefficients

_What is a _specific_ example of balancing communication vs computation cost from CW5 or CW6?
 Where possible, refer to a commit and source file._

The big challenge that came up in [Mining](https://github.com/HPCE/hpce-2017-cw5-EddyMalou/blob/master/provider/mining_GPU.hpp) was that the iteration space isn't fixed (theoretically it does have an extremely large upper bound, but that's not practical). To solve this puzzle, I essentially ask the GPU to compute batches of hashes, if a solution is found in that batch, break the infinite loop on the host, else compute another batch. In this case it was important to tune the size of the batch versus the cost of communication. The challenge here is that too big of a batch size limits the speed for small scales, while small batch sizes makes the communication cost extremely large for large scales. I found an appropriate middle-ground that happened to fit most scales nicely and stuck with that. A far better solution would have been to scale the batch size with the scale size, but I ran out of time necessary to tune that.