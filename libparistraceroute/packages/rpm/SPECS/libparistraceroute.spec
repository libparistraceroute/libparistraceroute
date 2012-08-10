 #
 # spec file for package paris-traceroute
 #
 # Copyright (c) 2012 Julian Cromarty julian.cromarty@gmail.com
 #
 # All modifications and additions to the file contributed by third parties
 # remain the property of their copyright owners, unless otherwise agreed
 # upon. The license for this file, and modifications and additions to the
 # file, is the same license as for the pristine package itself (unless the
 # license for the pristine package is not an Open Source License, in which
 # case the license is the MIT License). An "Open Source License" is a
 # license that conforms to the Open Source Definition (Version 1.9)
 # published by the Open Source Initiative.
 
 # Please submit bugfixes or comments via http://bugs.opensuse.org/
 #

# Basic Information
Name:libparistraceroute
Version:1.0
Release:1%{?dist}
Group:Development/Libraries/C and C++
Summary:A library dedicated to designing network measurement tools using crafted probes
License:BSD-2
URL:http://code.google.com/p/paris-traceroute
# Build Information
BuildRoot:%{_tmppath}/%{name}-%{version}-%{release}-root
# Source Information
Source0:paris-traceroute-0.1.tar.bz2
#Source2:AUTHORS
#Patch0:
# Dependency Information
#BuildRequires:gcc binutils libtool autoconf automake
#Requires:
%description
libparistraceroute is a library designed to simplify the process of creating advanced network measurement tools such as ping or traceroute through the use of custom probe packets 
%prep
%setup -q -n paris-traceroute-0.1
%build
%configure
make %{?_smp_mflags} RPM_OPT_FLAGS="$RPM_OPT_FLAGS"
%install
make install-lib DESTDIR=%{buildroot}
%clean
rm -rf %{buildroot}
%post
/sbin/ldconfig
%postun
/sbin/ldconfig
%files
%defattr(-,root,root,-)
%{_libdir}/libparistraceroute-1.0.so.1.0.0
#%{_libdir}/libparistraceroute-1.0.a
%exclude
%{_libdir}/libparistraceroute-1.0.la
%{_libdir}/libparistraceroute-1.0.so
%{_libdir}/libparistraceroute-1.0.so.1
%doc
%changelog
* Wed Jun 20 2012 Julian Cromarty <julian.cromarty@gmail.com> 0.1
- Initial Spec File
