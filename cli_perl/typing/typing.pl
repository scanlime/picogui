#!/usr/bin/perl
#
# PicoGUI/Perl typing program
#
# This is really messy, I'll write an easier-to-follow example
# program later.
#
# Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
# 
use PicoGUI;

$progname = "Typing Program";
$version = "0.01";

############################################################ Load resources

# Directory for student data files
$studentdir = '../students/';

# Load all data files from this directory
chdir('data');

# Load the lessons
open LESSONF,"lessons.txt" or die "Can't open lesson file: $!\n";
while (<LESSONF>) {
    next if (!/\S/);
    next if (/^\#/);
    if (/^:\s*(.*)/) {
	$name = $1;
	chomp $name;
	push @lesson_names,$name;
    }
    else {
	$lesson{$name} .= $_;
    }
}
close LESSONF;
foreach (keys %lesson) {
    chomp $lesson{$_};
};

# Load resources into the server's memory, and save
# the handles.
$check = NewBitmap(-file => "check.pnm");
$checkmask = NewBitmap(-file => "check_mask.pnm");
$bigfont = NewFont("Utopia",25);
$boldfont = NewFont("Helvetica",10,bold);
$ex = NewBitmap(-file => "x.pnm");
$exmask = NewBitmap(-file => "x_mask.pnm");
$circ = NewBitmap(-file => "circarrow.pnm");
$circmask = NewBitmap(-file => "circarrow_mask.pnm");
$redbox = NewBitmap(-file => "redbox.pnm");
$greenbox = NewBitmap(-file => "greenbox.pnm");
$boxmask = NewBitmap(-file => "boxmask.pnm");

# Load the theme file
RestoreTheme;
ThemeSet(-file => "wavyinsomnia.theme");
Update;

# This is for the "Start Over" button
 restartprog:
    EnterContext;

############################################################ Welcome

# Welcome message
EnterContext;
NewPopup(300,100);
$tb = NewWidget(-type => toolbar,-side => bottom);
NewWidget(-type=>label,-transparent=>1,-text=>NewString("Welcome to"),-font=>$boldfont);
NewWidget(-type=>label,-transparent=>1, -side=>all,-text=>NewString($progname),-font=>$bigfont);
NewWidget(-type => button,-inside => $tb,-bitmap=>$check,-hotkey => $PGKEY{RETURN},
	  -bitmask=>$checkmask,-text=>NewString("Click to continue"),-side=>all,
	  -onclick => \&ExitEventLoop);
# Process event handlers until an ExitEventLoop
EventLoop;
# This deletes any memory used between here and the
# last EnterContext
LeaveContext;

############################################################ Get username

# Get the user's name
EnterContext;
NewPopup(280,80);
$tb = NewWidget(-type => toolbar,-side => bottom);
NewWidget(-type=>label,-transparent=>1,-text=>NewString("What is your name?"),-font=>$boldfont);
$field = NewWidget(-type=>field,-font =>$bigfont,-side => all,
		   -inside => NewWidget(-type=>box,-side=>all));

NewWidget(-type => button,-inside => $tb,-bitmap=>$check,-hotkey => $PGKEY{RETURN},
	  -bitmask=>$checkmask,-text=>NewString("Ok"),-side=>left,
	  -onclick => sub {
     # Get and validate the input before continuing
     $name = $field->GetWidget(-text)->GetString;

     # No line-noise names
     return if ($name !~ /[a-z]/); 

     # The student name is munged into a file name
     $fname = lc($name);
     $fname =~ s/[^a-z]//g;

     # Try to open the student file
     if (open STUDENTF,$studentdir.$fname) {
	 # Sucess, read in saved data
	 while (<STUDENTF>) {
	     next if (!/\S/);
	     next if (/^\#/);
	     chomp;
	     ($key,$value) = split /:/,$_;
	     $student{$key} = $value;
	 }
	 close STUDENTF;
	 ExitEventLoop;
     }
     else {
	 # Student not found dialog box
	 
	 EnterContext;
	 NewPopup(200,150);

	 $tb2 = NewWidget(-type => toolbar,-side => bottom);
	 $tb1 = NewWidget(-type => toolbar,-side => bottom);
	 
	 NewWidget(-type=>bitmap,-side=>right,-lgop => 'or',-transparent=>1,
		   -bitmap  => NewBitmap(-file => "question.pnm"),
		   -bitmask => NewBitmap(-file => "question_mask.pnm"));
	 
	 NewWidget(-type=>label,-transparent=>1, -side=>all,
		   -text=>NewString("Your saved data\nwas not found"));
	 
	 NewWidget(-type => button,-side => all,-inside => $tb2,
		   -text=>NewString("I'm new here"),
		   -onclick => sub {
		       # We don't have to do anything special to make
		       # a new user- the user data is saved on completion
		       # of a lesson. But, this lets the user know that
		       # they are starting fresh.

		       LeaveContext;
		       ExitEventLoop;
		   });

	 NewWidget(-type => button,-side => all, -inside => $tb1,
		   -text=>NewString("That's odd, let me try again"),
		   -hotkey => $PGKEY{RETURN},
		   -onclick => sub {LeaveContext; Update;});
	 
	 Update;
     }
});

NewWidget(-type => button,-side => left,
	  -text=>NewString("Guest"),
	  -onclick => sub {
	      # Don't load any data, but also don't save any later...
	      $name = 'Guest';
	      $fname = '';

	      ExitEventLoop;
	  });

NewWidget(-type => button,-bitmap=>$ex,-hotkey => $PGKEY{ESCAPE},
	  -bitmask=>$exmask,-text=>NewString("Cancel"),-side=>right,
	  -onclick => \&Finis);

$field->focus;
EventLoop;
LeaveContext;

############################################################ Typing panel

$typingpanel = RegisterApp(-side => top,-height => 400);
$lessonname = NewWidget(-type => label,-font => NewFont("Utopia",25,grayline,italic),
			-transparent => 1, -align => right);
$lessoninfo = NewWidget(-type => label,-font => $boldfont,
			-transparent => 1, -align => nw);
$tb = NewWidget(-type => toolbar, -side => bottom);

NewWidget(-type => label, -color => 0xFFFFFF, -bgcolor => 0x000000,
	  -side => top,-text => NewString("Lesson contents:"),
	  -font => NewFont("Times",0,bold));

$lessontextwidget = NewWidget(-type => label,-transparent => 1,
			      -side => top, -align => nw,
			      -font => NewFont("Times"));

NewWidget(-type=>button,-inside => $tb,-side => right,
	  -text => NewString("Reset scores for this lesson"),
	  -onclick => sub {

     EnterContext;
     NewPopup(200,100);
     $tb = NewWidget(-type => toolbar,-side => bottom);

     NewWidget(-type=>bitmap,-side=>right,-lgop => 'or',-transparent=>1,
	       -bitmap  => NewBitmap(-file => "question.pnm"),
	       -bitmask => NewBitmap(-file => "question_mask.pnm"));
     
     NewWidget(-type=>label,-transparent=>1, -side=>all,
	       -text=>NewString("Erase the scores for\n\"$lname\"?"));

     NewWidget(-type => button,-bitmap=>$check,-hotkey => $PGKEY{'y'},
	       -bitmask=>$checkmask,-text=>NewString("Yes"),-side=>left,
	       -inside => $tb,-onclick => sub {
		   delete $student{$lname};
		   savestudent();
		   LeaveContext;
		   setlesson();
	       });
     
     NewWidget(-type => button,-bitmap=>$ex,-hotkey => $PGKEY{'n'},
	       -bitmask=>$exmask,-text=>NewString("No"),-side=>right,
	       -onclick => sub {LeaveContext; Update;});

     Update;

 });


NewWidget(-type=>button,-inside => $tb,-side => left,
	  -bitmap => $circ,-bitmask => $circmask,
	  -text => NewString("Try this lesson"),
	  -onclick => sub {
	      EnterContext;
	      NewPopup(600,380);
	      $tb = NewWidget(-type => toolbar,-side => bottom);

	      @lessontext = split /\n/,$lesson{$lname};
	      $lessonline = 0;
	      $str = '';
	      $correct = 0;
	      $incorrect = 0;
	      $starttime = 0;
	      $wpm = 0;
	      $lessonlinetotal = @lessontext;

	      $typeinfo = NewWidget(-type => label,-side => bottom,-bgcolor => 0x000000,
				    -color => 0xFFFFFF,-font => $boldfont);
	      $typeinfo_top = NewWidget(-type => label,-side => top,
				    -font => $boldfont,-align => left);
	      $typespace = NewWidget(-type => label,-side => all);

	      NewWidget(-type => button,-inside => $tb,-bitmap=>$ex,
			-bitmask=>$exmask,-text=>NewString("Give up"),-side=>left,
			-onclick => sub {
			    GiveKeyboard;
			    LeaveContext;
			    undef $tstext;
			    Update;
			});

	      GrabKeyboard(-onchar => \&typechar);

	      # Set things up initially
	      typechar();
	  });

############################################################ Lesson panel

$lessonpanel = RegisterApp(-side => left,-width => 150);
$p = NewWidget(-type => label,-transparent => 1,-side => top,-text => 
	  NewString("Select a lesson:"),-font => $boldfont);

foreach (@lesson_names) {
    $p = NewWidget(-type => box,-after => $p);
    $boxes{$_} = NewWidget(-type => bitmap,-inside => $p,-side => right,
	      -transparent => 1,-lgop => 'or',
	      -bitmap => $student{$_} ? $greenbox : $redbox,-bitmask => $boxmask);
    $w = NewWidget(-type => button,-side => all,
		   -text => NewString($_),-onclick => \&setlesson);
    $setlessonto = $w if (!$student{$_} and !$setlessonto);
}
if ($setlessonto) {
    setlesson($setlessonto);
}
else {
    setlesson($w);
}

############################################################ Load info bar

# Set up the information bar at the bottom
$infobar = RegisterApp(-type => toolbar,-side => bottom);
NewWidget(-type => button, -side => left, -text => NewString("Exit"),
          -hotkey => $PGKEY{ESCAPE},-bitmap => $ex,-bitmask => $exmask,
	  -onclick => sub {
     # Put up the cheezy but standard "are you SURE you want to quit???" box

     # Because the 'main' part of the app already has an event loop running,
     # we don't need one (nor can we have one)
     # Instead, use contexts to keep the dialog box seperate.

     EnterContext;
     NewPopup(200,100);
     $tb = NewWidget(-type => toolbar,-side => bottom);

     NewWidget(-type=>bitmap,-side=>right,-lgop => 'or',-transparent=>1,
	       -bitmap  => NewBitmap(-file => "question.pnm"),
	       -bitmask => NewBitmap(-file => "question_mask.pnm"));

     NewWidget(-type=>label,-transparent=>1, -side=>all,
	       -text=>NewString("Are you sure you\nwant to leave?"));

     NewWidget(-type => button,-bitmap=>$check,-hotkey => $PGKEY{'y'},
	       -bitmask=>$checkmask,-text=>NewString("Yes"),-side=>left,
	       -inside => $tb,-onclick => \&Finis);

     NewWidget(-type => button,
	       -text=>NewString("Start Over"),-side=>left,
	       -onclick => sub {
		   # This is messy. Must fix.

		   LeaveContext;  # This message box
		   LeaveContext;  # Most of the program (except the pnm's, etc)
		   goto restartprog;  # Eek!
	       });

     NewWidget(-type => button,-bitmap=>$ex,-hotkey => $PGKEY{'n'},
	       -bitmask=>$exmask,-text=>NewString("No"),-side=>right,
	       -onclick => sub {LeaveContext; Update;});

     Update;

});
NewWidget(-type => label,-transparent=>1,-text=>NewString("  Logged in as"),
	  -side => left);
NewWidget(-type=>label,-transparent=>1,-text=>NewString($name),
	  -font=>$boldfont,-side => left);

NewWidget(-type => button, -side => right, -text => NewString("About"),
	  -bitmap =>  NewBitmap(-file => "tux.pnm"),
	  -bitmask => NewBitmap(-file => "tux_mask.pnm"),
	  -onclick => sub {
     # About box

     EnterContext;
     NewPopup(550,380);

     $tb = NewWidget(-type => toolbar,-side => bottom);
     $b1 = NewWidget(-type => box,-side => top,-size => 75);

     #### Load all the about box strings

     $copyright = NewString(<<EOF);
$progname version $version
Copyright (C) 2000 Micah Dowty <micah\@homesoftware.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

Click the tabs below for more information...
EOF

     $gpl  = NewString(<<EOF);
		    GNU GENERAL PUBLIC LICENSE
		       Version 2, June 1991

 Copyright (C) 1989, 1991 Free Software Foundation, Inc.
                          675 Mass Ave, Cambridge, MA 02139, USA
 Everyone is permitted to copy and distribute verbatim copies
 of this license document, but changing it is not allowed.

			    Preamble

  The licenses for most software are designed to take away your
freedom to share and change it.  By contrast, the GNU General Public
License is intended to guarantee your freedom to share and change free
software--to make sure the software is free for all its users.  This
General Public License applies to most of the Free Software
Foundation's software and to any other program whose authors commit to
using it.  (Some other Free Software Foundation software is covered by
the GNU Library General Public License instead.)  You can apply it to
your programs, too.

  When we speak of free software, we are referring to freedom, not
price.  Our General Public Licenses are designed to make sure that you
have the freedom to distribute copies of free software (and charge for
this service if you wish), that you receive source code or can get it
if you want it, that you can change the software or use pieces of it
in new free programs; and that you know you can do these things.

  To protect your rights, we need to make restrictions that forbid
anyone to deny you these rights or to ask you to surrender the rights.
These restrictions translate to certain responsibilities for you if you
distribute copies of the software, or if you modify it.

  For example, if you distribute copies of such a program, whether
gratis or for a fee, you must give the recipients all the rights that
you have.  You must make sure that they, too, receive or can get the
source code.  And you must show them these terms so they know their
rights.

  We protect your rights with two steps: (1) copyright the software, and
(2) offer you this license which gives you legal permission to copy,
distribute and/or modify the software.

  Also, for each author's protection and ours, we want to make certain
that everyone understands that there is no warranty for this free
software.  If the software is modified by someone else and passed on, we
want its recipients to know that what they have is not the original, so
that any problems introduced by others will not reflect on the original
authors' reputations.

  Finally, any free program is threatened constantly by software
patents.  We wish to avoid the danger that redistributors of a free
program will individually obtain patent licenses, in effect making the
program proprietary.  To prevent this, we have made it clear that any
patent must be licensed for everyone's free use or not licensed at all.

  The precise terms and conditions for copying, distribution and
modification follow.

		    GNU GENERAL PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  0. This License applies to any program or other work which contains
a notice placed by the copyright holder saying it may be distributed
under the terms of this General Public License.  The "Program", below,
refers to any such program or work, and a "work based on the Program"
means either the Program or any derivative work under copyright law:
that is to say, a work containing the Program or a portion of it,
either verbatim or with modifications and/or translated into another
language.  (Hereinafter, translation is included without limitation in
the term "modification".)  Each licensee is addressed as "you".

Activities other than copying, distribution and modification are not
covered by this License; they are outside its scope.  The act of
running the Program is not restricted, and the output from the Program
is covered only if its contents constitute a work based on the
Program (independent of having been made by running the Program).
Whether that is true depends on what the Program does.

  1. You may copy and distribute verbatim copies of the Program's
source code as you receive it, in any medium, provided that you
conspicuously and appropriately publish on each copy an appropriate
copyright notice and disclaimer of warranty; keep intact all the
notices that refer to this License and to the absence of any warranty;
and give any other recipients of the Program a copy of this License
along with the Program.

You may charge a fee for the physical act of transferring a copy, and
you may at your option offer warranty protection in exchange for a fee.

  2. You may modify your copy or copies of the Program or any portion
of it, thus forming a work based on the Program, and copy and
distribute such modifications or work under the terms of Section 1
above, provided that you also meet all of these conditions:

    a) You must cause the modified files to carry prominent notices
    stating that you changed the files and the date of any change.

    b) You must cause any work that you distribute or publish, that in
    whole or in part contains or is derived from the Program or any
    part thereof, to be licensed as a whole at no charge to all third
    parties under the terms of this License.

    c) If the modified program normally reads commands interactively
    when run, you must cause it, when started running for such
    interactive use in the most ordinary way, to print or display an
    announcement including an appropriate copyright notice and a
    notice that there is no warranty (or else, saying that you provide
    a warranty) and that users may redistribute the program under
    these conditions, and telling the user how to view a copy of this
    License.  (Exception: if the Program itself is interactive but
    does not normally print such an announcement, your work based on
    the Program is not required to print an announcement.)

These requirements apply to the modified work as a whole.  If
identifiable sections of that work are not derived from the Program,
and can be reasonably considered independent and separate works in
themselves, then this License, and its terms, do not apply to those
sections when you distribute them as separate works.  But when you
distribute the same sections as part of a whole which is a work based
on the Program, the distribution of the whole must be on the terms of
this License, whose permissions for other licensees extend to the
entire whole, and thus to each and every part regardless of who wrote it.

Thus, it is not the intent of this section to claim rights or contest
your rights to work written entirely by you; rather, the intent is to
exercise the right to control the distribution of derivative or
collective works based on the Program.

In addition, mere aggregation of another work not based on the Program
with the Program (or with a work based on the Program) on a volume of
a storage or distribution medium does not bring the other work under
the scope of this License.

  3. You may copy and distribute the Program (or a work based on it,
under Section 2) in object code or executable form under the terms of
Sections 1 and 2 above provided that you also do one of the following:

    a) Accompany it with the complete corresponding machine-readable
    source code, which must be distributed under the terms of Sections
    1 and 2 above on a medium customarily used for software interchange; or,

    b) Accompany it with a written offer, valid for at least three
    years, to give any third party, for a charge no more than your
    cost of physically performing source distribution, a complete
    machine-readable copy of the corresponding source code, to be
    distributed under the terms of Sections 1 and 2 above on a medium
    customarily used for software interchange; or,

    c) Accompany it with the information you received as to the offer
    to distribute corresponding source code.  (This alternative is
    allowed only for noncommercial distribution and only if you
    received the program in object code or executable form with such
    an offer, in accord with Subsection b above.)

