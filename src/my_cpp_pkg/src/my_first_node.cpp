#include "rclcpp/rclcpp.hpp"

class MyNode : public rclcpp::Node //declaring a new class MyNode
{
public:
    MyNode() : Node("cpp_test"), counter_(0) //arg is node name
    {
        RCLCPP_INFO(this->get_logger(), "Hello world"); //creating functionalities of this class
        timer_ = this->create_wall_timer(std::chrono::seconds(1), 
                                         std::bind(&MyNode::timerCallback, this));
    }

private:
    void timerCallback()
    {
        RCLCPP_INFO(this->get_logger(), "Hello %d", counter_);
        counter_++;
    }
    
    rclcpp::TimerBase::SharedPtr timer_;
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