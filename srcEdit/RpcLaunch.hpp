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

#include <giomm.h>
#include <future>
#include <memory>
#include <JsonHelper.hpp>
#include <JsonObj.hpp>
#include <queue>
#include <atomic>

class RpcLaunch;

class ReaderListener
{
public:
    ReaderListener() = default;
    virtual ~ReaderListener() = default;

    virtual void notify(bool err, const gchar* string) = 0;
};

// works also as Notification
class RpcMessage
{
public:
     RpcMessage();
     virtual ~RpcMessage() = default;

     virtual std::shared_ptr<psc::json::JsonObj> create(RpcLaunch* rpcLaunch);
     virtual const char* getMethod() = 0;
};

class RpcRequest
: public RpcMessage
{
public:
    RpcRequest();

    virtual std::shared_ptr<psc::json::JsonObj> create(RpcLaunch* rpcLaunch) override;
    virtual void result(const std::shared_ptr<psc::json::JsonValue>& result) = 0;
    virtual void error(JsonObject* error);
    int getReqId();
protected:
    int m_reqId{0};
};


class LineReader
{
public:
    LineReader(gint out, ReaderListener* readerListener);
    explicit LineReader(const LineReader& orig) = delete;
    virtual ~LineReader();

    virtual void ready(GIOChannel *channel);
protected:
    void reads();

    gint m_out;
    GIOChannel* m_channel;
    ReaderListener* m_readerListener;
};


class RpcReader
: public LineReader
{
public:
    RpcReader(gint out, ReaderListener* readerListener);
    explicit RpcReader(const RpcReader& orig) = delete;
    virtual ~RpcReader() = default;

    static constexpr auto CONTENT_LENGTH = "Content-Length:";
    void ready(GIOChannel *channel) override;
};



class RpcLaunch
: public ReaderListener
{
public:
    RpcLaunch();
    explicit RpcLaunch(const RpcLaunch& orig) = delete;
    virtual ~RpcLaunch() = default;

    void run();
    virtual const std::vector<Glib::ustring> buildArgs() = 0;
    virtual void handleStatus(int id, JsonObject* jsonObj);
    int getNextReqId();
    void communicate(const std::shared_ptr<RpcMessage>& message);
    void childExited(bool stat);
    bool getResult();
    void initDone(JsonObject* json);
    GPid getChildPid() {
        return child_pid;
     }
protected:
    void doCommunicate(const std::shared_ptr<RpcMessage>& message);

    void notify(bool err, const gchar* string) override;
    virtual void serverExited();

private:
    std::shared_ptr<RpcReader> m_readerStd;
    std::shared_ptr<LineReader> m_readerErr;
    Glib::RefPtr<Gio::UnixOutputStream> m_strmStdin;
    gint child_stdout, child_stderr, child_stdin;
    GPid child_pid;
    bool m_stat{false};
    uint32_t m_reqId{0};
    Glib::RefPtr<Gio::File> m_projDir;
    std::map<gint64, std::shared_ptr<RpcRequest>> m_reponseMap;
    std::queue<std::shared_ptr<RpcMessage>> m_queue;
    std::atomic<bool> m_active{false};

};