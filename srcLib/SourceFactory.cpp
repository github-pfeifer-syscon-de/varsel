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

#include "SourceFactory.hpp"


SourceFactory::SourceFactory()
: EventBusListener::EventBusListener()
{
}

void
SourceFactory::notify(const std::shared_ptr<BusEvent>& busEvent)
{
    auto openEvent = std::dynamic_pointer_cast<OpenEvent>(busEvent);
    if (openEvent) {
        std::vector<std::shared_ptr<EventItem>> matchingFiles;
        matchingFiles.reserve(16);
        for (auto item : openEvent->getFiles()) {
            //GtkSourceLanguage* srcLang = SourceFile::getSourceLanguage(item);
            auto filetype = item->getFile()->query_file_type(Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
            if (filetype == Gio::FileType::FILE_TYPE_REGULAR) {
                matchingFiles.push_back(item);
                openEvent->remove(item);
            }
        }
        if (matchingFiles.size() > 0) {
            createSourceWindow(matchingFiles);
        }
    }
}

void
SourceFactory::createSourceWindow(const std::vector<std::shared_ptr<EventItem>>& matchingFiles)
{
    Glib::ustring cmd;
    cmd.reserve(64);
    cmd.append("srcEdit/va_edit");
    for (auto& eventItem : matchingFiles) {
        cmd.append(" ");
        cmd.append(eventItem->getFile()->get_path());
    }
    Glib::spawn_command_line_async(cmd);
}
