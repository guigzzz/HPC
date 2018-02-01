#ifndef user_random_projection_gpu_hpp
#define user_random_projection_gpu_hpp

#include "puzzler/puzzles/random_projection.hpp"
#include "tbb/parallel_for.h"
#include "CL/cl.hpp"

#include <stdexcept>
#include <cstdint>
#include <cstdio>
#include <string>
#include <fstream>
#include <streambuf>
#include <math.h>


void addDefinition(std::string& source, char const* id, double value)
{
	char buf[256];
	sprintf(buf, "#define %s %f\n", id, value);
	source.insert(source.begin(), buf, buf + strlen(buf));
}

class RandomProjectionGPUProvider
  : public puzzler::RandomProjectionPuzzle
{
public:
  RandomProjectionGPUProvider()
  {
      std::vector<cl::Platform> platforms;
	
		cl::Platform::get(&platforms);
		if(platforms.size()==0)
			throw std::runtime_error("No OpenCL platforms found.");

		for(unsigned i=0;i<platforms.size();i++){
			std::string vendor=platforms[i].getInfo<CL_PLATFORM_VENDOR>();
		}

		int selectedPlatform=0;
		if(getenv("HPCE_SELECT_PLATFORM")){
			selectedPlatform=atoi(getenv("HPCE_SELECT_PLATFORM"));
		}
		cl::Platform platform=platforms.at(selectedPlatform);  

        std::vector<cl::Device> devices;
		platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);	
		if(devices.size()==0){
			throw std::runtime_error("No opencl devices found.\n");
		}

		int selectedDevice=0;
		if(getenv("HPCE_SELECT_DEVICE")){
			selectedDevice=atoi(getenv("HPCE_SELECT_DEVICE"));
		}
		device=devices.at(selectedDevice);

		context = cl::Context(devices);

        std::string kernelSource=LoadSource("random_projection_kernel.cl");
        
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
    virtual std::string Name() const override
    { return "random_projection_gpu"; }

    cl::Context context;
    cl::Device device;
    cl::Program program;

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

	virtual void Execute(
			   puzzler::ILog *log,
			   const puzzler::RandomProjectionInput *input,
				puzzler::RandomProjectionOutput *output
			   ) const override
	{
        unsigned n=input->n;

		uint32_t p = ceil(std::pow(2.0, 28) / n);
        log->LogVerbose("p: %d, n: %u", p, n);
        
        cl::Kernel kernel(program, "random_projection_kernel");
        
		std::vector<uint64_t> acc(n * n, 0);
		std::mt19937 rnd(input->seed);

        std::vector<uint32_t> v(n, 0);
        size_t v_size = sizeof(uint32_t) * n;
        for(unsigned i = 0; i < n; i++) v[i] = (rnd()%2) * 0xFF;

		// generate random numbers in advance
		// comparison needs exact equivalence between reference and this implementation
		// there is no guarantee of thread-safety when the same mersenne twister is used between all threads.
        std::vector<uint32_t> random_numbers(n, 0);
		for(unsigned i = 0; i < n; i++){
			random_numbers[i] = rnd();
		}

		uint32_t padded_n = n;
		if(n < 1000) padded_n += n%2;
		else padded_n += 32 - n%32;

        cl::NDRange offset(0);
        cl::NDRange globalSize(padded_n * padded_n);
        cl::NDRange localSize(cl::NullRange);

        size_t acc_size = sizeof(uint64_t) * n * n;
        size_t seed_size = sizeof(uint32_t) * n;

        cl::Buffer accbuff(context, CL_MEM_WRITE_ONLY, acc_size);
        cl::Buffer vbuff(context, CL_MEM_READ_ONLY, v_size);
        cl::Buffer seedbuff(context, CL_MEM_READ_ONLY, seed_size);

        kernel.setArg(0, accbuff);
		kernel.setArg(1, vbuff);
		kernel.setArg(2, seedbuff);
        kernel.setArg(3, p);
		kernel.setArg(4, padded_n);
		kernel.setArg(5, n);
        
        cl::CommandQueue queue(context, device);

        cl::Event v_event;
        cl::Event seed_event;
        queue.enqueueWriteBuffer(vbuff, CL_FALSE, 0,  v_size, &v[0], NULL, &v_event);
        queue.enqueueWriteBuffer(seedbuff, CL_FALSE, 0,  seed_size, &random_numbers[0], NULL, &seed_event);
        
        cl::Event kernel_event;
        std::vector<cl::Event> dependencies = {v_event, seed_event};
        
        queue.enqueueNDRangeKernel(kernel, offset, globalSize, localSize, &dependencies, &kernel_event);

        std::vector<cl::Event> read_out_deps = {kernel_event};
        queue.enqueueReadBuffer(accbuff, CL_TRUE, 0, acc_size, &acc[0], &read_out_deps, NULL);

		std::vector<uint64_t> out(n, 0);
		for(unsigned i=0; i<n; i++){
			for(unsigned j=0; j<n; j++){
				out[i] += acc[j*n+i];
			}
		}
		output->acc=out;
		log->LogInfo("Finished");
    }

    virtual bool CompareOutputs(
      puzzler::ILog* /*log*/,
      const puzzler::RandomProjectionInput* /*input*/,
      const puzzler::RandomProjectionOutput* ref,
      const puzzler::RandomProjectionOutput *got) const override
    {
      return ref->acc==got->acc;
    }
};

#endif

// std::cerr<<"Found "<<devices.size()<<" devices\n";
		// for(unsigned i=0;i<devices.size();i++){
			// std::string name=devices[i].getInfo<CL_DEVICE_NAME>();
			// std::cerr
			// 	<< "max work group size " << devices[i].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl
			// 	<< "compute units " << devices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl
			// 	<< "global mem size " << devices[i].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() << std::endl
			// 	<< "local mem size " << devices[i].getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << std::endl
            //  << "extensions " << devices[i].getInfo<CL_DEVICE_EXTENSIONS>() << std::endl
            //   << "FP config " << devices[i].getInfo<CL_DEVICE_SINGLE_FP_CONFIG>() << std::endl
            //     << "max constant buffer size " << devices[i].getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>() << std::endl
            //     << "mem alloc size " << devices[i].getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() << std::endl
            //     << "max param size " << devices[i].getInfo<CL_DEVICE_MAX_PARAMETER_SIZE>() << std::endl
			// ;
			// std::cerr << "max work item sizes ";
			// for(const auto &sz : devices[i].getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>()){
				// std::cerr << sz << " ";
            // ;}
			// ;std::cerr << std::endl;
			// log->LogVerbose("  Device %u: %s", i, name.c_str());
		// }