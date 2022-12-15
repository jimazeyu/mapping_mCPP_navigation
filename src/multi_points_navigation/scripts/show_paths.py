#!/usr/bin/env python
# encoding: utf-8

# Show the
from visualization_msgs.msg import Marker
from visualization_msgs.msg import MarkerArray
from geometry_msgs.msg import Point

import sys
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
    # read file name from command line
    path_files = sys.argv[1:]
    rospy.loginfo("showing %d paths" % len(path_files))
    # read paths
    paths = []
    for path_file in path_files:
        paths.append(read_path(path_file))
    # publish paths
    rate = rospy.Rate(1)
    while not rospy.is_shutdown():
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
            # change marker's color
            marker.color.r = random.random()
            marker.color.g = random.random()
            marker.color.b = random.random()

            marker.points = []
            for x, y in path:
                marker.points.append(Point(x, y, 0))
            marker_array.markers.append(marker)
        pub.publish(marker_array)
        rate.sleep()