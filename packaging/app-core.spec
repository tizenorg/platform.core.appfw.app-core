
Name:       app-core
Summary:    Application basic
Version:    1.2
Release:    25
Group:      TO_BE/FILLED_IN
License:    Apache License, Version 2.0
Source0:    app-core-%{version}.tar.gz
BuildRequires:  pkgconfig(sensor)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(rua)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(sysman)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(gobject-2.0)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  cmake
BuildRequires:  sysman-devel


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
CFLAGS="-I/usr/lib/glib-2.0/include/ -I/usr/include/glib-2.0 -I/usr/lib/dbus-1.0/include -I/usr/include/dbus-1.0 -I/usr/include/e_dbus-1 -I/usr/include/ethumb-0 -I/usr/include/edje-1 -I/usr/include/efreet-1 -I/usr/include/embryo-1 -I/usr/include/ecore-1 -I/usr/include/eet-1 -I/usr/include/evas-1 -I/usr/include/eina-1 -I/usr/include/eina-1/eina  $(CFLAGS)" cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DENABLE_GTK=OFF


make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install


%post efl -p /sbin/ldconfig

%postun efl -p /sbin/ldconfig

%post common -p /sbin/ldconfig

%postun common -p /sbin/ldconfig





%files efl
%defattr(-,root,root,-)
%{_libdir}/libappcore-efl.so.*

%files efl-devel
%defattr(-,root,root,-)
%{_includedir}/appcore/appcore-efl.h
%{_libdir}/libappcore-efl.so
%{_libdir}/pkgconfig/appcore-efl.pc

%files common
%defattr(-,root,root,-)
%{_libdir}/libappcore-common.so.*

%files common-devel
%defattr(-,root,root,-)
%{_libdir}/libappcore-common.so
%{_libdir}/pkgconfig/appcore-common.pc
%{_includedir}/appcore/appcore-common.h
%{_includedir}/SLP_Appcore_PG.h

