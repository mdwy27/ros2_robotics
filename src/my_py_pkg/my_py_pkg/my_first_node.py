#ROS2 Python node example
#Creates a timer that logs an incrementing counter every second
#uses object-oriented programming, inherits all methods,properties from class Node

#!/usr/bin/env python3
import rclpy
from rclpy.node import Node

# MyNode inherits from Node, the base class for all ROS2 nodes
class MyNode(Node):

    def __init__(self):
        super().__init__("py_test")
        self.counter_ = 0 #initializing 
        self.get_logger().info("Hello world")
        self.create_timer(1.0, self.timer_callback)

    def timer_callback(self):
        self.get_logger().info("Hello world " + str(self.counter_))
        self.counter_+=1


def main(args=None):
    rclpy.init(args=args)
    node = MyNode()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == "__main__":
    main()