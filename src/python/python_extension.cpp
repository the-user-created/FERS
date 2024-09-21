// python_extension.cpp
// Implementation of Python extensions
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 7 March 2007

#include "python_extension.h"

#include "core/logging.h"
#include "math_utils/geometry_ops.h"

using logging::Level;
using math::Vec3;
using math::SVec3;

bool initialized;

namespace python
{
	void initPython()
	{
		if (!initialized)
		{
			Py_Initialize();
			LOG(Level::DEBUG, "Using Python version {}", Py_GetVersion());
			PyRun_SimpleString("import sys; sys.path.append('.');");
			initialized = true;
		}
	}

	// =================================================================================================================
	//
	// PYTHON EXTENSION CLASS
	//
	// =================================================================================================================

	PythonExtension::PythonExtension(const std::string& module, const std::string& function)
		: _data(std::make_unique<PythonExtensionData>()), _module(module), _function(function)
	{
		PyObject* modname = PyUnicode_FromString(module.c_str());
		_data->p_module = PyImport_Import(modname);
		Py_DECREF(modname);

		if (!_data->p_module)
		{
			PyErr_Print();
			throw std::runtime_error("Could not import Python module " + module);
		}

		PyObject* funcname = PyUnicode_FromString(function.c_str());
		_data->p_func = PyObject_GetAttr(_data->p_module, funcname);
		Py_DECREF(funcname);

		if (!_data->p_func || !PyCallable_Check(_data->p_func))
		{
			PyErr_Print();
			throw std::runtime_error("Could not import or call Python function " + function + " from module " + module);
		}
	}

	PythonExtension::~PythonExtension()
	{
		Py_DECREF(_data->p_func);
		Py_DECREF(_data->p_module);
	}

	// =================================================================================================================
	//
	// PYTHON PATH CLASS
	//
	// =================================================================================================================

	Vec3 PythonPath::getPosition(const RS_FLOAT t) const
	{
		PyObject* pargs = PyTuple_Pack(1, PyFloat_FromDouble(t));
		if (!pargs) { throw std::runtime_error("Could not create new Python Tuple in PythonPath"); }

		PyObject* result = PyObject_CallObject(_data->p_func, pargs);
		Py_DECREF(pargs);
		if (!result)
		{
			PyErr_Print();
			throw std::runtime_error("Call of function " + _function + " from module " + _module + " failed");
		}

		const Vec3 vec(PyFloat_AsDouble(PyTuple_GetItem(result, 0)),
		               PyFloat_AsDouble(PyTuple_GetItem(result, 1)),
		               PyFloat_AsDouble(PyTuple_GetItem(result, 2)));
		Py_DECREF(result);
		return vec;
	}

	// =================================================================================================================
	//
	// PYTHON NOISE CLASS
	//
	// =================================================================================================================

	RS_FLOAT PythonNoise::getSample() const
	{
		PyObject* result = PyObject_CallObject(_data->p_func, nullptr);
		if (!result)
		{
			PyErr_Print();
			throw std::runtime_error("Call of function " + _function + " from module " + _module + " failed");
		}

		const RS_FLOAT sample = PyFloat_AsDouble(result);
		Py_DECREF(result);
		return sample;
	}

	// =================================================================================================================
	//
	// PYTHON ANTENNA MOD CLASS
	//
	// =================================================================================================================

	RS_FLOAT PythonAntennaMod::getGain(const SVec3& direction) const
	{
		PyObject* pargs = PyTuple_Pack(2, PyFloat_FromDouble(direction.azimuth),
		                               PyFloat_FromDouble(direction.elevation));
		if (!pargs) { throw std::runtime_error("Could not create new Python Tuple in PythonPath"); }

		PyObject* result = PyObject_CallObject(_data->p_func, pargs);
		Py_DECREF(pargs);
		if (!result)
		{
			PyErr_Print();
			throw std::runtime_error("Call of function " + _function + " from module " + _module + " failed");
		}

		const RS_FLOAT sample = PyFloat_AsDouble(result);
		Py_DECREF(result);
		return sample;
	}
}
