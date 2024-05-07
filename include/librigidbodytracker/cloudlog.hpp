#pragma once

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include "librigidbodytracker/rigid_body_tracker.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <type_traits>

#include <pcl/common/transforms.h>
#include <pcl/registration/icp.h>
#include <pcl/registration/transformation_estimation_2D.h>

// point cloud log format:
// infinite repetitions of:
// timestamp (milliseconds) : uint32
// cloud size               : uint32
// [x y z, x y z, ... ]     : float32

#define markermax 60*4 

using Point = pcl::PointXYZ;
using Cloud = pcl::PointCloud<Point>;
using ICP = pcl::IterativeClosestPoint<Point, Point>;

namespace librigidbodytracker {

	class PointCloudLogger
	{
	public:
		PointCloudLogger(std::string file_path) : file(file_path, std::ios::binary | std::ios::out)
		{
			if (file.is_open())
			{
				std::cout << "File is open------------" << std::endl;
				std::cout << "File path: " << file_path << std::endl;
			}
			else
			{
				std::cout << "File is not open" << std::endl;
			}

		}

		void log(pcl::PointCloud<pcl::PointXYZ>::ConstPtr cloud)
		{
			auto stamp = std::chrono::high_resolution_clock::now();
			if (start == (decltype(start)())) {
				start = stamp;
			}
			auto millis = std::chrono::duration_cast<std::chrono::milliseconds>
				(stamp - start).count();
			log(millis, cloud);
		}

		void log(uint32_t millis, pcl::PointCloud<pcl::PointXYZ>::ConstPtr cloud)
		{
			write<uint32_t>(file, millis);
			// std::cout << "Current time in milliseconds: " << millis << std::endl;  // this work

			write<uint32_t>(file, cloud->size());
			for (pcl::PointXYZ const &p : *cloud) {
				static_assert(std::is_same<decltype(p.x), float>::value, "expected float");
				write(file, p.x);
				write(file, p.y);
				write(file, p.z);
			}
		}

		void flush()
		{
			file.flush();
			// std::cout << "finish flush" << std::endl;

		}

	protected:
		template <typename T>
		void write(std::ofstream &s, T const &t)
		{
			s.write((char const *)&t, sizeof(T));
		}
		std::ofstream file;
		std::chrono::high_resolution_clock::time_point start;
	};

// PointCloudPlayer is a class that have loading and playing back point cloud data function: load, play.
// load: The loaded point cloud data is stored in the timestamps and clouds member variables.
// play: takes a RigidBodyTracker object as input and iterates over the loaded point cloud data, updating the tracker with each point cloud at the corresponding timestamp.
	class PointCloudPlayer
	{
	public:
		void load(std::string path)
		{
			std::ifstream s(path, std::ios::binary | std::ios::in);
			if (!s) {
				throw std::runtime_error("PointCloudPlayer: bad file path.");
			}
			while (s) {
				uint32_t millis = read<uint32_t>(s);
				// TODO cleaner loop?
				if (!s) {
					break;
				}
				timestamps.push_back(millis);

				uint32_t size = read<uint32_t>(s);
				clouds.emplace_back(new pcl::PointCloud<pcl::PointXYZ>());
				clouds.back()->resize(size);
				for (uint32_t i = 0; i < size; ++i) {
					float x = read<float>(s);
					float y = read<float>(s);
					float z = read<float>(s);
					(*clouds.back())[i] = pcl::PointXYZ(x, y, z);
				}
			}
		}

