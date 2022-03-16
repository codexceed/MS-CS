from dpll import DPLLSolver


if __name__ == "__main__":
    with open("test_cases/case1/input", "r") as f:
        cnf = f.read().split("\n")

    solver = DPLLSolver(cnf, True)
    assignments = solver.solve()
    assignments = sorted([(k, v) for k, v in assignments.items()], key=lambda x: x[0])

    print("\n".join([f"{x} = {str(y).lower()}" for x, y in assignments]))
