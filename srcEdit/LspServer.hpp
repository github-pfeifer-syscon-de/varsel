/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4;  coding: utf-8; -*-  */
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

#pragma once

#include <memory>
#include <JsonObj.hpp>

#include "LspConf.hpp"
#include "RpcLaunch.hpp"


class LspServer;

// see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/
//   for protocol definitions
class LspExit
: public RpcMessage
{
public:
    LspExit();
    explicit LspExit(const LspExit& orig) = delete;
    virtual ~LspExit() = default;

    const char* getMethod() override;
};


class LspShutdown
: public RpcRequest
{
public:
    LspShutdown(RpcLaunch* rpcLaunch);
    explicit LspShutdown(const LspShutdown& orig) = delete;
    virtual ~LspShutdown() = default;

    virtual void result(const psc::json::PtrJsonValue& json) override;
    const char* getMethod() override;
private:
    RpcLaunch* m_rpcLaunch;
};

class LspInit
: public RpcRequest
{
public:
    LspInit(const Glib::RefPtr<Gio::File>& dir, LspServer* server);
    explicit LspInit(const LspInit& orig) = delete;
    virtual ~LspInit() = default;

    virtual void result(const psc::json::PtrJsonValue& json);
    virtual const char* getMethod();
    psc::json::PtrJsonObj create(RpcLaunch* rpcLaunch) override;

private:
    Glib::RefPtr<Gio::File> m_dir;
    LspServer* m_server;
};


class LspOpen
: public RpcMessage
{
public:
    LspOpen(const Glib::RefPtr<Gio::File>& file, const Glib::ustring& lang, int version);
    explicit LspOpen(const LspOpen& orig) = delete;
    virtual ~LspOpen() = default;

    virtual const char* getMethod();
    psc::json::PtrJsonObj create(RpcLaunch* rpcLaunch) override;
    virtual Glib::ustring getText();
protected:

private:
    Glib::RefPtr<Gio::File> m_file;
    Glib::ustring m_lang;
    int m_version;
    Glib::ustring m_text;
};

class LspClose
: public RpcMessage
{
public:
    LspClose(const Glib::RefPtr<Gio::File>& file);
    explicit LspClose(const LspClose& orig) = delete;
    virtual ~LspClose() = default;

    virtual const char* getMethod();
    psc::json::PtrJsonObj create(RpcLaunch* rpcLaunch) override;
private:
    Glib::RefPtr<Gio::File> m_file;
};

class LspLocation
{
public:
    LspLocation();
    LspLocation(uint32_t line, uint32_t character);
    LspLocation(uint32_t startLine, uint32_t startCharacter, uint32_t endLine, uint32_t endCharacter);
    LspLocation(const LspLocation& orig) = default;
    virtual ~LspLocation() = default;

    uint32_t getStartLine() const;
    void setStartLine(uint32_t startLine);
    uint32_t getStartCharacter() const;
    void setStartCharacter(uint32_t startCharacter);
    uint32_t getEndLine() const;
    void setEndLine(uint32_t endLine);
    uint32_t getEndCharacter() const;
    void setEndCharacter(uint32_t endCharacter);
    Glib::RefPtr<Gio::File> getUri() const;
    void setUri(const Glib::RefPtr<Gio::File>& file);

    void toJson(const psc::json::PtrJsonObj& param);
    bool fromJson(const psc::json::PtrJsonValue& json);
private:
    uint32_t m_startLine;
    uint32_t m_startCharacter;
    uint32_t m_endLine;
    uint32_t m_endCharacter;
    Glib::RefPtr<Gio::File> m_file;
} ;

class LspDocumentRef
: public RpcRequest
{
public:
    LspDocumentRef(const LspLocation& pos, const Glib::ustring& method);
    explicit LspDocumentRef(const LspDocumentRef& orig) = delete;
    virtual ~LspDocumentRef() = default;

    virtual void result(const psc::json::PtrJsonValue& json) override;
    virtual const char* getMethod();
    psc::json::PtrJsonObj create(RpcLaunch* rpcLaunch) override;

    static constexpr auto DEFININION_METHOD = "textDocument/definition";
    static constexpr auto DECLATATION_METHOD = "textDocument/declaration";
private:
    LspLocation m_pos;
    Glib::ustring m_method;
};

enum class CclsStatusKind
{
    None,
    Begin,
    Report,
    End
};

class LspStatusListener
{
public:
    virtual void notify(const Glib::ustring& status, CclsStatusKind kind, gint64 percent) = 0;
    virtual void serverExited() = 0;
};

class LspServer
: public RpcLaunch
{
public:
    LspServer(const PtrLanguage& language);
    explicit LspServer(const LspServer& orig) = delete;
    virtual ~LspServer() = default;

    const std::vector<Glib::ustring> buildArgs() override;

    void shutdown();
    void handleStatus(int id, JsonObject* jsonObj) override;
    void initDone(const psc::json::PtrJsonValue& init);
    bool supportsMethod(const Glib::ustring& method);
    void setStatusListener(LspStatusListener *statusListener);
    Glib::ustring getStatus();
    Glib::ustring getMessage();
    Glib::ustring getTitle();
    gint64 getPercent();
    PtrLanguage getLanguage();
protected:
    void decodeStatus(JsonObject* jsonObj);
    void serverExited() override;
private:
    PtrLanguage m_language;
    Glib::ustring m_status;
    Glib::ustring m_message;
    Glib::ustring m_title;
    gint64 m_percent{};
    LspStatusListener* m_statusListener{nullptr};
    psc::json::PtrJsonObj m_init;
};

using PtrLspServer = std::shared_ptr<LspServer>;

