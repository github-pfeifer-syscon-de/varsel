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

#include "SourceView.hpp"
#include "VarselApp.hpp"
#include "PrefDialog.hpp"

SourceFile::SourceFile(VarselApp* application)
: m_application{application}
{
}

Gtk::Widget*
SourceFile::load(std::shared_ptr<EventItem>& eventItem)
{
    auto scrollView = Gtk::make_managed<Gtk::ScrolledWindow>();
    auto file = eventItem->getFile();
    m_label = file->get_basename();
    //std::cout << "SourceView::SourceView"
    //          << " base " << base
    //          << " cont " << content << std::endl;

    // no free langManager,srcLang
    auto srcLang = getSourceLanguage(eventItem);
    if (srcLang) {
        m_buffer = gtk_source_buffer_new_with_language(srcLang);
    }
    else {
        m_buffer = gtk_source_buffer_new(nullptr);
    }
    gtk_source_buffer_set_highlight_syntax(m_buffer, true);
    m_sourceView = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(m_buffer));
    gtk_source_view_set_show_line_numbers(m_sourceView, true);
    gtk_source_view_set_show_line_marks(m_sourceView, true);

    // no free styleManager,scheme
    Glib::ustring cont = Glib::file_get_contents(file->get_path());
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(m_buffer), cont.c_str(), cont.length());   // handles utf-8
    gtk_container_add(GTK_CONTAINER(scrollView->gobj()), GTK_WIDGET(m_sourceView));
    return scrollView;
}

GtkSourceLanguage*
SourceFile::getSourceLanguage(const std::shared_ptr<EventItem>& item)
{
    GtkSourceLanguage* srcLang{nullptr};
    auto file = item->getFile();
    auto info = item->getFileInfo();
    if (info->get_file_type() == Gio::FileType::FILE_TYPE_REGULAR) {
        auto content = info->get_content_type();
        auto base = file->get_basename();
        auto langManager = gtk_source_language_manager_get_default();
        srcLang = gtk_source_language_manager_guess_language(langManager, base.c_str(), content.c_str());
    }
    return srcLang;
}

void
SourceFile::applyStyle(const std::string& style, const std::string& fontDesc)
{
    auto styleManager = gtk_source_style_scheme_manager_get_default();
    auto scheme = gtk_source_style_scheme_manager_get_scheme(styleManager, style.c_str());   //
    if (scheme && !style.empty()) {
        gtk_source_buffer_set_style_scheme(m_buffer, scheme);
    }
    if (!fontDesc.empty()) {
        // does not work
        //auto context = gtk_widget_get_pango_context(GTK_WIDGET(m_sourceView));
        //pango_context_set_font_description(context, desc.gobj());
        Pango::FontDescription desc = Pango::FontDescription(fontDesc);
        auto provider = Gtk::CssProvider::create();
        Glib::ustring size;
        if (desc.get_set_fields() & Pango::FontMask::FONT_MASK_SIZE) {
            size.reserve(16);
            size += std::to_string(static_cast<int>((float)desc.get_size() / 1024.0f));
            size += desc.get_size_is_absolute() ? "px" : "pt";
        }
        auto css = Glib::ustring::sprintf("#cssView {\n"
          "  font: %s \"%s\", Monospace;\n"
          "}", size, desc.get_family());
        //std::cout << "SourceFile::applyStyle"
        //          << fontDesc
        //          << std::hex << " fld " << desc.get_set_fields() << std::dec
        //          << " fam " << desc.get_family()
        //          << " siz " << (float)desc.get_size() / 1024.0f
        //          << " var " << desc.get_variant()
        //          << " wei " << desc.get_weight()
        //          << " sty " << desc.get_style()
        //          << " css " << css << std::endl;
        provider->load_from_data(css);
        gtk_widget_set_name(GTK_WIDGET(m_sourceView), "cssView");
        GtkStyleContext* context = gtk_widget_get_style_context(GTK_WIDGET(m_sourceView));
        gtk_style_context_add_provider(context,
                                 GTK_STYLE_PROVIDER(provider->gobj()),
                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);


    }

}

Glib::ustring
SourceFile::getLabel()
{
    return m_label;
}


SourceView::SourceView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder, VarselApp* varselApp, const std::vector<std::shared_ptr<EventItem>>& matchingFiles)
: Gtk::Window(cobject)
, m_application{varselApp}
{
    set_title(_("Source"));
    auto pix = Gdk::Pixbuf::create_from_resource(varselApp->get_resource_base_path() + "/varsel.png");
    set_icon(pix);

    refBuilder->get_widget("notebook", m_notebook);
    for (size_t n = 0; n < matchingFiles.size(); ++n) {
        auto item = matchingFiles[n];
        SourceFile sourceFile{m_application};
        auto widget = sourceFile.load(item);
        m_notebook->prepend_page(*widget, sourceFile.getLabel());
        m_files.emplace_back(std::move(sourceFile));
    }
    auto settings = m_application->getKeyFile();
    Glib::ustring fontDesc;
    if (settings->hasKey(VarselWin::CONFIG_GRP, PrefDialog::DEFAULT_FONT)) {
        if (!settings->getBoolean(VarselWin::CONFIG_GRP, PrefDialog::DEFAULT_FONT, true)
         && settings->hasKey(VarselWin::CONFIG_GRP, PrefDialog::CONFIG_FONT)) {
            fontDesc = settings->getString(VarselWin::CONFIG_GRP, PrefDialog::CONFIG_FONT);
        }
    }
    Glib::ustring style;
    if (settings->hasKey(VarselWin::CONFIG_GRP, PrefDialog::SOURCE_STYLE)) {
        style = settings->getString(VarselWin::CONFIG_GRP, PrefDialog::SOURCE_STYLE);
    }
    applyStyle(style, fontDesc);
    set_default_size(640, 480);
}

void
SourceView::applyStyle(const std::string& style, const std::string& fontDesc)
{
    for (auto iter = m_files.begin(); iter != m_files.end(); ++iter) {
        auto& file = *iter;
        file.applyStyle(style, fontDesc);
    }
}

SourceFactory::SourceFactory(VarselApp* varselApp)
: EventBusListener::EventBusListener()
, m_varselApp{varselApp}
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
            GtkSourceLanguage* srcLang = SourceFile::getSourceLanguage(item);
            if (srcLang) {
                matchingFiles.push_back(item);
                openEvent->remove(item);
            }
        }
        if (matchingFiles.size() > 0) {
            createSourceWindow(matchingFiles);
        }
    }
}


SourceView*
SourceFactory::createSourceWindow(const std::vector<std::shared_ptr<EventItem>>& matchingFiles)
{
    SourceView* sourceView{nullptr};
    auto builder = Gtk::Builder::create();
    try {
        builder->add_from_resource(m_varselApp->get_resource_base_path() + "/varsel-srcwin.ui");
        builder->get_widget_derived("SourceView", sourceView, m_varselApp, matchingFiles);
        m_varselApp->add_window(*sourceView);      // do this in this order to ensures the menu-bar comes up...
        sourceView->show_all();
    }
    catch (const Glib::Error &ex) {
        std::cerr << "Unable to load sourceView " << ex.what() << std::endl;
    }
    return sourceView;
}
