/**
 * @file prototype_timing.cpp
 * @brief Implementation file for the PrototypeTiming class.
 *
 * @authors David Young, Marc Brooker
 * @date 2024-09-17
 */

#include "prototype_timing.h"

#include "core/logging.h"
#include "noise/noise_utils.h"

using logging::Level;

namespace timing
{
	void PrototypeTiming::setAlpha(const RealType alpha, const RealType weight) noexcept
	{
		_alphas.emplace_back(alpha);
		_weights.emplace_back(weight);
	}

	void PrototypeTiming::copyAlphas(std::vector<RealType>& alphas, std::vector<RealType>& weights) const noexcept
	{
		alphas = _alphas;
		weights = _weights;
	}
}
