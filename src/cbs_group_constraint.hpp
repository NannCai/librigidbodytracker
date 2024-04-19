
#include <fstream>
#include <iostream>
#include "cbs_assignment.hpp"

using libMultiRobotPlanning::Assignment;


struct Constraint{
  std::string agent;
  std::set<std::string> taskSet;

  friend std::ostream& operator<<(std::ostream& os, const Constraint& c) {
    // os << "current Constraint: " ;
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
    if (solution.size() != n.solution.size()){
      return solution.size() < n.solution.size(); // Nodes with more pairs come first
    }
    if (cost != n.cost){
      return cost > n.cost;
    }
    return id > n.id;
  }

  friend std::ostream& operator<<(std::ostream& os, const HighLevelNode& c) {
    // os << "current HighLevelNode" << std::endl;
    os << "id: " << c.id << " cost: " << c.cost<< " Solution size: " << c.solution.size() << std::endl;
    
    if (c.solution.empty()) {
      os << "No sets in the solution map." << std::endl;
    }
    // else{
    else if (c.cost == 401){
      os << "solution:\n";
      for (const auto& s : c.solution) {
        os << s.first << ": ";
        for (const auto& element : s.second) {
          os << element << " ";
        }
        os << std::endl;
      }
    }
    // if (c.constraints.empty()) {
    //   os << "No constraints." << std::endl;
    // } else {
    //   os << "Constraints:" << std::endl;
    //   for (size_t i = 0; i < c.constraints.size(); ++i) {
    //     os << c.constraints[i];
    //   }
    // }
    return os;
  }
};

struct InputData {
    std::string agent;
    long cost;
    std::set<std::string> taskSet;
    int id;
};

void processInputFile(const std::string& inputFile, std::vector<InputData>& inputData) {
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
}

bool getFirstConflict(
    const std::map<std::string, std::set<std::string>>& solution,
    std::string& conflict_task) {
  std::unordered_map<std::string, int> taskCounts; 
  for (auto iter = solution.begin(); iter != solution.end(); ++iter) {
    std::set<std::string> current_set = iter->second;
    for (const std::string& task : current_set){
      taskCounts[task]++;
      if (taskCounts[task] > 1){
        // std::cout << "Element appearing more than once: task" << task << std::endl;
        conflict_task = task;
        // break;
        return true;
      }
    }
  }    
  return false;
}

void createConstraintsFromConflict(
    const std::map<std::string, std::set<std::string>>& solution,
    const std::string& conflict_task, 
    std::vector<Constraint>& constraints){
  int count = 0;
  for (const auto& pair : solution) {
    std::set<std::string> current_set = pair.second;
    for (const std::string& task : current_set){ 
      if (task == conflict_task){
        count++;
        Constraint con;
        con.agent = pair.first;
        con.taskSet = pair.second;
        constraints.push_back(con);
      }
      if (count>1){return ;}
    }
  }
}

void LowLevelSearch(
    const Constraint& new_constraint,
    const std::vector<InputData>& inputData,
    const HighLevelNode& P,
    HighLevelNode& newNode,
    int& id){
  newNode.id = id;
  ++id;
  newNode.constraints = P.constraints;
  newNode.constraints.push_back(new_constraint);
  Assignment<std::string, std::string> assignment;
  for (const auto& data : inputData) {
    bool skipData = false;
    for (const auto& constraint : newNode.constraints) {
      if (data.agent == constraint.agent && data.taskSet == constraint.taskSet) {
        skipData = true;
        break;
      }
    }
    if (!skipData) {
      assignment.setCost(data.agent, data.taskSet, data.cost);
    }
  }

  std::map<std::string, std::set<std::string>> solution;
  int64_t cost = assignment.solve(solution);

  newNode.cost = cost;
  newNode.solution = solution;
}