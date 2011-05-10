/*!
 * @file exception.h
 * @brief Exception declarations.
 *
 * @author Piotr Trojanek <piotr.trojanek@gmail.com>
 * @author Tomasz Winiarski <tomrobotics@gmail.com>
 *
 * @ingroup LIB
 */

#ifndef __TRANSFORMER_ERROR_H
#define __TRANSFORMER_ERROR_H

#include <stdint.h>
#include <boost/exception/all.hpp>
#include <boost/exception/diagnostic_information.hpp>
//#include <exception>

namespace mrrocpp {
namespace lib {
namespace exception {

//! Description used for diagnostic information in case of system errors.
//const char * SYSTEM_ERROR = "SYSTEM ERROR";

//! Description used for diagnostic information in case of fatal errors.
//const char * FATAL_ERROR = "FATAL ERROR";

//! Description used for diagnostic information in case of non fatal errors.
//const char * NON_FATAL_ERROR = "NON FATAL ERROR";

//! MRROC++ error class - by default three types are denoted (SYSTEM, FATAL, NON-FATAL).
typedef boost::error_info <struct mrrocpp_error_class_, char const *> mrrocpp_error_class;

//! Description of the MRROC++ error - it will be sent (by default) to the SR.
typedef boost::error_info <struct mrrocpp_error_description_, char const *> mrrocpp_error_description;

//#define MRROCPP_THROW_SYSTEM_EXCEPTION(EXCEPTION) BOOST_THROW_EXCEPTION(EXCEPTION<<mrrocpp_error_class(SYSTEM_ERROR))
//#define MRROCPP_THROW_FATAL_EXCEPTION(EXCEPTION) BOOST_THROW_EXCEPTION(EXCEPTION<<mrrocpp_error_class(FATAL))
//#define MRROCPP_THROW_NON_FATAL_EXCEPTION(EXCEPTION) BOOST_THROW_EXCEPTION(EXCEPTION<<mrrocpp_error_class(NON_FATAL_ERROR))

/*!
 * \brief Base class for all system errors.
 * \author tkornuta
 */
struct mrrocpp_system_error : virtual public std::exception, virtual public boost::exception
{
	mrrocpp_system_error()
	{
		*this << mrrocpp_error_class("SYSTEM ERROR");
	}

	virtual const char* what() const throw ()
	{
		return diagnostic_information_what(*this);
	}

	~mrrocpp_system_error() throw ()
	{
	}
};

/*!
 * \brief Base class for all fatal errors.
 * \author tkornuta
 */
struct mrrocpp_fatal_error : virtual public std::exception, virtual public boost::exception
{
	mrrocpp_fatal_error()
	{
		*this << mrrocpp_error_class("FATAL ERROR");
	}

	virtual const char* what() const throw ()
	{
		return diagnostic_information_what(*this);
	}

	~mrrocpp_fatal_error() throw ()
	{
	}
};

/*!
 * \brief Base class for all non fatal errors.
 * \author tkornuta
 */
struct mrrocpp_non_fatal_error : virtual public std::exception, virtual public boost::exception
{
	mrrocpp_non_fatal_error()
	{
		*this << mrrocpp_error_class("NON FATAL ERROR");
	}

	virtual const char* what() const throw ()
	{
		return diagnostic_information_what(*this);
	}

	~mrrocpp_non_fatal_error() throw ()
	{
	}
};

/**
 * System error (inter-process communication, filesystem, etc.)
 */
class System_error
{
};

/**
 * Fatal exception in framework or application
 */
class Fatal_error
{
public:
	//! Servo error number (1)
	const uint64_t error0;

	//! Servo error number (2)
	const uint64_t error1;

	/**
	 * Constructor
	 * @param err_no_0 servo error number (1)
	 * @param err_no_1 servo error number (2)
	 * @return
	 */
	Fatal_error(uint64_t err_no_0, uint64_t err_no_1);
};

/**
 * Non-fatal errors (type 1)
 * @author Tomasz Winiarski <tomrobotics@gmail.com>
 */
class NonFatal_error_1
{
public:
	//! Error in coordinate calculations
	const uint64_t error;

	/**
	 * Constructor
	 * @param err_no error value
	 */
	NonFatal_error_1(uint64_t err_no);
};

/**
 * Non-fatal errors (type 2)
 * @author Tomasz Winiarski <tomrobotics@gmail.com>
 */
class NonFatal_error_2
{
public:
	//! Error in coordinate calculations
	const uint64_t error;

	/**
	 * Constructor
	 * @param err_no error value
	 */
	NonFatal_error_2(uint64_t err_no);
};

/**
 * Non-fatal errors (type 3)
 * @author Tomasz Winiarski <tomrobotics@gmail.com>
 */
class NonFatal_error_3
{
public:
	//! Error in coordinate calculations
	const uint64_t error;

	/**
	 * Constructor
	 * @param err_no error value
	 */
	NonFatal_error_3(uint64_t err_no);
};

/**
 * Non-fatal errors (type 4)
 * @author Tomasz Winiarski <tomrobotics@gmail.com>
 */
class NonFatal_error_4
{
public:
	//! Error in coordinate calculations
	const uint64_t error;

	/**
	 * Constructor
	 * @param err_no error value
	 */
	NonFatal_error_4(uint64_t err_no);
};

} // namespace exception
} // namespace common
} // namespace mrrocpp

#endif
