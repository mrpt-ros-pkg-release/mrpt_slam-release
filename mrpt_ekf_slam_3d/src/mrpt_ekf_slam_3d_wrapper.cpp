/*
 * File: mrpt_ekf_slam_3d_wrapper.cpp
 * Author: Vladislav Tananaev
 *
 */

#include "mrpt_ekf_slam_3d/mrpt_ekf_slam_3d_wrapper.h"

#include <mrpt/version.h>
#if MRPT_VERSION >= 0x199
#include <mrpt/serialization/CArchive.h>
#endif

EKFslamWrapper::EKFslamWrapper() {
  rawlog_play_ = false;
  mrpt_bridge::convert(ros::Time(0), timeLastUpdate_);
}
EKFslamWrapper::~EKFslamWrapper() {}
bool EKFslamWrapper::is_file_exists(const std::string &name) {
  std::ifstream f(name.c_str());
  return f.good();
}

void EKFslamWrapper::get_param() {
  ROS_INFO("READ PARAM FROM LAUNCH FILE");
  n_.param<double>("ellipse_scale", ellipse_scale_, 1);
  ROS_INFO("ellipse_scale: %f", ellipse_scale_);

  n_.param<double>("rawlog_play_delay", rawlog_play_delay, 0.1);
  ROS_INFO("rawlog_play_delay: %f", rawlog_play_delay);

  n_.getParam("rawlog_filename", rawlog_filename);
  ROS_INFO("rawlog_filename: %s", rawlog_filename.c_str());

  n_.getParam("ini_filename", ini_filename);
  ROS_INFO("ini_filename: %s", ini_filename.c_str());

  n_.param<std::string>("global_frame_id", global_frame_id, "map");
  ROS_INFO("global_frame_id: %s", global_frame_id.c_str());

  n_.param<std::string>("odom_frame_id", odom_frame_id, "odom");
  ROS_INFO("odom_frame_id: %s", odom_frame_id.c_str());

  n_.param<std::string>("base_frame_id", base_frame_id, "base_link");
  ROS_INFO("base_frame_id: %s", base_frame_id.c_str());

  n_.param<std::string>("sensor_source", sensor_source, "scan");
  ROS_INFO("sensor_source: %s", sensor_source.c_str());
}
void EKFslamWrapper::init() {
  // get parameters from ini file
  if (!is_file_exists(ini_filename)) {
    ROS_ERROR_STREAM("CAN'T READ INI FILE");
    return;
  }

  EKFslam::read_iniFile(ini_filename);
  // read rawlog file if it  exists
  if (is_file_exists(rawlog_filename)) {
    ROS_WARN_STREAM("PLAY FROM RAWLOG FILE: " << rawlog_filename.c_str());
    rawlog_play_ = true;
  }

  state_viz_pub_ =
	  n_.advertise<visualization_msgs::MarkerArray>("/state_viz", 1); // map
  data_association_viz_pub_ = n_.advertise<visualization_msgs::MarkerArray>(
	  "/data_association_viz", 1); // data_association

  // read sensor topics
  std::vector<std::string> lstSources;
  mrpt::system::tokenize(sensor_source, " ,\t\n", lstSources);
  ROS_ASSERT_MSG(!lstSources.empty(),
				 "*Fatal*: At least one sensor source must be provided in "
				 "~sensor_sources (e.g. "
				 "\"scan\" or \"beacon\")");

  /// Create subscribers///
  sensorSub_.resize(lstSources.size());
  for (size_t i = 0; i < lstSources.size(); i++) {
	if (lstSources[i].find("landmark") != std::string::npos) {
	  sensorSub_[i] = n_.subscribe(lstSources[i], 1,
								   &EKFslamWrapper::landmarkCallback, this);
	} else {
	  ROS_ERROR("Can't find the sensor topics. The sensor topics should "
				"contain the word \"landmark\" in the name");
    }
  }

  init3Dwindow();
}

