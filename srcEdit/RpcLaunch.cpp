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
#include <glibmm.h>
#include <chrono>
#include <vector>
#include <psc_format.hpp>
#include <unistd.h>     // getpid
#include <JsonObj.hpp>
#include <JsonHelper.hpp>
#include <sys/wait.h>
#include <signal.h>
#include <Log.hpp>

#include "RpcLaunch.hpp"

static void
child_watch_cb(GPid     pid,
               gint     status,
               gpointer user_data)
{
    // Free any resources associated with the child here, such as I/O channels
    // on its stdout and stderr FDs. If you have no code to put in the
    // child_watch_cb() callback, you can remove it and the g_child_watch_add()
    // call, but you must also remove the G_SPAWN_DO_NOT_REAP_CHILD flag,
    // otherwise the child process will stay around as a zombie until this
    // process exits.
    g_autoptr(GError) error{nullptr};
    auto stat = g_spawn_check_wait_status(status, &error);
    if (user_data) {
        int exit{0};
        if (error) {
            exit = error->code;
        }
        psc::log::Log::logAdd(psc::log::Level::Debug,
                              [&] {
                                  return psc::fmt::format("wait_stat {} exit {}", stat, exit);
                              });
        auto launcher = static_cast<RpcLaunch*>(user_data);
        launcher->childExited(stat);
    }
    else {
        psc::log::Log::logAdd(psc::log::Level::Error,
                              [&] {
                                  return ("child_watch_cb no user_data");
                              });
    }
    g_spawn_close_pid(pid);
}


static gboolean
cb_out_watch(GIOChannel *channel, GIOCondition cond, gpointer user_data)
{
    if (cond == G_IO_HUP) {
        g_io_channel_shutdown(channel, false, nullptr);
        g_io_channel_unref(channel);
        return false;
    }
    if (user_data) {
        auto reader = static_cast<LineReader*>(user_data);
        reader->ready(channel);
    }
    else {
        psc::log::Log::logAdd(psc::log::Level::Error,
                      [&] {
                          return ("cb_out_watch output no user_data!");
                      });
    }

    return true;
}



LineReader::LineReader(gint out, ReaderListener* readerListener)
: m_out{out}
, m_readerListener{readerListener}
{
    m_channel = g_io_channel_unix_new(m_out);   // channels offer some convenient way to read stuff
    g_autoptr(GError) error{nullptr};
    g_io_channel_set_encoding(m_channel, nullptr, &error);
    if (error) {
        psc::log::Log::logAdd(psc::log::Level::Error,
                      [&] {
                          return psc::fmt::format("error {} set enconding", error->message);
                      });
    }
    /* Add watches to channels (read callback) */
    g_io_add_watch(m_channel, static_cast<GIOCondition>(G_IO_IN | G_IO_HUP), cb_out_watch, this);   // (GIOFunc)
}

LineReader::~LineReader()
{
    // the channel is handeled during Hup
    close(m_out);
}

void
LineReader::ready(GIOChannel *channel)
{
    g_autoptr(GError) error{nullptr};
    gchar *string{nullptr};
    gsize size{0};
    g_io_channel_read_line(channel, &string, &size, NULL, &error);
    if (error) {
        psc::log::Log::logAdd(psc::log::Level::Error,
                      [&] {
                          return psc::fmt::format("error {} reading", error->message);
                      });
    }
    else {
        m_readerListener->notify(true, string);
        g_free(string);
    }
}

// kept for reference, this is the classic "select/read" method (needs to be called on a separate thread)
void
LineReader::reads()
{
    std::array<char, 8192> outArr;
    fd_set fdset;
    FD_ZERO(&fdset); /* clear the set */
    FD_SET(m_out, &fdset); /* add our file descriptor to the set */

    fd_set fderr;

    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 50000000ul;

    sigset_t sigset;
    sigfillset(&sigset);    // any signal

    while (true) { // !strm->is_closed()
        //std::cout << "Readers::read fd " << m_out << std::endl;
        FD_ZERO(&fderr); /* clear the set */
        int rv = pselect(1, &fdset, NULL, &fderr, &timeout, NULL);
        if (rv == -1) {
            std::cout << "error select " <<  m_out << std::endl; /* an error accured */
            break;
        }
        else if (rv == 0) {
            //std::cout << "Reader::read timeout " << m_out << std::endl; /* a timeout occured */
        }
        else {
            if (FD_ISSET(m_out, &fderr)) {
                std::cout << "err set!" << m_out << std::endl;
            }
            int sizeRead = read(m_out, outArr.data(), outArr.size()); /* there was data to read */
            //std::cout << "Readers::read read " << sizeRead << std::endl;
            //gsize sizeStd = strm->read(outArr.data(), outArr.size());
            if (sizeRead > 0) {
                std::cout << "Reader::read got " << sizeRead << std::endl;
                std::string data(outArr.data(), sizeRead);    // use std::string as this may contain utf-8
                //m_str += data;
            }
            else {
                if (sizeRead < 0) {
                    break;
                }
                //using namespace std::chrono_literals; // ns, us, ms, s, h, etc.
                //std::this_thread::sleep_for(50ms);
            }
        }
    }
    //m_done = true;
    std::cout << "Readers::reads done " << m_out << std::endl;
}

