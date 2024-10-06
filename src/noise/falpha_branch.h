// falpha_branch.h
// Created by David Young on 9/17/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#pragma once

#include <memory>
#include <vector>

#include "config.h"
#include "signal_processing/dsp_filters.h"

namespace noise
{
	class FAlphaBranch
	{
	public:
		FAlphaBranch(RealType ffrac, unsigned fint, std::unique_ptr<FAlphaBranch> pre, bool last);

		~FAlphaBranch() = default;

		// Delete copy constructor and assignment operator to make the class noncopyable
		FAlphaBranch(const FAlphaBranch&) = delete;

		FAlphaBranch& operator=(const FAlphaBranch&) = delete;

		RealType getSample();

		void flush(RealType scale);

		[[nodiscard]] FAlphaBranch* getPre() const { return _pre.get(); }

	private:
		void init();

		void refill();

		RealType calcSample();

		std::unique_ptr<signal::IirFilter> _shape_filter;
		std::unique_ptr<signal::IirFilter> _integ_filter;
		std::unique_ptr<signal::IirFilter> _highpass;
		std::unique_ptr<signal::DecadeUpsampler> _upsampler;
		std::unique_ptr<FAlphaBranch> _pre;

		RealType _shape_gain{1.0};
		RealType _integ_gain{1.0};
		RealType _upsample_scale{};
		std::vector<RealType> _buffer{10}; // Initializing buffer to size 10
		unsigned _buffer_samples{};
		RealType _ffrac;
		RealType _fint;
		RealType _offset_sample{};
		bool _got_offset{false};
		RealType _pre_scale{1.0};
		bool _last;
	};
}
