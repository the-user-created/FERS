/**
 * @file path.h
 * @brief Provides the definition and functionality of the Path class
 * for handling coordinate-based paths with different interpolation types.
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
		 * @param type The interpolation type (default is INTERP_STATIC).
		 */
		explicit Path(const InterpType type = InterpType::INTERP_STATIC) noexcept : _type(type) {}

		~Path() = default;
		Path(const Path&) = delete;
		Path(Path&&) = delete;
		Path& operator=(const Path&) = delete;
		Path& operator=(Path&&) = delete;

		/**
		 * @brief Adds a coordinate to the path.
		 *
		 * @param coord The coordinate to be added.
		 */
		void addCoord(const Coord& coord) noexcept;

		/**
		 * @brief Finalizes the path, preparing it for interpolation.
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
		 * @param t The time parameter at which to get the position.
		 * @return The interpolated position as a Vec3 object.
		 * @throws PathException If finalize() has not been called before this method.
		 * @throws std::logic_error If using Python interpolation and the Python module has not been loaded.
		 */
		[[nodiscard]] Vec3 getPosition(RealType t) const;

		/**
		 * @brief Sets the Python path module and function for Python-based interpolation.
		 *
		 * @param modname The name of the Python module.
		 * @param pathname The name of the Python function or path.
		 */
		void setPythonPath(std::string_view modname, std::string_view pathname);

		/**
		 * @brief Changes the interpolation type.
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
	 * @param path The original path to be reflected.
	 * @param surf The surface to reflect the path across.
	 * @return A unique pointer to the new reflected path.
	 * @throws std::runtime_error If the path uses Python-based interpolation.
	 */
	std::unique_ptr<Path> reflectPath(const Path* path, const MultipathSurface* surf);
}
