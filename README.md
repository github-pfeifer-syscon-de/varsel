# varsel
this is far from being useful.

Some integration of the desktop tools i use most
e.g. shell, file-browser, editor, ...

If you are looking for some example how to
integrate vte or sourceview with gtkmm
this might provide some basic insight...

## Terminal Setup

The terminal needs to support directory detection
so you have to include in e.g. ~/.bashrc
after this part:
<pre>
# If not running interactively, don't do anything
[[ $- != *i* ]] && return
</pre>
this call:
<pre>
. /etc/profile.d/vte.sh
</pre>

## Editor Setup

The terminal uses fixed keybindings at the moment:

F9 open from terminal (with a selection it will be tried to be used as a filename)

### Source edit language server support

A simple implementation for the language server protocol was
added to the editor.
As a starting point ccls is added by default.
To make ccls work, a .ccls file is needed in
the source directory.
It can be created by the make2ccls.sh
script (it is included in genericImg). Use:
<pre>
cd genericImg/src
../make2ccls.sh
</pre>
If the file was created in genericImg/src it is used
as a test case here. And of course a ccls install provided
by your distro is required as well.

At the moment these two key-bindings are used:

Ctrl-D go to definition

Ctrl-Shift-D go to declation

Other implemenations for the language server protocol
exist but i only tested ccls
(ccls was not consistenly able to complete indexing when
the parallel mode was used so the contained settings
use single thread mode).
Idexing will take some time this is indicated by
the progress at the bottom of the window
(using language server function befor this has
completed will probably not work).

#### Language server config

To add or change use the language sections in
in "va_edit.conf" in ".config" in your home directory.
The "execute" value is split befor execution, this
allows passing parameters, but as spaces are used to
separate parameters you probably want to avoid them
whenever possible e.g. Json-Values can be written without.
To give a parameter that contains spaces quotes " can used
e.g. ... "This will be one parameter" ...
(will be passed without quotes).
To place a quote you need to double it e.g. "".

