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

#include <gtkmm.h>
#include <vector>
#include <map>
#include <memory>

#include "LspServer.hpp"
#include "EventBus.hpp"
#include "SourceFile.hpp"
#include "LspConf.hpp"

class EditApp;
class SourceView;
class SourceFile;

class SourceOpen
: public LspOpen
{
public:
    SourceOpen(const Glib::RefPtr<Gio::File>& file, const Glib::ustring& lang, int version, const Glib::ustring& text);
    explicit SourceOpen(const SourceOpen& orig) = delete;
    virtual ~SourceOpen() = default;

    Glib::ustring getText() override;
private:
    Glib::ustring m_text;
};

class SourceDocumentRef
: public LspDocumentRef
{
public:
    SourceDocumentRef(const LspLocation& pos, SourceView* sourceView, const Glib::ustring& method);
    explicit SourceDocumentRef(const SourceDocumentRef& orig) = delete;
    virtual ~SourceDocumentRef() = default;

    void result(const psc::json::PtrJsonValue& json) override;

private:
    SourceView* m_sourceView;
};

class SourceView
: public Gtk::ApplicationWindow
, public LspStatusListener
{
public:
    SourceView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder, EditApp* varselApp);
    explicit SourceView(const SourceView& orig) = delete;
    virtual ~SourceView() = default;

    void applyStyle(const std::string& style, const std::string& fontDesc);
    void newfile(const Glib::VariantBase& val);
    void save(const Glib::VariantBase& val);
    void saveAs(const Glib::VariantBase& val);
    void load(const Glib::VariantBase& val);
    void close(const Glib::VariantBase& val);
    void quit(const Glib::VariantBase& val);
    int showMessage(const Glib::ustring& msg, Gtk::MessageType msgType = Gtk::MessageType::MESSAGE_WARNING);
    void changeLabel(SourceFile* sourceFile);
    EditApp* getApplication()
    {
        return m_application;
    }
    void showFiles(const std::vector<Glib::RefPtr<Gio::File>>& matchingFiles);
    void showDocumentRef(SourceFile* sourceFile, const LspLocation& pos, const Glib::ustring& method);
    void gotoFilePos(const LspLocation& pos);
    void notify(const Glib::ustring& status, LspStatusKind kind, gint64 percent);
    void serverExited();

    static constexpr auto LANGUAGESERVER_ARGS = "languageServerArgs";
protected:
    void createActions();
    PtrSourceFile addFile(const Glib::RefPtr<Gio::File>& item);
    PtrSourceFile findView(const Glib::RefPtr<Gio::File>& item);
    int getIndex(const PtrSourceFile& view);
    void on_hide() override;
private:
    EditApp* m_application;
    Gtk::Notebook* m_notebook{nullptr};
    Gtk::ProgressBar* m_progress{nullptr};
    std::vector<PtrSourceFile> m_files;
    std::map<std::string, PtrLspServer> m_ccLangServers;
    LspConfs m_languages;
};


