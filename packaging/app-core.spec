%bcond_with x
%bcond_with wayland

Name:           app-core
Summary:        Application basic
Version:        1.2
Release:        0
Group:          Application Framework/Libraries
License:        Apache-2.0
Source0:        app-core-%{version}.tar.gz
%if %{with x}
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(eina)
%else
%if %{with wayland}
BuildRequires:  pkgconfig(ecore-wayland)
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  pkgconfig(tizen-extension-client)
BuildRequires:  pkgconfig(wayland-tbm-client)
%endif
%endif
Source1001:     app-core.manifest
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(sensor)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(rua)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(gobject-2.0)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(pkgmgr-info)
BuildRequires:  pkgconfig(ttrace)
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
Requires:   pkgconfig(x11)
%endif

%description common-devel
Application basics common (devel)


%package template
Summary:    App basics template
Group:      Development/Libraries

%description template
Application basics template

%if "%{?profile}" == "wearable"
%define appfw_feature_background_management 1
%else
%if "%{?profile}" == "mobile"
%define appfw_feature_background_management 1
%else
%if "%{?profile}" == "tv"
%define appfw_feature_background_management 0
%endif
%endif
%endif

%prep
%setup -q
cp %{SOURCE1001} .

%build
%if %{with wayland}
_WITH_WAYLAND=ON
%endif
%if %{with x}
_WITH_X11=ON
%endif
%if 0%{?appfw_feature_background_management}
_APPFW_FEATURE_BACKGROUND_MANAGEMENT=ON
%endif

%cmake . \
	-D_WITH_WAYLAND:BOOL=${_WITH_WAYLAND} \
	-D_WITH_X11:BOOL=${_WITH_X11} \
	-D_APPFW_FEATURE_BACKGROUND_MANAGEMENT:BOOL=${_APPFW_FEATURE_BACKGROUND_MANAGEMENT} \
	-DENABLE_GTK=OFF

make %{?_smp_mflags}


%install
rm -rf %{buildroot}
%make_install


%post -n app-core-efl -p /sbin/ldconfig

%postun -n app-core-efl -p /sbin/ldconfig

%post -n app-core-common -p /sbin/ldconfig

%postun -n app-core-common -p /sbin/ldconfig


%files efl
%manifest %{name}.manifest
%{_libdir}/libappcore-efl.so.*
%license LICENSE

%files efl-devel
%manifest %{name}.manifest
%{_includedir}/appcore/appcore-efl.h
%{_libdir}/libappcore-efl.so
%{_libdir}/pkgconfig/appcore-efl.pc

%files common
%manifest %{name}.manifest
%{_libdir}/libappcore-common.so.*
%license LICENSE

%files common-devel
%manifest %{name}.manifest
%{_libdir}/libappcore-common.so
%{_libdir}/pkgconfig/appcore-common.pc
%{_includedir}/appcore/appcore-common.h
%{_includedir}/SLP_Appcore_PG.h
