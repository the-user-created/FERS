/**
 * @file noise_generators.cpp
 * @brief Implementation file for noise generator classes.
 *
 * @authors David Young, Marc Brooker
 * @date 2006-08-14
 */

#include "noise_generators.h"

#include <cmath>
#include <ranges>
#include <stdexcept>
#include <utility>

#include "core/parameters.h"
#include "noise/falpha_branch.h"

namespace noise
{
	MultirateGenerator::MultirateGenerator(const RealType alpha, const unsigned branches)
	{
		const RealType beta = -(alpha - 2) / 2.0;
		const int fint = static_cast<int>(std::floor(beta));
		const RealType ffrac = std::fmod(beta, 1.0);

		createTree(ffrac, fint, branches);
		_scale = 1.0 / std::pow(10.0, (-alpha + 2.0) * 2.0);
	}

	void MultirateGenerator::skipSamples(const long long samples) const noexcept
	{
		if (const int skip_branches = static_cast<int>(std::log10(samples)) - 1; skip_branches > 0)
		{
			std::vector<FAlphaBranch*> flushbranches;
			const FAlphaBranch* branch = _topbranch.get();

			for (int i = 0; i < skip_branches && branch; ++i)
			{
				flushbranches.push_back(const_cast<FAlphaBranch*>(branch));
				branch = branch->getPre();
			}

			if (branch)
			{ const auto reduced_samples = samples / static_cast<long long>(std::pow(10.0, skip_branches));
				for (long long i = 0; i < reduced_samples; ++i) { const_cast<FAlphaBranch*>(branch)->getSample(); }
			}

			for (const auto& fb : std::ranges::reverse_view(flushbranches)) { fb->flush(1.0); }
		}
		else { for (long long i = 0; i < samples; ++i) { _topbranch->getSample(); } }
	}

	void MultirateGenerator::createTree(const RealType fAlpha, const int fInt, const unsigned branches)
	{
		// TODO: Should move to the constructor
		if (branches == 0)
		{
			LOG(logging::Level::FATAL, "Cannot create multirate noise generator with zero branches");
			throw std::runtime_error("Cannot create multirate noise generator with zero branches");
		}

		std::unique_ptr<FAlphaBranch> previous_branch = nullptr;
		for (unsigned i = 0; i < branches; ++i)
		{
			previous_branch = std::make_unique<FAlphaBranch>(fAlpha, fInt, std::move(previous_branch),
			                                                 i == branches - 1);
		}
		_topbranch = std::move(previous_branch);
	}

	void MultirateGenerator::reset() const noexcept
	{
		std::vector<FAlphaBranch*> branches;
		FAlphaBranch* branch = _topbranch.get();

		while (branch)
		{
			branches.push_back(branch);
			branch = branch->getPre();
		}

		for (const auto& b : std::ranges::reverse_view(branches)) { b->flush(1.0); }
	}

	ClockModelGenerator::ClockModelGenerator(const std::vector<RealType>& alpha, const std::vector<RealType>& inWeights,
	                                         const RealType frequency, const RealType phaseOffset,
	                                         const RealType freqOffset, int branches) noexcept
		: _weights(inWeights), _phase_offset(phaseOffset), _freq_offset(freqOffset), _frequency(frequency)
	{
		for (size_t i = 0; i < alpha.size(); ++i)
		{
			auto mgen = std::make_unique<MultirateGenerator>(alpha[i], branches);
			_generators.push_back(std::move(mgen));

			switch (static_cast<int>(alpha[i]))
			{
			case 2: _weights[i] *= std::pow(10.0, 1.225);
				break;
			case 1: _weights[i] *= std::pow(10.0, 0.25);
				break;
			case 0: _weights[i] *= std::pow(10.0, -0.25);
				break;
			case -1: _weights[i] *= std::pow(10.0, -0.5);
				break;
			case -2: _weights[i] *= std::pow(10.0, -1.0);
				break;
			default: break;
			}
		}
	}

	RealType ClockModelGenerator::getSample()
	{
		RealType sample = 0;
		for (size_t i = 0; i < _generators.size(); ++i) { sample += _generators[i]->getSample() * _weights[i]; }

		sample += _phase_offset;
		sample += 2 * PI * _freq_offset * static_cast<double>(_count) / params::rate();
		++_count;

		return sample;
	}

	void ClockModelGenerator::skipSamples(const long long samples)
	{
		for (const auto& generator : _generators) { generator->skipSamples(samples); }
		_count += samples;
	}

	void ClockModelGenerator::reset()
	{
		for (const auto& generator : _generators) { generator->reset(); }
		_count = 0;
	}

	bool ClockModelGenerator::enabled() const
	{
		return !_generators.empty() || _freq_offset != 0 || _phase_offset != 0;
	}
}
