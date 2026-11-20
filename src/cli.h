// `reelo config|default|library|--version|--help`: everything that needs no window. uQ
// Settings' page runs these. Anything else on the command line is a file or URL to play.
#pragma once

#include <QStringList>
#include <QTextStream>

namespace reelo {

/// Whether `args` (without argv[0]) is a CLI command rather than files to play.
bool isCliCommand(const QStringList &args);
/// Runs it, writing to `out`/`err`; returns the exit code.
int runCli(const QStringList &args, QTextStream &out, QTextStream &err);

} // namespace reelo
