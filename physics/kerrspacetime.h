#pragma once

#include <array>
#include <cmath>
#include <limits>

namespace physics {

struct BoyerLindquistState {
    double t = 0.0;
    double r = 10.0;
    double theta = 1.5707963267948966;
    double phi = 0.0;

    double pt = -1.0;
    double pr = 0.0;
    double ptheta = 0.0;
    double pphi = 0.0;
};

struct ContravariantMetric {
    double gtt = 0.0;
    double gtphi = 0.0;
    double grr = 0.0;
    double gthetatheta = 0.0;
    double gphiphi = 0.0;
};

class KerrSpacetime
{
public:
    KerrSpacetime(double mass, double spin);

    double mass() const { return m_mass; }
    double spin() const { return m_spin; }
    double horizonRadius() const;

    ContravariantMetric contravariantMetric(double r, double theta) const;
    double hamiltonian(const BoyerLindquistState& state) const;

    BoyerLindquistState geodesicDerivative(const BoyerLindquistState& state) const;

private:
    std::array<double, 2> metricGradient(const BoyerLindquistState& state) const;

    double m_mass = 1.0;
    double m_spin = 0.0;
};

} // namespace physics
