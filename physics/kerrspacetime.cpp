#include "kerrspacetime.h"

#include <algorithm>
#include <stdexcept>

namespace physics {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kMinSigma = 1.0e-12;
constexpr double kMinDelta = 1.0e-12;
constexpr double kDerivativeStep = 1.0e-5;

double clampTheta(double theta)
{
    return std::clamp(theta, 1.0e-5, kPi - 1.0e-5);
}

double safeSinSquared(double theta)
{
    const double s = std::sin(clampTheta(theta));
    return std::max(s * s, 1.0e-10);
}

} // namespace

KerrSpacetime::KerrSpacetime(double mass, double spin)
    : m_mass(mass)
    , m_spin(spin)
{
    if (!(mass > 0.0)) {
        throw std::invalid_argument("Kerr mass must be positive");
    }

    if (std::abs(spin) > mass) {
        throw std::invalid_argument("Kerr spin must satisfy |a| <= M");
    }
}

double KerrSpacetime::horizonRadius() const
{
    return m_mass + std::sqrt(std::max(0.0, m_mass * m_mass - m_spin * m_spin));
}

ContravariantMetric KerrSpacetime::contravariantMetric(double r, double theta) const
{
    const double clampedTheta = clampTheta(theta);
    const double cosTheta = std::cos(clampedTheta);
    const double sin2Theta = safeSinSquared(clampedTheta);
    const double sigma = std::max(r * r + m_spin * m_spin * cosTheta * cosTheta, kMinSigma);
    const double delta = std::max(r * r - 2.0 * m_mass * r + m_spin * m_spin, kMinDelta);
    const double a2 = m_spin * m_spin;
    const double radialTerm = r * r + a2;
    const double bigA = radialTerm * radialTerm - a2 * delta * sin2Theta;

    ContravariantMetric metric;
    metric.gtt = -bigA / (sigma * delta);
    metric.gtphi = -(2.0 * m_mass * m_spin * r) / (sigma * delta);
    metric.grr = delta / sigma;
    metric.gthetatheta = 1.0 / sigma;
    metric.gphiphi = (delta - a2 * sin2Theta) / (sigma * delta * sin2Theta);
    return metric;
}

double KerrSpacetime::hamiltonian(const BoyerLindquistState& state) const
{
    const ContravariantMetric metric = contravariantMetric(state.r, state.theta);
    return 0.5 * (
        metric.gtt * state.pt * state.pt +
        2.0 * metric.gtphi * state.pt * state.pphi +
        metric.grr * state.pr * state.pr +
        metric.gthetatheta * state.ptheta * state.ptheta +
        metric.gphiphi * state.pphi * state.pphi);
}

std::array<double, 2> KerrSpacetime::metricGradient(const BoyerLindquistState& state) const
{
    const double rStep = std::max(kDerivativeStep, std::abs(state.r) * kDerivativeStep);
    const double thetaStep = kDerivativeStep;

    const ContravariantMetric metricRp = contravariantMetric(state.r + rStep, state.theta);
    const ContravariantMetric metricRm = contravariantMetric(std::max(horizonRadius() * 1.0001, state.r - rStep), state.theta);
    const ContravariantMetric metricTp = contravariantMetric(state.r, state.theta + thetaStep);
    const ContravariantMetric metricTm = contravariantMetric(state.r, state.theta - thetaStep);

    const auto contract = [&](const ContravariantMetric& g) {
        return g.gtt * state.pt * state.pt +
               2.0 * g.gtphi * state.pt * state.pphi +
               g.grr * state.pr * state.pr +
               g.gthetatheta * state.ptheta * state.ptheta +
               g.gphiphi * state.pphi * state.pphi;
    };

    const double dHdr = 0.25 * (contract(metricRp) - contract(metricRm)) / rStep;
    const double dHdTheta = 0.25 * (contract(metricTp) - contract(metricTm)) / thetaStep;
    return {dHdr, dHdTheta};
}

BoyerLindquistState KerrSpacetime::geodesicDerivative(const BoyerLindquistState& state) const
{
    const ContravariantMetric metric = contravariantMetric(state.r, state.theta);
    const std::array<double, 2> gradient = metricGradient(state);

    BoyerLindquistState derivative;
    derivative.t = metric.gtt * state.pt + metric.gtphi * state.pphi;
    derivative.r = metric.grr * state.pr;
    derivative.theta = metric.gthetatheta * state.ptheta;
    derivative.phi = metric.gtphi * state.pt + metric.gphiphi * state.pphi;

    derivative.pt = 0.0;
    derivative.pr = -gradient[0];
    derivative.ptheta = -gradient[1];
    derivative.pphi = 0.0;
    return derivative;
}

} // namespace physics
