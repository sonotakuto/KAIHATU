#include "./includes.hpp"
#include "./glob_var.hpp"

int db_init(void){
	auto conn = std::make_unique<OdbcConnection>(ODBC_DSN, ODBC_USER, ODBC_PASS);
	g_db_session = std::make_unique<OdbcSession>(std::move(conn));
}