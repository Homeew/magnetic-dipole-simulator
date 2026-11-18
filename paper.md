---
title: 'Magnetic Dipole Field Simulator: A C++ toolkit for dipole localisation with Python bindings'
tags:
  - C++
  - Python
  - magnetic dipole
  - Levenberg-Marquardt
  - localisation
  - capsule endoscopy
  - nonlinear least squares
authors:
  - name: Omid Yaghoobian
    orcid: 0009-0005-8786-2416
    affiliation: 1
affiliations:
  - name: Department of Electrical and Computer Engineering, University of Saskatchewan, Canada
    index: 1
date: 18 November 2026
bibliography: paper.bib
---

# Summary

Magnetic dipole field simulators are essential tools for developing and validating
localisation algorithms in medical robotics, particularly for wireless capsule
endoscopy (WCE), where a magnetically tagged capsule must be tracked through the
gastrointestinal tract in real time. This package provides a standalone C++
implementation of the full dipole localisation pipeline: analytical field
computation, configurable sensor array modeling, additive Gaussian noise
simulation, and nonlinear parameter estimation via the Levenberg-Marquardt
(LM) algorithm [@levenberg1944; @marquardt1963]. Python bindings via pybind11
[@pybind11] expose the entire C++ API for interactive use and integration with
data-science workflows.

# Statement of Need

Reproducible, well-tested implementations of magnetic dipole localisation
pipelines are largely absent from the open-source ecosystem. Researchers
typically re-implement the same forward model and optimiser from scratch for
each study, making it difficult to compare results across papers or to isolate
the effect of sensor geometry, noise level, or solver configuration.

This simulator addresses that gap. It directly implements the core estimation
pipeline underlying a series of published studies on magnetic localisation for
WCE [@yaghoobian2025iccit; @yaghoobian2026sensors; @yaghoobian2026access;
@yaghoobian2026scirep], providing a clean, citable reference implementation
that other researchers can build on, benchmark against, or incorporate into
their own pipelines.

Key design goals:

- **Exactness**: the forward model uses the closed-form dipole field formula
  with no approximations.
- **Speed**: the LM solver uses an analytical Jacobian derived from the dipole
  field formula, converging in ~8 iterations to machine-precision cost
  (~$2 \times 10^{-17}$), compared to hundreds of iterations with
  finite-difference approximations.
- **Reproducibility**: a seeded Gaussian noise model and deterministic solver
  ensure fully reproducible results.
- **Interoperability**: pybind11 bindings allow the C++ solver to be called
  natively from Python with no subprocess overhead.

# Mathematics

The magnetic field produced by a dipole with moment $\mathbf{m}$ at position
$\mathbf{p}$, observed at $\mathbf{r}$, is:

$$\mathbf{B}(\mathbf{r}) = \frac{\mu_0}{4\pi}
  \frac{3(\mathbf{m} \cdot \hat{\mathbf{d}})\hat{\mathbf{d}} - \mathbf{m}}{|\mathbf{d}|^3}$$

where $\mathbf{d} = \mathbf{r} - \mathbf{p}$ and $\hat{\mathbf{d}} = \mathbf{d}/|\mathbf{d}|$.

The LM solver minimises the sum of squared residuals between predicted and
measured field components across all sensors:

$$F(\mathbf{x}) = \frac{1}{2} \sum_{i=1}^{3N} r_i(\mathbf{x})^2$$

where $\mathbf{x} = [p_x, p_y, p_z, m_x, m_y, m_z]^\top$ and $N$ is the number
of sensors. Each LM iteration solves the damped normal equations:

$$(\mathbf{J}^\top \mathbf{J} + \lambda \mathbf{I})\,\Delta\mathbf{x} = -\mathbf{J}^\top \mathbf{r}$$

The Jacobian $\mathbf{J}$ is computed analytically, giving exact derivatives of
all six field components with respect to all six parameters.

# Software Description

The package is structured as a static C++17 library (`dipole_lib`) with the
following components:

| Class | Responsibility |
|-------|---------------|
| `Vec3` | 3D vector arithmetic |
| `DipoleSource` | Forward field model and parameter serialisation |
| `SensorArray` | Sensor collection with ring, grid, and sphere factories |
| `GaussianNoise` | Reproducible additive white Gaussian noise |
| `LMSolver` | Levenberg-Marquardt nonlinear least-squares optimiser |

The build system uses CMake with FetchContent for dependency management
(Catch2 for testing, pybind11 for Python bindings). A GitHub Actions workflow
runs the full test suite on every push.

# Validation

A Monte Carlo study with 300 independent noise realisations per noise level
confirms the solver's robustness across six decades of noise magnitude
($10^{-11}$ T to $10^{-6}$ T). The solver achieves 100% convergence across
all tested conditions. At a realistic magnetometer noise floor of 1 nT, the
median position recovery error is 0.0005 mm with a 95th-percentile error of
0.0008 mm, consistent with the sub-millimetre accuracy reported in the
associated publications [@yaghoobian2026scirep].

# Acknowledgements

The author thanks Khan A. Wahid (University of Saskatchewan) for supervision
and guidance throughout the research programme that motivated this software.

# References
