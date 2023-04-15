#include <opencv2/opencv.hpp>

// #if (defined(_MSC_VER) or (defined(__GNUC__) and (7 <= __GNUC_MAJOR__)))
#include <filesystem>
namespace fs = ::std::filesystem;
// #else
// #include <experimental/filesystem>
// namespace fs = ::std::experimental::filesystem;
// #endif

#include "trt_ssd.hpp"
#include "utils/common_utils.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

struct Shape
{
  int width;
  int height;
  int channel;
};  // struct Shape

struct Config
{
  int num_max_detections{100};
  float threshold{0.5};
  Shape shape{300, 300, 3};
};  // struct config

std::vector<float> calculateSoftmax(const float * scores)
{
  // TODO
}

cv::Mat drawOutput(
  const cv::Mat & img, const float * scores, const float * boxes, const Config & config)
{
  const auto img_w = img.cols;
  const auto img_h = img.rows;
  cv::Mat viz = img.clone();
  for (int i = 0; i < config.num_max_detections; ++i) {
    // TODO: Add NMS?
    if (scores[i] < config.threshold) {
      std::cerr << "[WARN] No boxes are detected!! Score: " << scores[i] << " < "
                << config.threshold << std::endl;
      continue;
    }
    float x_offset = boxes[4 * i] * img_w / config.shape.width;
    float y_offset = boxes[4 * i + 1] * img_h / config.shape.height;
    float width = boxes[4 * i + 2] * img_w / config.shape.width;
    float height = boxes[4 * i + 3] * img_h / config.shape.height;
    const int x1 = std::max(0, static_cast<int>(x_offset));
    const int y1 = std::max(0, static_cast<int>(y_offset));
    const int x2 = std::min(img_w, static_cast<int>(x_offset + width));
    const int y2 = std::min(img_h, static_cast<int>(y_offset + height));
    std::cout << "(" << x1 << ", " << y1 << "), (" << x2 << ", " << y2 << ")" << std::endl;
    cv::rectangle(viz, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 0, 255), 3, 8, 0);
  }

  return viz;
}

int main(int argc, char * argv[])
{
  if (argc != 3) {
    std::cerr << "[ERROR] You must specify input path of image and engine!!" << std::endl;
    std::exit(1);
  }

  std::string img_path(argv[1]);
  cv::Mat img = cv::imread(img_path);

  const Config config = {100, 0.5};
  const int max_batch_size{1};  // TODO: support 8
  const std::string precision{"FP32"};
  const bool box_first{false};
  const bool verbose{false};

  std::string model_path(argv[2]);
  std::string engine_path, onnx_path;
  if (model_path.substr(model_path.find_last_of(".") + 1) == "engine") {
    engine_path = model_path;
    onnx_path = fs::path{model_path}.replace_extension("onnx").string();
  } else if (model_path.substr(model_path.find_last_of(".") + 1) == "onnx") {
    onnx_path = model_path;
    engine_path = fs::path{model_path}.replace_extension("engine").string();
  } else {
    std::cerr << "[ERROR] Unexpected extension: " << model_path << std::endl;
    std::exit(1);
  }

  std::cout << "[INFO] engine: " << engine_path << ", onnx: " << onnx_path << std::endl;

  std::unique_ptr<ssd::Model> model_ptr;
  if (fs::exists(engine_path)) {
    std::cout << "[INFO] Found engine file: " << engine_path << std::endl;
    model_ptr.reset(new ssd::Model(engine_path, box_first, verbose));
    if (max_batch_size != model_ptr->getMaxBatchSize()) {
      std::cout << "[INFO] Required max batch size " << max_batch_size
                << "does not correspond to Profile max batch size " << model_ptr->getMaxBatchSize()
                << ". Rebuild engine from onnx." << std::endl;
      model_ptr.reset(new ssd::Model(onnx_path, precision, max_batch_size, box_first, verbose));
      model_ptr->save(engine_path);
    }
  } else {
    std::cout << "[INFO] Could not find " << engine_path
              << ", try making TensorRT engine from onnx." << std::endl;
    model_ptr.reset(new ssd::Model(onnx_path, precision, max_batch_size, box_first, verbose));
    model_ptr->save(engine_path);
  }

  std::unique_ptr<float[]> out_scores =
    std::make_unique<float[]>(model_ptr->getMaxBatchSize() * model_ptr->getMaxDets());
  std::unique_ptr<float[]> out_boxes =
    std::make_unique<float[]>(model_ptr->getMaxBatchSize() * model_ptr->getMaxDets() * 4);
  if (!model_ptr->detect(img, out_scores.get(), out_boxes.get())) {
    std::cerr << "[ERROR] Fail to inference" << std::endl;
    std::exit(1);
  }

  cv::Mat viz = drawOutput(img, out_scores.get(), out_boxes.get(), config);
  cv::imshow("Output", viz);
  cv::waitKey(0);
}
