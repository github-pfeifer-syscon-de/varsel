/* -*- Mode: C; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2020 rpf
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
#include <KeyConfig.hpp>
#include <gtksourceview/gtksource.h>

#include "PrefDialog.hpp"
#include "VarselWin.hpp"
#include "VarselApp.hpp"


PrefDialog::PrefDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, VarselWin* parent)
: Gtk::Dialog(cobject)
, m_parent{parent}
{
    auto settings = parent->getKeyFile();
    builder->get_widget("fontButton", m_fontButton);
    builder->get_widget("defaultFont", m_defaultFont);
    if (settings->hasKey(VarselWin::CONFIG_GRP, CONFIG_FONT)) {
        Glib::ustring font = settings->getString(VarselWin::CONFIG_GRP, CONFIG_FONT);
        m_fontButton->set_font_name(font);
    }
    if (settings->hasKey(VarselWin::CONFIG_GRP, DEFAULT_FONT)) {
        m_defaultFont->set_active(settings->getBoolean(VarselWin::CONFIG_GRP, DEFAULT_FONT, false));
    }

    builder->get_widget("sourceStyle", m_sourceStyle);
    auto styleManager = gtk_source_style_scheme_manager_get_default();
    const gchar* const* ids = gtk_source_style_scheme_manager_get_scheme_ids(styleManager);
    for (size_t i = 0; i < 256; ++i) {
        if (ids[i] == nullptr) {
            break;
        }
        auto style = Glib::ustring(ids[i]);
        m_sourceStyle->append(style);
    }
    if (settings->hasKey(VarselWin::CONFIG_GRP, SOURCE_STYLE)) {
        m_sourceStyle->set_active_text(settings->getString(VarselWin::CONFIG_GRP, SOURCE_STYLE));
    }

    set_transient_for(*parent);
    m_defaultFont->signal_toggled().connect(
            sigc::mem_fun(*this, &PrefDialog::on_font_select));
    m_fontButton->signal_font_set().connect(
            sigc::mem_fun(*this, &PrefDialog::on_font_select));
    m_sourceStyle->signal_changed().connect(
            sigc::mem_fun(*this, &PrefDialog::on_sourcestyle_select));
}

void
PrefDialog::on_sourcestyle_select()
{
    VarselApp* app = m_parent->getApplication();
    auto windows = app->get_windows();
    auto style = m_sourceStyle->get_active_text();
    std::string fontDesc;
    bool defaultFont = m_defaultFont->get_active();    // need to pass param as settings are updated later
    if (!defaultFont) {
        fontDesc = m_fontButton->get_font_name();
    }
    for (size_t i = 0; i < windows.size(); ++i) {   // as there might be multiple windows
        auto sourceWindow = dynamic_cast<SourceView*>(windows[i]);
        if (sourceWindow != nullptr) {
            sourceWindow->applyStyle(style, fontDesc);
        }
    }
}

void
PrefDialog::on_font_select()
{
    bool defaultFont = m_defaultFont->get_active();    // need to pass param as settings are updated later
    m_fontButton->property_sensitive() = !defaultFont;
    std::string font;
    if (!defaultFont) {
        font = m_fontButton->get_font_name();
    }
    //std::cout << "PrefDialog::on_font_select " << font << std::endl;
    m_parent->apply_font(font);
}

void
PrefDialog::on_response(int response)
{
    //if (response ==  Gtk::RESPONSE_CLOSE) {
    auto settings = m_parent->getKeyFile();
    settings->setBoolean(VarselWin::CONFIG_GRP, DEFAULT_FONT, m_defaultFont->get_active());
    settings->setString(VarselWin::CONFIG_GRP, CONFIG_FONT, m_fontButton->get_font_name());
    settings->setString(VarselWin::CONFIG_GRP, SOURCE_STYLE, m_sourceStyle->get_active_text());
    //std::cout << "PrefDialog::on_response " << m_fontButton->get_font_name() << std::endl;
    m_parent->getApplication()->save_config();
    //}
}