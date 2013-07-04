#
# Pre-setting for libparistraceroute configure script
#
# This file is part of libparistraceroute
#

# Initial information about the project
AC_INIT([paris-traceroute], [0.1], [paris-traceroute@googlegroups.com])

# Define these substitions here to keep all version information in one place.
# For information on how to properly maintain the library version information,
# refer to the libtool manual, section "Updating library version information":
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
API_VERSION=1:0:0
AC_SUBST([LIBRARY_VERSION], [1.0])
AC_SUBST(API_VERSION)

# AC_CANONICAL_SYSTEM

# Check if the source folder is available
AC_CONFIG_SRCDIR([paris-traceroute/paris-traceroute.c])
AC_CONFIG_SRCDIR([traceroute/traceroute.c])

# automake initialization (mandatory) (no check for automake version)
AM_INIT_AUTOMAKE

# Use the C compiler for the following checks
AC_LANG([C])

AC_CONFIG_HEADER([libparistraceroute/config.h])
AC_C_BIGENDIAN([endian="big-endian"], [endian="little-endian"])
AC_CONFIG_MACRO_DIR([m4])

AC_DEFINE([USEPCAP], [1], [Use pcap library])
AC_DEFINE([BSDSOCKET], [1], [BSD socket interface])

#jo#os="unknow"
#jo#usable_pcap="no"
#jo#trpt_udp="yes"
#jo#trpt_tcp="yes"
#jo#trpt_others="yes"
#jo#trpt_buggy="no"

#CFLAGS="$CFLAGS -Wall -fno-strict-aliasing"
CFLAGS="-g -Wall -fno-strict-aliasing  -fno-omit-frame-pointer -g -std=gnu99 -Wall -Wsign-compare -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Wchar-subscripts -Wcast-qual"
# -Werror

#############
## Options ##
#############
# Debugging args
AC_ARG_ENABLE([debug],
	AC_HELP_STRING([--disable-debug], [Don t show debug messages]),
	[debug="no"], [debug="yes"])
AC_ARG_WITH([debug-level],
	AC_HELP_STRING([--with-debug-level],
			[Default versbosity of debug messages]),
	[debug_lvl="$withval"], [debug_lvl=1])

if test "x$debug" = "xyes"; then
	AC_DEFINE([DEBUG], [1], [Add debugging code])
	AC_DEFINE([DEBUG_LEVEL], [1], [Default debug level])
fi

#jo## Transport layer args
#jo#AC_ARG_ENABLE([udp],
#jo#	AC_HELP_STRING([--disable-udp], [Disable UDP support]),
#jo#	[trpt_udp="no"], [trpt_udp="yes"])
#jo#AC_ARG_ENABLE([tcp],
#jo#	AC_HELP_STRING([--disable-tcp], [Disable TCP support]),
#jo#	[trpt_tcp="no"], [trpt_tcp="yes"])
#jo#AC_ARG_ENABLE([anyprot],
#jo#	AC_HELP_STRING([--disable-anyprot], [Disable 'any protocol' support]),
#jo#	[trpt_others="no"], [trpt_others="yes"])
#jo#
#jo## LibPCap args
#jo#AC_ARG_ENABLE([pcap],
#jo#	AC_HELP_STRING([--disable-pcap], [Use raw-sockets instead of pcap]),
#jo#	[use_pcap=false], [use_pcap=yes])
#jo#AC_ARG_WITH([pcap], AC_HELP_STRING([--with-pcap=DIR], [use pcap in DIR]),
#jo#	[pcap_prefix=$withval])

#################################################################################
#
# Checks for programs
#

# Determine a C compiler to use and set CC. Set GCC to yes if compiler is gcc.
AC_PROG_CC
AC_LANG_C
AC_PROG_INSTALL
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

#################################################################################
#
# Checks for libraries
#

# Check for pthread...
#AC_CHECK_LIB([pthread], [pthread_create],,
#	AC_MSG_ERROR("Pthreads not found in -lpthread"))

