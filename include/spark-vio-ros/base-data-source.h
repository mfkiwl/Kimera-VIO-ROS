/**
 * @file   base-data-source.h
 * @brief  Base class for ROS wrappers for spark vio.
 * @author Yun Chang
 * @author Antoni Rosinol
 */

#pragma once

#include <functional>
#include <opencv2/core/core.hpp>
#include <opencv2/core/matx.hpp>
#include <string>

#define PCL_NO_PRECOMPILE  // Define this before you include any PCL headers
                           // to include the templated algorithms
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/point_cloud.h>

#include <image_transport/subscriber_filter.h>
#include <ros/callback_queue.h>
#include <ros/console.h>
#include <ros/ros.h>
#include <ros/spinner.h>
#include <sensor_msgs/Image.h>
#include <tf/transform_broadcaster.h>

// TODO(Toni): do we really need all these includes??
// I doubt we are using the imu frontend and the pipeline!
#include <ImuFrontEnd.h>
#include <StereoFrame.h>
#include <StereoImuSyncPacket.h>
#include <VioFrontEndParams.h>
#include <common/vio_types.h>
#include <datasource/DataSource.h>
#include <pipeline/Pipeline.h>

#include "spark-vio-ros/stereo-image-buffer.h"

namespace VIO {

/**
 * @breif Struct to hold mesh vertex data.
 */
struct PointNormalUV {
  PCL_ADD_POINT4D;
  PCL_ADD_NORMAL4D;
  float u;  // Texture coordinates.
  float v;
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
} EIGEN_ALIGN16;

class RosBaseDataProvider : public DataProvider {
 public:
  RosBaseDataProvider();
  virtual ~RosBaseDataProvider();

  // VIO output callback at keyframe rate
  void callbackKeyframeRateVioOutput(const SpinOutputPacket& vio_output);

 protected:
  // Stereo info
  struct StereoCalibration {
    CameraParams left_camera_info_;
    CameraParams right_camera_info_;
    gtsam::Pose3 camL_Pose_camR_;  // relative pose between cameras
  };

 protected:
  cv::Mat readRosImage(const sensor_msgs::ImageConstPtr& img_msg);

  cv::Mat readRosDepthImage(const sensor_msgs::ImageConstPtr& img_msg);

  // Parse camera calibration info (from param server)
  bool parseCameraData(StereoCalibration* stereo_calib);

  // Parse IMU calibration info (for ros online)
  bool parseImuData(ImuData* imudata, ImuParams* imuparams);

  // Publish all outputs by calling individual functions below
  void publishOutput(const SpinOutputPacket& vio_output);

 protected:
  VioFrontEndParams frontend_params_;
  StereoCalibration stereo_calib_;
  SpinOutputPacket vio_output_;

  // Define Node Handler for general use (Parameter server)
  ros::NodeHandle nh_;
  ros::NodeHandle nh_private_;

  // Define image transport for this and derived classes.
  std::unique_ptr<image_transport::ImageTransport> it_;

  // Define frame ids for odometry message
  std::string world_frame_id_;
  std::string base_link_frame_id_;

  // Queue to store and retrieve VIO output in a thread-safe way.
  ThreadsafeQueue<SpinOutputPacket> vio_output_queue_;

 private:
  // Define publisher for debug images.
  image_transport::Publisher debug_img_pub_;

  // Publishers
  ros::Publisher pointcloud_pub_;
  ros::Publisher per_frame_mesh_pub_;
  ros::Publisher odom_publisher_;
  ros::Publisher resil_publisher_;
  ros::Publisher frontend_stats_publisher_;
  ros::Publisher bias_publisher_;

  typedef pcl::PointCloud<pcl::PointXYZRGB> PointCloudXYZRGB;

  void publishTimeHorizonPointCloud(
      const Timestamp& timestamp, const PointsWithIdMap& points_with_id,
      const LmkIdToLmkTypeMap& lmk_id_to_lmk_type_map);
  void publishPerFrameMesh3D(const SpinOutputPacket& vio_output);
  void publishTimeHorizonMesh3D(const SpinOutputPacket& vio_output);
  void publishState(const SpinOutputPacket& vio_output);
  void publishFrontendStats(const SpinOutputPacket& vio_output) const;
  // Publish resiliency statistics
  void publishResiliency(const SpinOutputPacket& vio_output) const;
  void publishImuBias(const SpinOutputPacket& vio_output) const;
  void publishDebugImage(const Timestamp& timestamp,
                         const cv::Mat& debug_image);

  // Define tf broadcaster for world to base_link (IMU).
  tf::TransformBroadcaster odom_broadcaster_;
};

}  // namespace VIO

POINT_CLOUD_REGISTER_POINT_STRUCT(
    VIO::PointNormalUV,
    (float, x, x)(float, y, y)(float, x, z)(float, normal_x, normal_x)(
        float, normal_y, normal_y)(float, normal_z, normal_z)(float, u,
                                                              u)(float, v, v))
