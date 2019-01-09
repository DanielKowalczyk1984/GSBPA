#ifndef DURATION_ZDD_HPP
#define DURATION_ZDD_HPP
#include <tdzdd/DdEval.hpp>
#include <OptimalSolution.hpp>
#include <node_duration.hpp>
#include <vector>
#include <algorithm>
using namespace std;

template<typename T>
class ForwardZddNode {
  public:
    std::vector<std::shared_ptr<Node<T>>>  list;
    Job *job;

    ForwardZddNode() : job(nullptr) {}

    ~ForwardZddNode() {
        list.clear();
    }


    std::shared_ptr<Node<T>> add_weight(int _weight, int _layer, bool _rootnode = false, bool _terminal_node = false){
        for (auto &it : list) {
            if (it->GetWeight() == _weight) {
                return it;
            }
        }

        std::shared_ptr<Node<T>> node = std::make_shared<Node<T>>(_weight, _layer, _rootnode, _terminal_node);
        node->set_job(job);
        list.push_back(std::move(node));
        return list.back();
    }

    void set_job(Job *_job){
        job = _job;
    }

    Job* get_job(){
        return job;
    }
};

template<typename T>
bool my_compare(const std::shared_ptr<Node<T>> &lhs, const std::shared_ptr<Node<T>> &rhs){
    return *lhs < *rhs;
}

template<typename E, typename T> class ForwardZddBase : public 
    tdzdd::DdEval<E, ForwardZddNode<T>, Optimal_Solution<T>> {
protected:
    T *pi;
    int num_jobs;
public:
    ForwardZddBase(T *_pi, int _num_jobs): pi(_pi), num_jobs(_num_jobs) {
    }

    ForwardZddBase(int _num_jobs)
    :  pi(nullptr), num_jobs(_num_jobs){
    }

    ForwardZddBase(){
        pi = nullptr;
        num_jobs = 0;
    }

    ForwardZddBase(const ForwardZddBase<E, T> &src) {
        pi = src.pi;
        num_jobs = src.num_jobs;
    }

    void initializepi(T *_pi){
        pi = _pi;
    }

    virtual void initializenode(ForwardZddNode<T>& n) const  = 0;

    virtual void initializerootnode(ForwardZddNode<T>& n) const  = 0;

    virtual void evalNode(ForwardZddNode<T>& n) const = 0;

    // virtual Optimal_Solution<double> get_objective(ForwardZddNode<T>& n) const = 0;

    Optimal_Solution<T> get_objective(ForwardZddNode<T> &n) const {
        Optimal_Solution<T> sol(-pi[num_jobs]);

        int weight;


        auto m =  std::max_element(n.list.begin(), n.list.end(),my_compare<T>);
        weight = (*m)->GetWeight();

        Label<T> *ptr_node = &((*m)->state1);

        while(ptr_node->GetPrev() != nullptr) {
            Label<T> *aux_prev_node = ptr_node->GetPrev();
            Job *aux_job = aux_prev_node->GetJob();
            if(ptr_node->GetHigh()) {
                sol.C_max += aux_job->processingime;
                if(ptr_node->GetNode()) {
                    sol.cost = sol.cost + value_Fj(ptr_node->GetWeight(), aux_job) ;
                    sol.obj += pi[aux_job->job] - value_Fj(ptr_node->GetWeight(), aux_job);
                } else {
                    sol.cost = sol.cost + value_Fj(weight, aux_job);
                    sol.obj += pi[aux_job->job] - value_Fj(weight, aux_job);
                }
                g_ptr_array_add(sol.jobs, aux_job);
            }
            ptr_node = aux_prev_node;
        }

        assert(sol.C_max == weight);

        return sol;
    }

};

