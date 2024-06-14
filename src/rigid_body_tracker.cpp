#include "librigidbodytracker/rigid_body_tracker.h"

// PCL
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/common/transforms.h>
#include <pcl/registration/icp.h>
#include <pcl/registration/transformation_estimation_2D.h>
// #include <pcl/registration/transformation_estimation_lm.h>
#include <pcl/search/impl/search.hpp>

#include <set>
#include "assignment.hpp"
#include "cbs_group_constraint.hpp"
#include <random>


#include <limits>

// TEMP for debug
#include <cstdio>

using Point = pcl::PointXYZ;
using Cloud = pcl::PointCloud<Point>;
using ICP = pcl::IterativeClosestPoint<Point, Point>;

// enum TrackingMode {
//     PositionMode,
//     PoseMode,
//     HybridMode
// };

static Eigen::Vector3f pcl2eig(Point p)
{
  return Eigen::Vector3f(p.x, p.y, p.z);
}

static Point eig2pcl(Eigen::Vector3f v)
{
  return Point(v.x(), v.y(), v.z());
}

static float deltaAngle(float a, float b)
{
  return atan2(sin(a-b), cos(a-b));
}

namespace librigidbodytracker {

/////////////////////////////////////////////////////////////

RigidBody::RigidBody(
  size_t markerConfigurationIdx,
  size_t dynamicsConfigurationIdx,
  const Eigen::Affine3f& initialTransformation,
  const std::string& name)
  : m_markerConfigurationIdx(markerConfigurationIdx)
  , m_dynamicsConfigurationIdx(dynamicsConfigurationIdx)
  , m_lastTransformation(initialTransformation)
  , m_hasOrientation(false)
  , m_initialTransformation(initialTransformation)
  , m_lastValidTransform()
  , m_lastTransformationValid(false)
  , m_name(name)
{
}

const Eigen::Affine3f& RigidBody::transformation() const
{
  return m_lastTransformation;
}

const Eigen::Affine3f& RigidBody::initialTransformation() const
{
  return m_initialTransformation;
}

bool RigidBody::lastTransformationValid() const
{
  return m_lastTransformationValid;
}

/////////////////////////////////////////////////////////////

RigidBodyTracker::RigidBodyTracker(
  const std::vector<DynamicsConfiguration>& dynamicsConfigurations,
  const std::vector<MarkerConfiguration>& markerConfigurations,
  const std::vector<RigidBody>& rigidBodies)  // rigidBodies is the paras of rbs passinto the object m_rigidBodies
  : m_markerConfigurations(markerConfigurations)
  , m_dynamicsConfigurations(dynamicsConfigurations)
  , m_rigidBodies(rigidBodies)   // now m_rigidBodies is already have all the config paras from yaml file
  , m_trackPositionOnly(false)
  , m_trackingMode(PositionMode)
  , m_initialized(false)
  , m_init_attempts(0)
  , m_logWarn()
{
  // if at least one of the used marker configurations only has one marker,
  // then switch to position only tracking

  for (const RigidBody& rigidBody : m_rigidBodies) {
    Cloud::Ptr &rbMarkers = m_markerConfigurations[rigidBody.m_markerConfigurationIdx];
    size_t const rbNpts = rbMarkers->size();
    if (rbNpts == 1) {
      // m_trackingMode = PositionMode;
      m_trackPositionOnly = true;
    }
    else if(rbNpts > 1){
      m_trackingMode = PoseMode;
    }
  }

  if (m_trackPositionOnly && m_trackingMode == PoseMode) {
    // throw std::runtime_error("Cannot use single-marker and multi-marker configurations simultaneously.");
    std::cout << "!!has two marker mode!! developing the hybrid mode algorithm\n" ;
    m_trackingMode = HybridMode;
  }

}


void RigidBodyTracker::update(Cloud::Ptr pointCloud)
{
  update(std::chrono::high_resolution_clock::now(), pointCloud);
}
void RigidBodyTracker::update(std::chrono::high_resolution_clock::time_point time,
  Cloud::Ptr pointCloud, std::string inputP)
{
  // if (m_trackPositionOnly) {
  //   updatePosition(time, pointCloud);
  // } else {
  //   updatePose(time, pointCloud);
  // }

  // if (m_trackingMode == PositionMode) {
  //   updatePosition(time, pointCloud);
  // } else if (m_trackingMode == PoseMode) {
  //   updatePose(time, pointCloud);
  // }
  // else if (m_trackingMode == HybridMode){
  //   updateHybrid(time, pointCloud);
  // }
  // updatePose(time, pointCloud);
  inputPath = inputP;
  updateHybrid(time, pointCloud);

}

const std::vector<RigidBody>& RigidBodyTracker::rigidBodies() const
{
  return m_rigidBodies;
}

void RigidBodyTracker::setLogWarningCallback(
  std::function<void(const std::string&)> logWarn)
{
  m_logWarn = logWarn;
}

bool RigidBodyTracker::initializePose(Cloud::ConstPtr markersConst)
{
  std::cout << "-initializePose function-" << std::endl;

  if (markersConst->size() == 0) {
    return false;
  }

  // we need to mutate the cloud by deleting points
  // once they are assigned to an rigid body
  Cloud::Ptr markers(new Cloud(*markersConst));

  size_t const numRigidBodies = m_rigidBodies.size();

  ICP icp;
  icp.setMaximumIterations(5);
  icp.setInputTarget(markers);

  // prepare for knn query
  std::vector<int> nearestIdx;
  std::vector<float> nearestSqrDist;
  std::vector<int> rbTakePts;
  pcl::KdTreeFLANN<Point> kdtree;
  kdtree.setInputCloud(markers);

  // compute the distance between the closest 2 rigidBodies in the nominal configuration
  // we will use this value to limit allowed deviation from nominal positions
  float closest = std::numeric_limits<float>::max();
  for (int i = 0; i < numRigidBodies; ++i) {
    auto pi = m_rigidBodies[i].initialCenter();
    for (int j = i + 1; j < numRigidBodies; ++j) {
      float dist = (pi - m_rigidBodies[j].initialCenter()).norm();
      closest = std::min(closest, dist);
    }
  }
  float const max_deviation = closest / 3;

  //printf("Rigid Body tracker: limiting distance from nominal position "
  //  "to %f meters\n", max_deviation);

  bool allFitsGood = true;
  for (int iRb = 0; iRb < numRigidBodies; ++iRb) {
    RigidBody& rigidBody = m_rigidBodies[iRb];
    Cloud::Ptr &rbMarkers =
      m_markerConfigurations[rigidBody.m_markerConfigurationIdx];
    icp.setInputSource(rbMarkers);

    // find the points nearest to the rigidBodie's nominal position
    // (initial pos was loaded into lastTransformation from config file)
    size_t const rbNpts = rbMarkers->size();
    nearestIdx.resize(rbNpts);
    nearestSqrDist.resize(rbNpts);
    auto nominalCenter = eig2pcl(rigidBody.initialCenter());
    int nFound = kdtree.nearestKSearch(
      nominalCenter, rbNpts, nearestIdx, nearestSqrDist);

    if (nFound < rbNpts) {
      std::stringstream sstr;
      sstr << "error: only " << nFound
           << " neighbors found for rigid body " << rigidBody.name()
           << " (need " << rbNpts << ")";
      logWarn(sstr.str());
      allFitsGood = false;
      continue;
    }

    // only try to fit the rigid body if the k nearest neighbors
    // are reasonably close to the nominal rigid body position
    Eigen::Vector3f actualCenter(0, 0, 0);
    for (int i = 0; i < rbNpts; ++i) {
      actualCenter += pcl2eig((*markers)[nearestIdx[i]]);
    }
    actualCenter /= rbNpts;
    if ((actualCenter - pcl2eig(nominalCenter)).norm() > max_deviation) {
      std::stringstream sstr;
      sstr << "error: nearest neighbors of rigid body " << rigidBody.name()
           << " are centered at " << actualCenter
           << " instead of " << nominalCenter;
      logWarn(sstr.str());
      allFitsGood = false;
      continue;
    }

    // try ICP with guesses of many different yaws about knn centroid
    Cloud result;
    static int const N_YAW = 20;
    double bestErr = std::numeric_limits<double>::max();
    Eigen::Affine3f bestTransformation;
    for (int i = 0; i < N_YAW; ++i) {
      float yaw = i * (2 * M_PI / N_YAW);
      Eigen::Matrix4f tryMatrix = pcl::getTransformation(
        actualCenter.x(), actualCenter.y(), actualCenter.z(),
        0, 0, yaw).matrix();
      icp.align(result, tryMatrix);
      if (icp.hasConverged()) {
        double err = icp.getFitnessScore();
        if (err < bestErr) {
          bestErr = err;
          bestTransformation = icp.getFinalTransformation();
        }
      }
    }

    const DynamicsConfiguration& dynConf = m_dynamicsConfigurations[rigidBody.m_dynamicsConfigurationIdx];
    if (bestErr >= dynConf.maxFitnessScore) {
      std::stringstream sstr;
      sstr << "Initialize did not succeed (fitness too low) "
           << " for rigidBody " << rigidBody.name();
      logWarn(sstr.str());
      allFitsGood = false;
      continue;
    }

    // if the fit was good, this rigid body "takes" the markers, and they become
    // unavailable to all other rigidBodies so we don't double-assign markers
    // (TODO: this is so greedy... do we need a more global approach?)
    rigidBody.m_lastTransformation = bestTransformation;
    // remove highest indices first
    std::sort(rbTakePts.rbegin(), rbTakePts.rend());
    for (int idx : rbTakePts) {
      markers->erase(markers->begin() + idx);
    }
    // update search structures after deleting markers
    icp.setInputTarget(markers);
    kdtree.setInputCloud(markers);
  }

  ++m_init_attempts;
  return allFitsGood;
}

void RigidBodyTracker::updatePose(std::chrono::high_resolution_clock::time_point stamp,
  Cloud::ConstPtr markers)
{
  std::cout << "-updatePose function-"<< std::endl ;

  if (markers->empty()) {
    for (auto& rigidBody : m_rigidBodies) {
      rigidBody.m_lastTransformationValid = false;
    }
    return;
  }

  m_initialized = m_initialized || initializePose(markers);
  if (!m_initialized) {
    logWarn(
      "rigid body tracker initialization failed - "
      "check that position is correct, all markers are visible, "
      "and marker configuration matches config file");
    // Doesn't make too much sense to continue here - lets wait to be fully initialized
    return;
  }

  ICP icp;
  // pcl::registration::TransformationEstimationLM<Point, Point>::Ptr trans(new pcl::registration::TransformationEstimationLM<Point, Point>);
  // pcl::registration::TransformationEstimation2D<Point, Point>::Ptr trans(new pcl::registration::TransformationEstimation2D<Point, Point>);
  // pcl::registration::TransformationEstimation3DYaw<Point, Point>::Ptr trans(new pcl::registration::TransformationEstimation3DYaw<Point, Point>);
  // icp.setTransformationEstimation(trans);


  // // Set the maximum number of iterations (criterion 1)
  icp.setMaximumIterations(5);
  // // Set the transformation epsilon (criterion 2)
  // icp.setTransformationEpsilon(1e-8);
  // // Set the euclidean distance difference epsilon (criterion 3)
  // icp.setEuclideanFitnessEpsilon(1);

  icp.setInputTarget(markers);

  for (auto& rigidBody : m_rigidBodies) {
    rigidBody.m_lastTransformationValid = false;

    std::chrono::duration<double> elapsedSeconds = stamp-rigidBody.m_lastValidTransform;
    double dt = elapsedSeconds.count();

    // Set the max correspondence distance
    const DynamicsConfiguration& dynConf = m_dynamicsConfigurations[rigidBody.m_dynamicsConfigurationIdx];
    float maxV = dynConf.maxXVelocity;
    icp.setMaxCorrespondenceDistance(maxV * dt);
    // ROS_INFO("max: %f", maxV * dt);

    // Update input source
    icp.setInputSource(m_markerConfigurations[rigidBody.m_markerConfigurationIdx]);

    // Perform the alignment
    Cloud result;
    // auto deltaPos = Eigen::Translation3f(dt * rigidBody.m_velocity);
    // auto predictTransform = deltaPos * rigidBody.m_lastTransformation;
    auto predictTransform = rigidBody.m_lastTransformation;
    icp.align(result, predictTransform.matrix());
    if (!icp.hasConverged()) {
      // ros::Time t = ros::Time::now();
      // ROS_INFO("ICP did not converge %d.%d", t.sec, t.nsec);
      std::stringstream sstr;
      sstr << "ICP did not converge!"
           << " for rigidBody " << rigidBody.name();
      logWarn(sstr.str());
      continue;
    }

    // Obtain the transformation that aligned cloud_source to cloud_source_registered
    Eigen::Matrix4f transformation = icp.getFinalTransformation();
    // std::cout << "icp.getFinalTransformation():  \n"<< transformation << "\n";

    Eigen::Affine3f tROTA(transformation);
    float x, y, z, roll, pitch, yaw;
    pcl::getTranslationAndEulerAngles(tROTA, x, y, z, roll, pitch, yaw);

    // Compute changes:
    float last_x, last_y, last_z, last_roll, last_pitch, last_yaw;
    pcl::getTranslationAndEulerAngles(rigidBody.m_lastTransformation, last_x, last_y, last_z, last_roll, last_pitch, last_yaw);

    float vx = (x - last_x) / dt;
    float vy = (y - last_y) / dt;
    float vz = (z - last_z) / dt;
    float wroll = deltaAngle(roll, last_roll) / dt;
    float wpitch = deltaAngle(pitch, last_pitch) / dt;
    float wyaw = deltaAngle(yaw, last_yaw) / dt;

    // ROS_INFO("v: %f,%f,%f, w: %f,%f,%f, dt: %f", vx, vy, vz, wroll, wpitch, wyaw, dt);

    if (   fabs(vx) < dynConf.maxXVelocity
        && fabs(vy) < dynConf.maxYVelocity
        && fabs(vz) < dynConf.maxZVelocity
        && fabs(wroll) < dynConf.maxRollRate
        && fabs(wpitch) < dynConf.maxPitchRate
        && fabs(wyaw) < dynConf.maxYawRate
        && fabs(roll) < dynConf.maxRoll
        && fabs(pitch) < dynConf.maxPitch
        && icp.getFitnessScore() < dynConf.maxFitnessScore)
    {
      rigidBody.m_velocity = (tROTA.translation() - rigidBody.center()) / dt;
      rigidBody.m_lastTransformation = tROTA;  // tROTA is an object, so m_lastTransformation is also an object of class Eigen::Affine3f
      rigidBody.m_lastValidTransform = stamp;
      rigidBody.m_lastTransformationValid = true;
      rigidBody.m_hasOrientation = true;
    } else {
      std::stringstream sstr;
      sstr << "Dynamic check failed for rigidBody " << rigidBody.name() << std::endl;
      if (fabs(vx) >= dynConf.maxXVelocity) {
        sstr << "vx: " << vx << " >= " << dynConf.maxXVelocity << std::endl;
      }
      if (fabs(vy) >= dynConf.maxYVelocity) {
        sstr << "vy: " << vy << " >= " << dynConf.maxYVelocity << std::endl;
      }
      if (fabs(vz) >= dynConf.maxZVelocity) {
        sstr << "vz: " << vz << " >= " << dynConf.maxZVelocity << std::endl;
      }
      if (fabs(wroll) >= dynConf.maxRollRate) {
        sstr << "wroll: " << wroll << " >= " << dynConf.maxRollRate << std::endl;
      }
      if (fabs(wpitch) >= dynConf.maxPitchRate) {
        sstr << "wpitch: " << wpitch << " >= " << dynConf.maxPitchRate << std::endl;
      }
      if (fabs(wyaw) >= dynConf.maxYawRate) {
        sstr << "wyaw: " << wyaw << " >= " << dynConf.maxYawRate << std::endl;
      }
      if (fabs(roll) >= dynConf.maxRoll) {
        sstr << "roll: " << roll << " >= " << dynConf.maxRoll << std::endl;
      }
      if (fabs(pitch) >= dynConf.maxPitch) {
        sstr << "pitch: " << pitch << " >= " << dynConf.maxPitch << std::endl;
      }
      if (icp.getFitnessScore() >= dynConf.maxFitnessScore) {
        sstr << "fitness: " << icp.getFitnessScore() << " >= " << dynConf.maxFitnessScore << std::endl;
      }
      logWarn(sstr.str());
    }
    std::cout << "rigidBody.m_lastTransformation.matrix():\n"<< rigidBody.m_lastTransformation.matrix() << "\n"; 

  }

}

bool RigidBodyTracker::initializePosition(
  std::chrono::high_resolution_clock::time_point stamp,
  Cloud::ConstPtr markers)
{
  // Here, we use a simple task assignment to find the best initial matching
  libMultiRobotPlanning::Assignment<size_t, size_t> assignment;

  for (size_t i = 0; i < markers->size(); ++i) {
    Eigen::Vector3f marker = pcl2eig((*markers)[i]);
    for (size_t j = 0; j < m_rigidBodies.size(); ++j) {
      auto pi = m_rigidBodies[j].initialCenter();
      float dist = (pi - marker).norm();
      long cost = dist * 1000; // cost needs to be an integer -> convert to mm
      assignment.setCost(j, i, cost);
    }
  }

  std::map<size_t, size_t> solution; // maps rigidBodyId->markerId
  long totalCost = assignment.solve(solution);

  for (const auto& s : solution) {
    auto& rigidBody = m_rigidBodies[s.first];
    Eigen::Vector3f marker = pcl2eig((*markers)[s.second]);
    Eigen::Vector3f offset = pcl2eig((*m_markerConfigurations[rigidBody.m_markerConfigurationIdx])[0]);
    rigidBody.m_lastTransformation = Eigen::Translation3f(marker + offset);
    rigidBody.m_lastValidTransform = stamp;
    rigidBody.m_lastTransformationValid = true;
    rigidBody.m_hasOrientation = false;
  }

  return solution.size() == m_rigidBodies.size();
}

void RigidBodyTracker::updatePosition(std::chrono::high_resolution_clock::time_point stamp,
  Cloud::ConstPtr markers)
{
  static std::chrono::high_resolution_clock::time_point lastCall;
  std::chrono::duration<double> lastCallElapsedSeconds = stamp-lastCall;
  double lastCalldt = lastCallElapsedSeconds.count();
  lastCall = stamp;

  if (markers->empty()) {
    for (auto& rigidBody : m_rigidBodies) {
      rigidBody.m_lastTransformationValid = false;
    }
    return;
  }

  // re-initialize, if we have not received an update in a long time
  if (!m_initialized || lastCalldt > 0.4) {
    m_initialized = initializePosition(stamp, markers);
    if (!m_initialized) {
      logWarn(
        "rigid body tracker initialization failed - "
        "check that position is correct, all markers are visible, "
        "and marker configuration matches config file");
    }
    // Doesn't make too much sense to continue here - lets wait to be fully initialized
    return;
  }

  // In this case, we setup a task assignment problem, only considering markers that are in
  // close proximity to the previously known position. If we do not have a match for a
  // fixed amount of time, abandon that robot entirely (to avoid issues with spurios markers).
  libMultiRobotPlanning::Assignment<size_t, size_t> assignment; // rigidBodyIdx -> markerIdx

  // prepare for knn query
  std::vector<int> nearestIdx(5); // tune maximum number of neighbors here
  std::vector<float> nearestSqrDist(nearestIdx.size());
  pcl::KdTreeFLANN<Point> kdtree;
  kdtree.setInputCloud(markers);

  size_t const numRigidBodies = m_rigidBodies.size();
  for (int iRb = 0; iRb < numRigidBodies; ++iRb) {
    RigidBody& rigidBody = m_rigidBodies[iRb];
    Eigen::Vector3f offset = pcl2eig((*m_markerConfigurations[rigidBody.m_markerConfigurationIdx])[0]);

    rigidBody.m_lastTransformationValid = false;

    std::chrono::duration<double> elapsedSeconds = stamp-rigidBody.m_lastValidTransform;
    double dt = elapsedSeconds.count();
    if (dt > 0.5) {
      std::stringstream sstr;
      sstr << "Lost tracking for rigidBody " << rigidBody.name() << " skipping";
      logWarn(sstr.str());
      continue;
    }

    auto nominalCenter = eig2pcl(rigidBody.center());
    int nFound = kdtree.nearestKSearch(
      nominalCenter, nearestIdx.size(), nearestIdx, nearestSqrDist);

    if (nFound < 1) {
      std::stringstream sstr;
      sstr << "error: no neighbors found for rigidBody " << rigidBody.name();
      logWarn(sstr.str());
      continue;
    }

    const DynamicsConfiguration& dynConf = m_dynamicsConfigurations[rigidBody.m_dynamicsConfigurationIdx];

    bool foundPotentialMarker = false;
    for (int iMarker = 0; iMarker < nFound; ++iMarker) {   // loop all the near markers
      Eigen::Vector3f marker = pcl2eig((*markers)[nearestIdx[iMarker]]);

      // Compute changes:
      Eigen::Vector3f velocity = (marker - rigidBody.center() + offset) / dt;
      float vx = velocity.x();
      float vy = velocity.y();
      float vz = velocity.z();

      if (   fabs(vx) < dynConf.maxXVelocity
          && fabs(vy) < dynConf.maxYVelocity
          && fabs(vz) < dynConf.maxZVelocity)
      {
        float dist = (marker - rigidBody.center() + offset).norm();
        long cost = dist * 1000; // cost needs to be an integer -> convert to mm
        assignment.setCost(iRb, nearestIdx[iMarker], cost);
        foundPotentialMarker = true;
      }
    }
    if (!foundPotentialMarker) {
      std::stringstream sstr;
      sstr << "all dynamic check failed for rigidBody " << rigidBody.name() << std::endl;
      logWarn(sstr.str());
    }
  }

  std::map<size_t, size_t> solution; // maps rigidBodyId->markerId
  long totalCost = assignment.solve(solution);

  for (const auto& s : solution) {
    auto& rigidBody = m_rigidBodies[s.first];
    Eigen::Vector3f marker = pcl2eig((*markers)[s.second]);
    Eigen::Vector3f offset = pcl2eig((*m_markerConfigurations[rigidBody.m_markerConfigurationIdx])[0]);
    std::chrono::duration<double> elapsedSeconds = stamp-rigidBody.m_lastValidTransform;
    double dt = elapsedSeconds.count();

    rigidBody.m_velocity = (marker - rigidBody.center() + offset) / dt;
    rigidBody.m_lastTransformation = Eigen::Translation3f(marker + offset);
    rigidBody.m_lastValidTransform = stamp;
    rigidBody.m_lastTransformationValid = true;
    rigidBody.m_hasOrientation = false;
  }
}


bool RigidBodyTracker::initializeHybrid_old(Cloud::ConstPtr markers)
{
  std::cout << "-initializeHybrid function-"<< std::endl;
  // use the config to initial
  size_t const numRigidBodies = m_rigidBodies.size();
  for (int iRb = 0; iRb < numRigidBodies; ++iRb) {
    RigidBody& rigidBody = m_rigidBodies[iRb];

    Eigen::Vector3f configinitialCenter = rigidBody.initialCenter();
    Eigen::Matrix4f configTransformation = pcl::getTransformation(
      configinitialCenter.x(), configinitialCenter.y(), configinitialCenter.z(),
      0, 0, 0).matrix();

    rigidBody.m_lastTransformation = configTransformation;
    std::cout << "rigidBody.m_lastTransformation:  \n"<< configTransformation << "\n";
  }
  return true;  
}

bool RigidBodyTracker::initializeHybrid(Cloud::ConstPtr markersConst)
{
  std::cout << "-initializeHybrid function-" << std::endl;

  if (markersConst->size() == 0) {
    return false;
  }

  // we need to mutate the cloud by deleting points
  // once they are assigned to an rigid body
  Cloud::Ptr markers(new Cloud(*markersConst));

  size_t const numRigidBodies = m_rigidBodies.size();

  ICP icp;
  icp.setMaximumIterations(5);
  icp.setInputTarget(markers);

  // prepare for knn query
  std::vector<int> nearestIdx;
  std::vector<float> nearestSqrDist;
  std::vector<int> rbTakePts;
  pcl::KdTreeFLANN<Point> kdtree;
  kdtree.setInputCloud(markers);

  // compute the distance between the closest 2 rigidBodies in the nominal configuration
  // we will use this value to limit allowed deviation from nominal positions
  float closest = std::numeric_limits<float>::max();
  for (int i = 0; i < numRigidBodies; ++i) {
    auto pi = m_rigidBodies[i].initialCenter();
    for (int j = i + 1; j < numRigidBodies; ++j) {
      float dist = (pi - m_rigidBodies[j].initialCenter()).norm();
      closest = std::min(closest, dist);
    }
  }
  float const max_deviation = closest / 3;

  //printf("Rigid Body tracker: limiting distance from nominal position "
  //  "to %f meters\n", max_deviation);

  bool allFitsGood = true;
  for (int iRb = 0; iRb < numRigidBodies; ++iRb) {
    RigidBody& rigidBody = m_rigidBodies[iRb];
    Cloud::Ptr &rbMarkers =
      m_markerConfigurations[rigidBody.m_markerConfigurationIdx];
    icp.setInputSource(rbMarkers);

    // find the points nearest to the rigidBodie's nominal position
    // (initial pos was loaded into lastTransformation from config file)
    size_t const rbNpts = rbMarkers->size();
    nearestIdx.resize(rbNpts);
    nearestSqrDist.resize(rbNpts);
    auto nominalCenter = eig2pcl(rigidBody.initialCenter());
    int nFound = kdtree.nearestKSearch(
      nominalCenter, rbNpts, nearestIdx, nearestSqrDist);

    if (nFound < rbNpts) {
      std::stringstream sstr;
      sstr << "error: only " << nFound
           << " neighbors found for rigid body " << rigidBody.name()
           << " (need " << rbNpts << ")";
      logWarn(sstr.str());
      allFitsGood = false;
      continue;
    }

    if (rbNpts == 1) {
      Eigen::Vector3f configinitialCenter = rigidBody.initialCenter();
      Eigen::Matrix4f configTransformation = pcl::getTransformation(
        configinitialCenter.x(), configinitialCenter.y(), configinitialCenter.z(),
        0, 0, 0).matrix();

      rigidBody.m_lastTransformation = configTransformation;
      std::cout << "rigidBody.m_lastTransformation:  \n"<< configTransformation << "\n";
      continue;
    }

    // only try to fit the rigid body if the k nearest neighbors
    // are reasonably close to the nominal rigid body position
    Eigen::Vector3f actualCenter(0, 0, 0);
    for (int i = 0; i < rbNpts; ++i) {
      actualCenter += pcl2eig((*markers)[nearestIdx[i]]);
    }
    actualCenter /= rbNpts;
    if ((actualCenter - pcl2eig(nominalCenter)).norm() > max_deviation) {
      std::stringstream sstr;
      sstr << "error: nearest neighbors of rigid body " << rigidBody.name()
           << " are centered at " << actualCenter
           << " instead of " << nominalCenter;
      logWarn(sstr.str());
      allFitsGood = false;
      continue;
    }

    // try ICP with guesses of many different yaws about knn centroid
    Cloud result;
    static int const N_YAW = 20;
    double bestErr = std::numeric_limits<double>::max();
    Eigen::Affine3f bestTransformation;
    for (int i = 0; i < N_YAW; ++i) {
      float yaw = i * (2 * M_PI / N_YAW);
      Eigen::Matrix4f tryMatrix = pcl::getTransformation(
        actualCenter.x(), actualCenter.y(), actualCenter.z(),
        0, 0, yaw).matrix();
      icp.align(result, tryMatrix);
      if (icp.hasConverged()) {
        double err = icp.getFitnessScore();
        if (err < bestErr) {
          bestErr = err;
          bestTransformation = icp.getFinalTransformation();
        }
      }
    }

    const DynamicsConfiguration& dynConf = m_dynamicsConfigurations[rigidBody.m_dynamicsConfigurationIdx];
    if (bestErr >= dynConf.maxFitnessScore) {
      std::stringstream sstr;
      sstr << "Initialize did not succeed (fitness too low) "
           << " for rigidBody " << rigidBody.name();
      logWarn(sstr.str());
      allFitsGood = false;
      continue;
    }

    // if the fit was good, this rigid body "takes" the markers, and they become
    // unavailable to all other rigidBodies so we don't double-assign markers
    // (TODO: this is so greedy... do we need a more global approach?)
    rigidBody.m_lastTransformation = bestTransformation;
    // remove highest indices first
    std::sort(rbTakePts.rbegin(), rbTakePts.rend());
    for (int idx : rbTakePts) {
      markers->erase(markers->begin() + idx);
    }
    // update search structures after deleting markers
    icp.setInputTarget(markers);
    kdtree.setInputCloud(markers);
  }

  ++m_init_attempts;
  return allFitsGood;
}

void RigidBodyTracker::updateHybrid(std::chrono::high_resolution_clock::time_point stamp,
  Cloud::ConstPtr markers)   
{
  std::cout << "updateHybrid function\n" ;
  std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();


  if (markers->empty()) {
    for (auto& rigidBody : m_rigidBodies) {
      rigidBody.m_lastTransformationValid = false;
    }
    return;
  }
  m_initialized = m_initialized || initializeHybrid(markers);
  if (!m_initialized) {
    logWarn(
      "rigid body tracker initialization failed - "
      "check that position is correct, all markers are visible, "
      "and marker configuration matches config file");
    // Doesn't make too much sense to continue here - lets wait to be fully initialized
    return;
  }

  ICP icp;
  icp.setMaximumIterations(5);  
  icp.setInputTarget(markers);   
  icp.setRANSACOutlierRejectionThreshold(0.05);
  // icp.setTransformationEpsilon(0.01);
	// icp.setEuclideanFitnessEpsilon(EuclideanEpsilon);
  // icp.setRANSACIterations(3);


  // prepare for knn query
  std::vector<int> nearestIdx(5); // tune maximum number of neighbors here
  std::vector<float> nearestSqrDist(nearestIdx.size());
  pcl::KdTreeFLANN<Point> kdtree;
  kdtree.setInputCloud(markers);

  CBS_Assignment<std::string, std::string> CBS_assignment;
  std::set<CBS_InputData> cbs_data_set;
  std::map<std::set<std::string>, Eigen::Affine3f> groupsMap_Affine;

  // for (auto& rigidBody : m_rigidBodies) {
  size_t const numRigidBodies = m_rigidBodies.size();
  for (int iRb = 0; iRb < numRigidBodies; ++iRb) {
    RigidBody& rigidBody = m_rigidBodies[iRb];
    std::cout << "Rigid Body Name: " << rigidBody.name() << "---------" << std::endl;

    Cloud::Ptr &rbMarkers = m_markerConfigurations[rigidBody.m_markerConfigurationIdx];

    size_t const rbNpts = rbMarkers->size();
    // std::cout <<"rigidBody.m_markerConfigurationIdx: " << rigidBody.m_markerConfigurationIdx<< " rbNpts: "<<rbNpts << std::endl;
    // std::cout << "rigidBody.m_lastValidTransform: " << (rigidBody.m_lastValidTransform.time_since_epoch().count()) << std::endl;
    // std::cout << "rigidBody.m_lastTransformation.matrix():\n"<< rigidBody.m_lastTransformation.matrix() << "\n"; 

    std::chrono::duration<double> elapsedSeconds = stamp-rigidBody.m_lastValidTransform;
    double dt = elapsedSeconds.count();
    // Set the max correspondence distance
    const DynamicsConfiguration& dynConf = m_dynamicsConfigurations[rigidBody.m_dynamicsConfigurationIdx];

    if (rbNpts == 1) {
      std::cout << "single marker drone---"<< std::endl;
      if (dt > 0.5) {
        std::stringstream sstr;
        sstr << "Lost tracking for rigidBody " << rigidBody.name()<< "dt"<< dt << " skipping";
        logWarn(sstr.str());
        continue;
      }

      auto nominalCenter = eig2pcl(rigidBody.center());
      int nFound = kdtree.nearestKSearch(
        nominalCenter, nearestIdx.size(), nearestIdx, nearestSqrDist);
      if (nFound < 1) {
        std::stringstream sstr;
        sstr << "error: no neighbors found for rigidBody " << rigidBody.name();
        logWarn(sstr.str());
        continue;
      }


      Eigen::Vector3f offset = pcl2eig((*m_markerConfigurations[rigidBody.m_markerConfigurationIdx])[0]);
      bool foundPotentialMarker = false;
      for (int iMarker = 0; iMarker < nFound; ++iMarker) {   // loop all the near markers
        Eigen::Vector3f marker = pcl2eig((*markers)[nearestIdx[iMarker]]);

        // Compute changes:
        Eigen::Vector3f velocity = (marker - rigidBody.center() + offset) / dt;
        float vx = velocity.x();
        float vy = velocity.y();
        float vz = velocity.z();
        // std::cout << "marker: " << marker.transpose() << ", rigidBody.center(): " << rigidBody.center().transpose() << std::endl;
        // std::cout << "Velocity: (" << vx << ", " << vy << ", " << vz << ")"<< std::endl;
        // std::cout << "maxXVelocity: " << dynConf.maxXVelocity << std::endl;
        // std::cout << "maxYVelocity: " << dynConf.maxYVelocity << std::endl;
        // std::cout << "maxZVelocity: " << dynConf.maxZVelocity << std::endl;

        if (   fabs(vx) < dynConf.maxXVelocity
            && fabs(vy) < dynConf.maxYVelocity
            && fabs(vz) < dynConf.maxZVelocity)
        {
          float dist = (marker - rigidBody.center() + offset).norm();
          long cost = dist* 10e3;
          // assignment.setCost(iRb, nearestIdx[iMarker], cost);
          CBS_InputData data;
          data.taskSet.insert(std::to_string(nearestIdx[iMarker]));
          data.agent = std::to_string(iRb);
          data.cost = cost;
          cbs_data_set.insert(data);
          foundPotentialMarker = true;
        }
      }
      if (!foundPotentialMarker) {
        std::stringstream sstr;
        sstr << "all dynamic check failed for rigidBody " << rigidBody.name() << std::endl;
        logWarn(sstr.str());
      }
      continue;
    }

    // std::cout << "multi marker drone---"<< std::endl;

    float maxV = dynConf.maxXVelocity;
    icp.setMaxCorrespondenceDistance(maxV * dt);   
    // std::cout << "icp.setMaxCorrespondenceDistance(maxV * dt);  maxV * dt:" << maxV * dt << std::endl;

    // Update input source
    icp.setInputSource(m_markerConfigurations[rigidBody.m_markerConfigurationIdx]);   // move configure to frame point cloud 
    
    // Perform the alignment for k times
    int k= 3; // max_group_num
    auto predictTransform = rigidBody.m_lastTransformation;  


    // double noiseStdDev = 0.0001; // TODO test different para 0.01 0.001 0.1
    // std::default_random_engine generator;
    // std::normal_distribution<double> distribution(0, noiseStdDev);



    double translationNoiseStdDev = 0.01; 
    double rotationNoiseStdDev = 0.01; 

    // Create a random number generator
    std::default_random_engine generator;
    std::normal_distribution<double> translationDistribution(0, translationNoiseStdDev);
    std::normal_distribution<double> rotationDistribution(0, rotationNoiseStdDev);


    std::cout << "predictTransform/m_lastTransformation:  \n"<< predictTransform.matrix()<< "\n";
    static Eigen::Matrix4f lastTransformation; 

    // std::cout << "-----try k times icp to get different options:----  \n";   
    for (size_t i = 0; i < k; ++i)  {
      std::cout << "icp: " << i<< std::endl;
      Cloud result; 


      // Extract translation and Euler angles
      float p_x, p_y, p_z, p_roll, p_pitch, p_yaw;
      pcl::getTranslationAndEulerAngles(predictTransform, p_x, p_y, p_z, p_roll, p_pitch, p_yaw);

      // Add noise to translation and Euler angles
      p_x += translationDistribution(generator);
      p_y += translationDistribution(generator);
      p_z += translationDistribution(generator);
      p_roll += rotationDistribution(generator);
      p_pitch += rotationDistribution(generator);
      p_yaw += rotationDistribution(generator);

      // Reconstruct the transformation matrix with noise
      Eigen::Affine3f noisyTransform = pcl::getTransformation(p_x, p_y, p_z, p_roll, p_pitch, p_yaw).cast<float>();

      // // Debug output to check the noisy transformation matrix
      // std::cout << "Noisy predictTransform/m_lastTransformation:  \n" << noisyTransform.matrix() << "\n";

      // icp.align(result, predictTransform.matrix());  // in the result there are moved config markers
      icp.align(result, noisyTransform.matrix());

      if (!icp.hasConverged()) {
        std::stringstream sstr;
        sstr << "ICP did not converge!"
            << " for rigidBody " << rigidBody.name();
        logWarn(sstr.str());
        continue;
      }

      Eigen::Matrix4f transformation = icp.getFinalTransformation();
      // std::cout << "transformation:" << transformation << std::endl;
      if (i > 0 && !transformation.isApprox(lastTransformation)) {
          std::cout << "Transformation changed:" << std::endl;
          std::cout << "last" << lastTransformation << std::endl;
          std::cout << "current" << transformation << std::endl;
      }
      lastTransformation = transformation;  // Update the last transformation

      Eigen::Affine3f tROTA(transformation);

      float x, y, z, roll, pitch, yaw;
      pcl::getTranslationAndEulerAngles(tROTA, x, y, z, roll, pitch, yaw);
      float last_x, last_y, last_z, last_roll, last_pitch, last_yaw;
      pcl::getTranslationAndEulerAngles(rigidBody.m_lastTransformation, last_x, last_y, last_z, last_roll, last_pitch, last_yaw);

      float vx = (x - last_x) / dt;
      float vy = (y - last_y) / dt;
      float vz = (z - last_z) / dt;
      float wroll = deltaAngle(roll, last_roll) / dt;
      float wpitch = deltaAngle(pitch, last_pitch) / dt;
      float wyaw = deltaAngle(yaw, last_yaw) / dt;

      // ROS_INFO("v: %f,%f,%f, w: %f,%f,%f, dt: %f", vx, vy, vz, wroll, wpitch, wyaw, dt);

      if (   fabs(vx) < dynConf.maxXVelocity
          && fabs(vy) < dynConf.maxYVelocity
          && fabs(vz) < dynConf.maxZVelocity
          && fabs(wroll) < dynConf.maxRollRate
          && fabs(wpitch) < dynConf.maxPitchRate
          && fabs(wyaw) < dynConf.maxYawRate
          && fabs(roll) < dynConf.maxRoll
          && fabs(pitch) < dynConf.maxPitch
          && icp.getFitnessScore() < dynConf.maxFitnessScore)
      {
        CBS_InputData data;
        // Get the correspondence indices
        std::vector<size_t> correspondences;
        pcl::search::KdTree<Point>::Ptr search_tree = icp.getSearchMethodTarget();
        for (auto point : result.points) {
          std::vector<int> matched_indices;
          std::vector<float> matched_distances;
          search_tree->nearestKSearch(point, 1, matched_indices, matched_distances);
          correspondences.push_back(matched_indices[0]);
          data.taskSet.insert(std::to_string(matched_indices[0]));
        }

        // // Print result points
        // std::cout << "Result Points/ moved config markers :" << std::endl;
        // for (auto point : result.points) {
        //     std::cout << "Point: " << point << std::endl;
        // }
        // // Print correspondences
        // std::cout << "Correspondences/ markers that are found for current drones:" << std::endl;
        // for (size_t idx : correspondences) {
        //     std::cout <<"Correspond idx: "<< idx << " Point: " << (*markers)[idx] << std::endl;
        // }
        
        // Compute cost   
        float dist = sqrt(pow(x - last_x, 2) + pow(y - last_y, 2) + pow(z - last_z, 2));
        long cost = dist* 10e3;

        data.agent = std::to_string(iRb);
        data.cost = cost;
        cbs_data_set.insert(data);

        // std::set<std::string> keySet = data.taskSet;
        // keySet.insert(std::to_string(data.cost));
        groupsMap_Affine[data.taskSet] = tROTA;  // TODO maybe sth wrong


      } else {
        std::stringstream sstr;
        sstr << "Dynamic check failed for rigidBody " << rigidBody.name() << std::endl;
        if (fabs(vx) >= dynConf.maxXVelocity) {
          sstr << "vx: " << vx << " >= " << dynConf.maxXVelocity << std::endl;
        }
        if (fabs(vy) >= dynConf.maxYVelocity) {
          sstr << "vy: " << vy << " >= " << dynConf.maxYVelocity << std::endl;
        }
        if (fabs(vz) >= dynConf.maxZVelocity) {
          sstr << "vz: " << vz << " >= " << dynConf.maxZVelocity << std::endl;
        }
        if (fabs(wroll) >= dynConf.maxRollRate) {
          sstr << "wroll: " << wroll << " >= " << dynConf.maxRollRate << std::endl;
        }
        if (fabs(wpitch) >= dynConf.maxPitchRate) {
          sstr << "wpitch: " << wpitch << " >= " << dynConf.maxPitchRate << std::endl;
        }
        if (fabs(wyaw) >= dynConf.maxYawRate) {
          sstr << "wyaw: " << wyaw << " >= " << dynConf.maxYawRate << std::endl;
        }
        if (fabs(roll) >= dynConf.maxRoll) {
          sstr << "roll: " << roll << " >= " << dynConf.maxRoll << std::endl;
        }
        if (fabs(pitch) >= dynConf.maxPitch) {
          sstr << "pitch: " << pitch << " >= " << dynConf.maxPitch << std::endl;
        }
        if (icp.getFitnessScore() >= dynConf.maxFitnessScore) {
          sstr << "fitness: " << icp.getFitnessScore() << " >= " << dynConf.maxFitnessScore << std::endl;
        }
        logWarn(sstr.str());
      }
    }
  }

  // std::cout << "cbs_data_set:" << std::endl;
  for (const auto& data : cbs_data_set) {
    // std::cout <<  data;
    CBS_assignment.setCost(data.agent, data.taskSet, data.cost);
  }

  std::map<std::string, std::set<std::string>> solution;
  int64_t CBS_assignment_cost = CBS_assignment.solve(solution);
  // std::cout << "CBS_assignment_cost: " << CBS_assignment_cost << std::endl;
  HighLevelNode start;
  start.id = 0;
  start.cost = CBS_assignment_cost;
  start.solution = solution;
  std::cout << "The start HLN: ";
  std::cout << start;
  typename boost::heap::d_ary_heap<HighLevelNode, boost::heap::arity<2>,
                                    boost::heap::mutable_<true> >
      open;

  auto handle = open.push(start);
  (*handle).handle = handle;

  bool outputToFile = false; 
  solution.clear();
  int id = 1;
  HighLevelNode P;
  int m_highLevelExpanded = 0; 
  int m_lowLevelExpanded = 0;
  int duplicate = 0;
  while (!open.empty()) {
    m_highLevelExpanded++;
    // std::cout << "=========" << m_highLevelExpanded<< " Loop ==========="  << std::endl;
    P = open.top();
    open.pop();
    // std::cout << P;
    if (m_highLevelExpanded!=1){
      std::cout << "m_highLevelExpanded: " << m_highLevelExpanded << std::endl; // it always here
      std::cout << "start node:"<< start;
      std::cout << P;
      // exit(0);
    }


    if (P.solution.empty()) {
      std::cout << "Cannot find a solution!" << std::endl;
    }

    std::string conflict_task;
    if (!getFirstConflict(P.solution,conflict_task)) {
      // std::cout << "no conflict_task, Breaking out of the loop.\n";
      outputToFile = true; 
      break;
    }
    std::set<std::set<Constraint>> new_constraints;
    createConstraintsFromConflict(P.solution,conflict_task,new_constraints);
    for (const auto& new_constraint_set : new_constraints) {
      HighLevelNode newNode;
      LowLevelSearch(new_constraint_set,cbs_data_set,P,newNode,id);
      auto handle = open.push(newNode);
      (*handle).handle = handle;
    }
  }

  for (const auto& s : P.solution) {
    auto& rigidBody = m_rigidBodies[std::stoi(s.first)];  // TODO change the type of s.first
    std::set<std::string> current_set = s.second;
    // std::cout << "Marker Configuration Index: " << rigidBody.m_markerConfigurationIdx << std::endl; // done print
    // std::cout << "Marker Configuration Value: " << *m_markerConfigurations[rigidBody.m_markerConfigurationIdx] << std::endl;
    std::chrono::duration<double> elapsedSeconds = stamp-rigidBody.m_lastValidTransform;
    double dt = elapsedSeconds.count();

    if (current_set.size() == 1) {
        // std::cout << "Only one element in the set: " << *current_set.begin() << std::endl;
        int markerIndex = std::stoi(*current_set.begin());
        Eigen::Vector3f marker = pcl2eig((*markers)[markerIndex]);
        Eigen::Vector3f offset = pcl2eig((*m_markerConfigurations[rigidBody.m_markerConfigurationIdx])[0]);  // problem is here h

        rigidBody.m_velocity = (marker - rigidBody.center() + offset) / dt;
        rigidBody.m_lastTransformation = Eigen::Translation3f(marker + offset);
        rigidBody.m_lastValidTransform = stamp;
        rigidBody.m_lastTransformationValid = true;
        rigidBody.m_hasOrientation = false;
    }
    else{ // TODO groupsMap_Affine maybe not a good match 
      // std::cout << "More than one element in the set." << std::endl;
      rigidBody.m_lastTransformation = groupsMap_Affine[s.second];
      rigidBody.m_velocity = (rigidBody.m_lastTransformation.translation() - rigidBody.center()) / dt;
      rigidBody.m_lastValidTransform = stamp;
      rigidBody.m_lastTransformationValid = true;
      rigidBody.m_hasOrientation = false;
    }
  }


  std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();  
  std::chrono::duration<double> time_used = std::chrono::duration_cast<std::chrono::duration<double>>( t2-t1 );
  

  std::string inputfileName = inputPath.substr(inputPath.find_last_of("/\\") + 1);
  std::string outputDir = "./data/output/";
  auto now = std::chrono::system_clock::now();
  auto epoch = now.time_since_epoch();
  auto minutes = std::chrono::duration_cast<std::chrono::minutes>(epoch).count();
  // std::cout << "Minutes: " << minutes << std::endl;
  std::string outputFile = outputDir + inputfileName+"_"+ std::to_string(minutes);  // + inputFile
  outputFile = outputFile + ".txt";
  

  // std::cout << "Input File: " << inputfileName << std::endl;
  std::cout << "Output file: " << outputFile << std::endl;
  // std::cout << "runtime: " << time_used.count() << " seconds" << std::endl;
  // std::cout << "highLevelExpanded: " << m_highLevelExpanded << std::endl;
  // std::cout << "duplicate: " << duplicate << std::endl;


  std::ofstream out(outputFile, std::ios_base::app); // Open in append mode

  if (!out.is_open()) {
    std::cout << "File does not exist, creating a new file..." << std::endl;
    out.open(outputFile);
  }
  // out << "Input File: " << inputFile << std::endl;
  // out << "elapsedSeconds: " << elapsedSeconds << std::endl;  std::cout << "stamp: " << stamp.time_since_epoch().count() << std::endl;
  // std::cout << "stamp: " << stamp.time_since_epoch().count() << std::endl;
  out << "stamp: " << stamp.time_since_epoch().count() << std::endl;
  out << "Runtime: " << time_used.count() << " seconds" << std::endl;
  out << P;
  
  out << "transformation:"<< std::endl;
  for (int iRb = 0; iRb < numRigidBodies; ++iRb) {
    RigidBody& rigidBody = m_rigidBodies[iRb];
    Eigen::Quaternionf q(rigidBody.m_lastTransformation.rotation());
    // rigidBody.m_lastTransformation.translation()
    // Eigen::Vector3d rigidBodyTranslation = rigidBody.m_lastTransformation.translation();
    // std::cout << "Translation: " << rigidBodyTranslation.transpose() << std::endl;

    // std::cout << "Quaternion: " << q.coeffs().transpose() << std::endl;
    // std::cout << "Translation: " << rigidBodyTranslation.transpose() 
    out << iRb<< ": "  <<rigidBody.m_lastTransformation.translation().x()
    << " " <<rigidBody.m_lastTransformation.translation().y()
    << " " <<rigidBody.m_lastTransformation.translation().z()
    << " " <<q.x()
    << " " <<q.y()
    << " " <<q.z()
    << " " <<q.w()
    <<std::endl;


  }

}

void RigidBodyTracker::logWarn(const std::string& msg)
{
  if (m_logWarn) {
    m_logWarn(msg);
  }
}

} // namespace librigidbodytracker
