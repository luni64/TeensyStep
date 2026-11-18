"""
Mutation check of the library: puts one small error at a time into one of the source files of the core and
the API, runs the PC tests and reports whether they notice it. An error that no test notices ("SURVIVED") shows a gap in the tests.

    python mutate.py            all mutations
    python mutate.py period     only the mutations whose description contains this text

Takes several minutes. The source file is restored afterwards. When the code of a mutation no longer exists
in its file, the script says so; the lists below then have to be adapted.
"""
import re
import subprocess
import sys
import time
from pathlib import Path

TEST = Path(__file__).resolve().parent
CORE = TEST.parent / "src" / "core"

# (description, code in the file, replacement)
AXIS = [
    ("stop: braking point one step late", "int64_t cap  = wPull + timesTwoD(rho - 1);", "int64_t cap  = wPull + timesTwoD(rho);"),
    ("target with speed: braking point one step late", "int64_t cap    = arrive + timesTwoD(rho - 1);", "int64_t cap    = arrive + timesTwoD(rho);"),
    ("feasible: 'fast enough' ignored", "return slowEnough && fastEnough;", "return slowEnough || (fastEnough && false);"),
    ("feasible: 'slow enough' ignored", "return slowEnough && fastEnough;", "return fastEnough || (slowEnough && false);"),
    ("acceleration doubled", "int64_t up = (w < wPull ? wPull : w) + twoA;", "int64_t up = (w < wPull ? wPull : w) + 2 * twoA;"),
    ("no start at the pull-in speed", "int64_t up = (w < wPull ? wPull : w) + twoA;", "int64_t up = w + twoA;"),
    ("run-up one step short", "int64_t n = 1 + (wTe - first + twoA - 1) / twoA;", "int64_t n = (wTe - first + twoA - 1) / twoA;"),
    ("braking with the acceleration", "int64_t wNew = w > twoD ? w - twoD : 0;", "int64_t wNew = w > twoA ? w - twoA : 0;"),
    ("period from the new speed only", "period = 2.0f / (vFrom + vNew);", "period = 1.0f / vNew;"),
    ("partial speed change timed as a whole step", "else if (change >= rate)", "else if (change >= 0)"),
    ("arrival speed not set", "if (rho == 1) wNew = wTe;", ""),
    ("speed not kept after arrival", "mode  = Mode::velocity;\n                    wG    = wT;", "mode  = Mode::idle;\n                    wG    = wT;"),
    ("stop check with double deceleration", "return w <= wPull || w - timesTwoD(rho) <= wPull;", "return w <= wPull || w - timesTwoD(2 * rho) <= wPull;"),
    ("braking onto the target is no arrival", "b.arrived  = isMain;", "b.arrived  = false;"),
    ("no run-up: a short way takes the run-up anyway", "if (!runUp) return stepWithoutRunUp(rho, wTe);", ""),
    ("no run-up: too fast, but not braking", "wNew = w > wPull && w > twoD ? w - twoD : wTe;", "wNew = w;"),
    ("no run-up: too slow, jumps to the speed", "wNew = faster(wMax);\n                if (wNew > wTe) wNew = wTe;", "wNew = wTe;"),
    ("no run-up: short way for a speed not reported", "!feasible(w, rho, wT < wMax ? wT : wMax)) return errorCode::targetSpeedNotReached;", "!feasible(w, rho, wT < wMax ? wT : wMax)) return errorCode::OK;"),
    ("no run-up: short way for a stop not reported", "if (rho < 1 || !canStopIn(rho)) return errorCode::targetSpeedNotReached;", "if (rho < 1 || !canStopIn(rho)) return errorCode::OK;"),
    ("lowered limit applied without ramp", "return slower > limit ? slower : limit;", "return slower > limit ? limit : limit;"),
    ("position difference with the wrong sign", "return (int32_t)((uint32_t)a - (uint32_t)b);", "return (int32_t)((uint32_t)b - (uint32_t)a);"),
    ("pull-in speed ignored when stopping", "if (w <= wPull || 2 * w <= twoD)", "if (2 * w <= twoD)"),
    ("slow axis makes one more step when stopping", "if (w <= wPull || 2 * w <= twoD)", "if (w <= wPull)"),
    ("command during a period ignored until the step", "if (pending && pendVersion != version)", "if (pending && pendVersion != version && false)"),
    ("passed part of the period not counted", "pendDone = p.dir == dirBefore ? doneBefore : 0.0f;", "pendDone = p.dir == dirBefore ? 0.0f * doneBefore : 0.0f;"),
    ("passed part counted after a reversal", "pendDone = p.dir == dirBefore ? doneBefore : 0.0f;", "pendDone = p.dir == dirBefore ? doneBefore : doneBefore;"),
    ("maximum delay not applied", "if (rest <= maxDelay)", "if (rest <= maxDelay || true)"),
    ("coast: one step too far into the deceleration", "int64_t first   = braking + 1 > 2 ? braking + 1 : 2;", "int64_t first   = braking > 2 ? braking : 2;"),
    ("coast: the arriving step handed out without a decision", "int64_t first   = braking + 1 > 2 ? braking + 1 : 2;", "int64_t first   = braking + 1;"),
    ("coast: goes on after a command", "if (coastLeft > 0 && coastVersion == version) // the same step as the one before", "if (coastLeft > 0) // the same step as the one before"),
    ("coast: periods in pieces handed out whole", "coastLeft    = constant && coastEnabled && pendPieces == 1 && pendDone == 0.0f", "coastLeft    = constant && coastEnabled && pendDone == 0.0f"),
    ("coast: never used", "coastLeft    = constant && coastEnabled && pendPieces == 1", "coastLeft    = constant && false && pendPieces == 1"),
    ("coast: a new maximum delay not noticed", "maxDelay = seconds >= 0.00001f ? seconds : 0.00001f;\n            version++;", "maxDelay = seconds >= 0.00001f ? seconds : 0.00001f;"),
    ("given speeds not kept exactly", "if (wNew == wMax) return vMax;", ""),
]

