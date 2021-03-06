set(sources
    "src/BranchBoundTree.cpp"
    "src/BranchNode.cpp"
    "src/Instance.cpp"
    "src/Interval.cpp"
    "src/IO.cpp"
    "src/Job.cpp"
    "src/LocalSearch.cpp"
    "src/Lowerbound.cpp"
    "src/Model.cpp"
    "src/ModelInterface.cpp"
    "src/Parms.cpp"
    "src/PricerSolverArcTimeDP.cpp"
    "src/PricerSolverBase.cpp"
    "src/PricerSolverBdd.cpp"
    "src/PricerSolverBddBackward.cpp"
    "src/PricerSolverBddForward.cpp"
    "src/PricerSolverSimpleDP.cpp"
    "src/PricerSolverWrappers.cpp"
    "src/PricerSolverZdd.cpp"
    "src/PricerSolverZddBackward.cpp"
    "src/PricerSolverZddForward.cpp"
    "src/PricingStabilization.cpp"
    "src/Scheduleset.cpp"
    "src/SeperationSolver.cpp"
    "src/Solution.cpp"
    "src/StabilizationWrappers.cpp"
    "src/Statistics.cpp"
    "src/wct.cpp"
    "src/wctprivate.cpp"
    "src/ZeroHalfCuts.cpp"
    "src/ZeroHalfSystem.cpp"
)

set(exe_sources "src/main.cpp")

set(headers ${PROJECT_SOURCE_DIR}/include)
