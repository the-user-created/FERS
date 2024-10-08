/**
 * @file path.h
 * @brief Provides the definition and functionality of the Path class
 * for handling coordinate-based paths with different interpolation types.
 *
 * This header file defines the Path class,
 * which manages a series of coordinates and allows for different interpolation methods such as
 * static, linear, cubic, and Python-based interpolation.
 * The class also supports reflecting paths across surfaces.
 *
 * @authors David Young, Marc Brooker
 * @date 2024-09-13
 */

#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "config.h"
#include "coord.h"
#include "python/python_extension.h"

namespace math
{
	class MultipathSurface;
	class Vec3;

	/**
	 * @class Path
	 * @brief Represents a path with coordinates and allows for various interpolation methods.
	 *
	 * The Path class manages a sequence of coordinates
	 * and enables interpolation to calculate positions at different points in time.
	 * It supports static, linear, cubic, and Python-based interpolation types,
	 * as well as the ability to reflect paths across surfaces.
	 * The path can be finalized to prepare for interpolation,
	 * and various operations such as adding coordinates and setting Python-based paths are supported.
	 */
	class Path
	{
	public:
		/**
		 * @brief Types of interpolation supported by the Path class.
		 */
		enum class InterpType { INTERP_STATIC, INTERP_LINEAR, INTERP_CUBIC, INTERP_PYTHON };

		/**
		 * @brief Constructs a Path object with a specified interpolation type.
		 *
		 * Initializes the Path object with the provided interpolation type.
		 * Defaults to INTERP_STATIC if no type is specified.
		 *
		 * @param type The interpolation type (default is INTERP_STATIC).
		 */
		explicit Path(const InterpType type = InterpType::INTERP_STATIC) noexcept : _type(type) {}

		/**
		 * @brief Destructor for the Path class.
		 *
		 * Default destructor.
		 */
		~Path() = default;

		/**
		 * @brief Adds a coordinate to the path.
		 *
		 * Adds a coordinate to the path and ensures that the coordinates remain sorted in time (t).
		 * This also invalidates the finalization state, requiring a new call to finalize().
		 *
		 * @param coord The coordinate to be added.
		 */
		void addCoord(const Coord& coord) noexcept;

		/**
		 * @brief Finalizes the path, preparing it for interpolation.
		 *
		 * Prepares the path for interpolation based on the selected interpolation type.
		 * Must be called before using getPosition().
		 */
		void finalize();

		// [[nodiscard]] python::PythonPath* getPythonPath() const noexcept { return _pythonpath.get(); }
		/**
		 * @brief Retrieves the current interpolation type of the path.
		 *
		 * @return The interpolation type of the path.
		 */
		[[nodiscard]] InterpType getType() const noexcept { return _type; }

		/**
		 * @brief Gets the list of coordinates in the path.
		 *
		 * @return A constant reference to the vector of coordinates.
		 */
		[[nodiscard]] const std::vector<Coord>& getCoords() const noexcept { return _coords; }

		/**
		 * @brief Retrieves the position at a given time along the path.
		 *
		 * Computes and returns the interpolated position at the specified time t, based on the current interpolation type.
		 *
		 * @param t The time parameter at which to get the position.
		 * @return The interpolated position as a Vec3 object.
		 * @throws PathException If finalize() has not been called before this method.
		 * @throws std::logic_error If using Python interpolation and the Python module has not been loaded.
		 */
		[[nodiscard]] Vec3 getPosition(RealType t) const;

		/**
		 * @brief Sets the Python path module and function for Python-based interpolation.
		 *
		 * Assigns a Python module and function to be used for Python-based interpolation. This must be done before attempting
		 * to use Python-based interpolation.
		 *
		 * @param modname The name of the Python module.
		 * @param pathname The name of the Python function or path.
		 */
		void setPythonPath(std::string_view modname, std::string_view pathname);

		/**
		 * @brief Changes the interpolation type.
		 *
		 * Sets a new interpolation type and invalidates the finalization state, requiring a new call to finalize().
		 *
		 * @param settype The new interpolation type to be used.
		 */
		void setInterp(InterpType settype) noexcept;

	private:
		std::vector<Coord> _coords; ///< The list of coordinates in the path.
		std::vector<Coord> _dd; ///< The list of second derivatives for cubic interpolation.
		bool _final{false}; ///< Flag indicating whether the path has been finalized.
		InterpType _type; ///< The current interpolation type of the path.
		std::unique_ptr<python::PythonPath> _pythonpath{nullptr};
		///< The Python path module for Python-based interpolation.
	};

	/**
	 * @brief Reflects a path across a surface.
	 *
	 * Creates a new path by reflecting the coordinates of an existing path across a given MultipathSurface.
	 * Python-based paths are not supported for reflection.
	 *
	 * @param path The original path to be reflected.
	 * @param surf The surface to reflect the path across.
	 * @return A unique pointer to the new reflected path.
	 * @throws std::runtime_error If the path uses Python-based interpolation.
	 */
	std::unique_ptr<Path> reflectPath(const Path* path, const MultipathSurface* surf);
}
