#include "ros/ros.h"
#include "std_msgs/Float32MultiArray.h"
#include "std_msgs/Int32MultiArray.h"
#include "std_msgs/String.h"

#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <unistd.h> /////////////////////////////////

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
bool floatflag = false;
bool intflag = false;
bool charflag = false;
int sleeptime = 5000; //us

char *floattochar;
char *inttochar;
char *chartochar;
int floatdatasize = 0;
int intdatasize = 0;
int chardatasize = 0;

void float_callback(const std_msgs::Float32MultiArray &serial_msg)
{
    if (floatflag)
    {
        usleep(sleeptime);
        floatflag == false;
    }
    delete[] floattochar;
    floatdatasize = serial_msg.data.size();
    floattochar = new char[floatdatasize * 4 + 6];
    floattochar[0] = 'f';
    *(int *)(&floattochar[1]) = floatdatasize;
    for (int i = 0; i < floatdatasize; i++)
    {
        *(float *)(&floattochar[i * 4 + 5]) = serial_msg.data[i];
    }
    floattochar[floatdatasize * 4 + 5] = endmsg;

    floatflag = true;
}

void int_callback(const std_msgs::Int32MultiArray &serial_msg)
{
    if (intflag)
    {
        usleep(sleeptime);
        intflag == false;
    }
    delete[] inttochar;
    intdatasize = serial_msg.data.size();
    inttochar = new char[intdatasize * 4 + 6];
    inttochar[0] = 'i';
    *(int *)(&inttochar[1]) = intdatasize;
    for (int i = 0; i < intdatasize; i++)
    {
        *(int *)(&inttochar[i * 4 + 5]) = serial_msg.data[i];
    }
    inttochar[intdatasize * 4 + 5] = endmsg;

    intflag = true;
}

void string_callback(const std_msgs::String &serial_msg)
{
    if (charflag)
    {
        usleep(sleeptime);
        charflag == false;
    }
    delete[] chartochar;
    chardatasize = serial_msg.data.size();
    chartochar = new char[chardatasize + 6];
    chartochar[0] = 'c';
    *(int *)(&chartochar[1]) = chardatasize;
    std::string str = serial_msg.data;
    memcpy(&chartochar[5], str.c_str(), chardatasize);
    chartochar[chardatasize + 5] = endmsg;

    charflag = true;
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "serial_test_sync_node");
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
    
    while (ros::ok())
    {
        fd1 = open_serial(port_name.c_str());
        printf("Serial Connecting\n");
        sleep(1);
        if (fd1 >= 0)
            break;
    }

    ROS_INFO("Serial Success");

    char buf[256] = {0};
    int bufnum[2] = {0, 0}; // [read index, receive index]
    int bufthreshold = 128;
    bool endmsg_flag = false;
    bool skip_flag = false;
    char *buf_pub;
    int recv_data_size = 0;
    int arraysize = 0;
    int rec;
    ros::Rate loop_rate(sub_loop_rate);

    // remove initial_buff_data
    for (int i = 0; i < 1000; i++)
    {
        read(fd1, &buf_pub[0], sizeof(buf_pub));
        usleep(1000);
    }

    while (ros::ok())
    {
        int recv_data = read(fd1, &buf[bufnum[1]], sizeof(buf));

        if (recv_data > 0 && recv_data < bufthreshold)
        {
            bufnum[1] += recv_data;

            while (!(endmsg_flag || skip_flag))
            {
                recv_data_size++;
                if (buf[bufnum[0] + recv_data_size - 1] == endmsg)
                {
                    arraysize = *(int *)(&buf[bufnum[0] + 1]);
                    if (arraysize < 0 || arraysize > bufthreshold / 4){
                        skip_flag = true;
                        break;
                        }
                    switch (buf[bufnum[0]])
                    {
                    case 'f':
                    case 'i':
                        if (recv_data_size == arraysize * 4 + 6)
                            endmsg_flag = true;
                        else if (recv_data_size > arraysize * 4 + 6)
                            skip_flag = true;
                        break;
                    case 'c':
                        if (recv_data_size == arraysize + 6)
                            endmsg_flag = true;
                        else if (recv_data_size > arraysize + 6)
                            skip_flag = true;
                        break;
                    }
                }
                if (bufnum[0] + recv_data_size >= bufnum[1])
                {
                    break;
                }
            }

            if (endmsg_flag)
            {
                buf_pub = &buf[bufnum[0]];

                arraysize = *(int *)(&buf_pub[1]);

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
                        }
                        serial_pub_f.publish(pub_float);
                    }
                    else
                    {
                        ROS_INFO("Datasize Error Float");
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
                        }
                        serial_pub_i.publish(pub_int);
                    }
                    else
                    {
                        ROS_INFO("Datasize Error Int");
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
                        ROS_INFO("Datasize Error Char");
                    }
                    break;
                default:
                    ROS_INFO("Not Float / Int / Char");
                }
            }

            if (endmsg_flag || skip_flag)
            {
                bufnum[0] += recv_data_size;
                recv_data_size = 0;
                endmsg_flag = false;
                skip_flag = false;
                if (bufnum[0] >= bufthreshold)
                {
                    bufnum[1] -= bufnum[0];
                    for (int i = 0; i < bufnum[1]; i++)
                    {
                        buf[i] = buf[i + bufnum[0]];
                    }
                    bufnum[0] = 0;
                }
            }
        }

        // serial_write
        if (floatflag)
        {
            rec = write(fd1, floattochar, floatdatasize * 4 + 6);
            if (rec < 0)
            {
                ROS_ERROR_ONCE("Serial Fail: cound not write float");
            }
            floatflag = false;
        }
        else if (intflag)
        {
            rec = write(fd1, inttochar, intdatasize * 4 + 6);
            if (rec < 0)
            {
                ROS_ERROR_ONCE("Serial Fail: cound not write int");
            }
            intflag = false;
        }
        else if (charflag)
        {
            rec = write(fd1, chartochar, chardatasize + 6);
            if (rec < 0)
            {
                ROS_ERROR_ONCE("Serial Fail: cound not write char");
            }
            charflag = false;
        }

        ros::spinOnce();
        loop_rate.sleep();
    }
    return 0;
}