// Copyright (c) OpenMMLab. All rights reserved.

#include "plugins/gather_topk.hpp"

#include "plugins/gather_topk_kernel.hpp"
#include "plugins/trt_serialize.hpp"

#include <assert.h>
#include <stdio.h>

#include <chrono>

namespace trt_plugin
{
namespace
{
static const char * PLUGIN_VERSION{"1"};
static const char * PLUGIN_NAME{"GatherTopk"};
}  // namespace

GatherTopk::GatherTopk(const std::string & name) : TRTPluginBase(name)
{
}
GatherTopk::GatherTopk(const std::string & name, const void *, size_t) : TRTPluginBase(name)
{
}

// IPluginV2 methods
const char * GatherTopk::getPluginVersion() const noexcept
{
  return PLUGIN_VERSION;
}

const char * GatherTopk::getPluginType() const noexcept
{
  return PLUGIN_NAME;
}

int GatherTopk::getNbOutputs() const noexcept
{
  return 1;
}

size_t GatherTopk::getSerializationSize() const noexcept
{
  return 0;
}

void GatherTopk::serialize(void *) const noexcept
{
}

// IPluginV2Ext methods
nvinfer1::DataType GatherTopk::getOutputDataType(
  int, const nvinfer1::DataType * inputTypes, int) const noexcept
{
  return inputTypes[0];
}

// IPluginV2DynamicExt methods
nvinfer1::IPluginV2DynamicExt * GatherTopk::clone() const noexcept
{
  GatherTopk * plugin = new GatherTopk(mLayerName);
  plugin->setPluginNamespace(getPluginNamespace());
  return plugin;
}

nvinfer1::DimsExprs GatherTopk::getOutputDimensions(
  int, const nvinfer1::DimsExprs * inputs, int, nvinfer1::IExprBuilder &) noexcept
{
  assert(inputs[1].nbDims <= inputs[0].nbDims);
  nvinfer1::DimsExprs ret;
  ret.nbDims = inputs[0].nbDims;
  for (int i = 0; i < inputs[1].nbDims; ++i) {
    ret.d[i] = inputs[1].d[i];
  }
  for (int i = inputs[1].nbDims; i < inputs[0].nbDims; ++i) {
    ret.d[i] = inputs[0].d[i];
  }
  return ret;
}

bool GatherTopk::supportsFormatCombination(
  int pos, const nvinfer1::PluginTensorDesc * ioDesc, int, int) noexcept
{
  switch (pos) {
    case 0:
      // data
      return (ioDesc[pos].type == nvinfer1::DataType::kFLOAT &&
              ioDesc[pos].format == nvinfer1::TensorFormat::kLINEAR) ||
             (ioDesc[pos].type == nvinfer1::DataType::kINT32 &&
              ioDesc[pos].format == nvinfer1::TensorFormat::kLINEAR);
    case 1:
      // indices
      return ioDesc[pos].type == nvinfer1::DataType::kINT32 &&
             ioDesc[pos].format == nvinfer1::TensorFormat::kLINEAR;
    case 2:
      // output
      return ioDesc[pos].type == ioDesc[0].type && ioDesc[pos].format == ioDesc[0].format;
    default:
      return true;
  }
}

void GatherTopk::configurePlugin(
  const nvinfer1::DynamicPluginTensorDesc *, int, const nvinfer1::DynamicPluginTensorDesc *,
  int) noexcept
{
}

size_t GatherTopk::getWorkspaceSize(
  const nvinfer1::PluginTensorDesc *, int, const nvinfer1::PluginTensorDesc *, int) const noexcept
{
  return 0;
}

int GatherTopk::enqueue(
  const nvinfer1::PluginTensorDesc * inputDesc, const nvinfer1::PluginTensorDesc *,
  const void * const * inputs, void * const * outputs, void *, cudaStream_t stream) noexcept
{
  const int * dims = &(inputDesc[0].dims.d[0]);
  const int * dims_indices = &(inputDesc[1].dims.d[0]);
  int nbDims = inputDesc[0].dims.nbDims;
  int nbDims_index = inputDesc[1].dims.nbDims;

  const void * data = inputs[0];
  const void * indices = inputs[1];
  void * output = outputs[0];

  auto data_type = inputDesc[0].type;

  switch (data_type) {
    case nvinfer1::DataType::kFLOAT:
      gather_topk_impl<float>(
        reinterpret_cast<const float *>(data), reinterpret_cast<const int *>(indices), dims, nbDims,
        dims_indices, nbDims_index, reinterpret_cast<float *>(output), stream);
      break;
    case nvinfer1::DataType::kINT32:
      gather_topk_impl<int>(
        reinterpret_cast<const int *>(data), reinterpret_cast<const int *>(indices), dims, nbDims,
        dims_indices, nbDims_index, reinterpret_cast<int *>(output), stream);
      break;
    default:
      break;
  }
  return 0;
}

// PluginCreator
GatherTopkCreator::GatherTopkCreator()
{
  mPluginAttributes.clear();
  mFC.nbFields = mPluginAttributes.size();
  mFC.fields = mPluginAttributes.data();
}

const char * GatherTopkCreator::getPluginName() const noexcept
{
  return PLUGIN_NAME;
}

const char * GatherTopkCreator::getPluginVersion() const noexcept
{
  return PLUGIN_VERSION;
}

nvinfer1::IPluginV2 * GatherTopkCreator::createPlugin(
  const char * name, const nvinfer1::PluginFieldCollection *) noexcept
{
  auto * plugin = new GatherTopk(name);
  plugin->setPluginNamespace(getPluginNamespace());
  return plugin;
}

nvinfer1::IPluginV2 * GatherTopkCreator::deserializePlugin(
  const char * name, const void * serialData, size_t serialLength) noexcept
{
  auto plugin = new GatherTopk(name, serialData, serialLength);
  plugin->setPluginNamespace(getPluginNamespace());
  return plugin;
}

}  // namespace trt_plugin
