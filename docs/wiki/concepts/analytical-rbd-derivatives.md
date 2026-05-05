---
type: concept
canonical_name: "Analytical RBD Derivatives"
aliases: [analytical RBD derivatives, RBD analytical derivatives, analytical dynamics gradients]
created: 2026-05-04
updated: 2026-05-04
---

# Analytical RBD Derivatives

Closed-form partial derivatives of rigid-body dynamics algorithms — [[concepts/recursive-newton-euler-algorithm|RNEA]] (inverse) and [[concepts/articulated-body-algorithm|ABA]] (forward) — with respect to the dynamics input variables (`q`, `q̇`, `τ`, `f^ext`). The derivation in [[sources/carpentier-2018-rbd-analytical-derivatives]] makes these analytical computations as cheap as 3-5× the dynamics call itself, while preserving the kinematic-tree sparsity that finite differences and naïve Lagrangian derivations destroy.

For Apsis, analytical derivatives are the **canonical derivative path** for the MPC controller (REQ-GNC-008) and the LQR linearization (REQ-MBD-007). They are not an optimization detail — they are what makes whole-body model-based control on a multi-body spacecraft realistic at closed-loop simulator speeds.

## Why analytical (not finite-difference, not autodiff)

Three reasons, in order of importance:

**1. Speed scales with kinematic-tree depth, not N+1 calls.** Finite differences cost N+1 dynamics evaluations (one nominal + N perturbed). Analytical derivatives propagate the partial-derivative matrices alongside the dynamics quantities in a single tree traversal, so the total cost is fixed-multiple of one dynamics call. From [[sources/carpentier-2018-rbd-analytical-derivatives]] Table I: 6× speedup on KUKA (7 DOF), 13× on HyQ (18 DOF), **27× on Atlas (36 DOF)**. Speedup grows with DOF — exactly the regime Apsis cares about.

**2. Numerically exact.** Finite differences leak ~10⁻⁶ to 10⁻⁷ noise from the rounding of the perturbation increment ([[sources/carpentier-2018-rbd-analytical-derivatives]] Fig. 3). Partials that should be identically zero are noisy. Matters for any solver where Hessian conditioning depends on small derivative components being clean — MPC inner-loop QPs, LQR Riccati iterations, EKF covariance updates that linearize the dynamics.

**3. Faster than autodiff even with code generation.** Per [[sources/carpentier-2018-rbd-analytical-derivatives]] §V.A.2, analytical derivatives are 30% faster than the Giftthaler et al. 2017 autodiff+codegen approach on inverse dynamics, 60% faster on forward dynamics. The architecture's build-vs-reuse table mentions Pinocchio's autodiff support; for Apsis MPC the analytical path is preferable.

## The chain-rule structure: only RNEA-derivatives need to be derived

The load-bearing identity ([[sources/carpentier-2018-rbd-analytical-derivatives]] Eq. 24, §IV.B): forward and inverse dynamics are inverses of each other (`FD ∘ ID = id`). Differentiating the composition gives, for `u ∈ {q, q̇}`:

```
∂FD/∂u = -M(q)⁻¹ · ∂ID/∂u
```

So **ABA-derivatives are just RNEA-derivatives multiplied by `-M⁻¹`.** There is no separate ABA-derivative algorithm to derive, implement, or test. The other side ([[sources/carpentier-2018-rbd-analytical-derivatives]] Eq. 17): `∂FD/∂τ = M⁻¹` directly, from linearity in `τ`.

The bridge between RNEA-derivatives and ABA-derivatives is a single matrix multiplication. The RNEA-derivative algorithms are 6 + 4 lines added to the standard RNEA recursion ([[sources/carpentier-2018-rbd-analytical-derivatives]] Algorithms 2, 3). Compact, sparse, and bounded by the same complexity as RNEA itself.

## The spatial-algebra trick

The derivation hinges on two relations that avoid materializing dense 6×6×N derivative tensors of the placement matrices `X` ([[sources/carpentier-2018-rbd-analytical-derivatives]] §III.C.3, Eqs 13-14):

