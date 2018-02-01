#ifndef user_gpu_mining_hpp
#define user_gpu_mining_hpp

#include "puzzler/puzzles/mining.hpp"
#define __CL_ENABLE_EXCEPTIONS 
#include "CL/cl.hpp"
#include <chrono>

#include <stdexcept>
#include <cstdint>
#include <cstdio>
#include <string>
#include <fstream>
#include <streambuf>

std::string LoadSource(const char *fileName)
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

class MiningGPUProvider
  : public puzzler::MiningPuzzle
{
public:
  MiningGPUProvider()
	{
		std::vector<cl::Platform> platforms;
	
		cl::Platform::get(&platforms);
		if(platforms.size()==0)
			throw std::runtime_error("No OpenCL platforms found.");

		// std::cerr<<"Found "<<platforms.size()<<" platforms\n";
		for(unsigned i=0;i<platforms.size();i++){
			std::string vendor=platforms[i].getInfo<CL_PLATFORM_VENDOR>();
			// log->LogVerbose("  Platform %u: %s", i, vendor.c_str());
		}

		int selectedPlatform=0;
		if(getenv("HPCE_SELECT_PLATFORM")){
			selectedPlatform=atoi(getenv("HPCE_SELECT_PLATFORM"));
		}
		// log->LogVerbose("Choosing platform: %u", selectedPlatform);
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

		kernelSource=LoadSource("mining_kernel.cl");

		cl::Program::Sources sources;	// A vector of (data,length) pairs
		sources.push_back(std::make_pair(kernelSource.c_str(), kernelSource.size()+1));	// push on our single string
		
		program = cl::Program(context, sources);
		// char build_options[128];
    	// sprintf(build_options, 
		// 	"" 
		// 	);

		try{
			program.build(devices);//, &build_options[0]);
			
		}catch(...){
			for(unsigned i=0;i<devices.size();i++){
				std::cerr<<"Log for device "<<devices[i].getInfo<CL_DEVICE_NAME>()<<":\n\n";
				std::cerr<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[i])<<"\n\n";
			}
			throw;
		}
	}	

	std::string kernelSource;
	cl::Context context;
	cl::Program program;
	cl::Device device;
	
	std::string Name() const override
    { return "gpu_mining"; } 

	virtual void Execute(
			   puzzler::ILog *log,
			   const puzzler::MiningInput *input,
				puzzler::MiningOutput *output
	) const override
	{
		cl::Kernel kernel(program, "mining_kernel");

		uint64_t batch_size = std::pow(2.0, 22);
		uint64_t work_group_size = 256;
		uint64_t size_per_thread = 32;

		cl::NDRange offset(0);
		cl::NDRange globalSize(batch_size / size_per_thread);
		cl::NDRange localSize(work_group_size); 

		size_t number_output_items = batch_size / (size_per_thread * work_group_size);

		size_t buffer_size = sizeof(cl_ulong) * number_output_items;
		cl::Buffer buffhashes(context, CL_MEM_WRITE_ONLY, buffer_size);
		cl::Buffer buffinputs(context, CL_MEM_WRITE_ONLY, buffer_size);
		cl::Buffer keybuff(context, CL_MEM_READ_ONLY, sizeof(uint32_t) * input->key.size());

		std::vector<uint64_t> hashes(number_output_items, 0);
		std::vector<uint64_t> inputs(number_output_items, 0);

		kernel.setArg(0, buffhashes);
		kernel.setArg(1, buffinputs);
		kernel.setArg(3, batch_size);
		kernel.setArg(4, cl::__local(work_group_size * sizeof(cl_ulong)));
		kernel.setArg(5, input->rounds);
		kernel.setArg(6, keybuff);

		cl::CommandQueue queue(context, device);
		queue.enqueueWriteBuffer(keybuff, CL_TRUE, 0, sizeof(uint32_t) * input->key.size(), &input->key[0]);

		auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		std::mt19937 mt_rand(seed);

		uint64_t global_offset = mt_rand();
		log->LogVerbose("Starting offset: %u", global_offset);

		while(1){
			kernel.setArg(2, global_offset);
			global_offset += batch_size;

			cl::Event evExecutedKernel;
			queue.enqueueNDRangeKernel(kernel, offset, globalSize, localSize, NULL, &evExecutedKernel);	

			std::vector<cl::Event> copydeps = {evExecutedKernel};
			queue.enqueueReadBuffer(buffhashes, CL_FALSE, 0, buffer_size, &hashes[0], &copydeps);	
			queue.enqueueReadBuffer(buffinputs, CL_TRUE, 0, buffer_size, &inputs[0], &copydeps);
			
			auto min_hash_index = std::min_element(hashes.begin(), hashes.end()) - hashes.begin();
			if(hashes[min_hash_index] < input->threshold){
				output->input = inputs[min_hash_index];
				break;
			}
		}
	}
};

#endif