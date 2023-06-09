#ifndef NPP_UTILS_HPP_
#define NPP_UTILS_HPP_

#include "utils/cuda_utils.hpp"

#include <opencv2/opencv.hpp>

#include <npp.h>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>

#define CHECK_NPP_ERROR(s) (npp::check_error(s, __FILE__, __LINE__));

namespace npp
{
void check_error(const NppStatus status, const char * f, int n)
{
  if (status != NPP_SUCCESS) {
    std::stringstream ss;
    ss << "(NPP_STATUS_ERROR)@" << f << "#L" << n << ": status=" << status;
    throw std::runtime_error{ss.str()};
  }
}

void resize(
  const std::uint8_t * src, std::uint8_t * dst, const int in_w, const int in_h, const int out_w,
  const int out_h, cudaStream_t stream)
{
  NppiSize in_size{in_w, in_h}, out_size{out_w, out_h};
  uint8_t *cuda_src, *cuda_dst;
  size_t in_step = in_size.width * 3 * sizeof(uint8_t),
         out_step = out_size.width * 3 * sizeof(uint8_t);
  NppiRect in_roi{0, 0, in_size.width, in_size.height},
    out_roi{0, 0, out_size.width, out_size.height};

  CHECK_CUDA_ERROR(cudaMallocAsync(reinterpret_cast<void **>(&cuda_src), in_step, stream));
  CHECK_CUDA_ERROR(cudaMallocAsync(reinterpret_cast<void **>(&cuda_dst), out_step, stream));
  CHECK_CUDA_ERROR(cudaMemcpyAsync(
    cuda_src, src, in_size.width * 3 * sizeof(Npp8u), cudaMemcpyHostToDevice, stream));

  CHECK_NPP_ERROR(nppiResize_8u_C3R(
    cuda_src, in_step, in_size, in_roi, cuda_dst, out_step, out_size, out_roi, NPPI_INTER_LINEAR));

  //   CHECK_CUDA_ERROR(cudaMemcpyAsync(dst, cuda_dst, out_step, cudaMemcpyDeviceToHost, stream));
}

}  // namespace npp

#endif  // NPP_UTILS_HPP_