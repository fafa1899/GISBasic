//
#include <iostream>
#include <pqxx/pqxx>

//
#include <httplib.h>

#include <nlohmann/json.hpp>

using namespace std;
using namespace httplib;

nlohmann::json QueryBuilding(const string& x, const string& y) {
  nlohmann::json json;

  std::string connectionString =
      "dbname=postgis_34_sample user=postgres password=123456 "
      "hostaddr=127.0.0.1 port=5432";
  try {
    pqxx::connection connectionObject(connectionString.c_str());

    pqxx::work worker(connectionObject);

    string sqlTemplateString = R"(SELECT* 
FROM losangelesbuilding 
WHERE ST_Contains(geom_field, ST_SetSRID(ST_MakePoint(%s, %s), 32611));)";
    char sqlString[1024];
    sprintf(sqlString, sqlTemplateString.c_str(), x.c_str(), y.c_str());

    pqxx::result execResult = worker.exec(sqlString);
    for (int ci = 1; ci < execResult.columns() - 1; ++ci) {
    }
    if (execResult.size() > 0) {
      for (int j = 1; j < execResult[0].size() - 1; j++) {
        json[execResult.column_name(j)] = execResult[0][j].c_str();
      }
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return json;
}

void GetQueryBuilding(const Request& request, Response& response) {
  if (!request.has_param("x") || !request.has_param("y")) {
    response.status = 400;
    return;
  }

  nlohmann::json json =
      QueryBuilding(request.get_param_value("x"), request.get_param_value("y"));
  response.status = 200;
  response.set_content(json.dump(), "application/json");
}

int main(int argc, char* argv[]) {
  string workDir = getenv("GISBasic");
  workDir = workDir + "/../Code/Chaper10";

  Server server;

  server.set_mount_point("/", workDir);

  server.Get("/building/attribute", GetQueryBuilding);

  server.listen("0.0.0.0", 8000);

  return 0;
}