void EKFslamWrapper::odometryForCallback(CObservationOdometry::Ptr &_odometry,
										 const std_msgs::Header &_msg_header) {
  mrpt::poses::CPose3D poseOdom;
  if (this->waitForTransform(poseOdom, odom_frame_id, base_frame_id,
							 _msg_header.stamp, ros::Duration(1))) {
    _odometry = CObservationOdometry::Create();
    _odometry->sensorLabel = odom_frame_id;
    _odometry->hasEncodersInfo = false;
    _odometry->hasVelocities = false;
    _odometry->odometry.x() = poseOdom.x();
    _odometry->odometry.y() = poseOdom.y();
    _odometry->odometry.phi() = poseOdom.yaw();
  }
}

void EKFslamWrapper::updateSensorPose(std::string _frame_id) {
  CPose3D pose;
  tf::StampedTransform transform;
  try {
	listenerTF_.lookupTransform(base_frame_id, _frame_id, ros::Time(0),
								transform);

    tf::Vector3 translation = transform.getOrigin();
    tf::Quaternion quat = transform.getRotation();
    pose.x() = translation.x();
    pose.y() = translation.y();
    pose.z() = translation.z();
    tf::Matrix3x3 Rsrc(quat);
    CMatrixDouble33 Rdes;
    for (int c = 0; c < 3; c++)
      for (int r = 0; r < 3; r++)
        Rdes(r, c) = Rsrc.getRow(r)[c];
    pose.setRotationMatrix(Rdes);
    landmark_poses_[_frame_id] = pose;
  } catch (tf::TransformException ex) {
    ROS_ERROR("%s", ex.what());
    ros::Duration(1.0).sleep();
  }
}

bool EKFslamWrapper::waitForTransform(
	mrpt::poses::CPose3D &des, const std::string &target_frame,
	const std::string &source_frame, const ros::Time &time,
	const ros::Duration &timeout, const ros::Duration &polling_sleep_duration) {
  tf::StampedTransform transform;
  try {
	listenerTF_.waitForTransform(target_frame, source_frame, time,
								 polling_sleep_duration);
    listenerTF_.lookupTransform(target_frame, source_frame, time, transform);
  } catch (tf::TransformException) {
	ROS_INFO("Failed to get transform target_frame (%s) to source_frame (%s)",
			 target_frame.c_str(), source_frame.c_str());
    return false;
  }
  mrpt_bridge::convert(transform, des);
  return true;
}

void EKFslamWrapper::landmarkCallback(
	const mrpt_msgs::ObservationRangeBearing &_msg) {
#if MRPT_VERSION >= 0x130
  using namespace mrpt::maps;
  using namespace mrpt::obs;
#else
  using namespace mrpt::slam;
#endif
  CObservationBearingRange::Ptr landmark = CObservationBearingRange::Create();

  if (landmark_poses_.find(_msg.header.frame_id) == landmark_poses_.end()) {
    updateSensorPose(_msg.header.frame_id);
  } else {
    mrpt::poses::CPose3D pose = landmark_poses_[_msg.header.frame_id];
	mrpt_bridge::convert(_msg, landmark_poses_[_msg.header.frame_id],
						 *landmark);

    sf = CSensoryFrame::Create();
    CObservationOdometry::Ptr odometry;
    odometryForCallback(odometry, _msg.header);

    CObservation::Ptr obs = CObservation::Ptr(landmark);
    sf->insert(obs);
    observation(sf, odometry);
    timeLastUpdate_ = sf->getObservationByIndex(0)->timestamp;

    tictac.Tic();
    mapping.processActionObservation(action, sf);
    t_exec = tictac.Tac();
    ROS_INFO("Map building executed in %.03fms", 1000.0f * t_exec);
    ros::Duration(rawlog_play_delay).sleep();
    mapping.getCurrentState(robotPose_, LMs_, LM_IDs_, fullState_, fullCov_);
    viz_state();
    viz_dataAssociation();
    run3Dwindow();
    publishTF();
  }
}

