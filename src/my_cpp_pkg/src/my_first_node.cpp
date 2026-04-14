//This is an example of a ROS2 node written in C++
//This can be used as a framework moving forward to build upon as you write new nodes.
//This node here creates a timer that logs an incrementing counter every second

#include "rclcpp/rclcpp.hpp"

//MyNode inherits from rclcpp::Node, the base class for all ROS2 nodes
class MyNode : public rclcpp::Node //declaring a new class MyNode
{
public:
    //Constructor: initializes node with name "cpp_test" and a counter at 0
    MyNode() : Node("cpp_test"), counter_(0) //arg is node name
    {
        RCLCPP_INFO(this->get_logger(), "Hello world"); //creating functionalities of this class
        //Creating a timer that calls timerCallback every second
        timer_ = this->create_wall_timer(std::chrono::seconds(1), 
                                         std::bind(&MyNode::timerCallback, this));
    }

private:
    //called every second by the timer
    void timerCallback()
    {
        RCLCPP_INFO(this->get_logger(), "Hello %d", counter_);
        counter_++; //counts the number of callbacks
    }
    
    rclcpp::TimerBase::SharedPtr timer_; //timer object
    int counter_;
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv); //initialize ros2 communication
    auto node = std::make_shared<MyNode>(); //creating the node
    rclcpp::spin(node); //making the node spin
    rclcpp::shutdown(); //shutting down the node
    return 0;
}