		void play(librigidbodytracker::RigidBodyTracker &tracker) const
		{
			// for (size_t i = 0; i < clouds.size(); ++i) {
			for (size_t i = 0; i < 500; ++i) {
			// for (size_t i = 1000; i < 1700; ++i) {
				std::cout << i << " frame  ---------------------------------------------------"<< std::endl;
				auto dur = std::chrono::milliseconds(timestamps[i]);
				std::chrono::high_resolution_clock::time_point stamp(dur);
				// // Delete the last point in clouds[i]
				// if (!clouds[i]->empty()) {
				// 	// clouds[i]->pop_back();        
				// 	clouds[i]->resize(clouds[i]->size() - 1);
				// }
				// if (clouds[i]->size() >= 7) {
				// 	clouds[i]->erase(clouds[i]->begin(), clouds[i]->begin() + 3);
				// }		
					
				// // # new debugg print   the points in the current point cloud
				// // Ptr& cloud refers to the same object as clouds[i] but it's a reference, so it won't make a copy.
				// const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud = clouds[i];
				// for (size_t i = 0; i < cloud->size(); ++i) {
				// 	const pcl::PointXYZ& point = (*cloud)[i];  // !!! here reference and pointer need to be figure out
				// 	std::cout << "Point " << i << ": (" << point.x << ", " << point.y << ", " << point.z << ")\n";
				// }


				tracker.update(stamp, clouds[i]);
			}
		}

		void play_compute_offset(librigidbodytracker::RigidBodyTracker &tracker) const
		{
			pcl::PointXYZ totalCenter = {0.0, 0.0, 0.0};
			size_t validCloudsCount = 0;
			// for (size_t i = 0; i < clouds.size(); ++i) {
			for (size_t i = 0; i < 10; ++i) {
				std::cout << "\n  " << i << "  ------------------------------\n";
				auto dur = std::chrono::milliseconds(timestamps[i]);
				std::chrono::high_resolution_clock::time_point stamp(dur);
				// tracker.update(stamp, clouds[i]);

				const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud = clouds[i];
				// if (clouds[i]->size() >= 7) {
				// 	clouds[i]->erase(clouds[i]->begin(), clouds[i]->begin() + 3);
				// }		

				if (cloud->size() >= 7) {
					auto it = cloud->begin();
					while (it != cloud->end()) {
						if (it->z < 0.03) {
							it = cloud->erase(it);
						} else {
							++it;
						}
					}
				}
				for (size_t i = 0; i < cloud->size(); ++i) {
					const pcl::PointXYZ& point = (*cloud)[i];  // !!! here reference and pointer need to be figure out
					std::cout << "Point " << i << ": (" << point.x << ", " << point.y << ", " << point.z << ")\n";
				}


				if (cloud->size() < 3) {
					std::cout << "Not enough points. Skipping calculation.\n";
					continue;
				}

				pcl::PointXYZ center;
				center.x = ((*cloud)[0].x + (*cloud)[1].x + (*cloud)[2].x + (*cloud)[3].x) / 4.0;
				center.y = ((*cloud)[0].y + (*cloud)[1].y + (*cloud)[2].y + (*cloud)[3].y) / 4.0;
				center.z = ((*cloud)[0].z + (*cloud)[1].z + (*cloud)[2].z + (*cloud)[3].z) / 4.0;

				totalCenter.x += center.x;
				totalCenter.y += center.y;
				totalCenter.z += center.z;
				validCloudsCount++;
				std::cout << "Center of the first three points: (" << center.x << ", " << center.y << ", " << center.z << ")\n";

			}

			// Average Center: (0.0128561, 0.00107494, 0.0371899)
			if (validCloudsCount > 0) {
				totalCenter.x /= validCloudsCount;
				totalCenter.y /= validCloudsCount;
				totalCenter.z /= validCloudsCount;
				std::cout << "Average Center: (" << totalCenter.x << ", " << totalCenter.y << ", " << totalCenter.z << ")\n";
			} else {
				std::cout << "No valid point clouds found.\n";
			}
		}


