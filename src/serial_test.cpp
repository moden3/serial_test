#include "ros/ros.h"
#include "std_msgs/Float32MultiArray.h"
#include "std_msgs/Int32MultiArray.h"
#include "std_msgs/String.h"

#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <unistd.h>

int BAUDRATE = B115200;

int open_serial(const char *device_name)
{
    int fd1 = open(device_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    fcntl(fd1, F_SETFL, 0);
    //load configuration
    struct termios conf_tio;
    tcgetattr(fd1, &conf_tio);
    //set baudrate
    //speed_t BAUDRATE = B115200;
    cfsetispeed(&conf_tio, BAUDRATE);
    cfsetospeed(&conf_tio, BAUDRATE);
    //non canonical, non echo back
    conf_tio.c_lflag &= ~(ECHO | ICANON);
    //non blocking
    conf_tio.c_cc[VMIN] = 0;
    conf_tio.c_cc[VTIME] = 0;
    //store configuration
    tcsetattr(fd1, TCSANOW, &conf_tio);
    return fd1;
}

int fd1;
char endmsg = '\n';
bool writeflag = true;
int sleeptime = 5000; //us

void float_callback(const std_msgs::Float32MultiArray &serial_msg)
{
    while(writeflag==false){
        usleep(sleeptime);
    }
    writeflag == false;

    int datasize = serial_msg.data.size();
    char *floattochar;
    floattochar = new char[datasize * 4 + 6];
    floattochar[0] = 'f';
    *(int *)(&floattochar[1]) = datasize;
    //memcpy(&floattochar[1], &datasize, 4);
    for (int i = 0; i < datasize; i++)
    {
        *(float *)(&floattochar[i * 4 + 5]) = serial_msg.data[i];
        //memcpy(&floattochar[i * 4 + 5], &serial_msg.data[i], 4);
    }
    floattochar[datasize * 4 + 5] = endmsg;

    int rec = write(fd1, floattochar, datasize * 4 + 6);
    if (rec < 0)
    {
        ROS_ERROR_ONCE("Serial Fail: cound not write");
    }
    delete[] floattochar;
    writeflag = true;
}

void int_callback(const std_msgs::Int32MultiArray &serial_msg)
{
    while(writeflag==false){
        usleep(sleeptime);
    }
    writeflag == false;
    
    int datasize = serial_msg.data.size();
    char *inttochar;
    inttochar = new char[datasize * 4 + 6];
    inttochar[0] = 'i';
    *(int *)(&inttochar[1]) = datasize;
    //memcpy(&inttochar[1], &datasize, 4);
    for (int i = 0; i < datasize; i++)
    {
        *(int *)(&inttochar[i * 4 + 5]) = serial_msg.data[i];
        //memcpy(&inttochar[i * 4 + 5], &serial_msg.data[i], 4);
    }
    inttochar[datasize * 4 + 5] = endmsg;

    int rec = write(fd1, inttochar, datasize * 4 + 6);
    if (rec < 0)
    {
        ROS_ERROR_ONCE("Serial Fail: cound not write");
    }
    delete[] inttochar;
    writeflag = true;
}

void string_callback(const std_msgs::String &serial_msg)
{
    while(writeflag==false){
        usleep(sleeptime);
    }
    writeflag == false;
    
    int datasize = serial_msg.data.size();
    char *chartochar;
    chartochar = new char[datasize + 6];
    chartochar[0] = 'c';
    *(int *)(&chartochar[1]) = datasize;
    //memcpy(&chartochar[1], &datasize, 4);
    std::string str = serial_msg.data;
    memcpy(&chartochar[5], str.c_str(), datasize);
    chartochar[datasize + 5] = endmsg;

    int rec = write(fd1, chartochar, datasize + 6);
    if (rec < 0)
    {
        ROS_ERROR_ONCE("Serial Fail: cound not write");
    }
    delete[] chartochar;
    writeflag = true;
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "serial_test_node");
    ros::NodeHandle n;

    //Publisher
    ros::Publisher serial_pub_f = n.advertise<std_msgs::Float32MultiArray>("Serial_pub_float", 1000);
    ros::Publisher serial_pub_i = n.advertise<std_msgs::Int32MultiArray>("Serial_pub_int", 1000);
    ros::Publisher serial_pub_c = n.advertise<std_msgs::String>("Serial_pub_string", 1000);

    //Subscriber
    ros::Subscriber serial_sub_f = n.subscribe("Serial_sub_float", 100, float_callback);
    ros::Subscriber serial_sub_i = n.subscribe("Serial_sub_int", 100, int_callback);
    ros::Subscriber serial_sub_c = n.subscribe("Serial_sub_string", 100, string_callback);

    // Parameter
    ros::NodeHandle arg_n("~");
    std::string port_name = "/dev/ttyACM0";
    int sub_loop_rate = 200;
    arg_n.getParam("port", port_name);
    arg_n.getParam("baudrate", BAUDRATE);
    arg_n.getParam("looprate", sub_loop_rate);

    fd1 = open_serial(port_name.c_str());
    if (fd1 < 0)
    {
        ROS_ERROR("Serial Fail: cound not open %s", port_name.c_str());
        printf("Serial Fail\n");
        ros::shutdown();
    }

    char *buf_pub;
    buf_pub = new char[256];
    int recv_data_size = 0;
    int arraysize = 0;
    ros::Rate loop_rate(sub_loop_rate);
    while (ros::ok())
    {
        char buf[256] = {0};
        int recv_data = read(fd1, buf, sizeof(buf));
        //strcat(buf_pub,buf);
        if (recv_data > 0)
        {
            for (int i = 0; i < recv_data; i++)
            {
                buf_pub[recv_data_size + i] = buf[i];
            }
            recv_data_size += recv_data;
            //if (buf[recv_data - 1] == endmsg)
            if (buf_pub[recv_data_size - 1] == endmsg)
            {
                arraysize = *(int *)(&buf_pub[1]);
                //memcpy(&arraysize, &(buf_pub[1]), 4);

                switch (buf_pub[0])
                {
                case 'f':
                    if (recv_data_size == arraysize * 4 + 6)
                    {
                        std_msgs::Float32MultiArray pub_float;
                        pub_float.data.resize(arraysize);
                        for (int i = 0; i < arraysize; i++)
                        {
                            pub_float.data[i] = *(float *)(&buf_pub[i * 4 + 5]);
                            //memcpy(&pub_float.data[i], &buf_pub[i * 4 + 5], 4);
                        }
                        serial_pub_f.publish(pub_float);
                    }
                    else
                    {
                        ROS_INFO("Datasize Error");
                    }
                    break;
                case 'i':
                    if (recv_data_size == arraysize * 4 + 6)
                    {
                        std_msgs::Int32MultiArray pub_int;
                        pub_int.data.resize(arraysize);
                        for (int i = 0; i < arraysize; i++)
                        {
                            pub_int.data[i] = *(int *)(&buf_pub[i * 4 + 5]);
                            //memcpy(&pub_int.data[i], &buf_pub[i * 4 + 5], 4);
                        }
                        serial_pub_i.publish(pub_int);
                    }
                    else
                    {
                        ROS_INFO("Datasize Error");
                    }
                    break;
                case 'c':
                    if (recv_data_size == arraysize + 6)
                    {
                        std_msgs::String pub_string;
                        pub_string.data = &buf_pub[5];
                        serial_pub_c.publish(pub_string);
                    }
                    else
                    {
                        ROS_INFO("Datasize Error");
                    }
                    break;
                default:
                    ROS_INFO("Not float / int / char");
                }
                delete[] buf_pub;
                recv_data_size = 0;
                buf_pub = new char[256];
            }
            if (recv_data_size > 128)
            {
                delete[] buf_pub;
                recv_data_size = 0;
                buf_pub = new char[256];
                ROS_INFO("Buf Size Over");
            }
        }

        ros::spinOnce();
        loop_rate.sleep();
    }
    return 0;
}