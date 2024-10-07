//
// Created by David Young on 10/7/24.
//

#pragma once

#include "radar_obj.h"
#include "serial/response.h"

namespace radar
{
	class Receiver final : public Radar
	{
	public:
		enum class RecvFlag { FLAG_NODIRECT = 1, FLAG_NOPROPLOSS = 2 };

		explicit Receiver(Platform* platform,
		                  std::string name = "defRecv") noexcept : Radar(platform, std::move(name)) {}

		~Receiver() override = default;

		void addResponse(std::unique_ptr<serial::Response> response) noexcept;

		[[nodiscard]] bool checkFlag(RecvFlag flag) const noexcept { return _flags & static_cast<int>(flag); }

		void render();

		[[nodiscard]] RealType getNoiseTemperature() const noexcept { return _noise_temperature; }
		[[nodiscard]] RealType getWindowLength() const noexcept { return _window_length; }
		[[nodiscard]] RealType getWindowPrf() const noexcept { return _window_prf; }
		[[nodiscard]] RealType getWindowSkip() const noexcept { return _window_skip; }
		[[nodiscard]] Receiver* getDual() const noexcept { return _dual; }

		[[nodiscard]] RealType getNoiseTemperature(const math::SVec3& angle) const noexcept override;

		[[nodiscard]] RealType getWindowStart(int window) const;

		[[nodiscard]] int getWindowCount() const noexcept;

		[[nodiscard]] int getResponseCount() const noexcept;

		void setWindowProperties(RealType length, RealType prf, RealType skip) noexcept;

		void setFlag(RecvFlag flag) noexcept { _flags |= static_cast<int>(flag); }
		void setDual(Receiver* dual) noexcept { _dual = dual; }

		void setNoiseTemperature(RealType temp);

	private:
		std::vector<std::unique_ptr<serial::Response>> _responses;
		mutable std::mutex _responses_mutex;
		RealType _noise_temperature = 0;
		RealType _window_length = 0;
		RealType _window_prf = 0;
		RealType _window_skip = 0;
		Receiver* _dual = nullptr;
		int _flags = 0;
	};

	inline Receiver* createMultipathDual(Receiver* recv, const math::MultipathSurface* surf)
	{
		return createMultipathDualBase(recv, surf);
	}
}
