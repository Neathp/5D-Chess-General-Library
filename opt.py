import subprocess, optuna
from optuna.importance import get_param_importances
import optuna.visualization as vis
import numpy as np
import os
import time
from optuna.samplers import BaseSampler

class SPSASampler(BaseSampler):
    def __init__(self, param_names, a=0.1, c=0.1, alpha=0.602, gamma=0.101, seed=None):
        self.param_names = param_names
        self.rng = np.random.RandomState(seed)
        self.a = a
        self.c = c
        self.alpha = alpha
        self.gamma = gamma
        self.k = 0
        self.theta = {p: 0.0 for p in param_names}
        self.delta = None  # store perturbation vector

    def reseed_rng(self) -> None:
        self.rng.seed()

    def infer_relative_search_space(self, study, trial):
        return {}

    def sample_relative(self, study, trial, search_space):
        return {}

    def sample_independent(self, study, trial, param_name, param_distribution):
        # First trial initializes theta to midpoints
        if self.k == 0 and self.theta[param_name] == 0.0:
            if isinstance(param_distribution, optuna.distributions.IntDistribution):
                self.theta[param_name] = (param_distribution.low + param_distribution.high) / 2
            elif isinstance(param_distribution, optuna.distributions.FloatDistribution):
                self.theta[param_name] = (param_distribution.low + param_distribution.high) / 2

        # SPSA coefficients
        ak = self.a / ((self.k + 1) ** self.alpha)
        ck = self.c / ((self.k + 1) ** self.gamma)

        # Even trial: create perturbation vector
        if trial.number % 2 == 0:
            self.delta = {p: self.rng.choice([-1, 1]) for p in self.param_names}
            val = self.theta[param_name] + ck * self.delta[param_name]
        else:
            # Odd trial: use negative perturbation
            val = self.theta[param_name] - ck * self.delta[param_name]

        # Clamp
        if isinstance(param_distribution, optuna.distributions.IntDistribution):
            return int(np.clip(val, param_distribution.low, param_distribution.high))
        else:
            return float(np.clip(val, param_distribution.low, param_distribution.high))

    def update_after_pair(self, study, trial):
        # Only update after odd-numbered trial (i.e., both ± done)
        if trial.number % 2 == 1:
            t_plus = study.trials[trial.number - 1]
            t_minus = trial
            if t_plus.value is None or t_minus.value is None:
                return

            ak = self.a / ((self.k + 1) ** self.alpha)
            ck = self.c / ((self.k + 1) ** self.gamma)

            # Gradient estimate
            for p in self.param_names:
                ghat = (t_plus.value - t_minus.value) / (2 * ck * self.delta[p])
                self.theta[p] -= ak * ghat

            self.k += 1

