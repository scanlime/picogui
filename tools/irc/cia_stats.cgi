#!/usr/bin/env python
#
# Simple CGI script to generate a page with per-project
# message counts generated by the CIA bot.
#

import os, math
from cia_statreader import *

# Page heading
print """Content-type: text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<head>
  <title>CIA bot statistics</title>
  <style type="text/css" media="all"> @import url(http://navi.picogui.org/svn/picogui/trunk/tools/css/cia_stats.css);
  </style>
</head>
<body>

<div class="heading">
   <div class="title">CIA bot statistics</div>
   <div class="subtitle">Because SF stats weren't pointless enough</div>
</div>

<div><span class="section">Number of messages posted per project</span></div>
<div class="section">
  <div class="sectionTop"></div>
  <div class="row">
    <table>
"""

# Project table heading
print "<tr>",
for heading in ('project','mtbc') + statSubdirs:
    print '<th>%s</th>' % statHeadings[heading],
print "</tr>"

# Save the highest number for each column
columnMaxima = [0] * len(statSubdirs)
for project in projects:
    for i in xrange(len(statSubdirs)):
        if projectCounts[project][statSubdirs[i]] > columnMaxima[i]:
            columnMaxima[i] = projectCounts[project][statSubdirs[i]]

def convertDuration(t):
    """Convert a duration in seconds to other units depending on its magnitude"""
    if not t:
        t = ''
    elif t > 172800:
        t = "%.1fd" % (t / 86400)
    elif t > 7200:
        t = "%.1fh" % (t / 3600)
    elif t > 120:
        t = "%.1fm" % (t / 60)
    else:
        t = "%.1fs" % t
    return t

# Project table contents
for project in projects:
    print "<tr><td>%s</td><td>%s</td>" % (project, convertDuration(projectMTBC[project])),
    for statIndex in xrange(len(statSubdirs)):
        count = projectCounts[project][statSubdirs[statIndex]]
        if count:
            # Get a fraction of this count compared to the highest in the column
            logMax = math.log(columnMaxima[statIndex])
            if logMax > 0:
                fraction = math.log(count) / logMax
            else:
                fraction = 1.0
            # Multiply by the desired maximum bar length in EMs, add the minimum bar padding
            width = fraction * 4 + 0.2
            # A stupid trick for making bargraph thingies
            print """
            <td>
              <span class="bargraph" style="padding: 0em %.4fem;">%s</span>
            </td>
            """ % (width, count)
        else:
            print "<td></td>",
    print "</tr>"
    
print """
    </table>
    (graph is logarithmic. MTBC == Mean Time Between Commits)
  </div>
</div>

<div><span class="section">Totals</span></div>
<div class="section">
  <div class="sectionTop"></div>
  <div class="row">
    <ul>
"""
print "<li>%d channels</li>" % len(channels)
print "<li>%d projects</li>" % len(projects)
msgTotal = 0
for projCount in projectCounts.values():
    msgTotal += projCount[statSubdirs[0]]
print "<li>%d messages</li>" % msgTotal
print "<li>overall MTBC: %s</li>" % convertDuration(totalMTBC)
print """
    </ul>
  </div>
</div>

<div><span class="section">Most recent commits</span></div>
<div class="section">
  <div class="sectionTop"></div>
  <div class="commitBox">
    <ul>
"""
for command in readLatestCommands():
    if command[0] == "Announce":
        projectName = command[1]
        messageLine = htmlifyColorTags(command[2])
        print "<li><b>%s</b>: %s</li>" % (projectName, messageLine)
print """
    </ul>
  </div>
</div>

<div><span class="section">Channels the bot is currently in</span></div>
<div class="section">
  <div class="sectionTop"></div>
  <div class="row">
    <ul>
"""
for channel in channels:
    print "<li>%s</li>" % channel
print """
    </ul>
  </div>
</div>

</body>
</html>
"""