		void play_compute_markerposition(size_t numPoints)
		{
			std::cout << "play_compute_markerposition" << std::endl;

			std::vector<pcl::PointXYZ> totalPoints(numPoints, {0.0, 0.0, 0.0});
			size_t validCloudsCount = 0;

			// for (size_t i = 0; i < clouds.size(); ++i) {
			for (size_t i = 0; i < 10; ++i) {
				std::cout << "\n  " << i << "  ------------------------------\n";
				auto dur = std::chrono::milliseconds(timestamps[i]);
				std::chrono::high_resolution_clock::time_point stamp(dur);

				const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud = clouds[i];

				if (cloud->size() < numPoints) {
					std::cout << "Not enough points. Skipping calculation.\n";
					continue;
				}

				for (size_t j = 0; j < numPoints; ++j) {
					const pcl::PointXYZ& point = (*cloud)[j];
					totalPoints[j].x += point.x;
					totalPoints[j].y += point.y;
					totalPoints[j].z += point.z;
				}

				validCloudsCount++;
			}

			if (validCloudsCount > 0) {
				for (size_t j = 0; j < numPoints; ++j) {
					totalPoints[j].x /= validCloudsCount;
					totalPoints[j].y /= validCloudsCount;
					totalPoints[j].z /= validCloudsCount;

					std::cout << "Average Point " << j << ": (" << totalPoints[j].x << ", " << totalPoints[j].y << ", " << totalPoints[j].z << ")\n";
				}
			} else {
				std::cout << "No valid point clouds found.\n";
			}
		}



	protected:
		template <typename T>
		T read(std::ifstream &s)
		{
			T t;
			s.read((char *)&t, sizeof(t));
			return t;
		}
		std::vector<uint32_t> timestamps;
		std::vector<pcl::PointCloud<pcl::PointXYZ>::Ptr> clouds;
	};

	class PointCloudDebugger : public PointCloudPlayer
	{

	public:
		PointCloudDebugger(std::string file_path) : writepath(file_path) {}

		void convert(librigidbodytracker::RigidBodyTracker & tracker, std::vector<MarkerConfiguration> & config)
		{
			auto eig2pcl = [](Eigen::Vector3f v) {
			  return pcl::PointXYZ(v.x(), v.y(), v.z());
			};
			auto pcl2eig =[](Point p) {
			  return Eigen::Vector3f(p.x, p.y, p.z);
			};

			//play points
			for (size_t i = 0; i < clouds.size(); ++i) {
				std::cout << "\n  " << i << "  ------------------------------\n";
				auto dur = std::chrono::milliseconds(timestamps[i]);
				std::chrono::high_resolution_clock::time_point stamp(dur);
				tracker.update(stamp, clouds[i]);
				//continue;
				//make another output cloud
				matches.emplace_back(new pcl::PointCloud<pcl::PointXYZ>());
				matches.back()->reserve(markermax);
				//read updated rigidBodies
				const std::vector<RigidBody> & rigidBodies = tracker.rigidBodies();
				int a = 0;
				for (auto & rigidBody : rigidBodies) {
					++a;
					std::cout << "RigidBody vector size: " << rigidBodies.size() << "\n";
					std::cout << "RigidBody " << a << " processing\n";
					//debugging stuff
					Cloud::Ptr &rbMarkers = config[rigidBody.m_markerConfigurationIdx];
					size_t const rbNpts = rbMarkers->size();
					for (size_t j = 0; j < rbNpts; ++j) { //for each marker
						auto p = rigidBody.transformation() * pcl2eig((*rbMarkers)[j]); //get the pose of the robot
						matches.back()->push_back(eig2pcl(p));
					}
				}
			}
			std::cout << "Writing converted file\n";
			std::ofstream s(writepath, std::ios::binary | std::ios::out);
			for (size_t i = 0; i < matches.size(); ++i) {
				write(s, timestamps[i]);
				write(s, (uint32_t)matches[i]->size());
				for (pcl::PointXYZ const &p : *(matches[i])) {
					static_assert(std::is_same<decltype(p.x), float>::value, "expected float");
					write(s, p.x);
					write(s, p.y);
					write(s, p.z);
				}
			}
		}

	private:
		template <typename T>
		void write(std::ofstream &s, T const &t)
		{
			s.write((char const *)&t, sizeof(T));
		}
		std::string writepath;
		std::string path;
		std::vector<pcl::PointCloud<pcl::PointXYZ>::Ptr> matches;
	};

} //namespace librigidbodytracker
