/**
 * @file python_extension.h
 * @brief Python extensions for integrating with FERS.
 *
 * @authors David Young, Marc Brooker
 * @date 2007-03-07
 */

#pragma once

#include <memory>
#include <pytypedefs.h>
#include <string>
#include <string_view>

#include "config.h"

namespace math
{
	class Vec3;
	class SVec3;
}

namespace python
{
	/**
	 * @brief Initializes the Python interpreter if it is not already running.
	 *
	 * @throws std::runtime_error if the Python version is incompatible.
	 */
	void initPython();

	/**
	 * @struct PythonExtensionData
	 * @brief Holds Python module and function pointers.
	 */
	struct PythonExtensionData
	{
		PyObject* p_module = nullptr; ///< Pointer to the loaded Python module.

		PyObject* p_func = nullptr; ///< Pointer to the Python function.
	};

	/**
	 * @class PythonExtension
	 * @brief Base class for Python extensions in FERS.
	 */
	class PythonExtension
	{
	public:
		/**
		 * @brief Constructs a PythonExtension by loading the specified module and function.
		 *
		 * @param module Name of the Python module to load.
		 * @param function Name of the Python function to call.
		 *
		 * @throws std::runtime_error if the module or function cannot be loaded.
		 */
		PythonExtension(std::string_view module, std::string_view function);

		/**
		 * @brief Virtual destructor to clean up Python resources.
		 */
		virtual ~PythonExtension();

		PythonExtension(const PythonExtension&) = delete;
		PythonExtension& operator=(const PythonExtension&) = delete;
		PythonExtension(PythonExtension&&) = delete;
		PythonExtension& operator=(PythonExtension&&) = delete;

	protected:
		std::unique_ptr<PythonExtensionData> _data; ///< Stores the Python module and function pointers.

		std::string _module; ///< Name of the Python module.

		std::string _function; ///< Name of the Python function.
	};

	/**
	 * @class PythonPath
	 * @brief Python extension for retrieving positions in space.
	 */
	class PythonPath final : public PythonExtension
	{
	public:
		using PythonExtension::PythonExtension;

		/**
		 * @brief Retrieves the position at a given time.
		 *
		 * @param t The time parameter for which the position is calculated.
		 * @return The position (Vec3) at the given time.
		 * @throws std::runtime_error if the Python function call fails or returns an invalid result.
		 */
		[[nodiscard]] math::Vec3 getPosition(RealType t) const;
	};

	/**
	 * @class PythonAntennaMod
	 * @brief Python extension for antenna gain modulation.
	 */
	class PythonAntennaMod final : public PythonExtension
	{
	public:
		using PythonExtension::PythonExtension;

		/**
		 * @brief Calculates the antenna gain for a specific direction.
		 *
		 * @param direction The direction (SVec3) for which the gain is calculated.
		 * @return The antenna gain (RealType) for the given direction.
		 * @throws std::runtime_error if the Python function call fails or returns an invalid result.
		 */
		[[nodiscard]] RealType getGain(const math::SVec3& direction) const;
	};
}
