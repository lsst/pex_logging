// -*- lsst-c++ -*-
/** \file    Trace.h
  *
  * \ingroup pex
  *
  * \brief  Class providing basic run-time trace facilities.
  *
  * \author Robert Lupton, Princeton University
  */

#if !defined(LSST_PEX_UTILS_TRACE_H)        //!< multiple inclusion guard macro
#define LSST_PEX_UTILS_TRACE_H 1

#include <iostream>
#include <string>
#include <sstream>
#include <cstdarg>
#include <boost/format.hpp>

#include "lsst/pex/logging/Trace.h"

namespace lsst {
namespace pex {
namespace logging {

#if !defined(LSST_NO_TRACE)
#  define LSST_NO_TRACE 0               //!< True => turn off all tracing
#endif

#if !defined(LSST_MAX_TRACE)
#  define LSST_MAX_TRACE -1             //!< Maximum level to trace (only works for TTrace)
#endif

/**
 * \brief  Class providing basic run-time trace facilities.
 *
 *      Tracing is controlled on a per "component" basis, where a "component" 
 *      is a name of the form aaa.bbb.ccc where aaa is the Most significant 
 *      part; for example, the utilities library might be called "utils", 
 *      the doubly-linked list "utils.dlist", and the code to destroy a 
 *      list "utils.dlist.del".
 *
 *      All tracing may be disabled by recompiling with LSST_NO_TRACE defined
 *      to be non-zero
 *
 * \see templated function TTrace, which can control the maximum acceptable
 * verbosity via the CPP symbol LSST_TRACE_MAX
 *
 * \see Component class for details on the verbosity tree which
 *      determines when a trace record will be emitted.
 */
class Trace {
public:
#if !LSST_NO_TRACE
    /**
     * Return a Trace object (which will later print if verbosity is high enough
     * for name) to which a message can be attached with <<
     */
    Trace(const std::string& name,      //!< Name of component
          const int verbosity           //!< Desired verbosity
         ) :
        _print(check_level(name, verbosity)), _verbosity(verbosity) {
        ;
    }

    /**
     * Print fmt if verbosity is high enough for name
     *
     * fmt is actually a printf format, so you can specify %d (etc) and provide
     * the required arguments.  This format will not actually be evaluated if
     * the trace is not active, so it's more efficient than the boost::format variant
     */
    Trace(const std::string& name,      //!< Name of component
          const int verbosity,          //!< Desired verbosity
          const std::string& fmt,       //!< Message to write as a printf format
          ...
         ) :
        _print(check_level(name, verbosity)), _verbosity(verbosity) {
        if (_print) {
            va_list ap;

            va_start(ap, fmt);
            const int len = vsnprintf(NULL, 0, fmt.c_str(), ap) + 1; // "+ 1" for the '\0'
            va_end(ap);

            char msg[len];
            va_start(ap, fmt);
            (void)vsnprintf(msg, len, fmt.c_str(), ap);
            va_end(ap);
            
            trace(msg, true);
        }
    }

    /**
     * Print fmt if verbosity is high enough for name
     *
     * fmt is actually a printf format, so you can specify %d (etc) and provide
     * the required arguments.  This format will not actually be evaluated if
     * the trace is not active, so it's more efficient than the boost::format variant
     *
     * As this routine indirectly calls va_arg the value of ap is undefined upon
     * exit;  the caller should however call va_end
     */
    Trace(const std::string& name,      //!< Name of component
          const int verbosity,          //!< Desired verbosity
          const std::string& fmt,       //!< Message to write as a printf format
          va_list ap                    //!< variable arguments
         ) :
        _print(check_level(name, verbosity)), _verbosity(verbosity) {
        if (_print) {
            const int len = fmt.size() + 100; // guess; we can't call vsnprintf twice to get length
            char msg[len];

            (void)vsnprintf(msg, len, fmt.c_str(), ap);
            
            trace(msg, true);
        }
    }

    /**
     * Print msg if verbosity is high enough for name
     */
    Trace(const std::string& name,      //!< Name of component
          const int verbosity,          //!< Desired verbosity
          const boost::format& msg      //!< Message to write
         ) :
        _print(check_level(name, verbosity)), _verbosity(verbosity) {
        if (_print) {
            trace(msg.str(), true);
        }
    }

    /**
      * Add to a trace record being emitted.
      *
      */
    template<typename T>
    Trace& operator<<(T v) {
        if (_print) {
            std::ostringstream s;
            s << v;
            trace(s.str());
        }
        return *this;           
    }

#else
    Trace(const std::string& name, const int verbosity) {}
    Trace(const std::string& name, const int verbosity,
          const std::string& msg, ...) {}
    Trace(const std::string& name, const int verbosity,
          const boost::format& msg) {}

    template<typename T>
    Trace& operator<<(T v) {
        return *this;
    }
#endif

    static void reset();

    static void setDestination(std::ostream &fp);
    static void setVerbosity(const std::string &name);
    static void setVerbosity(const std::string &name, const int verbosity);
    static int  getVerbosity(const std::string &name);
    static void printVerbosity(std::ostream &fp = std::cout);
private:
    bool _print;
    int _verbosity;

    bool check_level(const std::string& name, const int verbosity);
    void trace(const std::string& msg);
    void trace(const std::string& msg, const bool add_newline);
};

template<int VERBOSITY>
void TTrace(const char *name,           //!< Name of component
            const char *fmt,            //!< Message to write as a printf format
            ...
           ) {
    if (LSST_MAX_TRACE < 0 || VERBOSITY <= LSST_MAX_TRACE) {
        va_list ap;
        va_start(ap, fmt);
        Trace(name, VERBOSITY, fmt, ap);
        va_end(ap);
    }
}
       
template<int VERBOSITY>
void TTrace(const std::string& name,      //!< Name of component
            const std::string& fmt,       //!< Message to write as a printf format
            ...
           ) {
    if (LSST_MAX_TRACE < 0 || VERBOSITY <= LSST_MAX_TRACE) {
        va_list ap;
        va_start(ap, fmt);
        Trace(name, VERBOSITY, fmt, ap);
        va_end(ap);
    }
}
       

} // namespace logging
} // namespace pex
} // namespace lsst
#endif
