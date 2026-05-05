---
type: concept
canonical_name: Generalized Rodrigues Parameters
aliases: [Gibbs vector, Rodrigues parameters, Modified Rodrigues Parameters, MRP, GRP]
---

# Generalized Rodrigues Parameters (GRP / MRP)

A family of **three-component attitude representations** derived as projections of the quaternion onto R³. The two standard members are the **Gibbs vector** `g = e tan(φ/2)` and the **Modified Rodrigues Parameters (MRP)** `p = e tan(φ/4)`. Both are non-singular except at one rotation angle (Gibbs at π, MRP at 2π) and trade global-uniqueness for the ability to be carried through linear math (covariance, control gain matrices) directly as 3-vectors.

## Definitions

For a unit quaternion `q = [q_v; q₄] = [e sin(φ/2); cos(φ/2)]`:

```
Gibbs vector:    g = q_v / q₄         = e tan(φ/2)        (singular at φ = π)
Modified RP:     p = q_v / (1 + q₄)   = e tan(φ/4)        (singular at φ = 2π)
Generalized RP:  g(a,f) = f · q_v / (a + q₄)              (a ∈ [0,1], f scalar)
```

The "generalized" form ([[sources/crassidis-2003-ukf-attitude|Crassidis & Markley 2003]] Eq. 8) interpolates: `(a, f) = (0, 1)` gives Gibbs; `(a, f) = (1, 1)` gives MRP; other choices appear in some references.

## Why three-component representations matter

Unit quaternions have four components but only three rotational DOF — the unit-norm constraint must hold. **Linear algebra (Kalman filter updates, control gain matrices) doesn't natively respect the constraint**. Two responses:

1. **MEKF** ([[concepts/mekf]]): keep the quaternion as the global state but use a 3-vector for covariance. The 3-vector is local-to-the-estimate (error representation).
2. **Direct 3-component state**: use Gibbs or MRP as the state itself. Possible for control problems where the trajectory stays bounded away from the singularity.

Either way, GRP/MRP is the math that makes the 3-vector well-defined.

## Modified Rodrigues Parameters: the practical default

MRP is the modern default for attitude estimation/control three-vector states because:

- **Larger non-singular range** than Gibbs (φ < 2π vs φ < π).
- **Smooth and bounded** for typical attitude-error magnitudes.
- **MRP shadow set switching**: when `|p| > 1` (i.e., φ > π), switch to the "shadow" MRP `p_s = -p / |p|²` which represents the same rotation in the other half of S³. This trick gives MRP **global non-singular coverage** with a single discontinuous switch (analogous to quaternion sign flip).
- **Linear in covariance to first order**: the 3-vector `p` for small errors is exactly twice the small-angle vector part of the error quaternion → the covariance map between quaternion and MRP is identity to first order.

## Apsis usage

- **MEKF covariance state** (REQ-GNC-003): use MRP for the 3-vector error representation. See [[sources/markley-2003-attitude-error-representations]] for the consistent second-order MEKF derivation.
- **USQUE sigma-point representation** (REQ-GNC-003 fallback): see [[sources/crassidis-2003-ukf-attitude]] §III for the GRP-parameterized sigma-point construction.
- **Lyapunov attitude controllers**: [[sources/schaub-1998-vscmg]] uses MRP-based Lyapunov functions for attitude regulation. Same representation flows naturally from estimator to controller.

## See also

- [[concepts/quaternion-attitude-representation]] — the underlying global state.
- [[concepts/mekf]] / [[concepts/unscented-kalman-filter]] — the filters that consume GRP/MRP as covariance state.
- [[sources/markley-2003-attitude-error-representations]] — the canonical taxonomy.
- [[sources/crassidis-2003-ukf-attitude]] — USQUE uses GRP for sigma-point parameterization.
