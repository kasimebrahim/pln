/*
 * opencog/rest/WebModule.h
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

#ifndef _OPENCOG_WEB_MODULE_H
#define _OPENCOG_WEB_MODULE_H

#include <opencog/server/Module.h>
#include <opencog/server/Request.h>

#include "mongoose.h"
#include <opencog/web/GetListRequest.h>
#include <opencog/web/ListURLHandler.h>
#include <opencog/web/AtomURLHandler.h>
#include <opencog/web/ServerRequestWrapper.h>

#define REST_PATH_PREFIX "/rest/0.2"
#define UI_PATH_PREFIX "/opencog"

namespace opencog
{

class WebModule : public Module
{

private:

    unsigned short _port;

    // Mongoose HTTP server context
    struct mg_context *ctx;

    // Timeout - how long we should wait for CogServer to fulfill requests
    // before giving up.
    int timeout;

    // Register AtomSpace API requests. We can't directly access the AtomSpace
    // due to the Web Http server running in it's own set of threads.
    Factory<GetListRequest, Request> getListFactory;
    Factory<GetAtomRequest, Request> getAtomFactory;

    std::string serverAddress;

    void setupURIsForREST();
    void setupURIsForUI();
public:

    static const unsigned int DEFAULT_PORT = 17034;
    static const char* DEFAULT_SERVER_ADDRESS;

    //! @todo create a function to generate header
    static const char* open_html_header;
    static const char* close_html_header;
    static const char* html_refresh_header;
    static const char* html_footer;

    static inline const char* id() {
        static const char* _id = "opencog::WebModule";
        return _id;
    }

    WebModule();
    virtual ~WebModule();
    virtual void init  (void);
    
    void setupURIs();
    static void return400(mg_connection* conn, const std::string& message);
    static void return404(mg_connection* conn);
    static void return500(mg_connection* conn, const std::string& message);

    // Class which wraps requests registered with and destined for CogServer
    ServerRequestWrapper requestWrapper;

    // Class that handles /atom/* requests
    AtomURLHandler atomURLHandler;

    // Class that handles /list/* requests
    ListURLHandler listURLHandler;

    // Handle neighborhood requests
    //NeighborhoodURLHandler neighborhoodURLHandler;

    // Atom URL Handler for human user interface
    //UIAtomURLHandler uiAtomURLHandler;

}; // class

}  // namespace

#endif // _OPENCOG_WEB_MODULE_H
