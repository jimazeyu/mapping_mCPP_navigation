#!/usr/bin/env python
# encoding: utf-8

import os
import rospy
import time
import math
import tf
import random

from visualization_msgs.msg import Marker
from geometry_msgs.msg import PointStamped, PoseStamped
from geometry_msgs.msg import Point
from move_base_msgs.msg import *


# read path
def read_path(filename):
    path = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            x, y = line.split()
            path.append((float(y), float(x), 0.1))
    return path

# if reaching the goal
def pose_callback(msg):
    global robot_name, index, status
    # log which robot reaches the goal
    rospy.loginfo("Robot %s reached the goal: %d", robot_name, index)
    # update index
    index = (index + 1) % len(path)
    # change the status
    status=1


if __name__=="__main__":
    rospy.init_node("publish_path", anonymous=True)
    # get robot_name from rosparam
    robot_name = rospy.get_param("~robot_name")
    # read file name from rosparam
    filename = rospy.get_param("~path_dir","/home/passoni/jibot_ws/mapping-mCPP-navigation/src/multi_points_navigation/paths/"+robot_name+".txt")

    # read the path
    path = read_path(filename)
    # move_base goals publisher
    goal_pub  = rospy.Publisher(robot_name+'/move_base_simple/goal', PoseStamped, queue_size = 1)
    # local path publisher
    path_pub = rospy.Publisher(robot_name+'/local_path', Marker, queue_size = 1)
    # judge if reaching the goal
    goal_status_sub = rospy.Subscriber(robot_name+'/move_base/result', MoveBaseActionResult, pose_callback) 

    # status(0 for waiting, 1 for moving)
    status = 0
    index = 0

    rate = rospy.Rate(10)

    # wait for rospy to be ready
    time.sleep(5)
    
    # initialize path
    marker = Marker()
    marker.header.frame_id = "map"
    marker.header.stamp = rospy.Time.now()
    marker.id = 0
    marker.type = Marker.LINE_STRIP
    marker.action = Marker.ADD
    marker.scale.x = 0.2
    marker.color.a = 1.0
    # orientation
    marker.pose.orientation.w = 1.0
    # marker's color
    marker.color.r = random.random()
    marker.color.g = random.random()
    marker.color.b = random.random()
    # add points
    marker.points.append(Point(path[0][0], path[0][1], path[0][2]))

    # main loop
    while not rospy.is_shutdown():
        # reset status
        status = 0
        # pre_index
        pre_index = (index-1+len(path))%len(path)
        # publish path
        pose = PoseStamped()
        pose.header.frame_id = "map"
        pose.header.seq = index
        pose.header.stamp = rospy.Time.now()
        # position
        pose.pose.position.x = path[index][0]
        pose.pose.position.y = path[index][1]
        pose.pose.position.z = path[index][2]
        # orientation
        dx = path[index][0] - path[pre_index][0]
        dy = path[index][1] - path[pre_index][1]
        yaw = math.atan2(dy, dx)
        q = tf.transformations.quaternion_from_euler(0, 0, yaw)
        pose.pose.orientation.x = q[0]
        pose.pose.orientation.y = q[1]
        pose.pose.orientation.z = q[2]
        pose.pose.orientation.w = q[3]
        goal_pub.publish(pose)

        # publish path
        marker.header.stamp = rospy.Time.now()
        marker.points.append(Point(path[index][0], path[index][1], path[index][2]))
        path_pub.publish(marker)

        # wait until status changes
        while(not rospy.is_shutdown() and not status):
            time.sleep(5)
            rospy.loginfo("Robot %s moving", robot_name)
        
        rate.sleep()