def objective(trial):
    args = ["out/opt.exe"]

    eval = False
    move = True
    ext = True 

    if eval:
        args.append("eval")
        args.append(str(trial.suggest_int("softmate", -20000, -1000)))
        args.append(str(trial.suggest_float("pieceVal", 1.0, 15.0)))
        args.append(str(trial.suggest_float("moveVal", 0.5, 5.0)))
        args.append(str(trial.suggest_float("kingExp", 0.5, 5.0)))
        args.append(str(trial.suggest_int("kingExpTol", 0, 6)))
        args.append(str(trial.suggest_int("tlValue", -5000, -100)))
        args.append(str(trial.suggest_int("unmoved", 0, 10)))

    if move:
        args.append("move")
        # Fixed params (tight ranges)
        args.append(str(629))  # inactiveTravelPenalty
        args.append(str(419))  # travelBasePenalty
        args.append(str(385))  # travelPenalty
        args.append(str(trial.suggest_int("killerBonus", 700, 4000)))
        args.append(str(trial.suggest_int("captureMVVMultiplier", -15, 15)))
        args.append(str(trial.suggest_int("captureLVAMalus", -15, 15)))
        args.append(str(trial.suggest_int("captureBaseBonus", 0, 500)))
        args.append(str(trial.suggest_int("quietMovePenaltyMultiplier", 0, 300)))
        args.append(str(trial.suggest_int("checkBonus", 1, 900)))
        args.append(str(trial.suggest_int("pastCheckBonus", 1, 2000)))
        args.append(str(503))  # travelCheckBonus

    if ext:
        args.append("ext")
        args.append(str(trial.suggest_int("PVExt", 0, 2)))
        args.append(str(trial.suggest_int("NMHReduct", 0, 3)))
        args.append(str(1))  # travelReduct
        args.append(str(1))  # inactiveTravelReduct
        args.append(str(trial.suggest_int("onlyReplyThreshold", 1, 5)))
        args.append(str(trial.suggest_int("onlyReply", 0, 2)))
        args.append(str(trial.suggest_int("checkExt", 0, 2)))
        args.append(str(trial.suggest_int("pastCheckExt", 0, 2)))
        args.append(str(1))  # travelCheckExt
        args.append(str(trial.suggest_int("LMR", 1, 4)))
        args.append(str(trial.suggest_int("LMRStart", 5, 50)))
        args.append(str(trial.suggest_int("LMRDepth", 1, 6)))

    start_time = time.time()
    proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    while proc.poll() is None:
        if time.time() - start_time > 300:
            proc.kill()
            raise optuna.TrialPruned()
        time.sleep(0.1)
    
    out, err = proc.communicate()

    if trial.number % 1 == 0:
        print(out)
    
    if "nodes=" not in out:
        print("Program error. stdout:\n", out, "\nstderr:\n", err)
        raise optuna.TrialPruned()
    
    lines = [l for l in out.strip().splitlines() if l.strip()]
    last = lines[-1]
    parts = dict(item.split("=", 1) for item in last.split() if "=" in item)

    solved = int(parts["solved"])
    nodes = int(parts["nodes"])

    max_positions = 14  # number of positions in your list
    score = (max_positions - solved) * 10**12 + nodes
    return score

def print_summary(study, N=10):
    print("Best params:", study.best_params)

    importance = get_param_importances(study)
    print("\nParameter importance:")
    for k, v in importance.items():
        print(f"{k}: {v:.3f}")

    best_trial = study.best_trial
    print("\nBest trial values (rough confidence estimate):")
    for k, v in best_trial.params.items():
        print(f"{k}: {v}")

    fig1 = vis.plot_param_importances(study)
    fig1.show()

    fig2 = vis.plot_optimization_history(study)
    fig2.show()

    valid_trials = [t for t in study.trials if t.value is not None]
    top_trials = sorted(valid_trials, key=lambda t: t.value)[:N]
    param_stats = {}
    for key in study.best_params.keys():
        vals = [t.params[key] for t in top_trials]
        mean = np.mean(vals)
        std = np.std(vals)
        param_stats[key] = (mean, std)

    print("\nParameter confidence-like stats (mean ± std) from top", N, "trials:")
    for k, (mean, std) in param_stats.items():
        print(f"{k}: {mean:.3f} ± {std:.3f}")

# def spsa_optimize(objective, param_names, n_pairs=100):
#     sampler = SPSASampler(param_names)
#     study = optuna.create_study(direction="minimize", sampler=sampler)

#     for _ in range(n_pairs):
#         # Evaluate θ+δ
#         trial1 = study.ask()
#         value1 = objective(trial1)
#         study.tell(trial1, value1)

#         # Evaluate θ–δ
#         trial2 = study.ask()
#         value2 = objective(trial2)
#         study.tell(trial2, value2)

#         # Update parameters
#         sampler.update_after_pair(study, trial2)

#     return study

if __name__ == "__main__":
    sampler = optuna.samplers.TPESampler(
        n_startup_trials=50,     # explore more before exploiting
        multivariate=True,       # better param interaction
        constant_liar=True       # helps with parallel workers
    )
    study = optuna.create_study(direction="minimize", sampler=sampler)
    max_threads = os.cpu_count() or 1
    try:
        study.optimize(objective, n_trials=1000, n_jobs=max(1, max_threads-2))
    except KeyboardInterrupt:
        print("\nInterrupted by user. Printing summary so far...")
        print_summary(study)
        raise SystemExit(0)

    print_summary(study)