bool EKFslamWrapper::rawlogPlay() {
  if (rawlog_play_ == false) {
    return false;
  } else {
    size_t rawlogEntry = 0;
#if MRPT_VERSION >= 0x199
	CFileGZInputStream f(rawlog_filename);
	auto rawlogFile = mrpt::serialization::archiveFrom(f);
#else
	CFileGZInputStream rawlogFile(rawlog_filename);
#endif

    CActionCollection::Ptr action;
    CSensoryFrame::Ptr observations;

	for (;;) {
	  if (ros::ok()) {
		if (!CRawlog::readActionObservationPair(rawlogFile, action,
												observations, rawlogEntry)) {
		  break; // file EOF
        }
        tictac.Tic();
        mapping.processActionObservation(action, observations);
        t_exec = tictac.Tac();
        ROS_INFO("Map building executed in %.03fms", 1000.0f * t_exec);
        ros::Duration(rawlog_play_delay).sleep();
		mapping.getCurrentState(robotPose_, LMs_, LM_IDs_, fullState_,
								fullCov_);
        // ros::spinOnce();
        viz_state();
        viz_dataAssociation();
        run3Dwindow();
      }
    }
	if (win3d) {
      cout << "\n Close the 3D window to quit the application.\n";
      win3d->waitForKey();
    }
    return true;
  }
}

// Local function to force the axis to be right handed for 3D. Taken from
// ecl_statistics
void EKFslamWrapper::makeRightHanded(Eigen::Matrix3d &eigenvectors,
									 Eigen::Vector3d &eigenvalues) {
  // Note that sorting of eigenvalues may end up with left-hand coordinate
  // system. So here we correctly sort it so that it does end up being
  // righ-handed and normalised.
  Eigen::Vector3d c0 = eigenvectors.block<3, 1>(0, 0);
  c0.normalize();
  Eigen::Vector3d c1 = eigenvectors.block<3, 1>(0, 1);
  c1.normalize();
  Eigen::Vector3d c2 = eigenvectors.block<3, 1>(0, 2);
  c2.normalize();
  Eigen::Vector3d cc = c0.cross(c1);
  if (cc.dot(c2) < 0) {
    eigenvectors << c1, c0, c2;
    double e = eigenvalues[0];
    eigenvalues[0] = eigenvalues[1];
    eigenvalues[1] = e;
  } else {
    eigenvectors << c0, c1, c2;
  }
}