GROUP = [
    ("group: members step half a step late", "m[i].err = length / 2;", "m[i].err = 0;"),
    ("group: going backwards is not the exact reverse", "if (mem.err < mem.share)", "if (mem.err <= mem.share)"),
    ("group: direction bits not reversed going backwards", "event.dirMask = (uint16_t)(~forward & all);", "event.dirMask = forward;"),
    ("group: speed limit not scaled by the share", "vMax           = std::fmin(vMax, m[i].limits.maxSpeed * perShare);", "vMax           = std::fmin(vMax, m[i].limits.maxSpeed);"),
    ("group: acceleration of the members ignored", "acc            = std::fmin(acc, m[i].limits.acceleration * perShare);", "acc            = std::fmin(acc, m[0].limits.acceleration * 1.0f);"),
    ("group: deceleration of the members ignored", "dec            = std::fmin(dec, m[i].limits.deceleration * perShare);", "dec            = std::fmin(dec, m[0].limits.deceleration * 1.0f);"),
    ("group: pull-in speed of the members ignored", "pull           = std::fmin(pull, m[i].limits.pullInSpeed * perShare);", "pull           = std::fmin(pull, m[0].limits.pullInSpeed * 1.0f);"),
    ("group: new limits not applied during a move", "if (kind != Kind::none && length > 0) applyLimits();", ""),
    ("group segments: other targets accepted within a move", "if (targets[i] != m[i].target) return errorCode::notSupported;", "(void)0;"),
    ("group segments: new targets refused behind the targets", "if (e.arrived) passed = true;", ""),
    ("group segments: speed at the targets ignored", "errorCode code = path.moveAbs((int32_t)longest, speedAtTarget);", "errorCode code = path.moveAbs((int32_t)longest, 0);"),
    ("group segments: change of the speed at the targets ignored", "errorCode code = path.moveAbs((int32_t)length, speedAtTarget);", "errorCode code = errorCode::OK;"),
    ("group segments: speed at the targets changed behind the targets", "if (!path.isMoving() || kind != Kind::move || passed) return errorCode::notSupported;", "if (!path.isMoving() || kind != Kind::move) return errorCode::notSupported;"),
    ("group segments: speed at the targets of a rotating group changed", "if (!path.isMoving() || kind != Kind::move || passed) return errorCode::notSupported;", "if (!path.isMoving() || passed) return errorCode::notSupported;"),
    ("group segments: given speed at the start ignored", "if (!keep) path.setSpeed(speedAtStart);", ""),
    ("group segments: negative speed accepted", "if (!(speedAtTarget >= 0) || !(keep || speedAtStart >= 0)) return errorCode::invalidArgument;", ""),
    ("group segments: rotating group takes a move", "if (kind != Kind::move) return errorCode::notSupported;\n                if (!passed)", "if (!passed)"),
    ("group segments: move over zero steps does not stop a running group", "path.emergencyStop(); // they are there, but moving: nothing is left to slow down on", ""),
    ("group speed: speed of the group ignored", "if (ownSpeed > 0 && !speedCapped) vMax = ownSpeed;", ""),
    ("group speed: acceleration of the group ignored", "if (ownAcc > 0 && !accCapped) acc = ownAcc;", ""),
    ("group speed: deceleration of the group ignored", "if (ownDec > 0 && !decCapped) dec = ownDec;", ""),
    ("group speed: limit by the members not noticed", "speedCapped = ownSpeed > vMax;", "speedCapped = false;"),
    ("group speed: acceleration limit by the members not noticed", "accCapped   = ownAcc > acc;", "accCapped   = false;"),
    ("group speed: not applied during a move", "ownSpeed = v > 0 ? v : 0;\n            if (kind == Kind::none || length == 0) return errorCode::OK;\n            applyLimits();", "ownSpeed = v > 0 ? v : 0;\n            if (kind == Kind::none || length == 0) return errorCode::OK;"),
    ("group speed: acceleration does not set the deceleration", "if (!separateDeceleration) ownDec = ownAcc;", ""),
    ("group: new ratio accepted while rotating", "if (delta > 1) return errorCode::notSupported;", "if (delta > 1 && false) return errorCode::notSupported;"),
    ("group: mixed direction change accepted while rotating", "if (sense != 0 && s != sense) return errorCode::notSupported;", ""),
    ("group: speed factor ignores the given speed", "return path.rotate(factor * baseSpeed);", "return path.rotate(factor * 1000.0f);"),
    ("group: member speed without direction", "return path.speed() * ((float)m[i].share / (float)length) * (float)m[i].dir;", "return path.speed() * ((float)m[i].share / (float)length);"),
    ("group: wait events carry a step", "if (e.dir == 0) // wait only\n            {\n                event.dirMask = lastDirs;", "if (e.dir == 0) // wait only\n            {\n                event.stepMask = 1;\n                event.dirMask = lastDirs;"),
]

