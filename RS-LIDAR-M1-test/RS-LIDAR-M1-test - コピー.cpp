﻿

#include <pcl/point_types.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/io/pcd_io.h> 

#include "rs_driver/api/lidar_driver.h"

#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <thread>


using namespace robosense::lidar;
using namespace pcl::visualization;
std::shared_ptr<PCLVisualizer> pcl_viewer;
std::mutex mex_viewer;

int savecount = 0;
std::stringstream writepath;
bool isSave = false;


bool checkKeywordExist(int argc, const char* const* argv, const char* str)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], str) == 0)
        {
            return true;
        }
    }
    return false;
}

bool parseArgument(int argc, const char* const* argv, const char* str, std::string& val)
{
    int index = -1;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], str) == 0)
        {
            index = i + 1;
        }
    }
    if (index > 0 && index < argc)
    {
        val = argv[index];
        return true;
    }
    return false;
}

void parseParam(int argc, char* argv[], RSDriverParam& param)
{
    param.wait_for_difop = false;
    std::string result_str;
    if (parseArgument(argc, argv, "-type", result_str))
    {
        param.lidar_type = param.strToLidarType(result_str);
    }
    if (parseArgument(argc, argv, "-msop", result_str))
    {
        param.input_param.msop_port = std::stoi(result_str);
    }
    if (parseArgument(argc, argv, "-difop", result_str))
    {
        param.input_param.difop_port = std::stoi(result_str);
    }

    parseArgument(argc, argv, "-host", param.input_param.host_address);
    parseArgument(argc, argv, "-multi_cast", param.input_param.multi_cast_address);

    if (parseArgument(argc, argv, "-x", result_str))
    {
        param.decoder_param.transform_param.x = std::stof(result_str);
    }
    if (parseArgument(argc, argv, "-y", result_str))
    {
        param.decoder_param.transform_param.y = std::stof(result_str);
    }
    if (parseArgument(argc, argv, "-z", result_str))
    {
        param.decoder_param.transform_param.z = std::stof(result_str);
    }
    if (parseArgument(argc, argv, "-roll", result_str))
    {
        param.decoder_param.transform_param.roll = std::stof(result_str);
    }
    if (parseArgument(argc, argv, "-pitch", result_str))
    {
        param.decoder_param.transform_param.pitch = std::stof(result_str);
    }
    if (parseArgument(argc, argv, "-yaw", result_str))
    {
        param.decoder_param.transform_param.yaw = std::stof(result_str);
    }
    if (parseArgument(argc, argv, "-pcap", param.input_param.pcap_path))
    {
        param.input_param.read_pcap = true;
    }
}

void printHelpMenu()
{
    RS_MSG << "Arguments are: " << RS_REND;
    RS_MSG << "        -msop             = LiDAR msop port number,the default value is 6699" << RS_REND;
    RS_MSG << "        -difop            = LiDAR difop port number,the default value is 7788" << RS_REND;
    RS_MSG << "        -host             = LiDAR destination address." << RS_REND;
    RS_MSG << "        -multi_cast       = LiDAR destination multicast group address." << RS_REND;
    RS_MSG << "        -type             = LiDAR type( RS16, RS32, RSBP, RS128, RS80, RSM1, RSHELIOS, RSROCK ), the "
        "default "
        "value is RS16"
        << RS_REND;
    RS_MSG << "        -x                = Transformation parameter, unit: m " << RS_REND;
    RS_MSG << "        -y                = Transformation parameter, unit: m " << RS_REND;
    RS_MSG << "        -z                = Transformation parameter, unit: m " << RS_REND;
    RS_MSG << "        -roll             = Transformation parameter, unit: radian " << RS_REND;
    RS_MSG << "        -pitch            = Transformation parameter, unit: radian " << RS_REND;
    RS_MSG << "        -yaw              = Transformation parameter, unit: radian " << RS_REND;
    RS_MSG << "        -pcap             = The path of the pcap file, if this argument is set, the driver "
        "will work in off-line mode and read the pcap file. Otherwise the driver work in online mode."
        << RS_REND;
}

