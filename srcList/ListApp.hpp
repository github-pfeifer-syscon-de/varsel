/* -*- Mode: C; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2024 rpf
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

#include <gtkmm.h>
#include <memory>
#include <Log.hpp>

#include "EventBus.hpp"

class VarselList;

/*
 * get the application up and running
 *   about and help dialog
 */
class ListApp
: public Gtk::Application
{
public:
    ListApp(int arc, char **argv);
    virtual ~ListApp() = default;

    void on_activate() override;
    void on_startup() override;
    void on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint) override;
    std::shared_ptr<EventBus> getEventBus();
protected:
    VarselList* createVarselWindow();
    VarselList* getOrCreateVarselWindow();

private:
    VarselList* m_varselList{nullptr};
    std::string m_exec;
    std::string get_file(const std::string& name);
    std::shared_ptr<EventBus> m_eventBus;
    void on_action_quit();
    void on_action_about();
    void on_action_help();
    std::shared_ptr<psc::log::Log> m_log;
};

