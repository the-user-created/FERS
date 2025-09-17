// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
* @file response.h
* @brief Classes for managing radar signal responses.
*/

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "config.h"
#include "interpolation/interpolation_point.h"

class XmlElement;

namespace fers_signal
{
	class RadarSignal;
}

namespace radar
{
	class Transmitter;
}

namespace serial
{
	/**
	* @class Response
	* @brief Manages radar signal responses from a transmitter.
	*/
	class Response
	{
	public:
		/**
		* @brief Constructor for the `Response` class.
		*
		* @param wave Pointer to the radar signal object.
		* @param transmitter Pointer to the transmitter object.
		*/
		Response(const fers_signal::RadarSignal* wave, const radar::Transmitter* transmitter) noexcept :
			_transmitter(transmitter), _wave(wave) {}

		~Response() = default;
		Response(const Response&) = delete;
		Response& operator=(const Response&) = delete;
		Response(Response&&) = delete;
		Response& operator=(Response&&) = delete;

		/**
		* @brief Retrieves the start time of the response.
		*
		* @return Start time as a `RealType`. Returns 0.0 if no points are present.
		*/
		[[nodiscard]] RealType startTime() const noexcept { return _points.empty() ? 0.0 : _points.front().time; }

		/**
		* @brief Retrieves the end time of the response.
		*
		* @return End time as a `RealType`. Returns 0.0 if no points are present.
		*/
		[[nodiscard]] RealType endTime() const noexcept { return _points.empty() ? 0.0 : _points.back().time; }

		/**
		* @brief Adds an interpolation point to the response.
		*
		* @param point The interpolation point to be added.
		* @throws std::logic_error If the new point has a time earlier than the last point.
		*/
		void addInterpPoint(const interp::InterpPoint& point);

		/**
		* @brief Renders the response as XML.
		*
		* @param root XML element that will contain the response data.
		*/
		void renderXml(const XmlElement& root) const noexcept;

		/**
		* @brief Renders the response as a CSV.
		*
		* @param of Output stream to write the CSV data to.
		*/
		void renderCsv(std::ofstream& of) const noexcept;

		/**
		* @brief Renders the response in binary format.
		*
		* @param rate Output parameter for the signal rate.
		* @param size Output parameter for the size of the binary data.
		* @param fracWinDelay Delay factor applied during windowing.
		* @return A vector of `ComplexType` representing the binary data.
		*/
		std::vector<ComplexType> renderBinary(RealType& rate, unsigned& size, RealType fracWinDelay) const;

		/**
		* @brief Retrieves the length of the response.
		*
		* @return The length of the response as a `RealType`.
		*/
		[[nodiscard]] RealType getLength() const noexcept { return endTime() - startTime(); }

		/**
		* @brief Retrieves the name of the associated transmitter.
		*
		* @return The name of the transmitter as a `std::string`.
		*/
		[[nodiscard]] std::string getTransmitterName() const noexcept;

	private:
		const radar::Transmitter* _transmitter; ///< Pointer to the transmitter object.
		const fers_signal::RadarSignal* _wave; ///< Pointer to the radar signal object.
		std::vector<interp::InterpPoint> _points; ///< Vector of interpolation points.

		/**
		* @brief Helper function to render a single interpolation point as XML.
		*
		* @param root XML element that will contain the point data.
		* @param point Interpolation point to be rendered.
		*/
		void renderResponseXml(const XmlElement& root, const interp::InterpPoint& point) const noexcept;
	};

	/**
	* @brief Compares the start times of two `Response` objects.
	*
	* @param a First response object to compare.
	* @param b Second response object to compare.
	* @return `true` if the start time of `a` is earlier than `b`, `false` otherwise.
	*/
	inline bool compareTimes(const std::unique_ptr<Response>& a, const std::unique_ptr<Response>& b)
	{
		return a->startTime() < b->startTime();
	}
}
