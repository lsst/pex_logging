// -*- lsst-c++ -*-
/**
  * \file logExample.cc
  *
  * \brief logExample.c demonstrates simple use of the Log facility.
  */

#include "lsst/pex/logging/Log.h"
#include "lsst/pex/logging/DualLog.h"
#include <fstream>

using namespace std;
using namespace lsst::pex::logging;
using namespace lsst;
using lsst::pex::logging::Log;
using lsst::pex::logging::Rec;

/** \brief Demonstrate the Log class.   
  *
  * See the documentation for the Log class for more information and example
  * usage.  
  */
int main(int argc, char *argv[]) {

    // in some function where you want to log a message, the first 
    // thing you should do is create a Log from the DefaultLog.  (In this 
    // example, the message verbosity threshold is at the INFO.)
    Log mylog(Log::getDefaultLog(), "myapp.myfunc");

    // for simple messages just use the log function.  The first argument is
    // the level of message; choices are: DEBUG, INFO, WARN, FATAL.  If the 
    // versbosity threshold is not low enough, this message will be ignored.
    mylog.log(Log::INFO, "I'm writing a message.");

    // you can use the boost::format object for more complex messages; 
    // however, be careful: you still pay the cost of the format call
    mylog.log(Log::INFO, 
              boost::format("Verbosity threshold: %d") % mylog.getThreshold());

    // If you want to send multiple messages and/or properties all in the
    // same message, you can use the shift operator.  Be sure to end the
    // message with Rec::endr
    Rec(mylog, Log::WARN) << "No convergence reached"
                          << Prop<int>("iterations", 541)
                          << Prop<float>("rms", 0.0032)
                          << Rec::endr;

    // Normally properties are not printed to the screen.  To see these, we'll
    // turn them on now.
    //
    // Outside the pipeline framework, the default logger is a ScreenLog.
    // If you want to see properties on the screen, you put this code anywhere
    // in your python script.  Note that the effect is global.
    try {
        ScreenLog slog = dynamic_cast<const ScreenLog&>(Log::getDefaultLog());
        slog.setScreenVerbose(true);
    }
    catch (bad_cast &e) { }

    // now try the complex message again
    Rec(mylog, Log::FATAL) << "No convergence reached"
                           << Prop<int>("iterations", 541)
                           << Prop<float>("rms", 0.0032)
                           << Rec::endr;

    // You can create your own log, if you wish, for testing purposes.
    // This log prints both to the screen and a log file, each with different
    // verbosities
    int fileThreshold = Log::DEBUG;
    int screenThreshold = Log::INFO;
    DualLog myOwnLog("example.log", fileThreshold, screenThreshold);
    myOwnLog.log(Log::DEBUG, "This will only go to the file");
    myOwnLog.log(Log::WARN, "You will see this on the screen");

    // you can create a named child Log from this one.  This one will be 
    // quieter, printing only warnings and failures.  
    Log child(myOwnLog, "myapp", Log::WARN);
    child.log(Log::INFO, "This one won't be printed in either destination.");

    // Logs have hierarchical relationships, inheriting destinations and 
    // verbosity thresholds.
    Log grandchild(child, "mysubcomponent");
    grandchild.log(Log::INFO, "We still won't see this message.");
    grandchild.setThreshold(Log::INFO);
    grandchild.log(Log::INFO, "But now we will see this in the file.");

    // Note that any future child log with the same name will remember its 
    // threshold, even if the original is destroyed.  Note that this one is 
    // 2 levels down from its root.
    Log anothergc(myOwnLog, "myapp.mysubcomponent");
    anothergc.log(Log::INFO, "Again, a file-only message");

    return 0;
}
