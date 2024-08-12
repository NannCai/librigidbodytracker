#pragma once

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include "librigidbodytracker/rigid_body_tracker.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <type_traits>
#include <random>


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

	class PointCloudPlayer
	{
	public:
		void load(std::string path)
		{
			std::ifstream s(path, std::ios::binary | std::ios::in);
			if (!s) {
				throw std::runtime_error("PointCloudPlayer: bad file path.");
			}
			inputPath = path;
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

		void play(librigidbodytracker::RigidBodyTracker &tracker,double pick_probability = 0,std::string outputPath = "") const
		{
			std::string inputfileName = inputPath.substr(inputPath.find_last_of("/\\") + 1);
			// std::string outputDir = "./data/output/";
			// auto now = std::chrono::system_clock::now();
			// auto epoch = now.time_since_epoch();
			// auto minutes = std::chrono::duration_cast<std::chrono::minutes>(epoch).count();
			size_t found = outputPath.find_last_of("/\\");
			std::string outputDir = outputPath.substr(0, found);

			std::string outputFile = outputPath + "_pointcloud";  
			outputFile = outputFile + ".txt";
			std::ofstream out(outputFile, std::ios::trunc); 
			if (!out.is_open()) {
				std::cout << "File does not exist, creating a new file... play funcion" << std::endl;
				out.open(outputFile);
			}

		    // const double pick_probability = 0.001;
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<> dis(0.0, 1.0);
			std::poisson_distribution<int> dis_poisson(8); 

			int total_removed_points = 0; 
			out << "total_removed_points: " << total_removed_points << "     "<<std::endl;
			out << "pick_probability: " << pick_probability << "      " <<std::endl;

			for (size_t i = 0; i < clouds.size(); ++i) {
				std::cout << i << " frame  ---------------------------------------------------"<< std::endl;
				auto dur = std::chrono::milliseconds(timestamps[i]);
				std::chrono::high_resolution_clock::time_point stamp(dur);
				if (clouds[i]->empty()) {
					continue;
				}


				// noise: remove point
				if (dis(gen) < pick_probability) {
					// std::cout << i << " frame, remove "<< std::endl;
					// int pc_num = 10; 
					int pc_num = clouds[i]->size(); 
					// int lambda = pc_num/3;
					// std::uniform_int_distribution<> dis_uni_int(1, pc_num);
				    // std::poisson_distribution<int> dis_poisson(lambda); // Poisson distribution with lambda = 5
				    

					int num_point = dis_poisson(gen);
					std::cout << "Number of removed point: " << num_point << std::endl;

					if (num_point > pc_num) {
						num_point = pc_num - 1;
					}

					std::set<int> indices_set;
					while (indices_set.size() < num_point) {
						indices_set.insert(rand() % pc_num);
					}

					// Convert set to vector and sort in descending order to avoid invalidation issues
					std::vector<int> indices(indices_set.begin(), indices_set.end());
					std::sort(indices.rbegin(), indices.rend());

					// Erase the points with the generated indices
					for (int idx : indices) {
						std::cout << "Removing point with id: " << idx << std::endl;
						clouds[i]->erase(clouds[i]->begin() + idx);
					}

					total_removed_points += num_point; 

				}


				// // noise: add points	
				// if (dis(gen) < pick_probability) {
				// 	int pc_num = clouds[i]->size();
				// 	int num_point = dis_poisson(gen);
				// 	std::cout << "Number of added points: " << num_point << std::endl;

				// 	for (int j = 0; j < num_point; ++j) {
				// 		Point new_point;
				// 		new_point.x = (std::uniform_real_distribution<>(-2.5, 2.5))(gen);
				// 		new_point.y = (std::uniform_real_distribution<>(-5.0, 5.0))(gen);
				// 		new_point.z = (std::uniform_real_distribution<>(0.0, 3.0))(gen);

				// 		std::cout << "Adding point: (" << new_point.x << ", " << new_point.y << ", " << new_point.z << ")" << std::endl;
				// 		clouds[i]->push_back(new_point);
						 
				// 	}
				// 	total_removed_points -= num_point;
				// }


				out << "stamp: " << stamp.time_since_epoch().count() << std::endl;
				const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud = clouds[i];
				for (size_t i = 0; i < cloud->size(); ++i) {
					const pcl::PointXYZ& point = (*cloud)[i]; 
					out << point.x << ", " << point.y << ", " << point.z << std::endl;
				}
				tracker.update(stamp, clouds[i], inputPath,outputPath);

				// tracker.update(stamp, clouds[i]);
			}


			// Write the total number of removed points to the end of the output file
			// std::ofstream out(outputFile, std::ios_base::app);
			out.seekp(0);
			out << "total_removed_points: " << total_removed_points;

			std::ofstream noiseInfo(outputDir + "/noise_info.txt", std::ios::app);
			std::cout << "noise_infoFile: " << outputDir + "/noise_info.txt" <<std::endl;
			if (!noiseInfo.is_open()) {
				std::cout << "Creating a new noise_info.txt file..." << std::endl;
				noiseInfo.open(outputDir + "/noise_info.txt");
			}
			noiseInfo << outputPath + ":"<< std::endl;
			noiseInfo << "total_removed_points: " << total_removed_points << std::endl;
			noiseInfo << "pick_probability: " << pick_probability <<std::endl;

			std::cout << "Total clouds size: " << clouds.size() << std::endl;
			std::cout << "Total number of removed points: " << total_removed_points << std::endl; 
			std::cout << "outputFile: " << outputFile <<std::endl;
		}


	private:
		std::string inputPath;
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
						auto p = rigidBody.transformation() * pcl2eig((*rbMarkers)[j]); //get real position
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
