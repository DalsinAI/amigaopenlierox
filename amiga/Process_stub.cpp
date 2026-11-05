/*
 *  OpenLieroX for AmigaOS: no child processes. src/common/Process.cpp runs
 *  the dedicated server's control scripts (Python and others) through
 *  pstreams or Boost.Process; the Amiga port has no dedicated server, so
 *  Process::open() always fails and the streams are empty.
 *
 *  The AmiCradle Team. LGPL, like OpenLieroX.
 */
#include <sstream>
#include "Process.h"
#include "Debug.h"

struct ProcessIntern {
	std::ostringstream in;
	std::istringstream out;
};

Process::Process() : data(new ProcessIntern()) {}
Process::~Process() { delete data; data = NULL; }
std::ostream& Process::in() { return data->in; }
std::istream& Process::out() { return data->out; }
void Process::close() {}

bool Process::open(const std::string& cmd, std::vector<std::string>, const std::string&) {
	warnings << "Process: cannot run " << cmd << " (no child processes in the Amiga port)" << endl;
	return false;
}
