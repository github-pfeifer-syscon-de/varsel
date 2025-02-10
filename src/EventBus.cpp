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


#include "EventBus.hpp"

EventItem::EventItem(Glib::RefPtr<Gio::File> file)
: m_file{file}
{

}

Glib::RefPtr<Gio::File>
EventItem::getFile()
{
    return m_file;
}


Glib::RefPtr<Gio::FileInfo>
EventItem::getFileInfo()
{
    if (!m_fileInfo) {
        m_fileInfo = m_file->query_info("*", Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
    }
    return m_fileInfo;
}


OpenEvent::OpenEvent()
: BusEvent::BusEvent()
{
}

void
OpenEvent::setContext(const std::vector<Glib::RefPtr<Gio::File>>& files)
{
    m_context.reserve(files.size());
    for (auto& file : files) {
        auto item = std::make_shared<EventItem>(file);
        m_context.emplace_back(std::move(item));
    }
}

bool OpenEvent::isAvail()
{
    return true;    // find a working method
}

void
OpenEvent::remove(const std::shared_ptr<EventItem>& item)
{
    for (auto iter = m_context.begin(); iter != m_context.end(); ) {
        auto& iitem = *iter;
        if (iitem == item) {
            iter = m_context.erase(iter);
        }
        else {
            ++iter;
        }
    }
}

std::vector<std::shared_ptr<EventItem>>
OpenEvent::getFiles()
{
    return m_context;
}

size_t
OpenEvent::getSize()
{
    return m_context.size();
}

bool
OpenEvent::isCompleted()
{
    return m_context.empty();
}

EventBus::EventBus()
{
}

void
EventBus::addListener(const pEventListener& listener)
{
    m_eventListner.push_back(listener);
}

void
EventBus::send(const std::shared_ptr<BusEvent>& event)
{
    for (auto& lsnr : m_eventListner) {
        lsnr->notify(event);
        if (event->isCompleted()) {
            break;     // finish if indicate it was handled
        }
    }
}