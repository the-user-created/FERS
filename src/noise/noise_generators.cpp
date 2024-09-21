// noise_generators.cpp
// Functions for generating different types of noise
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 14 August 2006

#include "noise_generators.h"

#include <ranges>

#include "core/parameters.h"

namespace noise
{
	// =================================================================================================================
	//
	// GAMMA GENERATOR CLASS
	//
	// =================================================================================================================

	GammaGenerator::GammaGenerator(const RS_FLOAT k) : _rng(std::make_unique<std::mt19937>(params::randomSeed())),
	                                                   _dist(k, 1.0) {}

	// =================================================================================================================
	//
	// WGN GENERATOR CLASS
	//
	// =================================================================================================================

	WgnGenerator::WgnGenerator(const RS_FLOAT stddev) : _rng(std::make_unique<std::mt19937>(params::randomSeed())),
	                                                    _dist(0.0, stddev), _stddev(stddev) {}

	WgnGenerator::WgnGenerator() : _rng(std::make_unique<std::mt19937>(params::randomSeed())),
	                               _dist(0.0, 1.0), _stddev(1.0) {}

	// =================================================================================================================
	//
	// MULTI-RATE GENERATOR CLASS
	//
	// =================================================================================================================

	MultirateGenerator::MultirateGenerator(const RS_FLOAT alpha, const unsigned branches)
	{
		const RS_FLOAT beta = -(alpha - 2) / 2.0;
		const int fint = static_cast<int>(std::floor(beta));
		const RS_FLOAT ffrac = fmod(beta, 1.0);

		createTree(ffrac, fint, branches);

		_scale = 1.0 / std::pow(10.0, (-alpha + 2.0) * 2.0);
	}

	void MultirateGenerator::skipSamples(long long samples) const
	{
		if (const int skip_branches = static_cast<int>(std::floor(std::log10(samples))) - 1; skip_branches > 0)
		{
			std::vector<FAlphaBranch*> flushbranches;
			const FAlphaBranch* branch = _topbranch.get(); // Using unique_ptr's get() to access raw pointer

			// Traverse the tree up to skip_branches or until the branch becomes null
			for (int i = 0; i < skip_branches && branch != nullptr; ++i)
			{
				flushbranches.push_back(const_cast<FAlphaBranch*>(branch));
				// Use const_cast to store non-const pointers for flushing
				branch = branch->getPre();
			}

			// If there is still a branch after skipping, call getSample the required number of times
			if (branch)
			{
				samples /= static_cast<long long>(std::pow(10.0, skip_branches));
				for (int i = 0; i < samples; ++i)
				{
					const_cast<FAlphaBranch*>(branch)->getSample(); // Remove constness for calling getSample
				}
			}

			// Flush the branches in reverse order to reset the chain
			const int size = static_cast<int>(flushbranches.size());
			if (size > 0) { flushbranches[size - 1]->flush(std::pow(10.0, skip_branches - 2.0)); }

			for (int i = size - 2; i >= 0; --i) { flushbranches[i]->flush(1.0); }
		}
		else
		{
			// No branch skipping required, just generate samples from the top branch
			for (int i = 0; i < samples; ++i) { _topbranch->getSample(); }
		}
	}

	void MultirateGenerator::createTree(const RS_FLOAT fAlpha, const int fInt, const unsigned branches)
	{
		if (branches == 0) { throw std::runtime_error("Cannot create multirate noise generator with zero branches"); }

		if (fAlpha == 0.0 && fInt == 0) { _topbranch = std::make_unique<FAlphaBranch>(0, 0, nullptr, true); }
		else
		{
			std::unique_ptr<FAlphaBranch> previous_branch = nullptr;

			// Build the chain of branches
			for (unsigned i = 0; i < branches - 1; ++i)
			{
				// Move the previous branch into the new FAlphaBranch constructor
				previous_branch = std::make_unique<FAlphaBranch>(fAlpha, fInt, std::move(previous_branch), false);
			}

			// The top branch is the last branch in the chain (set `last` to true)
			_topbranch = std::make_unique<FAlphaBranch>(fAlpha, fInt, std::move(previous_branch), true);
		}
	}

	void MultirateGenerator::reset() const
	{
		std::vector<FAlphaBranch*> flush_branches;
		FAlphaBranch* branch = _topbranch.get(); // Use get() to access the raw pointer from unique_ptr

		// Collect all branches
		while (branch)
		{
			flush_branches.push_back(branch);
			branch = branch->getPre();
		}

		// Flush branches in reverse order
		for (const auto& flush_branche : std::ranges::reverse_view(flush_branches)) { flush_branche->flush(1.0); }
	}

	// =================================================================================================================
	//
	// CLOCK MODEL GENERATOR CLASS
	//
	// =================================================================================================================


	ClockModelGenerator::ClockModelGenerator(const std::vector<RS_FLOAT>& alpha, const std::vector<RS_FLOAT>& inWeights,
	                                         const RS_FLOAT frequency, const RS_FLOAT phaseOffset,
	                                         const RS_FLOAT freqOffset, int branches)
		: _weights(inWeights), _phase_offset(phaseOffset), _freq_offset(freqOffset), _frequency(frequency)
	{
		auto iter = alpha.begin();
		auto witer = _weights.begin();

		for (; iter != alpha.end(); ++iter, ++witer)
		{
			auto mgen = std::make_unique<MultirateGenerator>(*iter, branches);
			_generators.push_back(std::move(mgen));

			switch (static_cast<int>(*iter))
			{
			case 2: *witer *= std::pow(10.0, 1.2250);
				break;
			case 1: *witer *= std::pow(10.0, 0.25);
				break;
			case 0: *witer *= std::pow(10.0, -0.25);
				break;
			case -1: *witer *= std::pow(10.0, -0.5);
				break;
			case -2: *witer *= std::pow(10.0, -1);
				break;
			default: *witer *= 1.0; // No change to the weight
				break;
			}
		}
	}

	RS_FLOAT ClockModelGenerator::getSample()
	{
		RS_FLOAT sample = 0;

		for (size_t i = 0; i < _generators.size(); ++i) { sample += _generators[i]->getSample() * _weights[i]; }

		sample += _phase_offset;
		sample += 2 * M_PI * _freq_offset * static_cast<double>(_count) / params::rate();
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
}