void EKFslamWrapper::computeEllipseOrientationScale(
	tf::Quaternion &orientation, Eigen::Vector3d &scale,
	const mrpt::math::CMatrixDouble covariance) {
  // initialize variables and empty matrices for eigen vectors and eigen values
  tf::Matrix3x3 tf3d;
  Eigen::Vector3d eigenvalues(Eigen::Vector3d::Identity());
  Eigen::Matrix3d eigenvectors(Eigen::Matrix3d::Zero());

  // compute eigen vectors and eigen values from covariance matrix
  Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eigensolver(
#if MRPT_VERSION >= 0x199
	  covariance.asEigen()
#else
	  covariance
#endif
  );

  // Compute eigenvectors and eigenvalues
  if (eigensolver.info() == Eigen::Success) {
    eigenvalues = eigensolver.eigenvalues();
    eigenvectors = eigensolver.eigenvectors();
  } else {
	ROS_ERROR_STREAM("failed to compute eigen vectors/values for position. Is "
					 "the covariance matrix correct?");
	eigenvalues = Eigen::Vector3d::Zero(); // Setting the scale to zero will
										   // hide it on the screen
    eigenvectors = Eigen::Matrix3d::Identity();
  }

  // Be sure we have a right-handed orientation system
  makeRightHanded(eigenvectors, eigenvalues);

  // Rotation matrix
  tf3d.setValue(eigenvectors(0, 0), eigenvectors(0, 1), eigenvectors(0, 2),
				eigenvectors(1, 0), eigenvectors(1, 1), eigenvectors(1, 2),
				eigenvectors(2, 0), eigenvectors(2, 1), eigenvectors(2, 2));

  // get orientation from rotation matrix
  tf3d.getRotation(orientation);
  // get scale
  scale[0] = eigenvalues[0];
  scale[1] = eigenvalues[1];
  scale[2] = eigenvalues[2];
}
void EKFslamWrapper::viz_dataAssociation() {
  // robot pose
  mrpt::poses::CPose3D robotPose;
  robotPose = CPose3D(robotPose_.mean);
  geometry_msgs::Point pointRobotPose;
  pointRobotPose.z = robotPose.z();
  pointRobotPose.x = robotPose.x();
  pointRobotPose.y = robotPose.y();

  // visualization of the data association
  visualization_msgs::MarkerArray ma;
  visualization_msgs::Marker line_strip;

  line_strip.header.frame_id = "/map";
  line_strip.header.stamp = ros::Time::now();

  line_strip.id = 0;
  line_strip.type = visualization_msgs::Marker::LINE_STRIP;
  line_strip.action = visualization_msgs::Marker::ADD;

  line_strip.lifetime = ros::Duration(0.1);
  line_strip.pose.position.x = 0;
  line_strip.pose.position.y = 0;
  line_strip.pose.position.z = 0;
  line_strip.pose.orientation.x = 0.0;
  line_strip.pose.orientation.y = 0.0;
  line_strip.pose.orientation.z = 0.0;
  line_strip.pose.orientation.w = 1.0;
  line_strip.scale.x = 0.02; // line uses only x component
  line_strip.scale.y = 0.0;
  line_strip.scale.z = 0.0;
  line_strip.color.a = 1.0;
  line_strip.color.r = 1.0;
  line_strip.color.g = 1.0;
  line_strip.color.b = 1.0;

  // Draw latest data association:
  const CRangeBearingKFSLAM::TDataAssocInfo &da =
	  mapping.getLastDataAssociation();

  for (auto it = da.results.associations.begin();
	   it != da.results.associations.end(); ++it) {
    const prediction_index_t idxPred = it->second;
    // This index must match the internal list of features in the map:
    CRangeBearingKFSLAM::KFArray_FEAT featMean;
    mapping.getLandmarkMean(idxPred, featMean);

    line_strip.points.clear();
    line_strip.points.push_back(pointRobotPose);
    geometry_msgs::Point pointLm;
    pointLm.z = featMean[2];
    pointLm.x = featMean[0];
    pointLm.y = featMean[1];
    line_strip.points.push_back(pointLm);
    ma.markers.push_back(line_strip);
    line_strip.id++;
  }

  data_association_viz_pub_.publish(ma);
}

