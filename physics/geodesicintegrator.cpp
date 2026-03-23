#include "geodesicintegrator.h"

#include <algorithm>
#include <utility>

namespace physics {
namespace {

BoyerLindquistState addScaled(
    const BoyerLindquistState& state,
    const BoyerLindquistState& derivative,
    double scale)
{
    BoyerLindquistState result = state;
    result.t += derivative.t * scale;
    result.r += derivative.r * scale;
    result.theta += derivative.theta * scale;
    result.phi += derivative.phi * scale;
    result.pt += derivative.pt * scale;
    result.pr += derivative.pr * scale;
    result.ptheta += derivative.ptheta * scale;
    result.pphi += derivative.pphi * scale;
    return result;
}

BoyerLindquistState combine(
    const BoyerLindquistState& base,
    const BoyerLindquistState& k1,
    const BoyerLindquistState& k2,
    const BoyerLindquistState& k3,
    const BoyerLindquistState& k4,
    double step)
{
    BoyerLindquistState result = base;
    result.t += step * (k1.t + 2.0 * k2.t + 2.0 * k3.t + k4.t) / 6.0;
    result.r += step * (k1.r + 2.0 * k2.r + 2.0 * k3.r + k4.r) / 6.0;
    result.theta += step * (k1.theta + 2.0 * k2.theta + 2.0 * k3.theta + k4.theta) / 6.0;
    result.phi += step * (k1.phi + 2.0 * k2.phi + 2.0 * k3.phi + k4.phi) / 6.0;
    result.pt += step * (k1.pt + 2.0 * k2.pt + 2.0 * k3.pt + k4.pt) / 6.0;
    result.pr += step * (k1.pr + 2.0 * k2.pr + 2.0 * k3.pr + k4.pr) / 6.0;
    result.ptheta += step * (k1.ptheta + 2.0 * k2.ptheta + 2.0 * k3.ptheta + k4.ptheta) / 6.0;
    result.pphi += step * (k1.pphi + 2.0 * k2.pphi + 2.0 * k3.pphi + k4.pphi) / 6.0;
    return result;
}

} // namespace

GeodesicIntegrator::GeodesicIntegrator(KerrSpacetime spacetime)
    : m_spacetime(std::move(spacetime))
{
}

BoyerLindquistState GeodesicIntegrator::stepRK4(
    const BoyerLindquistState& state,
    double step) const
{
    const BoyerLindquistState k1 = m_spacetime.geodesicDerivative(state);
    const BoyerLindquistState k2 = m_spacetime.geodesicDerivative(addScaled(state, k1, step * 0.5));
    const BoyerLindquistState k3 = m_spacetime.geodesicDerivative(addScaled(state, k2, step * 0.5));
    const BoyerLindquistState k4 = m_spacetime.geodesicDerivative(addScaled(state, k3, step));

    BoyerLindquistState next = combine(state, k1, k2, k3, k4, step);
    next.r = std::max(next.r, m_spacetime.horizonRadius() * 1.0001);
    return next;
}

std::vector<GeodesicSample> GeodesicIntegrator::integrate(
    const BoyerLindquistState& initialState,
    int steps,
    double stepSize) const
{
    std::vector<GeodesicSample> samples;
    samples.reserve(std::max(steps, 0) + 1);

    BoyerLindquistState current = initialState;
    samples.push_back({current, m_spacetime.hamiltonian(current)});

    for (int i = 0; i < steps; ++i) {
        current = stepRK4(current, stepSize);
        samples.push_back({current, m_spacetime.hamiltonian(current)});
    }

    return samples;
}

} // namespace physics
