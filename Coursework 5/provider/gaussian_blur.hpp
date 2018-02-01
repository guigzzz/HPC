#ifndef user_gaussian_blur_hpp
#define user_gaussian_blur_hpp

#include "tbb/parallel_for.h"
#include "puzzler/puzzles/gaussian_blur.hpp"
#include "gaussian_blur_GPU.hpp"

class GaussianBlurProvider
  : public puzzler::GaussianBlurPuzzle
{
public:
  GaussianBlurProvider()
  {
	  BlurGPU=std::make_shared<GaussianBlurProvider_GPU>();
	//   if(getenv("HPCE_EM_GAUSS_THRESH")) threshold=atoi(getenv("HPCE_EM_GAUSS_THRESH"));
  }
  uint32_t threshold = 300;
  std::shared_ptr<GaussianBlurProvider_GPU> BlurGPU;

	virtual void Execute(
			   puzzler::ILog *log,
			   const puzzler::GaussianBlurInput *input,
				puzzler::GaussianBlurOutput *output
			   ) const override
	{
		if(input->height < threshold){
			log->LogInfo("Scale too small, defaulting to TBB version");
			int height = (int)input->height;
			int width = (int)input->width;

			output->pixels.resize(width * height);

			int kernel_size = std::min(4*std::sqrt(input->radius), 20.0);

			// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/comment-page-1/
			// https://stackoverflow.com/questions/4690756/separable-2d-blur-kernel
			// http://www.programming-techniques.com/2013/03/gaussian-blurring-using-separable.html
			// http://www.programming-techniques.com/2013/02/gaussian-filter-generation-using-cc.html
			
			std::vector<double> temp(width * height,0.0);
			std::vector<double> coef(std::max(width, height),0);

			tbb::parallel_for(0, std::max(width, height), [&](int x){
				coef[x] = exp(-(x*x) / (2*input->radius));
			});

			double normalisor = (2 * 3.1415926535897932384626433832795 * input->radius);
			log->LogInfo("Radius: %f", input->radius);

			tbb::parallel_for(0, height, [&](int yOut){
				log->LogVerbose("row = %u", yOut);
				for(int xOut=0; xOut < width; xOut++){

					double acc=0.0;
					for(int i=std::max(xOut-kernel_size,0); i < std::min(xOut+kernel_size,width); i++){
						acc += coef[abs(i-xOut)] * input->pixels[yOut*width+i];
					}

					temp[ width*yOut+xOut ] = acc;
				}
			});

			tbb::parallel_for(0, width, [&](int xOut){
				log->LogVerbose("column = %u", xOut);
				for(int yOut=0; yOut < height; yOut++){

					double acc=0.0;
					for(int i=std::max(yOut-kernel_size,0); i<std::min(yOut+kernel_size,height); i++){
						acc += coef[abs(i-yOut)] * temp[i*width+xOut];
					}

					acc /= normalisor; 

					if(acc<0){
						acc=0;
					}else if(acc>255){
						acc=255;
					}

					output->pixels[ width*yOut+xOut ] = (uint8_t)acc;
				}
			});

		}else{
			log->LogInfo("Scale OK, using GPU version");
			BlurGPU->Execute(log, input, output);
		}
	}

};

#endif


