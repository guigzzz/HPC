#ifndef user_random_projection_hpp
#define user_random_projection_hpp

#include "puzzler/puzzles/random_projection.hpp"
#include "tbb/parallel_for.h"
#include "random_projection_gpu.hpp"

class RandomProjectionProvider
  : public puzzler::RandomProjectionPuzzle
{
public:
  RandomProjectionProvider()
  {
	ProjectionGPU = std::make_shared<RandomProjectionGPUProvider>();
  }
  std::shared_ptr<RandomProjectionGPUProvider> ProjectionGPU;

	static uint32_t lcg(uint32_t s)
	{
		return s*1664525+1013904223;
	}

	virtual void Execute(
			   puzzler::ILog *log,
			   const puzzler::RandomProjectionInput *input,
				puzzler::RandomProjectionOutput *output
			   ) const override
	{
		// if(input->n < 100){
		// 	log->LogInfo("Scale too small, defaulting to TBB version");
		// 	unsigned n=input->n;
		// 	double p=std::pow(2.0, 28)/n;

		// 	std::vector<uint64_t> acc(n * n, 0);
		// 	std::vector<uint32_t> random_numbers(n, 0);
		// 	std::mt19937 rnd(input->seed);

		// 	log->LogInfo("Creating input vector of size %u", n);
		// 	// std::vector<uint32_t> v(n);
		// 	// for(unsigned i=0; i<n; i++){
		// 	// 	v[i]=rnd()%2;
		// 	// }

		// 	size_t v_size = (uint32_t)ceil(n / 32.0);
		// 	std::vector<uint32_t> v(v_size, 0);
		// 	uint32_t counter = -1;

		// 	for(unsigned i = 0; i < n; i++){
		// 		if(! (i % 32)){
		// 			counter++;
		// 		}
		// 		v[counter] |= (rnd() % 2) << (i % 32);
		// 	}

		// 	// generate random numbers in advance
		// 	// comparison needs exact equivalence between reference and this implementation
		// 	// there is no guarantee of thread-safety when the same mersenne twister is used between all threads.
		// 	for(unsigned i = 0; i < n; i++){
		// 		random_numbers[i] = rnd();
		// 	}

		// 	auto kernel_function = [](
		// 		std::vector<uint64_t> &acc,
		// 		std::vector<uint32_t> &v,
		// 		std::vector<uint32_t> &random_numbers,
		// 		uint32_t n,  double p,
		// 		uint32_t i, uint32_t y
		// 		){
		// 			uint32_t seed = random_numbers[i] + y * n * 19937; //fast forward seed

		// 			uint64_t a = 0;
		// 			uint32_t counter = -1;
		// 			uint32_t curv;
		// 			for(uint32_t x=0; x<n; x++){
		// 				if(! (x % 32)){
		// 					counter++;
		// 					curv = v[counter];
		// 				}
		// 				uint32_t r=lcg(seed);
		// 				uint32_t rHi=r>>8, rLo=r&0xFF;
		// 				// a += (rHi < p) ? (rLo * v[x]) : 0;
		// 				a += ((rHi < p) && (curv & (1 << (x % 32)))) ? rLo : 0;
		// 				seed+=19937;
		// 			}
		// 			acc[i * n + y] = a;
		// 	};

		// 	log->LogInfo("Beginning projections");
		// 	tbb::parallel_for(0u, n, [&](unsigned i){
		// 	// for(unsigned i = 0; i < n; i++){

		// 		log->LogVerbose("Projection %u of %u", i, n);
		// 		for(unsigned y=0; y<n; y++)
		// 			kernel_function(acc, v, random_numbers, n, p, i, y);
		// 	});

		// 	std::vector<uint64_t> out(n, 0);
		// 	for(unsigned i=0; i<n; i++){
		// 		for(unsigned j=0; j<n; j++){
		// 			out[i] += acc[j*n+i];
		// 			// std::cout << acc[i*n+j] << " ";
		// 		}
		// 		// std::cout << std::endl;
		// 	}
		// 	// std::cout << std::endl;
		// 	output->acc=out;

		// 	log->LogInfo("Finished");
		// }
		// else{
			// log->LogInfo("Scale OK, using GPU version");
			ProjectionGPU->Execute(log, input, output);
		// }
	}

};

#endif
