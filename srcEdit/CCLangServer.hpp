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

#include "RpcLaunch.hpp"

// see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/
//   for protocol definitions
class CclsExit
: public RpcMessage
{
public:
    CclsExit();
    explicit CclsExit(const CclsExit& orig) = delete;
    virtual ~CclsExit() = default;

    const char* getMethod() override;
};


class CclsShutdown
: public RpcRequest
{
public:
    CclsShutdown(RpcLaunch* rpcLaunch);
    explicit CclsShutdown(const CclsShutdown& orig) = delete;
    virtual ~CclsShutdown() = default;

    virtual void result(const std::shared_ptr<psc::json::JsonValue>& json) override;
    const char* getMethod() override;
private:
    RpcLaunch* m_rpcLaunch;
};

class CclsInit
: public RpcRequest
{
public:
    CclsInit(const Glib::RefPtr<Gio::File>& dir);
    explicit CclsInit(const CclsInit& orig) = delete;
    virtual ~CclsInit() = default;

    virtual void result(const std::shared_ptr<psc::json::JsonValue>& json);
    virtual const char* getMethod();
    std::shared_ptr<psc::json::JsonObj> create(RpcLaunch* rpcLaunch) override;

private:
    Glib::RefPtr<Gio::File> m_dir;
};


class CclsOpen
: public RpcMessage
{
public:
    CclsOpen(const Glib::RefPtr<Gio::File>& file, const Glib::ustring& lang, int version);
    explicit CclsOpen(const CclsOpen& orig) = delete;
    virtual ~CclsOpen() = default;

    virtual const char* getMethod();
    std::shared_ptr<psc::json::JsonObj> create(RpcLaunch* rpcLaunch) override;
    virtual Glib::ustring getText();
protected:

private:
    Glib::RefPtr<Gio::File> m_file;
    Glib::ustring m_lang;
    int m_version;
    Glib::ustring m_text;
};

class CclsClose
: public RpcMessage
{
public:
    CclsClose(const Glib::RefPtr<Gio::File>& file);
    explicit CclsClose(const CclsClose& orig) = delete;
    virtual ~CclsClose() = default;

    virtual const char* getMethod();
    std::shared_ptr<psc::json::JsonObj> create(RpcLaunch* rpcLaunch) override;
private:
    Glib::RefPtr<Gio::File> m_file;
};

typedef struct
{
    int line;
    int character;
} TextPos;

class CclsDocumentRef
: public RpcRequest
{
public:
    CclsDocumentRef(const Glib::RefPtr<Gio::File>& file, const TextPos& pos, const Glib::ustring& method);
    explicit CclsDocumentRef(const CclsDocumentRef& orig) = delete;
    virtual ~CclsDocumentRef() = default;

    virtual void result(const std::shared_ptr<psc::json::JsonValue>& json) override;
    virtual const char* getMethod();
    std::shared_ptr<psc::json::JsonObj> create(RpcLaunch* rpcLaunch) override;

    static constexpr auto DEFININION_METHOD = "textDocument/definition";
    static constexpr auto DECLATATION_METHOD = "textDocument/declaration";
private:
    Glib::RefPtr<Gio::File> m_file;
    TextPos m_pos;
    Glib::ustring m_method;
};

enum class CclsStatusKind
{
    None,
    Begin,
    Report,
    End
};

class CclsStatusListener
{
public:
    virtual void notify(const Glib::ustring& status, CclsStatusKind kind, gint64 percent) = 0;
    virtual void serverExited() = 0;
};

class CcLangServer
: public RpcLaunch
{
public:
    CcLangServer(const std::string& launch);
    explicit CcLangServer(const CcLangServer& orig) = delete;
    virtual ~CcLangServer() = default;

    const std::vector<Glib::ustring> buildArgs() override;

    void shutdown();
    void handleStatus(int id, JsonObject* jsonObj) override;

    void setStatusListener(CclsStatusListener *statusListener);
    Glib::ustring getStatus();
    Glib::ustring getMessage();
    Glib::ustring getTitle();
    gint64 getPercent();
protected:
    void decodeStatus(JsonObject* jsonObj);
    void serverExited() override;
private:
    std::string m_launch;
    Glib::ustring m_status;
    Glib::ustring m_message;
    Glib::ustring m_title;
    gint64 m_percent{};
    CclsStatusListener* m_statusListener{nullptr};
};

using PtrCcLangServer = std::shared_ptr<CcLangServer>;

