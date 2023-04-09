#ifndef TRT_UTILS_HPP_
#define TRT_UTILS_HPP_

#include <NvInfer.h>

#include <iostream>


namespace trt
{
  struct Deleter
  {
    template<typename T>
    void operator()(T *obj) const
    {
      if (obj) {
        delete obj;
      }
    }
  }; // struct Deleter

  class Logger : public nvinfer1::ILogger
  {
  private:
    bool verbose_{true};

  public:
    explicit Logger(bool verbose) : verbose_(verbose) {}

    void log(Severity severity, const char *msg) noexcept override
    {
      if (verbose_ || ((severity != Severity::kINFO) && (severity != Severity::kVERBOSE))) {
        std::cout << msg << std::endl;
      }
    }
  }; // class Logger

} // namespace trt_utils

#endif // TRT_UTILS_HPP_