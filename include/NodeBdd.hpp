#ifndef NODE_DURATION_HPP
#define NODE_DURATION_HPP
#include <fmt/core.h>
#include <gurobi_c++.h>
#include <array>
#include <boost/dynamic_bitset.hpp>
#include <memory>
#include "Label.hpp"
#include "ModelInterface.hpp"
#include "NodeBase.hpp"

template <typename T = double>
class NodeBdd : public NodeBase {
   private:
    Job* job{nullptr};
    int  weight{};

   public:
    std::array<Label<NodeBdd<T>, T>, 2>                 forward_label{};
    std::array<Label<NodeBdd<T>, T>, 2>                 backward_label{};
    std::array<std::vector<std::weak_ptr<BddCoeff>>, 2> coeff_list{};
    std::array<std::vector<std::weak_ptr<NodeId>>, 2>   in_edges{};

    std::shared_ptr<NodeId> ptr_node_id{nullptr};
    std::array<double, 2>   cost{0.0, 0.0};
    std::array<double, 2>   reduced_cost{0.0, 0.0};
    std::array<double, 2>   lp_x{0.0, 0.0};
    std::array<double, 2>   best_sol_x{0.0, 0.0};

    std::array<bool, 2>     calc{true, true};
    size_t                  key{};
    bool                    visited{false};
    bool                    lp_visited{false};
    boost::dynamic_bitset<> all{};
    std::array<int, 2>      backward_distance{};
    std::array<int, 2>      in_degree{};
    std::array<GRBVar, 2>   y{};
    std::array<GRBVar, 2>   r{};
    GRBVar                  sigma{};
    std::array<double, 2>   coeff_cut{0.0, 0.0};

    /**
     * Constructor
     */
    NodeBdd() = default;
    NodeBdd(size_t i, size_t j) : NodeBase(i, j) {}

    NodeBdd<T>(const NodeBdd<T>& src) = default;
    NodeBdd<T>(NodeBdd<T>&& src) noexcept = default;
    NodeBdd<T>& operator=(const NodeBdd<T>& src) = default;
    NodeBdd<T>& operator=(NodeBdd<T>&& src) noexcept = default;
    ~NodeBdd<T>() = default;

    void set_weight(int _weight) { weight = _weight; }

    [[nodiscard]] int get_weight() const { return weight; }
    void              set_job(Job* _job) { job = _job; }

    [[nodiscard]] Job* get_job() const { return job; }

    [[nodiscard]] inline size_t get_nb_job() const { return job->job; }

    void reset_reduced_costs() { reduced_cost = cost; }

    void reset_reduced_costs_farkas() { reduced_cost = {0.0, 0.0}; }

    void reset_lp_x() {
        lp_x = {0.0, 0.0};
        lp_visited = false;
    }

    void add_coeff_list(const std::shared_ptr<BddCoeff>& ptr, bool high) {
        if (high) {
            coeff_list[1].push_back(ptr);
        } else {
            coeff_list[0].push_back(ptr);
        }
    }

    void add_coeff_list_clear() {
        coeff_list[0].clear();
        coeff_list[1].clear();
    }

    void adjust_reduced_costs(double _x, bool high) {
        if (high) {
            reduced_cost[1] -= _x;
        } else {
            reduced_cost[0] -= _x;
        }
    }

    bool operator!=(NodeBdd const& o) const { return !operator==(o); }

    friend std::ostream& operator<<(std::ostream& os, NodeBdd const& o) {
        os << "(" << o.branch[0];

        for (int i = 1; i < 2; ++i) {
            os << "," << o.branch[i];
        }

        return os << ")";
    }

    void init_node(int                   _weight,
                   [[maybe_unused]] bool _root_node = false,
                   bool                  _terminal_node = false) {
        if (!_terminal_node) {
            weight = _weight;
        } else {
            set_job(nullptr);
            weight = -1;
        }
    }

    void set_node_id_label(NodeId _id) {
        for (auto j = 0UL; j < 2; j++) {
            backward_label[j].set_node_id(_id);
            forward_label[j].set_node_id(_id);
        }
    }

    void set_job_label(Job* _job) {
        for (auto j = 0UL; j < 2; j++) {
            backward_label[j].set_job(_job);
            forward_label[j].set_job(_job);
        }
    }

    friend bool operator<=>(const NodeBdd<T>& lhs, const NodeBdd<T>& rhs) {
        return lhs.forward_label[0] <=> rhs.forward_label[0].f;
    }
};

#endif  // NODE_DURATION_HPP