void EKFslamWrapper::viz_state() {
  visualization_msgs::MarkerArray ma;
  visualization_msgs::Marker marker;
  marker.header.frame_id = "/map";
  marker.id = 0;
  marker.type = visualization_msgs::Marker::SPHERE;
  marker.action = visualization_msgs::Marker::ADD;
  marker.lifetime = ros::Duration(0);

  // get the covariance matrix 3x3 for each ellipsoid including robot pose
  mrpt::opengl::CSetOfObjects::Ptr objs;
  objs = mrpt::opengl::CSetOfObjects::Create();
  mapping.getAs3DObject(objs);

  // Count the number of landmarks + robot
  unsigned int objs_counter = 0;
  while (objs->getByClass<mrpt::opengl::CEllipsoid>(objs_counter)) {
    objs_counter++;
  }

  mrpt::opengl::CEllipsoid::Ptr landmark;

  for (size_t i = 0; i < objs_counter; i++) {
    landmark = objs->getByClass<mrpt::opengl::CEllipsoid>(i);

	float quantiles =
		landmark->getQuantiles(); // the scale of ellipse covariance
								  // visualization (usually 3  sigma)
    mrpt::math::CMatrixDouble covariance = landmark->getCovMatrix();

    // the landmark (or robot) mean position
	CPose3D pose = mrpt::poses::CPose3D(landmark->getPose());
	// For visualization of the covariance ellipses we need the size of the axis
	// and orientation

	Eigen::Vector3d scale; // size of axis of the ellipse
    tf::Quaternion orientation;
    computeEllipseOrientationScale(orientation, scale, covariance);

    marker.id++;
    marker.color.a = 1.0;
	if (i == 0) { // robot position
      marker.color.r = 1.0;
      marker.color.g = 0.0;
      marker.color.b = 0.0;
	} else {
      marker.color.r = 0.0;
      marker.color.g = 0.0;
      marker.color.b = 1.0;
    }
    marker.type = visualization_msgs::Marker::SPHERE;
    marker.pose.position.x = pose.x();
    marker.pose.position.y = pose.y();
    marker.pose.position.z = pose.z();
    marker.pose.orientation.x = orientation.x();
    marker.pose.orientation.y = orientation.y();
    marker.pose.orientation.z = orientation.z();
    marker.pose.orientation.w = orientation.w();
    marker.scale.x = ellipse_scale_ * quantiles * std::sqrt(scale[0]);
    marker.scale.y = ellipse_scale_ * quantiles * std::sqrt(scale[1]);
    marker.scale.z = ellipse_scale_ * quantiles * std::sqrt(scale[2]);
    ma.markers.push_back(marker);

    marker.id++;
    marker.type = visualization_msgs::Marker::TEXT_VIEW_FACING;
	if (i == 0) {
      marker.text = "robot";
	} else {
      marker.text = std::to_string(LM_IDs_[i]);
    }
    marker.pose.position.x = pose.x();
    marker.pose.position.y = pose.y();
    marker.pose.position.z = pose.z() + marker.scale.z + 0.1;
    marker.color.r = 1.0;
    marker.color.g = 1.0;
    marker.color.b = 1.0;
    marker.scale.x = 0.3;
    marker.scale.y = 0.3;
    marker.scale.z = 0.3;
    ma.markers.push_back(marker);
  }

  state_viz_pub_.publish(ma);
}

void EKFslamWrapper::publishTF() {
  mapping.getCurrentState(robotPose_, LMs_, LM_IDs_, fullState_, fullCov_);
  // Most of this code was copy and pase from ros::amcl
  mrpt::poses::CPose3D robotPose;

  robotPose = CPose3D(robotPose_.mean);
  // mapBuilder->mapPDF.getEstimatedPosePDF(curPDF);

  // curPDF.getMean(robotPose);

  tf::Stamped<tf::Pose> odom_to_map;
  tf::Transform tmp_tf;
  ros::Time stamp;
  mrpt_bridge::convert(timeLastUpdate_, stamp);
  mrpt_bridge::convert(robotPose, tmp_tf);

  try {
	tf::Stamped<tf::Pose> tmp_tf_stamped(tmp_tf.inverse(), stamp,
										 base_frame_id);
    listenerTF_.transformPose(odom_frame_id, tmp_tf_stamped, odom_to_map);
  } catch (tf::TransformException) {
	ROS_INFO("Failed to subtract global_frame (%s) from odom_frame (%s)",
			 global_frame_id.c_str(), odom_frame_id.c_str());
    return;
  }

  tf::Transform latest_tf_ =
	  tf::Transform(tf::Quaternion(odom_to_map.getRotation()),
					tf::Point(odom_to_map.getOrigin()));

  // We want to send a transform that is good up until a
  // tolerance time so that odom can be used

  ros::Duration transform_tolerance_(0.1);
  ros::Time transform_expiration = (stamp + transform_tolerance_);
  tf::StampedTransform tmp_tf_stamped(latest_tf_.inverse(),
									  transform_expiration, global_frame_id,
									  odom_frame_id);
  tf_broadcaster_.sendTransform(tmp_tf_stamped);
}
