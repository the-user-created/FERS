// python_extension.h
// Objects for extending FERS with Python
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 7 March 2007

#ifndef PYTHON_EXTENSION_H
#define PYTHON_EXTENSION_H

#include <memory>
#include <Python.h>
#include <string>

#include "config.h"
#include "math_utils/geometry_ops.h"

namespace rs_python
{
	void initPython();

	struct PythonExtensionData
	{
		PyObject *p_module, *p_func;
	};

	class PythonExtension
	{
	public:
		PythonExtension(const std::string& module, const std::string& function);

		~PythonExtension();

	protected:
		std::unique_ptr<PythonExtensionData> _data;
		std::string _module;
		std::string _function;
	};

	class PythonPath : public PythonExtension
	{
	public:
		PythonPath(const std::string& module, const std::string& function) : PythonExtension(module, function)
		{
		}

		~PythonPath() = default;

		[[nodiscard]] rs::Vec3 getPosition(RS_FLOAT t) const;
	};

	class PythonNoise : public PythonExtension
	{
	public:
		PythonNoise(const std::string& module, const std::string& function) : PythonExtension(module, function)
		{
		}

		~PythonNoise() = default;

		[[nodiscard]] RS_FLOAT getSample() const;
	};

	class PythonAntennaMod : public PythonExtension
	{
	public:
		PythonAntennaMod(const std::string& module, const std::string& function) : PythonExtension(module, function)
		{
		}

		~PythonAntennaMod() = default;

		[[nodiscard]] RS_FLOAT getGain(const rs::SVec3& direction) const;
	};
}

#endif
