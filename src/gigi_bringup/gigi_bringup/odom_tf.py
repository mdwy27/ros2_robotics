import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
from geometry_msgs.msg import TransformStamped
from tf2_ros import TransformBroadcaster

#node setup
#odom_tf listens to /odom
class OdomTF(Node):
    def __init__(self):
        super().__init__('odom_tf')
        self.broadcaster = TransformBroadcaster(self)
        self.create_subscription(Odometry, 'odom', self.odom_callback, 10)
#odom_callback uses information from messages sent to /odom
#uses Pi's timestamp to sync time so data doesn't reject data
#pose is copied one coordinate at a time (x, y, z -- sent in the message on /odom)
    def odom_callback(self, msg):
        t = TransformStamped()

        t.header.stamp = msg.header.stamp
        t.header.frame_id = msg.header.frame_id    # parent:"odom"
        t.child_frame_id = msg.child_frame_id      # child:"base_footprint"

        t.transform.translation.x = msg.pose.pose.position.x
        t.transform.translation.y = msg.pose.pose.position.y
        t.transform.translation.z = msg.pose.pose.position.z
        t.transform.rotation = msg.pose.pose.orientation

        self.broadcaster.sendTransform(t)

#keeping the node running, waiting to receive and parse messages from /odom
def main():
    rclpy.init()
    node = OdomTF()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.try_shutdown()