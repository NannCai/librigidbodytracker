#include <fstream>
#include <iostream>
#include <regex>

#include <boost/program_options.hpp>

// #include "assignment.hpp"
#include "assignment_group_constraint.hpp"

using libMultiRobotPlanning::Assignment;

int main(int argc, char* argv[]) {
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
    // std::set<std::string> taskSet;
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

  std::ofstream out(outputFile);
  out << "cost: " << c << std::endl;
  out << "assignment:" << std::endl;
  for (const auto& s : solution) {
    out << "  " << s.first << ": ";
    for (const auto& element : s.second) {
      out << element << " ";
    }
    out << std::endl;
  }

  return 0;
}