STEPPER = [
    ("api: maximum speed not changed during a move", "reversed = negative;\n            applyLimits();", "reversed = negative;"),
    ("api: relative move from the position at standstill", "int32_t here = c ? c->axis.position() : pos;", "int32_t here = pos;"),
    ("api: no arrival for a move over zero steps", "arrivals = arrivals + 1;\n            if (arrival) arrival();", "arrivals = arrivals + 1;"),
    ("api: hardware setting accepted during a move", "Engine::report(errorCode::notSupported);\n        return false;", "return true;"),
    ("api: blocking move returns at once", "while (isMoving()) backend::idle();\n        else", "{}\n        else"),
    ("api: command does not start the backend", "backend::start((unsigned)channel);\n        }\n    done:", "}\n    done:"),
    ("api: stop of a member does not stop its group", "code = cmd == Command::stop ? c->group.stop() : errorCode::motorBusy;", "code = errorCode::motorBusy;"),
    ("api: member of a moving group takes a move command", "if (!c->ending) // the motor is moving in a group", "if (!c->ending && cmd == Command::stop) // the motor is moving in a group"),
]

CHANNELS = [
    ("api: emergency stop counts steps that were not made", "s->pos       = (int32_t)((uint32_t)here - (uint32_t)(notExecuted ? notExecuted[i] : 0));", "s->pos       = (int32_t)((uint32_t)here - (uint32_t)(notExecuted ? 0 : 0));"),
    ("api: position not kept when a move ends", "s->pos       = (int32_t)((uint32_t)here - (uint32_t)(notExecuted ? notExecuted[i] : 0));", "s->pos       = s->pos + (int32_t)(0 * (uint32_t)(notExecuted ? here : here));"),
    ("api: motor stays busy when a move ends", "s->channel   = -1;", ""),
    ("api: arrival function not called", "s->arrivals = s->arrivals + 1;\n            if (s->arrival) s->arrival();", "s->arrivals = s->arrivals + 1;"),
    ("api: error not reported to the error function", "if (code != errorCode::OK && errFunc) errFunc(code);", "if (code != errorCode::OK && errFunc && false) errFunc(code);"),
    ("api: channel kept after a failed open", "if (code != errorCode::OK) return -1;\n            c.used = true;", "c.used = true;\n            if (code != errorCode::OK) return -1;"),
    ("api: pulse width not passed to the backend", "c.pulseWidth = largest(c.pulseWidth, motors[i]->pulseWidth);", ""),
    ("api: positions of group members not given back", "int32_t here = c.isGroup ? c.group.position(i) : c.axis.position();", "int32_t here = c.axis.position();"),
    ("api: group arrival function not called", "if (c.arrival) c.arrival();\n                return;", "return;"),
    ("api: speed factor of version 2 moves ignored", "core::Axis::maxSpeedLimit : s.vMax * c.speedFactor;", "core::Axis::maxSpeedLimit : s.vMax;"),
    ("api: version 2 rotation limited to the maximum speed", "l.maxSpeed     = c.unlimitedSpeed ? core::Axis::maxSpeedLimit : s.vMax * c.speedFactor;", "l.maxSpeed     = s.vMax * c.speedFactor;"),
    ("enable: driver not switched on", "if (enableUse[s->enablePin]++ == 0) backend::setPin(s->enablePin, s->enableActiveHigh);", "enableUse[s->enablePin]++;"),
    ("enable: no delay before the first step", "c.startDelay = largest(c.startDelay, s->onDelay); // also if another motor has just switched the shared signal on", ""),
    ("enable: driver switched off at once after a move", "c.ending  = true; // the move is over\n                c.offLeft = c.offDelay;", "c.ending  = true; // the move is over\n                c.offLeft = 0;"),
    ("enable: driver not switched off", "backend::setPin(s->enablePin, !s->enableActiveHigh);\n            }\n            c.used = false;", "(void)0;\n            }\n            c.used = false;"),
    ("enable: shared signal switched off with the first motor", "enableUse[s->enablePin] > 0 && --enableUse[s->enablePin] == 0)", "enableUse[s->enablePin] > 0 && --enableUse[s->enablePin] != 255)"),
    ("enable: motor counts as moving while the driver waits", "return !(c.ending && c.offDelay > 0);", "return !(c.ending && c.offDelay > 0 && false);"),
    ("enable: emergency stop with off-delay counts steps not made", "c.axis.setPosition((int32_t)((uint32_t)c.axis.position() - (uint32_t)notExecuted[0]));", "c.axis.setPosition(c.axis.position());"),
    ("enable: driver off at once after an emergency stop", "if (c.offDelay <= 0)\n            {\n                endNow(ch);\n                return;\n            }", "{\n                endNow(ch);\n                return;\n            }"),
]

