#include <fstream>
#include <iostream>
#include <regex>

#include <boost/program_options.hpp>

// #include "assignment.hpp"
#include "cbs_assignment.hpp"

using libMultiRobotPlanning::Assignment;

  // initialise the cbs
  // then use cbs.search to find the solution

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

  Assignment<std::string, std::string> assignment;
  
  std::ifstream input(inputFile);
  for (std::string line; getline(input, line);) {
    std::cout << "line: " << line << "  -------";
    std::cout << std::endl;
    std::stringstream stream(line);

    std::string agent;
    stream >> agent;
    int cost;
    stream >> cost;
    std::set<std::string> taskSet;
    std::string task;
    bool skipLine = false;
    while (stream >> task) {
      if (taskSet.find(task) != taskSet.end()) {
        // Task is already in the set, skip this line
        std::cout << "Skipping line with duplicate task: " << task << std::endl;
        skipLine = true;
        break;
      }
      taskSet.insert(task);
    }
    std::cout << "end add tasks" << std::endl;
    if (!skipLine) {
    std::cout << "Agent: " << agent << ", Cost: " << cost << ", Tasks: ";
    for (std::string task : taskSet) {
        std::cout << task << " ";
    }
    std::cout << std::endl;

    assignment.setCost(agent, taskSet, cost);

    }
    taskSet.clear();

  }

  // !!low level search 
  // TODO maybe output all solution in the loop
  // TODO only first should pick the best
  std::map<std::string, std::set<std::string>> solution;
  int64_t c = assignment.solve(solution);
    std::cout << "solution with cost: " << c << std::endl;
    for (const auto& s : solution) {
      std::cout << s.first << ": ";
      for (const auto& element : s.second) {
        std::cout << element << " ";
      }
      std::cout << std::endl;
    }


  // !!high level check 
  // Find common elements among sets
  if (!solution.empty()) {
      std::set<std::string> common_elements = solution.begin()->second;
      for (const auto& pair : solution) {
          std::set<std::string> current_set = pair.second;
          std::set<std::string> intersection;
          std::set_intersection(common_elements.begin(), common_elements.end(),
                                current_set.begin(), current_set.end(),
                                std::inserter(intersection, intersection.begin()));
          common_elements = std::move(intersection);
      }

      if (common_elements.empty()) {
          std::cout << "There are no common elements across all sets." << std::endl;
          // TODO return solution
      } 
      else {
          std::cout << "Common elements across all sets: ";
          for (const auto& element : common_elements) {
              std::cout << element << " ";
          }
          std::cout << std::endl;

          // TODO return new constraint (for the new lowlevel search)
      }
  } 
  else {
      std::cout << "No sets in the solution map." << std::endl;
  }


  // only left the common task be once in the whole input (have more than one posibilities)
  // try: remove the first group vertex that have 1 in it, and corresponding edges(2 edges)

  std::set<std::set<std::string>> groupsContainingOne; 

  for (const auto& group : assignment.getGroups()) {
    if (group.find("1") != group.end()) {
      groupsContainingOne.insert(group); // Add the group containing '1' to the set
      std::cout << "Group containing '1' found: ";
      for (const auto& task : group) {
        std::cout << task << " ";
      }
      std::cout << std::endl;
    }
  }

  // if (groupContainsElement1) {
  //     // Perform the necessary operations before solving the assignment
  //     std::cout << "A group containing '1' was found." << std::endl;
  // }




  Assignment<std::string, std::string> assignment1;  // can this work?? is this deepcopy from the original assignment??
  assignment1 = assignment;




  
  // ?? make sure more agent can have 



}
