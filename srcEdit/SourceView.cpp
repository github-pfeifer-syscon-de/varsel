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
#include <psc_i18n.hpp>
#include <Log.hpp>
#include <psc_format.hpp>

#include "SourceView.hpp"
#include "EditApp.hpp"
#include "PrefSourceView.hpp"

SourceOpen::SourceOpen(const Glib::RefPtr<Gio::File>& file, const Glib::ustring& lang, int version, const Glib::ustring& text)
: LspOpen::LspOpen(file, lang, version)
, m_text{text}
{
}

Glib::ustring
SourceOpen::getText()
{
    return m_text;
}

SourceDocumentRef::SourceDocumentRef(const LspLocation& pos, SourceView* sourceView, const Glib::ustring& method)
: LspDocumentRef::LspDocumentRef(pos, method)
, m_sourceView{sourceView}
{
}

void
SourceDocumentRef::result(const psc::json::PtrJsonValue& json)
{
    LspLocation pos;
    if (pos.fromJson(json)) {
        m_sourceView->gotoFilePos(pos);
        return;
    }
    // this is somewhat common case
    //   e.g. definiton is a symetric method as it works from cpp&hpp
    //   but declaration works only from cpp, returns empty array from hpp
    //psc::log::Log::logAdd(psc::log::Level::Error,
    //                      psc::fmt::format("Empty/unknown structure {} for definition!", struc));
    std::cout << "Empty/unknown structure "<< json->generate(2) << " for definition!" << std::endl;
}

SourceView::SourceView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder, EditApp* varselApp)
: Gtk::ApplicationWindow(cobject)
, m_application{varselApp}
{
    set_title(_("Source"));
    auto pix = Gdk::Pixbuf::create_from_resource(varselApp->get_resource_base_path() + "/va_edit.png");
    set_icon(pix);
    createActions();
    refBuilder->get_widget("notebook", m_notebook);
    refBuilder->get_widget("progress", m_progress);
    set_default_size(640, 480);
    m_languages.readConfig(m_application->getKeyFile());
}

