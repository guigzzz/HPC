#ifndef user_hold_time_gpu_hpp
#define user_hold_time_gpu_hpp

#define __CL_ENABLE_EXCEPTIONS
#include <algorithm>
#include <string>
#include <fstream>
#include "CL/cl.hpp"
#include "tbb/parallel_for.h"

#include "puzzler/puzzles/hold_time.hpp"

#define MAX_NODES_ROW_LENGTH 4  /* delay + 1..3 degrees */

namespace {
  std::string LoadKernelSource(const char *fileName)
  {
    std::string fullName = std::string("provider/") + fileName;

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

  void addDefine(std::string& source, char const* id, uint32_t value)
  {
    char buf[256];
    sprintf(buf, "#define %s %uu\n", id, value);
    source.insert(source.begin(), buf, buf + strlen(buf));
  }
}

class HoldTimeGPUProvider : public puzzler::HoldTimePuzzle
{
public:
  HoldTimeGPUProvider(){}

  public:
    virtual std::string Name() const override
    { return "hold_time_gpu"; }

protected:

  virtual void Execute(
    puzzler::ILog *log,
    const puzzler::HoldTimeInput *input,
    puzzler::HoldTimeOutput *output
  ) const override {
    // Selecting OpenCL platform
    std::vector<cl::Platform> platforms;

    cl::Platform::get(&platforms);
    if(platforms.size() == 0) {
      throw std::runtime_error("No OpenCL platforms found.");
    }

    log->LogVerbose("Found %u platforms", platforms.size());
    for(unsigned i = 0; i < platforms.size(); i++) {
      std::string vendor=platforms[i].getInfo<CL_PLATFORM_VENDOR>();
      log->LogVerbose("  Platform %u : %s", i, vendor.c_str());
    }

    int selectedPlatform = 0;
    if(getenv("HPCE_SELECT_PLATFORM")){
      selectedPlatform = atoi(getenv("HPCE_SELECT_PLATFORM"));
    }
    log->LogVerbose("Choosing platform #%u", selectedPlatform);
    cl::Platform platform=platforms.at(selectedPlatform);

    // Selecting a device
    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    if(devices.size() == 0) {
        throw std::runtime_error("No opencl devices found.\n");
    }

    log->LogVerbose("Found %u devices", devices.size());
    for(unsigned i = 0; i<devices.size(); i++) {
        std::string name=devices[i].getInfo<CL_DEVICE_NAME>();
        log->LogVerbose("  Device %u : %s", i, name.c_str());
        log->LogVerbose("    max work group size      : %u", devices[i].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>());
        log->LogVerbose("    compute units            : %u", devices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>());
        log->LogVerbose("    global mem size          : %u", devices[i].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>());
        log->LogVerbose("    local mem size           : %u", devices[i].getInfo<CL_DEVICE_LOCAL_MEM_SIZE>());
        log->LogVerbose("    max constant buffer size : %u", devices[i].getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>());
        log->LogVerbose("    mem alloc size           : %u", devices[i].getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>());
        log->LogVerbose("    max param size           : %u", devices[i].getInfo<CL_DEVICE_MAX_PARAMETER_SIZE>());

        log->LogVerbose("    max work item sizes: ");
        for(const auto &sz : devices[i].getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>()){
          log->LogVerbose("      %u", sz);
        }
    }

    int selectedDevice = 0;
    if(getenv("HPCE_SELECT_DEVICE")) {
        selectedDevice=atoi(getenv("HPCE_SELECT_DEVICE"));
    }
    log->LogVerbose("Choosing device #%u", selectedDevice);
    cl::Device device=devices.at(selectedDevice);

    // Creating an OpenCL context
    cl::Context context(devices);

    // Get and compile kernel
    std::string kernelSource = LoadKernelSource("hold_time_kernel.cl");
    addDefine(kernelSource, "NODES_COUNT", input->nodes.size());

    cl::Program::Sources sources;    // A vector of (data,length) pairs
    sources.push_back(std::make_pair(kernelSource.c_str(), kernelSource.size()+1));    // push on our single string

    cl::Program program(context, sources);
    try {
      program.build(devices);
    } catch(...) {
      for(unsigned i = 0; i < devices.size(); i++) {
        std::cerr << "Log for device " << devices[i].getInfo<CL_DEVICE_NAME>() << ":\n\n";
        std::cerr << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[i]) << "\n\n";
      }
      throw;
    }

    // Allocating buffers
    size_t n = input->nodes.size();
    unsigned ffCount = input->flipFlopCount;
    cl::Buffer buffNodes(context, CL_MEM_READ_ONLY, sizeof(cl_uint) * n * MAX_NODES_ROW_LENGTH);
    cl::Buffer buffLocalDistance(context, CL_MEM_WRITE_ONLY, sizeof(cl_uint));

    // Allocating buffers
    cl::Kernel kernel(program, "get_dijkstra_shortest_path");
    kernel.setArg(0, buffNodes);
    kernel.setArg(1, buffLocalDistance);

    // Flatten input->nodes into a 1D array
    std::vector<unsigned> nodes(n * MAX_NODES_ROW_LENGTH);
    tbb::parallel_for((size_t) 0, n, [&](unsigned row){
      unsigned j = 0;
      while (j < input->nodes[row].size()) {
        nodes[row*MAX_NODES_ROW_LENGTH+j] = input->nodes[row][j];
        j++;
      }
      while (j < MAX_NODES_ROW_LENGTH) {
        nodes[row*MAX_NODES_ROW_LENGTH+j] = UINT_MAX; // Sentinel for undefined inputs
        j++;
      }
    });

    // Command queue
    cl::CommandQueue queue(context, device);

    // Sync copy fixed nodes to kernel buffer
    queue.enqueueWriteBuffer(buffNodes, CL_TRUE, 0, sizeof(cl_uint)*nodes.size(), &nodes[0]);

    // Accumulate results
    std::vector<unsigned> localDistances(ffCount, UINT_MAX);

    for (unsigned srcFF = 0; srcFF < ffCount; srcFF++) {
      // Execute the kernel
      cl::NDRange offset(0);                  // Always start iterations at x=0, y=0
      cl::NDRange globalSize(ffCount);        // Global size must match the original loops
      cl::NDRange localSize = cl::NullRange;  // We don't care about local size

      cl::Event evExecutedKernel;
      queue.enqueueNDRangeKernel(kernel, offset, globalSize, localSize, NULL, &evExecutedKernel);

      // Sync copy back results
      std::vector<cl::Event> copyBackDependencies(1, evExecutedKernel);
      queue.enqueueReadBuffer(buffLocalDistance, CL_FALSE, 0, sizeof(cl_uint), &localDistances[srcFF], &copyBackDependencies);
    }
    // Wait for queue completion
    queue.enqueueBarrier();
    queue.finish();

    unsigned minDelay = *std::min_element(localDistances.begin(), localDistances.end());
    log->LogInfo("Min delay = %u", minDelay);
    output->minDelay = minDelay;
  }
};

#endif
