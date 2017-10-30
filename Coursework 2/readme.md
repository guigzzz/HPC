This coursework is due on:

    Fri Oct 27th at 22:00

Submisssion is via github.

I will be doing incremental testing starting from Tue 25th, but only
based on what you currently have in your github private repo. When
exactly that testing happens will be sporadic and unpredictable (no
more than once a day), both by intention (to avoid dependency on
a perceived "deadline") and necessity (I have to kick it off manually).

_Note: there were some problems with [my definition of test number 39](https://github.com/HPCE/hpce-2017-cw2/issues/37),
so it was passing when I expected things to pass, but not failing when I expected them to fail. For the final
run I'll take that test out. Thanks to @Filippo555 and @jeremych1000 for the discussion._

The process I'll follow is:

- Clone from your "master" branch

- Run tests on my local setup and record results

- Push the results back into a folder called `dt10_logs` in your private repository.

It is up to you to keep your repository in a compilable
state. You can do this by either:

1 - Collecting commits locally until you are happy with a "release" you want
    to push.
  
2 - Using a "dev" branch to handle speculative development, then push to
    the "master" branch when you reach milestones.

0. Overview
===========

This coursework explores parallelism using threaded-building
blocks in a bit more detail. You should see _linear speedup_ here
in a slightly more complex example than in CW1, i.e. the performance
rises in proportion to the number of CPU cores.

This distribution contains a basic object framework for creating
and using fourier transforms, as well as two implementations:

1. A direct fourier transform taking O(n^2) steps.
2. A recursive fast-fourier transform taking O(n log n) steps.

Also included within the package is a very simple test suite
to check that the transforms work, and a registry which allows
new transforms to be added to the package.

Your job in this coursework is to explore a number of basic
(but effective) ways of using TBB to accelerate existing code,
and to compare and contrast the performance benefits of them.

_Dislaimer: Never write your own fourier transform for production
use. Everything you do here will be slower and less accurate than
existing FFT libraries._

You may notice that this used to be CW3 a few years ago, but is now CW2.
This is to reflect the change in order and emphasis compared to previous years.
The older matlab experiment (which was awesome, but no-one
liked matlab) has disappeared in favour of the very
simple TBB intro in CW1, with this experiment to elaborate
the TBB approach and explore it in a bit more detail. There
is also more emphasis on exploring performance, as I think
that previously people focussed on just getting the code
working. There are quite a few graphs to get (five), but
if you follow the advice about how to automate things, it
is fairly quick to get them.

1. Environment and setup
=======================

Choose a Target Environment
---------------------------

You can select your environment in a similar way to CW1,
but the assessed compilation and evaluation will be done
under Ubuntu in AWS. My plan is to use a [c4.8xlarge](http://aws.amazon.com/ec2/instance-types/)
instance, but you don't need to optimise your code
specifically for that machine. This shouldn't matter
to you now (as you're not relying on anything apart from
TBB), but is more encouragement to [try out AWS](aws.md) before
you _have_ to later on.

Github accounts
---------------

Submission will be via github, so this time you'll need
to actually work with your private repository. I have created
you a private repository again, but this time it will be empty.
It is up to you `clone` a local copy of the master repository,
then `push` it back up to your private repository. There
are more details on this in the [brief git intro](git.md).
[Also.](http://xkcd.com/1597/)

_Note: It is not a disaster if submission via github
doesn't work, as I will also be getting people to do a blackboard
submission as backup so I get the code. However, I did this
for the last three years, and it worked fine._

The fourier transform framework
-------------------------------

The package you have been given is a slightly overblown
framework for implementing fourier transforms. The file
`include\fourier_transform.hpp` defines a generic
fourier transform interface, allowing forwards and
backwards transforms.

In the src directory, there are two fourier transform
implementations, and two programs:

1. `test_fourier_transform`, which will run a number of
   simple tests on a given transform to check it works.
   The level of acceptable error in the results is
   defined as 1e-9 (which is quite high).

2. `time_fourier_transform`, which will time a given
   transform for increasing transform sizes with a given
   level of allowed parallelism.

If you build the two programs use:

    make all

then you should be able to test and time the two existing transforms.
For example, you can do:

    bin/test_fourier_transform

which will list the two existing transforms, and:

    bin/time_fourier_transform hpce.fast_fourier_transform

to see the effect of increasing transform size on
execution time (look in `time_fourier_transform.cpp` to
see what the columns mean).

Even though there is no parallelism in the default
framework, it still relies on TBB to control parallelism,
so it will not build, link, or execute, without TBB
being available.

_Note: the direct fourier transform is very likely
to fail a _small_ number of tests, while the fast fourier transform
should pass all of them. This demonstrates that
even though two algorithms calculate the same result,
the order of calculation can change the numerical
accuracy significantly. In case you are not aware,
floating point is [not exact](http://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html),
and if you add together long sequences the error
adds up quickly._

You may wish to consider the algorithmic tradeoffs
between the sequential direct and fast fourier
transforms - what is an achievable `n` for the
direct versus the fast transform. Remember that
parallelism and optimisation are never a substitute
for an algorithm with a better intrinsic complexity.

2. Using tbb::parallel_for (badly) in direct_fourier_transform
======================================================

The file `src/direct_fourier_transform.cpp` contains a classic
discrete fourier transform, which takes O(n^2) operations to
do an n-point fourier transform. Classic 1st year maths :)

First we're going to parallelise the inner loop, in order to
directly observe the overheads of parallelisation.

Using tbb::parallel_for in the fourier transform
------------------------------------------------

The framework is designed to support multiple fourier
transforms which you can select between, so we'll
need a way of distinguishing your transform from
anyone else's (in principle I should be able to create
one giant executable containing everyone in the
class's transforms). The basic framework uses the namespace
`hpce`, but your classes will live in the namespace
`hpce::your_login`, and the source files in `src/your_login`.
For example, my namespace is `hpce::dt10`, and my
source files go in `src/dt10`.

There are five steps in this process:

1. Creating the new fourier transform class
2. Registering the class with the fourier framework
3. Adding the parallel constructs to the new class
4. Testing the parallel functionality (up to you)
5. Finding the new execution time (up to you, see notes at the end).

### Creating the new fourier transform class

Copy `src/direct_fourier_transform.cpp` into a new
file called `src/your_login/direct_fourier_transform_parfor_inner.cpp`.
Modify the new file so that the contained class is called
`hpce::your_login::direct_fourier_transform_parfor_inner`, and reports
`hpce.your_login.direct_fourier_transform_parfor_inner` from `name()`. Apart
from renaming, you don't need to change any functionality yet.

To declare something in a nested namespace, simply
insert another namespace declaration inside the existing
one. For example, if you currently have `hpce::my_class`:

    namespace hpce{
	  class my_class{
	    ...
	  };
	};

you could get it into a new namespace called bobble, by
changing it to:

    namespace hpce{
	  namespace bobble{
	    class my_class{
		  ...
		};
	  };
	};

which would result in a class with the name `hpce::bobble::my_class`.

Add your new file to the set of objects compiled into
the executable by adding it to `FOURIER_IMPLEMENTATION_OBJS` in the `makefile`,
and check that it still compiles.

### Register the class with the fourier framework

As part of the modifications, you probably noticed
a function at the bottom of the file called `std::shared_ptr<fourier_transform> Create_direct_fourier_transform()`,
and (hopefully!) modified to `std::shared_ptr<fourier_transform> Create_direct_fourier_transform_parfor_inner()`.
This is the factory function, used by the rest of the
framework to create transforms by name, without knowing
how they are implemented.

If you look in `src/fourier_transform_register_factories.cpp`, you'll
see a function called `fourier_transform::RegisterDefaultFactories`,
which is where you can register new transforms. To minimise
compile-time dependencies, the core framework knows nothing
about the transforms - all it knows is how to create them.

Towards the top is a space to declare your external factory
function, which can be uncommented. Then at the bottom
of `RegisterDefaultFactories`, uncomment the call which
registers the factory.

At this point, you should find that your new implementation
is listed if you build `test_fourier_transform` and run it with no
arguments:

    bin/test_fourier_transform

For example, I get:

    $ bin/test_fourier_transform
    hpce.direct_fourier_transform
    hpce.dt10.direct_fourier_transform_parfor_inner
    hpce.fast_fourier_transform

You can now test it or time it:

    bin/test_fourier_transform hpce.[your_login].direct_fourier_transform_parfor_inner

or:

    bin/time_fourier_transform hpce.[your_login].direct_fourier_transform_parfor_inner

Hopefully your implementation still works, in so far as the
execution will be identical, and the time should be the same
(beyond run-to-run variations due to the system).

If your transform doesn't turn up at run-time, or the code won't compile,
make very sure that you have renamed everything within
`src/your_login/direct_fourier_transform_parfor_inner.cpp` to the
new name. Also make sure that the factory function is declared
as `std::shared_ptr<fourier_transform> hpce::your_login::Create_direct_fourier_transform_parfor_inner()`,
both in `src/your_login/direct_fourier_transform_parfor_inner.cpp`, and in
`src/fourier_transform_register_factories.cpp` (particularly if you get a linker error).

### Add the parallel_for loop to the **_inner_** loop

You need to rewrite the inner loop in both `forwards_impl` and `backwards_impl`,
using the transformation of for loop to `tbb::parallel_for` shown previously. I would
suggest doing one, running the tests, and then doing the other. You'll
need to make sure that you include the appropriate header for parallel_loop from
TBB at the top of the file, so that the function can be found.


3. Exploring the grain size of parallel_for
===========================================

We are now going to explore tuning the grain size,
which is essentially adjusting the ratio of computation to
parallel overhead. This should provide much
more explicit versions of the tradeoffs that you
may have seen in the previous coursework.

Partitioners and grain size
---------------------------

By default, `tbb::parallel_for` uses something called
the `auto_partitioner`, which is used to [partition](https://www.threadingbuildingblocks.org/docs/help/reference/algorithms/partitioners.html)
your iteration space into parallel chunks. The auto-partitioner
attempts to balance the number of parallel tasks created
against the amount of computation at each iteration
point. Internally it tries to split the iteration space
up recursively into parallel chunks, and then switches
to serial execution within each chunk once it has enough
parallel tasks for the number of processors.

We can explore this and control it by using manual
grain size control, which explicitly says how big each
parallel task should be. There is an alternate form
of parallel_for, which describes a chunked iteration
space:

    tbb::parallel_for(tbb::blocked_range<unsigned>(i,j,K), [&](const tbb::blocked_range<unsigned> &chunk){
        for(unsigned i=chunk.begin(); i!=chunk.end(); i++ ){
            y[i]=myLoop(i);
        }
    }, tbb::simple_partitioner());

This is still equivalent to the original loop, but
now we have more control. If we unpack it a bit, we
could say:

    // Our iteration space is over the unsigneds
    typedef tbb::blocked_range<unsigned> my_range_t;

    // I want to iterate over the half-open space [i,j),
    // and the minimum parallel chunk size should be K.
    my_range_t range(i,j,K);

    // This will apply my function over the half-open
    // range [chunk.begin(),chunk.end) sequentially.
    auto f=[&](const my_range_t &chunk){
        for(unsigned i=chunk.begin(); i!=chunk.end(); i++ ){
            y[i]=myLoop(i);
        }
    };

We now have the choice of executing it directly:

    f(range); // Apply f over range [i,j) sequentially

or in parallel with chunk size of K:

    tbb::parallel_for(range, f, tbb::simple_partitioner());

The final [`tbb::simple_partitioner()`](https://software.intel.com/en-us/node/506152) argument is telling
TBB "I know what I am doing; I have decided that K is the
best chunk size."

_We could alternatively use [tbb::auto_partitioner](https://software.intel.com/en-us/node/506150),
which would tell TBB "I know that the chunk size should not be less than
K, but if you want to do larger chunks, go for it." In general auto_partitioner
will provide better adaptive performance, but for now we _want_ to see
where the bad performance occurs._

Environment Variables
---------------------

We want to vary the chunk size, but we also want it to be
user tunable for a specific machine. So we are going to allow the
user to choose a value K at run-time using an environment variable.

The function [`getenv`](http://www.cplusplus.com/reference/cstdlib/getenv/)
allows a program to read [environment variables](http://en.wikipedia.org/wiki/Environment_variable)
at run-time. So if I choose an environment variable called `HPCE_X`, I could
create a C++ program `read_x`:

    #include <cstdlib>

    int main()
    {
        char *v=getenv("HPCE_X");
        if(v==NULL){
            printf("HPCE_X is not set.\n");
        }else{
            printf("HPCE_X = %s\n", v);
        }
        return 0;
    }

then on the command line I could do:

    > ./read_x

      HPCE_X is not set.

    > export HPCE_X=wibble
    > ./read_x

      HPCE_X = wibble

    > export HPCE_X=100
    > ./read_x

      HCPE_X = 100

Environment variables are a way of defining ambient properties,
and are often used for tuning the execution of a program for the
specific machine they are on (I'm not saying it's the best way,
but it happens a lot). I used the `HPCE_` prefix as I want to
try to avoid clashes with other programs that might want a
variable called `X`.

### Add parameterisable chunking to your class

Modify your implementation of `direct_fourier_transform_parfor_inner`
to use a chunk size _K_, where _K_ is either
specified as a decimal integer using the environment
variable `HPCE_DIRECT_INNER_K`. If it is not set, you should use
a sensible default. For example, if the user does:

    export HPCE_DIRECT_INNER_K=16

you would use a chunk size of 16 for the inner loop.

_Hint: in order to save you time: [atoi](http://www.cplusplus.com/reference/cstdlib/atoi/) can
turn a string into an integer._

You should think about what "sensible default" means: the TBB user
guide gives [some guidance](https://www.threadingbuildingblocks.org/docs/help/tbb_userguide/Controlling_Chunking.html)
in the form of a "rule of thumb". Your rule of thumb
should probably take into account the approximate amount
of work per inner iteration point, so you don't want
to choose K=1. However, you would like to eventually
have _some_ parallelism, so you also can't choose a
really large default K.

**Task**: create a graph called `results/direct_inner_versus_k.pdf` which
explores the performance of K=[1..16] versus time for n=[64,256,1024,4096].
K should be on the x-axis, time on the y-axis, and each n is a different line.
Time should extend up to 30 seconds per test run, which you can achieve with:

    export HPCE_DIRECT_INNER_K = <Whatever K you want>
    bin/time_fourier_transform hpce.direct_fourier_transform_parfor_inner 0 30

You can generate this file manually by running the
program 16 times, picking out the values and putting
them in a spreadsheet, or you might want to
explore automation further using [scripting and pivot charts](csv_and_pivot.md).


4. Using tbb::parallel_for in direct_fourier_transform _properly_
=================================================================

You should be seeing different performance as you scale K (up to a point),
and hopefully be developing an intuition that the inner loop
is probably not where you want to accelerate in general (unless
loop-carried dependencies force you to).

Create a new implementation called `direct_fourier_transform_parfor_outer`
(with all the associated files and factory setup), and parallelise
over the outer loop.

Be very careful to test your output, and don't blindly assume
that things are correct. You may want to look carefully at
what the two loops are doing, and how they interact:

- Can you convert from accumulation into an array to accumulation into a scalar?

- Think about the 2d iteration space and the dependencies within it.

- Can you interchange the loops (iterate over a different dimension
  of the iteration space as the outer loop)?

**Task**: create a graph called `results/direct_outer_time_versus_p.pdf` which
explores the parallelism versus time for varying n.
n should be on the x-axis, time on the y-axis, and each P is a different line
(try to find a machine with at least 4 processors). Again, time should extend up to 30 seconds per test run.

**Task**: create a graph called `results/direct_outer_strong_scaling.pdf` which
explores the scaling with P processors. So the graph should have P on
the x-axis, ["scaling"](https://en.wikipedia.org/wiki/Scalability#Weak_versus_strong_scaling) on the y-axis, and have a line for n=[64,256,1024].
Again, try to find a machine with at least 4 cores (it doesn't have
to be the machine in front of you...)

Here we will define "scaling" as follows:

    T_S = Time taken for sequential version
    T_1 = Time taken for parallel version with one processor
    T_P = Time taken for parallel version with P processors

    S = T_S / (T_P * P)

_Linear scaling_ would mean than S=1, so we added P processors
and it is P times faster. _Sub-linear_ scaling means that S<1,
so we are not getting as much out of each additional processor.

Note that I'm defining scaling against T_S, which is the
original sequential version with no overhead from TBB. It
is entirely possible (and likely) that T_S / T_1 < 1. This
is just the principle that nothing comes for free. In
order to go parallel, we have to pay something for the
libraries.


5. Using tbb::task_group in fast_fourier_transform
==================================================

The file `src/fast_fourier_transform.cpp` contains a radix-2
fast fourier transform, which takes O(n log n) operations to
do a transform. There are two main opportunities for parallelism
in this algorithm, both in `recurse()`:

1. The recursive splitting, where the function makes two recursive
   calls to itself.

2. The iterative joining, where the results from the two recursive
   calls are joined together.

We will first exploit just the recursive splitting using
`tbb::task_group`.

Overview of tbb::task_group
---------------------------

Task groups allow us to specify sets of heterogenous
tasks that can run in parallel - by heterogenous, we
mean that each of the tasks can run different code and
perform a different function, unlike `parallel_for` where
one function is used for the whole iteration space.

Task groups are declared as an object on the stack:

    #include "tbb/task_group.h"

    // Within some function, create a group
    tbb::task_group group;

you can then add tasks to the group dynamically,
using anything which looks like a nullary (zero-input)
function as the starting point:

    unsigned x=1, y=2;
    group.run( [&x](){ x=x*2; } );
	group.run( [&y](){ y=y+2; } );

After this code runs, we can't say anything about the
values of x and y, as each one has been captured by
reference (the [&]) but we don't know if they have been
modified yet. It is possible that zero, one, or
both of the tasks have completed, so to rejoin the
tasks and synchronise we need to do:

    group.wait();

After this function executes, all tasks in the group
must have finished, so we know that x==2 and y==4.

An illegal use of this construct would be for both
tasks to have a reference to x:

    // don't do this
    unsigned x=7;
    group.run( [&x](){ x=x*2; } );
	group.run( [&x](){ x=x+2; } );
    group.wait();

Because both tasks have a reference to x, any changes
in one task are visible in the other task. We don't know
what order the two tasks will run in, so the output
could be one of:

- x == (7*2)+2
- x == (7+2)*2
- x == 7*2
- x == 7+2

This would be a case of a data-race condition, which is
why you should never have two threads sharing the same
memory.

### Create and register a new class

Copy `src/fast_fourier_transform.cpp` into a new
file called `src/your_login/fast_fourier_transform_taskgroup.cpp`.
Modify the new file so that the contained class is called
`hpce::your_login::fast_fourier_transform_taskgroup`, and reports
`hpce.your_login.fast_fourier_transform_taskgroup` from name(). Apart
from renaming, you don't need to change any functionality yet.

As before, register the implementation with the implementation
in `src/fourier_transform_register_factories.cpp`, and check that
the transform still passes the test cases.

### Use tbb::task_group to parallelise the recursion

In the fast fourier transform there is a natural splitting
recursion in the section:

    recurse(m,wn*wn,pIn,2*sIn,pOut,sOut);
    recurse(m,wn*wn,pIn+sIn,2*sIn,pOut+sOut*m,sOut);

Modify the code to use tbb::task_group to turn the two
calls into child tasks. Don't worry about efficiency
yet, and keep splitting the tasks down to the point of
individual elements - splitting down to individual
elements is the wrong thing to do, as it introduces
masses of overhead, but we are establishing a base-case here.

As before, test the implementation to make sure it still
works.


6. Adjustable grain size for the FFT
====================================

Our recursive parallel FFT is currently splitting down to
individual tasks, which goes against the
[TBB advice](https://software.intel.com/en-us/node/506060#tutorial_Controlling_Chunking) about the minimum work per task:

> A rule of thumb is that grainsize iterations of operator() should take at least 100,000 clock cycles to execute.
> For example, if a single iteration takes 100 clocks, then the grainsize needs to be at least 1000 iterations.

Go back into `src/your_login/fast_fourier_transform_taskgroup.cpp`
and make the base-case adjustable using the environment variable
`HPCE_FFT_RECURSION_K`. This should adjust things at run-time,
so that if I choose:

    export HPCE_FFT_RECURSION_K=2

then the work is split down to a two-point serial FFT,
while if we do:

    export HPCE_FFT_RECURSION_K=16

then the implementation will stop parallel recursion for
at a size of 16, and switch to serial recursion (i.e. normal
function calls).

You don't want the overhead of calling `getenv` all over the
place, so I would suggest calling it once at construction
time and caching it in a member variable for the lifetime
of the FFT instance.

_Note: this is an in-place modification, rather than a new class._

**Task:** : Create a graph called `results/fast_fourier_time_vs_recursion_k.pdf`. This
should have n on the x-axis, time on the y-axis, and lines for K=[2,4,8,16,32,64].

7. Using parallel iterations in the FFT
=======================================

Making the loop parallelisable
------------------------------

The FFT contains a for loop which at first glance appears to
be impossible to parallelise, due to the loop carried dependency
through w:

    std::complex<double> w=std::complex<double>(1.0, 0.0);

    for (size_t j=0;j<m;j++){
      std::complex<double> t1 = w*pOut[m+j];
      std::complex<double> t2 = pOut[j]-t1;
      pOut[j] = pOut[j]+t1;
      pOut[j+m] = t2;
      w = w*wn;
    }

However, we can in fact parallelise this loop as long as
we exploit some mathematical properties (which should be
your forte!) and batch things carefully.
On each iteration the loop calculates w=w*wn,
so if we look at the value of w at the start of each iteration we have:

1. `j=0`, `w=1`
2. `j=1`, `w=1 * wn`
3. `j=2`, `w=1 * wn * wn`

Generalising, we find that for iteration j, `w=wn^j`.

Hopefully it is obvious that raising something to the
power of i takes substantially less than i operations.
Try calculating `(1+1e-10)^1e10` in matlab, and notice:

1. It is almost equal to _e_. Nice.
2. It clearly does not take anything like 1e10 operations to calculate.

In C++ the `std::complex` class supports the `std::pow` operator,
which will raise a complex number to a real scalar, which
can be used to jump ahead in the computation. In principle
we could use this property to make the loops completely
independent, but this will likely slow things down, as
powering is quite expensive (compared to one multiply). Instead we can use the idea
of _agglomeration_, which means instead of reducing the
code to the finest grain parallel tasks we can, we'll
group some of them into sequential tasks to reduce
overhead. Agglomeration is actually the same principle as the chunking
above, but now we have to think more carefully about
how to split into chunks.

So within each chunk `[i,j)` we need to:

1. Skip `wn` ahead to `wn^i`
2. Iterate as before over `[i,j)`

Possible factors of `K` are easy to choose in this case,
because `m` is always a power of two, so `K` can be
any power of two less than or equal to `m` (including `K=1`).

### Create and register a new class

Create a new class based on `src/fast_fourier_transform.cpp`, with:
- File name: `src/your_login/fast_fourier_transform_parfor.cpp`
- Class name: `hpce::your_login::fast_fourier_transform_parfor`
- Display name: `hpce.your_login.fast_fourier_transform_parfor`

This class should be based on the sequential version, not on the
task based version, so there is only one kind of parallelism.

### Apply the loop transformation

First apply the loop transformation described above,
_without_ introducing any parallelism, and check it works
with various values of K, via the environment variable
`HPCE_FFT_LOOP_K`.

Note that `m` gets smaller and smaller as it splits, so
you need to worry about `m < K` at the leaves of the
recursion. A simple solution is to use a guarded
version, such that if `m < = K` the original code is used,
and if `m > K` the new code is used.

Once you have got it working with a non parallel chunked
loop, replace the outer loop with a `parallel_for` loop
using `simple_partitioner`, and check that it still works
for different values of `HPCE_FFT_LOOP_K`. You will
probably not see as much speed-up here, as the dominant
cost tends to be the recursive part.

As before, if `HPCE_FFT_LOOP_K` is not set, choose a sensible
default based on your analysis of the scaling with n, and/or
experiments. Though remember, it should be a sensible default
for all machines (even those with 64 cores).

8. Combine both types of parallelism
====================================

We now have two types of parallelism that we know work,
so the natural choice is to combine them together.
Create a new implementation called `fast_fourier_transform_combined`,
using the conventions for naming from before, and integrate
both forms of parallelism. This version should check both
the `HPCE_FFT_LOOP_K` and `HPCE_FFT_RECURSION_K` variables, and
fall back on a default if either or both is not set.

**Task:** Create a graph called `results/fast_fourier_recursion_versus_iteration.pdf`,
which puts n on the x-axis, time on the y-axis, has four lines:

- `RECURSION_K=1` and `LOOP_K=1`
- `RECURSION_K=1` and the value of `LOOP_K` which seems to work best.
- `LOOP_K=1` and the value of `RECURSION_K` which seems to work best.
- The combination of `LOOP_K` and `RECURSION_K` which seems to work best.

I'm beeing vague here with "seems to work best". I'll let you resolve it.

9. Submission
=============

Double-check your names all match up, as I'll be trying
to create your transforms both by direct instantiation,
and by pulling them out of the transform registry. Also,
don't forget that "[your_login]" or `$LOGIN` does actually mean your
login (your imperial login) needs to be substituted in
wherever it is mentioned.

Specific files (with appropriately named classes) which should
exist are:

- `src/$LOGIN/direct_fourier_transform_parfor_inner.cpp`
- `src/$LOGIN/direct_fourier_transform_parfor_outer.cpp`
- `src/$LOGIN/fast_fourier_transform_parfor.cpp`
- `src/$LOGIN/fast_fourier_transform_taskgroup.cpp`
- `src/$LOGIN/fast_fourier_transform_combined.cpp`

Graphs which should exist are:

- `results/direct_inner_versus_k.pdf`
- `results/direct_outer_time_versus_p.pdf`
- `results/direct_outer_strong_scaling.pdf`
- `results/fast_fourier_time_vs_recursion_k.pdf`
- `results/fast_fourier_recursion_versus_iteration.pdf`

However, you can put other files in if you want. Performance
results are always interesting, though not required.

Hopefully this should all still be in a git repository, and you
will also have a private remote repository in the HPCE
organisation. ["Push" your local repository to your private
remote repository](git.md), making sure all the source files have
been staged and added. (You can do a push whenever you
want as you go (it's actually a good idea), but make sure
you do one for "submission"). You may want to do a "clone" into
a separate directory to make sure that everything has
made it.

After pushing to your private remote repository,
zip up your directory, including the .git, but excluding
any binary files (executables, objects), and submit
it to black-board, just in case.

I would suggest compiling and running your submission
in AWS, just to get the feel of it, and to see how it
works in a system with lots of cores. A correctly written
submission should compile anywhere, with no real dependency
on the environment, but it is good to try things out.