The source code for a work means the preferred form of the work for
making modifications to it.  For an executable work, complete source
code means all the source code for all modules it contains, plus any
associated interface definition files, plus the scripts used to
control compilation and installation of the executable.  However, as a
special exception, the source code distributed need not include
anything that is normally distributed (in either source or binary
form) with the major components (compiler, kernel, and so on) of the
operating system on which the executable runs, unless that component
itself accompanies the executable.

If distribution of executable or object code is made by offering
access to copy from a designated place, then offering equivalent
access to copy the source code from the same place counts as
distribution of the source code, even though third parties are not
compelled to copy the source along with the object code.

  4. You may not copy, modify, sublicense, or distribute the Program
except as expressly provided under this License.  Any attempt
otherwise to copy, modify, sublicense or distribute the Program is
void, and will automatically terminate your rights under this License.
However, parties who have received copies, or rights, from you under
this License will not have their licenses terminated so long as such
parties remain in full compliance.

  5. You are not required to accept this License, since you have not
signed it.  However, nothing else grants you permission to modify or
distribute the Program or its derivative works.  These actions are
prohibited by law if you do not accept this License.  Therefore, by
modifying or distributing the Program (or any work based on the
Program), you indicate your acceptance of this License to do so, and
all its terms and conditions for copying, distributing or modifying
the Program or works based on it.

  6. Each time you redistribute the Program (or any work based on the
Program), the recipient automatically receives a license from the
original licensor to copy, distribute or modify the Program subject to
these terms and conditions.  You may not impose any further
restrictions on the recipients' exercise of the rights granted herein.
You are not responsible for enforcing compliance by third parties to
this License.

  7. If, as a consequence of a court judgment or allegation of patent
