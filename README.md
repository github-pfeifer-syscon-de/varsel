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
added to the editor. For this to work a .ccls file is needed in
the source directory. It can be created by the make2ccls.sh
script (it is included in genericImg). Use:
<pre>
cd genericImg/src
../make2ccls.sh
</pre>
If the file was created in genericImg/src it is used
as a test case here. And of course a ccls install provided
by your distro is required as well.

Ctrl-D go to declation

Ctrl-Shift-D go to definition

Other implemenations for the language server protocol exist
but i tested only ccls.
To change use the setting "languageServerArgs"
in "va_edit.conf" in ".config" in home directory.
