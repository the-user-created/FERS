// python_extension.h
// Objects for extending FERS with Python
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 7 March 2007

#pragma once

#include <memory>        // for unique_ptr
#include <pytypedefs.h>  // for PyObject
#include <string>        // for string
#include <string_view>   // for string_view

#include "config.h"      // for RealType

namespace math
{
	class Vec3;
	class SVec3;
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
