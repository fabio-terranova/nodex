#include "Core.h"
#include <Eigen/Dense>
#include <vector>

namespace Noddy {
  namespace Core {
    std::string getCoreVersion() { return "Noddy Core v0.0.1"; }

    using SignalMatrix = Eigen::Matrix<double, 2, 2>;

    struct Signal {
      SignalMatrix data;
      float        fs;
    };

    class Node {
    public:
      virtual ~Node() = default;

      virtual Signal process(const Signal& input) = 0;
    };

    class ForwardNode final : public Node {
    public:
      Signal process(const Signal& input) { return input; }
    };

    class DoubleNode final : public Node {
    public:
      Signal process(const Signal& input) {
        return Signal{input.data * 2, input.fs};
      }
    };

    class SqrtNode final : public Node {
    public:
      Signal process(const Signal& input) {
        return Signal{input.data.cwiseAbs().cwiseSqrt(), input.fs};
      }
    };

    using NodeRefsVector = std::vector<std::reference_wrapper<Node>>;

    class Graph {
    public:
      Graph(const NodeRefsVector nodes) : nodes_{nodes} {}

      void   addNode(Node& node);
      Signal execute(const Signal& input);

    private:
      NodeRefsVector nodes_;
    };

    void Graph::addNode(Node& node) { nodes_.push_back(node); }

    Signal Graph::execute(const Signal& input) {
      Signal current{input};
      for (auto node : nodes_) {
        current = node.get().process(current);
      }

      return current;
    }

    class Pipeline {
    public:
      Pipeline(Graph graph) : graph_{graph} {}

      Pipeline bandpass(double low, double high) const;

      Signal run(const Signal& input) const;

    private:
      Graph graph_;
    };
  } // namespace Core
} // namespace Noddy
