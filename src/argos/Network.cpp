#include "Network.h"

#include "Config.h"

using namespace mxnet::cpp;

Network::Network(const std::string& path)
{
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

    _args_map["data"] = NDArray(Shape(1, NetworkFeatures::NUM_FEATURES, 19, 19), global_ctx);

    _executor = std::unique_ptr<mxnet::cpp::Executor>(
        _net.SimpleBind(global_ctx,
                        _args_map,
                        std::map<std::string, NDArray>(),
                        std::map<std::string, OpReqType>(),
                        aux_map));
    NDArray::WaitAll();
}

Network::Result Network::apply(const Board& board)
{
    static const size_t inputs = 1;
    static const size_t channels = NetworkFeatures::NUM_FEATURES;
    static const size_t width = 19;
    static const size_t height = 19;

    const auto planes = board.getFeatures().getPlanes();

    const mx_float* data = &planes[0][0][0];
    _args_map["data"].SyncCopyFromCPU(data, inputs * channels * width * height);
    NDArray::WaitAll();
    _executor->Forward(false);
    NDArray::WaitAll();

    std::vector<float> policyOutput;
    _executor->outputs[0].SyncCopyToCPU(&policyOutput);
    std::vector<float> valueOutput;
    _executor->outputs[1].SyncCopyToCPU(&valueOutput);
    NDArray::WaitAll();

    Network::Result::Candidates candidates;
    for (size_t row = 0; row < 19; ++row) {
        for (size_t col = 0; col < 19; ++col) {
            const size_t posIdx = row * 19 + col;
            candidates.emplace_back(policyOutput[posIdx], Vertex::OfCoords(row, col));
        }
    }
    candidates.emplace_back(policyOutput[19*19], Vertex::Pass());

    return Network::Result(std::move(candidates), valueOutput[0] * 2 - 1);
}
