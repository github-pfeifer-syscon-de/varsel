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

#include "EventBus.hpp"
#include "ListFactory.hpp"
#include "SourceFactory.hpp"
#include "ExecFactory.hpp"

EventItem::EventItem(Glib::RefPtr<Gio::File> file)
: m_file{file}
{
}

const Glib::RefPtr<Gio::File>
EventItem::getFile() const
{
    return m_file;
}


Glib::RefPtr<Gio::FileInfo>
EventItem::getFileInfo()
{
    if (!m_file->query_exists()) {
        std::cout << "EventItem::getFileInfo for empty file " << m_file->get_path() << std::endl;
        return Glib::RefPtr<Gio::FileInfo>{};
    }
    if (!m_fileInfo) {
        m_fileInfo = m_file->query_info("*", Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
    }
    return m_fileInfo;
}


EventBus::EventBus()
{
    auto listListener = std::make_shared<ListFactory>();
    addListener(listListener);
    auto sourceListener = std::make_shared<SourceFactory>();
    addListener(sourceListener);
    auto execListener = std::make_shared<ExecFactory>();
    addListener(execListener);
}

void
EventBus::addListener(const pEventListener& listener)
{
    m_eventListner.push_back(listener);
}

void
EventBus::distribute(const std::vector<PtrEventItem>& files, Gtk::Menu* gtkMenu)
{
    for (auto& lsnr : m_eventListner) {
        lsnr->notify(files, gtkMenu);
        //if (files.empty()) {
        //    break;     // finish if indicate it was handled
        //}
    }
}
