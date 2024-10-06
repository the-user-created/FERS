// dsp_filters.h
// Digital Signal Processing support functions
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 30 July 2007

#pragma once

#include <memory>
#include <span>
#include <vector>

#include "config.h"

namespace signal
{
	void upsample(std::span<const ComplexType> in, std::span<ComplexType> out, unsigned ratio);

	void downsample(std::span<const ComplexType> in, std::span<ComplexType> out, unsigned ratio);

	class DspFilter
	{
	public:
		DspFilter() = default;

		virtual ~DspFilter() = default;

		// Pure virtual methods for filtering
		virtual RealType filter(RealType sample) = 0;

		virtual void filter(std::span<RealType> samples) = 0;

		// Disable copy constructor and copy assignment operator
		DspFilter(const DspFilter&) = delete;

		DspFilter& operator=(const DspFilter&) = delete;

		// Enable default move semantics
		DspFilter(DspFilter&&) noexcept = default;

		DspFilter& operator=(DspFilter&&) noexcept = default;
	};

	class IirFilter final : public DspFilter
	{
	public:
		IirFilter(std::span<const RealType> denCoeffs, std::span<const RealType> numCoeffs);

		IirFilter(const RealType* denCoeffs, const RealType* numCoeffs, unsigned order);

		~IirFilter() override = default;

		RealType filter(RealType sample) override;

		void filter(std::span<RealType> samples) override;

	private:
		std::vector<RealType> _a; // Denominator coefficients
		std::vector<RealType> _b; // Numerator coefficients
		std::vector<RealType> _w; // State (internal delay line)
		unsigned _order{};
	};

	class FirFilter final : public DspFilter
	{
	public:
		explicit FirFilter(std::span<const RealType> coeffs)
			: _filter(coeffs.begin(), coeffs.end()), _w(coeffs.size()), _order(coeffs.size()) {}

		~FirFilter() override = default;

		RealType filter(RealType) override { return 0; } // Not used
		void filter(std::span<RealType> samples) override;

		void filter(std::span<ComplexType> samples) const;

	private:
		std::vector<RealType> _filter; // FIR filter coefficients
		std::vector<RealType> _w; // State for the filter
		unsigned _order{};
	};

	class ArFilter final : public DspFilter
	{
	public:
		explicit ArFilter(std::span<const RealType> coeffs)
			: _w(coeffs.size()), _filter(coeffs.begin(), coeffs.end()), _order(coeffs.size()) {}

		~ArFilter() override = default;

		RealType filter(RealType sample) override;

		void filter(std::span<RealType> samples) override;

	private:
		std::vector<RealType> _w; // Internal state
		std::vector<RealType> _filter; // Coefficients
		unsigned _order{};

		RealType applyFilter(RealType sample);
	};

	class Upsampler
	{
	public:
		explicit Upsampler(int ratio);

		~Upsampler() = default;

		void upsample(std::span<const RealType> inSamples, std::span<RealType> outSamples);

		// Disable copy constructor and copy assignment operator
		Upsampler(const Upsampler&) = delete;

		Upsampler& operator=(const Upsampler&) = delete;

		// Enable default move semantics
		Upsampler(Upsampler&&) noexcept = default;

		Upsampler& operator=(Upsampler&&) noexcept = default;

	private:
		int _ratio;
		int _filter_size;
		std::vector<RealType> _filterbank;
		std::vector<RealType> _sample_memory;

		[[nodiscard]] RealType getSample(const std::span<const RealType> samples, const long n) const
		{
			return n >= 0 ? samples[n] : _sample_memory[n + _filter_size];
		}
	};

	class DecadeUpsampler
	{
	public:
		DecadeUpsampler();

		~DecadeUpsampler() = default;

		void upsample(RealType sample, std::span<RealType> out) const;

		void upsample(std::span<const RealType> in, std::span<RealType> out) const;

		// Disable copy constructor and copy assignment operator
		DecadeUpsampler(const DecadeUpsampler&) = delete;

		DecadeUpsampler& operator=(const DecadeUpsampler&) = delete;

		// Enable default move semantics
		DecadeUpsampler(DecadeUpsampler&&) noexcept = default;

		DecadeUpsampler& operator=(DecadeUpsampler&&) noexcept = default;

	private:
		std::unique_ptr<IirFilter> _filter;
	};
}