PtrSourceFile
SourceView::addFile(const Glib::RefPtr<Gio::File>& item)
{
    auto sourceFile = std::make_shared<SourceFile>(this);
    auto widget = sourceFile->buildSourceView(item);
    m_notebook->append_page(*widget, sourceFile->getLabel());

    auto settings = m_application->getKeyFile();
    Glib::ustring fontDesc;
    if (!settings->getBoolean(PrefSourceView::CONFIG_SRCVIEW_GRP, PrefSourceView::DEFAULT_FONT, true)
     && settings->hasKey(PrefSourceView::CONFIG_SRCVIEW_GRP, PrefSourceView::CONFIG_FONT)) {
        fontDesc = settings->getString(PrefSourceView::CONFIG_SRCVIEW_GRP, PrefSourceView::CONFIG_FONT);
    }
    Glib::ustring style;
    if (settings->hasKey(PrefSourceView::CONFIG_SRCVIEW_GRP, PrefSourceView::SOURCE_STYLE)) {
        style = settings->getString(PrefSourceView::CONFIG_SRCVIEW_GRP, PrefSourceView::SOURCE_STYLE);
    }
    sourceFile->applyStyle(style, fontDesc);

    widget->show_all();
    if (item) { // only if valid file
        // create language server early as init takes some time
        // at the moment we are limited to one language server per source dir
        auto dir = item->get_parent();
        auto langServIt = m_ccLangServers.find(dir->get_path());
        if (langServIt == m_ccLangServers.end()) {
            auto lang = m_languages.getLanguage(item);
            if (lang) {
                auto lspServer = std::make_shared<LspServer>(lang);
                lspServer->setStatusListener(this);
                auto init = std::make_shared<LspInit>(dir, lspServer.get());
                lspServer->communicate(init);
                m_ccLangServers.insert(std::make_pair(dir->get_path(), lspServer));
            }
            else {
                std::cout << "No language support for " << item->get_basename() << std::endl;
            }
        }
    }
    m_files.push_back(sourceFile);
    return sourceFile;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"   // switches are incomplete by intention

void
SourceView::notify(const Glib::ustring& status, LspStatusKind kind, gint64 percent)
{
    double value = 0.0;
    Glib::ustring stat;
    switch (kind) {
    case LspStatusKind::Begin:
        value = 0.0;
        stat = Glib::ustring(_("Started ")) + status;
        break;
    case LspStatusKind::Report:
        value = static_cast<double>(percent) / 100.0;
        stat = status;
        break;
    case LspStatusKind::End:
        value = 1.0;
        stat = Glib::ustring(_("Completed ")) + status;
        break;
    }
    m_progress->set_fraction(value);
    m_progress->set_text(stat);
}

#pragma GCC diagnostic pop

void
SourceView::serverExited()
{
    m_progress->set_fraction(0.0);
    m_progress->set_text(_("Server exited."));
}


PtrSourceFile
SourceView::findView(const Glib::RefPtr<Gio::File>& file)
{
    PtrSourceFile src;
    for (auto& view : m_files) {
        if (view->getFile()->get_path() == file->get_path()) {
            src = view;
            break;
        }
    }
    return src;
}

int
SourceView::getIndex(const PtrSourceFile& view)
{
    for (size_t i = 0; i < m_files.size(); ++i) {
        if (view == m_files[i]) {
            return i;
        }
    }
    return 0;
}


void
SourceView::showFiles(const std::vector<Glib::RefPtr<Gio::File>>& matchingFiles)
{
    for (size_t n = 0; n < matchingFiles.size(); ++n) {
        auto item = matchingFiles[n];
        addFile(item);
    }
    //applyStyle(style, fontDesc);
}

void
SourceView::createActions()
{
    auto newAction = add_action("new");
    newAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::newfile));
    auto saveAction = add_action("save");
    saveAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::save));
    auto saveAsAction = add_action("saveAs");
    saveAsAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::saveAs));
    auto loadAction = add_action("load");
    loadAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::load));
    auto closeAction = add_action("close");
    closeAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::close));
    //auto quitAction = Gio::SimpleAction::create(QUIT_ACTION);
    //refActionGroup->add_action(quitAction);
    //quitAction->signal_activate().connect(sigc::mem_fun(*this, &SourceView::quit));
    auto pref_action = Gio::SimpleAction::create("config");
    pref_action->signal_activate().connect(
        [this]  (const Glib::VariantBase& value)
		{
			try {
				auto builder = Gtk::Builder::create();
				PrefSourceView* prefDialog;
				builder->add_from_resource(m_application->get_resource_base_path() + "/pref-srcView.ui");
				builder->get_widget_derived("PrefDialog", prefDialog, this);
				prefDialog->run();
				delete prefDialog;  // as this is a toplevel component shoud destroy -> works
			}
			catch (const Glib::Error &ex) {
                showMessage(psc::fmt::vformat(
                        _("Unable to load {} error {}"),
                          psc::fmt::make_format_args("pref-srcView", ex)), Gtk::MessageType::MESSAGE_WARNING);
			}
		});
    add_action(pref_action);
}

void
SourceView::changeLabel(SourceFile* sourceFile)
{
    for (int n = 0; n < m_notebook->get_n_pages(); ++n) {
        auto& srcFile = m_files[n];
        if (srcFile.get() == sourceFile) {
            m_notebook->set_tab_label_text(*m_notebook->get_nth_page(n), srcFile->getLabel());
            break;
        }
    }
}


void
SourceView::applyStyle(const std::string& style, const std::string& fontDesc)
{
    for (auto iter = m_files.begin(); iter != m_files.end(); ++iter) {
        auto& file = *iter;
        file->applyStyle(style, fontDesc);
    }
}


void
SourceView::newfile(const Glib::VariantBase& val)
{
    Glib::RefPtr<Gio::File> file;
    addFile(file);
}


void
SourceView::save(const Glib::VariantBase& val)
{
    if (m_notebook->get_n_pages() > 0) {
        int page = m_notebook->get_current_page();
        auto srcFile = m_files[page];
        std::cout << "SourceView::save"
                  << " page " << page
                  << " src " << srcFile->getLabel()  << std::endl;
        srcFile->save();
    }
}

void
SourceView::saveAs(const Glib::VariantBase& val)
{
    if (m_notebook->get_n_pages() > 0) {
        int page = m_notebook->get_current_page();
        auto srcFile = m_files[page];
        std::cout << "SourceView::saveAs"
                  << " page " << page
                  << " src " << srcFile->getLabel()  << std::endl;
        srcFile->saveAs();
    }
}

