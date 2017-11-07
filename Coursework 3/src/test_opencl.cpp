#include <stdexcept>
#include <cmath>
#include <cstdint>
#include <memory>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <map>

#include <fstream>
#include <streambuf>

// Update: this doesn't work in windows - if necessary take it out. It is in
// here because some unix platforms complained if it wasn't heere.
#if !defined(WIN32) && !defined(_WIN32)
#include <alloca.h>
#endif

// Update: Work around deprecation warnings
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS 
#define __CL_ENABLE_EXCEPTIONS 
#include "CL/cl.hpp"

int main(int argc, char *argv[])
{
	std::map<cl_int, std::string> errmap;

#define ERRMAP(e)	errmap[e] = # e
	ERRMAP(CL_SUCCESS);
	ERRMAP(CL_INVALID_PROPERTY);
	ERRMAP(CL_INVALID_VALUE);
	ERRMAP(CL_INVALID_DEVICE);
	ERRMAP(CL_DEVICE_NOT_AVAILABLE);
	ERRMAP(CL_OUT_OF_HOST_MEMORY);
	ERRMAP(CL_INVALID_BINARY);
	ERRMAP(CL_INVALID_BUILD_OPTIONS);
	ERRMAP(CL_INVALID_OPERATION);
	ERRMAP(CL_COMPILER_NOT_AVAILABLE);
	ERRMAP(CL_BUILD_PROGRAM_FAILURE);
	ERRMAP(CL_OUT_OF_RESOURCES);
	ERRMAP(CL_OUT_OF_HOST_MEMORY);

	try{
	
		std::vector<cl::Platform> platforms;
			
		cl::Platform::get(&platforms);
		if(platforms.size()==0)
			throw std::runtime_error("No OpenCL platforms found.");
		
		std::cerr<<"Found "<<platforms.size()<<" platforms\n";
		for(unsigned i=0;i<platforms.size();i++){
			std::string vendor=platforms[i].getInfo<CL_PLATFORM_NAME>();
			std::cerr<<"  Platform "<<i<<" : "<<vendor<<"\n";
		}
		
		int selectedPlatform=0;
		if(getenv("HPCE_SELECT_PLATFORM")){
			selectedPlatform=atoi(getenv("HPCE_SELECT_PLATFORM"));
		}
		std::cerr<<"Choosing platform "<<selectedPlatform<<"\n";
		cl::Platform platform=platforms.at(selectedPlatform);
		
		std::vector<cl::Device> devices;
		platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);	
		if(devices.size()==0){
			throw std::runtime_error("No opencl devices found.\n");
		}
			
		std::cerr<<"Found "<<devices.size()<<" devices\n";
		for(unsigned i=0;i<devices.size();i++){
			std::string name=devices[i].getInfo<CL_DEVICE_NAME>();
			std::cerr<<"  Device "<<i<<" : "<<name<<"\n";
		}
		
		int selectedDevice=0;
		if(getenv("HPCE_SELECT_DEVICE")){
			selectedDevice=atoi(getenv("HPCE_SELECT_DEVICE"));
		}
		std::cerr<<"Choosing device "<<selectedDevice<<"\n";
		cl::Device device=devices.at(selectedDevice);
		
		devices.clear();
		devices.push_back(device);
		cl::Context context(devices);

		std::string kernelSource="__kernel void Add(__global float *x){ x[get_global_id(0)] += 0.125f; }\n";
		
		cl::Program::Sources sources(1, std::make_pair(kernelSource.c_str(), kernelSource.size()+1));

		cl::Program program(context, sources);
		program.build(devices);
		
		unsigned nThreads=1;
		
		size_t cbBuffer=4;
		cl::Buffer buffer(context, CL_MEM_READ_WRITE, cbBuffer);
		
		cl::Kernel kernel(program, "Add");
		
		kernel.setArg(0, buffer);
		
		cl::CommandQueue queue(context, device);
		
		float input=0.5;
		queue.enqueueWriteBuffer(buffer, CL_TRUE, 0, cbBuffer, &input);
		queue.enqueueBarrier();
			
		queue.enqueueNDRangeKernel(kernel, cl::NDRange(0), cl::NDRange(nThreads), cl::NullRange);
		queue.enqueueBarrier();
		
		float output;
		queue.enqueueReadBuffer(buffer, CL_TRUE, 0, cbBuffer, &output);
		
		if(output!=0.625){
			std::cerr<<"Program executed, but got the wrong output.\n";
			return 1;
		}else{
			std::cerr<<"Success, program executed kernel successfully.\n";
		}
	}catch(cl::Error &e){
		std::cerr<<"Exception from "<<e.what()<<" : ";

		std::map<cl_int, std::string>::iterator it;
		if ((it = errmap.find(e.err())) != errmap.end())
			std::cerr << it->second << std::endl;
		else
			std::cerr << "Unknown " << e.err() << std::endl;

		return 1;
	}
		
	return 0;
}