infringement or for any other reason (not limited to patent issues),
conditions are imposed on you (whether by court order, agreement or
otherwise) that contradict the conditions of this License, they do not
excuse you from the conditions of this License.  If you cannot
distribute so as to satisfy simultaneously your obligations under this
License and any other pertinent obligations, then as a consequence you
may not distribute the Program at all.  For example, if a patent
license would not permit royalty-free redistribution of the Program by
all those who receive copies directly or indirectly through you, then
the only way you could satisfy both it and this License would be to
refrain entirely from distribution of the Program.

If any portion of this section is held invalid or unenforceable under
any particular circumstance, the balance of the section is intended to
apply and the section as a whole is intended to apply in other
circumstances.

It is not the purpose of this section to induce you to infringe any
patents or other property right claims or to contest validity of any
such claims; this section has the sole purpose of protecting the
integrity of the free software distribution system, which is
implemented by public license practices.  Many people have made
generous contributions to the wide range of software distributed
through that system in reliance on consistent application of that
system; it is up to the author/donor to decide if he or she is willing
to distribute software through any other system and a licensee cannot
impose that choice.

This section is intended to make thoroughly clear what is believed to
be a consequence of the rest of this License.

  8. If the distribution and/or use of the Program is restricted in
certain countries either by patents or by copyrighted interfaces, the
original copyright holder who places the Program under this License
may add an explicit geographical distribution limitation excluding
those countries, so that distribution is permitted only in or among
countries not thus excluded.  In such case, this License incorporates
the limitation as if written in the body of this License.

  9. The Free Software Foundation may publish revised and/or new versions
