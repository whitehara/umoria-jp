%global debug_package %{nil}

%if 0%{?suse_version} || 0%{?rhel} == 8
%define __builddir %{_vpath_builddir}
%endif

Name:           umoria-jp
Version:        5.7.15
Release:        1%{?dist}
Summary:    	Umoria %{version} (Japanese localization / 日本語版)

License:        GPL-3.0
URL:            https://github.com/whitehara/umoria-jp
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  ncurses-devel gcc-c++ cmake
Requires:       ncurses-libs


%description
The Dungeons of Moria is a single player dungeon simulation originally written by Robert Alan Koeneke, with its first public release in 1983.
The game was originally developed using VMS Pascal before being ported to the C language by James E. Wilson in 1988, and released a Umoria.
This package is a Japanese localization fork of Umoria (日本語版).


%prep
%autosetup


%build

%if 0%{?rhel} == 8
mkdir %{_vpath_builddir} && (cd %{_vpath_builddir}; %cmake ..; %cmake_build )
%else
%cmake
%cmake_build
%endif


%install
mkdir -p $RPM_BUILD_ROOT/%{_bindir}
cp %{_vpath_builddir}/umoria/umoria $RPM_BUILD_ROOT/%{_bindir}/%{name}.bin
mkdir -p $RPM_BUILD_ROOT/%{_datadir}/games/%{name}
cp -R %{_vpath_builddir}/umoria/data $RPM_BUILD_ROOT/%{_datadir}/games/%{name}
mkdir -p $RPM_BUILD_ROOT/%{_docdir}/%{name}/historical
cp historical/* $RPM_BUILD_ROOT/%{_docdir}/%{name}/historical

cat << EOF > $RPM_BUILD_ROOT/%{_bindir}/%{name}
#!/bin/sh
CONFDIR=~/.config/%{name}
DATADIR=%{_datadir}/games/%{name}/data
BIN=%{_bindir}/%{name}.bin
[ ! -d \$CONFDIR ] && mkdir -p \$CONFDIR && ln -s \$DATADIR \$CONFDIR/data
[ ! -f \$CONFDIR/scores.dat ] && touch \$CONFDIR/scores.dat
(cd \$CONFDIR; \$BIN \$@)
EOF

chmod +x $RPM_BUILD_ROOT/%{_bindir}/%{name}

%postun
echo "Please remove each user's ~/.config/%{name} manually, if you need."

%files
%{_bindir}/%{name}*
%{_datadir}/games/%{name}/data/*
%{_docdir}/%{name}/historical/*
%license LICENSE
%doc *.md AUTHORS



%changelog
* Mon Jul 27 2026 whitehara <whitehara@users.noreply.github.com> - 5.7.15-1
- Rename package to umoria-jp for Japanese localization fork

* Sat Dec 30 2023 whitehara <whitehara@users.noreply.github.com> - 5.7.15-3
- Enable roguelike keys via the CLI

* Sat Jul 1 2023 whitehara <whitehara@users.noreply.github.com> - 5.7.15-2
- Add files in "historical" to doc (Thanks, Justin Koh)

* Sat Feb 18 2023 whitehara <whitehara@users.noreply.github.com> -5.7.15-1
- Add .spec file
