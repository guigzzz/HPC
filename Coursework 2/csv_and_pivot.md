Automation is a great way of helping to gather results,
and it can also help you get the data into a format that
you can experiment with. It also makes it much easier
to get _lot's_ of data. A common problem in final year
projects are graphs that only have four data-points, or
only explore one parameter. A compelling and convincing
graph should have enough data-points to make the case
that there is a genuine trend there.

## Gathering data in CSV

Notice that `bin/time_fourier_transform` has four parameters
(try running it with no parameters again):

    1. `name` : the implementation to benchmark.

    2. `P` : the number of CPUs to enable, where 0 means use all CPUs available.

    3. `maxTime` : total time to allow for all runs.

    4. `prefix` : a prefix which will precede each output line.

Also, notice that each output row is a row of comma seperated values, which
means the output is _also_ a csv file. If you do:

    bin/time_fourier_transform hpce.direct_fourier_transform 0 1 > dump.csv

then you'll end up with a file called `dump.csv` (I'm temporarily using
a `maxTime` of 1, for interactive use). This csv can then be loaded
into Excel, OpenOffice, matlab, python, etc., and used to explore
the data or plot graphs.

If you do:

    HPCE_DIRECT_INNER_K=8
    bin/time_fourier_transform hpce.[YOUR_LOGIN].direct_fourier_transform_parfor_inner 0 1 "${HPCE_DIRECT_INNER_K}, "

then you'll see that you have:

- selected a specific K; and
- benchmarched with that K; and
- prefixed the value of K to the output of each csv row.

Going further, you can do:

    HPCE_DIRECT_INNER_K=8
    bin/time_fourier_transform hpce.[YOUR_LOGIN].direct_fourier_transform_parfor_inner 0 1 "${HPCE_DIRECT_INNER_K}, " > dump_${HPCE_DIRECT_INNER_K}.csv

and you'll end up with a file called `dump_8.csv`, containing all the rows
for the run with K=8. If you want to peek inside, you can do:

    less dump_8.csv

Use `q` to exit `less`.

We can combine this with iteration in the shell:

    # Create a local variable with a list of K values
    KS="1 2 3 4 5"
    # Iterate over them and print (echo) them
    for K in $KS; do
        echo $K;
    done

Bringing both of those together we can run the program for
multiple K values, and have them written to csv. At this
point the shell command will be getting very unwieldy. I
suggest you create a file called `results/direct_parfor_inner_versus_k.sh`,
and then add the following:

    #!/bin/bash
    # The above shows that it is a bash file

    # Create a local variable with a list of K values
    KS="1 2 3 4 5"
    # Iterate over them and print (echo) them
    for K in $KS; do
        # Select the specific value of K and export to other programs
        export HPCE_DIRECT_INNER_K=${K}
        # Run the program with the chosen K, and save to dump_K.csv
        bin/time_fourier_transform hpce.[YOUR_LOGIN].direct_fourier_transform_parfor_inner 0 1 "${HPCE_DIRECT_INNER_K}, " > dump_${HPCE_DIRECT_INNER_K}.csv
    done

If you are in "true" unix, you will then need to do:

    chmod u+x results/direct_parfor_inner_versus_k.sh

This indicates that the user (`u`), i.e. you, should
be able to execute (`x`) this file.

You can then run it using:

    results/direct_parfor_inner_versus_k.sh

It may take some time to run, but you will see the
files called `dump_1.csv`, `dump_2.csv` and so
on start to be written.

Ok - I sense you are not impressed. It is all still
in files, not in a graph. What is the point?

Well first, you can choose an arbitrary set of K values,
and run it when you are away from the computer. For
example, if you run the [seq](https://en.wikipedia.org/wiki/Seq_(Unix)) command:

    seq 100

you'll get the numbers from 0 to 99 back.

If you do:

    KS=$(seq 100)

then `KS` will contain the output of seq, as the `$(command)`
syntax means that the output of `command` is captured. You
can then do:

    echo $KS

Combining this with the above automation, you should be
able to see how to get up to a dense spread of K values.

However, this still leaves us with a bunch of files,
each of which contains many rows for the same K.
If we take all of those rows across all the files,
we have all the data needed to plot a graph.

If you run:

    cat dump_*.csv

then it will take all the rows and dump to the screen,
in what looks like one long file.

So if instead you run:

    cat dump_*.csv > all.csv

it will take all those rows and send them to a file
called `all.csv`. Now we are ready to pivot!

### Pivot!

Excel is one tool that has good pivot features, but
open office and google spreadshets have it too,
and you can do it in matlab or python as well (as long as you
are willing to code it). If you have a csv file, with a number of
columns, then pivot tables let you:

- Select a data column for the x-axis

- Select a data column which will be plotted on the y-axis

- Choose a subset of rows to plot as lines, based on a filter
  over the columns.

You kind of need to play with it, but it is very valuable
for scenarios where you have a lot of data and
want to view it in different ways.

A rough guide is:

1. Open your .csv into Excel. It should automatically
   turn it into a table. If you paste csv data into
   excel, you can use "text to columns" to turn it into columnar data.

2. Add named headers to the columns, so shift all the data
   down by one row, and put meaningful names there.

3. Select all the data and choose "insert -> pivot-chart".

4. Select the columns you want on as the x-axis, values,
   and categories (things in the "PivotTable list").

5. Customise the selected columns and chart format as required.

I did a brief [video review](https://imperial.cloud.panopto.eu/Panopto/Pages/Viewer.aspx?id=40cf42c4-b801-4574-95d6-ac31f59da719) of this on panopto, as it is a GUI thing.

Pivot charts are great for exploring data, and make it
easier to redraw or re-examine the same data-set in
different ways.

