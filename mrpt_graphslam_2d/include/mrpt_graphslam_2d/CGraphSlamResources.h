/* +---------------------------------------------------------------------------+
   |                     Mobile Robot Programming Toolkit (MRPT)               |
   |                          http://www.mrpt.org/                             |
   |                                                                           |
   | Copyright (c) 2005-2016, Individual contributors, see AUTHORS file        |
   | See: http://www.mrpt.org/Authors - All rights reserved.                   |
   | Released under BSD License. See details in http://www.mrpt.org/License    |
   +---------------------------------------------------------------------------+ */

#ifndef CGRAPHSLAMRESOURCES_H
#define CGRAPHSLAMRESOURCES_H

// ROS
#include <ros/ros.h>
#include <mrpt_bridge/mrpt_bridge.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/PoseArray.h>
#include <geometry_msgs/TransformStamped.h>
#include <sensor_msgs/LaserScan.h>
#include <nav_msgs/Path.h>
#include <nav_msgs/Odometry.h>
#include <nav_msgs/OccupancyGrid.h>
#include <std_msgs/Header.h>

#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Vector3.h>

// MRPT
#include <mrpt/graphs/CNetworkOfPoses.h>
#include <mrpt/maps/COccupancyGridMap2D.h>
#include <mrpt/obs/CObservationOdometry.h>
#include <mrpt/obs/CObservation2DRangeScan.h>
#include <mrpt/system/threads.h>
#include <mrpt/system/string_utils.h>
#include <mrpt/system/filesystem.h>
#include <mrpt/system/os.h>
#include <mrpt/utils/mrpt_macros.h>
#include <mrpt/utils/COutputLogger.h>
#include <mrpt/graphslam/CGraphSlamEngine.h>
#include <mrpt/graphslam/apps_related/TUserOptionsChecker.h>
#include <mrpt/graphslam/apps_related/CGraphSlamHandler.h>

// cpp headers
#include <string>
#include <sstream>
#include <vector>

/**\brief Manage variables, ROS parameters and everything else related to the
 * graphslam-engine ROS wrapper.
 */
class CGraphSlamResources
{
public:
	/**\brief type of graph constraints */
	typedef typename mrpt::graphs::CNetworkOfPoses2DInf::constraint_t constraint_t;
	/**\brief type of underlying poses (2D/3D). */
	typedef typename mrpt::graphs::CNetworkOfPoses2DInf::constraint_t::type_value pose_t;

	CGraphSlamResources(
			mrpt::utils::COutputLogger* logger_in,
			ros::NodeHandle* nh
			);
	~CGraphSlamResources();


	void getParamsAsString(std::string* str_out);
	std::string getParamsAsString();

	/**\brief Read the problem configuration parameters
	 *
	 * \sa readROSParameters, printParams
	 */
	void readParams();
	/**\brief Print in a compact manner the overall problem configuration
	 * parameters
	 */
	void printParams();

	/**\name Sniffing methods
	 *
	 * Sniff measurements off their corresponding topics
	 */
	/**\{*/
	/**\brief Callback method for handling incoming odometry measurements in a ROS
	 * topic.
	 */
	void sniffOdom(const nav_msgs::Odometry::ConstPtr& ros_odom);
	/**\brief Callback method for handling incoming LaserScans objects in a ROS
	 * topic.
	 */
	void sniffLaserScan(const sensor_msgs::LaserScan::ConstPtr& ros_laser_scan);
	void sniffCameraImage();
	/** TODO - Implement this */
	void sniff3DPointCloud();
	/**\}*/
	/**\brief Indicate whether graphslam execution can proceed normally.
	 * \return False if user has demanded to exit (pressed <C-c>), True otherwise
	 */
	bool continueExec();
	/**\brief Generate the relevant report directory/files after the graphSLAM
	 * execution.
	 */
	void generateReport();
	/**\brief Provide feedback about the SLAM operation using ROS publilshers,
	 * update the registered frames using the tf2_ros::TransformBroadcaster
	 *
	 * Method makes the necessary calls to all the publishers of the class and
	 * broadcaster instances
	 *
	 * \sa continueExec
	 * \return True if the graphSLAM execution is to continue normally
	 */
	bool usePublishersBroadcasters();
	/**\brief Wrapper method around the setup* private class methods.
	 *
	 * Handy for setting up publishers, subscribers, services, TF-related stuff
	 * all at once from the user application
	 * 
	 * \note method should be called right after the call to
	 * CGraphSlamResources::readParams method
	 */
	void setupCommunication();


	static const std::string sep_header;
	static const std::string sep_subheader;
private:
	/**\brief Initialize the CGraphslamEngine object based on the user input. */
	void initGraphSLAM();
	/**\brief Process an incoming measurement.
	 *
	 * Method is a wrapper around the _process method 
	 *
	 * \note Method is automatically called when a new measurement is fetched
	 * from a subscribed topic
	 *
	 * \sa _process
	 */
	void processObservation(mrpt::obs::CObservationPtr& observ);
	/**\brief Low level wrapper for executing the
	 * CGraphSlamEngine::execGraphSlamStep method
	 *
	 * \sa processObservation();
	 */
	void _process(mrpt::obs::CObservationPtr& observ);
	/**\brief read configuration parameters from the ROS parameter server.
	 *
	 * \sa readParams
	 */
	void readROSParameters();
	void readStaticTFs();
	/**\brief Fill in the given string with the parameters that have been read
	 * from the ROS parameter server
	 *
	 * \sa getParamsAsString, readROSParameters
	 */
	void getROSParameters(std::string* str_out);
	/**\brief Verify that the parameters read are valid and can be used with the
	 * CGraphSlamEngine instance.
	 */
	void verifyUserInput();

