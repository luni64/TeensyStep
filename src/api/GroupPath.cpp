#include "GroupPath.h"

namespace TeensyStep
{
    namespace advanced
    {
        errorCode GroupPath::moveAbsAsync(const int32_t* targets, unsigned n, float speedAtTarget, float speedAtStart)
        {
            return g.start(targets, n, speedAtTarget, speedAtStart);
        }

        errorCode GroupPath::moveAbsAsync(std::initializer_list<int32_t> targets, float speedAtTarget, float speedAtStart)
        {
            return g.start(targets.begin(), (unsigned)targets.size(), speedAtTarget, speedAtStart);
        }

        errorCode GroupPath::moveRelAsync(const int32_t* distances, unsigned n, float speedAtTarget, float speedAtStart)
        {
            return g.startRel(distances, n, speedAtTarget, speedAtStart);
        }

        errorCode GroupPath::moveRelAsync(std::initializer_list<int32_t> distances, float speedAtTarget, float speedAtStart)
        {
            return g.startRel(distances.begin(), (unsigned)distances.size(), speedAtTarget, speedAtStart);
        }

        errorCode GroupPath::setSpeedAtTarget(float speedAtTarget)
        {
            return g.changeSpeedAtTarget(speedAtTarget);
        }
    }
}
