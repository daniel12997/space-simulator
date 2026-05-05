---
type: source
title: "Analytical Derivatives of Rigid Body Dynamics Algorithms"
raw_path: docs/raw/papers/carpentier-2018-rbd-analytical-derivatives.pdf
source_type: paper
reliability: peer-reviewed
ingested: 2026-05-04
authors: [Carpentier, Justin; Mansard, Nicolas]
publication_date: 2018-06
venue: "Robotics: Science and Systems (RSS) 2018, Pittsburgh"
companion_report: "hal-01790934 — Analytical inverse of the joint space inertia matrix"
---

# Carpentier & Mansard (2018) — Analytical Derivatives of Rigid Body Dynamics Algorithms

The derivation paper for the analytical-derivatives feature [[concepts/pinocchio-library|Pinocchio]] uses to make whole-body MPC realistic at closed-loop simulator speeds. Provides closed-form analytical partial derivatives of [[concepts/recursive-newton-euler-algorithm|RNEA]] (inverse dynamics) and [[concepts/articulated-body-algorithm|ABA]] (forward dynamics) with respect to the dynamics input variables (`q`, `q̇`, `τ`, `f^ext`), exploiting [[concepts/spatial-algebra|spatial-algebra]] structure to keep the algorithms compact and to preserve the kinematic-tree sparsity that finite-differences and naïve Lagrangian derivations destroy.

For Apsis, this is the **load-bearing enabler** for REQ-GNC-008 (MPC attitude/orbit controller). See [[concepts/analytical-rbd-derivatives]] for the standalone concept.

## The novel chain-rule insight

§IV.B, Eq. 24. Forward and inverse dynamics are inverses of each other (`FD ∘ ID = id`). Differentiating the composition with respect to any input `u ∈ {q, q̇}`:

```
∂FD/∂u = -M(q)⁻¹ · ∂ID/∂u
```

*"To the best of our knowledge, this is the first time that this specific relation between the partial derivatives of forward and inverse dynamics is highlighted and exploited in order to simplify the underlying computations."* (p. 6)

**Implication for any implementation**: derive RNEA-derivatives carefully; ABA-derivatives fall out from a single matrix multiplication by `-M⁻¹`. There is no separate ABA-derivative algorithm to maintain. This halves the implementation surface and the test surface.

The other side of this identity (Eq. 17): `∂FD/∂τ = M⁻¹` directly, since the dynamics is linear in `τ`. So if you have `M⁻¹` you have the τ-gradient for free.

## Algorithms 2 & 3 — derivatives of RNEA

§III.B and §III.C. Algorithms 2 (forward pass, 6 lines) and 3 (backward pass, 4 lines) extend the RNEA recursion (Algorithm 1) with partial-derivative propagation. The propagated quantities are 6×N matrices (motion-set or force-set), where N is the number of generalized coordinates:

- Forward pass: `∂v_i/∂u`, `∂a_i/∂u`, `∂h_i/∂u`, `∂f_i/∂u` (where `h_i = I_i v_i` is spatial momentum).
- Backward pass: `∂τ_i/∂u`, `∂f_λ(i)/∂u` propagated parent-ward.

The kinematic-tree sparsity is preserved by the chain-rule structure — body `i`'s derivatives depend only on its parent's derivatives, not on the entire state. This is what finite differences cannot capture: any finite-difference column perturbs one input variable, evaluates the *entire* dynamics, and produces a dense gradient even for a sparse Jacobian.

## Equations 13 and 14 — the spatial-algebra trick

§III.C.3. The placement transformation `ⁱX_λ(i)` between a body and its parent depends on `q` through the joint variable `q_i`. The naïve derivative `∂ⁱX_λ(i)/∂q_i` is a 6×6 matrix per joint, so the full `∂X/∂q` would be a 6×6×N tensor — expensive to materialize and hard to chain.

Eq. 13:
```
(∂ⁱX_λ(i)/∂q_i) · v_λ(i) = (ⁱX_λ(i) · v_λ(i)) × S_i
```

Reformulates the derivative as the **action of a known spatial-algebra operator on a known motion vector**. The cross-product `× S_i` is cheap. No tensor materialization. Eq. 14 does the same for the dual force-side action:

```
(∂λ(i)X_iᵀ / ∂u) · f_i = λ(i)X_iᵀ · (S_i ×* f_i)
```

These two relations are the bridge that lets the analytical derivatives match RNEA's O(n) complexity.

