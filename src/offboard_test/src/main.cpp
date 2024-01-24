#include "ros/ros.h"
#include "geometry_msgs/PoseStamped.h"
#include <mavros_msgs/CommandBool.h>
#include "mavros_msgs/SetMode.h"
#include "mavros_msgs/State.h"

//***********************************************
//*        callback function                    *                                    
//***********************************************
//uav state
mavros_msgs::State current_state;
void state_cb(const mavros_msgs::State::ConstPtr& msg){
    current_state = *msg;
}

geometry_msgs::PoseStamped current_position;
void cur_pos(const geometry_msgs::PoseStamped::ConstPtr& msg){
    current_position = *msg;
}



int main(int argc,char **argv)
{
    ros::init(argc,argv,"offboard");
    ros::NodeHandle nh;

    ros::Subscriber state_sub = nh.subscribe<mavros_msgs::State>
            ("mavros/state",1,state_cb);
    
    ros::Publisher local_pos_pub = nh.advertise<geometry_msgs::Pose>
            ("mavros/setpoint_position/local",10);

    ros::ServiceClient arming_client = nh.serviceClient<mavros_msgs::CommandBool>
            ("mavros/cmd/arming");
    ros::ServiceClient set_mode_client = nh.serviceClient<mavros_msgs::SetMode>
            ("mavros/set_mode");

    
    ros::Rate rate(30.0);

    while(ros::ok() && !current_state.connected){
        ros::spinOnce();
        rate.sleep();
    }

    geometry_msgs::PoseStamped pose;
    pose.pose.position.x = 0;
    pose.pose.position.y = 0;
    pose.pose.position.z = 0;

    //send a few setpoints before starting
    for(int i = 100; ros::ok() && i > 0; --i){
        local_pos_pub.publish(pose);
        ros::spinOnce();
        rate.sleep();
    }

    mavros_msgs::SetMode offb_set_mode;
    offb_set_mode.request.custom_mode = "OFFBOARD";

    mavros_msgs::CommandBool arm_cmd;
    arm_cmd.request.value = true;

    ros::Time last_request = ros::Time::now();
    ros::Time hovering_time = ros::Time::now();

    while(ros::ok() ){
        if( current_state.mode != "OFFBOARD" &&
            (ros::Time::now() - last_request > ros::Duration(5.0))){
            if( set_mode_client.call(offb_set_mode) &&
                offb_set_mode.response.mode_sent){
                ROS_INFO("Offboard enabled");
            }
            last_request = ros::Time::now();
        } else {
            if( !current_state.armed &&
                (ros::Time::now() - last_request > ros::Duration(5.0))){
                if( arming_client.call(arm_cmd) &&
                    arm_cmd.response.success){
                    ROS_INFO("Vehicle armed");
                }
                last_request = ros::Time::now();
            }
        }

        //local_pos_pub.publish(pose);
       // if(ros::Time::now()-hovering_time > ros::Duration(15.0)){
        //        pose.pose.position.x = 5*cos(RAD2DEG*t);
       //         pose.pose.position.y = 5*sin(RAD2DEG*t); 

        //        t+=0.001;
       // }

        ros::spinOnce();
        rate.sleep();
    }


    return 0;
}