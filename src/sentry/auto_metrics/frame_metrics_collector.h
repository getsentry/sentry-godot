#pragma once

#include "metrics_collector.h"

#include <random>

namespace sentry {

// Collects and reports frame time and FPS as metrics.
class FrameMetricsCollector final : public MetricsCollector {
private:
	std::minstd_rand _rng{ std::random_device{}() };
	uint64_t _num_frames = 0;
	uint64_t _last_usec = 0;
	double _sampled_frametime = 0.0;
	uint64_t _deadline = 0;

protected:
	void _process(uint64_t p_now_usec) override;

public:
	void reset() override;
};

} //namespace sentry
