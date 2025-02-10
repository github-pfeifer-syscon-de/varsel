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

#include <gtksourceview/gtksource.h>
#include <gtkmm.h>
#include <list>

#include "EventBus.hpp"

class VarselApp;

class SourceFile
{
public:
    SourceFile(VarselApp* application);
    virtual ~SourceFile() = default;

    Gtk::Widget* load(std::shared_ptr<EventItem>& eventItem);
    void applyStyle(const std::string& style, const std::string& fontDesc);
    Glib::ustring getLabel();
    static GtkSourceLanguage* getSourceLanguage(const std::shared_ptr<EventItem>& item);

private:
    VarselApp* m_application;
    GtkSourceBuffer* m_buffer{nullptr};
    GtkSourceView* m_sourceView{nullptr};
    Glib::ustring m_label;
};

class SourceView
: public Gtk::Window
{
public:
    SourceView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder, VarselApp* varselApp, const std::vector<std::shared_ptr<EventItem>>& matchingFiles);
    explicit SourceView(const SourceView& orig) = delete;
    virtual ~SourceView() = default;

    void applyStyle(const std::string& style, const std::string& fontDesc);
protected:

private:
    VarselApp* m_application;
    Gtk::Notebook* m_notebook{nullptr};
    std::list<SourceFile> m_files;
};



class SourceFactory
: public EventBusListener
{
public:
    SourceFactory(VarselApp* varselApp);
    explicit SourceFactory(const SourceFactory& listener) = delete;
    virtual ~SourceFactory() = default;

    SourceView* createSourceWindow(const std::vector<std::shared_ptr<EventItem>>& matchingFiles);
    void notify(const std::shared_ptr<BusEvent>& busEvent) override;
private:
    VarselApp* m_varselApp;
};
