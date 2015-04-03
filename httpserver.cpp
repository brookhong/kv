#include <string>
#include <string.h>
using namespace std;
#include "mongoose.h"
string queryDict(const char *idxFileName, const char *keyword);

static const char *html_form =
"<html><body>"
"<form method=\"POST\" action=\"/query\">"
"Word: <input type=\"text\" name=\"word\" />"
"<input type=\"submit\" value='query'/>"
"</form></body></html>";

static int handler(struct mg_connection *conn) {
    char var1[500];

    if (strcmp(conn->uri, "/query") == 0) {
        // User has submitted a form, show submitted data and a variable value
        // Parse form data. var1 and var2 are guaranteed to be NUL-terminated
        mg_get_var(conn, "word", var1, sizeof(var1));

        // Send reply to the client, showing submitted form values.
        // POST data is in conn->content, data length is in conn->content_len
        mg_send_header(conn, "Content-Type", "text/plain");
        string ret = queryDict((const char *)conn->server_param, var1);
        mg_printf_data(conn, "%s\n", ret.c_str());
    } else {
        // Show HTML form.
        mg_send_data(conn, html_form, strlen(html_form));
    }

    return 1;
}
typedef struct _HTTPD{
    const char * idxFileName;
    const char * port;
}HTTPD;

int stopped = 0;

void _httpServer(void *pv) {
    HTTPD *p = (HTTPD*)pv;
    static string sIdxFile = p->idxFileName;
    struct mg_server *server = mg_create_server((void*)sIdxFile.c_str());
    mg_set_option(server, "listening_port", p->port);
    mg_add_uri_handler(server, "/", handler);
    printf("Starting on port %s\n", mg_get_option(server, "listening_port"));
    while (!stopped) {
        mg_poll_server(server, 1000);
    }
    mg_destroy_server(&server);
}

#ifndef _WIN32
int httpServer(const char *idxFileName, const char *port) {
    HTTPD httpd;
    httpd.idxFileName = idxFileName;
    httpd.port = port;
    _httpServer(&httpd);
    return 0;
}
#endif
