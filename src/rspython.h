//rspython.h - Objects for extending FERS with Python
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//7 March 2007

#ifndef RS_PYTHON_H
#define RS_PYTHON_H

#include <string>

#include "config.h"
#include "rsgeometry.h"

namespace rs_python
{
	///Function which initializes python
	void initPython();

	///Class which implements Python functionality
	struct PythonExtensionData;

	///Parent class for all python calculation classes
	class PythonExtension
	{
	public:
		/// Constructor
		PythonExtension(const std::string& module, const std::string& function);

		/// Destructor
		~PythonExtension();

	protected:
		PythonExtensionData* _data; //!< Pointer to python specific data
		std::string _module; //!< Name of the module
		std::string _function; //!< Name of the function
	};

	///Python extension for calculating paths
	class PythonPath : public PythonExtension
	{
	public:
		/// Constructor
		PythonPath(const std::string& module, const std::string& function);

		/// Destructor
		~PythonPath();

		/// Get the position at the given time
		[[nodiscard]] rs::Vec3 getPosition(RS_FLOAT t) const;
	};

	/// Python extension for generating noise
	class PythonNoise : public PythonExtension
	{
	public:
		///Constructor
		PythonNoise(const std::string& module, const std::string& function);

		///Destructor
		~PythonNoise();

		/// Get a noise sample
		[[nodiscard]] RS_FLOAT getSample() const;
	};

	/// Python extension for generating noise
	class PythonAntennaMod : public PythonExtension
	{
	public:
		///Constructor
		PythonAntennaMod(const std::string& module, const std::string& function);

		///Destructor
		~PythonAntennaMod();

		/// Get a noise sample
		[[nodiscard]] RS_FLOAT getGain(const rs::SVec3& direction) const;
	};
}

#endif
