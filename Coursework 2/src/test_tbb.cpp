#include "tbb/parallel_for.h"

#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdio.h>

int main()
{
    int n=100000;
    std::vector<double> parts(n);

    tbb::parallel_for(0, n,
        [&](int i){
            double acc=0;
            for(int j=0; j<i; j++){
                acc+=sin(double(j)/i);
            }
            parts[i]=acc;
        }
    );

    double res=std::accumulate(parts.begin(), parts.end(), 0.0);

    fprintf(stdout, "sum( sum(sin(j/i), j=0..i-1, i=0..%d-1 ) = %f\n", n, res);

    return 0;
}
