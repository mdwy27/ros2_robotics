#!/usr/bin/env python3

#the following node was written using template_py_node.py as a template

import rclpy
from rclpy.node import Node
from example_interfaces.msg import String #importing data type String
#don't forget to add new dependencies, in this case example_interfaces.msg, to package.xml!
 
 
class RobotNewsStationNode(Node): # notice the class attributes defined below in this class declaration!
    def __init__(self): #constructor, methods indicate a publisher and timer will be created upon construction
        super().__init__("robot_news_station") #node name is in the double parentheses ""
        #notice how we made node name the same as the file name, common practice
        #adding additional class methods:
        self.robot_name_ = "C3PO"
        self.publisher_ = self.create_publisher(String, "robot_news", 10) #each topic needs a name and data type --> these are the args here
        #last argument sets buffer of messages sent to 10, can be adjusted but a good baseline
        self.timer_ = self.create_timer(0.5, self.publish_news)
        self.get_logger().info("Robot news station has been started.") #this message will display at end of constructor to indicate in serial monitor construction is complete

    def publish_news(self):
        msg = String()
        msg.data = "Hello, this is " + self.robot_name_ + " from the robot news station." # a message being published by node
        self.publisher_.publish(msg)
 
 
def main(args=None):
    rclpy.init(args=args)
    node = RobotNewsStationNode() #start node
    rclpy.spin(node) #make node spin
    rclpy.shutdown() #shut down node
 
 
if __name__ == "__main__":
    main()
