""" PGBuild.UI.None

Base classes for all UI modules, doesn't implement any UI at all.
"""
# 
# PicoGUI Build System
# Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
# 
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#  
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#  
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
# 
_svn_id = "$Id$"

import time, sys
import StringIO

class TimeStamp(object):
    """This time stamp object is applied to tasks and individual progress reports.
       UIs that can display multiple panes of information may choose to display this
       to give the user an indication of when each displayed message was first displayed.
       """
    def __init__(self, stamp=None):
        if stamp == None:
            stamp = time.time()
        self.time = stamp

    def __str__(self):
        return time.strftime("%H:%M:%S", time.localtime(self.time))


class Progress(object):
    """Base class for progress reporting. This manages verbosity levels and other such
       tedium, passing on the actual message output to a subclass to handle.

       This class supports dividing the program's execution into a hierarchy of tasks,
       and reporting progress within each of those tasks.

       Verbosity levels:
         1.0 is always the neutral verbosity level. -v command line switches, for example,
         add 1 to the verbosity level. The -q switch will set it to zero.

       Unimportance levels:
         Tasks and progress reports have unimportance levels- if the task/report's unimportance
         is greater than the verbosity, nothing will be output.

         The unimportance level of a task is added to all of its children's unimportance levels.
         This means that a trivial task can be given an unimportance of 1 for example, so that
         its children will have an unimportance of 2 and will only be output if there was a
         -v command line switch. However, if a critical error with an unimportance of -5 occurs
         in the trivial task, the task heading and that error will both be output.
       """

    def __init__(self, verbosityLevel=1, parent=None, taskName=None):
        self.parent = parent
        self.verbosityLevel = verbosityLevel
        self.taskName = taskName
        self.taskHeadingPrinted = 0
        self.timeStamp = TimeStamp()
        if parent:
            self.indentLevel = parent.indentLevel + 1
            self.root = parent.root
        else:
            self.indentLevel = 0
            self.root = self
            self.lastTask = self
        self._init()

    def _init(self):
        """Hook for extra initialization after the standard constructor"""
        pass

    def _showTaskHeading(self):
        """Hook for outputting a message when a task is started or changed"""
        pass

    def showTaskHeading(self):
        """We need to print a task heading if we're in a task we've never been in
           before, or if the current task has changed.
           This can also be called manually if a task is starting that will take
           some time before generating any progress reports.
           """
        if self.root.lastTask != self or not self.taskHeadingPrinted:
            if self.parent and not self.parent.taskHeadingPrinted:
                self.parent.showTaskHeading()            
            if self.taskName:
                self._showTaskHeading()
            self.taskHeadingPrinted = 1
            self.root.lastTask = self

    def _outputTest(self, unimportance):
        """Test whether a message is important enough to output,
           if so return true and make sure our task headings have
           been printed.
           """
        if unimportance > self.verbosityLevel:
            return 0
        self.showTaskHeading()
        return 1

    def _report(self, verb, noun):
        """Hook for reporting progress, given an action and subject"""
        pass
    
    def report(self, verb, noun, unimportance=1):
        if self._outputTest(unimportance):
            self._report(str(verb), str(noun))

    def task(self, name, unimportance=0):
        """Create a new Progress object representing a hierarchial task"""
        return self.__class__(self.verbosityLevel - unimportance, self, name)

    def _warning(self, text):
        """Hook for displaying a warning message"""
        pass

    def warning(self, text, unimportance=1):
        if self._outputTest(unimportance):
            self._warning(str(text))

    def _error(self, text):
        """Hook for displaying an error message"""
        pass
            
    def error(self, text, unimportance=-5):
        if self._outputTest(unimportance):
            self._error(str(text))

    def _message(self, text):
        """Hook for displaying a generic message"""
        pass

    def message(self, text, unimportance=1):
        if self._outputTest(unimportance):
            self._message(str(text))


