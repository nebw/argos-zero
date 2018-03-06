#include "Network.h"

#include "Config.h"

using namespace mxnet::cpp;

Network::Network(const std::string& path) {
    const std::string symbolPath = path + "-symbol.json";
    const std::string paramPath = path + "-0000.params";

    _net = Symbol::Load(symbolPath);

    Context global_ctx(static_cast<DeviceType>(config::defaultDevice), 0);

    std::map<std::string, NDArray> aux_map;

    std::map<std::string, NDArray> paramters;
    NDArray::Load(paramPath, nullptr, &paramters);
    for (const auto& k : paramters) {
        if (k.first.substr(0, 4) == "aux:") {
            auto name = k.first.substr(4, k.first.size() - 4);
            aux_map[name] = k.second.Copy(global_ctx);
        }
        if (k.first.substr(0, 4) == "arg:") {
            auto name = k.first.substr(4, k.first.size() - 4);
            _args_map[name] = k.second.Copy(global_ctx);
        }
    }
    NDArray::WaitAll();

    _args_map["data"] = NDArray(
        Shape(config::tree::batchSize, NetworkFeatures::NUM_FEATURES, config::boardSize, config::boardSize), global_ctx);

    _executor = std::unique_ptr<mxnet::cpp::Executor>(
        _net.SimpleBind(global_ctx, _args_map, std::map<std::string, NDArray>(),
                        std::map<std::string, OpReqType>(), aux_map));
    NDArray::WaitAll();
}

std::array<Network::Result, config::tree::batchSize> Network::apply(
    const std::array<NetworkFeatures::Planes, config::tree::batchSize>& planes) {
    static const size_t inputs = config::tree::batchSize;
    static const size_t channels = NetworkFeatures::NUM_FEATURES;
    static const size_t width = config::boardSize;
    static const size_t height = config::boardSize;

    const mx_float* data = &planes[0][0][0][0];
    _args_map["data"].SyncCopyFromCPU(data, inputs * channels * width * height);
    NDArray::WaitAll();
    _executor->Forward(false);
    NDArray::WaitAll();

    std::vector<float> policyOutput;
    _executor->outputs[0].SyncCopyToCPU(&policyOutput);
    std::vector<float> valueOutput;
    _executor->outputs[1].SyncCopyToCPU(&valueOutput);
    NDArray::WaitAll();

    std::array<Network::Result, config::tree::batchSize> results;
    for (size_t i = 0; i < config::tree::batchSize; ++i) {
        Network::Result::Candidates candidates;
        for (size_t row = 0; row < width; ++row) {
            for (size_t col = 0; col < height; ++col) {
                const size_t posIdx = i * height * width + row * width + col;
                candidates.emplace_back(policyOutput[posIdx], Vertex::OfCoords(row, col));
            }
        }
        //candidates.emplace_back(policyOutput[width * height], Vertex::Pass());
        candidates.emplace_back(0.0000001f, Vertex::Pass());
        results[i] = Network::Result(std::move(candidates), valueOutput[i] * 2 - 1);
    }
    return results;
}
