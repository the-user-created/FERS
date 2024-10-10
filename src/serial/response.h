/**
* @file response.h
* @brief Classes for managing radar signal responses.
*
* This file contains the `Response` class which handles the creation, storage, and
* rendering of radar signal responses in various formats such as XML, CSV, and binary.
* The `Response` class works with radar signal and transmitter objects to track
* interpolation points and generate structured output for further analysis or visualization.
*
* @authors David Young, Marc Brooker
* @date 2006-08-03
*/

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "config.h"
#include "interpolation/interpolation_point.h"

class XmlElement;

namespace signal
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
	*
	* The `Response` class stores radar signal response data and provides functionality
	* to render the response in multiple formats, including XML, CSV, and binary. It is responsible
	* for handling the interpolation points associated with radar signals and ensuring that
	* the response data can be used for further signal processing and visualization.
	* Copying of `Response` objects is disabled to ensure unique handling of each response.
	*/
	class Response
	{
	public:
		/**
		* @brief Constructor for the `Response` class.
		*
		* Initializes a response object with a radar signal and transmitter.
		*
		* @param wave Pointer to the radar signal object.
		* @param transmitter Pointer to the transmitter object.
		*/
		Response(const signal::RadarSignal* wave, const radar::Transmitter* transmitter) noexcept :
			_transmitter(transmitter), _wave(wave) {}

		/**
		* @brief Destructor for the `Response` class.
		*
		* Default destructor that releases any resources.
		*/
		~Response() = default;

		/// Delete copy constructor to prevent copying of `Response` objects.
		Response(const Response&) = delete;

		/// Delete copy assignment operator to prevent copying of `Response` objects.
		Response& operator=(const Response&) = delete;

		/**
		* @brief Retrieves the start time of the response.
		*
		* The start time corresponds to the time of the first interpolation point.
		*
		* @return Start time as a `RealType`. Returns 0.0 if no points are present.
		*/
		[[nodiscard]] RealType startTime() const noexcept { return _points.empty() ? 0.0 : _points.front().time; }

		/**
		* @brief Retrieves the end time of the response.
		*
		* The end time corresponds to the time of the last interpolation point.
		*
		* @return End time as a `RealType`. Returns 0.0 if no points are present.
		*/
		[[nodiscard]] RealType endTime() const noexcept { return _points.empty() ? 0.0 : _points.back().time; }

		/**
		* @brief Adds an interpolation point to the response.
		*
		* Appends a new interpolation point to the response. Points must be added in
		* ascending order of time.
		*
		* @param point The interpolation point to be added.
		* @throws std::logic_error If the new point has a time earlier than the last point.
		*/
		void addInterpPoint(const interp::InterpPoint& point);

		/**
		* @brief Renders the response as XML.
		*
		* Generates an XML representation of the radar response and attaches it to the
		* provided root element.
		*
		* @param root XML element that will contain the response data.
		*/
		void renderXml(const XmlElement& root) noexcept;

		/**
		* @brief Renders the response as a CSV.
		*
		* Outputs the response data in CSV format, writing it to the provided output stream.
		*
		* @param of Output stream to write the CSV data to.
		*/
		void renderCsv(std::ofstream& of) const noexcept;

		/**
		* @brief Renders the response in binary format.
		*
		* Converts the radar response data into a binary format for further processing.
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
		* Computes the duration of the response based on the start and end times.
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
		const signal::RadarSignal* _wave; ///< Pointer to the radar signal object.
		std::vector<interp::InterpPoint> _points; ///< Vector of interpolation points.

		/**
		* @brief Helper function to render a single interpolation point as XML.
		*
		* @param root XML element that will contain the point data.
		* @param point Interpolation point to be rendered.
		*/
		void renderResponseXml(const XmlElement& root, const interp::InterpPoint& point) const noexcept;

		/**
		* @brief Helper function to render a single interpolation point as CSV.
		*
		* @param of Output stream to write the CSV data to.
		* @param point Interpolation point to be rendered.
		*/
		void renderResponseCsv(std::ofstream& of, const interp::InterpPoint& point) const noexcept;
	};

	/**
	* @brief Compares the start times of two `Response` objects.
	*
	* Compares the start times of two `Response` objects held by unique pointers.
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
