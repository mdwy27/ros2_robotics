#include "rclcpp/rclcpp.hpp"
#include "example_interfaces/msg/string.hpp" //including the interface, same as we used with python sub/pub nodes
//but just a dif syntax compared to python, but serves same purpose^^

using namespace std::chrono_literals;

class RobotNewsStationNode : public rclcpp::Node 
{
public:
    RobotNewsStationNode() : Node("robot_news_station"), robot_name_("R2D2") //node constructor
        // first param is topic name ^ in the double parentheses
    {
        publisher_ = this->create_publisher<example_interfaces::msg::String>("robot_news", 10); // ten is the "queue size"
        timer_ = this->create_wall_timer(0.5s, std::bind(&RobotNewsStationNode::publishNews, this)); //first arg is duration of timer
        RCLCPP_INFO(this->get_logger(), "Robot News Station has been started"); //message to display to indicate construction is complete
    }
 
private:
    void publishNews()
    {
        auto msg = example_interfaces::msg::String();
        msg.data = std::string("Hi, this is ")+ robot_name_ + std::string(" from the robot news station.");
        publisher_->publish(msg); //publishing the message
    }
    std::string robot_name_;
    rclcpp::Publisher<example_interfaces::msg::String>::SharedPtr publisher_; //creating publisher attribute and shared pointer
    rclcpp::TimerBase::SharedPtr timer_; //creating a timer to regulate frequency at which publisher publishes messages to topic
};
 
int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<RobotNewsStationNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}