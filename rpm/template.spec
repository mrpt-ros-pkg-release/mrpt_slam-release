Name:           ros-jade-mrpt-ekf-slam-2d
Version:        0.1.3
Release:        0%{?dist}
Summary:        ROS mrpt_ekf_slam_2d package

Group:          Development/Libraries
License:        BSD
URL:            http://ros.org/wiki/mrpt_ekf_slam_2d
Source0:        %{name}-%{version}.tar.gz

Requires:       mrpt-devel
Requires:       ros-jade-dynamic-reconfigure
Requires:       ros-jade-mrpt-bridge
Requires:       ros-jade-nav-msgs
Requires:       ros-jade-roscpp
Requires:       ros-jade-roslib
Requires:       ros-jade-rospy
Requires:       ros-jade-sensor-msgs
Requires:       ros-jade-std-msgs
Requires:       ros-jade-tf
BuildRequires:  mrpt-devel
BuildRequires:  ros-jade-catkin
BuildRequires:  ros-jade-dynamic-reconfigure
BuildRequires:  ros-jade-mrpt-bridge
BuildRequires:  ros-jade-nav-msgs
BuildRequires:  ros-jade-roscpp
BuildRequires:  ros-jade-roslib
BuildRequires:  ros-jade-rospy
BuildRequires:  ros-jade-sensor-msgs
BuildRequires:  ros-jade-std-msgs
BuildRequires:  ros-jade-tf
BuildRequires:  ros-jade-visualization-msgs

%description
mrpt_ekf_slam_2d

%prep
%setup -q

%build
# In case we're installing to a non-standard location, look for a setup.sh
# in the install tree that was dropped by catkin, and source it.  It will
# set things like CMAKE_PREFIX_PATH, PKG_CONFIG_PATH, and PYTHONPATH.
if [ -f "/opt/ros/jade/setup.sh" ]; then . "/opt/ros/jade/setup.sh"; fi
mkdir -p obj-%{_target_platform} && cd obj-%{_target_platform}
%cmake .. \
        -UINCLUDE_INSTALL_DIR \
        -ULIB_INSTALL_DIR \
        -USYSCONF_INSTALL_DIR \
        -USHARE_INSTALL_PREFIX \
        -ULIB_SUFFIX \
        -DCMAKE_INSTALL_LIBDIR="lib" \
        -DCMAKE_INSTALL_PREFIX="/opt/ros/jade" \
        -DCMAKE_PREFIX_PATH="/opt/ros/jade" \
        -DSETUPTOOLS_DEB_LAYOUT=OFF \
        -DCATKIN_BUILD_BINARY_PACKAGE="1" \

make %{?_smp_mflags}

%install
# In case we're installing to a non-standard location, look for a setup.sh
# in the install tree that was dropped by catkin, and source it.  It will
# set things like CMAKE_PREFIX_PATH, PKG_CONFIG_PATH, and PYTHONPATH.
if [ -f "/opt/ros/jade/setup.sh" ]; then . "/opt/ros/jade/setup.sh"; fi
cd obj-%{_target_platform}
make %{?_smp_mflags} install DESTDIR=%{buildroot}

%files
/opt/ros/jade

%changelog
* Tue Sep 27 2016 Jose Luis <jlblanco@ual.es> - 0.1.3-0
- Autogenerated by Bloom

* Sat Sep 24 2016 Jose Luis <jlblanco@ual.es> - 0.1.2-0
- Autogenerated by Bloom

* Mon Aug 22 2016 Jose Luis <jlblanco@ual.es> - 0.1.1-0
- Autogenerated by Bloom

