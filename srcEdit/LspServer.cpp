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
#include <Log.hpp>
#include <StringUtils.hpp>
#include <cstring>

#include "LspServer.hpp"

LspExit::LspExit()
: RpcMessage::RpcMessage()
{
}

const char*
LspExit::getMethod()
{
    return "exit";
}


LspShutdown::LspShutdown(RpcLaunch* rpcLaunch)
: RpcRequest::RpcRequest()
, m_rpcLaunch{rpcLaunch}
{
}

void
LspShutdown::result(const psc::json::PtrJsonValue& json)
{
    auto exit = std::make_shared<LspExit>();
    m_rpcLaunch->communicate(exit);
}

const char*
LspShutdown::getMethod()
{
    return "shutdown";
}

LspInit::LspInit(const Glib::RefPtr<Gio::File>& dir, LspServer* server)
: RpcRequest::RpcRequest()
, m_dir{dir}
, m_server{server}
{
}

void
LspInit::result(const psc::json::PtrJsonValue& init)
{
    if (m_server) {
        m_server->initDone(init);
    }
}

psc::json::PtrJsonObj
LspInit::create(RpcLaunch* rpcLaunch)
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
    //caps->createObj("textDocument/typeDefinition");   // ? unsure we did get this right
    // caps->createObj("textDocument/signatureHelp");   // ?
    //auto textDef = caps->createObj("textDocument.definition");
    //textDef->set("dynamicRegistration", false);
	//textDef->set("linkSupport", false);

    return obj;
}

const char*
LspInit::getMethod()
{
    return "initialize";
}


LspOpen::LspOpen(const Glib::RefPtr<Gio::File>& file, const Glib::ustring& lang, int version)
: RpcMessage::RpcMessage()
, m_file{file}
, m_lang{lang}
, m_version{version}
{
}

const char*
LspOpen::getMethod()
{
    return "textDocument/didOpen";
}

