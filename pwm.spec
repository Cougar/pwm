
BuildRequires:	xorg-x11-devel

Summary:	PWM is a light window manager that can have multiple windows attached to a single frame.
Name:		pwm
Version:	20060517
Release:	3
License:	The "Artistic License"
Group:		System/GUI/Other
Source:		%{name}-%{version}.tar.gz
Buildroot:	/var/tmp/%{name}-root

%description
PWM is a rather lightweight window manager that can have multiple client
windows attached to a single frame. This feature helps keeping windows,
especially the numerous xterms, organized.

See http://modeemi.cs.tut.fi/~tuomov/pwm/ for new releases.

%prep

%setup

%build
make

%install
make BINDIR=$RPM_BUILD_ROOT/usr/bin ETCDIR=$RPM_BUILD_ROOT/etc MANDIR=$RPM_BUILD_ROOT/%{_mandir} DOCDIR=$RPM_BUILD_ROOT/usr/share/doc/packages/pwm install

%files
%doc config.txt LICENSE
%doc %{_mandir}/man?/*
%config /etc/pwm/*
/usr/bin/pwm