of the General Public License from time to time.  Such new versions will
be similar in spirit to the present version, but may differ in detail to
address new problems or concerns.

Each version is given a distinguishing version number.  If the Program
specifies a version number of this License which applies to it and "any
later version", you have the option of following the terms and conditions
either of that version or of any later version published by the Free
Software Foundation.  If the Program does not specify a version number of
this License, you may choose any version ever published by the Free Software
Foundation.

  10. If you wish to incorporate parts of the Program into other free
programs whose distribution conditions are different, write to the author
to ask for permission.  For software which is copyrighted by the Free
Software Foundation, write to the Free Software Foundation; we sometimes
make exceptions for this.  Our decision will be guided by the two goals
of preserving the free status of all derivatives of our free software and
of promoting the sharing and reuse of software generally.

			    NO WARRANTY

  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY
FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN
OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES
PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS
TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE
PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,
REPAIR OR CORRECTION.

  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR
REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,
INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING
OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED
TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY
YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER
PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

		     END OF TERMS AND CONDITIONS

EOF

     $fun = NewString(<<EOF);
$progname was created using PicoGUI,
a portable, small,and open source GUI
toolkit created by Micah Dowty.
You can learn more about PicoGUI at:
    http://pgui.sourceforge.net

PicoGUI was ported from Linux to Windows with SDL:
    http://libsdl.org

