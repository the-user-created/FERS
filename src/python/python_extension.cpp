/**
 * @file python_extension.cpp
 * @brief Implementation of Python extensions for integrating with FERS.
 *
 * @authors David Young, Marc Brooker
 * @date 2007-03-07
 */

#include "python_extension.h"

#include <Python.h>

#include "core/logging.h"
#include "math/geometry_ops.h"

using logging::Level;
using math::Vec3;
using math::SVec3;

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

			constexpr int major = PY_MAJOR_VERSION;
			constexpr int minor = PY_MINOR_VERSION;
			constexpr int micro = PY_MICRO_VERSION;

			const int version_value = getPythonVersionValue(major, minor, micro);

			const int min_version = getPythonVersionValue(3, 7, 0);
			const int max_version = getPythonVersionValue(3, 11, 13);

			if (const int incompatible_version = getPythonVersionValue(3, 12, 0); version_value >= incompatible_version) {
				throw std::runtime_error("Python version " + std::to_string(major) + "." +
										 std::to_string(minor) + "." + std::to_string(micro) +
										 " is incompatible with this program. Extensions will fail!");
			}

			if (version_value < min_version || version_value > max_version) {
				throw std::runtime_error("Python version " + std::to_string(major) + "." +
					std::to_string(minor) + "." + std::to_string(micro) +
					" is not supported. Please use a version between 3.7 and 3.11.12.");
			}

			LOG(Level::DEBUG, "Python version is within the supported range.");

			PyRun_SimpleString("import sys; sys.path.append('.')");
		}
	}

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

	Vec3 PythonPath::getPosition(const RealType t) const
	{
		PyObject* p_args = PyTuple_Pack(1, PyFloat_FromDouble(t));

		PyObject* p_value = PyObject_CallObject(_data->p_func, p_args);
		Py_DECREF(p_args);

		if (!p_value) {
			PyErr_Print();
			throw std::runtime_error("Error calling Python function for getPosition");
		}

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

	RealType PythonAntennaMod::getGain(const SVec3& direction) const
	{
		PyObject* p_args = PyTuple_Pack(2, PyFloat_FromDouble(direction.azimuth), PyFloat_FromDouble(direction.elevation));

		PyObject* p_value = PyObject_CallObject(_data->p_func, p_args);
		Py_DECREF(p_args);

		if (!p_value) {
			PyErr_Print();
			throw std::runtime_error("Error calling Python function for getGain");
		}

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
