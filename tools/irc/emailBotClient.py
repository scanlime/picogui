#!/usr/bin/env python
"""
 A hackish little client for the announceBot that accepts email messages from stdin.  
 This is meant to be used from a .forward file.
 The message should have a subject of the form "Announce <channel>".
 Every non-blank line of the body will be sent as a message to the bot.
 There's also a lot of goop in here to update stats.
"""
from twisted.internet import reactor, protocol
import sys, email, os, re, time
import irc_colors

baseDir = "/home/commits"
channelFile = "/home/commits/channels.list"
mailLog = os.path.join(baseDir, "mail.log")
commandLog = os.path.join(baseDir, "commands.log")
statsDir = os.path.join(baseDir, "stats")
urlDir = os.path.join(baseDir, "urls")
statsSubdirs = ("forever", "daily", "weekly", "monthly")
socketBaseName = "/tmp/announceBot.socket"

# Allowed commands, split up into those with content and those without
allowedTextCommands = ("Announce", "SendToChannels")
allowedControlCommands = ("JoinChannel", "PartChannel")
statCountedCommands = ("Announce",)

# Prohibited channels
# List from http://www.freenode.net/drones.shtml
badChannels = ("#!~!raisin!!",
               "0-xdcc!",
               "03337",
               "123",
               "acs45",
               "conscriptp",
               "efferz",
               "hackers",
               "infected",
               "livethisgame",
               "nodo747",
               "nonsense",
               "plazateam",
               "r3p4d",
               "scan",
               "secrets",
               "shell",
               "soviet-union",
               "techno-sound",
               "test12",
               "[alpha]",
               "\247\246\247\246\247",
               "\247\247\247",

               # Commits is in here, since the bot is in it by
               # default and shouldn't be able to leave
               "commits",
               )

# Little utilities to save and load numbers from files... for our cheesy stats directory
def saveInt(f, i):
    f = open(f, "w")
    f.write("%d\n" % i)
    f.close()

def loadInt(f):
    f = open(f)
    count = int(f.read().strip())
    f.close()
    return count

def addIntToFile(f, i):
    try:
        count = loadInt(f)
    except IOError:
        count = None
    if not count:
        count = 0
    count += i
    saveInt(f, count)

def incrementProjectCommits(project):
    if project.find(os.sep) >= 0:
        return
    for statsSubdir in statsSubdirs:
        addIntToFile(os.path.join(statsDir, statsSubdir, project), 1)

def updateCommitTimes(project):
    """Record data for the Mean Time Between Commits page"""
    currentTime = time.time()
    lastTime = None
    try:
        lastTime = loadInt(os.path.join(statsDir, 'mtbc', project + '.lastTime'))
    except IOError:
        pass
    if lastTime:
        addIntToFile(os.path.join(statsDir, 'mtbc', project + '.numSamples'), 1)
        addIntToFile(os.path.join(statsDir, 'mtbc', project + '.totalTime'), currentTime - lastTime)
    saveInt(os.path.join(statsDir, 'mtbc', project + '.lastTime'), currentTime)

def updateStats(project):
    incrementProjectCommits(project)
    updateCommitTimes(project)
    updateCommitTimes('commits')   # For overall MTBC

def applyColorTags(message):
    # We support tags of the form {red}, {bold}, {normal}, etc.
    message = re.sub('{bold}', irc_colors.BOLD, message)
    message = re.sub('{normal}', irc_colors.NORMAL, message)
    message = re.sub('{reverse}', irc_colors.REVERSE, message)
    message = re.sub('{underline}', irc_colors.UNDERLINE, message)
    for color in irc_colors.COLORS:
        message = re.sub('{%s}' % color, irc_colors.COLOR_PREFIX + irc_colors.COLORS[color], message)
    return message

def logCommand(cmd):
    f = open(commandLog, "a")
    f.write(cmd)
    f.close()

def setProjectURL(project, url):
    f = open(os.path.join(urlDir, project), "w")
    f.write("%s\n" % url)
    f.close()