GROUPAPI = [
    ("group api: busy member not noticed", "if (!Engine::channels[other].ending) return errorCode::motorBusy; // the member is moving on its own or in another group", ""),
    ("group api: targets not stored on the members", "for (unsigned i = 0; i < count; i++) motors[i]->setTargetAbs(targets[i]);", ""),
    ("group api: wrong number of targets accepted", "errorCode StepperGroup::start(const int32_t* targets, unsigned n, float speedAtTarget, float speedAtStart)\n    {\n        if (n != count || count == 0)", "errorCode StepperGroup::start(const int32_t* targets, unsigned n, float speedAtTarget, float speedAtStart)\n    {\n        if (n < count || count == 0)"),
    ("group speed api: settings of the group not given to the move", "applySettings(c, Setting::none); // a restriction by the members is reported by the move", ""),
    ("group speed api: speed not applied during a move", "if (ch >= 0) applySettings(Engine::channels[ch], Setting::speed);", "if (ch >= 0 && false) applySettings(Engine::channels[ch], Setting::speed);"),
    ("group speed api: acceleration not applied during a move", "if (ch >= 0) applySettings(Engine::channels[ch], Setting::acceleration);", "if (ch >= 0 && false) applySettings(Engine::channels[ch], Setting::acceleration);"),
    ("group speed api: restriction of a setter not reported", "if (changed == Setting::acceleration) Engine::report(acc != errorCode::OK || separateDeceleration ? acc : dec);", "if (changed == Setting::acceleration && false) Engine::report(acc != errorCode::OK || separateDeceleration ? acc : dec);"),
    ("group speed api: separate deceleration not used", "errorCode dec   = c.group.setDeceleration(separateDeceleration ? ownDec : ownAcc);", "errorCode dec   = c.group.setDeceleration(ownAcc);"),
    ("group speed api: speed of a group that stands still", "if (ch < 0 || Engine::channels[ch].ending) return 0.0f;\n        float v = Engine::channels[ch].group.lead().speed();", "if (ch < 0) return 0.0f;\n        float v = 1.0f + Engine::channels[ch].group.lead().speed();"),
    ("group api: a move of the group itself passes its targets at a speed", "return start(targets, n, 0, core::Group::keepSpeed);", "return start(targets, n, 100, core::Group::keepSpeed);"),
    ("group segments api: change of the speed at the targets does nothing", "code = Engine::channels[ch].group.setSpeedAtTarget(speedAtTarget);", "code = speedAtTarget >= 0 ? errorCode::OK : errorCode::invalidArgument;"),
    ("group segments api: speed at the start not passed on", "code = c.group.moveAbs(targets, speedAtTarget, speedAtStart);", "code = c.group.moveAbs(targets, speedAtTarget, speedAtStart * 0 + core::Group::keepSpeed);"),
    ("group api: speeds for the speed factor not remembered", "hasBaseSpeed = !allZero;", "hasBaseSpeed = false;"),
    ("group api: a member twice in a group", "if (known || count >= maxMembers)", "if (count >= maxMembers)"),
    ("group api: group counts as moving when a member moves alone", "if (!c.isGroup || c.count != count) return -1;", "if (c.count != count) return -1;"),
    ("group api: stop does nothing", "if (ch >= 0) code = Engine::channels[ch].group.stop();\n        }\n        return Engine::report(code);\n    }\n\n    errorCode StepperGroup::stop()", "(void)ch;\n        }\n        return Engine::report(code);\n    }\n\n    errorCode StepperGroup::stop()"),
    ("group api: member without a target moves to position 0", "targets[i] = motors[i]->hasTarget ? motors[i]->target : motors[i]->getPosition();", "targets[i] = motors[i]->target;"),
]

