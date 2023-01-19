#!/usr/bin/env python2
# encoding: utf-8

# Show the
from visualization_msgs.msg import Marker
from visualization_msgs.msg import MarkerArray
from geometry_msgs.msg import Point

import os
import rospy
import random

# read the path from the file
def read_path(path_file):
    path = []
    with open(path_file, 'r') as f:
        for line in f:
            line = line.strip()
            if line == '':
                continue
            x, y = line.split()
            path.append((float(y), float(x)))
    return path

if __name__=="__main__":
    rospy.init_node("show_paths")
    pub = rospy.Publisher("/paths", MarkerArray, queue_size=10)
    # read file name from rosparam
    path_dir = rospy.get_param("~path_dir","/home/passoni/jibot_ws/test_paths")
    # read all the files from the directory
    path_files = [os.path.join(path_dir, f) for f in os.listdir(path_dir) if os.path.isfile(os.path.join(path_dir, f))]
    # read all the paths
    paths = [read_path(path_file) for path_file in path_files]
    rospy.loginfo("Found %d paths in %s" % (len(paths), path_dir))
    # create the marker array
    marker_array = MarkerArray()
    for i, path in enumerate(paths):
        marker = Marker()
        marker.header.frame_id = "map"
        marker.header.stamp = rospy.Time.now()
        marker.id = i
        marker.type = Marker.LINE_STRIP
        marker.action = Marker.ADD
        marker.scale.x = 0.1
        marker.color.a = 1.0
        # orientation
        marker.pose.orientation.w = 1.0
        # marker's color
        marker.color.r = random.random()
        marker.color.g = random.random()
        marker.color.b = random.random()
        # add points
        marker.points = []
        for x, y in path:
            marker.points.append(Point(x, y, 0))
        marker_array.markers.append(marker)

    rate = rospy.Rate(1)
    while not rospy.is_shutdown():
        pub.publish(marker_array)
        rate.sleep()