class Interface(object):
    """Class responsible for driving PGBuild's execution based on user input
       and configuration settings. This base class only considers configuration
       settings, but it includes hooks for adding a UI.
       """
    progressClass = Progress
    timeStampClass = TimeStamp
    
    def __init__(self, ctx):
        ctx.ui = self

        # Set up a progress reporter object at the specified verbosity
        self.verbosity = int(ctx.config.eval("invocation/option[@name='verbosity']/text()"))
        ctx.progress = self.progressClass(self.verbosity)

        ctx.progress.message("Using UI module %s" % self.__class__.__module__, 2)

    def cleanup(self, ctx):
        pass

    def run(self, ctx):
        """Execute the UI. By default, this just executes
           all PGBuild Tasks according to invocation options.
           """
        import PGBuild.Tasks
        PGBuild.Tasks.TaskList(ctx, PGBuild.Tasks.allTasks).run()
        
    def exitWithError(self, ctx, message):
        ctx.progress.error(message)
        self.cleanup(ctx)
        sys.exit(1)

    def exception(self, ctx, exc_info):
        """This is called when PGBuild.Main catches an exception. This default
           implementation gives subclasses a chance to clean up, then tries to convert
           the exception into an error for the Progress class if it's one of our types.
           """
        import PGBuild.Errors
        if isinstance(exc_info[1], PGBuild.Errors.ExternalError) and not \
               ctx.config.eval("invocation/option[@name='traceback']/text()"):
            # Pretty-print ExternalErrors a bit, hiding the details that might scare people
            message = ""
            try:
                message += exc_info[0].explanation + ",\n"
            except AttributeError:
                pass
            try:
                message += exc_info[1].args
            except AttributeError:
                pass
            if not message:
                message = str(exc_info[1])
            self.exitWithError(ctx, message)
        else:
            self.cleanup(ctx)
            self._exception(ctx, exc_info)

    def _exception(self, ctx, exc_info):
        raise exc_info[0], exc_info[1], exc_info[2]

    def list(self, ctx, listPath):
        """Guts of the --list command line option- list the contents of an XPath"""
        results = ctx.config.xpath(listPath)

        # A little bit of voodoo-
        # If we have only one result and that result has no name attribute,
        # list all the children. If we have multiple results, just list
        # those results. This means that paths like "packages" or
        # "package/package[@name='foo']" both work.
        if len(results) == 1:
            try:
                results[0].attributes['name']
            except KeyError:
                results = results[0].childNodes
            
        self.listNodes(ctx, results)

    def listNodes(self, ctx, nodes):
        """Given a list of DOM nodes, print a table with the node names and description summaries.
           This is used to format the output of the --list and --search options.
           """
        # This converts the node list into a list of (name, description) tuples, to be passed
        # to showTable(). This makes it easier to override in the other UIs.
        table = []
        for node in nodes:
            if node.nodeType == node.ELEMENT_NODE:
                try:
                    # An element with a name, list it by name without recursion. Try to get a description.
                    try:
                        description = ctx.config.xpath('description/summary/text()', node)[0].data
                    except IndexError:
                        description = ""
                    table.append((node.attributes['name'].value, description))
                except KeyError:
                    pass
        table.sort()
        if table:
            self.showTable(ctx, table)
        else:
            ctx.progress.message("No results")

    def showTable(self, ctx, table, write=sys.stdout.write):
        """Display the supplied sequence of sequences as a table"""
        columns = len(table[0])

        # Find the maximum width of each column
        maxWidths = [0] * columns
        for row in table:
            for col in xrange(columns):
                if len(row[col]) > maxWidths[col]:
                    maxWidths[col] = len(row[col])

        # Create a format string for this table
        separator = " : "
        formatString = ""
        for width in maxWidths:
            formatString += "%%-%ds%s" % (width, separator)

        # Output it using the generated format string
        strippedSeparator = separator.strip()
        for row in table:
            # Format the row, then strip off excess whitespace or separators from the end
            line = formatString % row
            while True:
                line = line.strip()
                if line.endswith(strippedSeparator):
                    line = line[:-len(strippedSeparator)]
                else:
                    break
            write(line + "\n")
            
### The End ###
        
    
