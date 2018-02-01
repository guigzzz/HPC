#ifndef user_mining_hpp
#define user_mining_hpp

#include "puzzler/puzzles/mining.hpp"
#include "tbb/task_group.h"
#include "tbb/task_scheduler_init.h"
#include "mining_GPU.hpp"

#include <chrono>
using namespace std::chrono;

class MiningProvider
  : public puzzler::MiningPuzzle
{
public:
  MiningProvider()
	{
		MiningGPU=std::make_shared<MiningGPUProvider>();
	}	

	std::shared_ptr<MiningGPUProvider> MiningGPU;

	// static uint64_t TEA_hash (uint64_t v, const uint32_t *k, unsigned rounds) {
    //   uint32_t v0=v&0xFFFFFFFFull, v1=v>>32, sum=0, i;           /* set up */
    //   uint32_t delta=0x9e3779b9;                     /* a key schedule constant */
    //   uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */
      
    //   uint64_t res=1234567801234567ull;
    //   for (i=0; i < rounds; i++) {                       /* basic cycle start */
    //       sum += delta;
    //       v0 += ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
    //       v1 += ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
    //       res=((res << 7) ^ (res>>57)) + (v0&v1);
    //   }     /* end cycle */
    //   return res;
	// 	}

	virtual void Execute(
			   puzzler::ILog *log,
			   const puzzler::MiningInput *input,
				puzzler::MiningOutput *output
	) const override
	{
		// if(input->threshold > std::pow(2.0, 64) / (400 * 400)){
		// 	log->LogInfo("Scale too small defaulting to reference implementation");
		// 	ReferenceExecute(log, input, output);
		// }
		// else{
			// log->LogInfo("Using GPU mining");
			MiningGPU->Execute(log, input, output);
		// }
		// high_resolution_clock::time_point t1 = high_resolution_clock::now();

		// bool need_break = false;
		// // tbb::atomic<uint64_t> best = 0xFFFFFFFFFFFFFFFFull;
		// // tbb::atomic<uint64_t> total_hashes = 0;

		// unsigned num_threads = tbb::task_scheduler_init::default_num_threads();
		// tbb::task_group group;
		// for(unsigned i = 0; i < num_threads; i++){

		// 	group.run( [&](){ 
		// 		std::mt19937_64 iter((time_t)i);
		// 		// uint64_t local_hashes = 0;
		// 		uint64_t found_i = 0;
		// 		while(1){
		// 			for(int j = 0; j < 10; j++){
		// 				uint64_t i=iter();
		// 				uint64_t got=TEA_hash(i, &input->key[0], input->rounds);
		// 				found_i = (got < input->threshold) ? i : 0;
		// 			}

		// 			if(need_break) break;
		// 			else if(found_i)
		// 			{
		// 				need_break = true;
		// 				output->input=found_i;
		// 				break;
		// 			}
		// 		}
		// 		group.cancel();
		// 		// total_hashes += locWWal_hashes;
		// 	});

		// }
		// group.wait();
		// high_resolution_clock::time_point t2 = high_resolution_clock::now();
		// duration<uint64_t> time_span = duration_cast<duration<uint64_t>>(t2 - t1);
		// std::cout << "hashes per second (new): " << total_hashes / time_span.count() << std::endl;
		// MiningPuzzle::ReferenceExecute(log, input, output);

		
	}

};

#endif