psc::json::PtrJsonObj
LspOpen::create(RpcLaunch* rpcLaunch)
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
LspOpen::getText()
{
    if (m_text.empty()) {
        g_autoptr(GError) error{nullptr};
        auto channel = g_io_channel_new_file(m_file->get_path().c_str(), "r", &error);
        if (error) {
            psc::log::Log::logAdd(psc::log::Level::Error, psc::fmt::format("Error {} opening {}", error->message, m_file->get_path()));
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
                psc::log::Log::logAdd(psc::log::Level::Error, psc::fmt::format("Error {} reading {}", error->message, m_file->get_path()));
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



LspClose::LspClose(const Glib::RefPtr<Gio::File>& file)
: RpcMessage::RpcMessage()
, m_file{file}
{
}

const char*
LspClose::getMethod()
{
    return "textDocument/didClose";
}

psc::json::PtrJsonObj
LspClose::create(RpcLaunch* rpcLaunch)
{
    auto obj = RpcMessage::create(rpcLaunch);
    auto params = obj->createObj("params");
    auto textDoc = params->createObj("textDocument");
    textDoc->set("uri", m_file->get_uri().c_str());
    return obj;
}

LspLocation::LspLocation()
: LspLocation::LspLocation{0, 0}
{
}

LspLocation::LspLocation(uint32_t line, uint32_t character)
: LspLocation::LspLocation{line, character, line, character}
{
}

LspLocation::LspLocation(uint32_t startLine, uint32_t startCharacter, uint32_t endLine, uint32_t endCharacter)
: m_startLine{startLine}
, m_startCharacter{startCharacter}
, m_endLine{endLine}
, m_endCharacter{endCharacter}
{
}

uint32_t
LspLocation::getStartLine() const
{
    return m_startLine;
}

void
LspLocation::setStartLine(uint32_t startLine)
{
    m_startLine = startLine;
}

uint32_t
LspLocation::getStartCharacter() const
{
    return m_startCharacter;
}

void
LspLocation::setStartCharacter(uint32_t startCharacter)
{
    m_startCharacter = startCharacter;
}

uint32_t
LspLocation::getEndLine() const
{
    return m_endLine;
}

void
LspLocation::setEndLine(uint32_t endLine)
{
    m_endLine = endLine;
}

uint32_t
LspLocation::getEndCharacter() const
{
    return m_endCharacter;
}

void
LspLocation::setEndCharacter(uint32_t endCharacter)
{
    m_endCharacter = endCharacter;
}

Glib::RefPtr<Gio::File>
LspLocation::getUri() const
{
    return m_file;
}

void
LspLocation::setUri(const Glib::RefPtr<Gio::File>& file)
{
    m_file = file;
}

void
LspLocation::toJson(const psc::json::PtrJsonObj& params)
{
    auto textDoc = params->createObj("textDocument");
    textDoc->set("uri", m_file->get_uri().c_str());
    auto pos = params->createObj("position");
    pos->set("line", m_startLine);
    pos->set("character", m_startCharacter);
}

bool
LspLocation::fromJson(const psc::json::PtrJsonValue& json)
{
    psc::json::PtrJsonObj location;
    if (json->isArray()) {
        auto arr = json->getArray();
        if (arr->getSize() > 0) {
            auto val = arr->get(0);
            location = val->getObject();
        }
    }
    else {
        location = json->getObject();
    }
    if (location) {
        auto uri = location->getValue("uri");
        m_file = Gio::File::create_for_uri(uri->getString());
        auto rangeVal = location->getValue("range");
        auto range = rangeVal->getObject();
        auto startVal = range->getValue("start");
        auto start = startVal->getObject();
        auto jstartLine = start->getValue("line");
        m_startLine = static_cast<uint32_t>(jstartLine->getInt());
        auto jstartCharacter = start->getValue("character");
        m_startCharacter = static_cast<uint32_t>(jstartCharacter->getInt());
        auto endVal = range->getValue("end");
        if (endVal && endVal->isObject()) {
            auto end = endVal->getObject();
            auto jendLine = end->getValue("line");
            m_endLine = static_cast<uint32_t>(jendLine->getInt());
            auto jendCharacter = end->getValue("character");
            m_endCharacter = static_cast<uint32_t>(jendCharacter->getInt());
        }
        else {
            m_endLine = m_startLine;
            m_endCharacter = m_startCharacter;
        }
        return true;
    }
    return false;
}


LspDocumentRef::LspDocumentRef(const LspLocation& pos, const Glib::ustring& method)
: RpcRequest::RpcRequest()
, m_pos{pos}
, m_method(method)
{
}

void
LspDocumentRef::result(const psc::json::PtrJsonValue& json)
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
LspDocumentRef::getMethod()
{
    return m_method.c_str();
}

psc::json::PtrJsonObj
LspDocumentRef::create(RpcLaunch* rpcLaunch)
{
    auto obj = RpcRequest::create(rpcLaunch);
    auto params = obj->createObj("params");
    m_pos.toJson(params);
    return obj;
}


LspServer::LspServer(const PtrLspConf& language)
: RpcLaunch::RpcLaunch()
, m_language{language}
{
    run();
}

void
LspServer::shutdown()
{
    //std::string term{"\u0004"}; // use ^d to terminate ?
    //auto bytes_written = m_strmStdin->write(term);
    //kill(child_pid, SIGINT);   // SIGTERM
    auto shut = std::make_shared<LspShutdown>(this);
    communicate(shut);
}

const std::vector<Glib::ustring>
LspServer::buildArgs()
{
    auto args = StringUtils::splitQuoted(m_language->getExecute(), ' ', '"');
    //for (size_t i = 0; i < args.size(); ++i) {
    //    std::cout << "Arg[" << i << "] = \"" << args[i] << "\"" << std::endl;
    //}
    return args;
}

void
LspServer::setStatusListener(LspStatusListener *statusListener)
{
    m_statusListener = statusListener;
}


Glib::ustring
LspServer::getStatus()
{
    return m_status;
}

Glib::ustring
LspServer::getMessage()
{
    return m_message;
}

Glib::ustring
LspServer::getTitle()
{
    return m_title;
}

gint64
LspServer::getPercent()
{
    return m_percent;
}

void
LspServer::decodeStatus(JsonObject* jsonObj)
{
    if (json_object_has_member(jsonObj, "params")) {
        auto params = json_object_get_object_member(jsonObj, "params");
        if (json_object_has_member(params, "token")) {
            m_status = json_object_get_string_member(params, "token");
        }
        if (json_object_has_member(params, "value")) {
            auto value = json_object_get_object_member(params, "value");
            LspStatusKind statusKind{LspStatusKind::None};
            if (json_object_has_member(value, "kind")) {
                auto kind = json_object_get_string_member(value, "kind");
                if (std::strcmp(kind, "begin") == 0) {
                    statusKind = LspStatusKind::Begin;
                }
                else if (std::strcmp(kind, "report") == 0) {
                    statusKind = LspStatusKind::Report;
                }
                else if (std::strcmp(kind, "end") == 0) {
                    statusKind = LspStatusKind::End;
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
LspServer::serverExited()
{
    if (m_statusListener) {
        m_statusListener->serverExited();
    }
}

void
LspServer::initDone(const psc::json::PtrJsonValue& init)
{
    m_init = init->getObject();
}

bool
LspServer::supportsMethod(const Glib::ustring& method)
{
    auto cap = m_init->getValue("capabilities")->getObject();
    if (method == LspDocumentRef::DEFININION_METHOD) {
        return cap->getValue("definitionProvider")->getBool();
    }
    if (method == LspDocumentRef::DECLATATION_METHOD) {
        return cap->getValue("declarationProvider")->getBool();
    }
    return true;    // we can't be sure
}

void
LspServer::handleStatus(int id, JsonObject* jsonObj)
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

PtrLspConf
LspServer::getLanguage()
{
    return m_language;
}