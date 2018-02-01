#ifndef user_gaussian_blur_gpu_hpp
#define user_gaussian_blur_gpu_hpp

#define __CL_ENABLE_EXCEPTIONS 
#include "CL/cl.hpp"

#include "tbb/parallel_for.h"

#include "puzzler/puzzles/gaussian_blur.hpp"

// To go at the top of the file
#include <fstream>
#include <streambuf>

class GaussianBlurProvider_GPU
  : public puzzler::GaussianBlurPuzzle
{
public:
  GaussianBlurProvider_GPU()
  {

	std::vector<cl::Platform> platforms;
	
	cl::Platform::get(&platforms);
	if(platforms.size()==0)
			throw std::runtime_error("No OpenCL platforms found.");

	// std::cerr<<"Found "<<platforms.size()<<" platforms\n";
	for(unsigned i=0;i<platforms.size();i++){
		std::string vendor=platforms[i].getInfo<CL_PLATFORM_VENDOR>();
		// std::cerr<<"  Platform "<<i<<" : "<<vendor<<"\n";
	}

	int selectedPlatform=0;
	if(getenv("HPCE_SELECT_PLATFORM")){
		selectedPlatform=atoi(getenv("HPCE_SELECT_PLATFORM"));
	}
	// std::cerr<<"Choosing platform "<<selectedPlatform<<"\n";
	cl::Platform platform=platforms.at(selectedPlatform);  

	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);	
	if(devices.size()==0){
		throw std::runtime_error("No opencl devices found.\n");
	}
		
	// std::cerr<<"Found "<<devices.size()<<" devices\n";
	for(unsigned i=0;i<devices.size();i++){
		std::string name=devices[i].getInfo<CL_DEVICE_NAME>();
		// std::cerr<<"  Device "<<i<<" : "<<name<<"\n";
	}

	int selectedDevice=0;
	if(getenv("HPCE_SELECT_DEVICE")){
		selectedDevice=atoi(getenv("HPCE_SELECT_DEVICE"));
	} 
	// std::cerr<<"Choosing device "<<selectedDevice<<"\n";
	
	device=devices.at(selectedDevice);

	context = cl::Context(devices);

	std::string kernelSource=LoadSource("gaussian_blur_kernel.cl");

	cl::Program::Sources sources;	// A vector of (data,length) pairs
	sources.push_back(std::make_pair(kernelSource.c_str(), kernelSource.size()+1));	// push on our single string

	program = cl::Program(context, sources);

	try{
		program.build(devices);
	}catch(...){
		for(unsigned i=0;i<devices.size();i++){
			std::cerr<<"Log for device "<<devices[i].getInfo<CL_DEVICE_NAME>()<<":\n\n";
			std::cerr<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[i])<<"\n\n";
		}
		throw;
	}
}
	cl::Context context;
	cl::Program program;
	cl::Device device;

	std::string LoadSource(const char *fileName) const
	{
		std::string baseDir="provider";
		
		std::string fullName=baseDir + "/" + fileName;
		
		std::ifstream src(fullName, std::ios::in | std::ios::binary);
		if(!src.is_open())
			throw std::runtime_error("LoadSource : Couldn't load cl file from '"+fullName+"'.");
		
		// Read all characters of the file into a string
		return std::string(
			(std::istreambuf_iterator<char>(src)), // Node the extra brackets.
			std::istreambuf_iterator<char>()
		);
	}


	virtual std::string Name() const override
    { return "gaussian_blur_GPU"; }

	virtual void Execute(
			   puzzler::ILog *log,
			   const puzzler::GaussianBlurInput *input,
				puzzler::GaussianBlurOutput *output
			   ) const override
	{
		int height = (int)input->height;
		int width = (int)input->width;

		output->pixels.resize(width * height);

		int kernel_size = std::min(4*std::sqrt(input->radius),10.0);
		log->LogInfo("kernel size: %i", kernel_size);

		// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/comment-page-1/
		// https://stackoverflow.com/questions/4690756/separable-2d-blur-kernel
		// http://www.programming-techniques.com/2013/03/gaussian-blurring-using-separable.html
		// http://www.programming-techniques.com/2013/02/gaussian-filter-generation-using-cc.html
		typedef float kernel_op_type;

		size_t cbBufferInput=sizeof(cl_uchar)*width*height;
		size_t cbBufferTemp = sizeof(kernel_op_type)*width*height;
		size_t cbBufferOutput = sizeof(cl_uchar)*width*height;
		size_t cbBufferCoef = sizeof(kernel_op_type)*std::max(width, height);

		cl::Buffer buffInput(context, CL_MEM_READ_ONLY, cbBufferInput);
		cl::Buffer buffTemp(context, CL_MEM_READ_WRITE, cbBufferTemp);
		cl::Buffer buffOutput(context, CL_MEM_WRITE_ONLY, cbBufferOutput);
		cl::Buffer buffCoef(context, CL_MEM_READ_ONLY, cbBufferCoef);

		cl::Kernel horizontal_kernel(program, "horizontal_kernel");
		cl::Kernel vertical_kernel(program, "vertical_kernel");

		std::vector<kernel_op_type> coef(std::max(width, height),0);
		tbb::parallel_for(0, std::max(width, height), [&](int x){
			coef[x] = exp(-(x*x) / (2*input->radius));
		});
		
		kernel_op_type normalisor = (2 * 3.1415926535897932384626433832795 * input->radius);
		// std::cout << "Radius: " << input->radius << std::endl;
		log->LogInfo("Radius: %f", input->radius);

		horizontal_kernel.setArg(0, buffInput);
		horizontal_kernel.setArg(1, buffCoef);
		horizontal_kernel.setArg(2, buffTemp);
		horizontal_kernel.setArg(3, kernel_size);
		horizontal_kernel.setArg(4, height);
		horizontal_kernel.setArg(5, width);

		vertical_kernel.setArg(0, buffTemp);
		vertical_kernel.setArg(1, buffCoef);
		vertical_kernel.setArg(2, buffOutput);
		vertical_kernel.setArg(3, kernel_size);
		vertical_kernel.setArg(4, height);
		vertical_kernel.setArg(5, width);
		vertical_kernel.setArg(6, normalisor);

		cl::NDRange offset(0);
		cl::NDRange globalSize(height*width);
		cl::NDRange localSize=cl::NullRange;

		cl::CommandQueue queue(context, device);
		cl::Event copied_input;
		cl::Event copied_coeffs;
		queue.enqueueWriteBuffer(buffInput, CL_TRUE, 0, cbBufferInput, &input->pixels[0], NULL, &copied_input);
		queue.enqueueWriteBuffer(buffCoef, CL_TRUE, 0, cbBufferCoef, &coef[0], NULL, &copied_coeffs);

		// Horizontal Pass
		std::vector<cl::Event> horizontalKernelDependencies = {copied_input, copied_coeffs};
		cl::Event ExecutedHorizontalKernel;
		queue.enqueueNDRangeKernel(horizontal_kernel, offset, globalSize, localSize, &horizontalKernelDependencies, &ExecutedHorizontalKernel);

		// Vertical Pass
		std::vector<cl::Event> verticalKernelDependencies = {ExecutedHorizontalKernel};
		cl::Event ExecutedVerticalKernel;
		queue.enqueueNDRangeKernel(vertical_kernel, offset, globalSize, localSize, &verticalKernelDependencies, &ExecutedVerticalKernel);

		// Get Results
		std::vector<cl::Event> copyback = {ExecutedVerticalKernel};
		queue.enqueueReadBuffer(buffOutput, CL_TRUE, 0, cbBufferOutput, &output->pixels[0], &copyback);
	}

	virtual bool CompareOutputs(
      puzzler::ILog* log,
      const puzzler::GaussianBlurInput* input,
      const puzzler::GaussianBlurOutput* ref,
      const puzzler::GaussianBlurOutput *got) const override
    {
      int errors = 0;
      bool ok = true;
      for(unsigned i=0; i<ref->pixels.size(); i++){
        double r=ref->pixels[i], g=got->pixels[i];
        if( std::abs(r-g) > 2 ){
          errors++;
		//   std::cout << "y: " << std::floor(i / (float)input->width) << ", x: " << i % input->width << ", ref: " << r << ", got: " << g << std::endl;
		  log->LogInfo("y: %i, x: %i, ref: %i, got: %i", (int)std::floor(i / (float)input->width), i % input->width, (int)r, (int)g);
          ok = false;
        }
      }
    //   std::cout << "errors: " << errors << std::endl;
	  log->LogInfo("number of errors: %i", errors);
      return ok;
    }

};

#endif


