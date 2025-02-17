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
#include <KeyConfig.hpp>
#include <Log.hpp>

#include "VarselWin.hpp"
#include "SourceView.hpp"
#include "EventBus.hpp"


/*
 * get the application up and running
 *   about and help dialog
 */
class VarselApp
: public Gtk::Application
{
public:
    VarselApp(int arc, char **argv);
    virtual ~VarselApp() = default;

    void on_activate() override;
    void on_startup() override;
    void on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint) override;
    std::shared_ptr<KeyConfig> getKeyFile();
    void save_config();
    std::shared_ptr<EventBus> getEventBus();
protected:
    VarselWin* createVarselWindow();
    VarselWin* getOrCreateVarselWindow();

private:
    VarselWin* m_varselWindow{nullptr};
    std::string m_exec;
    std::shared_ptr<KeyConfig> m_config;
    std::string get_file(const std::string& name);
    void on_action_quit();
    void on_action_about();
    void on_action_help();
    std::shared_ptr<EventBus> m_eventBus;
    std::shared_ptr<psc::log::Log> m_log;
};

