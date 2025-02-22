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
#include <VarselConfig.hpp>

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
    builder->get_widget("transparency", m_transparencyScale);
    builder->get_widget("background", m_backroundColor);
    if (settings->hasKey(VarselWin::CONFIG_GRP, CONFIG_FONT)) {
        Glib::ustring font = settings->getString(VarselWin::CONFIG_GRP, CONFIG_FONT);
        m_fontButton->set_font_name(font);
    }
    m_defaultFont->set_active(settings->getBoolean(VarselWin::CONFIG_GRP, DEFAULT_FONT, false));
    Gdk::RGBA backgrdColor = settings->getColor(VarselWin::CONFIG_GRP, CONFIG_BACKGROUND);
    m_backroundColor->set_rgba(backgrdColor);
    std::cout << "PrefDialog::PrefDialog transp " << backgrdColor.get_alpha()
              <<  " scaled " << backgrdColor.get_alpha() * TRANSPARENT_TO_ALPHA << std::endl;
    m_transparencyScale->set_value(backgrdColor.get_alpha() * TRANSPARENT_TO_ALPHA);
    set_transient_for(*parent);
    m_defaultFont->signal_toggled().connect(
            sigc::mem_fun(*this, &PrefDialog::on_value_changed));
    m_fontButton->signal_font_set().connect(
            sigc::mem_fun(*this, &PrefDialog::on_value_changed));
    m_backroundColor->signal_color_set().connect(
            sigc::mem_fun(*this, &PrefDialog::on_value_changed));
    m_transparencyScale->signal_value_changed().connect(
            sigc::mem_fun(*this, &PrefDialog::on_value_changed));
}


void
PrefDialog::on_value_changed()
{
    bool defaultFont = m_defaultFont->get_active();    // need to pass param as settings are updated later
    m_fontButton->property_sensitive() = !defaultFont;
    std::string font;
    if (!defaultFont) {
        font = m_fontButton->get_font_name();
    }
    auto transparency = static_cast<int>(m_transparencyScale->get_value());
    Gdk::RGBA backgrdColor = m_backroundColor->get_rgba();
    backgrdColor.set_alpha_u(transparency * TRANSPARENT_TO_ALPHA);
    m_parent->apply_font(font, backgrdColor);
}

void
PrefDialog::on_response(int response)
{
    //if (response ==  Gtk::RESPONSE_CLOSE) {
    auto settings = m_parent->getKeyFile();
    settings->setBoolean(VarselWin::CONFIG_GRP, DEFAULT_FONT, m_defaultFont->get_active());
    settings->setString(VarselWin::CONFIG_GRP, CONFIG_FONT, m_fontButton->get_font_name());
    auto backgrdColor = m_backroundColor->get_rgba();
    backgrdColor.set_alpha_u(static_cast<int>(m_transparencyScale->get_value() * TRANSPARENT_TO_ALPHA));
    std::cout << "PrefDialog::on_response transp " << m_transparencyScale->get_value()
              <<  " scaled " << m_transparencyScale->get_value() * TRANSPARENT_TO_ALPHA << std::endl;
   settings->setColor(VarselWin::CONFIG_GRP, CONFIG_BACKGROUND, backgrdColor);
    //std::cout << "PrefDialog::on_response " << m_fontButton->get_font_name() << std::endl;
    m_parent->getApplication()->save_config();
    //}
}