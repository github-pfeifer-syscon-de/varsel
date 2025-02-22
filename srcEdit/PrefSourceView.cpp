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
#include <VarselConfig.hpp>
#include <gtksourceview/gtksource.h>

#include "SourceView.hpp"
#include "EditApp.hpp"
#include "PrefSourceView.hpp"

PrefSourceView::PrefSourceView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, SourceView* parent)
: Gtk::Dialog(cobject)
, m_parent{parent}
{
    auto settings = m_parent->getApplication()->getKeyFile();
    builder->get_widget("sourceStyle", m_sourceStyle);
    builder->get_widget("fontButton", m_fontButton);
    builder->get_widget("defaultFont", m_defaultFont);
    if (settings->hasKey(CONFIG_SRCVIEW_GRP, CONFIG_FONT)) {
        Glib::ustring font = settings->getString(CONFIG_SRCVIEW_GRP, CONFIG_FONT);
        m_fontButton->set_font_name(font);
    }
    m_defaultFont->set_active(settings->getBoolean(CONFIG_SRCVIEW_GRP, DEFAULT_FONT, false));

    set_transient_for(*parent);

    auto styleManager = gtk_source_style_scheme_manager_get_default();
    const gchar* const* ids = gtk_source_style_scheme_manager_get_scheme_ids(styleManager);
    for (size_t i = 0; i < 256; ++i) {
        if (ids[i] == nullptr) {
            break;
        }
        auto style = Glib::ustring(ids[i]);
        m_sourceStyle->append(style);
    }
    if (settings->hasKey(CONFIG_SRCVIEW_GRP, SOURCE_STYLE)) {
        m_sourceStyle->set_active_text(settings->getString(CONFIG_SRCVIEW_GRP, SOURCE_STYLE));
    }

    m_sourceStyle->signal_changed().connect(
            sigc::mem_fun(*this, &PrefSourceView::on_sourcestyle_select));
    m_defaultFont->signal_toggled().connect(
            sigc::mem_fun(*this, &PrefSourceView::on_sourcestyle_select));
    m_fontButton->signal_font_set().connect(
            sigc::mem_fun(*this, &PrefSourceView::on_sourcestyle_select));

}

void
PrefSourceView::on_sourcestyle_select()
{
    auto style = m_sourceStyle->get_active_text();
    std::string fontDesc;
    bool defaultFont = m_defaultFont->get_active();    // need to pass param as settings are updated later
    m_fontButton->property_sensitive() = !defaultFont;
    if (!defaultFont) {
        fontDesc = m_fontButton->get_font_name();
    }
    m_parent->applyStyle(style, fontDesc);
}


void
PrefSourceView::on_response(int response)
{
    //if (response ==  Gtk::RESPONSE_CLOSE) {
    auto settings = m_parent->getApplication()->getKeyFile();
    settings->setString(CONFIG_SRCVIEW_GRP, SOURCE_STYLE, m_sourceStyle->get_active_text());
    settings->setBoolean(CONFIG_SRCVIEW_GRP, DEFAULT_FONT, m_defaultFont->get_active());
    settings->setString(CONFIG_SRCVIEW_GRP, CONFIG_FONT, m_fontButton->get_font_name());
    //std::cout << "PrefSourceView::on_response " << m_fontButton->get_font_name() << std::endl;
    m_parent->getApplication()->save_config();
    //}
}