RpcReader::RpcReader(gint out, ReaderListener* readerListener)
: LineReader(out, readerListener)
{
}

void
RpcReader::ready(GIOChannel *channel)
{
    int contentLen{0};
    while (true) {      // first we are in header mode
        g_autoptr(GError) error{nullptr};
        gchar *string{nullptr};
        gsize size{0};
        g_io_channel_read_line(channel, &string, &size, NULL, &error);
        if (error) {
            psc::log::Log::logAdd(psc::log::Level::Error,
                      [&] {
                          return psc::fmt::format("error {} reading header", error->message);
                      });
        }
        else {
            uint32_t n = 0;
            while (size > 0
                && n < 2
                && (string[size-1] == '\n' || string[size-1] == '\r')) {
                string[size-1] = '\0';
                --size;
                ++n;
            }
            auto contentLenPos = strstr(string, CONTENT_LENGTH);
            if (contentLenPos) {
                try {
                    std::size_t end;
                    contentLen = std::stoi(contentLenPos + std::char_traits<char>::length(CONTENT_LENGTH), &end);
                }
                catch (const std::exception &exc) {
                    psc::log::Log::logAdd(psc::log::Level::Error,
                        [&] {
                            return psc::fmt::format("Error {} processing content-length", exc.what());
                        });
                }
            }
            g_free(string);
            if (size == 0) {
                break;      // we are out of header
            }
        }
    }
    if (contentLen > 0) {
        auto buf = std::vector<gchar>(contentLen + 1);
        g_autoptr(GError) error{nullptr};
        gsize size{0};
        g_io_channel_read_chars(channel, &buf[0], contentLen, &size, &error);
        if (error) {
            psc::log::Log::logAdd(psc::log::Level::Error,
                      [&] {
                          return psc::fmt::format("error {} reading content", error->message);
                      });
        }
        else {
            if (size > 0) {
                buf[size] = '\0';
                if (m_readerListener) {
                    m_readerListener->notify(false, buf.data());
                }
            }
        }
    }
}


RpcLaunch::RpcLaunch()
{
}

void
RpcLaunch::run()
{
    const std::vector<std::string> args = buildArgs();
    std::vector<gchar *> argv;
    argv.reserve(args.size() + 1);
    for (uint32_t i = 0; i <= args.size(); ++i) {
        if (i < args.size()) {
            //std::cout << "Launch::Launch arg[" << i << "] " << args[i] << std::endl;
            argv.push_back(const_cast<gchar *>(args[i].c_str()));
        }
        else {
            argv.push_back(nullptr);
        }
    }

    g_autoptr(GError) error{nullptr};
    // Spawn child process.
    g_spawn_async_with_pipes(nullptr    // workingdir
                            , &argv[0]  // arguments
                            , nullptr   // env
                            , static_cast<GSpawnFlags>(G_SPAWN_DO_NOT_REAP_CHILD)   //  | G_SPAWN_SEARCH_PATH
                            , nullptr   // child_setup
                            , this      // user data
                            , &child_pid
                            , &child_stdin, &child_stdout, &child_stderr
                            , &error);
    if (error) {
        psc::log::Log::logAdd(psc::log::Level::Error,
                  [&] {
                      return psc::fmt::format("error {} spawning child failed", error->message);
                  });
        return;
    }
    psc::log::Log::logAdd(psc::log::Level::Debug,
              [&] {
                  return psc::fmt::format("spawning child {} in {} out {} err {}", child_pid, child_stdin, child_stdout, child_stderr);
              });
    // Add a child watch function which will be called when the child process
    // exits.
    g_child_watch_add(child_pid, child_watch_cb, this);
    // You could watch for output on @child_stdout and @child_stderr using
    // #GUnixInputStream or #GIOChannel here.
    //using namespace std::chrono_literals; // ns, us, ms, s, h, etc.

    m_readerStd = std::make_shared<RpcReader>(child_stdout, this);
    m_readerErr = std::make_shared<LineReader>(child_stderr, this);
    m_strmStdin = Gio::UnixOutputStream::create(child_stdin, true);
}

