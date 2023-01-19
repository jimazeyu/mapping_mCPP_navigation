#include"ros/ros.h"
#include"nav_msgs/Odometry.h"
#include"geometry_msgs/PoseStamped.h"
#include"sensor_msgs/Imu.h"
#include"tf2/LinearMath/Quaternion.h"
#include"tf2/LinearMath/Matrix3x3.h"
// boost
#include"boost/thread/thread.hpp"

class NoiseAdder
{
public:
    NoiseAdder(ros::NodeHandle &nh, ros::NodeHandle &private_nh)
    :nh_(nh),
    private_nh_(private_nh)
    {
        // 初始化参数
        if(!private_nh_.getParam("uwb_noise",uwb_std))
            uwb_std=0.1;
        if(!private_nh_.getParam("odom_noise_xy",odom_noise_xy))
            odom_noise_xy=0.05;
        if(!private_nh_.getParam("odom_noise_theta",odom_noise_theta))
            odom_noise_theta=0.01;
        // 订阅odom
        odom_sub = nh_.subscribe("odom", 10, &NoiseAdder::odomCallback, this);
        // 订阅imu（为了时间同步）
        imu_sub = nh_.subscribe("imu", 10, &NoiseAdder::imuCallback, this);

        // 发布odom_noise
        odom_noise_pub = nh_.advertise<nav_msgs::Odometry>("odom_noise", 10);
        // 发布uwb
        uwb_noise_pub = nh_.advertise<nav_msgs::Odometry>("uwb", 10);
        // 发布imu
        imu_pub = nh_.advertise<sensor_msgs::Imu>("imu_fake", 10);
    }
    // imu回调函数
    void imuCallback(const sensor_msgs::Imu::ConstPtr &imuu)
    {
        // 上锁
        boost::mutex::scoped_lock lock(mutex);
        // copy imu
        imu = *imuu;
        publish();
    }

