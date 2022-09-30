#include "game/main_loop.h"
#include "entry/game_loader.h"

#include <iostream>

#include <halley/support/console.h>
#include <halley/support/debug.h>

#include "halley/core/api/halley_api.h"

#include <chrono>
#include <thread>
#include <cstdint>

using namespace Halley;

MainLoop::MainLoop(IMainLoopable& target, GameLoader& reloader)
	: target(target)
	, reloader(reloader)
{
}

void MainLoop::run()
{
	capFrameRate = true;
	fps = target.getTargetFPS();

	do {
		runLoop();
	} while (tryReload());
}

void MainLoop::runLoop()
{
	std::cout << ConsoleColour(Console::GREEN) << "\nStarting main loop." << ConsoleColour() << std::endl;

	using Clock = std::chrono::steady_clock;
	using namespace std::chrono_literals;

	int64_t nSteps = 0;
	Clock::time_point startTime;
	Clock::time_point targetTime;
	Clock::time_point lastTime;
	startTime = targetTime = lastTime = Clock::now();

	if (fps <= 0) {
		while (isRunning()) {
			target.transitionStage();
			constexpr Time fixedDelta = 1.0 / 60.0;
			const Clock::time_point curTime = Clock::now();
			target.onFixedUpdate(fixedDelta);
			target.onTick(curTime, fixedDelta);
		}
	} else {
		while (isRunning()) {
			if (target.transitionStage()) {
				// Reset counters
				startTime = targetTime = lastTime = Clock::now();
				nSteps = 0;
			}
			Clock::time_point curTime = Clock::now();

			// Got any fixed updates to do?
			if (curTime >= targetTime) {
				// Figure out how many steps we need...
				const Time fixedDelta = 1.0 / fps;
				int stepsNeeded = int(std::chrono::duration<float>(curTime - targetTime).count() * fps);

				// Run up to 5 (if we're more than 5 frames late, ignore them. C'est la vie)
				for (int i = 0; i < std::min(stepsNeeded, 5); i++) {
					target.onFixedUpdate(fixedDelta);
				}

				// Update target
				nSteps += stepsNeeded;
				targetTime = startTime + std::chrono::microseconds((nSteps * 1000000ll) / fps);
			} else {
				// Nope, release CPU
				std::this_thread::yield();
			}

			// Run variable update
			const auto delta = std::min(std::chrono::duration<double>(curTime - lastTime).count(), 0.1); // Never step by more than 100ms
			target.onTick(curTime, delta);
			lastTime = curTime;
		}
	}

	std::cout << ConsoleColour(Console::GREEN) << "Main loop terminated." << ConsoleColour() << std::endl;
}

bool MainLoop::isRunning() const
{
	return target.isRunning() && !reloader.needsToReload();
}

bool MainLoop::tryReload() const
{
	if (reloader.needsToReload()) {
		reloader.reload();
		return true;
	}
	return false;
}
