#!/usr/bin/env python
#
# Simple CGI script to generate a page with per-project
# message counts generated by the CIA bot.
#

import os

statDir = "/home/commits/stats"
channelFile = "/home/commits/channels.list"

# List out the subdirs explicitly so we can set the order-
# the first one here is used as the sort key
statSubdirs = ('forever', 'monthly', 'weekly', 'daily')

projects = os.listdir(os.path.join(statDir, statSubdirs[0]))
channels = open(channelFile).read().strip().split("\n")

projectCounts = {}
for project in projects:
    # Build a map of counts indexed by subdirectory
    counts = {}
    for subdir in statSubdirs:
        counts[subdir] = int(open(os.path.join(statDir, subdir, project)).read().strip())
    projectCounts[project] = counts

# Sort the project list by the 'forever' count, descending
def countSort(a,b):
    return cmp(projectCounts[b][statSubdirs[0]],
               projectCounts[a][statSubdirs[0]])
projects.sort(countSort)

# Page heading
print """Content-type: text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<head>
  <title>CIA bot statistics</title>
</head>
<body>
<h1>CIA bot statistics</h1>
"Because SF stats weren't pointless enough"

<p>Number of messages posted per project:</p>

<table border="1" cellspacing="0" cellpadding="5">
"""

# Project table heading
print "<tr>",
for heading in ('Project',) + statSubdirs:
    print "<td><b>%s</b></td>" % heading,
print "</tr>"

# Project table contents
for project in projects:
    print "<tr><td>%s</td>" % project,
    for subdir in statSubdirs:
        print "<td>%s</td>" % projectCounts[project][subdir],
    print "</tr>"
    
print """

</table>

<p>Channels the bot is currently in:</p>

<ul>
"""
for channel in channels:
    print "<li>%s</li>" % channel
print """
</ul>

</body>
</html>
"""
