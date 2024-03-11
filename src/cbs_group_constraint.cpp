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
  std::string agent;
  std::set<std::string> taskSet;

  friend std::ostream& operator<<(std::ostream& os, const Constraint& c) {
    os << "current Constraint: " ;
    os << "Agent: " << c.agent << ", Tasks: ";
    for (const std::string& task : c.taskSet) {
      os << task << " ";
    }
    os << std::endl;
    return os;
  }
};

struct HighLevelNode {
  std::map<std::string, std::set<std::string>> solution;
  std::vector<Constraint> constraints;

  long cost;
  int id;

  typename boost::heap::d_ary_heap<HighLevelNode, boost::heap::arity<2>,
                                    boost::heap::mutable_<true> >::handle_type
      handle;

  bool operator<(const HighLevelNode& n) const {
    // if (cost != n.cost)
    // return cost > n.cost;
    // return id > n.id;

    // First, prioritize by the number of pairs in the solution
    if (solution.size() != n.solution.size()){
      return solution.size() < n.solution.size(); // Nodes with more pairs come first
    }

    // If the number of pairs is the same, then prioritize by cost (lower cost comes first)
    return cost > n.cost; // Note: This is intentionally '>' to make the priority queue a min-heap based on cost


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

struct InputData {
    std::string agent;
    long cost;
    std::set<std::string> taskSet;
    int id;
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
    std::cout << "Agent: " << data.agent << ", Cost: " << data.cost << ", Tasks: ";
    for (const std::string& task : data.taskSet) {
      std::cout << task << " ";
    }
    assignment.setCost(data.agent, data.taskSet, data.cost);
  }

  std::cout << "-----solve assignment------" << std::endl;
  std::map<std::string, std::set<std::string>> solution;
  int64_t cost = assignment.solve(solution);

  std::cout << "-----put solution into a HighLevelNode------" << std::endl;
  HighLevelNode start;
  start.id = 0;
  start.cost = cost;
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

    if (P.solution.empty()) {
      std::cout << "Cannot find a solution!" << std::endl;
    }
    auto iter = P.solution.begin();

    std::string common_element;
    std::map<std::string, int> elementCounts;
    for (; iter != P.solution.end(); ++iter) {
      std::set<std::string> current_set = iter->second;
      for (const std::string& task : current_set){
        elementCounts[task]++;
        if (elementCounts[task] > 1){
          std::cout << "Element appearing more than once: task" << task << std::endl;
          common_element = task;
        }
      }
    }
    // std::cout << "!common_element.empty()"<<!common_element.empty() << std::endl; 

    std::vector<Constraint> constraints;
    if (!common_element.empty()) {
      std::cout << "Common element"  << ": "<< common_element << std::endl;
      for (const auto& pair : P.solution) {
        std::set<std::string> current_set = pair.second;
        for (const std::string& task : current_set){ 
          if (task == common_element){
            Constraint con;
            con.agent = pair.first;
            con.taskSet = pair.second;
            std::cout << con;
            constraints.push_back(con);
          }
        }
      }
    }
    else{
      std::cout << "no common_element, Breaking out of the loop.\n";
      std::cout << P;
      break;  
    }

    for (const auto& constraint : constraints) {
      std::cout << constraint;
      std::cout << "-----low level search------" << std::endl;
      Assignment<std::string, std::string> assignment;
      for (const auto& data : inputData) {
        std::cout << "Agent: " << data.agent << ", Cost: " << data.cost << ", Tasks: ";
        for (const std::string& task : data.taskSet) {
          std::cout << task << " ";
        }
        if (data.agent==constraint.agent && data.taskSet == constraint.taskSet){
          std::cout << "Condition: data.agent==constraint.agent && data.taskSet == constraint.taskSet, skip\n";
          continue;}
        assignment.setCost(data.agent, data.taskSet, data.cost);
      }
      std::map<std::string, std::set<std::string>> solution;
      int64_t cost = assignment.solve(solution);

      HighLevelNode newNode = P;
      newNode.id = id;
      newNode.cost = cost;
      newNode.solution = solution;
      newNode.constraints.push_back(constraint);
      std::cout << newNode;

      auto handle = open.push(newNode);
      (*handle).handle = handle;
      ++id;
    }


      // TODO feed Common pair into the new assignment as the constraint



  }









}