## Performance benchmarks

§V, Table I (single-core 2.2 GHz Intel i7, Eigen 3.3.4):

| Robot | DOF | ID | ID derivatives (analytical) | ID derivatives (FD) | Speedup |
|---|---|---|---|---|---|
| KUKA-LWR | 7 | 1.20 µs | 3.34 µs | 21.26 µs | 6.4× |
| HyQ | 18 | 2.14 µs | 7.01 µs | 88.52 µs | 12.6× |
| Atlas | 36 | 5.51 µs | 16.72 µs | 452.46 µs | **27×** |

| Robot | FD | FD derivatives (analytical) | FD derivatives (FD) | Speedup |
|---|---|---|---|---|
| KUKA-LWR | 1.78 µs | 5.78 µs | 22.67 µs | 3.9× |
| HyQ | 4.28 µs | 14.24 µs | 94.23 µs | 6.6× |
| Atlas | 9.81 µs | 45.20 µs | 470.14 µs | 10.4× |

Two trends: analytical derivatives are 3-5× the cost of the dynamics call itself (predictable — same algorithmic structure), and **the speedup over finite differences grows with DOF** because finite differences cost N+1 dynamics calls.

Vs Giftthaler et al. 2017 (current best autodiff + code generation): 30% faster on ID derivatives, 60% faster on FD derivatives — *without* using code generation themselves (§V.A.2).

## Numerical accuracy — exact zeros where they should be

§V.B, Fig. 3. With `q̇ = q̈ = 0`, gravity off, no external forces, the partial derivative `∂FD/∂q̇` is identically zero. Finite differences leak ~10⁻⁶ to 10⁻⁷ noise from the rounding of the small input increment. Analytical derivatives produce **exact zero**. Matters for any solver where Hessian conditioning depends on small derivative components being clean — MPC inner-loop QP solvers, LQR Riccati iterations, EKF covariance updates with a dynamics linearization.

## A new M⁻¹ algorithm

§IV.C. The standard way to compute `M(q)⁻¹` is [[concepts/composite-rigid-body-algorithm|CRBA]] to form `M`, then sparse Cholesky `M = LDLᵀ`, then `M⁻¹ = L⁻ᵀ D⁻¹ L⁻¹`. The authors propose a 3-pass algorithm that computes `M⁻¹` directly *without* ever materializing `M`. Exploits kinematic-tree sparsity, exploits M's symmetry (compute upper or lower triangle only).

Reported as ~2× faster than CRBA+Cholesky for high-DOF robots; competitive at low DOF (Fig. 2c). Detail is in companion report **hal-01790934**, *"Analytical inverse of the joint space inertia matrix"* (Carpentier 2018) — not in Apsis's raw corpus. Worth ingesting later if `M⁻¹` becomes a measured bottleneck for the MPC inner loop.

## Apsis relevance

- **REQ-GNC-008** (MPC attitude/orbit controller, S priority) — this paper is the enabling derivation. Without it, whole-body MPC for any Apsis spacecraft beyond ~7 DOF would be too slow for closed-loop sim. With it, a humanoid-class 36-DOF MPC step is ~17 µs of derivatives + the QP solve, which is well within a 100 Hz controller tick budget.
- **REQ-MBD-007** (Jacobian and mass matrix queries for LQR/MPC) — the M⁻¹ algorithm is part of this requirement's coverage.
- **subsystems §5.4** ("MPC for slew planning") — the derivative budget calculation that makes this realistic.
- **architecture §3 Foundation > Spacecraft internals (build-vs-reuse table)** — credits Pinocchio for "Featherstone algorithms, autodiff support". This paper shows analytical derivatives are 30-60% faster than autodiff + codegen *and* numerically exact. The canonical MPC derivative path for Apsis should be **analytical**, not autodiff. Flagged below.

## Surfaced for human review (no silent spec edits)

The architecture's build-vs-reuse table at `docs/01-architecture.md` §5 credits Pinocchio for *"Featherstone algorithms, autodiff support"*. Not wrong — Pinocchio does support autodiff via templated scalar type. But undersells what Pinocchio actually delivers: **analytical RBD derivatives, faster than any autodiff approach and numerically exact.** Worth one sentence in the architecture text noting that Apsis's MPC and LQR controllers (REQ-GNC-008, REQ-MBD-007) should use the analytical-derivative path as canonical, with autodiff as fallback for user-defined dynamics extensions.
