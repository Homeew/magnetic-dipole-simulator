#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "vec3.hpp"
#include "dipole.hpp"
#include "sensor.hpp"
#include "sensor_array.hpp"
#include "noise.hpp"
#include "lm_solver.hpp"

namespace py = pybind11;

PYBIND11_MODULE(dipole_py, m) {
    m.doc() = "Magnetic dipole field simulator — C++ core";

    // ── Vec3 ─────────────────────────────────────────────────────────────────
    py::class_<Vec3>(m, "Vec3")
        .def(py::init<double, double, double>(), py::arg("x")=0, py::arg("y")=0, py::arg("z")=0)
        .def_readwrite("x", &Vec3::x)
        .def_readwrite("y", &Vec3::y)
        .def_readwrite("z", &Vec3::z)
        .def("norm",   &Vec3::norm)
        .def("dot",    &Vec3::dot)
        .def("__add__", [](const Vec3& a, const Vec3& b){ return a + b; })
        .def("__sub__", [](const Vec3& a, const Vec3& b){ return a - b; })
        .def("__mul__", [](const Vec3& v, double s){ return v * s; })
        .def("__repr__", [](const Vec3& v){
            return "Vec3(" + std::to_string(v.x) + ", "
                           + std::to_string(v.y) + ", "
                           + std::to_string(v.z) + ")";
        });

    // ── DipoleSource ─────────────────────────────────────────────────────────
    py::class_<DipoleSource>(m, "DipoleSource")
        .def(py::init<Vec3, Vec3>(), py::arg("position"), py::arg("moment"))
        .def_readwrite("position", &DipoleSource::position)
        .def_readwrite("moment",   &DipoleSource::moment)
        .def("fieldAt", &DipoleSource::fieldAt, py::arg("obs"))
        .def("__repr__", [](const DipoleSource& d){
            return "DipoleSource(pos=(" + std::to_string(d.position.x) + ","
                   + std::to_string(d.position.y) + "," + std::to_string(d.position.z)
                   + "), mom=(" + std::to_string(d.moment.x) + ","
                   + std::to_string(d.moment.y) + "," + std::to_string(d.moment.z) + "))";
        });

    // ── Sensor ───────────────────────────────────────────────────────────────
    py::class_<Sensor>(m, "Sensor")
        .def_readonly("position", &Sensor::position)
        .def_readonly("label",    &Sensor::label)
        .def("measure", &Sensor::measure);

    // ── SensorArray ──────────────────────────────────────────────────────────
    py::class_<SensorArray>(m, "SensorArray")
        .def_readonly("sensors",      &SensorArray::sensors)
        .def("size",                  &SensorArray::size)
        .def("measure",               &SensorArray::measure)
        .def("measureNoisy",          &SensorArray::measureNoisy)
        .def_static("ring",   &SensorArray::ring,
                    py::arg("n"), py::arg("radius"), py::arg("z"),
                    py::arg("cx")=0.0, py::arg("cy")=0.0)
        .def_static("grid",   &SensorArray::grid,
                    py::arg("rows"), py::arg("cols"),
                    py::arg("halfW"), py::arg("halfH"), py::arg("z"))
        .def_static("sphere", &SensorArray::sphere,
                    py::arg("n"), py::arg("radius"));

    // ── GaussianNoise ────────────────────────────────────────────────────────
    py::class_<GaussianNoise>(m, "GaussianNoise")
        .def(py::init<double, uint64_t>(), py::arg("sigma"), py::arg("seed")=42)
        .def_readwrite("sigma", &GaussianNoise::sigma)
        .def("corrupt", &GaussianNoise::corrupt)
        .def("reseed",  &GaussianNoise::reseed);

    // ── LMSolver::Options ────────────────────────────────────────────────────
    py::class_<LMSolver::Options>(m, "LMOptions")
        .def(py::init<>())
        .def_readwrite("lambda0",      &LMSolver::Options::lambda0)
        .def_readwrite("lambdaFactor", &LMSolver::Options::lambdaFactor)
        .def_readwrite("ftol",         &LMSolver::Options::ftol)
        .def_readwrite("xtol",         &LMSolver::Options::xtol)
        .def_readwrite("gtol",         &LMSolver::Options::gtol)
        .def_readwrite("maxIter",      &LMSolver::Options::maxIter)
        .def_readwrite("verbose",      &LMSolver::Options::verbose);

    // ── LMResult ─────────────────────────────────────────────────────────────
    py::class_<LMResult>(m, "LMResult")
        .def_readonly("estimated",  &LMResult::estimated)
        .def_readonly("finalCost",  &LMResult::finalCost)
        .def_readonly("iterations", &LMResult::iterations)
        .def_readonly("converged",  &LMResult::converged)
        .def_readonly("message",    &LMResult::message);

    // ── LMSolver ─────────────────────────────────────────────────────────────
    py::class_<LMSolver>(m, "LMSolver")
        .def(py::init<>())
        .def(py::init<LMSolver::Options>(), py::arg("opts"))
        .def("solve", &LMSolver::solve,
             py::arg("array"), py::arg("measurements"), py::arg("init"));
}
