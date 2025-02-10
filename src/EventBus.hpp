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

#pragma once

#include <glibmm.h>
#include <giomm.h>
#include <memory>
#include <list>

class BusEvent
{
public:
    BusEvent() = default;
    explicit BusEvent(const BusEvent& orig) = delete;
    virtual ~BusEvent() = default;

    virtual bool isCompleted() = 0;
};

class EventItem
{
public:
    EventItem(Glib::RefPtr<Gio::File> file);
    explicit EventItem(const EventItem& orig) = delete;
    ~EventItem() = default;

    Glib::RefPtr<Gio::File> getFile();
    Glib::RefPtr<Gio::FileInfo> getFileInfo();

private:
    Glib::RefPtr<Gio::File> m_file;
    Glib::RefPtr<Gio::FileInfo> m_fileInfo;
};

class OpenEvent
: public BusEvent
{
public:
    OpenEvent();
    explicit OpenEvent(const OpenEvent& orig) = delete;
    virtual ~OpenEvent() = default;

    void setContext(const std::vector<Glib::RefPtr<Gio::File>>& files);
    bool isAvail();
    void remove(const std::shared_ptr<EventItem>& item);
    std::vector<std::shared_ptr<EventItem>> getFiles();
    size_t getSize();
    bool isCompleted() override;
private:
    std::vector<std::shared_ptr<EventItem>> m_context;
};

class EventBusListener
{
public:
    EventBusListener() = default;
    explicit EventBusListener(const EventBusListener& orig) = delete;
    virtual ~EventBusListener() = default;

    virtual void notify(const std::shared_ptr<BusEvent>& busEvent) = 0;
};

using pEventListener = std::shared_ptr<EventBusListener>;

class EventBus
{
public:
    EventBus();
    explicit EventBus(const EventBus& orig) = delete;
    virtual ~EventBus() = default;

    void addListener(const pEventListener& listener);
    void send(const std::shared_ptr<BusEvent>& event);
protected:
private:
    std::list<pEventListener> m_eventListner;
};

