#include "GroupPath.h"
#include "Channels.h"

namespace TeensyStep
{
    namespace advanced
    {
        using detail::Channel;
        using detail::Engine;
        using detail::Lock;

        GroupPath::~GroupPath()
        {
            Lock lock; // moves that were started go on; they do not call this object any more
            for (Channel& c : Engine::channels)
                if (c.pathOwner == this) c.pathOwner = nullptr;
        }

        errorCode GroupPath::add(const int32_t* targets, unsigned n, const PathMove& move)
        {
            if (n != g.count || g.count == 0) return Engine::report(errorCode::invalidArgument);
            errorCode code = errorCode::OK;

            // The usual case, a move that shall wait behind the one on its way: it is prepared without the
            // lock, so that the steps of the move on its way are made in time meanwhile, and handed over
            // with it.
            int running = -1;
            {
                Lock lock;
                running = g.runningChannel();
                if (running >= 0 && Engine::channels[running].ending) running = -1;
                if (running >= 0 && Engine::channels[running].group.overrideFactor() != factor) Engine::channels[running].group.setOverride(factor);
            }
            if (running >= 0)
            {
                core::Group::Line line;
                if (Engine::channels[running].group.prepareWaiting(line, targets, move, code))
                {
                    if (isError(code)) return Engine::report(code);
                    bool waits = false;
                    {
                        Lock lock;
                        Channel& c = Engine::channels[running];
                        if (g.runningChannel() == running && !c.ending && c.group.commitWaiting(line))
                        {
                            c.arrival   = g.arrival;
                            c.pathOwner = this;
                            for (unsigned i = 0; i < g.count; i++) g.motors[i]->setTargetAbs(targets[i]);
                            waits = true;
                        }
                    }
                    if (waits) return Engine::report(code);
                } // otherwise the move on its way has arrived meanwhile, or none was: as below
            }
            {
                Lock lock;
                int ch;
                code = g.takeChannel(ch);
                if (code == errorCode::OK)
                {
                    Channel& c = Engine::channels[ch];
                    if (c.group.overrideFactor() != factor) c.group.setOverride(factor);
                    code = c.group.addMove(targets, move);
                    if (!isError(code))
                    {
                        c.ending    = false;
                        c.arrival   = g.arrival;
                        c.pathOwner = this;
                        backend::start((unsigned)ch);
                        for (unsigned i = 0; i < g.count; i++) g.motors[i]->setTargetAbs(targets[i]); // the members remember the targets
                    }
                    else if (!c.group.isMoving())
                    {
                        Engine::endNow((unsigned)ch); // a channel that was taken for this move only
                    }
                }
            }
            return Engine::report(code);
        }

        errorCode GroupPath::add(std::initializer_list<int32_t> targets, const PathMove& move)
        {
            return add(targets.begin(), (unsigned)targets.size(), move);
        }

        unsigned GroupPath::room() const
        {
            Lock lock;
            int ch = g.runningChannel();
            if (ch < 0 || Engine::channels[ch].ending) return 2;
            return Engine::channels[ch].group.room();
        }

        void GroupPath::onRoom(void (*function)(GroupPath&, void*), void* context)
        {
            Lock lock;
            roomFunction = function;
            roomContext  = context;
        }

        errorCode GroupPath::overrideSpeed(float f)
        {
            if (!(f > 0 && f <= 1)) return Engine::report(errorCode::invalidArgument);
            Lock lock;
            factor = f;
            int ch = g.runningChannel();
            if (ch >= 0 && !Engine::channels[ch].ending) Engine::channels[ch].group.setOverride(f);
            return errorCode::OK;
        }

        float GroupPath::getSpeed() const
        {
            Lock lock;
            int ch = g.runningChannel();
            if (ch < 0 || Engine::channels[ch].ending) return 0.0f;
            return Engine::channels[ch].group.pathSpeed();
        }

        void GroupPath::moveArrived()
        {
            done = done + 1;
            if (roomFunction) roomFunction(*this, roomContext);
        }
    }
}
