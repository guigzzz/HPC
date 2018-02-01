#ifndef user_heat_world_hpp
#define user_heat_world_hpp

#include <fstream>
#include <iomanip>
#include <streambuf>
#include "tbb/parallel_for.h"
#define __CL_ENABLE_EXCEPTIONS
#include "CL/cl.hpp"

#include "puzzler/puzzles/heat_world.hpp"


class HeatWorldProvider
  : public puzzler::HeatWorldPuzzle
{
private:
  std::string LoadSource(const char *fileName) const
  {
    std::string baseDir = "provider";
    if (getenv("HPCE_CL_SRC_DIR")) {
      baseDir = getenv("HPCE_CL_SRC_DIR");
    }

    std::string fullName = baseDir + "/" + fileName;

    // Open a read-only binary stream over the file
    std::ifstream src(fullName, std::ios::in | std::ios::binary);
    if(!src.is_open()) {
      throw std::runtime_error("LoadSource : Couldn't load cl file from '"+fullName+"'.");
    }

    // Read all characters of the file into a string
    return std::string(
      (std::istreambuf_iterator<char>(src)), // Node the extra brackets.
          std::istreambuf_iterator<char>()
    );
  }


  void initOpenCL() {
    std::vector<cl::Platform> platforms;

    cl::Platform::get(&platforms);
    if (platforms.size() == 0) {
      throw std::runtime_error("No OpenCL platforms found.");
    }
    // std::cerr<<"Found "<<platforms.size()<<" platforms\n";
    for (unsigned i = 0; i < platforms.size(); i++) {
      std::string vendor=platforms[i].getInfo<CL_PLATFORM_VENDOR>();
      // std::cerr << "  Platform " << i << " : " << vendor << "\n";
    }

    int selectedPlatform = 0;
    if (getenv("HPCE_SELECT_PLATFORM")) {
      selectedPlatform=atoi(getenv("HPCE_SELECT_PLATFORM"));
    }
    // std::cerr << "Choosing platform " << selectedPlatform << "\n";
    cl::Platform platform=platforms.at(selectedPlatform);

    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    if (devices.size() == 0) {
      throw std::runtime_error("No opencl devices found.\n");
    }

    // std::cerr << "Found " << devices.size() << " devices\n";
    for (unsigned i = 0; i < devices.size(); i++) {
      std::string name=devices[i].getInfo<CL_DEVICE_NAME>();
      // std::cerr << "  Device " << i << " : " << name << "\n";
    }

    int selectedDevice = 0;
    if (getenv("HPCE_SELECT_DEVICE")) {
      selectedDevice = atoi(getenv("HPCE_SELECT_DEVICE"));
    }
    // std::cerr << "Choosing device " << selectedDevice << "\n";
    device = devices.at(selectedDevice);

    context = cl::Context(devices);

    std::string kernelSource = LoadSource("heat_world_kernel.cl");

    cl::Program::Sources sources;  // A vector of (data,length) pairs
    sources.push_back(std::make_pair(kernelSource.c_str(), kernelSource.size()+1));  // push on our single string

    cl::Program program(context, sources);

    char build_options[50];
    sprintf(build_options, " ");
    try {
      program.build(devices, &build_options[0]);
    } catch(...) {
      for(unsigned i = 0; i < devices.size(); i++) {
        std::cerr << "Log for device " << devices[i].getInfo<CL_DEVICE_NAME>() << ":\n\n";
        std::cerr << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[i]) << "\n\n";
      }
      throw;
    }

    kn = cl::Kernel(program, "kernel_xy");
  }

  cl::Device device;
  cl::Context context;
  cl::Kernel kn;


public:
  HeatWorldProvider()
  {
    initOpenCL();
  }

  virtual void Execute(
    puzzler::ILog *log,
    const puzzler::HeatWorldInput *input,
    puzzler::HeatWorldOutput *output
  ) const override {
    unsigned n = input->n;
    float outer = input->alpha;
    float inner = 1 - outer / 4;
    auto properties = input->properties;

    size_t cbBufferProp  = sizeof(uint8_t)*n*n;
    size_t cbBuffer      = sizeof(float)*n*n;

    cl::Buffer buffProperties(context, CL_MEM_READ_ONLY, cbBufferProp);
    cl::Buffer buffState(context, CL_MEM_READ_WRITE, cbBuffer);
    cl::Buffer buffBuffer(context, CL_MEM_READ_WRITE, cbBuffer);

    cl::Kernel kernel = kn;
    kernel.setArg(0, buffState);
    kernel.setArg(1, buffProperties);
    kernel.setArg(2, buffBuffer);
    kernel.setArg(3, inner);
    kernel.setArg(4, outer);

    cl::CommandQueue queue(context, device);
    queue.enqueueWriteBuffer(buffState,  CL_FALSE, 0, cbBuffer, &input->state[0]);
    queue.enqueueWriteBuffer(buffBuffer, CL_FALSE, 0, cbBuffer, &input->state[0]);

    std::vector<uint8_t> packed(n*n, 0);

    tbb::parallel_for(0u, n*n, [&](unsigned index){
      if (!((properties[index] & Cell_Fixed) || (properties[index] & Cell_Insulator))) {
        packed[index] |= 0x1;
        if (!(properties[index-n] & Cell_Insulator)){
          packed[index] |= 0x2;
        }
        if (!(properties[index+n] & Cell_Insulator)){
          packed[index] |= 0x4;
        }
        if (!(properties[index-1] & Cell_Insulator)){
          packed[index] |= 0x8;
        }
        if (!(properties[index+1] & Cell_Insulator)){
          packed[index] |= 0x10;
        }
      }
    });

    queue.enqueueWriteBuffer(buffProperties, CL_FALSE, 0, cbBufferProp, &packed[0]);
    queue.enqueueBarrier();

    cl::NDRange offset(0, 0);             // Always start iterations at x=0, y=0
    cl::NDRange globalSize(n, n);         // Global size must match the original loops
    cl::NDRange localSize = cl::NullRange;  // We don't care about local size

    for (unsigned t = 0; t < n; t++) {
      log->LogDebug("Time step %d", t);

      kernel.setArg(0, buffState);
      kernel.setArg(2, buffBuffer);
      kernel.setArg(5, t);

      queue.enqueueNDRangeKernel(kernel, offset, globalSize, localSize);
      queue.enqueueBarrier();

      std::swap(buffState, buffBuffer);
    }

    output->state.resize(n*n);
    queue.enqueueReadBuffer(buffState, CL_TRUE, 0, cbBuffer, &output->state[0]);
  }
};

#endif
