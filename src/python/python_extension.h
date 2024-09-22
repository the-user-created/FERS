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

namespace math
{
	struct Vec3;
	struct SVec3;
}

namespace python
{
	void initPython();

	struct PythonExtensionData
	{
		PyObject *p_module = nullptr;
		PyObject *p_func = nullptr;
	};

	class PythonExtension
	{
	public:
		PythonExtension(std::string_view module, std::string_view function);

		virtual ~PythonExtension();

	protected:
		std::unique_ptr<PythonExtensionData> _data;
		std::string _module;
		std::string _function;
	};

	class PythonPath final : public PythonExtension
	{
	public:
		using PythonExtension::PythonExtension;

		[[nodiscard]] math::Vec3 getPosition(RealType t) const;
	};

	// NOTE: This class is not used in the current version of FERS
	class PythonNoise final : public PythonExtension
	{
	public:
		using PythonExtension::PythonExtension;

		[[nodiscard]] RealType getSample() const;
	};

	class PythonAntennaMod final : public PythonExtension
	{
	public:
		using PythonExtension::PythonExtension;

		[[nodiscard]] RealType getGain(const math::SVec3& direction) const;
	};
}

#endif
