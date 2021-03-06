#include "vcx/VCXParser.hpp"

#include "cmake/CMakeListsWriter.hpp"
#include "cmake/CMakeConfigTemplateWriter.hpp"
#include "cmake/CMakeMsc.hpp"
#include "cmake/CMakeSubDirRegistering.hpp"

#include <map>

using namespace proj2cmake;
namespace fs = boost::filesystem;
using ProjectsPaths = std::map<fs::path, std::vector<vcx::ProjectPair*>>;

void writeGeneratedNote(std::ostream& os, const char* procName)
{
   os << "#" << std::endl;
   os << "# This file was genared by " << procName << " and will be overwritten on it's next run!" << std::endl;
   os << "# Please put all configurations in the cmake_conf/*.cmake files." << std::endl;
   os << "#" << std::endl;
   os << std::endl;
}

void writeProject(std::ofstream& os, const vcx::Solution& solution, const ProjectsPaths& dirToProj)
{
   os << "project(" << solution.name << ")" << std::endl;
   os << std::endl;

//   fs::path confFile = "cmake_conf";
//   confFile /= (solution.name + ".cmake");

//   //os << "IF(EXISTS \"${" << solution.name << "_SOURCE_DIR}/cmake_conf/" << solution.name << ".cmake\")" << std::endl;
//   os << "include(\"${" << solution.name << "_SOURCE_DIR}/" + confFile.string() + "\")" << std::endl;
//   //os << "ENDIF()" << std::endl;
//   os << std::endl;

//   auto fullConfPath = solution.basePath / confFile;
//   if(fs::exists(fullConfPath) == false)
//   {
//      fs::create_directories(fullConfPath.parent_path());
//      cmake::ConfigTemplateWriter writer(solution);
//      std::ofstream os(fullConfPath.native());
//      writer(os);
//   }

   cmake::CMakeSubDirRegistering subDirRegister(os);
   for(auto&& subDir : dirToProj)
   {
      if (solution.basePath == subDir.first)
         continue;

      subDirRegister(subDir.second[0]->first.projectFile.parent_path());
   }
   os << std::endl;
}

int main(int argc, char** argv)
{
   namespace fs = boost::filesystem;

   auto procName = argv[0];

   if(argc < 2)
   {
      std::cerr << "Usage: " << procName << " <SolutionFile>" << std::endl;
      return 1;
   }

   auto solutionFile = argv[1];

   if(boost::iends_with(solutionFile, ".sln") == false)
   {
      std::cerr << "The first parameter has to be a Visual Studio solution file (*.sln)" << std::endl;
      return 1;
   }

   vcx::SolutionParser parser(solutionFile);
   auto solution = parser();

   ProjectsPaths dirToProj;

   for(auto&& p : solution.projects)
   {
      auto&& pInfo = p.first;
      auto&& project = p.second;

      size_t nFiles = project.compileFiles.size() + project.includeFiles.size();

      if(nFiles == 0)
         continue;

      auto cmakeSrcFile = solution.basePath / pInfo.projectFile;
      cmakeSrcFile.replace_extension(".cmake");

      if (nFiles > 1)
      {
          cmake::ListsWriter writer(p);

          std::ofstream os(cmakeSrcFile.native());
          writer(os);
      }

      dirToProj[cmakeSrcFile.parent_path()].push_back(&p);
   }

   bool hasProject = false;

   for(auto&& p : dirToProj)
   {
       auto f = p.first / "CMakeLists.txt";
       std::ofstream os(f.native());
       //writeGeneratedNote(os, procName);

       os << "cmake_minimum_required(VERSION 3.14)" << std::endl;
       os << std::endl;
       if(p.first == solution.basePath)
       {
           writeProject(os, solution, dirToProj);
           hasProject = true;
       }

       for(auto&& pr : p.second)
       {
           auto&& pInfo = pr->first;
           auto&& project = pr->second;

           os << std::endl;

           os << "include(GMGCommon)" << std::endl;

           os << std::endl;

           os << cmake::cmakeStartType(pInfo.name, project.type);

           size_t nFiles = project.compileFiles.size() + project.includeFiles.size();

           if (nFiles == 1)
           {
               for(auto&& f : project.compileFiles)
               {
                   os << " " << f;
               }

               for(auto&& f : project.includeFiles)
               {
                   os << " " << f;
               }
           }
           os << ")" << std::endl;
           os << std::endl;

           if (nFiles > 1) {
               auto cmakeSrcFile = solution.basePath / pInfo.projectFile;
               cmakeSrcFile.replace_extension(".cmake");

               os << "include(\"" + cmakeSrcFile.filename().string() + "\")" << std::endl;
               os << std::endl;
           }

           os << "target_link_libraries(" << cmake::tokenize(pInfo.name);

           for(auto&& proc : project.referencedProjects)
           {
               os << " " << cmake::tokenize(proc.name) << std::endl;
           }
           os << ")" << std::endl;
       }
   }

   if (!hasProject)
   {
      auto f = solution.basePath / "CMakeLists.txt";
      std::ofstream os(f.native());
      writeProject(os, solution, dirToProj);
   }

   return 0;
}