Check out these other fun and educational web sites too:
    http://sourceforge.net
    http://slashdot.org
    http://www.gnu.org
    http://freshmeat.net

And most importantly:
    http://userfriendly.org
Home of the fuzzy guy on the right, Dust Puppy!
EOF

     # The boxes to display pages of content
     $gplb = NewWidget(-type => box,-side => top,-bordercolor => 0x000000,-size => 0);
     $copyb = NewWidget(-type => box,-side => all);
     $funb = NewWidget(-type => box,-side => top,-size => 0);

     NewWidget(-type=>label,-side=>left,-transparent=>1,-font=>$bigfont,
	       -text => NewString("About\n$progname"),-inside => $b1);
     NewWidget(-type=>bitmap,-side=>right,-lgop => 'and',-transparent=>1,
	       -bitmap  => NewBitmap(-file => "bigtux.pnm"),-lgop=>'or',
	       -bitmask => NewBitmap(-file => "bigtux_mask.pnm"));

     # The GPL box
     $l1 = NewWidget(-type => label, -text => $gpl,
                     -inside => $gplb,
		     -side => all, -align => nw,
                     -font => NewFont('Courier',12,fixed));
     $sc = NewWidget(-type => scroll,-bind => $l1,-before => $l1);

     # Copyright box
     NewWidget(-type => label,-side => all,-text=>$copyright,
               -transparent => 1,
               -font => NewFont('Courier',12,fixed,bold),-inside => $copyb);

     # The Fun Box
     NewWidget(-type => bitmap,-side => right,-inside => $funb,
               -transparent => 1,
	       -bitmap  => NewBitmap(-file => "dustpuppy.pnm"),-lgop=>'or',
	       -bitmask => NewBitmap(-file => "dustpuppy_mask.pnm"));
     NewWidget(-type => label,-side => all,-text=>$fun,
               -transparent => 1,
               -font => NewFont('Helvetica',10,bold));

     # Toolbar buttons
     NewWidget(-type => button,-bitmap=>$check,-hotkey => $PGKEY{RETURN},
	       -bitmask=>$checkmask,-text=>NewString("Close"),-side=>left,
	       -inside => $tb,-onclick =>  sub {LeaveContext; Update;});

     NewWidget(-type => button,-side=>right,
               -bitmap => $circ, -bitmask => $circmask,
	       -text=>NewString("GPL"),
	       -onclick => sub {
          $gplb->SetWidget(-side => all);
          $copyb->SetWidget(-side => top,-size => 0);
          $funb->SetWidget(-side => top,-size => 0);
          Update;
     });
     
     NewWidget(-type => button,-side=>right,
               -bitmap => $circ, -bitmask => $circmask,
	       -text=>NewString("Fun Stuff"),
	       -onclick => sub {
          $gplb->SetWidget(-side => top,-size => 0);
          $copyb->SetWidget(-side => top,-size => 0);
          $funb->SetWidget(-side => all);
          Update;
     });

     NewWidget(-type => button,-side=>right,
               -bitmap => $circ, -bitmask => $circmask,
	       -text=>NewString("Copyright"),
	       -onclick => sub {
          $gplb->SetWidget(-side => top,-size => 0);
          $copyb->SetWidget(-side => all);
          $funb->SetWidget(-side => top,-size => 0);
          Update;
     });

     NewWidget(-type => label,-side => right,-transparent => 1,
               -text=>NewString("Click one for more information:"));

     Update;

});

