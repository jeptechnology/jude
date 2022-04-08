#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <sstream>

#include "linenoise.hpp"
#include "RestApiClient.h"

namespace // local
{
   std::vector<std::string> TokenizeBuffer(const char* str, char c = ' ')
   {
      std::vector<std::string> result;

      while (*str)
      {
         const char* begin = str;

         while (*str != c && *str)
         {
            str++;
         }

         result.push_back(std::string(begin, str));

         if (*str)
         {
            str++;
            if (*str == '\0')
            {
               // trailing space...
               result.push_back("");
            }
         }
      }

      return result;
   }

   std::unique_ptr<jude::RestApiClient> judeService;
   std::vector<std::string> commands { "get", "set", "post", "patch", "delete", "put" };

   void completionHook(const char* editBuffer, std::vector<std::string>& completions)
   {
      auto tokens = TokenizeBuffer(editBuffer);
      if (tokens.size() == 0)
      { 
         for (auto& c : commands)
         {
            completions.push_back(c);
         }

         return;
      }

      auto& command = tokens[0];
      if (tokens.size() == 1)
      {
         for (auto& c : commands)
         {
            if (c.compare(0, tokens[0].length(), tokens[0]) ==0)
            {
               completions.push_back(c);
            }
         }
      }
      else if (tokens.size() == 2)
      {
         if (std::find(commands.begin(), commands.end(), command) == commands.end())
         {
            return;
         }

         for (const auto &suffix : judeService->Completions(tokens[1]))
         {
            completions.push_back(command + " " + suffix);
         }
      }
      else if (tokens.size() == 3 && (command == "patch" || command == "set"))
      {
         for (const auto &suffix : judeService->Completions(tokens[1] + " " + tokens[2]))
         {
            completions.push_back(command + " " + suffix);
         }
      }         
   }
}

int main(int argc, char *argv[])
{
   if (argc < 2)
   {
      std::cerr << "Please provide a base connection url e.g. jude_cli localhost:8080" << std::endl;
      return -1;
   }

   std::string baseurl = argv[1];

   const auto path = "history.txt";

   // Enable the multi-line mode
   linenoise::SetMultiLine(true);

   // Set max length of the history
   linenoise::SetHistoryMaxLen(32);

   judeService = std::make_unique<jude::RestApiClient>(baseurl);

   // Setup completion words every time when a user types
   linenoise::SetCompletionCallback(completionHook);

   // Load history
   linenoise::LoadHistory(path);


   while (true) 
   {
      std::string line;

      std::stringstream prompt;
      prompt << "\x1b[1;32m" << judeService->Prompt() << "@" << baseurl << "\x1b[0m> ";

      auto quit = linenoise::Readline(prompt.str().c_str(), line);

      if (quit) 
      {
         break;
      }
      
      auto tokens = TokenizeBuffer(line.c_str());
      if (tokens.size() < 2)
      {
         if(tokens.size() == 1)
         {
            std::cout << "Invalid Command: expected at least one argument" << std::endl;
         } 
         continue;
      }

      auto command = tokens[0];
      auto url = tokens[1];
      auto body = tokens.size() > 2 ? tokens[2] : "";

      if (command == "get")
      {
         std::cout << judeService->Get(url) << std::endl;
      }
      else if (command == "post")
      {
         std::cout << judeService->Post(url, body) << std::endl;
      }
      else if (command == "patch" || command == "set")
      {
         std::cout << judeService->Patch(url, body) << std::endl;
      }
      else if ((command == "put"))
      {
         std::cout << judeService->Put(url, body) << std::endl;
      }
      else if (command == "delete")
      {
         std::cout << judeService->Delete(url) << std::endl;
      }

      // Add line to history
      linenoise::AddHistory(line.c_str());

      // Save history
      linenoise::SaveHistory(path);
   }

   return 0;
}
