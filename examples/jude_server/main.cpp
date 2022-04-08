#include "autogen/server_example/TestDB.h"

#include <cstdlib>
#include <jude/integration/http/HttpServer.h>


extern "C" jude_os_interface_t jude_porting_interface_cpp11;
jude_os_interface_t *jude_os = &jude_porting_interface_cpp11;

int main(int argc, char *argv[])
{
   jude::TestDB db;

   int port = 8080;

   if (argc > 1)
   {
      port = atoi(argv[2]);
   }

   jude::HttpServer server(db, jude_user_Admin);

   server.Serve("localhost", port);
   
   return 0;
}