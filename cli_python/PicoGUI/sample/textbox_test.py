import PicoGUI
app = PicoGUI.Application('Textbox Test')
tb = app.addWidget('scrollbox').addWidget('Textbox','inside')
tb.side = 'all'
tb.insertmode = 'atcursor'

tb.text = """Beastie Boys - Just a Test

Like a flag at half mast as frames click fast
Not a thing will last as past is past
Like stacks of thoughts that got played and worn
Used over and over till they were tired and torn
Like a broken clock that can't tell time
Like a thick ass book that's filled with wack rhymes
Like a scorching blaze that burned the sand
Like a band that planned and planned and planned
And flew down like a raven in the dark of night
And snatched up the worm helpless to fight
And brought it back to the nest singing microphone check
One two one two this is just a test

Like sand in one hand You can't hold for long
Like scheming on a plan that goes all wrong
Like fudge and caramel they're not the same
And it's a shame all you talk is game
You act like a diamond waiting to be set
In a gold ring, as if, I bet
As time goes by in this give and take
As long as I learn I will make mistakes
What do I want? What do I need?
Why do I want it? What's in it for me?
It's the imagery of technology
Is what you get is what you see
Don't worry your mind
When you give it your best
One two one two this is just a test

--------8<--------

The formal name is "All-purpose Humanoid Fighting Machine - Android Evangelion" (HANYOU HITOGATA KESSEN HEIKI - JINZOU NINGEN EVANGELION). 14 years of time and an astronomical amount of money were used to build the Evangelion in order to carry out the Instrumentality Project. Able to both deploy and neutralize an A. T. Field, it is the only fighting force that humans have to counter the Angels. It operates on electric power, and is limited to about 5 minutes of active time without external supply. Therefore it is normally supplied with electrical power through the umbilical cable. (The Evangelion Mass-Production Models from Eva-05 onward are equipped with the S2 engine and thus do not need this cable.) Originally the Eva do not have souls. For this reason, the soul of Yui Ikari was made to dwell in Eva-01. The name Evangelion is thought to derive from "Eve", wife of Adam in the Old Testament, and "Evangel", the Greek word for "gospel". Source: The End of Evangelion : Glossary contained within The End of Evangelion - Theatrical Program.

--------8<--------

Pink Floyd - Wish You Were Here

so, so you think you can tell
heaven from hell
blue skies from pain
can you tell a green field
from a cold steel rail
a smile from a veil
do you think you can tell?
and did they get you to trade your heroes for ghosts?
hot ashes for trees?
hot air for a cool breeze?
cold comfort for change?
and did you exchange a walk on part in the war
for a lead role in a cage?
how i wish
how i wish you were here
we're just two lost souls swimming in a fish bowl
year after year
running over the same old ground
what have we found?
the same old fears
wish you were here

-- The End --"""

def stream(ev, w):
    if ev.hasMod('ctrl'):
        if ev.char == 's':
            # stream
            w.stream('\nsome more text\n')
        elif ev.char == 'n':
            # nuke
            w.nuke()
        elif ev.char == ' ':
            print 'no status to display'

app.link(stream, tb, 'kbd keyup')

app.run()
print "Testing text readback:\n\n\"" + tb.text
