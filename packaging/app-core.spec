%bcond_with x
%bcond_with wayland

Name:       app-core
Summary:    Application basic
Version:    1.2
Release:    48
Group:      Application Framework
License:    Apache-2.0
Source0:    app-core-%{version}.tar.gz
%if %{with wayland} && !%{with x}
Source101:  packaging/core-efl-wayland.target
%else
Source101:  packaging/core-efl-x.target
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(ecore-x)
%endif
Source1001: 	app-core.manifest
BuildRequires:  pkgconfig(sensor)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(rua)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(gobject-2.0)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(libtzplatform-config)
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
%if %{with x}
Requires:  pkgconfig(x11)
%endif


%description common-devel
Application basics common (devel)

%package template
Summary:    App basics template
Group:      Development/Libraries

%description template
Application basics template


%prep
%setup -q 
cp %{SOURCE1001} .

%build
%cmake . \
%if %{with wayland} && !%{with x}
-Dwith_wayland=TRUE\
%endif
-DENABLE_GTK=OFF

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
install -d %{buildroot}%{_unitdir_user}/core-efl.target.wants
install -m0644 %{SOURCE101} %{buildroot}%{_unitdir_user}/core-efl.target

mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}-efl
cp LICENSE %{buildroot}/usr/share/license/%{name}-efl-devel
cp LICENSE %{buildroot}/usr/share/license/%{name}-common
cp LICENSE %{buildroot}/usr/share/license/%{name}-common-devel

%post efl -p /sbin/ldconfig

%postun efl -p /sbin/ldconfig

%post common -p /sbin/ldconfig

%postun common -p /sbin/ldconfig





%files efl
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/libappcore-efl.so.*
/usr/share/license/%{name}-efl

%files efl-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/appcore/appcore-efl.h
%{_libdir}/libappcore-efl.so
%{_libdir}/pkgconfig/appcore-efl.pc
/usr/share/license/%{name}-efl-devel

%files common
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/libappcore-common.so.*
%{_prefix}/lib/systemd/user/core-efl.target
%{_prefix}/lib/systemd/user/core-efl.target.wants/
/usr/share/license/%{name}-common

%files common-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/libappcore-common.so
%{_libdir}/pkgconfig/appcore-common.pc
%{_includedir}/appcore/appcore-common.h
%{_includedir}/SLP_Appcore_PG.h
/usr/share/license/%{name}-common-devel

