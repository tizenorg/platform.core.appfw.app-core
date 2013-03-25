Name:       app-core
Summary:    Application basic
Version:    1.2
Release:    42
Group:      Application Framework
License:    Apache License, Version 2.0
Source0:    app-core-%{version}.tar.gz
Source101:  packaging/core-efl.target
BuildRequires:  pkgconfig(sensor)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(rua)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(gobject-2.0)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  cmake


%description
SLP common application basic



%package efl
Summary:    App basic EFL
Group:      Development/Libraries
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description efl
Application basic EFL

%package efl-devel
Summary:    App basic EFL (devel)
Group:      Development/Libraries
Requires:   %{name}-efl = %{version}-%{release}
Requires:   %{name}-common-devel = %{version}-%{release}

%description efl-devel
Application basic EFL (devel)

%package common
Summary:    App basics common
Group:      Development/Libraries
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description common
Application basics common

%package common-devel
Summary:    App basics common (devel)
Group:      Development/Libraries
Requires:   %{name}-common = %{version}-%{release}
Requires:   pkgconfig(sensor)
Requires:   pkgconfig(vconf)
Requires:   pkgconfig(elementary)
Requires:   pkgconfig(aul)

%description common-devel
Application basics common (devel)

%package template
Summary:    App basics template
Group:      Development/Libraries

%description template
Application basics template


%prep
%setup -q 

%build
%cmake . -DENABLE_GTK=OFF


make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
install -d %{buildroot}%{_prefix}/lib/systemd/user/core-efl.target.wants
install -m0644 %{SOURCE101} %{buildroot}%{_prefix}/lib/systemd/user/


%post efl -p /sbin/ldconfig

%postun efl -p /sbin/ldconfig

%post common -p /sbin/ldconfig

%postun common -p /sbin/ldconfig





%files efl
%manifest app-core.manifest
%defattr(-,root,root,-)
%{_libdir}/libappcore-efl.so.*

%files efl-devel
%defattr(-,root,root,-)
%{_includedir}/appcore/appcore-efl.h
%{_libdir}/libappcore-efl.so
%{_libdir}/pkgconfig/appcore-efl.pc

%files common
%manifest app-core.manifest
%defattr(-,root,root,-)
%{_libdir}/libappcore-common.so.*
%{_prefix}/lib/systemd/user/core-efl.target
%{_prefix}/lib/systemd/user/core-efl.target.wants/

%files common-devel
%defattr(-,root,root,-)
%{_libdir}/libappcore-common.so
%{_libdir}/pkgconfig/appcore-common.pc
%{_includedir}/appcore/appcore-common.h
%{_includedir}/SLP_Appcore_PG.h