############################################################ Main loop and cleanup

EventLoop;
Finis;

############################################################ Subs

sub Finis {
    print "Exiting with clean shutdown...\n";
    RestoreBackground;
    Update;
    exit 0;
}

sub setlesson {
    my ($self) = @_;
    my ($str,$hlname);

    # This text is already stored as a handle, so don't duplicate it
    if ($self) {
	$lessonname->SetWidget(-text => $hlname = $self->GetWidget(-text));
    }
    else {
	$hlname = $lessonname->GetWidget(-text);
    }
    $lname = $hlname->GetString;

    $lessontextscroll->delete if ($lessontextscroll);

    if ($student{$lname}) {
	($ntimes,$bestaccuracy,$bestwpm,
	 $total,$incorrect,$accuracy,$wpm) = split / /,$student{$lname};
	$plural = 's';
	$plural = '' if ($ntimes==1);

	$boxes{$lname}->SetWidget(-bitmap => $greenbox);

	$str = <<EOF;
You have completed "$lname" $ntimes time$plural.

Your most recent scores are:
    $total total keystrokes
    $incorrect errors
    $accuracy% accuracy
    $wpm words per minute

Your best scores for this lesson are:
    $bestaccuracy% accuracy 
    $bestwpm words per minute
EOF
    }
    else {
	$boxes{$lname}->SetWidget(-bitmap => $redbox);

	$str = "You have not completed \"$lname\" yet.";
    }

    $lessoninfo->ReplaceText("\n$str\n");
    $lessontextwidget->ReplaceText($lesson{$lname});

    Update;
}

