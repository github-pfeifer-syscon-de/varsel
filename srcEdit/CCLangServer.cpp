/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2025 RPf <gpl3@pfeifer-syscon.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <JsonObj.hpp>
#include <Log.hpp>
#include <StringUtils.hpp>
#include <cstring>

#include "CCLangServer.hpp"

CclsExit::CclsExit()
: RpcMessage::RpcMessage()
{
}

const char*
CclsExit::getMethod()
{
    return "exit";
}


CclsShutdown::CclsShutdown(RpcLaunch* rpcLaunch)
: RpcRequest::RpcRequest()
, m_rpcLaunch{rpcLaunch}
{
}

void
CclsShutdown::result(const std::shared_ptr<psc::json::JsonValue>& json)
{
    auto exit = std::make_shared<CclsExit>();
    m_rpcLaunch->communicate(exit);
}

const char*
CclsShutdown::getMethod()
{
    return "shutdown";
}

CclsInit::CclsInit(const Glib::RefPtr<Gio::File>& dir)
: RpcRequest::RpcRequest()
, m_dir{dir}
{
}

void
CclsInit::result(const std::shared_ptr<psc::json::JsonValue>& json)
{
    //m_rpcLaunch->initDone(json);
}

std::shared_ptr<psc::json::JsonObj>
CclsInit::create(RpcLaunch* rpcLaunch)
{
    auto obj = RpcRequest::create(rpcLaunch);
    auto params = obj->createObj("params");
    pid_t pid = getpid();
    params->set("processId", static_cast<gint64>(pid)); // support exiting ccls if parent dies...
    params->set("rootUri", m_dir->get_uri().c_str());
    auto caps = params->createObj("capabilities");
    // these are minimal requirements
    caps->createObj("textDocument/didOpen");
    caps->createObj("textDocument/didChange");
    caps->createObj("textDocument/didClose");
    //auto textDef = caps->createObj("textDocument.definition");
    //textDef->set("dynamicRegistration", false);
	//textDef->set("linkSupport", false);

    return obj;
}

const char*
CclsInit::getMethod()
{
    return "initialize";
}


CclsOpen::CclsOpen(const Glib::RefPtr<Gio::File>& file, const Glib::ustring& lang, int version)
: RpcMessage::RpcMessage()
, m_file{file}
, m_lang{lang}
, m_version{version}
{
}

const char*
CclsOpen::getMethod()
{
    return "textDocument/didOpen";
}

std::shared_ptr<psc::json::JsonObj>
CclsOpen::create(RpcLaunch* rpcLaunch)
{
    auto obj = RpcMessage::create(rpcLaunch);
    auto params = obj->createObj("params");
    auto textDoc = params->createObj("textDocument");
    textDoc->set("uri", m_file->get_uri().c_str());
    textDoc->set("languageId", m_lang);
    textDoc->set("version", m_version);
    Glib::ustring text = getText();
    textDoc->set("text", text);
    return obj;
}

Glib::ustring
CclsOpen::getText()
{
    if (m_text.empty()) {
        g_autoptr(GError) error{nullptr};
        auto channel = g_io_channel_new_file(m_file->get_path().c_str(), "r", &error);
        if (error) {
            std::cout << "Error " << error->message << " opening " << m_file->get_path() << std::endl;
            return m_text;
        }
        g_io_channel_set_encoding(channel, "UTF-8", nullptr);
        auto info = m_file->query_info("*");
        m_text.reserve(info->get_size());
        while (true) {
            gchar *string{nullptr};
            gsize size{0};
            GIOStatus stat = g_io_channel_read_line(channel, &string, &size, NULL, &error);
            if (stat == G_IO_STATUS_EOF) {
                break;
            }
            if (error) {
                std::cout << "Error " << error->message << " reading " << m_file->get_path() << std::endl;
                break;
            }
            else {
                m_text += string;
                g_free(string);
            }
        }
        g_io_channel_shutdown(channel, false, nullptr);
        g_io_channel_unref(channel);
    }
    return m_text;
}



CclsClose::CclsClose(const Glib::RefPtr<Gio::File>& file)
: RpcMessage::RpcMessage()
, m_file{file}
{
}

const char*
CclsClose::getMethod()
{
    return "textDocument/didClose";
}

std::shared_ptr<psc::json::JsonObj>
CclsClose::create(RpcLaunch* rpcLaunch)
{
    auto obj = RpcMessage::create(rpcLaunch);
    auto params = obj->createObj("params");
    auto textDoc = params->createObj("textDocument");
    textDoc->set("uri", m_file->get_uri().c_str());
    return obj;
}