# Check for libpcap...
#PCAPCC=""
#PCAPLD=""
##if test "x$use_pcap" = "xyes"; then
#        if test -n "$libpcap_prefix"; then
#                test -f "$pcap_prefix/pcap.h" && PCAPCC="$PCAPCC -I$pcap_prefix"
#                test -d "$pcap_prefix/bpf" && PCAPCC="$PCAPCC -I$pcap_prefix/bpf"
#                test -f "$pcap_prefix/include/pcap.h" && PCAPCC="$PCAPCC -I$pcap_prefix/include"
#                test -f "$pcap_prefix/pcap.a" && PCAPLD="$PCAPLD -L$pcap_prefix"
#                test -f "$pcap_prefix/libpcap.a" && PCAPLD="$PCAPLD -L$pcap_prefix"
#                test -f "$pcap_prefix/lib/pcap.a" && PCAPLD="$PCAPLD -L$pcap_prefix/lib"
#                test -f "$pcap_prefix/lib/libpcap.a" && PCAPLD="$PCAPLD -L$pcap_prefix/lib"
#
#                if test -z "$PCAPCC" -o -z "$PCAPLD"; then
#                        AC_MSG_NOTICE(No valid pcap lib found at $pcap_prefix)
#                else
#                        CFLAGS="$PCAPCC $CFLAGS"
#                        LDFLAGS="$PCAPLD $LDFLAGS"
#                        AC_MSG_NOTICE([found lipcap at $pcap_prefix])
#                fi
#        fi
#        AC_CHECK_LIB([pcap], [pcap_open_live], [usable_pcap=yes])
##fi

#################################################################################
#
# Checks for header files
#

# Check for stdlib.h, stdarg.h, string.h and float.h and defines STDC_HEADERS
AC_HEADER_STDC(, "Standard headers missing")
# Check other headers
AC_CHECK_HEADERS([arpa/inet.h netinet/in.h netinet/ip.h sys/socket.h],
	AC_DEFINE([BSDSOCKET], [1], [BSD socket interface])
	[os="unix"])

AC_CHECK_HEADERS([winsock2.h],
	AC_DEFINE([WINSOCKET], [1], [WIN socket 2 interface])
	[os="win32"])

AC_CHECK_HEADERS([netlink/netlink.h net/rtnetlink.h], [os=linux])

AC_CHECK_HEADER([stdlib.h])
AC_CHECK_HEADER([string.h])
AC_CHECK_HEADER([unistd.h])

#################################################################################
#
# Checks for typedefs, structures
#

#AC_HEADER_STDBOOL(, AC_MSG_ERROR("bool missing"))
#AC_C_CONST(, AC_MSG_ERROR("const missing"))
AC_HEADER_TIME(, AC_MSG_ERROR("header time missing"))

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_CHECK_FUNC([inet_ntoa])
AC_CHECK_FUNC([memset])
AC_CHECK_FUNC([socket])
AC_CHECK_FUNC([strdup])
AC_CHECK_FUNC([strerror])

#################################################################################
#
# Compile time options
#

# Some initialization (default implementation to use)
#jo#sender="raw-socket"
#jo#sniffer="raw-socket"
#jo#route="socket"
#jo#SENDER_OBJ="sender.rawsock.lo"
#jo#SNIFFER_OBJ="sniffer.rawsock.lo"
#jo#ROUTE_OBJ="route.socket.lo"
#jo#
#jo## PCap
#jo#if test "x$usable_pcap" = "xyes"; then
#AC_DEFINE([USEPCAP], [1], [Use pcap library])
#jo#	SNIFFER_OBJ="sniffer.pcap.lo"
#LDFLAGS="-lpcap $LDFLAGS"
#jo#	#ROUTE_OBJ="route.pcap.lo"
#jo#	ROUTE_OBJ="route.socket.lo"
#jo#	sniffer="pcap"
#jo#	#route="pcap"
#jo#	route="socket"
#jo#fi
#jo#

