#include <jude/integration/http/httplib.h>
#include <sstream>
#include "RestApiClient.h"
#include "JsonPrettyPrinter.h"

namespace
{
   std::string ProcessResponse(httplib::Result &&response)
   {
      std::stringstream ss;
      if (!response)
      {
         ss << "Error: Could not access remote DB" << std::endl;
      }
      else
      {
         auto body = response.value().body;
         if (body.length())
         {
            if (response.value().get_header_value("Content-Type") == "application/json")
            {
               JsonPrettyPrinter prettyPrinter(ss);
               prettyPrinter.Print(body);
            }
            else
            {
               ss << body << std::endl;
            }
         }
         else
         {
            ss << "[No Response Body]";
         }

         ss << std::endl << "HTTP "<< response.value().status << std::endl;
      }
      return ss.str();
   }
}

namespace jude
{

RestApiClient::RestApiClient(const std::string& url)
{
   m_cli = new httplib::Client(url);
}

RestApiClient::~RestApiClient()
{
   delete m_cli;
}

std::string RestApiClient::Get(const std::string& url)
{
   return ProcessResponse(m_cli->Get(url.c_str()));
}

std::string RestApiClient::Delete(const std::string& url)
{
   return ProcessResponse(m_cli->Delete(url.c_str()));
}

std::string RestApiClient::Post(const std::string& url, const std::string& body)
{
   return ProcessResponse(m_cli->Post(url.c_str(), body, "applicaiton/json"));
}

std::string RestApiClient::Patch(const std::string& url, const std::string& body)
{
   return ProcessResponse(m_cli->Patch(url.c_str(), body, "applicaiton/json"));
}

std::string RestApiClient::Put(const std::string& url, const std::string& body)
{
   return ProcessResponse(m_cli->Put(url.c_str(), body, "applicaiton/json"));
}

std::string RestApiClient::Prompt()
{
   httplib::Params params = { { "prompt", ""}};

   auto response = m_cli->Get("/prompt", params, httplib::Headers(), httplib::Progress());
   if (response)
   {
      return response.value().body;
   }

   return "";
}

std::vector<std::string> RestApiClient::Completions(const std::string& url)
{
   std::vector<std::string> completions;

   httplib::Params params = { { "completions", ""}};

   auto response = m_cli->Get(url.c_str(), params, httplib::Headers(), httplib::Progress());
   if (response && response->status == 200)
   {
      std::istringstream input;
      input.str(response.value().body);

      for (std::string line; std::getline(input, line); ) 
      {
         completions.push_back(line);
      }
   }

   return completions;
}

} // namespace jude
