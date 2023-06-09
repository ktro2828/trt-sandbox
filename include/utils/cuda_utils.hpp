#ifndef CUDA_UTILS_HPP_
#define CUDA_UTILS_HPP_

#include <cublas_v2.h>
#include <cuda.h>
#include <cuda_runtime_api.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <type_traits>

#define CHECK_CUDA_ERROR(e) (cuda::check_error(e, __FILE__, __LINE__));

#define CUDA_1D_KERNEL_LOOP(i, n) \
  for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < (n); i += blockDim.x * gridDim.x)

#define THREADS_PER_BLOCK 512

#define DIVUP(m, n) ((m) / (n) + ((m) % (n) > 0))

inline int GET_BLOCKS(const int N)
{
  int optimal_block_num = DIVUP(N, THREADS_PER_BLOCK);
  constexpr int max_block_num = 4096;
  return std::min(optimal_block_num, max_block_num);
}

namespace cuda
{
void check_error(const ::cudaError_t e, const char * f, int n)
{
  if (e != ::cudaSuccess) {
    ::std::stringstream s;
    s << ::cudaGetErrorName(e) << " (" << e << ")@" << f << "#L" << n << ": "
      << ::cudaGetErrorString(e);
    throw ::std::runtime_error{s.str()};
  }
}

struct Deleter
{
  void operator()(void * p) const { CHECK_CUDA_ERROR(::cudaFree(p)); }
};  // struct Deleter

template <typename T>
using unique_ptr = ::std::unique_ptr<T, Deleter>;

template <typename T>
typename ::std::enable_if<::std::is_array<T>::value, cuda::unique_ptr<T>>::type make_unique(
  const ::std::size_t n)
{
  using U = typename ::std::remove_extent<T>::type;
  U * p;
  CHECK_CUDA_ERROR(::cudaMalloc(reinterpret_cast<void **>(&p), sizeof(U) * n));
  return cuda::unique_ptr<T>{p};
}

template <typename T>
cuda::unique_ptr<T> make_unique()
{
  T * p;
  CHECK_CUDA_ERROR(::cudaMalloc(reinterpret_cast<void **>(&p), sizeof(T)));
  return cuda::unique_ptr<T>{p};
}
}  // namespace cuda

#endif  // CUDA_UTILS_HPP_
