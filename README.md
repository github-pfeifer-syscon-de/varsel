# varsel
this is far from being useful.

Some integration of the desktop tools i use most
e.g. shell, file-browser, editor, ...

If you are looking for some example how to
integrate vte or sourceview with gtkmm
this might provide some basic insight...

## Setup

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

## Config

The terminal uses fixed keybindings at the moment:

F9 open from terminal (with a selection it will be tried to be used as a filename)