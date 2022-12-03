# Userspace program
The userspace program is separated into two applications/executables in order to learn something about inter process communication (IPC). Using a modern linux system, I choose DBus to let the two services communicate with each other. I am aware that there are much simpler ways to let the two processes talk to each other (or write just one application with two threads) but with the aim of learning new technologies and architecture ideas, I choose to use DBus.

## IPC with DBus
+ DBus is a publisher-subscriber communication bus that has two kinds of busses: The *system bus* and the *session bus*
    - *System bus*: Lifes on the system and is always active if the Dbus-Daemon is running. For example the creation of a new device file is detected by the file manager
    - *Session bus*: Lifes on every user session individually. Used by all programs that run on the user interaction layer
+ Dbus is used (as an example) to show all user and system applications that an external hardware device is connected to the system.
    - E.g. if we plug in an USB-stick, all user applications like _nautilus_ and `df` know that the USB-stick is connected to the system.
+ Convenience tooling: d-feet (Install it with `$ apt install d-feet`)
+ DBus is mainly used on the distributions _GNOME_ and _KDE_. Alternatives are _corba_ and _DCOM_
+ DBus implements a BUS system where every connected service can deliver functions or receive/send messages
+ The DBus configuration can be found in 
    - `/etc/dbus-1/system.d`: Configuration of the system bus
    - `/etc/dbus-1/session.d`: Configuration of the session bus
+ Dbus support is given natively by the creator (freedesktop) by the glibc, python3 and QT
    - https://dbus.freedesktop.org/doc/dbus-python/tutorial.html - on default python3, dbus is commonly preinstalled!
    - https://docs.gtk.org/gio/migrating-gdbus.html

## References
[1] 
[2] 
[3] 
[4] 