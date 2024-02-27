#include <fstream>
#include <iostream>
#include <regex>

#include <boost/program_options.hpp>
#include <boost/heap/d_ary_heap.hpp>

// #include "assignment.hpp"
#include "cbs_assignment.hpp"
// #include "assignment.hpp"

using libMultiRobotPlanning::Assignment;

struct Constraint{
  int agent;
  std::set<std::string> taskSet;

  friend std::ostream& operator<<(std::ostream& os, const Constraint& c) {
    // for (const auto& vc : c.vertexConstraints) {
    //   os << vc << std::endl;
    // }
    // for (const auto& ec : c.edgeConstraints) {
    //   os << ec << std::endl;
    // }
    os << "Agent: " << c.agent << ", Tasks: ";
    for (const std::string& task : c.taskSet) {
      os << task << " ";
    }
    os << std::endl;
    return os;
  }
};

struct HighLevelNode {
  // std::vector<PlanResult<State, Action, Cost> > solution;
  std::map<std::string, std::set<std::string>>  solution;
  // constraints: [(a1,t1,t2),(a5,t3,t9)] 
  std::vector<Constraint> constraints;

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
    os << "current HighLevelNode" << std::endl;
    os << "id: " << c.id << " cost: " << c.cost << std::endl;
    
    if (c.solution.empty()) {
      std::cout << "No sets in the solution map." << std::endl;
    }
    else{
      for (const auto& s : c.solution) {
        std::cout << s.first << ": ";
        for (const auto& element : s.second) {
          std::cout << element << " ";
        }
        std::cout << std::endl;
      }
    }

    if (c.constraints.empty()) {
      os << "No constraints." << std::endl;
    } else {
      os << "Constraints:" << std::endl;
      for (size_t i = 0; i < c.constraints.size(); ++i) {
        os << c.constraints[i];
      }
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
      int id;
  };

  std::vector<InputData> inputData;
  int input_id = 0;

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
          data.id = input_id++;
          inputData.push_back(data);
      }
  }


  // !!! low level search
  // first with no constraints
  std::cout << "-----low level search------" << std::endl;
  std::cout << "-----loading data, set cost------" << std::endl;
  Assignment<std::string, std::string> assignment;
  for (const auto& data : inputData) {
    std::string group;
    std::cout << "Agent: " << data.agent << ", Cost: " << data.cost << ", Tasks: ";
    for (const std::string& task : data.taskSet) {
      std::cout << task << " ";
      // group += 't'+task + "_";
      // group += task + "_";
    }
    // group.pop_back();  // Remove the last underscore
    // std::cout << "Group: " << group << std::endl;
    assignment.setCost(data.agent, data.taskSet, data.cost);
  }

  std::cout << "-----solve assignment------" << std::endl;
  // std::map<std::string, std::string> solution;
  std::map<std::string, std::set<std::string>> solution;
  int64_t c = assignment.solve(solution);

  std::cout << "-----put res into a HighLevelNode------" << std::endl;
  HighLevelNode start;
  start.id = 0;
  start.cost = c;
  start.solution = solution;
  std::cout << start;

  typename boost::heap::d_ary_heap<HighLevelNode, boost::heap::arity<2>,
                                    boost::heap::mutable_<true> >
      open;

  std::cout << "-----HighLevelNode into Queue------" << std::endl;
  auto handle = open.push(start);
  (*handle).handle = handle;

  solution.clear();
  int id = 1;
  while (!open.empty()) {
    std::cout << "-----high level check------" << std::endl;
    HighLevelNode P = open.top();

    open.pop();

    if (!P.solution.empty()) {
      std::set<std::string> common_elements = P.solution.begin()->second;
      std::map<std::string, std::set<std::string>> common_pair;

      for (const auto& pair : P.solution) {
        std::set<std::string> current_set = pair.second;
        std::set<std::string> intersection;
        std::set_intersection(common_elements.begin(), common_elements.end(),
                              current_set.begin(), current_set.end(),
                              std::inserter(intersection, intersection.begin()));
        if (!intersection.empty()) {
            common_elements = std::move(intersection);
        }
      }
      if (!common_elements.empty()) {
        std::cout << "Common elements"  << ": ";
        for (const auto& element : common_elements) {
            std::cout << element << " ";
        }
        std::cout << std::endl;



        std::cout << "Common pairs "  << ": "<< std::endl;
        for (const auto& pair : P.solution) {
          std::set<std::string> current_set = pair.second;
          if (std::includes(current_set.begin(), current_set.end(), common_elements.begin(), common_elements.end())) {
            common_pair[pair.first] = pair.second;
            std::cout << "agent: " << pair.first << ", tasks: ";
            for (const auto& element : pair.second) {
              std::cout << element << " ";
            }
            std::cout << std::endl;
          }
        }
      }

    }
    ++id;
  }









}
