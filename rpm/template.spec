Name:           ros-melodic-mrpt-icp-slam-2d
Version:        0.1.10
Release:        1%{?dist}
Summary:        ROS mrpt_icp_slam_2d package

Group:          Development/Libraries
License:        BSD
URL:            http://ros.org/wiki/mrpt_icp_slam_2d
Source0:        %{name}-%{version}.tar.gz

Requires:       ros-melodic-dynamic-reconfigure
Requires:       ros-melodic-mrpt-bridge
Requires:       ros-melodic-mrpt-rawlog
Requires:       ros-melodic-mrpt1
Requires:       ros-melodic-nav-msgs
Requires:       ros-melodic-roscpp
Requires:       ros-melodic-roslaunch
Requires:       ros-melodic-roslib
Requires:       ros-melodic-rviz
Requires:       ros-melodic-sensor-msgs
Requires:       ros-melodic-std-msgs
Requires:       ros-melodic-tf
Requires:       ros-melodic-visualization-msgs
BuildRequires:  ros-melodic-catkin
BuildRequires:  ros-melodic-dynamic-reconfigure
BuildRequires:  ros-melodic-mrpt-bridge
BuildRequires:  ros-melodic-mrpt1
BuildRequires:  ros-melodic-nav-msgs
BuildRequires:  ros-melodic-roscpp
BuildRequires:  ros-melodic-roslaunch
BuildRequires:  ros-melodic-roslib
BuildRequires:  ros-melodic-sensor-msgs
BuildRequires:  ros-melodic-std-msgs
BuildRequires:  ros-melodic-tf
BuildRequires:  ros-melodic-visualization-msgs

%description
mrpt_icp_slam_2d contains a wrapper on MRPT's 2D ICP-SLAM algorithms.

%prep
%setup -q

%build
# In case we're installing to a non-standard location, look for a setup.sh
# in the install tree that was dropped by catkin, and source it.  It will
# set things like CMAKE_PREFIX_PATH, PKG_CONFIG_PATH, and PYTHONPATH.
if [ -f "/opt/ros/melodic/setup.sh" ]; then . "/opt/ros/melodic/setup.sh"; fi
mkdir -p obj-%{_target_platform} && cd obj-%{_target_platform}
%cmake .. \
        -UINCLUDE_INSTALL_DIR \
        -ULIB_INSTALL_DIR \
        -USYSCONF_INSTALL_DIR \
        -USHARE_INSTALL_PREFIX \
        -ULIB_SUFFIX \
        -DCMAKE_INSTALL_LIBDIR="lib" \
        -DCMAKE_INSTALL_PREFIX="/opt/ros/melodic" \
        -DCMAKE_PREFIX_PATH="/opt/ros/melodic" \
        -DSETUPTOOLS_DEB_LAYOUT=OFF \
        -DCATKIN_BUILD_BINARY_PACKAGE="1" \

make %{?_smp_mflags}

%install
# In case we're installing to a non-standard location, look for a setup.sh
# in the install tree that was dropped by catkin, and source it.  It will
# set things like CMAKE_PREFIX_PATH, PKG_CONFIG_PATH, and PYTHONPATH.
if [ -f "/opt/ros/melodic/setup.sh" ]; then . "/opt/ros/melodic/setup.sh"; fi
cd obj-%{_target_platform}
make %{?_smp_mflags} install DESTDIR=%{buildroot}

%files
/opt/ros/melodic

%changelog
* Sat Oct 05 2019 Jose Luis Blanco Claraco <jlblanco@ual.es> - 0.1.10-1
- Autogenerated by Bloom

* Sun Apr 14 2019 Jose Luis Blanco Claraco <jlblanco@ual.es> - 0.1.9-0
- Autogenerated by Bloom

* Fri Sep 21 2018 Jose Luis Blanco Claraco <jlblanco@ual.es> - 0.1.8-0
- Autogenerated by Bloom

* Wed Jul 25 2018 Jose Luis Blanco Claraco <jlblanco@ual.es> - 0.1.6-0
- Autogenerated by Bloom

