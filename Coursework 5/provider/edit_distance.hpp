#ifndef user_edit_distance_hpp
#define user_edit_distance_hpp

#include "puzzler/puzzles/edit_distance.hpp"
#include "tbb/task_group.h"
#include "tbb/task_scheduler_init.h"

class EditDistanceProvider
  : public puzzler::EditDistancePuzzle
{
public:
	// int selectedAlgo=0;
  EditDistanceProvider()
  {
	  
		// if(getenv("HPCE_ED_ALGO")) selectedAlgo=atoi(getenv("HPCE_ED_ALGO"));
  }

	virtual void Execute(
		puzzler::ILog *log,
		const puzzler::EditDistanceInput *input,
		puzzler::EditDistanceOutput *output
		) const override
	{
		unsigned m=input->s.size();
		unsigned n=input->t.size();
		std::vector<uint8_t> s=input->s; // Length m
		std::vector<uint8_t> t=input->t; // Length n

		unsigned distance = 0;

		log->LogInfo("Problem Info: m: %u, n: %u", m, n);

		// if(selectedAlgo == 2){
		// 	log->LogInfo("Using algorithm: Optimised reference");

		// 	std::vector<unsigned> d0(n + 1);
		// 	std::vector<unsigned> d1(n + 1);

		// 	for(unsigned j=0; j<=n; j++){
		// 		d0[j] = j;
		// 	}

		// 	for(unsigned i = 0; i < m; i++){
		// 		d1[0] = i + 1;
		// 		for(unsigned j = 0; j < n; j++){
		// 			d1[j + 1] = std::min(1 + std::min(d1[j], d0[j + 1]), d0[j] + (s[i] != t[j]));
		// 		}
		// 		std::swap(d0, d1);
		// 	}

		// 	distance=d0[n];

		// }else if(selectedAlgo == 1){
		// 	log->LogInfo("Using algorithm: Skewed Loop");

		// 	if(n < m){ 
		// 		// transpose matrix
		// 		log-> LogVerbose("n < m, tansposing implicit matrix...");
		// 		std::swap(s, t);
		// 		std::swap(n, m);
		// 		log-> LogVerbose("new m: %u, new n: %u", m, n);
		// 	}

		// 	std::vector<unsigned> dStg( size_t(m+1)*(n+1) );

		// 	// Helper to provide 2d accesses
		// 	auto d=[&](unsigned i,unsigned j) -> unsigned &
		// 	{
		// 		return dStg[i*size_t(n+1)+j];
		// 	};
			

		// 	for(unsigned i=0; i<=m; i++){
		// 		d(i,0) = i;
		// 	}

		// 	for(unsigned j=0; j<=n; j++){
		// 		d(0,j) = j;
		// 	}

		// 	auto update_dist = [&](unsigned x, unsigned y){
		// 		d(x,y) = std::min(1 + std::min( d(x-1,y), d(x,y-1) ), d(x-1,y-1) + (s[x-1] != t[y-1]));
		// 	};

		// 	for(unsigned i=1; i<=m; i++){
		// 		for(unsigned j=1; j<=i; j++){
		// 			unsigned x = i-j+1;
		// 			unsigned y = j;
		// 			update_dist(x,y);
		// 		}
		// 	}

		// 	for(unsigned i = m+1; i<= n; i++){
		// 		for(unsigned j = 1; j <= m; j++){
		// 			unsigned x = m-j+1;
		// 			unsigned y = i - m + j;
		// 			update_dist(x,y);
		// 		}
		// 	}

		// 	for(unsigned i=n+1; i<=n+m-1; i++){
		// 		for(unsigned j=1; j<=n+m-i; j++){
		// 			unsigned x = m-j+1;
		// 			unsigned y = i-m+j;
		// 			update_dist(x,y);
		// 		}
  		// 	}

		// 	distance = d(m, n);
			
		// }else{ // == 0
		// log->LogInfo("Using algorithm: Blocked skewed Loop");
		
		unsigned num_threads = tbb::task_scheduler_init::default_num_threads();
		tbb::task_group group;

		unsigned blocked_m = num_threads * 8;
		unsigned blocked_n = num_threads * 8;

		if(m > 2000 || n > 2000){

			std::vector<unsigned> dStg( size_t(m+1)*(n+1) );

			auto d=[&](unsigned i,unsigned j) -> unsigned &
			{
				return dStg[i*size_t(n+1)+j];
			};
			

			for(unsigned i=0; i<=m; i++){
				d(i,0) = i;
			}

			for(unsigned j=0; j<=n; j++){
				d(0,j) = j;
			}

			unsigned block_size_rows = m / blocked_m;
			unsigned block_size_cols = n / blocked_n;

			auto compute_block = [&](unsigned x, unsigned y){
				unsigned num_rows = block_size_rows;
				unsigned num_cols = block_size_cols;

				if (x == blocked_m - 1) num_rows += m % blocked_m;
				if (y == blocked_n - 1) num_cols += n % blocked_n;

				unsigned offset_x = x * block_size_rows;
				unsigned offset_y = y * block_size_cols;

				std::vector<unsigned> d0(num_cols + 1);
				std::vector<unsigned> d1(num_cols + 1);

				for(unsigned j=0; j<=num_cols; j++){
					d0[j] = d(offset_x, offset_y + j);
				}

				for(unsigned i = 1; i < num_rows; i++){
					d1[0] = d(offset_x + i, offset_y);

					for(unsigned j = 1; j <= num_cols; j++){
						d1[j] = std::min(1 + std::min(d0[j], d1[j - 1]), d0[j - 1] + (s[offset_x + i - 1] != t[offset_y + j - 1]));
					}
					d(offset_x + i, offset_y + num_cols) = d1[num_cols];

					std::swap(d0, d1);
				}

				d1[0] = d(offset_x + num_rows, offset_y);
				for(unsigned j = 1; j <= num_cols; j++){
					d1[j] = std::min(1 + std::min(d1[j - 1], d0[j]), d0[j - 1] + (s[offset_x + num_rows - 1] != t[offset_y + j - 1]));
					d(offset_x + num_rows, offset_y + j) = d1[j];
				}
			};

			for(unsigned i=1; i<=blocked_m ; i++){
				for(unsigned j=1; j<=i; j++){
					unsigned x = i-j+1;
					unsigned y = j;
					group.run([=](){
						compute_block(x - 1, y - 1);
					});
				}
				group.wait();
			}

			for(unsigned i = blocked_m+1; i<= blocked_n; i++){
				for(unsigned j = 1; j <= blocked_m; j++){
					unsigned x = blocked_m-j+1;
					unsigned y = i - blocked_m + j;
					group.run([=](){
						compute_block(x - 1, y - 1);
					});
				}
				group.wait();
			}

			for(unsigned i=blocked_n+1; i<=blocked_n+blocked_m-1; i++){
				for(unsigned j=1; j<=blocked_n+blocked_m-i; j++){
					unsigned x = blocked_m-j+1;
					unsigned y = i-blocked_m+j;
					group.run([=](){
						compute_block(x - 1, y - 1);
					});
				}
				group.wait();
  			}

			distance = d(m, n);

		}
		else{
			log->LogInfo("Problem size too small, falling back to optimised reference");
			std::vector<unsigned> d0(n + 1);
			std::vector<unsigned> d1(n + 1);

			for(unsigned j=0; j<=n; j++){
				d0[j] = j;
			}

			for(unsigned i = 0; i < m; i++){
				d1[0] = i + 1;
				for(unsigned j = 0; j < n; j++){
					d1[j + 1] = std::min(1 + std::min(d1[j], d0[j + 1]), d0[j] + (s[i] != t[j]));
				}
				std::swap(d0, d1);
			}

			distance=d0[n];
		}
		// }
		
		log->LogInfo("Distance = %u", distance);
		output->distance=distance;
	}
};

#endif
