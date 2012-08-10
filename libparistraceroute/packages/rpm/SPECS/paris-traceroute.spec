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
Name:paris-traceroute
Version:0.1
Release:1%{?dist}
Group:Development/Libraries/C and C++
Summary:A new version of the traceroute algorithm that can handle load balancing
License:BSD-2
URL:http://paris-traceroute.net
# Build Information
BuildRoot:%{_tmppath}/%{name}-%{version}-%{release}-root
# Source Information
Source0:paris-traceroute-0.1.tar.bz2
#Source2:AUTHORS
#Patch0:
# Dependency Information
#BuildRequires:gcc binutils libtool automake autoconf
Requires:libparistraceroute
%description
Paris traceroute is a new version of the well-known network diagnosis and measurement tool. It addresses problems caused by load balancers with the initial implementation of traceroute.
%prep
%setup -q
%build
%configure
make %{?_smp_mflags} RPM_OPT_FLAGS="$RPM_OPT_FLAGS"
%install
make install-bin DESTDIR=%{buildroot}
%clean
rm -rf %{buildroot}
%post
/sbin/ldconfig
%postun
/sbin/ldconfig
%files
%defattr(-,root,root,-)
/usr/bin/paris-traceroute
#/usr/bin/traceroute
%{_mandir}/man1/paris-traceroute.1*
%doc
%changelog
* Wed Jul 4 2012 Julian Cromarty <julian.cromarty@gmail.com> 0.1
- Initial Spec File
- Forgot source0 definition
- Fixed changelog date format
- Added installed files and dirs to files section
- Added group, description and moved defattrs to correct place
- Removed development files
- Removed libraries