	/**\brief Reset the flags indicating whether we have received new data
	 * (odometry, laser scans etc.)
	 */
	void resetReceivedFlags();
	/**\name Methods for setting up topic subscribers, publishers, and
	 * corresponding services
	 *
	 * \sa setupCommunication
	 */
	/**\{*/
	void setupSubs();
	void setupPubs();
	void setupSrvs();
	/**\}*/

	/**\brief Pointer to the logging instance */
	mrpt::utils::COutputLogger* m_logger;
	ros::NodeHandle* nh;

	// ROS server parameters
	/**\name node, edge, optimizer modules in string representation */
	/**\{*/
	std::string m_node_reg;
	std::string m_edge_reg;
	std::string m_optimizer;
	/**\}*/

	/**\name graphslam-engine various filenames */
	/**\{*/
	std::string m_ini_fname; /**<.ini configuration file */
	std::string m_gt_fname; /**<ground-truth filename */
	/**\}*/

	/**\brief Minimum logging level
	 *
	 * \note Value is fetched  from the ROS Parameter Server (not from the
	 * external .ini file.
	 */
	mrpt::utils::VerbosityLevel m_min_logging_level;

	/**\brief Are MRPT visuals on? */
	bool m_disable_MRPT_visuals;

	/**\brief Struct instance holding the available deciders/optimizers that the
	 * user can issue
	 */
	mrpt::graphslam::supplementary::TUserOptionsChecker m_graphslam_opts;
	CGraphSlamHandler* m_graphslam_handler;

	bool m_has_read_config;

	/**\name Received measurements - boolean flags
	 *
	 * \brief Flags that indicate if any new measurements have arrived in the
	 * corresponding topics.
	 */
	/**\{*/
	bool m_received_odom;
	bool m_received_laser_scan;
	bool m_received_camera;
	bool m_received_point_cloud;
	/**\}*/

	/**\name Processed measurements
	 *
	 * Measurements that the class can the class instance is keeping track
	 * of and passes to the CGraphSlamEngine instance.
	 */
	/**\{*/
	/**\brief Received laser scan - converted into MRPT CObservation* format */
	mrpt::obs::CObservationOdometryPtr m_mrpt_odom;
	mrpt::obs::CObservation2DRangeScanPtr m_mrpt_laser_scan;
	/**\}*/

	mrpt::graphslam::CGraphSlamEngine<mrpt::graphs::CNetworkOfPoses2DInf>*
		m_graphslam_engine;

	/**\name Subscribers - Publishers
	 *
	 * ROS Topic Subscriber/Publisher instances
	 * */
	/**\{*/
	ros::Subscriber m_odom_sub;
	ros::Subscriber m_laser_scan_sub;
	ros::Subscriber m_camera_scan_sub;
	ros::Subscriber m_point_cloud_scan_sub;

	ros::Publisher m_curr_robot_pos_pub;
	ros::Publisher m_robot_trajectory_pub;
	ros::Publisher m_robot_tr_poses_pub;
	ros::Publisher m_gt_trajectory_pub; // TODO
	ros::Publisher m_SLAM_eval_metric_pub; // TODO
	ros::Publisher m_odom_trajectory_pub;
	ros::Publisher m_gridmap_pub;
	/**\}*/

	/**\name Topic Names
	 *
	 * Names of the topics that the class instance subscribes or publishes to
	 */
	/**\{*/
	std::string m_odom_topic;
	std::string m_laser_scan_topic;
	std::string m_camera_topic;
	std::string m_point_cloud_topic;

	std::string m_curr_robot_pos_topic;
	std::string m_robot_trajectory_topic;
	std::string m_robot_tr_poses_topic;
	std::string m_odom_trajectory_topic;
	std::string m_SLAM_eval_metric_topic;
	std::string m_gridmap_topic; // TODO
	/**\}*/

	/**\name TransformBroadcasters - TransformListeners
	 */
	/**\{*/
	tf2_ros::Buffer m_buffer;
	tf2_ros::TransformBroadcaster m_broadcaster;
	/**\}*/

	/**\name TF Frame IDs
	 * Names of the current TF Frames available
	 */
	/**\{*/
	/**\brief Frame that the robot starts from. In a single-robot SLAM
	 * setup this can be set to the world frame
	 */
	std::string m_anchor_frame_id;
	std::string m_base_link_frame_id;
	std::string m_odom_frame_id;
	std::string m_laser_frame_id;
	/**\}*/

	/**\name Geometrical Configuration
	 * \brief Variables that setup the geometrical dimensions, distances between
	 * the different robot parts etc.
	 */
	/**\{*/
	geometry_msgs::TransformStamped m_base_laser_transform; // TODO - either use it or lose it
	geometry_msgs::TransformStamped m_anchor_odom_transform;
	/**\}*/

	/**\brief Odometry path of the robot.
	 * Handy mostly for visualization reasons.
	 */
	nav_msgs::Path m_odom_path;

	/**\brief Times a messge has been published => usePublishersBroadcasters method is called
	 */
	int m_pub_seq;

	/**\brief Total counter of the processed measurements
	 */
	size_t m_measurement_cnt;

	int m_queue_size;
};

#endif /* end of include guard: CGRAPHSLAMRESOURCES_H */
