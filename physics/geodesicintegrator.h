#pragma once

#include "kerrspacetime.h"

#include <vector>

namespace physics {

struct GeodesicSample {
    BoyerLindquistState state;
    double hamiltonian = 0.0;
};

class GeodesicIntegrator
{
public:
    explicit GeodesicIntegrator(KerrSpacetime spacetime);

    const KerrSpacetime& spacetime() const { return m_spacetime; }

    BoyerLindquistState stepRK4(const BoyerLindquistState& state, double step) const;
    std::vector<GeodesicSample> integrate(
        const BoyerLindquistState& initialState,
        int steps,
        double stepSize) const;

private:
    KerrSpacetime m_spacetime;
};

} // namespace physics
