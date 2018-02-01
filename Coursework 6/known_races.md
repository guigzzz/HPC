For those interested in the attempt to get rid of concurrency
bugs, this is the current state.

Things already tried
--------------------

- Search for all static and global variables across TestU01 and
  consider them for thread safety, adapting as necessary (part
  of the conversion process). Wherever possible things are converted
  to `static const`.

- Look for any "escapes" via file IO or operating system
  functions where files are written then read from.

- Run many instances of the benchmarks against each other in different
  threads with different source RNGs. Compare the output of each run
  against the previous run for exact pvalue outputs. Leave it running
  for hours with 32-threads on a 16 core machine.

- Use `valgrind` with `drd` analyser on the above soak program.

- Use `clang` with `-fsanitise=threads` on the above soak program.

Known data-races
----------------

- The posix version of `lgamma` writes to a shared-variable `signgam` (both
  valgrind and `-fsantise=threads` pick this up). This is benign, because
  no part of `TestU01` actually reads from it.

- There are some potentially racing accesses to global variables related
  to output by printing in TestU01. These are benign as long as nobody flips
  them during output, which is not how they are used (as far as I know they
  will only be modified by the user).

- If you enable printing of logging to `stdout` using `swrite_Basic` and
  friends, then output will potentially be interleaved. This can't really
  be helped due to the deep assumption of `stdout` - getting rid of it
  would require a lot of work, but actually is not that much benefit.

Potential weak points
---------------------

I'm still not absolutely sure about Crush, as it is the most
complex use-case, and takes so long to run. However I have no
reason to think it is broken, unless some kind of global
variable or side-effect was missed.

