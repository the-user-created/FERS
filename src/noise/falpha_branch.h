// falpha_branch.h
// Created by David Young on 9/17/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef FALPHA_BRANCH_H
#define FALPHA_BRANCH_H

#include <config.h>
#include <memory>

#include "math_utils/dsp_filters.h"

namespace rs
{
	class DecadeUpsampler;
	class IirFilter;

	class FAlphaBranch
	{
	public:
		FAlphaBranch(RS_FLOAT ffrac, unsigned fint, std::unique_ptr<FAlphaBranch> pre, bool last);

		~FAlphaBranch() { clean(); }

		// Delete copy constructor and assignment operator to make the class noncopyable
		FAlphaBranch(const FAlphaBranch&) = delete;

		FAlphaBranch& operator=(const FAlphaBranch&) = delete;

		RS_FLOAT getSample();

		void flush(RS_FLOAT scale);

		[[nodiscard]] FAlphaBranch* getPre() const { return _pre.get(); }

	private:
		void init();

		void clean();

		void refill();

		RS_FLOAT calcSample();

		std::unique_ptr<IirFilter> _shape_filter;
		RS_FLOAT _shape_gain{};
		std::unique_ptr<IirFilter> _integ_filter;
		RS_FLOAT _integ_gain{};
		RS_FLOAT _upsample_scale;
		std::unique_ptr<IirFilter> _highpass;
		std::unique_ptr<FAlphaBranch> _pre;
		bool _last;
		std::unique_ptr<DecadeUpsampler> _upsampler;
		std::vector<RS_FLOAT> _buffer;
		unsigned _buffer_samples{};
		RS_FLOAT _ffrac;
		RS_FLOAT _fint;
		RS_FLOAT _offset_sample{};
		bool _got_offset{};
		RS_FLOAT _pre_scale{};
	};
}

#endif //FALPHA_BRANCH_H
