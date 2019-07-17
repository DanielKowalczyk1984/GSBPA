#include "PricerSolverBase.hpp"
#include "MipGraph.hpp"

class PricerSolverArcTimeDp : public PricerSolverBase {
private:
    int Hmax;
    int n;
    size_t size_graph;
    boost::unordered_set <Job*> **graph;
    std::vector<Job*> vector_jobs;
    Job j0;
    double **F;
    Job ***A;
    int **B;
    int **p_matrix;

    typedef boost::unordered_set<Job*>::iterator job_iterator;
public:
    PricerSolverArcTimeDp(GPtrArray *_jobs, int _num_machines, int _Hmax);
    ~PricerSolverArcTimeDp();
    void init_table() override;

    void reduce_cost_fixing(double *pi, int UB, double LB) override;
    void evaluate_nodes(double *pi, int UB, double LB) override;
    
    void build_mip() override;
    void construct_lp_sol_from_rmp(const double *columns, const GPtrArray* schedule_sets, int num_columns, double *x) override;
    void represent_solution(Solution *sol)  override;
    double* project_solution(Solution *sol) override;
    void add_constraint(Job *job, GPtrArray *list, int order) override;
    void disjunctive_inequality(double *x, Solution *sol) override;

    OptimalSolution<double> pricing_algorithm(double *_pi) override;
    
    void iterate_zdd() override;
    void create_dot_zdd(const char* name) override;
    void print_number_nodes_edges() override;
    int get_num_remove_nodes() override ;
    int get_num_remove_edges() override ;
    size_t get_datasize() override ;
    size_t get_size_graph() override ;
    int get_num_layers() override ;
    void print_num_paths() override;
    double get_cost_edge(int idx) override;

    bool check_schedule_set(GPtrArray* set) override;


    int delta1(const int &i,const int &j, const int &t) {
        Job *tmp_i = vector_jobs[i];
        Job *tmp_j = vector_jobs[j];
        return (value_Fj(t, tmp_i) + value_Fj(t + tmp_j->processing_time, tmp_j))
            - (value_Fj(t + tmp_j->processing_time - tmp_i->processing_time, tmp_j)
                + value_Fj(t + tmp_j->processing_time, tmp_i) );
    }

    void remove_arc(const int &i, const int &j, const int &t) {
        Job *tmp_i = vector_jobs[i];
        // auto it = graph[j][t].find(tmp_i);
        graph[j][t].erase(tmp_i) ;
    }

    int delta2(const int &j, const int &t) {
        Job *tmp_j = vector_jobs[j];
        return value_Fj(t, tmp_j) - value_Fj(t + 1, tmp_j);
    }
};