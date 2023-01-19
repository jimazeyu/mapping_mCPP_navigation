// 本文件会发布odom和EKF修正后的odom的路径
#include"ros/ros.h"
#include"nav_msgs/Odometry.h"
#include"geometry_msgs/PoseStamped.h"
#include"geometry_msgs/PoseWithCovarianceStamped.h"
// path
#include"nav_msgs/Path.h"

// odom路径
nav_msgs::Path odom_path;
// odom_combined路径
nav_msgs::Path odom_combined_path;

// publisher和subscriber
ros::Publisher odom_path_pub;
ros::Publisher odom_combined_path_pub;
ros::Subscriber odom_sub;
ros::Subscriber odom_combined_sub;

// odom回调函数
void odomCallback(const nav_msgs::Odometry::ConstPtr &odom)
{
    // 将odom插入odom_path
    geometry_msgs::PoseStamped pose_stamped;
    pose_stamped.header = odom->header;
    pose_stamped.pose = odom->pose.pose;
    odom_path.poses.push_back(pose_stamped);
    // 发布odom_path
    odom_path_pub.publish(odom_path);
    // sleep
    ros::Duration(0.3).sleep();
    ROS_INFO("odom_path size: %d",odom_path.poses.size());
}

// odom_combined回调函数
void odomCombinedCallback(const geometry_msgs::PoseWithCovarianceStampedConstPtr &odom_combined)
{
    // 将odom_combined插入odom_combined_path
    geometry_msgs::PoseStamped pose_stamped;
    pose_stamped.header = odom_combined->header;
    pose_stamped.pose = odom_combined->pose.pose;
    odom_combined_path.poses.push_back(pose_stamped);
    // 发布odom_combined_path
    odom_combined_path_pub.publish(odom_combined_path);
    // sleep
    ros::Duration(0.3).sleep();
    ROS_INFO("odom_combined_path size: %d",odom_combined_path.poses.size());
}

int main(int argc,char **argv)
{
    ros::init(argc,argv,"publish_path");
    ros::NodeHandle nh;

    // 初始化odom_path
    odom_path.header.frame_id = "odom";
    odom_path.poses.clear();
    // 初始化odom_combined_path
    odom_combined_path.header.frame_id = "odom";
    odom_combined_path.poses.clear();

    odom_path_pub = nh.advertise<nav_msgs::Path>("odom_path",10);
    odom_combined_path_pub = nh.advertise<nav_msgs::Path>("odom_combined_path",10);
    odom_sub = nh.subscribe("odom_noise",10,odomCallback);
    odom_combined_sub = nh.subscribe("/robot_pose_ekf/odom_combined",10,odomCombinedCallback);
    ros::spin();
    return 0;
}