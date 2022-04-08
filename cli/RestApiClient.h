#include <string>
#include <vector>

namespace httplib 
{
   class Client;
}

namespace jude
{
   class RestApiClient
   {
      httplib::Client *m_cli;

   public:
      RestApiClient(const std::string& baseUrl);
      ~RestApiClient();

      std::string Prompt();
      std::string Get(const std::string& path);
      std::string Delete(const std::string& path);
      std::string Post(const std::string& path, const std::string& body);
      std::string Patch(const std::string& path, const std::string& body);
      std::string Put(const std::string& path, const std::string& body);
      std::vector<std::string> Completions(const std::string& path);
      std::string Swagger();
   };
}
