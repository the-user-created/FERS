// falpha_branch.h
// Created by David Young on 9/17/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef FALPHA_BRANCH_H
#define FALPHA_BRANCH_H

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

		~FAlphaBranch() { clean(); }

		// Delete copy constructor and assignment operator to make the class noncopyable
		FAlphaBranch(const FAlphaBranch&) = delete;

		FAlphaBranch& operator=(const FAlphaBranch&) = delete;

		RealType getSample();

		void flush(RealType scale);

		[[nodiscard]] FAlphaBranch* getPre() const { return _pre.get(); }

	private:
		void init();

		void clean();

		void refill();

		RealType calcSample();

		std::unique_ptr<signal::IirFilter> _shape_filter;
		RealType _shape_gain{};
		std::unique_ptr<signal::IirFilter> _integ_filter;
		RealType _integ_gain{};
		RealType _upsample_scale;
		std::unique_ptr<signal::IirFilter> _highpass;
		std::unique_ptr<FAlphaBranch> _pre;
		bool _last;
		std::unique_ptr<signal::DecadeUpsampler> _upsampler;
		std::vector<RealType> _buffer;
		unsigned _buffer_samples{};
		RealType _ffrac;
		RealType _fint;
		RealType _offset_sample{};
		bool _got_offset{};
		RealType _pre_scale{};
	};
}

#endif //FALPHA_BRANCH_H
