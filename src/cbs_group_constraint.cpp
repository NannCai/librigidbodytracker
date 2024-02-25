#include <fstream>
#include <iostream>
#include <regex>

#include <boost/program_options.hpp>
#include <boost/heap/d_ary_heap.hpp>

// #include "assignment.hpp"
// #include "cbs_assignment.hpp"
#include "assignment.hpp"

using libMultiRobotPlanning::Assignment;

struct Constraints{
  friend std::ostream& operator<<(std::ostream& os, const Constraints& c) {
    // for (const auto& vc : c.vertexConstraints) {
    //   os << vc << std::endl;
    // }
    // for (const auto& ec : c.edgeConstraints) {
    //   os << ec << std::endl;
    // }
    return os;
  }
};

struct HighLevelNode {
  // std::vector<PlanResult<State, Action, Cost> > solution;
  std::map<std::string, std::string> solution;
  // constraints: [(a1,t1,t2),(a5,t3,t9)] 
  std::vector<Constraints> constraints;
  // std::vector<Constraints> constraints;

  // Cost cost;
  long cost;

  int id;

  typename boost::heap::d_ary_heap<HighLevelNode, boost::heap::arity<2>,
                                    boost::heap::mutable_<true> >::handle_type
      handle;

  bool operator<(const HighLevelNode& n) const {
    // if (cost != n.cost)
    return cost > n.cost;
    // return id > n.id;
  }

  friend std::ostream& operator<<(std::ostream& os, const HighLevelNode& c) {
    os << "id: " << c.id << " cost: " << c.cost << std::endl;
    for (size_t i = 0; i < c.solution.size(); ++i) {
      os << "Agent: " << i << std::endl;
      // os << " States:" << std::endl;
      // for (size_t t = 0; t < c.solution[i].states.size(); ++t) {
      //   os << "  " << c.solution[i].states[t].first << std::endl;
      // }
      std::cout << "solution: " << std::endl;
      for (const auto& s : c.solution) {
      std::cout << s.first << ": " << s.second << std::endl;
      }
      os << " Constraints:" << std::endl;
      os << c.constraints[i];
      // os << " cost: " << c.solution[i].cost << std::endl;
    }
    return os;
  }
};


int main(int argc, char* argv[]) {
  // !! initialise input
  namespace po = boost::program_options;
  // Declare the supported options.
  po::options_description desc("Allowed options");
  std::string inputFile;
  std::string outputFile;
  desc.add_options()("help", "produce help message")(
      "input,i", po::value<std::string>(&inputFile)->required(),
      "input cost (txt)")
      ("output,o",po::value<std::string>(&outputFile)->required(),
      "output file (YAML)");

  try {
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") != 0u) {
      std::cout << desc << "\n";
      return 0;
    }
  } catch (po::error& e) {
    std::cerr << e.what() << std::endl << std::endl;
    std::cerr << desc << std::endl;
    return 1;
  }

  struct InputData {
      std::string agent;
      long cost;
      std::set<std::string> taskSet;
  };

  std::vector<InputData> inputData;

  std::ifstream input(inputFile);
  for (std::string line; getline(input, line);) {
      std::stringstream stream(line);

      InputData data;
      stream >> data.agent;
      stream >> data.cost;
      std::string task;
      bool skipLine = false;
      while (stream >> task) {
          if (data.taskSet.find(task) != data.taskSet.end()) {
              // Task is already in the set, skip this line
              skipLine = true;
              break;
          }
          data.taskSet.insert(task);
      }

      if (!skipLine) {
          inputData.push_back(data);
      }
  }

  Assignment<std::string, std::string> assignment;

  // !!! low level search
  // first with no constraints
  for (const auto& data : inputData) {
      std::string group;

      std::cout << "Agent: " << data.agent << ", Cost: " << data.cost << ", Tasks: ";
      for (const std::string& task : data.taskSet) {
          std::cout << task << " ";
          group += 't'+task + "_";

      }

      group.pop_back();  // Remove the last underscore
      std::cout << "Group: " << group << std::endl;

      assignment.setCost(data.agent, group, data.cost);
  }

  std::map<std::string, std::string> solution;
  int64_t c = assignment.solve(solution);
  std::cout << "solution with cost: " << c << std::endl;
  for (const auto& s : solution) {
    std::cout << s.first << ": " << s.second << std::endl;
  }


  HighLevelNode start;


}
