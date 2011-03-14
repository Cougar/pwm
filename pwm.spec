
BuildRequires:	xorg-x11-devel

Summary:	The first window manager to introduce tabbing at the window manager level
Name:		pwm
Version:	20070720
Release:	2
License:	the Clarified Artistic License or the GNU GPL version 2 or later
URL:		http://tuomov.iki.fi/software
Group:		System/GUI/Other
Source:		%{name}-%{version}.tar.bz2
Buildroot:	/var/tmp/%{name}-root

%description
PWM is a rather lightweight window manager that can have multiple client
windows attached to a single frame. This feature helps keeping windows,
especially the numerous xterms, organized.

See http://tuomov.iki.fi/software for new releases.

%prep

%setup

%build
make

%install
make BINDIR=$RPM_BUILD_ROOT/usr/bin ETCDIR=$RPM_BUILD_ROOT/etc MANDIR=$RPM_BUILD_ROOT/%{_mandir} DOCDIR=$RPM_BUILD_ROOT/usr/share/doc/packages/pwm install

%files
%defattr(-,root,root)
%doc config.txt LICENSE
%doc %{_mandir}/man?/*
%dir /etc/pwm
%config /etc/pwm/*
/usr/bin/pwm

%changelog
* Mon Mar 14 2011 cougar@random.ee
- updated to pwm-20070720
