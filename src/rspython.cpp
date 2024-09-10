//rspython.cpp - Implementation of Python extensions
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//7 March 2007


#include "rspython.h"

#include <Python.h>
#include <stdexcept>

#include "rsdebug.h"

using namespace rs_python;

/// Keep track of whether python has been globally initialized
bool initialized;


///Initialize the python environment
void rs_python::initPython()
{
	if (!initialized)
	{
		Py_Initialize();
		rs_debug::printf(rs_debug::RS_VERBOSE, "Using Python version %s\n", Py_GetVersion());
		PyRun_SimpleString("import sys; sys.path.append('.');");
		initialized = true;
	}
}


// Define the PythonExtensionImpl class
struct rs_python::PythonExtensionData
{
	PyObject *p_module, *p_func;
};

//
// PythonExtension Implementation
//


/// Constructor
PythonExtension::PythonExtension(const std::string& module, const std::string& function):
	_module(module),
	_function(function)
{
	_data = new PythonExtensionData;
	//Import the module
	PyObject* modname = PyUnicode_FromString(module.c_str());
	_data->p_module = PyImport_Import(modname);
	Py_DECREF(modname);

	if (!_data->p_module)
	{
		PyErr_Print();
		throw std::runtime_error("Could not import Python module " + module);
	}
	//Import the function
	PyObject* funcname = PyUnicode_FromString(function.c_str());
	_data->p_func = PyObject_GetAttr(_data->p_module, funcname);
	Py_DECREF(funcname);
	if (!_data->p_func)
	{
		PyErr_Print();
		throw std::runtime_error("Could not import Python function " + function + " from module " + module);
	}
	if (!PyCallable_Check(_data->p_func))
	{
		throw std::runtime_error("Python object " + function + " from module " + module + " is not callable");
	}
}

/// Destructor
PythonExtension::~PythonExtension()
{
	Py_DECREF(_data->p_func);
	Py_DECREF(_data->p_module);
	delete _data;
}

//
// PythonPath Implementation
//

/// Constructor
PythonPath::PythonPath(const std::string& module, const std::string& function):
	PythonExtension(module, function)
{
}

/// Destructor
PythonPath::~PythonPath()
{
}

/// Get the position at a given time
rs::Vec3 PythonPath::getPosition(const RS_FLOAT t) const
{
	//Insert t into a tuple for passing to the function
	PyObject* pargs = PyTuple_Pack(1, PyFloat_FromDouble(t));
	//Check that the Tuple was created successfully
	if (!pargs)
	{
		throw std::runtime_error("Could not create new Python Tuple in PythonPath");
	}
	//Call the function
	PyObject* result = PyObject_CallObject(_data->p_func, pargs);
	//Release the arguments
	Py_DECREF(pargs);
	//Check the call was completed
	if (!result)
	{
		PyErr_Print();
		throw std::runtime_error("Call of function " + _function + " from module " + _module + " failed");
	}
	//Unpack the results into Vec3
	PyObject* px = PyTuple_GetItem(result, 0);
	PyObject* py = PyTuple_GetItem(result, 1);
	PyObject* pz = PyTuple_GetItem(result, 2);
	if ((!px) || (!py) || (!pz))
	{
		PyErr_Print();
		throw std::runtime_error("Python function " + _function + " did not return expected tuple");
	}
	rs::Vec3 vec(PyFloat_AsDouble(px), PyFloat_AsDouble(py), PyFloat_AsDouble(pz));
	Py_DECREF(result);
	return vec;
}


//
//PythonNoise Implementation
//

///Constructor
PythonNoise::PythonNoise(const std::string& module, const std::string& function):
	PythonExtension(module, function)
{
}

///Destructor
PythonNoise::~PythonNoise()
{
}

/// Get a noise sample
RS_FLOAT PythonNoise::getSample() const
{
	//Call the function
	PyObject* result = PyObject_CallObject(_data->p_func, nullptr);
	//Check the call was completed
	if (!result)
	{
		PyErr_Print();
		throw std::runtime_error("Call of function " + _function + " from module " + _module + " failed");
	}
	//Unpack the results
	const RS_FLOAT sample = PyFloat_AsDouble(result);
	Py_DECREF(result);
	return sample;
}

//
// PythonAntennaMod Implementation
//

///Constructor
PythonAntennaMod::PythonAntennaMod(const std::string& module, const std::string& function):
	PythonExtension(module, function)
{
}

///Destructor
PythonAntennaMod::~PythonAntennaMod()
{
}

/// Get the antenna gain in the specified direction
RS_FLOAT PythonAntennaMod::getGain(const rs::SVec3& direction) const
{
	//Insert t into a tuple for passing to the function
	PyObject* pargs = PyTuple_Pack(2, PyFloat_FromDouble(direction.azimuth), PyFloat_FromDouble(direction.elevation));
	//Check that the Tuple was created successfully
	if (!pargs)
	{
		throw std::runtime_error("Could not create new Python Tuple in PythonPath");
	}
	//Call the function
	PyObject* result = PyObject_CallObject(_data->p_func, pargs);
	//Release the arguments
	Py_DECREF(pargs);
	//Check the call was completed
	if (!result)
	{
		PyErr_Print();
		throw std::runtime_error("Call of function " + _function + " from module " + _module + " failed");
	}
	//Translate the result
	const RS_FLOAT sample = PyFloat_AsDouble(result);
	Py_DECREF(result);
	return sample;
}
