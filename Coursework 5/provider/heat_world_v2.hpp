#ifndef user_heat_world_hpp
#define user_heat_world_hpp

#include <fstream>
#include <iomanip>
#include <streambuf>
#include <cmath>
#include "tbb/parallel_for.h"
#define __CL_ENABLE_EXCEPTIONS
#include "CL/cl.hpp"

#include "puzzler/puzzles/heat_world.hpp"


class HeatWorldV2Provider
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

  void addDefine(std::string& source, char const* id, uint32_t value)
  {
    char buf[256];
    sprintf(buf, "#define %s %uu\n", id, value);
    source.insert(source.begin(), buf, buf + strlen(buf));
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

    std::string kernelSource = LoadSource("heat_world_kernel_v2.cl");
    addDefine(kernelSource, "PIXEL_SIDE_PER_THREAD", pixel_side_per_thread);

    cl::Program::Sources sources;  // A vector of (data,length) pairs
    sources.push_back(std::make_pair(kernelSource.c_str(), kernelSource.size()+1));  // push on our single string

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

    kn = cl::Kernel(program, "kernel_xy");
  }

  cl::Device device;
  cl::Context context;
  cl::Kernel kn;
  unsigned pixel_side_per_thread;
  unsigned thread_side_per_group;


public:
  HeatWorldV2Provider()
  {
    pixel_side_per_thread = 4;
    thread_side_per_group = 16;
    initOpenCL();
  }

	virtual std::string Name() const override
    { return "heat_world_v2"; }

  virtual void Execute(
    puzzler::ILog *log,
    const puzzler::HeatWorldInput *input,
    puzzler::HeatWorldOutput *output
  ) const override {
    unsigned n = input->n;
    float outer = input->alpha;
    float inner = 1 - outer / 4;
    auto properties = input->properties;

    unsigned size_per_thread = pixel_side_per_thread*pixel_side_per_thread;
    //unsigned work_group_size = thread_side_per_group*thread_side_per_group;
    //unsigned size_per_boundary_thread = n % size_per_thread;
    unsigned threads_per_side = std::ceil(float(n) / pixel_side_per_thread);

    cl::NDRange offset(0, 0);
    cl::NDRange globalSize(threads_per_side, threads_per_side);
    cl::NDRange localSize(thread_side_per_group, thread_side_per_group);
    std::cerr << "globalSize: "  << threads_per_side << std::endl;
    std::cerr << "localSize: "  << thread_side_per_group << std::endl;

    //unsigned patch_size_bits  = 2 * size_per_thread + 4 * pixel_side_per_thread;
    //unsigned patch_size_bytes = std::ceil(float(patch_size_bits) / sizeof(cl_uint));

    // size_t cbBufferProp = patch_size_bytes * threads_per_side*threads_per_side;
    size_t cbBufferProp = sizeof(cl_uchar) * n * n;
    size_t cbBuffer     = sizeof(cl_float) * n * n;

    cl::Buffer buffProperties(context, CL_MEM_READ_ONLY, cbBufferProp);
    cl::Buffer buffState(context, CL_MEM_READ_WRITE, cbBuffer);
    cl::Buffer buffBuffer(context, CL_MEM_READ_WRITE, cbBuffer);

    cl::Kernel kernel = kn;
    kernel.setArg(0, buffState);
    kernel.setArg(1, buffProperties);
    kernel.setArg(2, buffBuffer);
    kernel.setArg(3, inner);
    kernel.setArg(4, outer);
    // Pixels per side
    kernel.setArg(5, n);
    // State copy in local mem
    unsigned localStateSize = std::pow((thread_side_per_group*pixel_side_per_thread + 2), 2);
    std::cerr << "localStateSize: "  << localStateSize << std::endl;
    kernel.setArg(6, cl::__local(localStateSize * sizeof(cl_float)));

    cl::CommandQueue queue(context, device);
    queue.enqueueWriteBuffer(buffState,  CL_FALSE, 0, cbBuffer, &input->state[0]);
    queue.enqueueWriteBuffer(buffBuffer, CL_FALSE, 0, cbBuffer, &input->state[0]);

    // uint8_t packed[cbBufferProp] = {0};
    //
    // auto create_regular_patch = [&](y, x) { // y, x are thread coordinates
    //   for (unsigned j = y*pixel_side_per_thread; j < pixel_side_per_thread; j++) {
    //     for (unsigned i = 0; i < pixel_side_per_thread; i++) {
    //       unsigned index = j * n + i;
    //       if (!(properties[index] & 0x3)) { // 0x3 = Cell_Fixed | Cell_Insulator
    //         packed[index] |= 0x1;
    //         if (!(properties[index-n] & Cell_Insulator)){
    //           packed[index] |= 0x2;
    //         }
    //         if (!(properties[index+n] & Cell_Insulator)){
    //           packed[index] |= 0x4;
    //         }
    //         if (!(properties[index-1] & Cell_Insulator)){
    //           packed[index] |= 0x8;
    //         }
    //         if (!(properties[index+1] & Cell_Insulator)){
    //           packed[index] |= 0x10;
    //         }
    //       }
    //     }
    //   }
    // };
    // auto create_boundary_patch = [&](y, x){
    //
    // };
    //
    // tbb::parallel_for(0u, threads_per_side, [&](unsigned rowThreadIdx){
    //   unsigned columnThreadIdx = 0;
    //   while (columnThreadIdx++ < int(n / size_per_thread)) {
    //     create_regular_patch(rowThreadIdx, columnThreadIdx);
    //   }
    //
    //   if (size_per_boundary_thread > 0) {
    //     create_boundary_patch(rowThreadIdx, columnThreadIdx);
    //   }
    // });

    std::vector<uint8_t> packed(n*n, 0);

    tbb::parallel_for(0u, n*n, [&](unsigned index){
      //if (!(properties[index] & 0x3)) { // 0x3 = Cell_Fixed | Cell_Insulator
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

    for (unsigned t = 0; t < n; t++) {
      log->LogDebug("Time step %d", t);

      kernel.setArg(0, buffState);
      kernel.setArg(2, buffBuffer);

      queue.enqueueNDRangeKernel(kernel, offset, globalSize, localSize);
      queue.enqueueBarrier();
      queue.finish();

      std::swap(buffState, buffBuffer);
    }

    output->state.resize(n*n);
    queue.enqueueReadBuffer(buffState, CL_TRUE, 0, cbBuffer, &output->state[0]);

    log->Log(puzzler::Log_Verbose, [&](std::ostream &dst){
      dst<<std::fixed<<std::setprecision(3)<<"\n";
      for(unsigned y=0; y<n; y++){
        for(unsigned x=0; x<n; x++){
          dst<<" "<<std::setw(6)<<output->state[y*n+x];
        }
        dst<<"\n";
      }
    });

  }
};

#endif
