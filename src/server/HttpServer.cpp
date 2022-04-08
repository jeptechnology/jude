/*
 * The MIT License (MIT)
 * Copyright © 2022 James Parker
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal 
 * in the Software without restriction, including without limitation the rights 
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <jude/database/Database.h>
#include <jude/core/cpp/Stream.h>

#include <jude/server/httplib.h>
#include <jude/server/HttpServer.h>

using namespace jude;
using namespace httplib;

namespace
{
   std::string dump_headers(const Headers &headers)
   {
      std::string s;
      char buf[BUFSIZ];

      for (auto it = headers.begin(); it != headers.end(); ++it)
      {
         const auto &x = *it;
         snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
         s += buf;
      }

      return s;
   }

   std::string log(const Request &req, const Response &res)
   {
      std::string s;
      char buf[BUFSIZ];

      s += "================================\n";

      snprintf(buf, sizeof(buf), "%s %s %s", req.method.c_str(),
               req.version.c_str(), req.path.c_str());
      s += buf;

      std::string query;
      for (auto it = req.params.begin(); it != req.params.end(); ++it)
      {
         const auto &x = *it;
         snprintf(buf, sizeof(buf), "%c%s=%s",
                  (it == req.params.begin()) ? '?' : '&', x.first.c_str(),
                  x.second.c_str());
         query += buf;
      }
      snprintf(buf, sizeof(buf), "%s\n", query.c_str());
      s += buf;

      s += dump_headers(req.headers);

      s += "--------------------------------\n";

      snprintf(buf, sizeof(buf), "%d %s\n", res.status, res.version.c_str());
      s += buf;
      s += dump_headers(res.headers);
      s += "\n";

      if (!res.body.empty())
      {
         s += res.body;
      }

      s += "\n";

      return s;
   }

   StringOutputStream ReadContent(const ContentReader &content_reader)
   {
      StringOutputStream output;

      content_reader([&](const char *data, size_t data_length) {
        return output.Write(reinterpret_cast<const uint8_t*>(data), data_length) == data_length;
      });      

      return output;
   }
}

namespace jude
{
   std::string HttpServer::GetCompletionsFor(const std::string& prefix)
   {
      std::string responseContent;

      for (auto const& completion : m_database.SearchForPath(CRUD::READ, prefix.c_str(), 32))
      {
         responseContent += (completion + "\n");
      }

      return responseContent;
   }

   int HttpServer::Serve(const std::string& host, int port)
   {
   #ifdef CPPHTTPLIB_OPENSSL_SUPPORT
      SSLServer svr(SERVER_CERT_FILE, SERVER_PRIVATE_KEY_FILE);
   #else
      Server svr;
   #endif

      if (!svr.is_valid())
      {
         printf("server has an error...\n");
         return -1;
      }

      svr.Get("/.*", [&](const Request &req, Response &res) {
         jude::StringOutputStream output(1024 * 1024, 1024);
         
         if (req.has_param("completions"))
         {
            res.set_content(GetCompletionsFor(req.path), "text/plain");
         }
         else if (req.has_param("swagger"))
         {
            m_database.GenerateYAMLforSwaggerOAS3(output, jude_user_Admin);
            res.set_content(output.GetString(), "application/yaml");
         }
         else if (req.has_param("prompt"))
         {
            auto prompt = m_database.GetName();
            if (prompt.length() == 0)
            {
               prompt = "DB";
            }
            res.set_content(prompt, "text/plain");
         }
         else
         {
            auto result = m_database.RestGet(req.path.c_str(), output);
            res.status = result.GetCode();
            if (result)
            {
               res.set_content(output.GetString(), "application/json");
            }
            else
            {
               res.set_content(result.GetDetails(), "text/plain");
            }
         }
      });

      svr.Post("/.*", [&](const Request &req, Response &res, const ContentReader &content_reader) {
         
         // Read body
         auto body = ReadContent(content_reader);
         jude::RomInputStream input(body.GetString().c_str());
         auto result = m_database.RestPost(req.path.c_str(), input);
         res.status = result.GetCode();
         if (result)
         {
            jude::StringOutputStream output;
            std::stringstream newPath;
            newPath << req.path << "/" << result.GetCreatedObjectId();
            m_database.RestGet(newPath.str().c_str(), output);         
            res.set_content(output.GetString(), "application/json");
         }
         else
         {
            res.set_content(result.GetDetails(), "text/plain");
         }
      });

      svr.Patch("/.*", [&](const Request &req, Response &res, const ContentReader &content_reader) {
         
         // Read body
         auto body = ReadContent(content_reader);
         jude::RomInputStream input(body.GetString().c_str());
         auto result = m_database.RestPatch(req.path.c_str(), input);
         res.status = result.GetCode();
         if (result)
         {
            jude::StringOutputStream output;
            m_database.RestGet(req.path.c_str(), output);
            res.set_content(output.GetString(), "application/json");
         }
         else
         {
            res.set_content(result.GetDetails(), "text/plain");
         }
      });

      svr.Delete("/.*", [&](const Request &req, Response &res) {
         // Read body
         auto result = m_database.RestDelete(req.path.c_str());
         res.status = result.GetCode();
         if (result)
         {
            res.set_content("OK", "text/plain");
         }
         else
         {
            res.set_content(result.GetDetails(), "text/plain");
         }
      });

      svr.set_error_handler([](const Request & /*req*/, Response &res) {
         const char *fmt = "{ \"StatusCode\": %d, \"Error\":\"%s\"}";
         char buf[BUFSIZ];
         snprintf(buf, sizeof(buf), fmt, res.status, res.body.length() ? res.body.c_str() : "Internal Error");
         res.set_content(buf, "application/json");
      });

      svr.set_logger([](const Request &req, const Response &res) { printf("%s", log(req, res).c_str()); });

      std::cout << "Server listening on " << host << ":" << port << std::endl;
      svr.listen(host.c_str(), port);

      return 0;
   }
}