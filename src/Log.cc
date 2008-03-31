//////////////////////////////////////////////////////////////////////////////
// Log.cc
//
// Contact: Ray Plante
// 
//////////////////////////////////////////////////////////////////////////////

#include "lsst/pex/logging/Log.h"
#include "lsst/pex/logging/ScreenLog.h"
#include "lsst/pex/logging/Trace.h"

#include <boost/shared_ptr.hpp>

using namespace std;

// #define EXEC_TRACE  20
// static void execTrace( string s, int level = EXEC_TRACE ){
//     lsst::pex::logging::Trace( "pex.logging.Log", level, s );
// }

namespace lsst {
namespace pex {
namespace logging {

using boost::shared_ptr;

///////////////////////////////////////////////////////////
//  Log
///////////////////////////////////////////////////////////

/**
 * the conventional verbosity level for messages that aid in debugging.  
 * This value is set to -10 with the intention that messages with 
 * negative verbosity levels (or more precisely, >= -10) will be printed 
 * when the threshold is set to this value.  
 */
const int Log::DEBUG = -10;

/**
 * the conventional verbosity level for messages that are informational
 * and which report on normal behavior.  The value is set to 0 with the 
 * intention that this is the default threshold for logs and their 
 * destinations.  
 */
const int Log::INFO = 0;

/**
 * the conventional verbosity level for messages that warn about 
 * abnormal but non-fatal behavior.  The value is set to 10.
 */
const int Log::WARN = 10;

/**
 * the conventional verbosity level for messages that report on fatal
 * behavior.  The value is set to 20.  Note that the logging module 
 * makes no attempt to shutdown execution or other wise affect control 
 * flow when a message having a verbosity exceeding this level.  
 */
const int Log::FATAL = 20;

/**
 * a magic threshold value that indicates that a threshold of a Log
 * should be set to its nearest ancestor
 */
const int Log::INHERIT_THRESHOLD = Component::INHERIT_VERBOSITY;

/**
 * create a null log.  This constructor should 
 * not normally be employed to obtain a Log; the static getDefaultLog() 
 * method should be used instead.  This is provided primarily for 
 * subclasses and containers that require a no-arg constructor.  
 * @param threshold   the initial verbosity threshold for this log
 * @param name        the initial name for this log.  An empty string 
 *                    (the default) denotes a root log.
 */
Log::Log(const int threshold, const string& name) 
    : _threshold(threshold), _name(name), _thresholds(new Component("", INFO)), 
      _destinations(), _preamble()
{
    completePreamble();
}

/**
 * create a fully configured Log.  This constructor is 
 * not normally employed to obtain a Log; the static getDefaultLog() 
 * method or the createChildLog() method of should be used instead.  
 * @param destinations   the list of LogDestinations to attach to this 
 *                         Log.
 * @param preamble       a list of data properties that should be included 
 *                         with every recorded message to the Log.  This
 *                         constructor will automatically a property ("LOG")
 *                         giving the Log name.  
 * @param name           the name to give this log.  By default, the 
 *                         name will be an empty string, signifying a 
 *                         root log.  
 * @param threshold      the verbosity threshold to assign to this Log.  
 *                         Messages sent to this log must have a verbosity
 *                         level equal to or greater than this value to 
 *                         be recorded.  (Note that thresholds associated
 *                         with destinations have their own thresholds that
 *                         will override this one.)
 */
Log::Log(const list<shared_ptr<LogDestination> > &destinations, 
         const list<shared_ptr<DataProperty> > &preamble,
         const string &name, const int threshold)
    : _threshold(threshold), _name(name), 
      _thresholds(new Component(name, threshold)), 
      _destinations(destinations), _preamble(preamble)
{  
    completePreamble();
}

/**
 * create a copy
 */
Log::Log(const Log& that) 
    : _threshold(that._threshold), _name(that._name), 
      _thresholds(that._thresholds),
      _destinations(that._destinations), _preamble(that._preamble)
{ }

/** 
 * delete this Log
 */
Log::~Log() { }

/**
 * create a copy
 */
Log& Log::operator=(const Log& that) {
    _threshold = that._threshold; 
    _name = that._name;
    _thresholds = that._thresholds;
    _destinations = that._destinations;
    _preamble = that._preamble;
    return *this;
}

void Log::completePreamble() {
    shared_ptr<DataProperty> nameprop(new DataProperty("LOG", _name));

    LogRecord::DataListT::iterator i;
    for(i = _preamble.begin(); i != _preamble.end(); i++) {
        if ((*i)->getName() == "LOG") {
            *i = nameprop;
            return;
        }
    }
    _preamble.push_back( nameprop );
}

/**
 * set a DataProperty to the preamble, overwriting any DataProperty with 
 * the same name.
 */
void Log::setPreambleProperty(const DataProperty& dp) {
    _preamble.remove_if(DataPropertyNameEquals(dp.getName())); 
    addPreambleProperty(dp);
}

/**
 * create a child of a given Log
 * @param parent        the Log that will serve as its parent.  
 * @param childName     the name of the child log, relative to the given 
 *                          parent.  The full child name will be 
 *                          constructed from this name prepended by the 
 *                          parent's name and a ".".  If the parent Log 
 *                          is a root Log, then the "." is not prepended.
 * @param threshold     the threshold for the new child Log.  The default
 *                          value is Log::INHERIT_THRESHOLD which means that 
 *                          it will be set to the threshold of the parent.  
 */
Log::Log(const Log& parent, const string& childName, int threshold)
    : _threshold(threshold), _name(parent.getName()), 
      _thresholds(parent._thresholds), _destinations(parent._destinations), 
      _preamble(parent._preamble)
{ 
    if (_name.length() > 0) _name += _sep;
    _name += childName;

    if (_threshold > INHERIT_THRESHOLD) 
        _thresholds->add(_name, _threshold, _sep);

    completePreamble();
}

/**
 * set the verbosity threshold for a child Log.  When a child Log of the
 * same name is created, it will be assigned this threshold.  Any existing
 * Log object with name that has already explicitly set its verbosity
 * threshold will not be affected; however, those that are set to inherit
 * the threshold will be. 
 * @param name       the relative name of the child log.  This cannot be an
 *                      empty string.  
 * @param threshold  the verbosity threshold to set Logs with this name to.
 */
void Log::setThresholdFor(const string& name, int threshold) const {
    string fullname = getName();
    if (fullname.length() > 0) fullname += _sep;
    fullname += name;
    _thresholds->add(fullname, threshold, _sep);
}

/**
 * create a child Log.  A child Log is a Log that is attached to a sub
 * channel of this log.  Its full name will be formed from this parent's 
 * name followed by a dot-delimiter (".") followed by the childName 
 * parameter (e.g. "parentName.childName"), unless the parent is the 
 * root Log (whose name is an empty string), in which case the full child
 * name will just be the value of childName.  
 *
 * This method will create a new Log object and return it as a pointer.  
 * The caller is responsible for deleting the object.
 * 
 * If the threshold for the child Log is not provided, it is set to the 
 * threshold of this Log, its parent.  
 *
 * If the threshold for the child Log is not provided, it is set to the 
 * threshold the channel had the last it was accessed.  If this is the 
 * first time the channel will be used it will be set to the current 
 * threshold of its parent.  
 * 
 * @param childName   the name of the new child log relative to this one.
 *                    The full name of the child log will be the parent's 
 *                    name followed by a "." (if the length of the parent's 
 *                    name is non-zero), followed by the given child name. 
 *                    This cannot be an empty string.  
 * @param threshold   the verbosity threshold to set for this Log.  If not 
 *                    provided, it will default to the threshold of this 
 *                    log, its parent.  
 */
Log *Log::createChildLog(const string& childName, int threshold) const {
    return new Log(*this, childName, threshold);
}

/**
 * send a message to the log
 * @param verbosity    how loud the message should be
 * @param message      a simple bit of text to send in the message
 * @param properties   a list of properties to include in the message.
 */
void Log::log(int verbosity, const string& message, 
              const list<shared_ptr<DataProperty> >& properties) 
{
    int threshold = getThreshold();
    if (verbosity < threshold)
        return;
    LogRecord rec(threshold, verbosity, _preamble);
    rec.addComment(message);
    for(LogRecord::DataIteratorT i=properties.begin(); 
        i != properties.end();
        i++)
    {
        rec.addProperty(*i);
    }
    send(rec);
}

/**
 * send a message to the log
 * @param verbosity    how loud the message should be
 * @param message      a simple bit of text to send in the message
 * @param properties   a list of properties to include in the message.
 */
void Log::log(int verbosity, const string& message, const DataProperty& prop) {

    int threshold = getThreshold();
    if (verbosity < threshold)
        return;
    LogRecord rec(threshold, verbosity, _preamble);
    rec.addComment(message);
    rec.addProperty(prop);
    send(rec);
}

/**
 * send a simple message to the log
 * @param verbosity    how loud the message should be
 * @param message      a simple bit of text to send in the message
 */
void Log::log(int verbosity, const string& message) {
    int threshold = getThreshold();
    if (verbosity < threshold)
        return;
    LogRecord rec(threshold, verbosity, _preamble);
    rec.addComment(message);
    send(rec);
}

/**
 * send a fully formed LogRecord to the log destinations
 */
void Log::send(const LogRecord& record) {
    if (record.getVerbosity() < getThreshold()) 
        return;
    list<shared_ptr<LogDestination> >::iterator i;
    for(i = _destinations.begin(); i != _destinations.end(); i++) {
        (*i)->write(record);
    }
}

/**
 * add a destination to this log.  The destination stream will included
 * in all child Logs created from this log after a call to this function.
 * All previously created logs, including ancestor logs, will be 
 * unaffected.  The NetLogger format will be used with this new 
 * destination.  
 * @param destination   the stream to send messages to
 * @param threshold     the verbosity threshold to use to filter messages
 *                         sent to the stream.
 */
void Log::addDestination(ostream& destination, int threshold) {
    shared_ptr<LogFormatter> frmtr(new NetLoggerFormatter());
    addDestination(destination, threshold, frmtr);
}

/**
 * add a destination to this log.  The destination stream will included
 * in all child Logs created from this log after a call to this function.
 * All previously created logs, including ancestor logs, will be 
 * unaffected.  
 * @param destination   a pointer to the stream to send messages to.  The
 *                         caller is responsible for ensuring that the 
 *                         stream is neither closed nor its memory freed
 *                         for the life of this Log.  
 * @param threshold     the verbosity threshold to use to filter messages
 *                         sent to the stream.
 * @param formatter     the log formatter to use.
 */
void Log::addDestination(ostream& destination, int threshold, 
                         const shared_ptr<LogFormatter> &formatter) 
{
    shared_ptr<LogDestination> dest( 
                new LogDestination(&destination, formatter, threshold));
    addDestination(dest);
}

const Log& Log::getDefaultLog() {
    if (defaultLog == 0) defaultLog = new ScreenLog();
    return *defaultLog;
}

void Log::setDefaultLog(Log *deflog) {
    if (defaultLog != 0) delete defaultLog;
    defaultLog = deflog;
}

void Log::createDefaultLog(const list<shared_ptr<LogDestination> >& dests, 
                           const list<shared_ptr<DataProperty> >& preamble,
                           const string& name, const int threshold)
{
    Log::setDefaultLog(new Log(dests, preamble, name, threshold));
}

void Log::closeDefaultLog()  {  setDefaultLog(0);  }

Log *Log::defaultLog = 0;
const string Log::_sep(".");

LogRec::~LogRec() {
    if (! _sent) *this << endr;
}

// this used to be inlined but it was causing problems in SWIG/Python-land.
// I'm not sure that the problem has really been solved by un-inlining it 
// or just hidden it away.  
LogRec& LogRec::operator<<(const string& comment) {
    addComment(comment);
    return *this;
}

}}} // end lsst::pex::logging

