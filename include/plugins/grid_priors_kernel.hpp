// Copyright (c) OpenMMLab. All rights reserved.

#ifndef GRID_PRIORS_KERNEL_HPP_
#define GRID_PRIORS_KERNEL_HPP_

#include <cuda_runtime.h>

namespace trt_plugin
{
template <typename scalar_t>
void grid_priors_impl(
  const scalar_t * base_anchor, scalar_t * output, int num_base_anchors, int feat_w, int feat_h,
  int stride_w, int stride_h, cudaStream_t stream);
}  // namespace trt_plugin

#endif  // GRID_PRIORS_KERNEL_HPP_