template<typename E, typename T> class ForwardZddCycle : public ForwardZddBase<E, T> {

  public:
    using ForwardZddBase<E, T>::pi;
    using ForwardZddBase<E, T>::num_jobs;
    ForwardZddCycle(T *_pi, int _num_jobs): ForwardZddBase<E, T>(_pi, _num_jobs) {
    }

    explicit ForwardZddCycle(int _num_jobs) : ForwardZddBase<E, T>(_num_jobs){
    }

    ForwardZddCycle(): ForwardZddBase<E, T>(){
        pi = nullptr;
        num_jobs = 0;
    }

    ForwardZddCycle(const ForwardZddCycle<E, T> &src) {
        pi = src.pi;
        num_jobs = src.num_jobs;
    }

    void initializenode(ForwardZddNode<T>& n) const override {
        for (auto &it: n.list) {
            if(it->GetWeight() == 0) {
                it->state1.UpdateSolution(-pi[num_jobs], nullptr, false);
                it->state2.UpdateSolution(-DBL_MAX/2, nullptr, false);
            } else {
                it->state1.UpdateSolution(-DBL_MAX/2, nullptr, false);
                it->state2.UpdateSolution(-DBL_MAX/2, nullptr, false);
            }
        }
    }

    void initializerootnode(ForwardZddNode<T> &n) const override {
        for(auto &it: n.list){
            it->state1.f = -pi[num_jobs];
            it->state2.SetF(-DBL_MAX/2);
        }
    }

    void evalNode(ForwardZddNode<T> &n) const override
    {
        Job *tmp_j = n.get_job();
        assert(tmp_j != nullptr);

        for (auto &it : n.list) {
            int      weight = it->GetWeight();
            T g;
            std::shared_ptr<Node<T>> p0 = it->n;
            std::shared_ptr<Node<T>> p1 = it->y;
            double result = - value_Fj(weight + tmp_j->processingime, tmp_j) + pi[tmp_j->job];

            /**
             * High edge calculation
             */
            Job *prev = it->state1.GetPrevJob();
            Job *aux1 = p1->state1.GetPrevJob();
            double diff = (prev == nullptr ) ? true : (value_diff_Fij(weight, tmp_j, prev) >= 0 );

            if(prev != tmp_j && diff) {
                g = it->state1.GetF() + result;
                if(g > p1->state1.GetF()) {
                    if(aux1 != tmp_j) {
                        p1->state2.UpdateSolution(p1->state1);
                    }
                    p1->state1.UpdateSolution(g, &(it->state1), true);
                } else if ((g > p1->state2.GetF()) && (aux1 != tmp_j)) {
                    p1->state2.UpdateSolution(g, &(it->state1), true);
                }
            } else  {
                g = it->state2.GetF() + result;
                if(g > p1->state1.GetF()) {
                    if(aux1 != tmp_j) {
                        p1->state2.UpdateSolution(p1->state1);
                    }
                    p1->state1.UpdateSolution(g, &(it->state2), true);
                } else if ((g >= p1->state2.GetF()) && (aux1 != tmp_j)) {
                    p1->state2.UpdateSolution(g, &(it->state2), true);
                }
            }

            /**
             * Low edge calculation
             */
            aux1 = p0->state1.GetPrevJob();
            if(it->state1.GetF() > p0->state1.GetF()) {
                if(prev != aux1) {
                    p0->state2.UpdateSolution(p0->state1);
                }
                p0->state1.UpdateSolution(it->state1);
            } else if ((it->state1.GetF() > p0->state2.GetF()) && (aux1 != prev)){
                p0->state2.UpdateSolution(it->state1);
            }
        }
    }

    Optimal_Solution<T> getValue(ForwardZddNode<T> const &n){
        Optimal_Solution<T> sol;

        return sol;
    }
};

template<typename E, typename T> class ForwardZddSimple : public ForwardZddBase<E, T> {
  public:
    using ForwardZddBase<E, T>::pi;
    using ForwardZddBase<E, T>::num_jobs;
    ForwardZddSimple(T *_pi, int _num_jobs): ForwardZddBase<E, T>(_pi, _num_jobs) {
    }

    explicit ForwardZddSimple(int _num_jobs)
    :  ForwardZddBase<E, T> (_num_jobs){
    }

    ForwardZddSimple(){
        pi = nullptr;
        num_jobs = 0;
    }

    ForwardZddSimple(const ForwardZddSimple<E, T> &src) {
        pi = src.pi;
        num_jobs = src.num_jobs;
    }

    void initializenode(ForwardZddNode<T>& n) const override {
        for (auto &it: n.list) {
            if(it->GetWeight() == 0) {
                it->state1.UpdateSolution(-pi[num_jobs], nullptr, false);
            } else {
                it->state1.UpdateSolution(-DBL_MAX/2, nullptr, false);
            }
        }
    }

    void initializerootnode(ForwardZddNode<T> &n) const override {
        for(auto &it: n.list){
            it->state1.f = -pi[num_jobs];
        }
    }

    void initializepi(T *_pi){
        pi = _pi;
    }

    void evalNode(ForwardZddNode<T> &n) const override {
        Job *tmp_j = n.get_job();
        assert(tmp_j != nullptr);

        for (auto &it : n.list) {
            int      weight = it->GetWeight();
            T g;
            std::shared_ptr<Node<T>> p0 = it->n;
            std::shared_ptr<Node<T>> p1 = it->y;
            double result = - value_Fj(weight + tmp_j->processingime, tmp_j) + pi[tmp_j->job];

            /**
             * High edge calculation
             */
            g = it->state1.GetF() + result;
            if(g > p1->state1.GetF()) {
                p1->state1.UpdateSolution(g, &(it->state1), true);
            }

            /**
             * Low edge calculation
             */
            if(it->state1.GetF() > p0->state1.GetF()) {
                p0->state1.UpdateSolution(it->state1);
            }
        }
    }

    Optimal_Solution<T> getValue(ForwardZddNode<T> const &n){
        Optimal_Solution<T> sol;
        return sol;
    }
};

#endif // DURATION_ZDD_HPP