void printParam(const RSDriverParam& param)
{
    if (param.input_param.read_pcap)
    {
        RS_INFOL << "Working mode: ";
        RS_INFO << "Offline Pcap " << RS_REND;
        RS_INFOL << "Pcap Path: ";
        RS_INFO << param.input_param.pcap_path << RS_REND;
    }
    else
    {
        RS_INFOL << "Working mode: ";
        RS_INFO << "Online LiDAR " << RS_REND;
    }
    RS_INFOL << "MSOP Port: ";
    RS_INFO << param.input_param.msop_port << RS_REND;
    RS_INFOL << "DIFOP Port: ";
    RS_INFO << param.input_param.difop_port << RS_REND;
    RS_INFOL << "Host Adress: ";
    RS_INFO << param.input_param.host_address << RS_REND;
    RS_INFOL << "Multi-cast Group Adress: ";
    RS_INFO << param.input_param.multi_cast_address << RS_REND;
    RS_INFOL << "LiDAR Type: ";
    RS_INFO << param.lidarTypeToStr(param.lidar_type) << RS_REND;
    RS_INFOL << "Transformation Parameters (x, y, z, roll, pitch, yaw): " << RS_REND;
    RS_INFOL << "x: ";
    RS_INFO << std::fixed << param.decoder_param.transform_param.x << RS_REND;
    RS_INFOL << "y: ";
    RS_INFO << std::fixed << param.decoder_param.transform_param.y << RS_REND;
    RS_INFOL << "z: ";
    RS_INFO << std::fixed << param.decoder_param.transform_param.z << RS_REND;
    RS_INFOL << "roll: ";
    RS_INFO << std::fixed << param.decoder_param.transform_param.roll << RS_REND;
    RS_INFOL << "pitch: ";
    RS_INFO << std::fixed << param.decoder_param.transform_param.pitch << RS_REND;
    RS_INFOL << "yaw: ";
    RS_INFO << std::fixed << param.decoder_param.transform_param.yaw << RS_REND;
}
/**
 * @brief The point cloud callback function. This function will be registered to lidar driver.
 *              When the point cloud message is ready, driver can send out messages through this function.
 * @param msg  The lidar point cloud message.
 */
void pointCloudCallback(const PointCloudMsg<pcl::PointXYZI>& msg)
{
    
    /* Note: Please do not put time-consuming operations in the callback function! */
    /* Make a copy of the message and process it in another thread is recommended*/
    pcl::PointCloud<pcl::PointXYZI>::Ptr pcl_pointcloud(new pcl::PointCloud<pcl::PointXYZI>);
    pcl_pointcloud->points.assign(msg.point_cloud_ptr->begin(), msg.point_cloud_ptr->end());
    pcl_pointcloud->height = msg.height;
    pcl_pointcloud->width = msg.width;
    pcl_pointcloud->is_dense = false;
    PointCloudColorHandlerGenericField<pcl::PointXYZI> point_color_handle(pcl_pointcloud, "intensity");
    {
        const std::lock_guard<std::mutex> lock(mex_viewer);
        pcl_viewer->updatePointCloud<pcl::PointXYZI>(pcl_pointcloud, point_color_handle, "rslidar");
    }

    if (isSave == true) {
        writepath << "pcd_data_" << savecount << ".pcd";
        pcl::io::savePCDFileASCII(writepath.str(), *pcl_pointcloud);//バイナリデータの場合savePCDFileBinary
        writepath.str("");
        savecount++;
        RS_INFO << "seve data" << RS_REND;
        isSave = false;
    }
}

/**
 * @brief The exception callback function. This function will be registered to lidar driver.
 * @param code The error code struct.
 */
void exceptionCallback(const Error& code)
{
    /* Note: Please do not put time-consuming operations in the callback function! */
    /* Make a copy of the error message and process it in another thread is recommended*/
    RS_WARNING << code.toString() << RS_REND;
}

int main(int argc, char* argv[])
{
    
    if (argc < 2)
    {
        RS_INFOL << "Use 'rs_driver_viewer -h/--help' to check the argument menu..." << RS_REND;
    }
    if (checkKeywordExist(argc, argv, "-h") || checkKeywordExist(argc, argv, "--help"))
    {
        printHelpMenu();
        return 0;
    }

    pcl_viewer = std::make_shared<PCLVisualizer>("RSPointCloudViewer");
    pcl_viewer->setBackgroundColor(0.0, 0.0, 0.0);
    pcl_viewer->addCoordinateSystem(1.0);
    pcl::PointCloud<pcl::PointXYZI>::Ptr pcl_pointcloud(new pcl::PointCloud<pcl::PointXYZI>);
    pcl_viewer->addPointCloud<pcl::PointXYZI>(pcl_pointcloud, "rslidar");
    pcl_viewer->setPointCloudRenderingProperties(PCL_VISUALIZER_POINT_SIZE, 2, "rslidar");

    LidarDriver<pcl::PointXYZI> driver;  ///< Declare the driver object

    
    RSDriverParam param;     
    parseParam(argc, argv, param);
    printParam(param);
    driver.regExceptionCallback(exceptionCallback);  ///< Register the exception callback function into the driver
    driver.regRecvCallback(pointCloudCallback);      ///< Register the point cloud callback function into the driver
    if (!driver.init(param))                         ///< Call the init function and pass the parameter
    {
        RS_ERROR << "Driver Initialize Error..." << RS_REND;
        return -1;
    }
    
    driver.start();  ///< The driver thread will start
    RS_INFO << "RoboSense Lidar-Driver Viewer start......" << RS_REND;

    while (!pcl_viewer->wasStopped())
    {
        {
            const std::lock_guard<std::mutex> lock(mex_viewer);
            pcl_viewer->spinOnce();
        }

        if (GetAsyncKeyState('S')) { 
            isSave = true;
        }
     
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}
