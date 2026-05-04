#!/usr/bin/env python3

#this is a template to be used moving forward when writing nodes with python, using OOP

import rclpy
from rclpy.node import Node
from example_interfaces.msg import String #import String data type --> add example_interfaces to dependencies in package.xml
 
 
class SmartphoneNode(Node):
    def __init__(self):
        super().__init__("smartphone") #node name in parentheses
        self.subscriber_ = self.create_subscription(String, "robot_news", self.callback_robot_news, 10) 
        #^ note how this fxn has 4 parameters, compared to analagous function for publisher node
        #make sure second arg, topic name, is EXACT SAME as publisher topic name
        #this ensures subscriber and publisher subscribe/publish to same topic and thus communicate correctly
        self.get_logger().info("Smartphone has been started.") # A message to report node has started spinning

    def callback_robot_news(self, msg: String): #a subscriber callback, processed when subscriber receives a message from publisher
        self.get_logger().info(msg.data)
 
def main(args=None):
    rclpy.init(args=args)
    node = SmartphoneNode() 
    rclpy.spin(node)
    rclpy.shutdown()
 
 
if __name__ == "__main__":
    main()
