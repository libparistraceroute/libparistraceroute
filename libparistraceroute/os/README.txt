--------------------------------------------------------------------
Overview
--------------------------------------------------------------------

This directory contains header to abstract the underlying operating system.

We mimic the hierarchy of a GNU/Linux Debian.

As a result, the programmer has to include "os/x.h" instead of <x.h> if
the header x.h is not "fully" cross platform.

--------------------------------------------------------------------
Why ?
--------------------------------------------------------------------

- Some standard headers may exists let say on FreeBSD and Debian.  However they
may include different headers internally. These headers will also avoid these
consideration in order to guarantee that the behaviour is the same for all the
operating systems.

- If the design is too different among the OS, we will implement MACROS in these
headers that will be called in libparistraceroute instead of the native
function.

--------------------------------------------------------------------
Philosophy:
--------------------------------------------------------------------

1) We always include "../sys.h" which allow to get a #define depending on the
  underlying Operating system.

2) We split the file in section, one per target OS.

#ifdef LINUX
//...
#endif

#ifdef FREEBSD
//...
#endif

//...

- GNU/Linux:
	- we basically include the corresponding linux header.
- Other OSes:
	- if the header also exists on the considered OS, we include the header
	  normally included by the Linux header. (e.g: <sys/types.h> is included by
	  <netinet/ip_icmp.h> under linux, not on FreeBSD.
	- otherwise we rely on macros to adopt a uniform code in libparistraceroute. 
	  And as a result we'll do the same in the #ifdef LINUX ... #endif section. 