void
RpcLaunch::notify(bool err, const gchar* string)
{
    if (string) {
        if (!err) {
            psc::log::Log::logAdd(psc::log::Level::Debug,
                [&] {
                    return psc::fmt::format("notify {} {}", (err ? "err" : "std"), string);
                });
            try {
                JsonHelper jsonHelper;
                jsonHelper.load_data(string);
                auto root = jsonHelper.get_root_object();
                if (root
                 && json_object_has_member(root, "id")) {
                    int id = json_object_get_int_member(root, "id");
                    auto responseIter = m_reponseMap.find(id);
                    if (responseIter != m_reponseMap.end()) {
                        auto& response = *responseIter;
                        if (json_object_has_member(root, "result"))  {
                            auto value = std::make_shared<psc::json::JsonValue>(json_object_get_member(root, "result"));
                            response.second->result(value);
                        }
                        else if (json_object_has_member(root, "error"))  {
                            auto error = json_object_get_object_member(root, "error");
                            response.second->error(error);
                        }
                        responseIter = m_reponseMap.erase(responseIter);
                    }
                    else {
                        psc::log::Log::logAdd(psc::log::Level::Warn,
                            [&] {
                                return psc::fmt::format("no response for id  {}", id);
                            });
                    }
                }
            }
            catch (const JsonException& exc) {
                psc::log::Log::logAdd(psc::log::Level::Error,
                    [&] {
                        return psc::fmt::format("error {} parsing json", exc.what());
                    });
            }
            m_active = false;
            if (m_queue.size() > 0) {
                auto next = m_queue.front();
                m_queue.pop();
                doCommunicate(next);
            }
        }
    }
}

int RpcLaunch::getNextReqId()
{
    return ++m_reqId;
}


void
RpcLaunch::communicate(const std::shared_ptr<RpcMessage>& message)
{
    if (m_active) {
        m_queue.push(message);
    }
    else {
        doCommunicate(message);
    }
}



void
RpcLaunch::doCommunicate(const std::shared_ptr<RpcMessage>& message)
{
    auto json = message->create(this);
    auto response = std::dynamic_pointer_cast<RpcRequest>(message);
    if (response) {
        m_active = true;    // only in this case we know when server is done
        m_reponseMap.insert(std::make_pair(response->getReqId(), response));
    }
    Glib::ustring request = json->generate();
    psc::log::Log::logAdd(psc::log::Level::Debug,
        [&] {
            std::string info;
            if (request.length() > 255ul) {
                info = request.substr(0, 255ul) + "...";
            }
            else {
                info = request;
            }
            return psc::fmt::format("communicate {} ", info);
        });
    Glib::ustring header = psc::fmt::format("Content-Length: {}\r\n\r\n", request.bytes());
    request = header + request;
    auto size = m_strmStdin->write(request);
    //std::cout << "RpcLaunch::communicate "
    //          << " size " << size << std::endl;

}

RpcMessage::RpcMessage()
{
}

std::shared_ptr<psc::json::JsonObj>
RpcMessage::create(RpcLaunch* rpcLaunch)
{
    auto obj = std::make_shared<psc::json::JsonObj>();
    obj->set("jsonrpc", "2.0");
    obj->set("method", getMethod());
    return obj;
}

RpcRequest::RpcRequest()
{
}

std::shared_ptr<psc::json::JsonObj>
RpcRequest::create(RpcLaunch* rpcLaunch)
{
    m_reqId = rpcLaunch->getNextReqId();
    auto obj = RpcMessage::create(rpcLaunch);
    obj->set("id", m_reqId);
    return obj;
}

void
RpcRequest::error(JsonObject* error)
{
    psc::log::Log::logAdd(psc::log::Level::Error,
        [&] {
            psc::json::JsonObj err(error);
            return psc::fmt::format("Request error {}", err.generate(2));
        });
}


int
RpcRequest::getReqId()
{
    return m_reqId;
}


void
RpcLaunch::childExited(bool stat)
{
    psc::log::Log::logAdd(psc::log::Level::Debug,
        [&] {
            return psc::fmt::format("childExited {}", (stat ? "normal" : "abnormaly" ));
        });
    m_stat = stat;
}

bool
RpcLaunch::getResult()
{
    return m_stat;
}

