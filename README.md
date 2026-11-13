![CI](https://github.com/Homeew/magnetic-dipole-simulator/actions/workflows/ci.yml/badge.svg)
[![Documentation](https://img.shields.io/badge/docs-doxygen-blue.svg)](https://homeew.github.io/magnetic-dipole-simulator/)
# Magnetic Dipole Field Simulator

A standalone C++ simulation of magnetic dipole fields with configurable
sensor array modeling and nonlinear least-squares parameter estimation.

## What it does

1. Models the magnetic field of a dipole source in 3D (exact analytical formula)
2. Places a configurable array of sensors around the source (ring, grid, or sphere)
3. Corrupts measurements with additive Gaussian noise (realistic magnetometer model)
4. Recovers the dipole position and moment using Levenberg-Marquardt optimisation

## Results

| Array type                | Sensors | Position error | Moment error  |
|---------------------------|---------|----------------|---------------|
| Ring (r=0.08m, z=0.10m)   | 16      | 0.0007 mm      | 0.00003 A.m2  |
| Grid (0.5x0.5m, z=0.30m)  | 64      | 0.46 mm        | 0.011 A.m2    |

Noise level: 1 nT (SNR ~ 111 dB)

## Visualisations

## Dashboard

![Dashboard](plots/plot_dashboard.png)

## LM Solver Convergence Animation

![LM convergence](plots/lm_convergence.gif)

### Sensor Array Geometry
![Sensor geometry](plots/plot1_sensor_geometry.png)

### B-Field Vectors at Each Sensor
![Field vectors](plots/plot2_field_vectors.png)

### LM Solver Cost Convergence
![Cost curve](plots/plot3_cost_curve.png)

### True vs Estimated Dipole
![Comparison](plots/plot4_comparison.png)

## Physics

The magnetic field of a dipole at position p with moment m, observed at r:

    B(r) = (mu0/4pi) x [3(m.rhat)rhat - m] / |r|^3

where r = observation - p and rhat = r/|r|.

## Project structure

    include/
      vec3.hpp          - 3D vector math
      dipole.hpp        - dipole field model
      noise.hpp         - Gaussian noise generator
      sensor.hpp        - single 3-axis magnetometer
      sensor_array.hpp  - sensor collection + ring/grid/sphere factories
      lm_solver.hpp     - Levenberg-Marquardt solver
    src/
      sensor_array.cpp  - geometry implementations
      lm_solver.cpp     - full LM optimisation loop
      main.cpp          - demo: simulate, solve, print results
    tests/
      test_dipole.cpp   - 129 unit test assertions (Catch2)
    plots/
      plot1_sensor_geometry.png
      plot2_field_vectors.png
      plot3_cost_curve.png
      plot4_comparison.png

## Build

    mkdir build && cd build
    cmake ..
    make -j4

Requires: g++ (C++17), cmake >= 3.14. Catch2 is downloaded automatically.

## Run

    ./dipole_sim         # full demo
    ./dipole_sim --csv   # CSV output for Python plotting
    ./tests              # unit tests

## Class design

| Class          | Responsibility                                   |
|----------------|--------------------------------------------------|
| Vec3           | 3D vector arithmetic                             |
| DipoleSource   | Field model + parameter serialisation            |
| Sensor         | Single 3-axis measurement                        |
| SensorArray    | Collection of sensors + geometry factories       |
| GaussianNoise  | Reproducible noise injection                     |
| LMSolver       | Nonlinear least-squares via Levenberg-Marquardt  |
| LMResult       | Solver output (estimate, cost, convergence info) |
