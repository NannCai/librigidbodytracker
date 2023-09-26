#include "librigidbodytracker/rigid_body_tracker.h"
#include "librigidbodytracker/cloudlog.hpp"
#include "yaml-cpp/yaml.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

// static std::string YAMLDIR = "../../../../crazyswarm/launch";
static std::string YAMLDIR = "../../../../crazyswarm2/ros_ws/src/crazyswarm/launch";
static void log_stderr(std::string s)
{
  std::cout << s << "\n";
}

static pcl::PointXYZ eig2pcl(Eigen::Vector3f v)
{
  return pcl::PointXYZ(v.x(), v.y(), v.z());
}

std::string wholefile(std::string path)
{
  std::cout << "wholefile function" << std::endl;
  std::cout << "path"<< path << std::endl;
  std::ifstream t(path);
  std::string str;

  t.seekg(0, std::ios::end); 
  
  str.reserve(t.tellg());   // wrong here

  t.seekg(0, std::ios::beg);

  str.assign((std::istreambuf_iterator<char>(t)),
              std::istreambuf_iterator<char>());

  return str;
}

YAML::Node rosparams()
{
  std::cout << "rosparams function" << std::endl;
  std::string file = wholefile(YAMLDIR + "/hover_swarm.launch");  // wrong here
  std::cout << "finish wholefile function" << std::endl;
  auto begin = file.find("<rosparam>") + strlen("<rosparam>");
  auto end = file.find("</rosparam>");
  std::cout << "begin "<<begin<< "end "<< end<< std::endl;   //egin352end1881
  
  return YAML::Load(file.substr(begin, end - begin));
  std::cout << "finish rosparams function" << std::endl;
  
}

Eigen::Vector3f asVec(YAML::Node const &node)
{
  assert(node.IsSequence());
  assert(node.size() == 3);
  return Eigen::Vector3f(
    node[0].as<float>(), node[1].as<float>(), node[2].as<float>());
}

static void readMarkerConfigurations(
  std::vector<librigidbodytracker::MarkerConfiguration> &markerConfigurations)
{
  std::cout << "readMarkerConfigurations" << std::endl;
  YAML::Node config_root = rosparams();  // something wrong here, before change the YAMLDIR
  auto markerRoot = config_root["markerConfigurations"];    // here have the wrong configure i think
  // std::cout << "config_root"<< std::endl;
  assert(markerRoot.IsMap());

  markerConfigurations.clear();
  for (auto &&config : markerRoot) {    // didnt goes into this loop
    auto val = config.second; // first is key
    assert(val.IsMap());
    auto offset = asVec(val["offset"]);
    markerConfigurations.push_back(pcl::PointCloud<pcl::PointXYZ>::Ptr(
      new pcl::PointCloud<pcl::PointXYZ>));
    for (auto &&point : val["points"]) {
      auto pt = asVec(point.second) + offset;
      markerConfigurations.back()->push_back(eig2pcl(pt));
    }
  }
}

static void readDynamicsConfigurations(
  std::vector<librigidbodytracker::DynamicsConfiguration>& dynamicsConfigurations)
{
  std::cout << "readDynamicsConfigurations function" << std::endl;
  YAML::Node config_root = rosparams();
  auto dynRoot = config_root["dynamicsConfigurations"];
  assert(dynRoot.IsMap());

  dynamicsConfigurations.clear();
  for (auto &&dyn : dynRoot) {
    auto val = dyn.second; // first is key
    assert(val.IsMap());
    dynamicsConfigurations.push_back(librigidbodytracker::DynamicsConfiguration());
    auto &conf = dynamicsConfigurations.back();
    conf.maxXVelocity = val["maxXVelocity"].as<float>();
    conf.maxYVelocity = val["maxYVelocity"].as<float>();
    conf.maxZVelocity = val["maxZVelocity"].as<float>();
    conf.maxPitchRate = val["maxPitchRate"].as<float>();
    conf.maxRollRate = val["maxRollRate"].as<float>();
    conf.maxYawRate = val["maxYawRate"].as<float>();
    conf.maxRoll = val["maxRoll"].as<float>();
    conf.maxPitch = val["maxPitch"].as<float>();
    conf.maxFitnessScore = val["maxFitnessScore"].as<float>();
  }
}

static void readObjects(std::vector<librigidbodytracker::RigidBody> &objects)
{
  std::cout << "readObjects function" << std::endl;
  YAML::Node cfs_root = YAML::LoadFile(YAMLDIR + "/crazyflies.yaml");
  auto cfs = cfs_root["crazyflies"];
  assert(cfs.IsSequence());
  for (auto &&cf : cfs) {
    assert(cf.IsMap());
    auto initPos = cf["initialPosition"];
    Eigen::Affine3f xf(Eigen::Translation3f(asVec(initPos)));
    objects.emplace_back(0, 0, xf, cf["id"].as<std::string>());
  }
}

int main(int argc, char **argv)
{
  using namespace librigidbodytracker;
  // std::cout << "argc"<< argc << std::endl;

  if (argc < 2) {
    std::cerr << "error: requires filename argument\n";
    return -1;
  }
  // else{
  //   std::cout << "argv[1]"<< argv[1] << std::endl;
  // }

  std::vector<DynamicsConfiguration> dynamicsConfigurations;
  std::vector<MarkerConfiguration> markerConfigurations;
  std::vector<RigidBody> objects;

  readMarkerConfigurations(markerConfigurations);   // wrong here
  //readMarkerConfigurations -rosparams()- wholefile- str.reserve(t.tellg()); 
  readDynamicsConfigurations(dynamicsConfigurations);  // then wrong here
  // readDynamicsConfigurations- rosparams- wholefile 
  std::cout << "before readObjects" << std::endl;
  
  readObjects(objects);

  std::cout << dynamicsConfigurations.size() << " dynamics configurations, "
            << markerConfigurations.size() << " marker configurations, "
            << objects.size() << " crazyflies.\n";

  librigidbodytracker::RigidBodyTracker tracker(
      dynamicsConfigurations,
      markerConfigurations,
      objects);

  tracker.setLogWarningCallback(&log_stderr);
  if (argc < 3) {
    PointCloudPlayer player;
    player.load(argv[1]);
    player.play(tracker);
  }
  else {
    PointCloudDebugger debugger(argv[2]);
    debugger.load(argv[1]);
    debugger.convert(tracker,markerConfigurations);
  }
}