class AnnounceClient(protocol.Protocol):
    def connectionMade(self):
        # at this point we have got a message, have done some basic parsing, and know that it's meant for a channel
        # Don't allow known bad channels, or names with slashes
        if len(subjectFields) == 2 or (not subjectFields[1] in badChannels and subjectFields[1].find(os.sep) < 0):
            # Send allowed text commands
            if subjectFields[0] in allowedTextCommands:
                # Our lame little stat page
                if subjectFields[0] in statCountedCommands:
                    updateStats(subjectFields[1])
                
                # This limits the length of the maximum message, mainly to prevent DOS'ing the bot too badly
                for l in lines:
                    self.transport.write(l)

            # Send allowed control commands
            elif subjectFields[0] in allowedControlCommands:
                commandLine = "%s %s\r\n" % (subjectFields[0], subjectFields[1])
                logCommand(commandLine)
                self.transport.write(commandLine)
            
        self.transport.loseConnection()
    
    def connectionLost(self, reason):
        from twisted.internet import reactor
        reactor.stop()

class AnnounceClientFactory(protocol.ClientFactory):
    protocol = AnnounceClient

    def clientConnectionFailed(self, connector, reason):
        reactor.stop()
    
    def clientConnectionLost(self, connector, reason):
        reactor.stop()

if __name__ == '__main__':
    import sys

    mailMsg  = email.message_from_file(sys.stdin)
    
    # log it
    f = open(mailLog, "a")
    f.write(mailMsg.as_string())
    f.close()

    subjectFields = mailMsg['Subject'].split(" ")
    message = mailMsg.get_payload()
    # Commands we process here
    if subjectFields[0] == "SetProjectURL":
        setProjectURL(subjectFields[1], message.split("\n")[0].strip())
    elif subjectFields[0] == "Die":
        os.system("killall python")
    else:
        try:
            # this block only completes if the command has a second argument, ie it is related to a channel
            
            subjectFields[1] = subjectFields[1].lower()
            # strip the hash from the channel if the user gave us one
            if subjectFields[1][0] == "#":
                subjectFields[1] = subjectFields[1][1:]
            
            # now locate the right socket for the given channel
            import glob
            channelFilesCurrentlyInExistance = glob.glob(channelFile + ".*");
            channelFilesCurrentlyInExistance.sort()
            socketName = socketBaseName
            lastBotID = "1"
            for cf in channelFilesCurrentlyInExistance:
                f = open(cf)
                channelList = {}
                lastBotID = re.compile('.*\.(.*)$').search(cf).group(1)
                for line in f.readlines():
                    line = line.strip()
                    if line == subjectFields[1]:
                        # we have found the channel, ie a bot is already present in it
                        socketName = socketBaseName + "." + lastBotID
                        #print "found socket = " + socketName
                f.close()

            # we did not find the channel, ie we want to join it
            if socketName == socketBaseName:
                # we always choose the last bot to be started for joining channels
                socketName = socketBaseName + "." + lastBotID;
                #print "socketName is " + socketName

            # convert the message into the lines to send to the socket, and log it
            # This limits the length of the maximum message, mainly to prevent DOS'ing the bot too badly
            lines = []
            for line in message.split("\n")[:8]:
                line = line.strip()
                if len(line) > 0:
                    commandLine = "%s %s %s\r\n" % (subjectFields[0], subjectFields[1], line)
                    # Log the message before we colorize it, so it can be used in places other than IRC.
                    logCommand(commandLine)
                    lines.append(applyColorTags(commandLine))

                
            # now launch the client
            f = AnnounceClientFactory()
            reactor.connectUNIX(socketName, f)
            reactor.run()
            
            # if we were not the first bot, AND this was not a Join/Part request (which should only go to the last bot)
            # we need to send it to the first bot as well so things appear in #commits
            if socketName != socketBaseName + ".1" and subjectFields[0] == "Announce":
                socketName = socketBaseName + ".1"
                f = AnnounceClientFactory()
                reactor.connectUNIX(socketName, f)
                reactor.run()
        except IndexError:
            # this command does not relate to a channel
            pass
    