CclsDocumentRef::CclsDocumentRef(const Glib::RefPtr<Gio::File>& file, const TextPos& pos, const Glib::ustring& method)
: RpcRequest::RpcRequest()
, m_file{file}
, m_pos{pos}
, m_method(method)
{
}

void
CclsDocumentRef::result(const std::shared_ptr<psc::json::JsonValue>& json)
{
    if (json->isObject()) {
        auto obj = json->getObject();
        std::cout << "RpcDef::result def "
                  << std::endl << obj->generate(2) << std::endl;
    }
    else if (json->isArray()) {
        auto arr = json->getArray();
        std::cout << "RpcDef::result def "
                  << std::endl << arr->generate(2) << std::endl;
    }
    else {
        psc::log::Log::logAdd(psc::log::Level::Warn, "Unkown type for definition result");
    }
}

const char*
CclsDocumentRef::getMethod()
{
    return m_method.c_str();
}

std::shared_ptr<psc::json::JsonObj>
CclsDocumentRef::create(RpcLaunch* rpcLaunch)
{
    auto obj = RpcRequest::create(rpcLaunch);
    auto params = obj->createObj("params");
    auto textDoc = params->createObj("textDocument");
    textDoc->set("uri", m_file->get_uri().c_str());
    auto pos = params->createObj("position");
    pos->set("line", m_pos.line);
    pos->set("character", m_pos.character);
    return obj;
}


CcLangServer::CcLangServer(const std::string& launch)
: RpcLaunch::RpcLaunch()
, m_launch{launch}
{
    run();
}

void
CcLangServer::shutdown()
{
    //std::string term{"\u0004"}; // use ^d to terminate ?
    //auto bytes_written = m_strmStdin->write(term);
    //kill(child_pid, SIGINT);   // SIGTERM
    auto shut = std::make_shared<CclsShutdown>(this);
    communicate(shut);
}

const std::vector<Glib::ustring>
CcLangServer::buildArgs()
{
    std::vector<Glib::ustring> args;
    args.reserve(8);
    StringUtils::split(m_launch, ' ', args);
    return args;
}

void
CcLangServer::setStatusListener(CclsStatusListener *statusListener)
{
    m_statusListener = statusListener;
}


Glib::ustring
CcLangServer::getStatus()
{
    return m_status;
}

Glib::ustring
CcLangServer::getMessage()
{
    return m_message;
}

Glib::ustring
CcLangServer::getTitle()
{
    return m_title;
}

gint64
CcLangServer::getPercent()
{
    return m_percent;
}

void
CcLangServer::decodeStatus(JsonObject* jsonObj)
{
    if (json_object_has_member(jsonObj, "params")) {
        auto params = json_object_get_object_member(jsonObj, "params");
        if (json_object_has_member(params, "token")) {
            m_status = json_object_get_string_member(params, "token");


        }
        if (json_object_has_member(params, "value")) {
            auto value = json_object_get_object_member(params, "value");
            CclsStatusKind statusKind{CclsStatusKind::None};
            if (json_object_has_member(value, "kind")) {
                auto kind = json_object_get_string_member(value, "kind");
                if (std::strcmp(kind, "begin") == 0) {
                    statusKind = CclsStatusKind::Begin;
                }
                else if (std::strcmp(kind, "report") == 0) {
                    statusKind = CclsStatusKind::Report;
                }
                else if (std::strcmp(kind, "end") == 0) {
                    statusKind = CclsStatusKind::End;
                }

                if (json_object_has_member(value, "title")) {
                    m_title = json_object_get_string_member(value, "title");
                }
                if (json_object_has_member(value, "message")) {
                    m_message = json_object_get_string_member(value, "message");
                }
                if (json_object_has_member(value, "percentage")) {
                    m_percent = json_object_get_int_member(value, "percentage");
                }

            }
            if (m_statusListener) {
                m_statusListener->notify(m_status, statusKind, m_percent);
            }
        }
    }
}

void
CcLangServer::serverExited()
{
    if (m_statusListener) {
        m_statusListener->serverExited();
    }
}

void
CcLangServer::handleStatus(int id, JsonObject* jsonObj)
{
    if (json_object_has_member(jsonObj, "method")) {
        std::string method = json_object_get_string_member(jsonObj, "method");
        if (method == "window/workDoneProgress/create") {
            decodeStatus(jsonObj);
        }
        else if (method == "$/progress") {
            decodeStatus(jsonObj);
        }
        else if (method == "$ccls/publishSemanticHighlight") {
            //decodeHighlight(jsonObj);
        }
        else if (method == "textDocument/publishDiagnostics") {
            //decodeDiagnostic(jsonObj);
        }

    }
}