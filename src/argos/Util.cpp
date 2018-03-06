#include "Util.h"

#include <pthread.h>
#include <algorithm>
#include <iomanip>
#include <ostream>
#include <thread>

#include "Node.h"
#include "Position.h"

namespace {
struct TreeBuffer {
    size_t rootEvaluations;
    std::string depth;

    TreeBuffer(size_t rootEvaluations) : rootEvaluations(rootEvaluations) {}

    void push(char c) {
        depth.push_back(' ');
        depth.push_back(' ');
        depth.push_back(c);
        depth.push_back(' ');
    }

    void pop() {
        depth.pop_back();
        depth.pop_back();
        depth.pop_back();
        depth.pop_back();
    }
};
}  // namespace

void printTree(Node* node, std::string const& posName, Player const& player, std::ostream& out,
               TreeBuffer& buffer, float fractionPrint) {
    out << std::fixed << std::setprecision(2) << posName << " ("
        << node->statistics().num_evaluations << " | "
        << "WR:" << node->winrate(player) * 100 << "%"
        << " PN:" << node->statistics().prior.load() * 100 << "%"
        << ")\n";
    {
        auto const& children = node->children();
        if (!children.is_initialized()) { return; }
    }

    std::vector<Node*> children;
    for (size_t i = 0; i < node->children()->size(); ++i) {
        children.push_back((*node->children())[i].get());
    }
    std::sort(children.begin(), children.end(), [](Node* a, Node* b) {
        return a->statistics().num_evaluations > b->statistics().num_evaluations;
    });

    for (size_t cnt = 0; cnt < children.size(); ++cnt) {
        Node* child = children[cnt];

        if ((child->statistics().num_evaluations >
             node->statistics().num_evaluations * fractionPrint) &&
            (child->statistics().num_evaluations >
             static_cast<float>(buffer.rootEvaluations) / 250)) {
            out << buffer.depth << " `--";
            buffer.push((cnt + 1) < child->isExpanded() ? '|' : ' ');
            const auto playerStr = child->position()->actPlayer().Other().ToGtpString() + ":";
            printTree(child, playerStr + child->parentMove().ToGtpString(), player, out, buffer,
                      fractionPrint);
            buffer.pop();
        }
    }
}

void printTree(Node* node, Player const& player, ostream& out, float fractionPrint) {
    TreeBuffer buffer(node->statistics().num_evaluations);

    const auto playerStr = node->position()->actPlayer().Other().ToGtpString() + ":";
    printTree(node, playerStr + node->parentMove().ToGtpString(), player, out, buffer,
              fractionPrint);

    out.flush();
}

void resetThreadAffinity() {
    cpu_set_t cpuset;
    pthread_t thread = pthread_self();
    CPU_ZERO(&cpuset);
    for (size_t j = 0; j < std::thread::hardware_concurrency(); ++j) {
        CPU_SET(j, &cpuset);
    }
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}