    // odom回调函数
    void odomCallback(const nav_msgs::Odometry::ConstPtr &odom)
    {
        // 上锁
        boost::mutex::scoped_lock lock(mutex);
        // 判断是否为第一次收到
        if(first_odom)
        {
            last_odom=*odom;
            odom_noise=*odom;
            first_odom=false;
            return ;
        }
        // 发布uwb_noise
        uwb_noise.header = odom->header;
        uwb_noise.child_frame_id = odom->child_frame_id;
        uwb_noise.pose.pose.position.x = odom->pose.pose.position.x + gaussianNoise(0, uwb_std);
        uwb_noise.pose.pose.position.y = odom->pose.pose.position.y + gaussianNoise(0, uwb_std);
        uwb_noise.pose.pose.position.z = odom->pose.pose.position.z;
        uwb_noise.pose.pose.orientation.w = 1.0;
        // 修改协方差
        uwb_noise.pose.covariance[0] = uwb_std;
        uwb_noise.pose.covariance[7] = uwb_std;
        uwb_noise.pose.covariance[14] = 0.000001;
        uwb_noise.pose.covariance[21] = 9999999;
        uwb_noise.pose.covariance[28] = 9999999;
        uwb_noise.pose.covariance[35] = 9999999;
        uwb_noise_pub.publish(uwb_noise);

        // 判断与上一次有没有运动，eps表示误差
        if (fabs(odom->pose.pose.position.x - last_odom.pose.pose.position.x) < eps &&
            fabs(odom->pose.pose.position.y - last_odom.pose.pose.position.y) < eps &&
            fabs(odom->pose.pose.orientation.w - last_odom.pose.pose.orientation.w) < eps)
        {
            ROS_INFO("Stop");
            last_odom = *odom;
            odom_noise_pub.publish(odom_noise);
            return;
        }
        
        ROS_INFO("Move");
        odom_noise.header = odom->header;
        odom_noise.child_frame_id = odom->child_frame_id;
        // 如发生平移，位置累加噪声
        if(fabs(odom->pose.pose.position.x - last_odom.pose.pose.position.x) > eps ||
            fabs(odom->pose.pose.position.y - last_odom.pose.pose.position.y) > eps )
        {
            odom_noise.pose.pose.position.x = odom_noise.pose.pose.position.x + (odom->pose.pose.position.x-last_odom.pose.pose.position.x) + gaussianNoise(0, odom_noise_xy);
            odom_noise.pose.pose.position.y = odom_noise.pose.pose.position.y + (odom->pose.pose.position.y-last_odom.pose.pose.position.y) + gaussianNoise(0, odom_noise_xy);
            odom_noise.pose.pose.position.z = odom->pose.pose.position.z;
        }
        // 添加姿态噪声
        tf2::Quaternion q;
        q.setX(odom->pose.pose.orientation.x);
        q.setY(odom->pose.pose.orientation.y);
        q.setZ(odom->pose.pose.orientation.z);
        q.setW(odom->pose.pose.orientation.w);
        double roll, pitch, yaw;
        tf2::Matrix3x3(q).getRPY(roll, pitch, yaw);
        tf2::Quaternion q_last;
        q_last.setX(last_odom.pose.pose.orientation.x);
        q_last.setY(last_odom.pose.pose.orientation.y);
        q_last.setZ(last_odom.pose.pose.orientation.z);
        q_last.setW(last_odom.pose.pose.orientation.w);
        double roll_last, pitch_last, yaw_last;
        tf2::Matrix3x3(q_last).getRPY(roll_last, pitch_last, yaw_last);
        // 计算和上一帧累计误差
        double delta_yaw = yaw - yaw_last;
        // 如发生旋转，则增加旋转误差
        if(fabs(delta_yaw)> eps)
        {
            tf2::Quaternion q_noise;
            q_noise.setX(odom_noise.pose.pose.orientation.x);
            q_noise.setY(odom_noise.pose.pose.orientation.y);
            q_noise.setZ(odom_noise.pose.pose.orientation.z);
            q_noise.setW(odom_noise.pose.pose.orientation.w);
            double roll_noise, pitch_noise, yaw_noise;
            tf2::Matrix3x3(q).getRPY(roll_noise, pitch_noise, yaw_noise);
            //odom_noise旋转delta_yaw
            yaw_noise += delta_yaw + gaussianNoise(0, odom_noise_theta);
            // RPY转四元数
            q_noise.setRPY(roll_noise, pitch_noise, yaw_noise);
            odom_noise.pose.pose.orientation.x = q_noise.x();
            odom_noise.pose.pose.orientation.y = q_noise.y();
            odom_noise.pose.pose.orientation.z = q_noise.z();
            odom_noise.pose.pose.orientation.w = q_noise.w();

            // 修改协方差
            odom_noise.pose.covariance[0] = odom_noise_xy;
            odom_noise.pose.covariance[7] = odom_noise_xy;
            odom_noise.pose.covariance[14] = 0.000001;
            odom_noise.pose.covariance[21] = 9999999;
            odom_noise.pose.covariance[28] = 9999999;
            odom_noise.pose.covariance[35] = odom_noise_theta;
        }
        // 更新odom
        last_odom = *odom;
    }
    // 生成高斯噪声
    double uniform_rand(double lowerBndr, double upperBndr)
    {
        return lowerBndr + ((double) std::rand() / (RAND_MAX + 1.0)) * (upperBndr - lowerBndr);
    }
    double gaussianNoise(double mean, double sigma)
    {
        double x, y, r2;
        do {
            x = -1.0 + 2.0 * uniform_rand(0.0, 1.0);
            y = -1.0 + 2.0 * uniform_rand(0.0, 1.0);
            r2 = x * x + y * y;
        } while (r2 > 1.0 || r2 == 0.0);
        return mean + sigma * y * std::sqrt(-2.0 * log(r2) / r2);
    }

    // 发布所有数据，并同步时间戳
    void publish()
    {
        if(first_odom)
        {
            return;
        }
        // 发布imu
        imu.header.stamp = ros::Time::now();
        imu_pub.publish(imu);
        // 发布odom_noise
        odom_noise.header.stamp = ros::Time::now();
        odom_noise_pub.publish(odom_noise);
        // 发布uwb_noise
        uwb_noise.header.stamp = ros::Time::now();
        uwb_noise_pub.publish(uwb_noise);
    }

private:
    ros::NodeHandle nh_;
    ros::NodeHandle private_nh_;
    ros::Subscriber odom_sub;
    ros::Subscriber imu_sub;
    ros::Publisher odom_noise_pub;
    ros::Publisher uwb_noise_pub;
    ros::Publisher imu_pub;
    nav_msgs::Odometry odom_noise;
    nav_msgs::Odometry uwb_noise;
    sensor_msgs::Imu imu;
    // 上一次odom
    nav_msgs::Odometry last_odom;
    // eps
    const double eps = 0.005;
    // 噪声大小
    double uwb_std;
    double odom_noise_xy;
    double odom_noise_theta; 
    // 互斥锁
    boost::mutex mutex;
    // 判断是否为第一次收到odom
    bool first_odom = true;
};


int main(int argc,char**argv)
{
    ros::init(argc, argv, "add_noise");
    ros::NodeHandle nh;
    ros::NodeHandle private_nh("~");
    NoiseAdder noise_adder(nh, private_nh);
    ros::spin();
    return 0;
}