sub typechar {

    if ($_[1]) {
	$starttime = time() if (!$starttime);

	$c = pack 'c',($_[1] & 0xFF);
	
	if (length $lessontext[$lessonline] == length $str) {
	    if ($c eq "\r") {
		$str = '';
		$lessonline++;
		if (!$lessontext[$lessonline]) {

		    $congrat = '';

		    # Save student data
		    ($ntimes,$bestaccuracy,$bestwpm) = split / /,$student{$lname};
		    if ($accuracy>$bestaccuracy) {
			$diff = $accuracy-$bestaccuracy;
			$bestaccuracy = $accuracy;
			$congrat .= "You improved your accuracy by $diff%\n";
		    }
		    if ($wpm>$bestwpm) {
			$diff = $wpm-$bestwpm;
			$bestwpm = $wpm;
			$congrat .= "You beat your speed record by $diff WPM\n";
		    }
		    $student{$lname} = join(' ',++$ntimes,$bestaccuracy,
					    $bestwpm,$total,$incorrect,$accuracy,$wpm);
		    savestudent();

		    # Go away
		    GiveKeyboard;
		    LeaveContext;
		    undef $tstext;

		    setlesson($currentlessonwidget);
		    		    
		    if ($ntimes!=1 and $congrat) {
			# A cute little dialog box

			EnterContext;
			NewPopup(300,200);
			$tb = NewWidget(-type => toolbar,-side => bottom);

			NewWidget(-type=>label,-transparent=>1, -side=>top,
				  -text=>NewString("\nGood Job!"),-font=>$bigfont);
			NewWidget(-type=>label,-transparent=>1, -side=>all,
				  -text=>NewString($congrat),-font => $boldfont);

			NewWidget(-type => button,-bitmap=>$check,-hotkey => $PGKEY{RETURN},
				  -bitmask=>$checkmask,-text=>NewString("Yay!"),-side=>all,
				  -onclick => sub {LeaveContext; Update;},-inside => $tb);
			
			Update;
		    }

		    return;
		}
		$correct++;
	    }
	    else {
		$incorrect++;
	    }
	}
	
	elsif (substr($lessontext[$lessonline],length $str,1) eq $c) {
	    $str .= $c;
	    $correct++;
	}
	else {
	    $incorrect++;
	}

	if (($currenttime=time()) != $starttime) {
	    # WPM calculations from 1968
	    
	    $wpm = sprintf "%.1f",($correct/5.0) / (($currenttime-$starttime)/60.0);
	}

    }	
    
    if (length $lessontext[$lessonline] > 45) {
	$typespace->SetWidget(-font => $boldfont);
    }
    else {
	$typespace->SetWidget(-font => $bigfont);
    }
    $typespace->ReplaceText($lessontext[$lessonline]."\n".$str.'_');

    $total = $correct+$incorrect;
    if ($total) {
	$accuracy = sprintf "%.1f",100.0 * $correct / $total;
    }
    else {
	$accuracy = '100.0';
    }

    $linenum = $lessonline+1;
    $typeinfo_top->ReplaceText("$lname [line $linenum of $lessonlinetotal]");
    $typeinfo->ReplaceText("Total keys: $total  Errors: $incorrect  Accuracy: $accuracy%  WPM: $wpm");
    Update;
}

sub savestudent {
    return if (!$fname);

    if (open STUDENTF,'>'.$studentdir.$fname) {
	foreach (keys %student) {
	    print STUDENTF "$_:$student{$_}\n";
	}
	close STUDENTF;
    }
    else {
	# This hopefully won't happen, but just in case...
	# This is really stupid, need to add a real error dialog
	# box to picogui.  Should go in a 'composite widget' library
	
	$err = $!;
	
	EnterContext;
	NewPopup(200,100);
	
	NewWidget(-type=>label,-transparent=>1, -side=>all,
		   -text=>NewString("Error saving student data:\n$err"));
	
	Update;
	sleep 5;
	LeaveContext;
	Update;
    }
}

### The End ###




