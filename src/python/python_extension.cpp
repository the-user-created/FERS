// python_extension.cpp
// Implementation of Python extensions
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 7 March 2007

#include "python_extension.h"

#include <Python.h>

#include "core/logging.h"             // for log, LOG, Level
#include "math_utils/geometry_ops.h"  // for Vec3, SVec3

using logging::Level;
using math::Vec3;
using math::SVec3;

// Helper function to convert version components to a numeric value for comparison
int getPythonVersionValue(const int major, const int minor, const int micro) {
	return major * 10000 + minor * 100 + micro;
}

namespace python
{
	void initPython()
	{
		if (!Py_IsInitialized())
		{
			Py_Initialize();
			LOG(Level::DEBUG, "Using Python version {}", Py_GetVersion());
			// Get Python version details
			constexpr int major = PY_MAJOR_VERSION;
			constexpr int minor = PY_MINOR_VERSION;
			constexpr int micro = PY_MICRO_VERSION;

			// Convert the version to numeric value for easy comparison
			const int version_value = getPythonVersionValue(major, minor, micro);

			// Define the compatible version range (3.7.0 to 3.11.10)
			const int min_version = getPythonVersionValue(3, 7, 0);
			const int max_version = getPythonVersionValue(3, 11, 10);

			// Check if the version is outside the allowed range
			if (const int incompatible_version = getPythonVersionValue(3, 12, 0); version_value >= incompatible_version) {
				throw std::runtime_error("Python version " + std::to_string(major) + "." +
										 std::to_string(minor) + "." + std::to_string(micro) +
										 " is incompatible with this program. Extensions will fail!");
			}

			if (version_value < min_version || version_value > max_version) {
				throw std::runtime_error("Python version " + std::to_string(major) + "." +
					std::to_string(minor) + "." + std::to_string(micro) +
					" is not supported. Please use a version between 3.7 and 3.11.10.");
			}

			LOG(Level::DEBUG, "Python version is within the supported range.");

			PyRun_SimpleString("import sys; sys.path.append('.')");
		}
	}

	// =================================================================================================================
	//
	// PYTHON EXTENSION CLASS
	//
	// =================================================================================================================

	PythonExtension::PythonExtension(const std::string_view module, const std::string_view function)
		: _data(std::make_unique<PythonExtensionData>()), _module(module), _function(function)
	{
		initPython();

		_data->p_module = PyImport_ImportModule(_module.c_str());
		if (!_data->p_module) {
			PyErr_Print();
			throw std::runtime_error("Failed to load Python module: " + std::string(_module));
		}

		_data->p_func = PyObject_GetAttrString(_data->p_module, _function.c_str());
		if (!_data->p_func || !PyCallable_Check(_data->p_func)) {
			PyErr_Print();
			throw std::runtime_error("Failed to load Python function: " + std::string(_function));
		}
	}

	PythonExtension::~PythonExtension()
	{
		if (_data) {
			Py_XDECREF(_data->p_func);
			Py_XDECREF(_data->p_module);
		}
	}

	// =================================================================================================================
	//
	// PYTHON PATH CLASS
	//
	// =================================================================================================================

	Vec3 PythonPath::getPosition(const RealType t) const
	{
		// Prepare the argument list for the Python function
		PyObject* p_args = PyTuple_Pack(1, PyFloat_FromDouble(t));

		// Call the Python function
		PyObject* p_value = PyObject_CallObject(_data->p_func, p_args);
		Py_DECREF(p_args);

		if (!p_value) {
			PyErr_Print();
			throw std::runtime_error("Error calling Python function for getPosition");
		}

		// Assuming the Python function returns a tuple (x, y, z)
		Vec3 result;
		if (PyTuple_Check(p_value) && PyTuple_Size(p_value) == 3) {
			result.x = PyFloat_AsDouble(PyTuple_GetItem(p_value, 0));
			result.y = PyFloat_AsDouble(PyTuple_GetItem(p_value, 1));
			result.z = PyFloat_AsDouble(PyTuple_GetItem(p_value, 2));
		} else {
			PyErr_Print();
			Py_DECREF(p_value);
			throw std::runtime_error("Python function did not return a valid Vec3 tuple");
		}

		Py_DECREF(p_value);
		LOG(Level::TRACE, "PythonPath::getPosition: t={}, x={}, y={}, z={}", t, result.x, result.y, result.z);
		return result;
	}

	// =================================================================================================================
	//
	// PYTHON NOISE CLASS
	//
	// =================================================================================================================

	RealType PythonNoise::getSample() const
	{
		// Call the Python function with no arguments
		PyObject* p_value = PyObject_CallObject(_data->p_func, nullptr);

		if (!p_value) {
			PyErr_Print();
			throw std::runtime_error("Error calling Python function for getSample");
		}

		// Extract the result, assuming it is a float
		const RealType result = PyFloat_AsDouble(p_value);
		if (PyErr_Occurred()) {
			PyErr_Print();
			Py_DECREF(p_value);
			throw std::runtime_error("Error converting Python result to RealType");
		}

		Py_DECREF(p_value);
		LOG(Level::TRACE, "PythonNoise::getSample: {}", result);
		return result;
	}

	// =================================================================================================================
	//
	// PYTHON ANTENNA MOD CLASS
	//
	// =================================================================================================================

	RealType PythonAntennaMod::getGain(const SVec3& direction) const
	{
		// Prepare the argument list for the Python function
		PyObject* p_args = PyTuple_Pack(2, PyFloat_FromDouble(direction.azimuth), PyFloat_FromDouble(direction.elevation));

		// Call the Python function
		PyObject* p_value = PyObject_CallObject(_data->p_func, p_args);
		Py_DECREF(p_args);

		if (!p_value) {
			PyErr_Print();
			throw std::runtime_error("Error calling Python function for getGain");
		}

		// Extract the result, assuming it is a float
		const RealType result = PyFloat_AsDouble(p_value);
		if (PyErr_Occurred()) {
			PyErr_Print();
			Py_DECREF(p_value);
			throw std::runtime_error("Error converting Python result to RealType");
		}

		Py_DECREF(p_value);
		LOG(Level::TRACE, "PythonAntennaMod::getGain: {}, {} = {}", direction.azimuth, direction.elevation, result);
		return result;
	}
}
