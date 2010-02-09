/*
 * opencog/rest/WebModule.cc
 *
 * Copyright (C) 2010 by Singularity Institute for Artificial Intelligence
 * All Rights Reserved
 *
 * Written by Joel Pitt <joel@fruitionnz.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "WebModule.h"

#include <opencog/server/CogServer.h>
#include <opencog/server/Request.h>
#include <opencog/util/Config.h>

#include <sstream>
#include <boost/bind.hpp>
#include <boost/mem_fn.hpp>
#include <boost/function.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

using namespace opencog;

WebModule *rest_mod;

// load/unload functions for the Module interface
extern "C" const char* opencog_module_id()   { return WebModule::id(); }
extern "C" Module*     opencog_module_load() {
    rest_mod = new WebModule();
    return rest_mod;
}
extern "C" void        opencog_module_unload(Module* module) { delete module; }

const char* WebModule::DEFAULT_SERVER_ADDRESS = "http://localhost";

//! @todo create a function to generate header
const char* WebModule::open_html_header = "HTTP/1.1 200 OK\r\n"
    "content-Type: text/html\r\n\r\n"
    "<html><head>";
const char* WebModule::close_html_header = "</head><body>";

const char* WebModule::html_refresh_header = 
    "<META HTTP-EQUIV=\"Refresh\" CONTENT=\"5\">";

const char* WebModule::html_footer = "</body></html>\r\n";

WebModule::WebModule() : _port(DEFAULT_PORT), serverAddress(DEFAULT_SERVER_ADDRESS)
{
    logger().debug("[WebModule] constructor");

    if (config().has("Web_PORT"))
        _port = config().get_int("Web_PORT");
    if (config().has("Web_SERVER"))
        serverAddress = config().get("Web_SERVER");

    // Register all requests with CogServer
    CogServer& cogserver = static_cast<CogServer&>(server());
    cogserver.registerRequest(GetAtomRequest::info().id, &getAtomFactory); 
    cogserver.registerRequest(GetListRequest::info().id, &getListFactory); 

    timeout = 100;

}

WebModule::~WebModule()
{
    rest_mod = NULL;
    logger().debug("[WebModule] destructor");
    //CogServer& cogserver = static_cast<CogServer&>(server());
    mg_stop(ctx);
}

static const char* DEFAULT_WEB_PATH[] =
{
    DATADIR,
    "../opencog/web", // For running from bin dir that's in root of src
#ifndef WIN32
    "/usr/share/opencog/www",
    "/usr/local/share/opencog/www",
#endif // !WIN32
    NULL
};

void WebModule::init()
{
    logger().debug("[WebModule] init");
    // Set the port that the embedded mongoose webserver will listen on.
    std::stringstream port_str;
    port_str << _port;
    ctx = mg_start();
    mg_set_option(ctx, "ports", port_str.str().c_str());
    // Turn on admin page
    //mg_set_option(ctx, "admin_uri", "/admin/");
    // Find and then set path for web resource files
    int i = 0;
    for (; DEFAULT_WEB_PATH[i] != NULL; ++i) {
        boost::filesystem::path webPath(DEFAULT_WEB_PATH[i]);
        webPath /= "processing.js";
        if (boost::filesystem::exists(webPath)) {
            break;
        }
    }
    mg_set_option(ctx, "root", DEFAULT_WEB_PATH[i]);
    // Turn off directory listing
    mg_set_option(ctx, "dir_list", "no");
    // Set up the urls
    setupURIs();
}

void viewAtomPage( struct mg_connection *conn,
        const struct mg_request_info *ri, void *data);
void viewListPage( struct mg_connection *conn,
        const struct mg_request_info *ri, void *data);
void makeRequest( struct mg_connection *conn,
        const struct mg_request_info *ri, void *data);

void WebModule::setupURIs()
{
    setupURIsForREST();
    setupURIsForUI();
}

void WebModule::setupURIsForUI()
{
    // Support both "atom/UUID" and "atom?handle=UUID"
    mg_set_uri_callback(ctx, UI_PATH_PREFIX "/atom/*", viewAtomPage, NULL);
    mg_set_uri_callback(ctx, UI_PATH_PREFIX "/atom", viewAtomPage, NULL);
    mg_set_uri_callback(ctx, UI_PATH_PREFIX "/list", viewListPage, NULL);
    mg_set_uri_callback(ctx, UI_PATH_PREFIX "/list/*", viewListPage, NULL);
    mg_set_uri_callback(ctx, UI_PATH_PREFIX "/request/*", makeRequest, NULL);
}

void WebModule::setupURIsForREST()
{
    static char rest_str[] = "rest";
    // atom/type/* support GET atoms of type.
    mg_set_uri_callback(ctx, REST_PATH_PREFIX "/atom/type/*", viewListPage,
            rest_str);
    // atom/ support GET/PUT/POST == get info/create/create
    mg_set_uri_callback(ctx, REST_PATH_PREFIX "/atom/", viewListPage,
            rest_str);
    // atom/* support GET, get atom info
    mg_set_uri_callback(ctx, REST_PATH_PREFIX "/atom/*", viewAtomPage,
            rest_str);
    // server/request/request_name, POST
    mg_set_uri_callback(ctx, REST_PATH_PREFIX "/server/request/*", makeRequest,
            rest_str);
}

void WebModule::return400(mg_connection* conn, const std::string& message)
{
    mg_printf(conn, "HTTP/1.1 400 %s\r\n", message.c_str());
}

void WebModule::return404(mg_connection* conn)
{
    mg_printf(conn, "HTTP/1.1 404 Not found.\r\n");
}

void WebModule::return500(mg_connection* conn, const std::string& message)
{
    mg_printf(conn, "HTTP/1.1 500 %s\r\n", message.c_str());
}

void viewAtomPage( struct mg_connection *conn,
        const struct mg_request_info *ri, void *data)
{ rest_mod->atomURLHandler.handleRequest(conn, ri, data); }

void viewListPage( struct mg_connection *conn,
        const struct mg_request_info *ri, void *data)
{ rest_mod->listURLHandler.handleRequest(conn, ri, data); }

void makeRequest ( struct mg_connection *conn,
        const struct mg_request_info *ri, void *data)
{ rest_mod->requestWrapper.handleRequest(conn, ri, data); }