GROUPPATH = [
    ("group path: speed at the targets not passed on (list)", "return g.start(targets.begin(), (unsigned)targets.size(), speedAtTarget, speedAtStart);", "return g.start(targets.begin(), (unsigned)targets.size(), speedAtTarget * 0, speedAtStart);"),
    ("group path: speed at the targets not passed on (array)", "return g.start(targets, n, speedAtTarget, speedAtStart);", "return g.start(targets, n, speedAtTarget * 0, speedAtStart);"),
    ("group path: speed at the start not passed on", "return g.start(targets.begin(), (unsigned)targets.size(), speedAtTarget, speedAtStart);", "return g.start(targets.begin(), (unsigned)targets.size(), speedAtTarget, speedAtStart * 0 + keepSpeed);"),
    ("group path: relative move taken as absolute (array)", "return g.startRel(distances, n, speedAtTarget, speedAtStart);", "return g.start(distances, n, speedAtTarget, speedAtStart);"),
    ("group path: relative move taken as absolute (list)", "return g.startRel(distances.begin(), (unsigned)distances.size(), speedAtTarget, speedAtStart);", "return g.start(distances.begin(), (unsigned)distances.size(), speedAtTarget, speedAtStart);"),
    ("group path: change of the speed at the targets not passed on", "return g.changeSpeedAtTarget(speedAtTarget);", "return g.changeSpeedAtTarget(speedAtTarget * 0);"),
]

