#ifndef PRICER_SOLVER_BDD_FORWARD_HPP
#define PRICER_SOLVER_BDD_FORWARD_HPP
#include "PricerEvaluateBdd.hpp"
#include "PricerSolverBdd.hpp"
// #include "interval.h"

class PricerSolverBddSimple : public PricerSolverBdd {
   private:
    ForwardBddSimpleDouble  evaluator;
    BackwardBddSimpleDouble reversed_evaluator;
    BackwardBddFarkasDouble farkas_evaluator;

   public:
    // PricerSolverBddSimple(GPtrArray*  _jobs,
    //                       int         _num_machines,
    //                       GPtrArray*  _ordered_jobs,
    //                       const char* p_name,
    //                       int         _Hmax,
    //                       int*        _take_jobs,
    //                       double      _UB);
    // PricerSolverBddSimple(const PricerSolverBddSimple& src)
    //     : PricerSolverBdd(src),
    //       evaluator(src.evaluator),
    //       reversed_evaluator(src.reversed_evaluator),
    //       farkas_evaluator(src.farkas_evaluator){};

    explicit PricerSolverBddSimple(const Instance& instance);

    PricerSolverBddSimple(const PricerSolverBddSimple&) = default;
    PricerSolverBddSimple(PricerSolverBddSimple&&) = default;
    PricerSolverBddSimple& operator=(PricerSolverBddSimple&&) = default;
    PricerSolverBddSimple& operator=(const PricerSolverBddSimple&) = delete;
    virtual ~PricerSolverBddSimple() override = default;

    std::unique_ptr<PricerSolverBase> clone() const override {
        // auto* tmp =
        //     g_ptr_array_copy(get_ordered_jobs(), g_copy_interval_pair, NULL);
        return std::make_unique<PricerSolverBddSimple>(*this);
    };

    OptimalSolution<double> pricing_algorithm(double* _pi) override;
    OptimalSolution<double> farkas_pricing(double* _pi) override;

    void compute_labels(double* _pi);
    bool evaluate_nodes(double* pi) final;
};

class PricerSolverBddCycle : public PricerSolverBdd {
   private:
    ForwardBddCycleDouble   evaluator;
    BackwardBddCycleDouble  reversed_evaluator;
    BackwardBddFarkasDouble farkas_evaluator;

   public:
    // PricerSolverBddCycle(GPtrArray*  _jobs,
    //                      int         _num_machines,
    //                      GPtrArray*  _ordered_jobs,
    //                      const char* p_name,
    //                      int         _Hmax,
    //                      int*        _take_jobs,
    //                      double      _UB);

    explicit PricerSolverBddCycle(const Instance& instance);

    PricerSolverBddCycle(const PricerSolverBddCycle&) = default;
    PricerSolverBddCycle(PricerSolverBddCycle&&) = default;
    PricerSolverBddCycle& operator=(PricerSolverBddCycle&&) = default;
    PricerSolverBddCycle& operator=(const PricerSolverBddCycle&) = delete;
    virtual ~PricerSolverBddCycle() override = default;

    std::unique_ptr<PricerSolverBase> clone() const override {
        // auto* tmp =
        //     g_ptr_array_copy(get_ordered_jobs(), g_copy_interval_pair, NULL);
        return std::make_unique<PricerSolverBddCycle>(*this);
    }

    OptimalSolution<double> pricing_algorithm(double* _pi) override;
    OptimalSolution<double> farkas_pricing(double* _pi) override;

    void compute_labels(double* _pi);
    bool evaluate_nodes(double* pi) final;
};

#endif  // PRICER_SOLVER_BDD_FORWARD_HPP