# OS-dependant
case "$os" in
	"win32")
		# Windows ldflags
		LDFLAGS="$LDFLAGS -lws2_32"

		# From WinXP SP2, only ICMP and UDP are usable with raw-sockets
		trpt_tcp=no
		trpt_others=no

		# Use Winsocket
		AC_DEFINE([WINSOCKET], [1], [Use winsocket2 interface])

		# Test if pcap is usable
		if test "x$usable_pcap" != "xyes"; then
			AC_MSG_ERROR([WinPCap missing])
		fi
		;;
	
	"linux")
		# Use BSD socket
		AC_DEFINE([BSDSOCKET], [1], [Use BSD socket interface])

		# Use netlink route impl if pcap is not available
		if test "x$usable_pcap" != "xyes"; then
			AC_DEFINE([ROUTE_NETLINK], [1], [Use netlink socket])
			ROUTE_OBJ="route.netlink.lo"
			route="netlink"
		fi
		;;
	
	"bsd")
		# Use BSD socket
		AC_DEFINE([BSDSOCKET], [1], [Use BSD socket interface])

		# Use bsd route socket
		if test "x$usable_pcap" != "xyes"; then
			AC_DEFINE([ROUTE_BSD], [1], [Use BSD route socket])
			ROUTE_OBJ="route.bsd.lo"
			trtp_buggy=yes
			route="bsd"
		fi
		;;
	
	"unix")
		# Use BSD socket
		AC_DEFINE([BSDSOCKET], [1], [Use BSD socket interface])
		;;
	
	"*")
		# Use BSD socket
		AC_DEFINE([BSDSOCKET], [1], [Use BSD socket interface])
		# Unsupported
		AC_MSG_NOTICE(["Unsupported OS, compilation or exec may fail"])
		;;
esac

#jo## Defines
#jo#AC_SUBST([SENDER_OBJ])
#jo#AC_SUBST([SNIFFER_OBJ])
#jo#AC_SUBST([ROUTE_OBJ])
#jo#
#jo#transport="ICMP"
#jo#if test "x$trpt_udp" = "xyes"; then
#jo#	AC_DEFINE([TRTP_UDP], [1], [Use UDP probes])
#jo#	transport="$transport UDP"
#jo#fi
#jo#if test "x$trpt_tcp" = "xyes"; then
#jo#	AC_DEFINE([TRTP_TCP], [1], [Use TCP probes])
#jo#	transport="$transport TCP"
#jo#fi
#jo#if test "x$trpt_others" = "xyes"; then
#jo#	AC_DEFINE([TRTP_OTHERS], [1], [Use probes with custom protocol])
#jo#	transport="$transport Others"
#jo#fi
#jo#if test "x$trpt_buggy" = "xyes"; then
#jo#	transport="$transport (buggy, use pcap impl. if possible)"
#jo#fi

## Output Makefiles
# Override the template file name of the generated .pc file, so that there
# is no need to rename the template file when the API version changes.
AC_CONFIG_FILES(
	[Makefile]
    [libparistraceroute/libparistraceroute-$LIBRARY_VERSION.pc:libparistraceroute/libparistraceroute.pc.in]
	[libparistraceroute/Makefile]
	[paris-traceroute/Makefile]
	[traceroute/Makefile]
	[man/Makefile]
	[doc/Makefile]
)
AC_OUTPUT

echo "   Results"
echo "   ~~~~~~~"
echo "Probe sender:     $sender"
echo "Network sniffer:  $sniffer"
echo "Route impl.:      $route"
echo "Protocol:         IPv4"
echo "Transport:        $transport"
echo "Debug code:       $debug"
echo "Debug level:      $debug_lvl"
echo "Endianess:        $endian"
echo "OS:               $os"
echo "CFLAGS:           $CFLAGS"
echo "LDFLAGS:          $LDFLAGS"

