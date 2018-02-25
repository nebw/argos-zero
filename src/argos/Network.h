#pragma once

#include "mxnet-cpp/MxNetCpp.h"
#include "ego.hpp"

class Network {
public:
    struct Result {
        struct Candidate {
            Candidate(float prior, Vertex&& vertex) : prior(prior), vertex(std::move(vertex)) {}

            bool operator<(const Candidate& right) const { return prior < right.prior; }

            const float prior;
            const Vertex vertex;
        };

        typedef std::vector<Candidate> Candidates;

        Candidates candidates;
        float value;

        Result(Candidates&& candidates, float value) : candidates(std::move(candidates)), value(value) {}
    };

    Network(const std::string& path);
    Result apply(Board const& board);

private:
    mxnet::cpp::Symbol _net;
    std::unique_ptr<mxnet::cpp::Executor> _executor;
    std::map<std::string, mxnet::cpp::NDArray> _args_map;
};
