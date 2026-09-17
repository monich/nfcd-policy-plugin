Name: nfcd-policy-plugin

Version: 1.0.0
Release: 0
Summary: nfcd plugin for NFC policy tracking
License: BSD
URL: https://github.com/sailfishos/nfcd-policy-plugin
Source: %{name}-%{version}.tar.bz2

%define nfcd_version 1.2.7

BuildRequires: pkgconfig
BuildRequires: pkgconfig(libglibutil)
BuildRequires: pkgconfig(nfcd-plugin) >= %{nfcd_version}

Requires: nfcd >= %{nfcd_version}

%define plugin_dir %{_libdir}/nfcd/plugins

%description
%{summary}.

%prep
%setup -q

%build
make %{_smp_mflags} release

%install
%make_build DESTDIR=%{buildroot} PLUGIN_DIR=%{plugin_dir} install

%post
systemctl reload-or-try-restart nfcd.service ||:

%postun
systemctl reload-or-try-restart nfcd.service ||:

%files
%dir %{plugin_dir}
%{plugin_dir}/*.so
%license LICENSE
