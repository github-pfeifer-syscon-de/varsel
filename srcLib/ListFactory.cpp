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


#include "ListFactory.hpp"
#include "Archiv.hpp"



ListFactory::ListFactory()
: EventBusListener::EventBusListener()
{
}


void
ListFactory::notify(const std::shared_ptr<BusEvent>& busEvent)
{
    auto openEvent = std::dynamic_pointer_cast<OpenEvent>(busEvent);
    if (openEvent) {
        for (auto& item : openEvent->getFiles()) {
            auto file = item->getFile();
            auto type = file->query_file_type();
            std::cout << "ListFactory::notify testing " << item->getFile()->get_path() << std::endl;
            auto basename = file->get_basename();
            //std::cout << "basename " << basename << std::endl;
            if (type == Gio::FileType::FILE_TYPE_DIRECTORY) {
                createListWindow(item);     // this may either be directory or a git repos
                openEvent->remove(item);
            }
            if (type == Gio::FileType::FILE_TYPE_REGULAR) {
                Archiv archiv{file};
                if (archiv.canRead()) {
                    createListWindow(item);
                    openEvent->remove(item);
                }
            }
        }
    }
}


void
ListFactory::createListWindow(const std::shared_ptr<EventItem>& eventItem)
{
    Glib::ustring cmd;
    cmd.reserve(64);
    cmd.append("srcList/va_list");
    cmd.append(" ");
    cmd.append(eventItem->getFile()->get_path());
    Glib::spawn_command_line_async(cmd);
}