#!/bin/sh
#
# Simple CGI script to generate a page with per-project
# message counts generated by the CIA bot.
#
STATDIR=/home/commits/stats
CHANNELFILE=/home/commits/channels.list

# Sure, let's use bash as an HTML templating language :-)
echo -ne "Content-type: text/html\n\n"
cat <<EOF

<html>
<head><title>CIA bot statistics</title></head>
<h1>CIA bot statistics</h1>

<p>Number of messages posted per project:</p>
<p>
<table border cellspacing=0 cellpadding=5>
<tr><td> <b>Project</b> </td><td> <b>Messages</b> </td></tr>

EOF
(cd $STATDIR; for file in *; do
  echo "<tr><td>" $file "</td><td>" `cat $file` "</td></tr>"
done) | sort -n -r +3
cat <<EOF

</table>
</p>

<p>Channels the bot is currently in:</p>

<ul>
EOF
sort $CHANNELFILE | xargs -n1 -i echo "<li>" {} "</li>"
cat <<EOF
</ul>

</html>
EOF
