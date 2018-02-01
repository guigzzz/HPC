#ifndef local_puzzles_hpp
#define local_puzzles_hpp

#include "hold_time.hpp"
#include "hold_time_dijkstra.hpp"
#include "hold_time_gpu.hpp"
#include "gaussian_blur.hpp"
#include "edit_distance.hpp"
#include "random_projection.hpp"
#include "mining.hpp"
#include "heat_world.hpp"
#include "heat_world_v2.hpp"
#include "mining_GPU.hpp"
#include "random_projection_gpu.hpp"
#include "gaussian_blur_GPU.hpp"

void puzzler::PuzzleRegistrar::UserRegisterPuzzles()
{
  Register(std::make_shared<HoldTimeProvider>());
  Register(std::make_shared<HoldTimeDijkstraProvider>());
  Register(std::make_shared<HoldTimeGPUProvider>());
  Register(std::make_shared<GaussianBlurProvider>());
  Register(std::make_shared<GaussianBlurProvider_GPU>());
  Register(std::make_shared<EditDistanceProvider>());
  Register(std::make_shared<RandomProjectionProvider>());
  Register(std::make_shared<RandomProjectionGPUProvider>());
  Register(std::make_shared<MiningProvider>());
  Register(std::make_shared<MiningGPUProvider>());
  Register(std::make_shared<HeatWorldProvider>());
  //Register(std::make_shared<HeatWorldV2Provider>());
}


#endif
