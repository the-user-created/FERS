/**
 * @file python_extension.h
 * @brief Python extensions for integrating with FERS.
 *
 * This header file provides the class definitions and functionality
 * needed to integrate Python extensions within the FERS simulation environment.
 * It includes initialization functions for the Python interpreter and classes that wrap Python modules and functions,
 * enabling C++ objects to interact with Python code for custom simulations.
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
	 * This function initializes the Python runtime environment, ensuring that the interpreter
	 * is ready for executing Python scripts and extensions. It verifies the Python version
	 * compatibility with the application.
	 *
	 * @throws std::runtime_error if the Python version is incompatible.
	 */
	void initPython();

	/**
	 * @struct PythonExtensionData
	 * @brief Holds Python module and function pointers.
	 *
	 * This structure stores pointers to the Python module and function that are loaded
	 * and called within the PythonExtension class.
	 */
	struct PythonExtensionData
	{
		/**
		 * @brief Pointer to the loaded Python module.
		 */
		PyObject *p_module = nullptr;

		/**
		 * @brief Pointer to the Python function.
		 */
		PyObject *p_func = nullptr;
	};

	/**
	 * @class PythonExtension
	 * @brief Base class for Python extensions in FERS.
	 *
	 * This class provides a wrapper around Python modules and functions, allowing
	 * integration of Python scripts into the FERS framework. It handles the loading
	 * of Python modules and functions and offers base functionality for derived classes
	 * to execute Python code.
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
		 *
		 * The destructor ensures that the Python module and function references are
		 * properly released when the object is destroyed.
		 */
		virtual ~PythonExtension();

	protected:
		/**
		 * @brief Stores the Python module and function pointers.
		 */
		std::unique_ptr<PythonExtensionData> _data;

		/**
		 * @brief Name of the Python module.
		 */
		std::string _module;

		/**
		 * @brief Name of the Python function.
		 */
		std::string _function;
	};

	/**
	 * @class PythonPath
	 * @brief Python extension for retrieving positions in space.
	 *
	 * This class extends the PythonExtension class to execute a Python function that
	 * returns a position in 3D space (Vec3) based on a time parameter. It is commonly used
	 * in path simulations where a dynamic position is calculated by Python.
	 */
	class PythonPath final : public PythonExtension
	{
	public:
		using PythonExtension::PythonExtension;

		/**
		 * @brief Retrieves the position at a given time.
		 *
		 * Calls the associated Python function to calculate the position in space
		 * at the specified time.
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
	 *
	 * This class extends PythonExtension to interact with Python functions that
	 * compute antenna gain based on the direction (azimuth and elevation). It is useful
	 * in simulations involving directional antennas where gain is dynamically calculated
	 * through Python.
	 */
	class PythonAntennaMod final : public PythonExtension
	{
	public:
		using PythonExtension::PythonExtension;

		/**
		 * @brief Calculates the antenna gain for a specific direction.
		 *
		 * Calls the associated Python function to compute the antenna gain based on
		 * the azimuth and elevation provided by the SVec3 direction vector.
		 *
		 * @param direction The direction (SVec3) for which the gain is calculated.
		 * @return The antenna gain (RealType) for the given direction.
		 * @throws std::runtime_error if the Python function call fails or returns an invalid result.
		 */
		[[nodiscard]] RealType getGain(const math::SVec3& direction) const;
	};
}