```
(∂ⁱX_λ(i)/∂q_i) · v_λ(i) = (ⁱX_λ(i) · v_λ(i)) × S_i      [motion-side action, Eq. 13]
(∂λ(i)X_iᵀ / ∂u) · f_i  = λ(i)X_iᵀ · (S_i ×* f_i)        [force-side action, Eq. 14]
```

Both reformulate the derivative-of-transformation as the **action of a known operator on a known vector**. No tensor materialization. This is what keeps the analytical algorithm at O(n) — without these tricks, the explicit `∂X/∂q` would force an O(n³) tensor product per joint per step.

The relations rest on [[concepts/spatial-algebra]] — Lie-group structure of SE(3) and the cross-product / dual-cross-product operators on motion and force vectors.

## Cost ballpark for Apsis

A typical Apsis spacecraft is wide-and-shallow: a free-floating root + a handful of parallel appendages (reaction wheels, gimbals, solar arrays, antennas). Maybe 10-15 DOF total, kinematic-tree depth 2-3. Interpolating between [[sources/carpentier-2018-rbd-analytical-derivatives]] Table I numbers (KUKA 7-DOF and HyQ 18-DOF):

- Inverse dynamics call: ~1.5 µs
- ID + analytical derivatives: ~5 µs
- Forward dynamics call: ~3 µs
- FD + analytical derivatives: ~10 µs

For an MPC controller running at 50 Hz with a 20-step horizon and ~5 inner Newton iterations per QP solve, derivative cost is `5 × 20 × 5 × 10 µs = 5 ms` per controller tick — fits comfortably in the 20 ms tick budget. Without analytical derivatives, finite-difference equivalents would be ~10× more — outside budget.

## Inverse joint-space inertia matrix M⁻¹

The MPC tooling needs `M⁻¹` for the `∂FD/∂u = -M⁻¹ · ∂ID/∂u` multiplication. Two ways:

- **Standard**: [[concepts/composite-rigid-body-algorithm|CRBA]] computes `M`, sparse Cholesky `M = LDLᵀ`, then `M⁻¹ = L⁻ᵀ D⁻¹ L⁻¹`. Pinocchio implements both steps.
- **Direct M⁻¹**: a 3-pass algorithm proposed in [[sources/carpentier-2018-rbd-analytical-derivatives]] §IV.C and detailed in companion report hal-01790934 (not in Apsis's corpus) that computes `M⁻¹` *without* materializing `M`. ~2× faster than CRBA+Cholesky on high-DOF robots; competitive at low DOF.

Apsis should use Pinocchio's bundled implementation for both — when the time comes to compare, swap based on measured timings.

## What's NOT analytical

A few derivative computations are not in the scope of [[sources/carpentier-2018-rbd-analytical-derivatives]] (or [[concepts/pinocchio-library|Pinocchio]]'s native support):

- **Derivatives w.r.t. model parameters** (link masses, lengths, inertias). The paper notes (Conclusion) the same approach extends to these but the algorithms are not given. Useful for co-design and for system-identification fits to telemetry. Defer.
- **Derivatives of contact-constrained dynamics**. Apsis spacecraft don't have contacts in nominal flight; relevant only if landing/docking is added. Defer.
- **Higher-order derivatives** (Hessians of the dynamics). Some MPC and trust-region solvers use these. Available via autodiff over the analytical first-derivative algorithms; cost is then comparable to autodiff. Out of scope here.

## References

- **[[sources/carpentier-2018-rbd-analytical-derivatives]]** — the canonical derivation paper. RSS 2018.
- **[[sources/carpentier-2019-pinocchio]]** §II.G — Pinocchio's implementation overview, Fig. 4 benchmarks.
- **Carpentier 2018 (companion report, hal-01790934)** — details of the direct M⁻¹ algorithm. Not yet ingested.
- **Giftthaler et al. (2017)** — *Automatic differentiation of rigid body dynamics for optimal control and estimation*, Advanced Robotics — the prior best (autodiff+codegen) approach. Not in our corpus.