void
SourceView::load(const Glib::VariantBase& val)
{
    if (m_notebook->get_n_pages() > 0) {
        std::cout << "SourceView::load" << std::endl;
        int page = m_notebook->get_current_page();
        auto srcFile = m_files[page];
        srcFile->load();
    }
}

void
SourceView::close(const Glib::VariantBase& val)
{
    if (m_notebook->get_n_pages() > 0) {
        std::cout << "SourceView::close" << std::endl;
        int page = m_notebook->get_current_page();
        auto srcFile = m_files[page];
        srcFile->checkSave();
        m_notebook->remove_page(page);
        m_files.erase(m_files.begin() + page);
        if (m_files.empty()) {
            quit(val);
        }
    }
}

void
SourceView::on_hide()
{
    auto keyFile = m_application->getKeyFile();
    m_languages.saveConfig(keyFile);
    keyFile->saveConfig();
    Gtk::ApplicationWindow::on_hide();
}

void
SourceView::quit(const Glib::VariantBase& val)
{
    hide();
}

void
SourceView::showDocumentRef(SourceFile* sourceFile, const LspLocation& pos, const Glib::ustring& method)
{
    auto file = pos.getUri();
    std::cout << "SourceView::showDocumentRef"
              << " method " << method
              << " file " << file->get_path()
              << " pos " << pos.getStartLine() << ", " << pos.getStartCharacter() << std::endl;
    auto path = file->get_parent()->get_path();
    auto ccLangServIt = m_ccLangServers.find(path);
    if (ccLangServIt != m_ccLangServers.end()) {
        auto lspServer = (*ccLangServIt).second;
        if (!lspServer->getLanguage()->hasPrerequisite(file)) {
            auto preq = lspServer->getLanguage()->getPrerequisiteFile();
            Glib::ustring parts;
            parts.reserve(64);
            for (auto& pre : preq) {
                parts += pre + ";";
            }
            showMessage(psc::fmt::vformat(_("To use this functions a language server protocol implementation and a setup e.g. {} file(s) are required in {}")
                                         , psc::fmt::make_format_args(parts, path)));
            return; // no lookup possible
        }
        if (!lspServer->supportsMethod(method)) {
            showMessage(psc::fmt::vformat(_("The server did not announce support for method {}")
                                        , psc::fmt::make_format_args(method)));
            return; // no lookup possible
        }
        auto text = sourceFile->getText();
        // it might be more appropriate to do this when showing/changing tab, but then we need to support incremental changes
        auto open = std::make_shared<SourceOpen>(file, lspServer->getLanguage()->getLspLanguage(), 0, text);
        lspServer->communicate(open);

        auto lookup = std::make_shared<SourceDocumentRef>(pos, this, method);
        lspServer->communicate(lookup);

        auto close = std::make_shared<LspClose>(file);
        lspServer->communicate(close);
    }
}

void
SourceView::gotoFilePos(const LspLocation& pos)
{
    std::cout << "SourceView::gotoFilePos"
              << " file " << pos.getUri()->get_path()
              << " pos " << pos.getStartLine() << ", " << pos.getStartCharacter() << std::endl;
    Glib::RefPtr<Gio::File> file = pos.getUri();
    auto view = findView(file);
    if (!view) {
        view = addFile(file);
    }
    auto ctx = Glib::MainContext::get_default();
    ctx->signal_idle().connect_once( // delay navigation as it wont work when view was just created
        [this,view,pos] {
            m_notebook->set_current_page(getIndex(view));
            view->show(pos);
    });
}



int
SourceView::showMessage(const Glib::ustring& msg, Gtk::MessageType msgType)
{
    Gtk::ButtonsType buttons = Gtk::ButtonsType::BUTTONS_OK;
    if (msgType == Gtk::MessageType::MESSAGE_QUESTION) {
        buttons = Gtk::ButtonsType::BUTTONS_YES_NO;
    }
    Gtk::MessageDialog messagedialog(*this, msg, false, msgType, buttons);
    messagedialog.set_transient_for(*this);
    int ret = messagedialog.run();
    messagedialog.hide();
    return ret;
}