SRC = CORE.parent
FILES = [(CORE / "Axis.cpp", AXIS), (CORE / "Group.cpp", GROUP), (SRC / "api" / "Stepper.cpp", STEPPER),
         (SRC / "api" / "Channels.cpp", CHANNELS), (SRC / "api" / "StepperGroup.cpp", GROUPAPI), (SRC / "api" / "GroupPath.cpp", GROUPPATH)]


def kill_tests():
    """Ends a test program that is still running (it does not always die with the script that started it)."""
    if sys.platform == "win32":
        subprocess.run(["taskkill", "/F", "/IM", "tests.exe"], capture_output=True)
        time.sleep(0.3)
    else:
        subprocess.run(["pkill", "-f", str(TEST / ".build" / "tests.exe")], capture_output=True)


def run_tests(timeout: int) -> str | None:
    """Output of the PC tests, or None if they did not end in time. The output goes to a file, not to a pipe,
    and a run that does not end is killed with its child processes: otherwise a hanging test program would
    keep this script waiting forever."""
    log = TEST / ".build" / "mutate.log"
    log.parent.mkdir(exist_ok=True)
    kill_tests()  # a test program left over from the previous run would block the linker
    with open(log, "w", encoding="utf-8") as f:
        p = subprocess.Popen([sys.executable, str(TEST / "run_tests.py")], stdout=f, stderr=subprocess.STDOUT)
        try:
            p.wait(timeout)
        except subprocess.TimeoutExpired:
            if sys.platform == "win32":
                subprocess.run(["taskkill", "/F", "/T", "/PID", str(p.pid)], capture_output=True)
            else:
                p.kill()
            p.wait()
            kill_tests()
            return None
    return log.read_text(encoding="utf-8", errors="replace")


def main() -> int:
    missing = [name for src, mutants in FILES for name, a, _ in mutants if src.read_text(encoding="utf-8").count(a) != 1]
    if missing:
        print("code not found (or not unique) for:", *missing, sep="\n  ")
        return 2
    survivors = total = 0
    for src, mutants in FILES:
        orig = src.read_text(encoding="utf-8")
        selected = [m for m in mutants if len(sys.argv) < 2 or sys.argv[1] in m[0]]
        total += len(selected)
        try:
            for name, a, b in selected:
                src.write_text(orig.replace(a, b), encoding="utf-8", newline="\n")
                out = run_tests(60)
                if out is not None and "LINK FAILED" in out: # the test program of the run before was still going away
                    time.sleep(2)
                    out = run_tests(60)
                if out is None:
                    print(f"{name:56} noticed (the tests did not end)", flush=True)
                    continue
                m = re.search(r"test cases:\s+(\d+) \|\s+(\d+) passed \|\s+(\d+) failed", out)
                if m:
                    failed = int(m.group(3))
                    verdict = f"noticed by {failed} test cases" if failed else "SURVIVED"
                else:
                    failed = 1
                    verdict = "does not build" if "FAILED" in out else "noticed (the tests crashed)"
                survivors += failed == 0
                print(f"{name:56} {verdict}", flush=True)
        finally:
            src.write_text(orig, encoding="utf-8", newline="\n")
    run_tests(300)  # rebuild the original
    print(f"\n{survivors} of {total} survived")
    return 1 if survivors else 0


if __name__ == "__main__":
    sys.exit(main())
