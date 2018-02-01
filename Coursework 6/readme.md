HPCE 2017 CW6
=============

_(2017/12/04 : Clarifying (again) the stress penalty, to make the documentation match the code._

_(2017/11/28 : See the end of the readme for some additional notes)_

The goal of this coursework is to take an existing library
called [TestU01](http://simul.iro.umontreal.ca/testu01/tu01.html),
and parallelise, optimise, or otherwise accelerate it.

TestU01 is used for testing random number generators (RNG), and
tries to empirically look for statistical flaws in the output of
an RNG that are not likely to occur by chance. As RNGs have improved,
the number of samples needed to show non-randomness has
also increased, so these days we might need to look at 2^38 or
more random numbers before seeing failures, and apply many different
types of statistical tests.  But because the random numbers are often
used in massively parallel compute simulations, the number of
random numbers consumed in one simulation is growing to 2^48 or
more, so it is important to look for flaws in even larger samples.

There is substantial documentation for TestU01 in the included
pdfs. It covers the main APIs, design intent, and also the details
of the statistics. It is up to you to decide how deep you want
to go. Originally TestU01 was single-threaded C, but it has been
converted to C++ and made thread-safe. The only remaining unsafe
part I'm aware of is the `unif01_Gen` struct - **you cannot safely use
the same `unif01_Gen` class from two different tasks or threads
at the same time.** You can use two _different_ instances of
`unif01_Gen` at the same time, but they must be seperate.

Your goal is to optimise the TestU01 library with three use-cases
in mind (or two for a pair):

- [Certification](drivers/driver_certify.cpp): making sure that an RNG can pass a relatively slow
  standardised test-suite, in this case the Crush benchmark. The
  metric is simply the execution time (latency).
  
- [Search](drivers/driver_search.cpp): evaluating many different RNG instances in order to search
  for good RNG parameters. The metric is how many times the SmallCrush
  benchmark can be executed per second, which is measured as the number
  of complete test results printed to stdout, divided by the amount of
  time the program is allowed to run before the user terminates it (this
  will be of the order of 10s of seconds).

- [Stress testing](drivers/driver_stress.cpp) : Trying to test as large a sample
  as possible within a time budget, using the benchmark Rabbit. Given a
  time-budget `t` seconds [of between 2 and 180 seconds](https://github.com/HPCE/hpce-2017-cw6/issues/27)
  (passed as a command-line parameter), your program should
  respond within 0.5 sec with a bid size `n` printed to `stdout`, and then
  proceed to apply the test. If the total measured execution time of the
  program is `g` seconds, then the metric is `log2(n) * ( (g/t<1) ? 1.0 : exp(-g/t) ) `.
  The idea is that you should achieve the largest `n` possible within the
  time budget, [but that you are penalised for exceeding that budget](https://github.com/HPCE/hpce-2017-cw6/issues/19).
  See [drivers/driver_stress.cpp](drivers/driver_stress.cpp) as a model
  of the logic (though feel free to change the internals), and also see
  the discussion [here]() for an explanation of the intent.

As part of the input to the use-cases there is also an RNG source
called the "workload" (see `include/workload.hpp`). This is code
supplied by the user, and not under your control. So the inputs to
an application are:

- Your (potentially customised) TestU01 library

- Your (potentially customised) driver program

- An externally sourced workload containing the RNG(s)

There is an example workload in the `workloads` directory called
`workload_std.cpp`, which exposes two random number generators
from the C++ standard library. The default makefile knows
how to build `bin/${use-case}_${workload}` from `drivers/driver_${use-case}.cpp`
and `workload/workload_${workload}.cpp`.

Stress testing for `std` workload with a random deadline:
```
$ make bin/stress_std
$ bin/stress_std
```
or with a chosen deadline of 10 seconds:
```
$ make bin/stress_std
$ bin/stress_std 10.0
```

Search for `std` workload:
```
$ make bin/search_std
$ bin/search_std
```

Certification for `std` workload:
```
$ make bin/certify_std
$ bin/certify_std
```

Compilation for assesment will consist of copying a workload
into `workloads`, then running `make bin/{use-case}_{workload}`.
Execution will start from the root directory of the repository,
as in the examples above.

Correctness
-----------

Correctness for statistical testing is quite interesting, as it doesn't
matter which part of an RNG's output stream is tested, as long as each
test only looks at some contiguous and in-order part of the stream. So
if one test consumes `n` random number, then it could consume `x_1,x_2,...x_{n}`,
or `x_{n},x_{n+1},...,x_{2n}` - statistical quality should be invariant
to where you look in the stream, However, it does depend on the order
in which you look, so you can't test `x_1,x_4,x_10,x_3,...` as randomness
also depends on the ordering of the samples.

One approach to correctness is to return bit-exact answers compared to
the sequential version, which makes verification easier. However, it
also limits flexibility and is a bit unrealistic. Instead, as long as
your tests still give the same statistical answer, then it is acceptable.
Each test returns a p-value, and they should behave in a particular
way statistically - if the RNG is passing the test, then they look
like uniform numbers in U(0,1), otherwise they are heavily biased towards
0 or 1. I'll be testing your implementations on different workloads,
and have a very good idea of what the distribution of p-values should
look like for each (i.e. I can tune workloads to particular known p-value
distributions), so as long as your tests are statistically plausible
then they are correct.

This gives you the freedom to play around with algorithms and floating-point
issues, but if you want to stick to bit-exact answers that should
work too.

Non thread-safe Bug bounty
----------------------

As far as I'm aware I've eliminated all thread unsafe conditions
from TestU01, but there may still be things lurking. I've done
soak tests on multi-core machines where I run the benchmarks
repeatedly in parallel with themselves, but experience suggests something
else is likely to turn up. To encourage people to share (as this
is particularly difficult to deal with for those weaker on programming),
I'm willing to award up to a 2% bonus (for this coursework) to groups
who find something that is thread unsafe and can point to the problem
area (and ideally a fix). I'm not too worried about latent bugs, or
problems with IO paths that aren't exercised (I'm aware that the
standard report writing is interleaved), but anything that
causes genuine race conditions or thread-safety related crashes
crashes is of particular interest.

General bug-reports and bug-fixes are still very much welcome, but
I particularly want to reward any thread-unsafe parts as they tend
to be difficult to debug and affect many people, and generally
demonstrate the most expertise on the part of those who can find
and fix them.
  
Assessment
----------

The target environment is either a `g3.4xlarge` (note that this is
newer than in previous courseworks), or a `c4.8xlarge`. You
should indicate in `group.md` which one you want to use.
Two additional libraries will be available [see #17](https://github.com/HPCE/hpce-2017-cw6/issues/17):
- boost
- CUDA

The marks for the coursework are split up into three parts:

- Performance (60%) : How well it does in the two (pairs) or three (triples) use-cases.
                      As before, each pair should nominate two uses-cases.

- Group analsis (20%) : Each group needs to provide two (pairs) or three (triples)
                        suggestions for improvement - see the attached `group.md`.

- Individual analysis (20%) : Each individual should separately submit a 
                       document `reflection.md` via blackboard (this is something that
                       is looking back, so is not included yet).

Getting started
---------------

You may be confused about how to get started. Some suggestions are:

- Compile and run the programs.

- Get some initial high-level profiling, e.g. using perf or gdb

- Try to get a feel for the scaling properties of each use-case.

- Try to work out the _important_ tasks in the program, and
  the flows of data between different tasks.
  
- Brain-storm some possible strategies for improvement, and try to
  identify the effort versus improvement of each.
  
- Allocate strategies to people and get started.

Additional notes
----------------

### Problem complexity (2017/11/29)

A few people have been getting a bit frustrated, or worried
that they can't immediately see what to do (while I can see
that others are happily ticking along with parallelising, so
it is not a uniform experience).

There is no new information to come (as we have no more
lectures), and the only big hint is in the existing readme
and the way the coursework is setup (it requires thinking
about the problem at a high-level).

It is a difficult task, and part of the approach is about
being realistic. So some generic advice/observations:

- Don't start at the lowest-level. Initial profiling is not
  supposed to help you optimise, it is supposed to help
  you understand where the hot-spots are. 

- You shouldn't be expecting huge easy speed-ups, as that doesn't
  happen in most applications. There are a number of
  initial opportunities that can be identified (some of
  which are very effective), but each one requires changing
  the code in a few places, and thinking carefully about
  whether it is safe.

- There is no correct answer or single point of attack. I
  have identified multiple sources of speed-up in each use-case
  (and exploited them), so I know they are there. They also
  align well with the lectures and concepts taught in the course,
  but you have to look for them.

- Some use-cases are intrinsically harder than others, due to the
  types of potential parallelism and optimisations at play.
  Some can be made to scale linearly, some can't, but they can
  all be made faster with parallelism, and faster still with
  restructuring and optimisation.

- You need to decide an appropriate level of time investment,
  then stop. The EEE guidelines for a 100% coursework course
  are an **upper-bound** of 150 hours total, so about 130 hours
  max on the coursework parts. Hopefully most people spend much
  less than this - personally I would probably invest at most
  20 hours per-person of _active_ time in CW6.
  
- Time invested is often more effective when it is spread
  around. One of the use-cases takes significant time to
  run (particularly before initial parallelisation), so you
  should be thinking of running experiments while you're
  doing something else, then coming back to the results later on.
  It is your _active_ time that is important, so only when you are
  thinking and doing - if you are staring at a screen watching
  results tick past (or waiting for results) for more than 
  1 min, then you might rethink how you are working. It is
  sometimes better to decide to wait 1 hour and get rich results,
  rather than limit yourself to shallow results limited by
  how long you are willing to wait for things to run. 

- The actual performance part of the the coursework is only
  worth 60% of the marks, and that is across all three (or two)
  use-cases. If you can't get much speed-up it is not a disaster
  (similar to not being able to answer 20% of an exam, when you've
  already completed 80% of the exam's questions, and know that
  you aced the first 40%).

- Ultimately, it's a masters course where everybody has already
  got 40% built-in - parts of the assesment need to actually
  test skill and insight, and whether people can generalise
  the approaches and solutions in the course to new problems.
  
